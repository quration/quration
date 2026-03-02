/**
 * @file qret/cmd/asm.cpp
 * @brief Define 'asm' subcommand in qret.
 */

#include "qret/cmd/asm.h"

#include <boost/program_options.hpp>

#include <fstream>
#include <iostream>
#include <memory>

#include "qret/base/log.h"
#include "qret/base/option.h"
#include "qret/base/string.h"
#include "qret/codegen/machine_function.h"
#include "qret/target/compile_format.h"
#include "qret/target/sc_ls_fixed_v0/pipeline_state.h"
#include "qret/target/target_registry.h"

namespace qret::cmd {
ReturnStatus Asm(
        const std::string& input,
        const std::string& output,
        const std::string& source,
        const std::string& target,
        bool print_metadata
) {
    const auto source_format = qret::CompileFormatFromString(source);
    if (source_format != qret::CompileFormat::SC_LS_FIXED_V0) {
        std::cerr << "source format is not supported for asm command: " << source << std::endl;
        return ReturnStatus::Failure;
    }
    const auto option_storage = qret::OptionStorage::GetOptionStorage();
    if (option_storage->Contains("sc_ls_fixed_v0-print-inst-metadata")) {
        if (const auto* option_ptr =
                    std::get_if<qret::Option<bool>*>( &option_storage->At(
                            "sc_ls_fixed_v0-print-inst-metadata"
                    ))) {
            (*option_ptr)->SetValue(print_metadata);
        }
    }

    auto state = qret::sc_ls_fixed_v0::LoadPipelineState(input);
    if (!state.parameter.target.has_value()) {
        std::cerr << "SC_LS_FIXED_V0 pipeline state must contain parameter.target" << std::endl;
        return ReturnStatus::Failure;
    }

    auto target_machine = std::unique_ptr<qret::sc_ls_fixed_v0::ScLsFixedV0TargetMachine>(
            new qret::sc_ls_fixed_v0::ScLsFixedV0TargetMachine(*state.parameter.target)
    );
    // Reconstruct MachineFunction from pipeline state and hand it to asm printer.
    auto mf = qret::MachineFunction(target_machine.get());
    qret::sc_ls_fixed_v0::ApplyPipelineState(state, mf);

    const auto* target_registry = qret::TargetRegistry::GetTargetRegistry();
    const auto* target_obj = target_registry->GetTarget(GetLowerString(target));
    if (target_obj == nullptr) {
        return ReturnStatus::Failure;
    }
    if (!target_obj->HasAsmPrinter()) {
        std::cerr << "target '" << target << "' does not provide asm printer" << std::endl;
        return ReturnStatus::Failure;
    }

    auto streamer = target_obj->CreateAsmStreamer();
    auto asm_printer = target_obj->CreateAsmPrinter(std::move(streamer));
    asm_printer->RunOnMachineFunction(mf);

    auto ofs = std::ofstream(output);
    if (!ofs.good()) {
        std::cerr << "failed to open: " << output << std::endl;
        return ReturnStatus::Failure;
    }
    ofs << asm_printer->GetStreamer()->ToString();
    ofs.close();

    return ReturnStatus::Success;
}

ReturnStatus CommandAsm::Main(int argc, const char** argv) {
    namespace po = boost::program_options;

    // clang-format off
    auto description = po::options_description(R"(qret 'asm' options)");
    description.add_options()
        ("help,h", "Show this help and exit.")
        ("quiet", "Suppress non-error output.")
        ("verbose", "Enable verbose logging (more detail than default).")
        ("debug", "Enable debug logging (most detailed; implies --verbose).")
        ("color", "Enable colored output.")
        ("source,s", po::value<std::string>()->default_value("SC_LS_FIXED_V0"), "Source representation. Currently only 'SC_LS_FIXED_V0' is supported.")
        ("target,t", po::value<std::string>()->default_value("SC_LS_FIXED_V0"), "Target name used to select asm printer.")
        ("print-metadata", po::value<bool>()->value_name("BOOL")->default_value(true), "Print instruction metadata. "
         "If false, SC_LS_FIXED_V0 output will omit instruction metadata.")
        ("input,i", po::value<std::string>(), "Input file")
        ("output,o", po::value<std::string>()->default_value("out.asm"), "Output file")
    ; // NOLINT
    // clang-format on

    auto positional = po::positional_options_description();
    positional.add("input", 1);
    positional.add("output", 1);

    auto vm = po::variables_map();
    try {
        po::store(
                po::command_line_parser(argc, argv)
                        .options(description)
                        .positional(positional)
                        .run(),
                vm
        );
        po::notify(vm);
    } catch (const po::error_with_option_name& ex) {
        std::cerr << ex.what() << std::endl;
        std::cerr << "To get the list of available options, run 'qret asm --help'." << std::endl;
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
    const auto source = vm["source"].as<std::string>();
    const auto target = vm["target"].as<std::string>();
    const auto print_metadata = vm["print-metadata"].as<bool>();

    return Asm(input, output, source, target, print_metadata);
}
}  // namespace qret::cmd
