#include "frontend/frontend.h"

#include <nanobind/make_iterator.h>
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/unique_ptr.h>
#include <nanobind/stl/vector.h>

#include <cassert>
#include <stdexcept>
#include <string_view>

#include "qret/frontend/argument.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/openqasm2.h"
#include "qret/ir/module.h"
#include "qret/parser/openqasm2.h"
#include "qret/parser/openqasm3.h"

#include "common.h"
#include "frontend/gate.h"
#include "frontend/ir.h"
#include "frontend/pycircuit.h"
#include "frontend/transforms/inliner.h"

namespace pyqret {
using namespace qret::frontend;
namespace nb = nanobind;

template <ObjectType type_, SizeType size_>
void BindQuantumObject(nb::module_& m, const std::string& class_name) {
    using QuantumObject = QuantumBase<type_, size_>;
    using SingleQuantumObject = QuantumBase<type_, SizeType::Single>;
    // using ArrayQuantumObject = QuantumBase<type_, SizeType::Array>;

    auto bind = nb::class_<QuantumObject>(m, class_name.c_str());
    bind.def("is_qubit", [](const QuantumObject& q) { return q.IsQubit(); })
            .def("is_register", [](const QuantumObject& q) { return q.IsRegister(); })
            .def("size", [](const QuantumObject& q) { return q.Size(); })
            .def("__str__", BindFmtUsingOstream<QuantumObject>);

    if constexpr (size_ == SizeType::Single) {
        bind.def("get_id", [](const QuantumObject& q) {
                return q.GetId();
            }).def(nb::self + nb::self);
    }
    if constexpr (size_ == SizeType::Array) {
        bind.def("range",
                 [](const QuantumObject& q, std::uint64_t start, std::uint64_t size) {
                     return q.Range(start, size);
                 })
                .def("__getitem__",
                     [](const QuantumObject& q, std::uint64_t idx) { return q[idx]; })
                .def(nb::self + nb::self)
                .def("__add__",
                     [](const QuantumObject& lhs, const SingleQuantumObject& rhs) {
                         return lhs + rhs;
                     })
                .def("__radd__",
                     [](const QuantumObject& rhs, const SingleQuantumObject& lhs) {
                         return lhs + rhs;
                     })
                .def(nb::self += nb::self, nb::rv_policy::none)
                .def("__iadd__",
                     [](const QuantumObject& lhs, const SingleQuantumObject& rhs) {
                         return lhs + rhs;
                     })
                .def(
                        "__iter__",
                        [](const QuantumObject& q) {
                            return nb::make_iterator(
                                    nb::type<QuantumObject>(),
                                    "QuantumIterator",
                                    q.begin(),
                                    q.end()
                            );
                        },
                        // Nurse = return value, Patient = q
                        // "q" should be kept alive at least until "return value" is
                        // freed by the garbage collector.
                        nb::keep_alive<0, 1>()
                );
    }
}

void BindCircuitBuilder(nb::module_& m) {
    nb::class_<CircuitBuilder>(m, "CircuitBuilder")
            .def(
                    "__init__",
                    [](CircuitBuilder* builder, qret::ir::Module& m) {
                        new (builder) CircuitBuilder(&m);
                    },
                    // Nurse = builder, Patient = m
                    // "m" should be kept alive at least until "builder" is freed by the garbage
                    // collector.
                    nb::keep_alive<1, 2>()
            )
            .def("_load_module",
                 [](CircuitBuilder& builder, ir::Module& module) { builder.LoadModule(&module); })
            .def("contains",
                 [](const CircuitBuilder& builder, std::string_view name) {
                     return builder.Contains(name);
                 })
            .def("get_circuit",
                 [](const CircuitBuilder& builder, std::string_view name) {
                     auto ret = PyCircuit();
                     ret.frontend = builder.GetCircuit(name);
                     ret.ir = ret.frontend->GetIR();
                     const auto& args = ret.GetFrontend().GetArgument();
                     auto pyarg = PyArgument();
                     for (const auto& [name, type, size, attr] : args) {
                         pyarg.Add(name, type, size, attr, size != 1);
                     }
                     ret.argument = pyarg;
                     return ret;
                 })
            .def("get_circuit_list", &CircuitBuilder::GetCircuitList)
            .def("parse_openqasm2",
                 [](CircuitBuilder& builder, const std::string& input, const std::string& name) {
                     auto ast = qret::openqasm2::ParseOpenQASM2File(input);
                     auto* circuit = qret::frontend::BuildCircuitFromAST(ast, builder, name);

                     auto pyarg = PyArgument();
                     pyarg.SetBuilder(&builder);
                     const auto& args = circuit->GetArgument();
                     for (const auto& [name, type, size, attr] : args) {
                         pyarg.Add(name, type, size, attr, size != 1);
                     }
                     auto ret = PyCircuit{
                             .argument = pyarg,
                             .frontend = circuit,
                             .ir = circuit->GetIR()
                     };
                     return ret;
                 })
            .def("parse_openqasm3",
                 [](CircuitBuilder& builder, const std::string& input, const std::string& name) {
                     auto ast = qret::openqasm3::ParseOpenQASM3File(input);
                     auto* circuit = qret::frontend::BuildCircuitFromAST(ast, builder, name);

                     auto pyarg = PyArgument();
                     pyarg.SetBuilder(&builder);
                     const auto& args = circuit->GetArgument();
                     for (const auto& [name, type, size, attr] : args) {
                         pyarg.Add(name, type, size, attr, size != 1);
                     }
                     auto ret = PyCircuit{
                             .argument = pyarg,
                             .frontend = circuit,
                             .ir = circuit->GetIR()
                     };
                     return ret;
                 })
            .def(
                    "_create_circuit",
                    [](CircuitBuilder& builder, std::string_view name, const PyArgument& pyarg) {
                        auto* circuit = builder.CreateCircuit(name);
                        const auto keys = pyarg.Keys();
                        for (const auto& name : keys) {
                            const auto& info = pyarg.GetArgInfo(name);
                            circuit->GetMutArgument()
                                    .Add(name, info.GetType(), info.size, info.attribute);
                        }
                        auto ret = PyCircuit{
                                .argument = pyarg,
                                .frontend = circuit,
                                .ir = circuit->GetIR()
                        };
                        ret.argument.SetBuilder(&builder);
                        return ret;
                    },
                    // Nurse = return value, Patient = builder
                    // "builder" should be kept alive at least until "return value" is freed by the
                    // garbage collector.
                    nb::keep_alive<0, 1>()
            )
            .def("_begin_circuit_definition",
                 [](CircuitBuilder& builder, PyCircuit& py) {
                     if (!py.HasFrontend()) {
                         throw std::runtime_error("No frontend format in circuit");
                     }
                     builder.BeginCircuitDefinition(&py.GetFrontend());
                 })
            .def("_end_circuit_definition",
                 [](CircuitBuilder& builder, PyCircuit& py) {
                     if (!py.HasFrontend()) {
                         throw std::runtime_error("No frontend format in circuit");
                     }
                     builder.EndCircuitDefinition(&py.GetFrontend());
                 })
            .def("_get_temporal_register",
                 [](CircuitBuilder& builder) -> Register {
                     auto* ir = builder.GetCurrentCircuit()->GetIR();
                     const auto id = ir->GetNumRegisters() + ir->GetNumTmpRegisters();
                     ir->IncNumTmpRegisters();
                     return builder.GetRegister(id);
                 })
            .def("_get_temporal_registers",
                 [](CircuitBuilder& builder, std::size_t size) -> Registers {
                     auto* ir = builder.GetCurrentCircuit()->GetIR();
                     const auto offset = ir->GetNumRegisters() + ir->GetNumTmpRegisters();
                     ir->AddNumTmpRegisters(size);
                     return builder.GetRegisters(offset, size);
                 });
}

void BindFrontend(nanobind::module_& m) {
    BindCircuitBuilder(m);

    BindQuantumObject<ObjectType::Qubit, SizeType::Single>(m, "Qubit");
    BindQuantumObject<ObjectType::Register, SizeType::Single>(m, "Register");
    BindQuantumObject<ObjectType::Qubit, SizeType::Array>(m, "Qubits");
    BindQuantumObject<ObjectType::Register, SizeType::Array>(m, "Registers");

    // pycircuit.h
    BindPyCircuit(m);

    // gate.h
    {
        auto m_gate = m.def_submodule("gate");
        {
            auto m_intrinsic = m_gate.def_submodule("intrinsic");
            BindIntrinsicGate(m_intrinsic);
        }
        {
            auto m_cf = m_gate.def_submodule("control_flow");
            BindControlFlow(m_cf);
        }
    }

    // ir.h
    BindIR(m);

    // transforms
    {
        // inliner.h
        auto m_transforms = m.def_submodule("transforms");
        BindInliner(m_transforms);
    }
}
}  // namespace pyqret
