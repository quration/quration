/**
 * @file qret/cmd/compile.cpp
 * @brief Define 'compile' subcommand in qret.
 */

#include "qret/cmd/compile.h"

#include <boost/program_options.hpp>
#include <boost/program_options/variables_map.hpp>
#include <yaml-cpp/yaml.h>

#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "qret/base/log.h"
#include "qret/base/option.h"
#include "qret/base/string.h"
#include "qret/cmd/common.h"
#include "qret/target/compile_format.h"
#include "qret/target/compile_request.h"
#include "qret/target/target_compile_backend.h"
#include "qret/target/target_registry.h"

namespace qret::cmd {
namespace po = boost::program_options;
using Description = boost::program_options::options_description;

// OptionStorage keeps typed option definitions used across commands.
// These helpers expose them to boost::program_options descriptions.
template <qret::Storable T>
void AddDescriptionImpl(const qret::Option<T>* option_ptr, Description& desc) {
    const auto& option = *option_ptr;
    desc.add_options()(
            option.GetCommand().data(),
            boost::program_options::value<T>()->default_value(option.GetDefaultValue()),
            option.GetDescription().data()
    );
}

template <qret::Storable T>
void AddDescription(
        const qret::Option<T>* option_ptr,
        Description& generic,
        Description& hidden,
        Description& really_hidden
) {
    switch (option_ptr->GetHiddenFlag()) {
        case qret::OptionHidden::NotHidden:
            AddDescriptionImpl(option_ptr, generic);
            break;
        case qret::OptionHidden::Hidden:
            AddDescriptionImpl(option_ptr, hidden);
            break;
        case qret::OptionHidden::ReallyHidden:
            AddDescriptionImpl(option_ptr, really_hidden);
            break;
        default:
            break;
    }
}

void AddDescriptions(Description& generic, Description& hidden, Description& really_hidden) {
    const auto& options = *qret::OptionStorage::GetOptionStorage();
    for (const auto& [command, option_ptr] : options) {
        // Forward each typed option to the right visibility bucket.
        std::visit(
                [&generic, &hidden, &really_hidden](const auto& option_ptr) {
                    AddDescription(option_ptr, generic, hidden, really_hidden);
                },
                option_ptr
        );
    }
}

template <qret::Storable T>
void ReadOptionValue(qret::Option<T>* option_ptr, const VariablesMap& vm) {
    auto& option = *option_ptr;
    if (vm.Contains(std::string(option.GetCommand()))) {
        option.SetValue(vm.Get<T>(std::string(option.GetCommand())));
    }
}

void ReadOptionValues(const VariablesMap& vm) {
    // Synchronize global OptionStorage values with parsed command-line/YAML values.
    const auto& options = *qret::OptionStorage::GetOptionStorage();
    for (const auto& [command, option_ptr] : options) {
        std::visit([&vm](const auto& option_ptr) { ReadOptionValue(option_ptr, vm); }, option_ptr);
    }
}

// Adapter from target-side option registration to boost::program_options.
class BoostCompileOptionRegistrar : public qret::CompileOptionRegistrar {
public:
    explicit BoostCompileOptionRegistrar(Description& desc)
        : desc_(desc) {}

    void AddFlagOption(std::string_view name, std::string_view description) override {
        const auto n = std::string(name);
        const auto d = std::string(description);
        desc_.add_options()(n.c_str(), d.c_str());
    }

    void AddStringOption(
            std::string_view name,
            std::string_view value_name,
            std::string_view description
    ) override {
        const auto n = std::string(name);
        const auto v = std::string(value_name);
        const auto d = std::string(description);
        desc_.add_options()(n.c_str(), po::value<std::string>()->value_name(v), d.c_str());
    }

    void AddStringOptionWithDefault(
            std::string_view name,
            std::string_view default_value,
            std::string_view value_name,
            std::string_view description
    ) override {
        const auto n = std::string(name);
        const auto def = std::string(default_value);
        const auto v = std::string(value_name);
        const auto d = std::string(description);
        desc_.add_options()(
                n.c_str(),
                po::value<std::string>()->default_value(def)->value_name(v),
                d.c_str()
        );
    }

    void AddUInt64Option(
            std::string_view name,
            std::uint64_t default_value,
            std::string_view description
    ) override {
        const auto n = std::string(name);
        const auto d = std::string(description);
        desc_.add_options()(
                n.c_str(),
                po::value<std::uint64_t>()->default_value(default_value),
                d.c_str()
        );
    }

    void AddDoubleOption(
            std::string_view name,
            double default_value,
            std::string_view description
    ) override {
        const auto n = std::string(name);
        const auto d = std::string(description);
        desc_.add_options()(
                n.c_str(),
                po::value<double>()->default_value(default_value),
                d.c_str()
        );
    }

private:
    Description& desc_;
};

// Adapter from VariablesMap (CLI + optional YAML) to target-side option reader.
class VariablesMapCompileOptionReader : public qret::CompileOptionReader {
public:
    explicit VariablesMapCompileOptionReader(const VariablesMap& vm)
        : vm_(vm) {}

    [[nodiscard]] bool Contains(std::string_view key) const override {
        return vm_.Contains(std::string(key));
    }
    [[nodiscard]] std::string GetString(std::string_view key) const override {
        return vm_.Get<std::string>(std::string(key));
    }
    [[nodiscard]] std::string
    GetString(std::string_view key, std::string_view default_value) const override {
        return vm_.Get<std::string>(std::string(key), std::string(default_value));
    }
    [[nodiscard]] std::uint64_t GetUInt64(std::string_view key) const override {
        return vm_.Get<std::uint64_t>(std::string(key));
    }
    [[nodiscard]] std::uint64_t
    GetUInt64(std::string_view key, std::uint64_t default_value) const override {
        return vm_.Get<std::uint64_t>(std::string(key), default_value);
    }
    [[nodiscard]] double GetDouble(std::string_view key, double default_value) const override {
        return vm_.Get<double>(std::string(key), default_value);
    }
    [[nodiscard]] std::vector<qret::PassConfig> GetPassConfigList(
            std::string_view key
    ) const override {
        auto ret = std::vector<qret::PassConfig>();
        const auto config_list = ParsePass(vm_, std::string(key));
        ret.reserve(config_list.size());
        for (const auto& config : config_list) {
            ret.emplace_back(config.arg, config.cmd, config.input, config.output, config.runner);
        }
        return ret;
    }

private:
    const VariablesMap& vm_;
};

void AddCommonCompileOptions(Description& generic) {
    // clang-format off
    generic.add_options()
        ("help,h", "Show this help and exit.")
        ("help-hidden", "Show help including hidden options.")
        ("quiet", "Suppress non-error output.")
        ("verbose", "Enable verbose logging (more detail than default).")
        ("debug", "Enable debug logging (most detailed; implies --verbose).")
        ("color", "Enable colored output.")
        ("pipeline", po::value<std::string>()->value_name("FILE"), "Path to a pipeline specification file.")
        ("input,i", po::value<std::string>()->value_name("FILE"), "Path to the input file.")
        ("function,f", po::value<std::string>()->value_name("NAME"), "[source=IR] Name of the function to compile.")
        ("output,o", po::value<std::string>()->default_value("a.json")->value_name("FILE"), "Path to the output SC_LS_FIXED_V0 file.")
        ("source,s", po::value<std::string>()->default_value("IR")->value_name("KIND"), "Source representation: 'IR', 'OpenQASM2', or 'SC_LS_FIXED_V0'.")
        ("target,t", po::value<std::string>()->default_value("SC_LS_FIXED_V0")->value_name("KIND"), "Target machine name.")
    ; // NOLINT
    // clang-format on
}

void AddTargetCompileOptions(Description& generic) {
    // Each target backend can contribute its own CLI options.
    const auto* registry = qret::TargetRegistry::GetTargetRegistry();
    auto registrar = BoostCompileOptionRegistrar(generic);
    for (const auto& entry : registry->GetCompileBackends()) {
        entry.second->AddCompileOptions(registrar);
    }
}

std::optional<qret::CompileRequest> BuildCompileRequest(const VariablesMap& vm) {
    if (!vm.Contains("input")) {
        std::cerr << "missing required option: --input <file>" << std::endl;
        return std::nullopt;
    }

    qret::CompileRequest request;
    request.input = vm.Get<std::string>("input");
    request.function_name = vm.Get<std::string>("function", "");
    request.output = vm.Get<std::string>("output");
    request.target_name = GetLowerString(vm.Get<std::string>("target", "SC_LS_FIXED_V0"));
    request.source_format = qret::CompileFormatFromString(vm.Get<std::string>("source"));

    // IR input requires function selection.
    // OpenQASM2 and SC_LS_FIXED_V0 inputs already encode a single pipeline entry.
    if (request.source_format == qret::CompileFormat::IR && request.function_name.empty()) {
        std::cerr << "--function option is required" << std::endl;
        return std::nullopt;
    }
    return request;
}

ReturnStatus CompileByTargetBackend(const qret::CompileRequest& request, const VariablesMap& vm) {
    // compile.cpp is a thin dispatcher; target-specific behavior lives in backends.
    const auto* registry = qret::TargetRegistry::GetTargetRegistry();
    const auto* backend = registry->GetCompileBackend(request.target_name);
    if (backend == nullptr) {
        return ReturnStatus::Failure;
    }
    if (!backend->Supports(request.source_format)) {
        std::cerr << "source format is not supported for target '" << request.target_name << "'"
                  << std::endl;
        return ReturnStatus::Failure;
    }

    auto options = VariablesMapCompileOptionReader(vm);
    if (!backend->Compile(request, options)) {
        return ReturnStatus::Failure;
    }
    return ReturnStatus::Success;
}

ReturnStatus CommandCompile::Main(int argc, const char** argv) {
    // 1) Build command-line schema from common options + target-specific options.
    auto generic = po::options_description(R"(qret 'compile' options)", 120, 40);
    AddCommonCompileOptions(generic);
    AddTargetCompileOptions(generic);

    auto hidden = po::options_description("Hidden options", 120, 40);
    hidden.add_options()(
            "help-really-hidden",
            "Display available options including really hidden ones"
    );
    auto really_hidden = po::options_description("Really hidden options", 120, 40);

    AddDescriptions(generic, hidden, really_hidden);

    po::options_description visible_options;
    visible_options.add(generic);
    po::options_description hidden_options;
    hidden_options.add(generic).add(hidden);
    po::options_description cmdline_options;
    cmdline_options.add(generic).add(hidden).add(really_hidden);

    // 2) Parse command-line options first. YAML pipeline, if set, is loaded afterward.
    auto vm = VariablesMap();
    try {
        po::store(po::parse_command_line(argc, argv, cmdline_options), vm.vm);
        po::notify(vm.vm);
    } catch (const po::error_with_option_name& ex) {
        std::cerr << ex.what() << std::endl;
        std::cerr << "To get the list of available options, run 'qret compile --help'."
                  << std::endl;
        return ReturnStatus::Failure;
    }

    // 3) Early-return for help modes before any execution setup.
    if (vm.Contains("help")) {
        std::cout << visible_options;
        return ReturnStatus::Success;
    }
    if (vm.Contains("help-hidden")) {
        std::cout << hidden_options;
        return ReturnStatus::Success;
    }
    if (vm.Contains("help-really-hidden")) {
        std::cout << cmdline_options;
        return ReturnStatus::Success;
    }

    // 4) Configure logging.
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

    // 5) Optional YAML pipeline overrides source of option values via VariablesMap.
    if (vm.vm.count("pipeline") > 0) {
        LOG_DEBUG("Use pipeline file to compile.");
        vm.yaml = YAML::LoadFile(vm.vm.at("pipeline").as<std::string>());
    }

    // 6) Propagate parsed values into global option storage used by pass/backends.
    ReadOptionValues(vm);

    // 7) Build normalized request and dispatch to selected target backend.
    try {
        const auto request = BuildCompileRequest(vm);
        if (!request.has_value()) {
            return ReturnStatus::Failure;
        }
        return CompileByTargetBackend(request.value(), vm);
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return ReturnStatus::Failure;
    }
}
}  // namespace qret::cmd
