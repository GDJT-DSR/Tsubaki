#include "file_list.h"
#include "logger.h"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

using namespace plugins;

bool FileList::add(std::string file) {
    if (auto it = m_filesums.find(file); it != m_filesums.end()) {
        return false;
    }
    m_filesums[file].hash = "<NONE>";
    return true;
}

void FileList::set(std::string_view file, std::string_view hash) {
    m_filesums[std::string(file)].hash = hash;
}
std::optional<std::string_view> FileList::getHash(const std::string &file) {
    auto it = m_filesums.find(file);
    if (it == m_filesums.end()) {
        return std::nullopt;
    }
    return it->second.hash;
}
// std::unordered_set<std::string_view>
// FileList::getFilesByHash(const std::string &hash) {
//     return m_file_by_sums[hash];
// }

void FileList::initSizes() {
    const auto &logger = Logger::GetInstance();
    for (auto it = m_filesums.begin(); it != m_filesums.end();) {
        std::error_code ec;
        it->second.size = std::filesystem::file_size(it->first, ec);
        if (ec) {
            logger(LogLevel::WARN, "Get size of file {} error. Skipped.",
                   it->first);
            it = m_filesums.erase(it);
        } else {
            ++it;
        }
    }
}
uintmax_t FileList::getTotalSize(bool b) {
    uintmax_t ret = 0;
    const auto &logger = Logger::GetInstance();
    for (const auto &[key, val] : m_filesums) {
        if (!b || val.hash.empty() || val.hash.front() == '<')
            ret += val.size;
        // logger(LogLevel::INFO, "{} {}", val.hash, val.size);
    }
    return ret;
}
