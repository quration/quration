/**
 * @file qret/cmd/print.cpp
 * @brief Define 'print' sumcommand in qret.
 */

#include "qret/cmd/print.h"

#include <boost/program_options.hpp>
#include <fmt/format.h>

#include <algorithm>
#include <iostream>
#include <vector>

#include "qret/base/cast.h"
#include "qret/base/json.h"
#include "qret/base/log.h"
#include "qret/cmd/common.h"
#include "qret/ir/context.h"
#include "qret/ir/function.h"
#include "qret/ir/instructions.h"
#include "qret/ir/json.h"
#include "qret/ir/module.h"

namespace qret::cmd {
namespace {
void PrintSection(std::string_view section_name) {
    std::cout << "[" << section_name << "]\n";
}

void PrintSummaryLine(std::string_view key, std::string_view value) {
    std::cout << "  " << fmt::format("{:<22}", key) << ": " << value << "\n";
}

std::string GetIndent(std::uint64_t depth) {
    return std::string(4 * depth, ' ');  // NOLINT
}
}  // namespace
ReturnStatus PrintImpl(
        const ir::Function* func,
        std::uint64_t max_depth,
        std::uint64_t depth,
        bool print_debug_info
) {
    if (depth == 0) {
        PrintSection("Function");
    }
    std::cout << GetIndent(depth) << func->GetName() << ":\n";
    for (const auto& bb : *func) {
        std::cout << " " << GetIndent(depth) << "[block] " << bb.GetName() << "\n";
        for (const auto& inst : bb) {
            // Print instruction
            if (print_debug_info || !inst.IsOptHint()) {
                std::cout << "  " << GetIndent(depth);
                inst.Print(std::cout);
            }
            if (print_debug_info) {
                inst.PrintMetadata(std::cout);
            }
            if (print_debug_info || !inst.IsOptHint()) {
                std::cout << "\n";
            }

            // Print the callee function of CallInst
            if (inst.IsCall() && depth + 1 < max_depth) {
                std::cout << "  " << GetIndent(depth) << "-->\n";
                const auto* call = Cast<ir::CallInst>(&inst);
                PrintImpl(call->GetCallee(), max_depth, depth + 1, print_debug_info);
                std::cout << "  " << GetIndent(depth) << "<--  (@" << call->GetCallee()->GetName()
                          << ")\n";
            }
        }
    }
    return ReturnStatus::Success;
}
ReturnStatus Print(
        const std::string& input,
        const std::string& function_name,
        std::uint64_t depth,
        bool print_debug_info
) {
    // Load the input json, which is a serialized IR module.
    LOG_INFO("Load json.");
    auto ifs = std::ifstream(input);
    auto j = qret::Json::parse(ifs);

    // Deserialize IR module from json
    LOG_INFO("Construct ir from json.");
    qret::ir::IRContext context;
    qret::ir::LoadJson(j, context);
    const auto* module = context.owned_module.back().get();

    LOG_INFO("Find function.");
    const auto* func = module->GetFunction(function_name);
    if (func == nullptr) {
        std::cerr << "function of name '" << function_name << "' not found" << std::endl;
        std::cerr << "available functions:" << std::endl;
        auto names = std::vector<std::string>{};
        for (const auto& function : *module) {
            names.push_back(std::string(function.GetName()));
        }
        std::sort(names.begin(), names.end());
        for (const auto& name : names) {
            std::cerr << "  - " << name << std::endl;
        }
        return ReturnStatus::Failure;
    }

    return PrintImpl(func, depth, 0, print_debug_info);
}
ReturnStatus PrintSummary(const std::string& input, std::string_view function_name = "") {
    // Load the input json, which is a serialized IR module.
    LOG_INFO("Load json.");
    auto ifs = std::ifstream(input);
    auto j = qret::Json::parse(ifs);

    // Deserialize IR module from json
    LOG_INFO("Construct ir from json.");
    qret::ir::IRContext context;
    qret::ir::LoadJson(j, context);
    const auto* module = context.owned_module.back().get();

    if (!function_name.empty()) {
        const auto* function = module->GetFunction(function_name);
        if (function == nullptr) {
            std::cerr << "function of name '" << function_name << "' not found" << std::endl;
            std::cerr << "available functions:" << std::endl;
            auto names = std::vector<std::string>{};
            for (const auto& func : *module) {
                names.push_back(std::string(func.GetName()));
            }
            std::sort(names.begin(), names.end());
            for (const auto& name : names) {
                std::cerr << "  - " << name << std::endl;
            }
            return ReturnStatus::Failure;
        }

        PrintSection("Function Summary");
        PrintSummaryLine("module", module->GetName());
        PrintSummaryLine("function", function->GetName());
        PrintSummaryLine("basic_block_count", std::to_string(function->GetNumBBs()));
        PrintSummaryLine("instruction_count", std::to_string(function->GetInstructionCount()));
        PrintSummaryLine("qubit_count", std::to_string(function->GetNumQubits()));
        PrintSummaryLine("register_count", std::to_string(function->GetNumRegisters()));
        PrintSummaryLine("tmp_register_count", std::to_string(function->GetNumTmpRegisters()));
        PrintSummaryLine("contains_measurement", function->DoesMeasurement() ? "yes" : "no");
        return ReturnStatus::Success;
    }

    PrintSection("Module Summary");
    PrintSummaryLine("name", module->GetName());
    PrintSummaryLine(
            "function_count",
            std::to_string(std::distance(module->begin(), module->end()))
    );

    auto names = std::vector<std::string>();
    for (const auto& func : *module) {
        names.emplace_back(func.GetName());
    }
    std::sort(names.begin(), names.end());
    std::cout << "  functions:\n";
    for (const auto& name : names) {
        std::cout << "    - " << name << "\n";
    }

    return ReturnStatus::Success;
}
ReturnStatus CommandPrint::Main(int argc, const char** argv) {
    namespace po = boost::program_options;

    // Define description.
    // clang-format off
    auto description = po::options_description(R"(qret 'print' options)");
    description.add_options()
        ("help,h", "Show this help and exit.")
        ("quiet", "Suppress non-error output.")
        ("verbose", "Enable verbose logging (more detail than default).")
        ("debug", "Enable debug logging (most detailed; implies --verbose).")
        ("color", "Enable colored output.")
        ("input,i", po::value<std::string>(), "Path to input IR file.")
        ("summary,s", "Print summary only. If --function is omitted, print module summary.")
        ("function,f", po::value<std::string>()->value_name("NAME"), "Function name to print; with --summary prints only the function summary.")
        ("depth,d", po::value<std::uint64_t>()->default_value(1), "Descend only 'depth' call deep")
        ("print_debug_info", "Print debug info")
    ; // NOLINT
    // clang-format on

    auto vm = po::variables_map();
    try {
        po::store(po::parse_command_line(argc, argv, description), vm);
        po::notify(vm);
    } catch (const po::error_with_option_name& ex) {
        std::cerr << ex.what() << std::endl;
        std::cerr << "To get the list of available options, run 'qret print --help'." << std::endl;
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
    if (vm.count("summary") > 0 && vm.count("function") == 0) {
        const auto input = vm["input"].as<std::string>();
        return PrintSummary(input);
    }
    if (vm.count("summary") > 0 && vm.count("function") > 0) {
        const auto input = vm["input"].as<std::string>();
        const auto function_name = vm["function"].as<std::string>();
        return PrintSummary(input, function_name);
    }
    if (vm.count("function") == 0) {
        const auto input = vm["input"].as<std::string>();
        return PrintSummary(input, "");
    }

    const auto input = vm["input"].as<std::string>();
    const auto function_name = vm["function"].as<std::string>();
    const auto depth = vm["depth"].as<std::uint64_t>();
    const auto print_debug_info = vm.count("print_debug_info") > 0;

    return Print(input, function_name, depth, print_debug_info);
}
}  // namespace qret::cmd
