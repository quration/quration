/**
 * @file qret/cmd/parse.cpp
 * @brief Define 'parse' sumcommand in qret.
 */

#include "qret/cmd/parse.h"

#include <boost/program_options.hpp>
#include <fmt/format.h>

#include <iostream>

#include "qret/base/json.h"
#include "qret/base/log.h"
#include "qret/cmd/common.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/openqasm2.h"
#include "qret/ir/context.h"
#include "qret/ir/function.h"
#include "qret/ir/json.h"  // DO NOT DELETE
#include "qret/ir/module.h"
#include "qret/parser/openqasm2.h"
#include "qret/parser/openqasm3.h"

namespace qret::cmd {
ReturnStatus ParseOpenQASM2(const std::string& input, const std::string& output) {
    LOG_INFO("Construct OpenQASM2 ast.");
    auto ast = qret::openqasm2::ParseOpenQASM2File(input);

    LOG_INFO("Build IR from OpenQASM2 ast.");
    qret::ir::IRContext context;
    auto* module = qret::ir::Module::Create("OpenQASM2", context);
    auto builder = qret::frontend::CircuitBuilder(module);
    qret::frontend::BuildCircuitFromAST(ast, builder);

    LOG_INFO("Save IR.");
    auto ofs = std::ofstream(output);
    if (!ofs.good()) {
        std::cerr << "failed to open: " << output << std::endl;
        return ReturnStatus::Failure;
    }

    auto j = qret::Json();
    j = *module;
    ofs << j << std::endl;
    ofs.close();

    return ReturnStatus::Success;
}
ReturnStatus ParseOpenQASM3(const std::string& input, const std::string& output) {
    LOG_INFO("Construct OpenQASM3 ast.");
    auto ast = qret::openqasm3::ParseOpenQASM3File(input);

    LOG_INFO("Build IR from OpenQASM3 ast.");
    qret::ir::IRContext context;
    auto* module = qret::ir::Module::Create("OpenQASM3", context);
    auto builder = qret::frontend::CircuitBuilder(module);
    qret::frontend::BuildCircuitFromAST(ast, builder);

    LOG_INFO("Save IR.");
    auto ofs = std::ofstream(output);
    if (!ofs.good()) {
        std::cerr << "failed to open: " << output << std::endl;
        return ReturnStatus::Failure;
    }

    auto j = qret::Json();
    j = *module;
    ofs << j << std::endl;
    ofs.close();

    return ReturnStatus::Success;
}
ReturnStatus Parse(const std::string& input, const std::string& output, const std::string& format) {
    if (format == "OpenQASM2") {
        return ParseOpenQASM2(input, output);
    } else if (format == "OpenQASM3") {
        return ParseOpenQASM3(input, output);
    }
    std::cerr << "unknown format: " << format << ". expected OpenQASM2 or OpenQASM3" << std::endl;
    return ReturnStatus::Failure;
}
ReturnStatus CommandParse::Main(int argc, const char** argv) {
    namespace po = boost::program_options;

    // Define description.
    // clang-format off
    auto description = po::options_description(R"(qret 'parse' options)");
    description.add_options()
        ("help,h", "Show this help and exit.")
        ("quiet", "Suppress non-error output.")
        ("verbose", "Enable verbose logging (more detail than default).")
        ("debug", "Enable debug logging (most detailed; implies --verbose).")
        ("color", "Enable colored output.")
        ("input,i", po::value<std::string>(), "Input file")
        ("output,o", po::value<std::string>()->default_value("ir.json"), "Output file")
        ("format,f", po::value<std::string>()->default_value("OpenQASM2"), "Format of input file ('OpenQASM2' or 'OpenQASM3'). OpenQASM3 is a compatibility subset.")
    ; // NOLINT
    // clang-format on

    auto vm = po::variables_map();
    try {
        po::store(po::parse_command_line(argc, argv, description), vm);
        po::notify(vm);
    } catch (const po::error_with_option_name& ex) {
        std::cerr << ex.what() << std::endl;
        std::cerr << "To get the list of available options, run 'qret parse --help'." << std::endl;
        return ReturnStatus::Failure;
    }

    if (vm.count("help") > 0) {
        std::cout << description;
        return ReturnStatus::Success;
    }
    if (vm.count("quiet") > 0) {
        qret::Logger::DisableConsoleOutput();
        qret::Logger::DisableFileOutput();
    } else if (vm.count("debug") > 0) {
        qret::Logger::EnableConsoleOutput();
        qret::Logger::SetLogLevel(qret::LogLevel::Debug);
    } else if (vm.count("verbose") > 0) {
        qret::Logger::EnableConsoleOutput();
        qret::Logger::SetLogLevel(qret::LogLevel::Info);
    } else {
        qret::Logger::EnableConsoleOutput();
        qret::Logger::SetLogLevel(qret::LogLevel::Warn);
    }
    if (vm.count("color") > 0) {
        qret::Logger::EnableColorfulOutput();
    } else {
        qret::Logger::DisableColorfulOutput();
    }

    if (vm.count("input") == 0) {
        std::cerr << "missing required option: --input <file>" << std::endl;
        return ReturnStatus::Failure;
    }

    const auto input = vm["input"].as<std::string>();
    const auto output = vm["output"].as<std::string>();
    const auto format = vm["format"].as<std::string>();

    return Parse(input, output, format);
}
}  // namespace qret::cmd
