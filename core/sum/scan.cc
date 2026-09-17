#include "scan.h"
#include "common/common.h"
#include "plugins/arg_parser.h"
#include "plugins/file_list.h"
#include "plugins/logger.h"
#include "sum/filter.h"
#include <filesystem>
#include <iostream>
#include <system_error>

namespace fs = std::filesystem;
using namespace sum;
using namespace plugins;
void scanFile(std::string_view path, FileList &fl) {
    fl.add(fs::absolute(path).lexically_normal().string());
}
void scanDictionary(std::string_view path_str, FileList &fl) {
    const auto &parser = ArgParser::GetInstance();
    const auto &logger = Logger::GetInstance();
    // fs::path path{path_str};
    std::error_code ec;
    fs::path path = fs::canonical(path_str, ec);

    if (ec) {
        logger(plugins::LogLevel::WARN, "Dictionary {} not found. Skipped.",
               path_str);
    }

    bool allow_symlinks = parser.getValue("--allow-symlinks") != nullptr;
    auto options = std::filesystem::directory_options::skip_permission_denied;
    if (allow_symlinks)
        options |= std::filesystem::directory_options::follow_directory_symlink;
    try {
        for (const auto &entry :
             std::filesystem::recursive_directory_iterator(path, options)) {
            if (std::filesystem::is_regular_file(entry.status()) ||
                (allow_symlinks &&
                 std::filesystem::is_symlink(entry.status()))) {
                fl.add(entry.path().string());
            }
        }
    } catch (const std::filesystem::filesystem_error &msg) {
        // throw std::runtime_error(
        //     "Fatal: Filesystem error while traversing " + dir_path + ": "
        //     + msg.what());
        logger(LogLevel::ERROR, "filesystem error while traversing {} : {}",
               path_str, msg.what());
    }
}

void scanStdinList(FileList &fl) {
    const auto &logger = Logger::GetInstance();
    logger(plugins::LogLevel::INFO,
           "--> SUM: Loading file list from stdin ...");
}

void sum::scan(std::string_view input, FileList &list) {
    const auto &logger = plugins::Logger::GetInstance();
    if (input == "stdin") {
        logger(plugins::LogLevel::INFO, "SUM: Will load file list (tsubaki "
                                        "file list format) from stdin...\n");
        common::loadFilesFromStream(list, std::cin, "stdin", "SUM");
    } else if (input == "stdin-plain-list") {
        logger(plugins::LogLevel::INFO,
               "SUM: Will load file list (plain list format) from stdin...");
        scanStdinList(list);
    } else {
        if (auto status = fs::status(input); fs::is_regular_file(status)) {
            logger(
                plugins::LogLevel::INFO,
                "SUM: {} is a regular file.\nCaculating {} hash of this file.",
                input, plugins::ArgParser::GetInstance().getCommands()[1]);
            scanFile(input, list);
        } else if (fs::is_directory(status)) {

            logger(plugins::LogLevel::INFO,
                   "SUM: {} is a dictionary.\nCaculating {} hash of all files "
                   "in the dictionary.",
                   input, plugins::ArgParser::GetInstance().getCommands()[1]);
            scanDictionary(input, list);
        } else {
            logger(plugins::LogLevel::WARN,
                   "SUM: {} is not a regular file or a directory; it will be "
                   "skiped.",
                   input);
            return;
        }
    }
    sum::filter(list);
}
