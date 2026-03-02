/**
 * @file qret/target/sc_ls_fixed_v0/sc_ls_fixed_v0_compile_backend.cpp
 * @brief Compile backend for SC_LS_FIXED_V0 target.
 */

#include "qret/target/sc_ls_fixed_v0/sc_ls_fixed_v0_compile_backend.h"

#include <fmt/format.h>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "qret/base/log.h"
#include "qret/base/string.h"
#include "qret/codegen/machine_function.h"
#include "qret/codegen/machine_function_pass.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/openqasm2.h"
#include "qret/ir/context.h"
#include "qret/ir/function.h"
#include "qret/ir/json.h"
#include "qret/ir/module.h"
#include "qret/parser/openqasm2.h"
#include "qret/pass.h"
#include "qret/target/sc_ls_fixed_v0/external_pass.h"
#include "qret/target/sc_ls_fixed_v0/lowering.h"
#include "qret/target/sc_ls_fixed_v0/pipeline_state.h"
#include "qret/target/sc_ls_fixed_v0/sc_ls_fixed_v0_target_machine.h"
#include "qret/target/sc_ls_fixed_v0/topology.h"
#include "qret/target/target_registry.h"
#include "qret/transforms/ipo/inliner.h"
#include "qret/transforms/scalar/decomposition.h"
#include "qret/transforms/scalar/ignore_global_phase.h"
#include "qret/transforms/scalar/static_condition_pruning.h"

namespace qret::sc_ls_fixed_v0 {
namespace {
// This backend depends only on CompileOptionReader, not on value sources.
std::vector<qret::PassConfig> GetDefaultPass(const CompileFormat source) {
    if (source == CompileFormat::SC_LS_FIXED_V0) {
        return {};
    }

    auto ret = std::vector<qret::PassConfig>();
    ret.emplace_back("sc_ls_fixed_v0::init_compile_info");
    if (source == CompileFormat::IR || source == CompileFormat::OPENQASM2) {
        ret.emplace_back("sc_ls_fixed_v0::mapping");
    }
    ret.emplace_back("sc_ls_fixed_v0::routing");
    ret.emplace_back("sc_ls_fixed_v0::calc_info_without_topology");
    ret.emplace_back("sc_ls_fixed_v0::calc_info_with_topology");
    ret.emplace_back("sc_ls_fixed_v0::dump_compile_info");
    return ret;
}

std::optional<qret::ir::Function*>
LoadFunctionFromIR(const qret::CompileRequest& request, qret::ir::IRContext& context) {
    LOG_INFO("Load IR.");
    auto ifs = std::ifstream(request.input);
    auto j = qret::Json::parse(ifs);

    LOG_INFO("Find function.");
    qret::ir::LoadJson(j, context);
    auto* func = context.owned_module.back()->GetFunction(request.function_name);
    if (func == nullptr) {
        std::cerr << "function of name '" << request.function_name << "' not found" << std::endl;
        return std::nullopt;
    }
    return func;
}

std::optional<qret::ir::Function*>
LoadFunctionFromOpenQASM2(const qret::CompileRequest& request, qret::ir::IRContext& context) {
    LOG_INFO("Load OpenQASM2.");
    const auto ast = qret::openqasm2::ParseOpenQASM2File(request.input);

    LOG_INFO("Build IR from OpenQASM2.");
    auto* module = qret::ir::Module::Create("OpenQASM2", context);
    auto builder = qret::frontend::CircuitBuilder(module);
    const auto entry_name = request.function_name.empty() ? "main" : request.function_name;
    auto* circuit = qret::frontend::BuildCircuitFromAST(ast, builder, entry_name);
    return circuit->GetIR();
}

std::optional<std::shared_ptr<qret::sc_ls_fixed_v0::Topology>> LoadTopology(
        const qret::CompileOptionReader& options
) {
    if (!options.Contains("sc_ls_fixed_v0_topology")) {
        std::cerr << "Topology is not specified." << std::endl;
        return std::nullopt;
    }
    const auto topology_path = options.GetString("sc_ls_fixed_v0_topology");
    auto topology_ifs = std::ifstream(topology_path);
    return topology_path.ends_with(".json")
            ? qret::sc_ls_fixed_v0::Topology::FromJSON(qret::Json::parse(topology_ifs))
            : qret::sc_ls_fixed_v0::Topology::FromYAML(qret::LoadFile(topology_path));
}

std::optional<qret::sc_ls_fixed_v0::ScLsFixedV0MachineOption> GetMachineOption(
        const qret::CompileOptionReader& options,
        const qret::sc_ls_fixed_v0::Topology& topology
) {
    const auto enable_pbc_mode = options.Contains("sc_ls_fixed_v0_enable_pbc_mode");
    const auto machine_type_str = options.GetString("sc_ls_fixed_v0_machine_type", "auto");
    const auto required_type = qret::sc_ls_fixed_v0::GetMachineType(topology);
    auto machine_type = required_type;
    if (machine_type_str != "auto") {
        try {
            machine_type = qret::sc_ls_fixed_v0::ScLsFixedV0MachineTypeFromString(machine_type_str);
        } catch (const std::exception&) {
            std::cerr << "Error: Invalid --sc_ls_fixed_v0_machine_type '" << machine_type_str
                      << "'. Valid values are 'auto', 'Dim2', 'Dim3', 'DistributedDim2', "
                         "'DistributedDim3'.\n";
            return std::nullopt;
        }
    }
    if (!qret::sc_ls_fixed_v0::IsCompatible(machine_type, required_type)) {
        std::cerr << "Error: The specified machine type '" << machine_type_str
                  << "' is not compatible with the topology's minimum requirement of '"
                  << qret::sc_ls_fixed_v0::ToString(required_type) << "'.\n"
                  << "Please use a machine type that is either identical to or more advanced than '"
                  << qret::sc_ls_fixed_v0::ToString(required_type) << "'.\n";
        return std::nullopt;
    }
    if (enable_pbc_mode && machine_type != qret::sc_ls_fixed_v0::ScLsFixedV0MachineType::Dim2) {
        std::cerr << "Error: --sc_ls_fixed_v0_enable_pbc_mode currently supports only "
                     "--sc_ls_fixed_v0_machine_type=Dim2.\n";
        return std::nullopt;
    }

    return qret::sc_ls_fixed_v0::ScLsFixedV0MachineOption{
            .type = machine_type,
            .enable_pbc_mode = enable_pbc_mode,
            .use_magic_state_cultivation =
                    options.Contains("sc_ls_fixed_v0_use_magic_state_cultivation"),
            .magic_factory_seed_offset =
                    options.GetUInt64("sc_ls_fixed_v0_magic_factory_seed_offset", 0),
            .magic_generation_period = options.GetUInt64("sc_ls_fixed_v0_magic_generation_period"),
            .prob_magic_state_creation =
                    options.GetDouble("sc_ls_fixed_v0_prob_magic_state_creation", 1.0),
            .maximum_magic_state_stock =
                    options.GetUInt64("sc_ls_fixed_v0_maximum_magic_state_stock"),
            .entanglement_generation_period =
                    options.GetUInt64("sc_ls_fixed_v0_entanglement_generation_period"),
            .maximum_entangled_state_stock =
                    options.GetUInt64("sc_ls_fixed_v0_maximum_entangled_state_stock"),
            .reaction_time = options.GetUInt64("sc_ls_fixed_v0_reaction_time"),
            .physical_error_rate = options.GetDouble("sc_ls_fixed_v0_physical_error_rate", 0.0),
            .drop_rate = options.GetDouble("sc_ls_fixed_v0_drop_rate", 0.0),
            .code_cycle_time_sec = options.GetDouble("sc_ls_fixed_v0_code_cycle_time_sec", 0.0),
            .allowed_failure_prob = options.GetDouble("sc_ls_fixed_v0_allowed_failure_prob", 0.0)
    };
}

bool RunCompilation(
        const qret::CompileRequest& request,
        const std::shared_ptr<qret::sc_ls_fixed_v0::Topology>& topology,
        const qret::sc_ls_fixed_v0::ScLsFixedV0MachineOption& option,
        const std::vector<qret::PassConfig>& pass_config
) {
    auto target_machine = qret::sc_ls_fixed_v0::ScLsFixedV0TargetMachine::New(topology, option);

    auto mf = qret::MachineFunction(target_machine.get());
    auto manager = qret::MFPassManager();

    if (request.source_format == qret::CompileFormat::IR
        || request.source_format == qret::CompileFormat::OPENQASM2) {
        qret::ir::IRContext context;
        const auto func = request.source_format == qret::CompileFormat::IR
                ? LoadFunctionFromIR(request, context)
                : LoadFunctionFromOpenQASM2(request, context);
        if (!func.has_value()) {
            return false;
        }
        mf.SetIR(*func);

        LOG_INFO("Simplify IR before compiling to SC_LS_FIXED_V0.");
        qret::ir::RecursiveInlinerPass().RunOnFunction(**func);
        qret::ir::StaticConditionPruningPass().RunOnFunction(**func);
        qret::ir::DecomposeInst().RunOnFunction(**func);
        qret::ir::IgnoreGlobalPhase().RunOnFunction(**func);

        LOG_INFO("Lowering IR to the machine function of SC_LS_FIXED_V0.");
        qret::sc_ls_fixed_v0::Lowering().RunOnMachineFunction(mf);
    } else {
        LOG_INFO("Load SC_LS_FIXED_V0 pipeline state file.");
        auto state = qret::sc_ls_fixed_v0::LoadPipelineState(request.input);
        qret::sc_ls_fixed_v0::ApplyPipelineState(state, manager, target_machine, mf);
    }

    LOG_INFO("Run passes.");
    const auto add_pass = [&manager](std::string_view arg) {
        const auto* registry = qret::PassRegistry::GetPassRegistry();
        if (!registry->Contains(arg)) {
            throw std::runtime_error(fmt::format("unknown pass: {}", arg));
        }
        auto* pass = manager.AddPass((registry->GetPassInfo(arg)->GetNormalCtor())());
        if (pass == nullptr) {
            throw std::runtime_error(fmt::format("Cannot use pass {} from command line.", arg));
        }
    };
    const auto add_external_pass = [&manager](const qret::PassConfig& config) {
        manager.AddPass(
                std::unique_ptr<qret::sc_ls_fixed_v0::ExternalPass>(
                        new qret::sc_ls_fixed_v0::ExternalPass(
                                config.arg,
                                config.cmd,
                                config.input,
                                config.output,
                                config.runner
                        )
                )
        );
    };
    for (const auto& config : pass_config) {
        if (config.IsExternalPass()) {
            add_external_pass(config);
        } else {
            add_pass(config.arg);
        }
    }
    manager.Run(mf);

    LOG_INFO("Save SC_LS_FIXED_V0 pipeline state file.");
    const auto state = qret::sc_ls_fixed_v0::BuildPipelineState(manager, target_machine, mf);
    qret::sc_ls_fixed_v0::SavePipelineState(request.output, state);
    return true;
}
}  // namespace

std::string_view ScLsFixedV0CompileBackend::TargetName() const {
    return "sc_ls_fixed_v0";
}

void ScLsFixedV0CompileBackend::AddCompileOptions(qret::CompileOptionRegistrar& registrar) const {
    registrar.AddStringOption(
            "sc_ls_fixed_v0_topology",
            "FILE",
            "Path to the SC_LS_FIXED_V0 topology file."
    );
    registrar.AddStringOptionWithDefault(
            "sc_ls_fixed_v0_machine_type",
            "auto",
            "TYPE",
            "SC_LS_FIXED_V0 machine type: 'Dim2', 'Dim3', 'DistributedDim2', or "
            "'DistributedDim3' (currently unsupported). "
            "When 'auto' (default), the type is inferred from --sc_ls_fixed_v0_topology as the "
            "minimum required."
    );
    registrar.AddFlagOption(
            "sc_ls_fixed_v0_enable_pbc_mode",
            "Enable Pauli Based Computing lowering mode."
    );
    registrar.AddFlagOption(
            "sc_ls_fixed_v0_use_magic_state_cultivation",
            "Simulate magic-state factories using the cultivation method "
            "(requires --sc_ls_fixed_v0_magic_factory_seed_offset, "
            "--sc_ls_fixed_v0_prob_magic_state_creation)."
    );
    registrar.AddUInt64Option(
            "sc_ls_fixed_v0_magic_factory_seed_offset",
            0,
            "Base seed offset for RNG initialization of each magic-state factory. "
            "Required only if --sc_ls_fixed_v0_use_magic_state_cultivation=true."
    );
    registrar.AddUInt64Option(
            "sc_ls_fixed_v0_magic_generation_period",
            15,
            "Beats required to produce one magic state."
    );
    registrar.AddDoubleOption(
            "sc_ls_fixed_v0_prob_magic_state_creation",
            1.0,
            "Per-attempt success probability for magic-state creation. "
            "Required only if --sc_ls_fixed_v0_use_magic_state_cultivation=true."
    );
    registrar.AddUInt64Option(
            "sc_ls_fixed_v0_maximum_magic_state_stock",
            10000,
            "Maximum number of magic states storable in a factory."
    );
    registrar.AddUInt64Option(
            "sc_ls_fixed_v0_entanglement_generation_period",
            100,
            "Beats required to generate one entangled pair."
    );
    registrar.AddUInt64Option(
            "sc_ls_fixed_v0_maximum_entangled_state_stock",
            10,
            "Maximum number of entangled pairs storable in a factory."
    );
    registrar.AddUInt64Option(
            "sc_ls_fixed_v0_reaction_time",
            1,
            "Feed-forward latency in beats from measurement to error-corrected value."
    );
    registrar.AddDoubleOption(
            "sc_ls_fixed_v0_physical_error_rate",
            0.0,
            "Physical error rate p for logical error estimation."
    );
    registrar.AddDoubleOption(
            "sc_ls_fixed_v0_drop_rate",
            0.0,
            "Drop rate Lambda for logical error estimation."
    );
    registrar.AddDoubleOption(
            "sc_ls_fixed_v0_code_cycle_time_sec",
            0.0,
            "Code cycle time in seconds (t_cycle) for execution time estimation."
    );
    registrar.AddDoubleOption(
            "sc_ls_fixed_v0_allowed_failure_prob",
            0.0,
            "Allowed failure probability (eps) for logical error estimation."
    );
    registrar.AddStringOption(
            "sc_ls_fixed_v0_pass",
            "PASS",
            "SC_LS_FIXED_V0 compile pass to run. Accepts a single pass or a comma-separated "
            "list."
    );
}

bool ScLsFixedV0CompileBackend::Supports(const CompileFormat source) const {
    const auto source_supported = source == CompileFormat::IR || source == CompileFormat::OPENQASM2
            || source == CompileFormat::SC_LS_FIXED_V0;
    return source_supported;
}

bool ScLsFixedV0CompileBackend::Compile(
        const qret::CompileRequest& request,
        const qret::CompileOptionReader& options
) const {
    if (!Supports(request.source_format)) {
        std::cerr << "source format is not supported for target 'SC_LS_FIXED_V0'" << std::endl;
        return false;
    }
    if (request.source_format == qret::CompileFormat::IR && request.function_name.empty()) {
        std::cerr << "--function option is required" << std::endl;
        return false;
    }

    const auto topology = LoadTopology(options);
    if (!topology.has_value()) {
        return false;
    }
    const auto machine_option = GetMachineOption(options, *topology.value());
    if (!machine_option.has_value()) {
        return false;
    }
    const auto pass_config = options.Contains("sc_ls_fixed_v0_pass")
            ? options.GetPassConfigList("sc_ls_fixed_v0_pass")
            : GetDefaultPass(request.source_format);
    return RunCompilation(request, topology.value(), machine_option.value(), pass_config);
}
}  // namespace qret::sc_ls_fixed_v0
