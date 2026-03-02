#include "backend/backend.h"

#include <nanobind/make_iterator.h>
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/chrono.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/unique_ptr.h>
#include <nanobind/stl/vector.h>

#include <memory>
#include <ostream>

#include "qret/codegen/machine_function_pass.h"
#include "qret/pass.h"
#include "qret/target/sc_ls_fixed_v0/compile_info.h"
#include "qret/target/sc_ls_fixed_v0/external_pass.h"
#include "qret/target/sc_ls_fixed_v0/lowering.h"
#include "qret/target/sc_ls_fixed_v0/sc_ls_fixed_v0_target_machine.h"
#include "qret/target/target_machine.h"
#include "qret/transforms/ipo/inliner.h"
#include "qret/transforms/scalar/decomposition.h"
#include "qret/transforms/scalar/ignore_global_phase.h"
#include "qret/transforms/scalar/static_condition_pruning.h"

#include "backend/mf.h"
#include "common.h"
#include "frontend/pycircuit.h"

namespace pyqret {
using namespace qret;
namespace nb = nanobind;

struct PyScLsFixedV0Option {
    std::string topology;
    bool use_magic_state_cultivation = false;
    std::size_t magic_factory_seed_offset = 0;
    std::size_t magic_generation_period = 15;
    double prob_magic_state_creation = 1.0;
    std::size_t maximum_magic_state_stock = 10000;
    std::uint64_t entanglement_generation_period = 100;
    std::uint64_t maximum_entangled_state_stock = 10;
    std::size_t reaction_time = 1;
    double physical_error_rate = 0.0;
    double drop_rate = 0.0;
    double code_cycle_time_sec = 0.0;
    double allowed_failure_prob = 0.0;
};
std::ostream& operator<<(std::ostream& out, const PyScLsFixedV0Option& o) {
    return out << fmt::format(
                   "topology={},use_magic_state_cultivation={},magic_factory_seed_offset={},magic_"
                   "generation_period={},prob_magic_state_creation={},maximum_magic_state_stock={},"
                   "entanglement_generation_period={},maximum_entangled_state_stock={},reaction_"
                   "time={},physical_error_rate={},drop_rate={},code_cycle_time_sec={},"
                   "allowed_failure_prob={}",
                   o.topology,
                   o.use_magic_state_cultivation,
                   o.magic_factory_seed_offset,
                   o.magic_generation_period,
                   o.prob_magic_state_creation,
                   o.maximum_magic_state_stock,
                   o.entanglement_generation_period,
                   o.maximum_entangled_state_stock,
                   o.reaction_time,
                   o.physical_error_rate,
                   o.drop_rate,
                   o.code_cycle_time_sec,
                   o.allowed_failure_prob
           );
}

namespace {
void AddPassImpl(MFPassManager& manager, std::string_view pass_name) {
    const auto* pi = PassRegistry::GetPassRegistry()->GetPassInfo(pass_name);
    manager.AddPass((pi->GetNormalCtor())());
}
void AddExternalPassImpl(
        MFPassManager& manager,
        std::string_view pass_name,
        std::string_view cmd,
        std::string_view input,
        std::string_view output
) {
    manager.AddPass(
            std::unique_ptr<qret::sc_ls_fixed_v0::ExternalPass>(
                    new qret::sc_ls_fixed_v0::ExternalPass(pass_name, cmd, input, output, "")
            )
    );
}
std::unique_ptr<sc_ls_fixed_v0::ScLsFixedV0TargetMachine> CreateMachine(
        const PyScLsFixedV0Option& option
) {
    auto topology = sc_ls_fixed_v0::Topology::FromYAML(LoadFile(option.topology));
    auto ret = sc_ls_fixed_v0::ScLsFixedV0TargetMachine::New(
            topology,
            sc_ls_fixed_v0::ScLsFixedV0MachineOption{
                    .type = sc_ls_fixed_v0::GetMachineType(*topology),
                    .magic_generation_period = option.magic_generation_period,
                    .maximum_magic_state_stock = option.maximum_magic_state_stock,
                    .entanglement_generation_period = option.entanglement_generation_period,
                    .maximum_entangled_state_stock = option.maximum_entangled_state_stock,
                    .reaction_time = option.reaction_time,
                    .physical_error_rate = option.physical_error_rate,
                    .drop_rate = option.drop_rate,
                    .code_cycle_time_sec = option.code_cycle_time_sec,
                    .allowed_failure_prob = option.allowed_failure_prob
            }
    );
    return ret;
}
}  // namespace

enum class PyOptLevel : std::uint8_t { O0, O1, O2, O3, Os, Oz, Manual };
std::string ToString(PyOptLevel opt_level) {
    switch (opt_level) {
        case PyOptLevel::O0:
            return "O0";
        case PyOptLevel::O1:
            return "O1";
        case PyOptLevel::O2:
            return "O2";
        case PyOptLevel::O3:
            return "O3";
        case PyOptLevel::Os:
            return "Os";
        case PyOptLevel::Oz:
            return "Oz";
        case PyOptLevel::Manual:
            return "Manual";
    }
}

struct PyCompileOption {
    // General option
    PyOptLevel opt_level = PyOptLevel::O0;
    bool verbose = false;

    // TODO: add target field (sc_ls_fixed_v0, openqasm2, etc...)
    // The following is target-specific options
    PyScLsFixedV0Option sc_ls_fixed_v0_option;
};
std::ostream& operator<<(std::ostream& out, const PyCompileOption& o) {
    return out << fmt::format("verbose={},opt_level={}", o.verbose, ToString(o.opt_level));
}

struct PyCompiler {
    PyCompileOption option;
    MFPassManager manager;
    std::unique_ptr<TargetMachine> machine;
    MFAnalysis analysis;

    sc_ls_fixed_v0::ScLsFixedV0CompileInfo compile_info;

    explicit PyCompiler(const PyCompileOption& option)
        : option{option} {
        // TODO: Add different passes according to the `opt_level`
        if (option.opt_level != PyOptLevel::Manual) {
            AddPassImpl(manager, "sc_ls_fixed_v0::init_compile_info");
            AddPassImpl(manager, "sc_ls_fixed_v0::mapping");
            AddPassImpl(manager, "sc_ls_fixed_v0::routing");
            AddPassImpl(manager, "sc_ls_fixed_v0::calc_info_without_topology");
            AddPassImpl(manager, "sc_ls_fixed_v0::calc_info_with_topology");
        }

        // Set target machine
        machine = CreateMachine(option.sc_ls_fixed_v0_option);
    }

    MFAnalysis Compile(PyCircuit& circuit) {
        if (!circuit.HasIR()) {
            throw std::runtime_error("No IR format in circuit");
        }

        ir::DecomposeInst().RunOnFunction(circuit.GetIR());
        ir::IgnoreGlobalPhase().RunOnFunction(circuit.GetIR());
        ir::RecursiveInlinerPass().RunOnFunction(circuit.GetIR());
        ir::StaticConditionPruningPass().RunOnFunction(circuit.GetIR());
        circuit.mf = MachineFunction(
                static_cast<const sc_ls_fixed_v0::ScLsFixedV0TargetMachine*>(machine.get())
        );
        circuit.mf->SetIR(&circuit.GetIR());
        sc_ls_fixed_v0::Lowering().RunOnMachineFunction(*circuit.mf);
        manager.Run(circuit.mf.value());
        const auto& ret = manager.GetAnalysis();
        if (circuit.mf->GetCompileInfo() != nullptr) {
            compile_info = *static_cast<const sc_ls_fixed_v0::ScLsFixedV0CompileInfo*>(
                    circuit.mf->GetCompileInfo()
            );
        }
        return ret;
    }
};

void BindOptLevel(nb::module_& m) {
    nb::enum_<PyOptLevel>(m, "OptLevel", "Optimization level")
            .value("O0", PyOptLevel::O0)
            .value("O1", PyOptLevel::O1)
            .value("O2", PyOptLevel::O2)
            .value("O3", PyOptLevel::O3)
            .value("Os", PyOptLevel::Os)
            .value("Oz", PyOptLevel::Oz)
            .value("Manual", PyOptLevel::Manual);
}
void BindCompileOption(nb::module_& m) {
    using namespace nb::literals;
    {
        auto def = PyCompileOption();
        nb::class_<PyCompileOption>(m, "CompileOption")
                .def("__init__",
                     [](PyCompileOption* t, PyOptLevel opt_level, bool verbose) {
                         new (t) PyCompileOption{opt_level, verbose};
                     })
                .def_rw("verbose", &PyCompileOption::verbose)
                .def_rw("opt_level", &PyCompileOption::opt_level)
                .def_rw("sc_ls_fixed_v0_option", &PyCompileOption::sc_ls_fixed_v0_option)
                .def("__str__", BindFmtUsingOstream<PyCompileOption>);
    }
    {
        auto def = PyScLsFixedV0Option();
        nb::class_<PyScLsFixedV0Option>(m, "ScLsFixedV0Option")
                .def("__init__",
                     [](PyScLsFixedV0Option* t,
                        const std::string& topology,
                        bool use_magic_state_cultivation,
                        std::size_t magic_factory_seed_offset,
                        std::size_t magic_generation_period,
                        double prob_magic_state_creation,
                        std::size_t maximum_magic_state_stock,
                        std::size_t entanglement_generation_period,
                        std::size_t maximum_entangled_state_stock,
                        std::size_t reaction_time,
                        double physical_error_rate,
                        double drop_rate,
                        double code_cycle_time_sec,
                        double allowed_failure_prob) {
                         new (t) PyScLsFixedV0Option{
                                 topology,
                                 use_magic_state_cultivation,
                                 magic_factory_seed_offset,
                                 magic_generation_period,
                                 prob_magic_state_creation,
                                 maximum_magic_state_stock,
                                 entanglement_generation_period,
                                 maximum_entangled_state_stock,
                                 reaction_time,
                                 physical_error_rate,
                                 drop_rate,
                                 code_cycle_time_sec,
                                 allowed_failure_prob
                         };
                     })
                .def_rw("topology", &PyScLsFixedV0Option::topology)
                .def_rw("use_magic_state_cultivation",
                        &PyScLsFixedV0Option::use_magic_state_cultivation)
                .def_rw("magic_factory_seed_offset",
                        &PyScLsFixedV0Option::magic_factory_seed_offset)
                .def_rw("magic_generation_period", &PyScLsFixedV0Option::magic_generation_period)
                .def_rw("prob_magic_state_creation",
                        &PyScLsFixedV0Option::prob_magic_state_creation)
                .def_rw("maximum_magic_state_stock",
                        &PyScLsFixedV0Option::maximum_magic_state_stock)
                .def_rw("entanglement_generation_period",
                        &PyScLsFixedV0Option::entanglement_generation_period)
                .def_rw("maximum_entangled_state_stock",
                        &PyScLsFixedV0Option::maximum_entangled_state_stock)
                .def_rw("reaction_time", &PyScLsFixedV0Option::reaction_time)
                .def_rw("physical_error_rate", &PyScLsFixedV0Option::physical_error_rate)
                .def_rw("drop_rate", &PyScLsFixedV0Option::drop_rate)
                .def_rw("code_cycle_time_sec", &PyScLsFixedV0Option::code_cycle_time_sec)
                .def_rw("allowed_failure_prob", &PyScLsFixedV0Option::allowed_failure_prob)
                .def("__str__", BindFmtUsingOstream<PyScLsFixedV0Option>);
    }
}
void BindCompiler(nb::module_& m) {
    nb::class_<PyCompiler>(m, "Compiler")
            .def(nb::init<PyCompileOption>())
            .def_ro("option", &PyCompiler::option)
            .def_static(
                    "get_available_passes",
                    []() {
                        auto ret = std::vector<std::string>();
                        const auto* registry = PassRegistry::GetPassRegistry();
                        for (const auto& [name, pi] : *registry) {
                            ret.emplace_back(name);
                        }
                        return ret;
                    }
            )
            .def("add_pass",
                 [](PyCompiler& compiler, std::string_view pass_name) {
                     AddPassImpl(compiler.manager, pass_name);
                 })
            .def("add_external_pass",
                 [](PyCompiler& compiler,
                    std::string_view pass_name,
                    std::string_view cmd,
                    std::string_view input,
                    std::string_view output) {
                     AddExternalPassImpl(compiler.manager, pass_name, cmd, input, output);
                 })
            .def("get_pass_list",
                 [](const PyCompiler& c) {
                     auto ret = std::vector<std::string>();
                     for (const auto& pass : c.manager) {
                         ret.emplace_back(pass->GetPassName());
                     }
                     return ret;
                 })
            .def("compile", [](PyCompiler& c, PyCircuit& circuit) { return c.Compile(circuit); })
            .def("get_compile_info", [](const PyCompiler& c) { return c.compile_info; });

    nb::class_<MFAnalysis>(m, "CompileResult")
            .def("get_run_order",
                 [](const MFAnalysis& a) {
                     auto ret = std::vector<std::string>();
                     for (const auto& pass : a.run_order) {
                         ret.emplace_back(pass->GetPassName());
                     }
                     return ret;
                 })
            .def("get_elapsed_time", [](const MFAnalysis& a) {
                auto ret = std::vector<MFAnalysis::Time>();
                for (const auto& pass : a.run_order) {
                    ret.emplace_back(a.elapsed_ms.at(pass));
                }
                return ret;
            });
}
void BindCompileInfo(nb::module_& m) {
    using sc_ls_fixed_v0::ScLsFixedV0CompileInfo;
    nb::class_<ScLsFixedV0CompileInfo>(m, "ScLsFixedV0CompileInfo")
            // fields
            .def_ro("magic_generation_period", &ScLsFixedV0CompileInfo::magic_generation_period)
            .def_ro("maximum_magic_state_stock", &ScLsFixedV0CompileInfo::maximum_magic_state_stock)
            .def_ro("reaction_time", &ScLsFixedV0CompileInfo::reaction_time)
            .def_ro("runtime", &ScLsFixedV0CompileInfo::runtime)
            .def_ro("runtime_without_topology", &ScLsFixedV0CompileInfo::runtime_without_topology)
            .def_ro("gate_count", &ScLsFixedV0CompileInfo::gate_count)
            .def("count_per_gate",
                 [](const ScLsFixedV0CompileInfo& info) {
                     auto ret = std::map<std::string, std::uint64_t>();
                     for (const auto& [type, count] : info.gate_count_dict) {
                         ret.emplace(ToString(type), count);
                     }
                     return ret;
                 })
            .def_ro("gate_depth", &ScLsFixedV0CompileInfo::gate_depth)
            .def_ro("gate_throughput", &ScLsFixedV0CompileInfo::gate_throughput)
            .def_ro("measurement_feedback_count",
                    &ScLsFixedV0CompileInfo::measurement_feedback_count)
            .def_ro("measurement_feedback_depth",
                    &ScLsFixedV0CompileInfo::measurement_feedback_depth)
            .def_ro("measurement_feedback_rate", &ScLsFixedV0CompileInfo::measurement_feedback_rate)
            .def_ro("runtime_estimation_measurement_feedback_count",
                    &ScLsFixedV0CompileInfo::runtime_estimation_measurement_feedback_count)
            .def_ro("runtime_estimation_measurement_feedback_depth",
                    &ScLsFixedV0CompileInfo::runtime_estimation_measurement_feedback_depth)
            .def_ro("magic_state_consumption_count",
                    &ScLsFixedV0CompileInfo::magic_state_consumption_count)
            .def_ro("magic_state_consumption_depth",
                    &ScLsFixedV0CompileInfo::magic_state_consumption_depth)
            .def_ro("magic_state_consumption_rate",
                    &ScLsFixedV0CompileInfo::magic_state_consumption_rate)
            .def_ro("runtime_estimation_magic_state_consumption_count",
                    &ScLsFixedV0CompileInfo::runtime_estimation_magic_state_consumption_count)
            .def_ro("runtime_estimation_magic_state_consumption_depth",
                    &ScLsFixedV0CompileInfo::runtime_estimation_magic_state_consumption_depth)
            .def_ro("magic_factory_count", &ScLsFixedV0CompileInfo::magic_factory_count)
            .def_ro("entanglement_consumption_count",
                    &ScLsFixedV0CompileInfo::entanglement_consumption_count)
            .def_ro("entanglement_consumption_depth",
                    &ScLsFixedV0CompileInfo::entanglement_consumption_depth)
            .def_ro("entanglement_consumption_rate",
                    &ScLsFixedV0CompileInfo::entanglement_consumption_rate)
            .def_ro("runtime_estimation_entanglement_consumption_count",
                    &ScLsFixedV0CompileInfo::runtime_estimation_entanglement_consumption_count)
            .def_ro("runtime_estimation_entanglement_consumption_depth",
                    &ScLsFixedV0CompileInfo::runtime_estimation_entanglement_consumption_depth)
            .def_ro("entanglement_factory_count",
                    &ScLsFixedV0CompileInfo::entanglement_factory_count)
            .def_ro("chip_cell_count", &ScLsFixedV0CompileInfo::chip_cell_count)
            .def_ro("chip_cell_algorithmic_qubit",
                    &ScLsFixedV0CompileInfo::chip_cell_algorithmic_qubit)
            .def_ro("chip_cell_algorithmic_qubit_ratio",
                    &ScLsFixedV0CompileInfo::chip_cell_algorithmic_qubit_ratio)
            .def_ro("chip_cell_active_qubit_area",
                    &ScLsFixedV0CompileInfo::chip_cell_active_qubit_area)
            .def_ro("chip_cell_active_qubit_area_ratio",
                    &ScLsFixedV0CompileInfo::chip_cell_active_qubit_area_ratio)
            .def_ro("qubit_volume", &ScLsFixedV0CompileInfo::qubit_volume)
            .def_ro("code_distance", &ScLsFixedV0CompileInfo::code_distance)
            .def_ro("execution_time_sec", &ScLsFixedV0CompileInfo::execution_time_sec)
            .def_ro("num_physical_qubits", &ScLsFixedV0CompileInfo::num_physical_qubits)
            // methods
            .def("to_json", [](const ScLsFixedV0CompileInfo& info) { return info.Json().dump(); })
            .def("to_markdown", [](const ScLsFixedV0CompileInfo& info) { return info.Markdown(); });
}

void BindBackend(nanobind::module_& m) {
    BindOptLevel(m);
    BindCompileOption(m);
    BindCompiler(m);
    BindMF(m);
    BindCompileInfo(m);
}
}  // namespace pyqret
