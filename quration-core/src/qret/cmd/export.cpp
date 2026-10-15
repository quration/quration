/**
 * @file qret/cmd/export.cpp
 * @brief Define 'export' subcommand in qret.
 */

#include "qret/cmd/export.h"

#include <boost/program_options.hpp>
#include <fmt/format.h>

#include <fstream>
#include <iostream>
#include <vector>

#include "qret/base/json.h"
#include "qret/base/log.h"
#include "qret/cmd/common.h"
#include "qret/exporter/qir.h"
#include "qret/ir/context.h"
#include "qret/ir/function.h"
#include "qret/ir/json.h"  // DO NOT DELETE
#include "qret/ir/module.h"
#include "qret/transforms/ipo/inliner.h"

namespace qret::cmd {
ReturnStatus ExportQIR(
        const std::string& input,
        const std::string& output,
        const std::string& entry,
        bool inline_entry
) {
    LOG_INFO("Load IR json.");
    auto ifs = std::ifstream(input);
    if (!ifs.good()) {
        std::cerr << "failed to open: " << input << std::endl;
        return ReturnStatus::Failure;
    }
    auto j = qret::Json::parse(ifs);

    LOG_INFO("Construct ir from json.");
    qret::ir::IRContext context;
    qret::ir::LoadJson(j, context);
    auto* module = context.owned_module.back().get();

    // LoadJson only checks "entry_function" exists among the module's functions; it isn't
    // retained anywhere on Module itself. Without `--entry`, fall back to it here (rather than
    // ExportModuleToQIR's own fallback - the module's first function - which only happens to
    // match the entry point for modules where nothing reordered functions after import).
    auto effective_entry = entry;
    if (effective_entry.empty() && j.contains("entry_function")) {
        effective_entry = j.at("entry_function").get<std::string>();
    }

    if (inline_entry) {
        LOG_INFO("Inline sub-circuit calls into the entry function.");
        auto* entry_func =
                effective_entry.empty() ? &*module->begin() : module->GetFunction(effective_entry);
        if (entry_func == nullptr) {
            std::cerr << "entry function '" << effective_entry << "' not found in module"
                      << std::endl;
            return ReturnStatus::Failure;
        }
        effective_entry = std::string(entry_func->GetName());
        qret::ir::RecursiveInlinerPass().RunOnFunction(*entry_func);
        // Everything is now inlined into the entry, so the other functions are unreachable; drop
        // them to emit single-entry-point QIR.
        auto unused = std::vector<qret::ir::Function*>();
        for (auto& func : *module) {
            if (&func != entry_func) {
                unused.push_back(&func);
            }
        }
        for (auto* func : unused) {
            qret::ir::RemoveFromParent(func);
        }
    }

    LOG_INFO("Export to QIR.");
    const auto qir_text = qret::qir::ExportModuleToQIR(*module, effective_entry);

    LOG_INFO("Save QIR.");
    auto ofs = std::ofstream(output);
    if (!ofs.good()) {
        std::cerr << "failed to open: " << output << std::endl;
        return ReturnStatus::Failure;
    }
    ofs << qir_text;
    ofs.close();

    return ReturnStatus::Success;
}
ReturnStatus Export(
        const std::string& input,
        const std::string& output,
        const std::string& format,
        const std::string& entry,
        bool inline_entry
) {
    if (format == "qir") {
        return ExportQIR(input, output, entry, inline_entry);
    }
    std::cerr << "unknown format: " << format << ". expected qir" << std::endl;
    return ReturnStatus::Failure;
}
ReturnStatus CommandExport::Main(int argc, const char** argv) {
    namespace po = boost::program_options;

    // Define description.
    // clang-format off
    auto description = po::options_description(R"(qret 'export' options)");
    description.add_options()
        ("help,h", "Show this help and exit.")
        ("quiet", "Suppress non-error output.")
        ("verbose", "Enable verbose logging (more detail than default).")
        ("debug", "Enable debug logging (most detailed; implies --verbose).")
        ("color", "Enable colored output.")
        ("input,i", po::value<std::string>(), "Input IR json file")
        ("output,o", po::value<std::string>()->default_value("out.ll"), "Output file")
        ("format,f", po::value<std::string>()->default_value("qir"), "Format of output file ('qir').")
        ("entry", po::value<std::string>()->default_value(""), "Name of the entry function within the input IR module. If empty, uses the input JSON's own 'entry_function' field, falling back to the module's first function if that's absent too.")
        ("inline", "Inline all sub-circuit calls into the entry function and drop the now-unused functions, producing single-entry-point QIR (more compatible with QIR consumers; may enlarge the output for circuits that reuse sub-circuits heavily).")
    ; // NOLINT
    // clang-format on

    auto vm = po::variables_map();
    try {
        po::store(po::parse_command_line(argc, argv, description), vm);
        po::notify(vm);
    } catch (const po::error_with_option_name& ex) {
        std::cerr << ex.what() << std::endl;
        std::cerr << "To get the list of available options, run 'qret export --help'." << std::endl;
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
    const auto entry = vm["entry"].as<std::string>();
    const auto inline_entry = vm.count("inline") > 0;

    return Export(input, output, format, entry, inline_entry);
}
}  // namespace qret::cmd
