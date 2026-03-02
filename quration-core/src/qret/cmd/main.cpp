/**
 * @file qret/cmd/main.cpp
 * @brief Define qret.
 */

#include "qret/cmd/main.h"

#include <boost/program_options.hpp>

#include <algorithm>
#include <cassert>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>

#include "qret/base/log.h"
#include "qret/cmd/asm.h"
#include "qret/cmd/common.h"
#include "qret/cmd/compile.h"
#include "qret/cmd/diagram.h"
#include "qret/cmd/opt.h"
#include "qret/cmd/parse.h"
#include "qret/cmd/print.h"
#include "qret/cmd/profile.h"
#include "qret/cmd/simulate.h"
#include "qret/version.h"

namespace qret::cmd {
std::vector<std::unique_ptr<SubCommand>> GetSubCommands() {
    auto ret = std::vector<std::unique_ptr<SubCommand>>();
    ret.emplace_back(std::unique_ptr<CommandAsm>(new CommandAsm()));
    ret.emplace_back(std::unique_ptr<CommandCompile>(new CommandCompile()));
    ret.emplace_back(std::unique_ptr<CommandDiagram>(new CommandDiagram()));
    ret.emplace_back(std::unique_ptr<CommandOpt>(new CommandOpt()));
    ret.emplace_back(std::unique_ptr<CommandParse>(new CommandParse()));
    ret.emplace_back(std::unique_ptr<CommandPrint>(new CommandPrint()));
    ret.emplace_back(std::unique_ptr<CommandProfile>(new CommandProfile()));
    ret.emplace_back(std::unique_ptr<CommandSimulate>(new CommandSimulate()));
    std::ranges::sort(ret.begin(), ret.end(), [](const auto& lhs, const auto& rhs) {
        return lhs->Name() < rhs->Name();
    });
    return ret;
}

ReturnStatus ShowHelp(const std::vector<std::unique_ptr<SubCommand>>& sub_commands) {
    std::cout << "-- Quration: Quantum Resource Estimation Toolchain for FTQC (version "
              << QRET_VERSION_MAJOR << "." << QRET_VERSION_MINOR << "." << QRET_VERSION_PATCH
              << ") --" << std::endl
              << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  qret [command] [options]" << std::endl << std::endl;
    std::cout << "Example usage:" << std::endl;
    std::cout << "  qret help [ -h, --help ]" << std::endl;
    std::cout << "  qret version [ -v, --version ]" << std::endl;
    std::cout << "  qret compile -h" << std::endl;
    std::cout << "  qret simulate -h" << std::endl;
    std::cout << "Available commands:" << std::endl;
    std::cout << "  help version";
    for (auto&& command : sub_commands) {
        std::cout << " " << command->Name();
    }
    std::cout << std::endl;
    return ReturnStatus::Success;
}

ReturnStatus ShowVersion() {
    std::cout << "Quration: Quantum Resource Estimation Toolchain for FTQC version "
              << QRET_VERSION_MAJOR << "." << QRET_VERSION_MINOR << "." << QRET_VERSION_PATCH
              << std::endl;
    return ReturnStatus::Success;
}

ReturnStatus QretMainImpl(int argc, const char** argv) {
    auto sub_commands = GetSubCommands();

    if (argc == 1) {
        return ShowHelp(sub_commands);
    }

    const auto command = std::string(argv[1]);
    if (command == "help" || command == "-h" || command == "--help") {
        return ShowHelp(sub_commands);
    } else if (command == "version" || command == "-v" || command == "--version") {
        return ShowVersion();
    } else {
        for (auto& sub_command : sub_commands) {
            if (command != sub_command->Name()) {
                continue;
            }

            return sub_command->Main(argc - 1, argv + 1);
        }
    }

    // `command` is not found in sub_commands.
    throw std::invalid_argument(
            fmt::format(
                    "unrecognised command '{}'\n"
                    "To get the list of available commands, run 'qret help'.",
                    command
            )
    );
}

int QretMain(int argc, const char** argv) noexcept {
    namespace po = boost::program_options;

    try {
        Logger::DisableColorfulOutput();
        Logger::DisableFileOutput();
        Logger::DisableConsoleOutput();
        const auto ret = QretMainImpl(argc, argv);
        return static_cast<int>(ret);
    } catch (const po::error_with_option_name& ex) {
        std::cerr << ex.what() << std::endl;
        std::cerr << "To get the list of available options, run 'qret " << argv[1] << " --help'."
                  << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }

    return 1;
}
}  // namespace qret::cmd
