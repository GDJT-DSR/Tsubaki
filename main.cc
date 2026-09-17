#include "arg_parser.h"
#include "common/common.h"
#include "common/help.h"
#include "logger.h"
#include "platform.h"
#include "sum/sum.h"
#include <string_view>

int main(int argc, char *argv[]) {
    plugins::platform::enableUtf8Console();
    plugins::ArgParser &parser = plugins::ArgParser::GetInstance();
    const auto &logger = plugins::Logger::GetInstance();
    parser.parse(argc, argv);
    common::setLogLevel();

    if (parser.getValue("-h") || parser.getValue("--help")) {
        common::printHelp(false);
        return 0;
    }
    if (parser.getValue("--help-cn")) {
        common::printHelp(true);
        return 0;
    }

    const auto &commands = parser.getCommands();
    if (commands.empty()) {
        logger(plugins::LogLevel::ERROR,
               "At least one command is required.\nType 'tsubaki --help' for "
               "usage.");
        return 1;
    }

    const std::string_view command = commands.front();
    if (command == "help") {
        common::printHelp(false);
        return 0;
    }
    if (command == "sum") {
        return sum::invoke();
    }

    logger(plugins::LogLevel::ERROR,
           "Unknown command '{}'. Type 'tsubaki --help' for usage.", command);
    return 1;
}
