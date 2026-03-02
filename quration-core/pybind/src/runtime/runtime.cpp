#include "runtime/runtime.h"

#include <nanobind/make_iterator.h>
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/operators.h>
#include <nanobind/stl/complex.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/unique_ptr.h>
#include <nanobind/stl/vector.h>

#include <stdexcept>
#include <string_view>

#include "qret/base/type.h"
#include "qret/runtime/full_quantum_state.h"
#include "qret/runtime/simulator.h"
#include "qret/runtime/toffoli_state.h"

#include "frontend/pycircuit.h"

namespace pyqret {
namespace nb = nanobind;
using qret::BigInt;
using namespace qret::runtime;

void BindRuntime(nanobind::module_& m) {
    using QuantumStateType = SimulatorConfig::StateType;
    nb::enum_<QuantumStateType>(m, "QuantumStateType", "Type of quantum state to use in simulator.")
            .value("Toffoli", SimulatorConfig::StateType::Toffoli, "Use ToffoliState")
            .value("FullQuantum", SimulatorConfig::StateType::FullQuantum, "Use FullQuantumState");

    nb::class_<SimulatorConfig>(m, "SimulatorConfig")
            .def("__init__",
                 [](SimulatorConfig* config,
                    const QuantumStateType state_type,
                    std::uint64_t seed,
                    std::uint64_t max_superpositions,
                    bool save_operation_matrix) {
                     new (config) SimulatorConfig{
                             .state_type = state_type,
                             .seed = seed,
                             .max_superpositions = max_superpositions,
                             .use_qulacs = false,
                             .save_operation_matrix = save_operation_matrix,
                     };
                 })
            .def_rw("state_type", &SimulatorConfig::state_type)
            .def_rw("seed", &SimulatorConfig::seed)
            .def_rw("max_superpositions", &SimulatorConfig::max_superpositions)
            .def_rw("use_qulacs", &SimulatorConfig::use_qulacs)
            .def_rw("save_operation_matrix", &SimulatorConfig::save_operation_matrix);

    nb::class_<ToffoliState>(m, "ToffoliState")
            .def("get_num_superpositions", &ToffoliState::GetNumSuperpositions)
            .def("is_computational_basis", &ToffoliState::IsComputationalBasis)
            .def("calc_0_prob", &ToffoliState::Calc0Prob)
            .def("calc_1_prob", &ToffoliState::Calc1Prob)
            .def("is_superposed", &ToffoliState::IsSuperposed)
            .def("get_global_phase", [](const ToffoliState& state) { return state.GetPhase(); })
            .def("get_value", &ToffoliState::GetValue)
            .def("get_raw_vector", [](const ToffoliState& state) {
                auto tmp = state.GetRawVector();
                auto ret = std::vector<
                        std::pair<std::complex<double>, std::vector<std::int_fast8_t>>>();
                for (const auto& [coef, s] : tmp) {
                    ret.emplace_back(coef, s);
                }
                return ret;
            });

    nb::class_<FullQuantumState>(m, "FullQuantumState")
            .def("get_num_superpositions", &FullQuantumState::GetNumSuperpositions)
            .def("is_computational_basis", &FullQuantumState::IsComputationalBasis)
            .def("calc_0_prob", &FullQuantumState::Calc0Prob)
            .def("calc_1_prob", &FullQuantumState::Calc1Prob)
            .def("is_superposed", &FullQuantumState::IsSuperposed)
            .def("get_entropy", &FullQuantumState::GetEntropy)
            .def("sampling", &FullQuantumState::Sampling)
            .def(
                    "get_state_vector",
                    [](const FullQuantumState& state) {
                        const auto dim = 1ULL << state.NumQubits();
                        auto* ptr = state.GetStateVector();
                        if (ptr == nullptr) {
                            throw std::runtime_error("State vector is null.");
                        }

                        using RetType =
                                nb::ndarray<const std::complex<double>, nb::numpy, nb::shape<-1>>;
                        return RetType(
                                ptr,
                                {dim},  // Shape
                                nb::handle()
                        );
                    },
                    // Nurse = return value, Patient = state
                    // "state" should be kept alive at least until "return value" is freed by the
                    // garbage collector.
                    nb::keep_alive<0, 1>()
            )
            .def(
                    "get_operation_matrix",
                    [](const FullQuantumState& state) {
                        const auto dim = 1ULL << state.NumQubits();
                        auto* ptr = state.GetOperationMatrix();
                        if (ptr == nullptr) {
                            throw std::runtime_error("Operation matrix is null.");
                        }

                        using RetType = nb::
                                ndarray<const std::complex<double>, nb::numpy, nb::shape<-1, -1>>;
                        return RetType(
                                ptr,
                                {dim, dim},  // Shape: {rows, cols}
                                nb::handle()
                        );
                    },
                    // Nurse = return value, Patient = state
                    // "state" should be kept alive at least until "return value" is freed by the
                    // garbage collector.
                    nb::keep_alive<0, 1>()
            );

    nb::class_<Simulator>(m, "Simulator")
            .def(
                    "__init__",
                    [](Simulator* sim, const SimulatorConfig& config, const PyCircuit& func) {
                        if (!func.HasIR()) {
                            throw std::runtime_error("No IR function in input object");
                        }
                        new (sim) Simulator(config, &func.GetIR());
                    },
                    // Nurse = sim, Patient = function object
                    // "function object" should be kept alive until "sim" is freed by the garbage
                    // collector.
                    nb::keep_alive<1, 3>()
            )
            .def("get_config",
                 &Simulator::GetConfig,
                 nb::rv_policy::reference_internal,
                 // Nurse = return value, Patient = sim
                 // "sim" should be kept alive at least until "return value" is
                 // freed by the garbage collector.
                 nb::keep_alive<0, 1>())
            .def("set",
                 [](Simulator& sim, std::string_view name, const BigInt& value) {
                     sim.SetValueByName(name, value);
                 })
            .def("run",
                 [](Simulator& sim) {
                     auto ret = std::map<std::string, std::vector<bool>>();
                     sim.RunAll();
                     const auto& state = sim.GetState();
                     const auto* const func = sim.GetFunction();
                     const auto& argument = func->GetArgument();

                     for (const auto& [name, _] : argument.registers) {
                         ret[name] = state.ReadRegistersByName(name);
                     }
                     return ret;
                 })
            .def(
                    "get_state",
                    [](const Simulator& sim) {
                        const auto& config = sim.GetConfig();
                        switch (config.state_type) {
                            case SimulatorConfig::StateType::Toffoli:
                                return nb::cast(sim.GetToffoliState(), nb::rv_policy::reference);
                            case SimulatorConfig::StateType::FullQuantum:
                                return nb::cast(
                                        sim.GetFullQuantumState(),
                                        nb::rv_policy::reference
                                );
                            default:
                                throw std::runtime_error(
                                        "only Toffoli and FullQuantum is supported"
                                );
                        }
                    },
                    // Nurse = return value, Patient = sim
                    // "sim" should be kept alive at least until "return value" is
                    // freed by the garbage collector.
                    nb::keep_alive<0, 1>()
            )
            .def(
                    "get_toffoli_state",
                    [](const Simulator& sim) -> auto& { return sim.GetToffoliState(); },
                    nb::rv_policy::reference,
                    // Nurse = return value, Patient = sim
                    // "sim" should be kept alive at least until "return value" is
                    // freed by the garbage collector.
                    nb::keep_alive<0, 1>()
            )
            .def(
                    "get_full_quantum_state",
                    [](const Simulator& sim) -> auto& { return sim.GetFullQuantumState(); },
                    nb::rv_policy::reference,
                    // Nurse = return value, Patient = sim
                    // "sim" should be kept alive at least until "return value" is
                    // freed by the garbage collector.
                    nb::keep_alive<0, 1>()
                    //
            );
}
}  // namespace pyqret
