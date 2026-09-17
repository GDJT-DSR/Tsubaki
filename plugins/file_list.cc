#include "file_list.h"
#include "logger.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

using namespace plugins;

bool FileList::add(std::string file) {
    auto [it, inserted] = m_filesums.try_emplace(std::move(file));
    if (!inserted)
        return false;
    it->second.hash = "<NONE>";
    return true;
}

bool FileList::add(std::string file, uintmax_t size) {
    auto [it, inserted] = m_filesums.try_emplace(std::move(file));
    if (!inserted)
        return false;
    it->second.hash = "<NONE>";
    it->second.size = size;
    return true;
}

void FileList::set(std::string_view file, std::string_view hash) {
    m_filesums[std::string(file)].hash = hash;
}

void FileList::initSizes() {
    const auto &logger = Logger::GetInstance();

    std::vector<const std::string *> pending;
    for (const auto &[key, val] : m_filesums) {
        if (val.size == unknown_size)
            pending.push_back(&key);
    }
    if (pending.empty())
        return;

    const size_t count = pending.size();
    std::vector<uintmax_t> sizes(count);
    std::vector<std::error_code> errors(count);

    const size_t threads = std::max(1u, std::thread::hardware_concurrency());
    const size_t chunk = (count + threads - 1) / threads;

    std::vector<std::thread> workers;
    workers.reserve(threads);
    for (size_t t = 0; t < threads; ++t) {
        const size_t begin = t * chunk;
        if (begin >= count)
            break;
        const size_t end = std::min(count, begin + chunk);
        workers.emplace_back([&, begin, end] {
            for (size_t i = begin; i < end; ++i) {
                sizes[i] = std::filesystem::file_size(*pending[i], errors[i]);
            }
        });
    }
    for (auto &worker : workers)
        worker.join();

    for (size_t i = 0; i < count; ++i) {
        if (errors[i]) {
            logger(LogLevel::WARN, "Get size of file {} error. Skipped.",
                   *pending[i]);
            m_filesums.erase(*pending[i]);
        } else {
            m_filesums[*pending[i]].size = sizes[i];
        }
    }
}

uintmax_t FileList::getTotalSize(bool skip_computed) {
    uintmax_t ret = 0;
    for (const auto &[key, val] : m_filesums) {
        if (val.size == unknown_size)
            continue;
        if (!skip_computed || val.hash.empty() || val.hash.front() == '<')
            ret += val.size;
    }
    return ret;
}
