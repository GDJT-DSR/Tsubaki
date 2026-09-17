#include "sum/sum.h"
#include "common/common.h"
#include "plugins/arg_parser.h"
#include "plugins/encoder.h"
#include "plugins/file_list.h"
#include "plugins/logger.h"
#include "plugins/progress_bar.h"
#include "plugins/thread_pool.h"
#include "scan.h"
#include "sum/filter.h"
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <format>
#include <future>
#include <iostream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>
namespace {

// 仅由信号处理函数写入、主线程读取，必须为 sig_atomic_t
volatile std::sig_atomic_t g_interrupted = 0;

std::tuple<int, int, bool> calc(plugins::ThreadPool &pool,
                                plugins::FileList &fl,
                                const plugins::Logger &logger,
                                plugins::ProgressBar &bar,
                                uintmax_t total_bytes) {
    int success = 0;
    int error = 0;
    bool interrupted = false;
    std::size_t done = 0;
    uintmax_t bytes_done = 0;

    auto tick = [&]() {
        ++done;
        bar.update(done,
                   std::format("{} / {}", common::formatFileSize(bytes_done),
                               common::formatFileSize(total_bytes)));
    };

    // 输出单个文件的结果并推进进度
    auto emit = [&](const std::string &key, plugins::FileList::FileInfo &val) {
        bar.clear();
        if (!val.fut.valid()) {
            if (!val.hash.empty() && val.hash.front() != '<') {
                std::cout << val.hash << ' ' << key << '\n';
                ++success;
            } else {
                std::cout << "<NONE> " << key << '\n';
                ++error;
            }
        } else {
            try {
                auto hash = val.fut.get();
                std::cout << hash << ' ' << key << '\n';
                ++success;
            } catch (plugins::ThreadPool::exceptions) {
                std::cout << "<NONE> " << key << '\n';
            } catch (...) {
                logger(plugins::LogLevel::WARN,
                       "Error occured when calculating the sum of {}", key);
                std::cout << "<NONE> " << key << '\n';
                ++error;
            }
        }
        if (val.size != plugins::FileList::unknown_size) {
            bytes_done += val.size;
        }
        tick();
    };

    // 主线程等待单个文件的最长时间；超过则先跳过，稍后回读。
    constexpr auto kWaitTimeout = std::chrono::milliseconds(300);
    constexpr auto kPollInterval = std::chrono::milliseconds(100);

    // 第一遍超时跳过的文件暂存于此，待全部遍历结束后再统一处理。
    std::vector<std::pair<const std::string *, plugins::FileList::FileInfo *>>
        pending;

    pool.initAndStart();
    for (auto &[key, val] : fl) {
        if (g_interrupted) {
            interrupted = true;
            break;
        }
        if (!val.fut.valid()) {
            emit(key, val);
            continue;
        }
        if (val.fut.wait_for(kWaitTimeout) == std::future_status::ready) {
            emit(key, val);
        } else {
            pending.emplace_back(&key, &val);
        }
    }

    // 第二遍：回读被跳过的文件，此时大多已完成。
    for (auto &[key, val] : pending) {
        if (g_interrupted) {
            interrupted = true;
            break;
        }
        while (val->fut.wait_for(kPollInterval) !=
               std::future_status::ready) {
            if (g_interrupted) {
                interrupted = true;
                break;
            }
        }
        if (interrupted) {
            break;
        }
        emit(*key, *val);
    }

    bar.finish();
    pool.stop();
    return {success, error, interrupted};
}

// 信号处理函数只能执行异步信号安全操作，因此仅置标志位，
// 实际的线程池停止由主线程在 calc 返回后完成。
void sig(int) { g_interrupted = 1; }

} // namespace

int sum::invoke() {
    const auto &parser = plugins::ArgParser::GetInstance();
    const auto &encoder = plugins::Encoder::GetInstance();
    const auto &logger = plugins::Logger::GetInstance();

    const auto &commands = parser.getCommands();

    if (commands.size() < 2) {
        logger(plugins::LogLevel::ERROR,
               "Missing checksum algorithm.\nUsage: tsubaki sum <algorithm> "
               "<path...>");
        return 1;
    }
    if (commands.size() < 3) {
        logger(plugins::LogLevel::ERROR,
               "Missing input path.\nUsage: tsubaki sum <algorithm> <path...>");
        return 1;
    }

    std::signal(SIGINT, &sig);

    std::string_view sum_type = commands[1];
    const EVP_MD *md = encoder.getMdByName(sum_type);
    if (!md) {
        logger(plugins::LogLevel::WARN,
               "the sum type {} is not supported. All results will be recorded "
               "as <NONE>.",
               sum_type);
    }

    logger(plugins::LogLevel::INFO, "===SUM [SCAN]===");

    plugins::FileList list;
    for (auto it = commands.begin() + 2; it != commands.end(); ++it) {
        scan(*it, list);
    }

    logger(plugins::LogLevel::INFO, "");
    logger(plugins::LogLevel::INFO, "===SUM [FILTER]===");
    logger(plugins::LogLevel::INFO, "");

    sum::filter(list);

    if (list.empty()) {
        logger(plugins::LogLevel::INFO,
               "==>SUM: No files matching the requirements were found. "
               "Calculation will not be started.");
        return md ? 0 : 1;
    }
    logger(plugins::LogLevel::INFO,
           "-->SUM: Eventually {} regular files were loaded.", list.size());
    logger(plugins::LogLevel::INFO, "");
    logger(plugins::LogLevel::INFO, "===SUM [CONFIG]===");
    logger(plugins::LogLevel::INFO, "");
    logger(plugins::LogLevel::INFO, "-->SUM: Force-rescan is {}.",
           (parser.getValue("--force-scan") ? "on" : "off"));

    bool skip_if_computed = !parser.getValue("--force-scan");
    uintmax_t size = list.getTotalSize(skip_if_computed);
    logger(plugins::LogLevel::INFO, "-->SUM: Total size is {} bytes ({})", size,
           common::formatFileSize(size));
    logger(plugins::LogLevel::INFO, "");
    logger(plugins::LogLevel::INFO, "===SUM [PROGRESS]===");
    logger(plugins::LogLevel::INFO, "");
    if (parser.getValue("--test")) {
        logger(plugins::LogLevel::INFO,
               "-->SUM: Calculation will not be started for argument: --test.");
        return 0;
    }
    logger(plugins::LogLevel::INFO, "-->SUM: Calculating checksums...");

    // 使用线程池。必须在提交任务前启动工作线程：队列有界，
    // 若线程池未启动，提交超过队列容量的任务会一直等待空位而死锁。
    auto &pool = plugins::ThreadPool::GetInstance();
    pool.initAndStart();

    for (auto &[key, val] : list) {
        if (g_interrupted) {
            break;
        }
        if (!md) {
            val.hash = "<NONE>";
            continue;
        }
        if (!val.hash.empty() && val.hash.front() != '<') {
            if (skip_if_computed) {
                continue;
            }
            val.hash.clear();
        }
        val.fut = pool.submit(
            [md, path = std::string_view(key), size = val.size]() {
                return plugins::Encoder::encodeFile(path, size, md);
            });
    }

    // 默认仅在 stderr 为终端、终端足够宽且日志级别不高于 INFO 时启用，
    // 可用 --progress / --no-progress 强制覆盖。
    // 终端过窄时进度条会换行并破坏 '\r' 原地刷新，因此自动禁用该功能。
    bool progress_enabled;
    if (parser.getValue("--no-progress")) {
        progress_enabled = false;
    } else if (parser.getValue("--progress")) {
        progress_enabled = true;
    } else {
        progress_enabled = logger.m_min_level <= plugins::LogLevel::INFO &&
                           plugins::ProgressBar::terminalIsWideEnough();
    }
    plugins::ProgressBar bar(list.size(), std::cerr, progress_enabled);

    auto start_time = std::chrono::system_clock::now();
    auto start_at = std::chrono::steady_clock::now();
    auto [succeed, error, interrupted] =
        calc(pool, list, logger, bar, list.getTotalSize(false));
    auto end_at = std::chrono::steady_clock::now();
    auto end_time = std::chrono::system_clock::now();

    const std::string interrupted_line =
        interrupted ? "# Interrupted: yes\n" : "";
    std::cout << std::format("# \n# ----------General Report----------\n"
                             "# Total: {}\n"
                             "# Succeed: {}\n"
                             "# Failed: {}\n"
                             "# Unprocessed: {}\n"
                             "{}"
                             "# Time started: {:%Y-%m-%d %H:%M:%S}\n"
                             "# Time finished: {:%Y-%m-%d %H:%M:%S}\n"
                             "# Duration: {}\n"
                             "# Command: {}",

                             list.size(), succeed, error,
                             list.size() - succeed - error, interrupted_line,
                             start_time, end_time, end_at - start_at,
                             parser.getCommand())
              << std::endl;

    if (interrupted) {
        return 130;
    }
    return error > 0 ? 1 : 0;
}
