/**
 * @file qret/cmd/opt.cpp
 * @brief Define 'opt' sumcommand in qret.
 */

#include "qret/cmd/opt.h"

#include <boost/program_options.hpp>
#include <fmt/format.h>

#include <algorithm>
#include <iostream>
#include <vector>

#include "qret/base/json.h"
#include "qret/base/log.h"
#include "qret/base/option.h"
#include "qret/cmd/common.h"
#include "qret/ir/context.h"
#include "qret/ir/function.h"
#include "qret/ir/function_pass.h"
#include "qret/ir/json.h"
#include "qret/ir/module.h"
#include "qret/pass.h"
#include "qret/transforms/external/external_pass.h"

namespace qret::cmd {
ReturnStatus OptIR(
        const std::string& input,
        const std::string& function_name,
        const std::string& output,
        const std::vector<PassConfig>& pass_config
) {
    // Load the input json, which is a serialized IR module.
    LOG_INFO("Load IR.");
    auto ifs = std::ifstream(input);
    auto j = qret::Json::parse(ifs);

    // Deserialize IR module from json
    LOG_INFO("Load IR.");
    qret::ir::IRContext context;
    qret::ir::LoadJson(j, context);
    auto* func = context.owned_module.back()->GetFunction(function_name);
    if (func == nullptr) {
        std::cerr << "function of name '" << function_name << "' not found" << std::endl;
        std::cerr << "available functions:" << std::endl;
        auto names = std::vector<std::string>{};
        const auto* module = context.owned_module.back().get();
        for (const auto& function : *module) {
            names.emplace_back(function.GetName());
        }
        std::sort(names.begin(), names.end());
        for (const auto& name : names) {
            std::cerr << "  - " << name << std::endl;
        }
        return ReturnStatus::Failure;
    }

    LOG_INFO("Optimize IR.");
    for (const auto& config : pass_config) {
        const auto* registry = qret::PassRegistry::GetPassRegistry();

        if (config.IsExternalPass()) {
            auto pass = qret::ir::ExternalPass(
                    config.arg,
                    config.cmd,
                    config.input,
                    config.output,
                    config.runner
            );
            pass.RunOnFunction(*func);
        } else {
            if (!registry->Contains(config.arg)) {
                throw std::runtime_error(fmt::format("unknown pass: {}", config.arg));
            }
            auto pass = (registry->GetPassInfo(config.arg)->GetNormalCtor())();
            if (pass == nullptr) {
                throw std::runtime_error(
                        fmt::format("Cannot use pass {} from command line.", config.arg)
                );
            }
            static_cast<ir::FunctionPass*>(pass.get())->RunOnFunction(*func);
        }
    }

    LOG_INFO("Save IR.");
    auto ofs = std::ofstream(output);
    if (!ofs.good()) {
        std::cerr << "failed to open: " << output << std::endl;
        return ReturnStatus::Failure;
    }
    ofs << Json(*context.owned_module.back());
    ofs.close();

    return ReturnStatus::Success;
}
ReturnStatus CommandOpt::Main(int argc, const char** argv) {
    namespace po = boost::program_options;

    // Define description.
    // clang-format off
    auto description = po::options_description(R"(qret 'opt' options)");
    description.add_options()
        ("help,h", "Show this help and exit.")
        ("quiet", "Suppress non-error output.")
        ("verbose", "Enable verbose logging (more detail than default).")
        ("debug", "Enable debug logging (most detailed; implies --verbose).")
        ("color", "Enable colored output.")
        ("pipeline", po::value<std::string>(), "Pipeline file")
        ("input,i", po::value<std::string>(), "Input file")
        ("function,f", po::value<std::string>(), "Function name to optimize")
        ("output,o", po::value<std::string>(), "Output file")
        ("ir-static-condition-pruning-seed", po::value<std::uint64_t>()->default_value(0), "Seed of ir::static_condition_pruning pass.")
        ("pass", po::value<std::string>(), "Optimization pass")
    ; // NOLINT
    // clang-format on

    auto vm = VariablesMap();
    try {
        po::store(po::parse_command_line(argc, argv, description), vm.vm);
        po::notify(vm.vm);
    } catch (const po::error_with_option_name& ex) {
        std::cerr << ex.what() << std::endl;
        std::cerr << "To get the list of available options, run 'qret opt --help'." << std::endl;
        return ReturnStatus::Failure;
    }

    // Check basic options
    if (vm.Contains("help")) {
        std::cout << description;
        return ReturnStatus::Success;
    }
    if (vm.Contains("quiet")) {
        qret::Logger::DisableConsoleOutput();
        qret::Logger::DisableFileOutput();
    } else if (vm.Contains("debug")) {
        qret::Logger::EnableConsoleOutput();
        qret::Logger::SetLogLevel(qret::LogLevel::Debug);
    } else if (vm.Contains("verbose")) {
        qret::Logger::EnableConsoleOutput();
        qret::Logger::SetLogLevel(qret::LogLevel::Info);
    } else {
        qret::Logger::EnableConsoleOutput();
        qret::Logger::SetLogLevel(qret::LogLevel::Warn);
    }
    if (vm.Contains("color")) {
        qret::Logger::EnableColorfulOutput();
    } else {
        qret::Logger::DisableColorfulOutput();
    }

    // Check 'pipeline' option at first to update VariablesMap class.
    if (vm.vm.count("pipeline") > 0) {
        LOG_DEBUG("Use pipeline file to compile.");
        vm.yaml = YAML::LoadFile(vm.vm.at("pipeline").as<std::string>());
    }

    if (!vm.Contains("input")) {
        std::cerr << "missing required option: --input <file>" << std::endl;
        return ReturnStatus::Failure;
    }
    if (!vm.Contains("function")) {
        std::cerr << "missing required option: --function <name>" << std::endl;
        return ReturnStatus::Failure;
    }
    if (!vm.Contains("output")) {
        std::cerr << "missing required option: --output <file>" << std::endl;
        return ReturnStatus::Failure;
    }
    const auto input = vm.Get<std::string>("input");
    const auto function_name = vm.Get<std::string>("function");
    const auto output = vm.Get<std::string>("output");
    if (vm.Contains("ir-static-condition-pruning-seed")) {
        std::get<Option<std::uint64_t>*>(
                OptionStorage::GetOptionStorage()->At("ir-static-condition-pruning-seed")
        )
                ->SetValue(vm.Get<std::uint64_t>("ir-static-condition-pruning-seed"));
    }
    const auto pass_config = ParsePass(vm, "pass");

    return OptIR(input, function_name, output, pass_config);
}
}  // namespace qret::cmd
