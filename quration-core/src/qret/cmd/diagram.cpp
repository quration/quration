/**
 * @file qret/cmd/diagram.cpp
 * @brief Define 'diagram' sumcommand in qret.
 */

#include "qret/cmd/diagram.h"

#include <boost/program_options.hpp>
#include <fmt/format.h>

#include <algorithm>
#include <iostream>
#include <vector>

#include "qret/analysis/visualizer.h"
#include "qret/base/json.h"
#include "qret/base/log.h"
#include "qret/cmd/common.h"
#include "qret/ir/context.h"
#include "qret/ir/function.h"
#include "qret/ir/json.h"
#include "qret/ir/module.h"

namespace qret::cmd {
ReturnStatus Diagram(
        const std::string& input,
        const std::string& function_name,
        const std::string& output,
        const std::string& format,
        bool display_num_calls
) {
    // Load the input json, which is a serialized IR module.
    LOG_INFO("Load IR.");
    auto ifs = std::ifstream(input);
    auto j = qret::Json::parse(ifs);

    // Deserialize IR module from json
    LOG_INFO("Find function.");
    qret::ir::IRContext context;
    qret::ir::LoadJson(j, context);
    const auto* func = context.owned_module.back()->GetFunction(function_name);
    if (func == nullptr) {
        std::cerr << "function of name '" << function_name << "' not found" << std::endl;
        std::cerr << "available functions:" << std::endl;
        const auto* module = context.owned_module.back().get();
        auto names = std::vector<std::string>{};
        for (const auto& function : *module) {
            names.emplace_back(function.GetName());
        }
        std::sort(names.begin(), names.end());
        for (const auto& name : names) {
            std::cerr << "  - " << name << std::endl;
        }
        return ReturnStatus::Failure;
    }

    LOG_INFO("Draw diagram.");
    auto diagram = std::string();
    if (format == "CFG") {
        diagram = GenCFG(*func);
    } else if (format == "CallGraph") {
        diagram = GenCallGraph(*func, display_num_calls);
    } else if (format == "LaTeX") {
        diagram = GenLaTeX(*func);
    } else if (format == "ComputeGraph") {
        diagram = GenComputeGraph(*func);
    } else {
        std::cerr << "unknown format: " << format
                  << ". expected CFG, CallGraph, LaTeX, or ComputeGraph" << std::endl;
        return ReturnStatus::Failure;
    }

    LOG_INFO("Save diagram.");
    auto ofs = std::ofstream(output);
    if (!ofs.good()) {
        std::cerr << "failed to open: " << output << std::endl;
        return ReturnStatus::Failure;
    }
    ofs << diagram;
    ofs.close();

    return ReturnStatus::Success;
}
ReturnStatus CommandDiagram::Main(int argc, const char** argv) {
    namespace po = boost::program_options;

    // Define description.
    // clang-format off
    auto description = po::options_description(R"(qret 'diagram' options)");
    description.add_options()
        ("help,h", "Show this help and exit.")
        ("quiet", "Suppress non-error output.")
        ("verbose", "Enable verbose logging (more detail than default).")
        ("debug", "Enable debug logging (most detailed; implies --verbose).")
        ("color", "Enable colored output.")
        ("input,i", po::value<std::string>(), "Input file")
        ("function", po::value<std::string>(), "Function name to draw")
        ("output,o", po::value<std::string>(), "Output file (default is 'out.dot' or 'out.tex')")
        ("graph-format,g", po::value<std::string>(), "Format of diagram to generate ('CFG', 'CallGraph', 'LaTeX', or 'ComputeGraph')")
        /****** CallGraph options ******/
        ("display_num_calls",  "[CallGraph] Display how many times a function is called")
    ; // NOLINT
    // clang-format on

    auto vm = po::variables_map();
    try {
        po::store(po::parse_command_line(argc, argv, description), vm);
        po::notify(vm);
    } catch (const po::error_with_option_name& ex) {
        std::cerr << ex.what() << std::endl;
        std::cerr << "To get the list of available options, run 'qret diagram --help'."
                  << std::endl;
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
    if (vm.count("function") == 0) {
        std::cerr << "missing required option: --function <name>" << std::endl;
        return ReturnStatus::Failure;
    }
    if (vm.count("graph-format") == 0) {
        std::cerr << "missing required option: --graph-format <type>" << std::endl;
        return ReturnStatus::Failure;
    }

    const auto input = vm["input"].as<std::string>();
    const auto function_name = vm["function"].as<std::string>();
    const auto format = vm["graph-format"].as<std::string>();
    const auto output = [&]() {
        if (vm.count("output") > 0) {
            return vm["output"].as<std::string>();
        } else if (format == "LaTeX") {
            return std::string("out.tex");
        }
        return std::string("out.dot");
    }();
    const auto display_num_calls = vm.count("display_num_calls") > 0;

    return Diagram(input, function_name, output, format, display_num_calls);
}
}  // namespace qret::cmd
