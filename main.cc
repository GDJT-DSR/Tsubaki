#include "arg_parser.h"
#include "common/common.h"
#include "logger.h"
#include "sum/sum.h"

int main(int argc, char *argv[]) {
    plugins::ArgParser &parser = plugins::ArgParser::GetInstance();
    const auto &logger = plugins::Logger::GetInstance();
    parser.parse(argc, argv);
    common::setLogLevel();
    if (parser.getCommands().empty()) {

        // std::cerr << "==>Error: At least one argument is required.\nType "
        //              "'tsubaki help' for usage."
        // k
        //           << std::endl;
        logger(plugins::LogLevel::ERROR,
               "At least one command is required.\nType 'tsubaki help' for "
               "usage.");
        return 1;
    }
    sum::invoke();

    return 0;
}
