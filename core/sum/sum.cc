#include "sum/sum.h"
#include "plugins/arg_parser.h"
#include "plugins/encoder.h"
#include "plugins/file_list.h"
#include "plugins/logger.h"
#include "plugins/thread_pool.h"
#include "scan.h"
#include "sum/filter.h"
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string_view>
#include <sys/signal.h>
#include <sys/types.h>
#include <tuple>
#include <vector>
namespace {
std::string formatFileSize(uintmax_t bytes) {
    static const char *units = " KMGTPE";
    const int unitCount = strlen(units) / sizeof(units[0]);

    if (bytes == 0) {
        return "0 B";
    }

    int unitIndex = static_cast<int>(std::log2(bytes) / 10.0);
    if (unitIndex >= unitCount) {
        unitIndex = unitCount - 1;
    }

    double size = static_cast<double>(bytes) / std::pow(1024.0, unitIndex);

    return std::format("{:.2f} {}B", size, units[unitIndex]);
}

std::tuple<int, int> calc(plugins::ThreadPool &pool, plugins::FileList &fl,
                          const plugins::Logger &logger) {
    int success = 0;
    int error = 0;
    pool.initAndStart();
    for (auto &[key, val] : *fl) {
        if (!val.fut.valid()) {
            continue;
        }
        try {
            auto hash = val.fut.get();
            std::cout << hash << ' ' << key << '\n';
            ++success;
        } catch (plugins::ThreadPool::exceptions e) {
            std::cout << "<NONE> " << key << '\n';
        } catch (...) {
            logger(plugins::LogLevel::WARN,
                   "Error occured when calculating the sum of {}", key);
            std::cout << "<NONE> " << key << '\n';
            ++error;
        }
    }
    pool.stop();
    return {success, error};
}

void sig(int s) {
    if (s == SIGINT) {
        plugins::ThreadPool::GetInstance().stop();
    }
}

} // namespace

void sum::invoke() {
    const auto &parser = plugins::ArgParser::GetInstance();
    const auto &encoder = plugins::Encoder::GetInstance();
    const auto &logger = plugins::Logger::GetInstance();

    const auto &commands = parser.getCommands();

    if (commands.size() < 2) {
        logger(plugins::LogLevel::ERROR, "At least 2 parameters needed.");
        return;
    }

    std::string_view sum_type = commands.front();
    const EVP_MD *md = encoder.getMdByName(sum_type);
    if (!md) {
        logger(plugins::LogLevel::ERROR, "the sum type {} is not supported.",
               sum_type);
    }

    logger(plugins::LogLevel::INFO, "===SUM [SCAN]===");

    plugins::FileList list;
    for (auto it = commands.begin() + 1; it != commands.end(); ++it) {
        scan(*it, list);
    }

    logger(plugins::LogLevel::INFO, "");
    logger(plugins::LogLevel::INFO, "===SUM [FILTER]===");
    logger(plugins::LogLevel::INFO, "");

    sum::filter(list);

    if (list->empty()) {
        logger(plugins::LogLevel::INFO,
               "==>SUM: No files matching the requirements were found. "
               "Calculation will not be started.");
        return;
    }
    logger(plugins::LogLevel::INFO,
           "-->SUM: Eventually {} regular files were loaded.", list->size());
    logger(plugins::LogLevel::INFO, "");
    logger(plugins::LogLevel::INFO, "===SUM [CONFIG]===");
    logger(plugins::LogLevel::INFO, "");
    logger(plugins::LogLevel::INFO, "-->SUM: Force-rescan is {}.",
           (parser.getValue("--force-scan") ? "on" : "off"));

    bool skip_if_computed = !parser.getValue("--force-scan");
    uintmax_t size = list.getTotalSize(skip_if_computed);
    logger(plugins::LogLevel::INFO, "-->SUM: Total size is {} bytes ({})", size,
           formatFileSize(size));
    logger(plugins::LogLevel::INFO, "");
    logger(plugins::LogLevel::INFO, "===SUM [PROGRESS]===");
    logger(plugins::LogLevel::INFO, "");
    if (parser.getValue("--test")) {
        logger(plugins::LogLevel::INFO,
               "-->SUM: Calculation will not be started for argument: --test.");
    }
    logger(plugins::LogLevel::INFO, "-->SUM: Calculating checksums...");

    // 使用线程池
    auto &pool = plugins::ThreadPool::GetInstance();

    for (auto &[key, val] : *list) {
        if (!val.hash.empty() && val.hash.front() != '<') {
            if (skip_if_computed) {
                std::cout << val.hash << '\n';
                continue;
            }
            val.hash.clear();
        }
        val.fut = pool.submit<std::string>(
            [md, path = std::string_view(key), size = val.size]() {
                return plugins::Encoder::encodeFile(path, size, md);
            });
    }

    signal(SIGINT, &sig);
    auto start_time = std::chrono::system_clock::now();
    auto start_at = std::chrono::steady_clock::now();
    auto [succeed, error] = calc(pool, list, logger);
    auto end_at = std::chrono::steady_clock::now();
    auto end_time = std::chrono::system_clock::now();
    std::cout << std::format("# \n# ----------General Report----------\n"
                             "# Total: {}\n"
                             "# Succeed: {}\n"
                             "# Failed: {}\n"
                             "# Unprocessed: {}\n"
                             "# Time started: {:%Y-%m-%d %H:%M:%S}\n"
                             "# Time finished: {:%Y-%m-%d %H:%M:%S}\n"
                             "# Duration: {}\n"
                             "# Command: {}",

                             list->size(), succeed, error,
                             list->size() - succeed - error, start_time,
                             end_time, end_at - start_at, parser.getCommand())
              << std::endl;
}
