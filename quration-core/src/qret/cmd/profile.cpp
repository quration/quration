/**
 * @file qret/cmd/profile.cpp
 * @brief Define 'profile' subcommand in qret.
 */

#include "qret/cmd/profile.h"

#include <boost/program_options.hpp>

#include <fstream>
#include <iostream>
#include <memory>

#include "qret/base/log.h"
#include "qret/target/compile_format.h"
#include "qret/target/sc_ls_fixed_v0/calc_compile_info.h"
#include "qret/target/sc_ls_fixed_v0/pipeline_state.h"

namespace qret::cmd {
ReturnStatus Profile(
        const std::string& input,
        const std::string& output,
        const std::string& source,
        const std::string& format
) {
    const auto source_format = qret::CompileFormatFromString(source);
    if (source_format != qret::CompileFormat::SC_LS_FIXED_V0) {
        std::cerr << "source format is not supported for profile command: " << source << std::endl;
        return ReturnStatus::Failure;
    }
    if (format != "json" && format != "markdown") {
        std::cerr << "unknown format: " << format << ". expected json or markdown" << std::endl;
        return ReturnStatus::Failure;
    }

    auto state = qret::sc_ls_fixed_v0::LoadPipelineState(input);
    if (!state.parameter.target.has_value()) {
        std::cerr << "SC_LS_FIXED_V0 pipeline state must contain parameter.target" << std::endl;
        return ReturnStatus::Failure;
    }

    auto target_machine = std::unique_ptr<qret::sc_ls_fixed_v0::ScLsFixedV0TargetMachine>(
            new qret::sc_ls_fixed_v0::ScLsFixedV0TargetMachine(*state.parameter.target)
    );
    // Rebuild MachineFunction and compute compile info from the current program.
    auto mf = qret::MachineFunction(target_machine.get());
    qret::sc_ls_fixed_v0::ApplyPipelineState(state, mf);

    qret::sc_ls_fixed_v0::CompileInfoWithoutTopology().RunOnMachineFunction(mf);
    qret::sc_ls_fixed_v0::CompileInfoWithTopology().RunOnMachineFunction(mf);
    qret::sc_ls_fixed_v0::CompileInfoWithQecResourceEstimation().RunOnMachineFunction(mf);

    if (!mf.HasCompileInfo()) {
        std::cerr << "Failed to compute compile info." << std::endl;
        return ReturnStatus::Failure;
    }

    const auto& compile_info =
            *static_cast<const qret::sc_ls_fixed_v0::ScLsFixedV0CompileInfo*>(mf.GetCompileInfo());

    auto ofs = std::ofstream(output);
    if (!ofs.good()) {
        std::cerr << "failed to open: " << output << std::endl;
        return ReturnStatus::Failure;
    }
    if (format == "json") {
        ofs << compile_info.Json() << std::endl;
    } else {
        ofs << compile_info.Markdown() << std::endl;
    }
    ofs.close();

    return ReturnStatus::Success;
}

ReturnStatus CommandProfile::Main(int argc, const char** argv) {
    namespace po = boost::program_options;

    // clang-format off
    auto description = po::options_description(R"(qret 'profile' options)");
    description.add_options()
        ("help,h", "Show this help and exit.")
        ("quiet", "Suppress non-error output.")
        ("verbose", "Enable verbose logging (more detail than default).")
        ("debug", "Enable debug logging (most detailed; implies --verbose).")
        ("color", "Enable colored output.")
        ("source,s", po::value<std::string>()->default_value("SC_LS_FIXED_V0"), "Source representation. Currently only 'SC_LS_FIXED_V0' is supported.")
        ("format,f", po::value<std::string>()->default_value("json"), "Output format: 'json' or 'markdown'.")
        ("input,i", po::value<std::string>(), "Input file")
        ("output,o", po::value<std::string>()->default_value("compile_info.json"), "Output file")
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
        std::cerr << "To get the list of available options, run 'qret profile --help'."
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

    const auto input = vm["input"].as<std::string>();
    const auto output = vm["output"].as<std::string>();
    const auto source = vm["source"].as<std::string>();
    const auto format = vm["format"].as<std::string>();

    return Profile(input, output, source, format);
}
}  // namespace qret::cmd
