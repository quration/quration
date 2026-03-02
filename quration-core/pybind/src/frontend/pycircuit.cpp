#include "frontend/pycircuit.h"

#include <nanobind/make_iterator.h>
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/unique_ptr.h>
#include <nanobind/stl/vector.h>

#include <string_view>

#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"

namespace pyqret {
namespace nb = nanobind;
using namespace qret::frontend;

void BindPyCircuit(nanobind::module_& m) {
    nb::enum_<Circuit::Type>(m, "QuantumType", "Type of quantum object.")
            .value("Qubit", Circuit::Type::Qubit)
            .value("Register", Circuit::Type::Register);

    nb::enum_<Circuit::Attribute>(
            m,
            "QuantumAttribute",
            "Attribute for quantum circuit's argument."
    )
            // .value("None", Circuit::Attribute::None)
            .value("Operate", Circuit::Attribute::Operate)
            .value("CleanAncilla", Circuit::Attribute::CleanAncilla)
            .value("DirtyAncilla", Circuit::Attribute::DirtyAncilla)
            .value("Catalyst", Circuit::Attribute::Catalyst)
            .value("Input", Circuit::Attribute::Input)
            .value("Output", Circuit::Attribute::Output);

    nb::class_<PyArgInfo>(m, "ArgInfo")
            // fields
            .def_static(
                    "operate",
                    [](std::string_view arg_name) {
                        return PyArgInfo{
                                .arg_name = std::string(arg_name),
                                .size = 1,
                                .attribute = Circuit::Attribute::Operate,
                                .is_array = false
                        };
                    }
            )
            .def_static(
                    "operates",
                    [](std::string_view arg_name, std::size_t size) {
                        return PyArgInfo{
                                .arg_name = std::string(arg_name),
                                .size = size,
                                .attribute = Circuit::Attribute::Operate,
                                .is_array = true
                        };
                    }
            )
            .def_static(
                    "clean_ancilla",
                    [](std::string_view arg_name) {
                        return PyArgInfo{
                                .arg_name = std::string(arg_name),
                                .size = 1,
                                .attribute = Circuit::Attribute::CleanAncilla,
                                .is_array = false
                        };
                    }
            )
            .def_static(
                    "clean_ancillae",
                    [](std::string_view arg_name, std::size_t size) {
                        return PyArgInfo{
                                .arg_name = std::string(arg_name),
                                .size = size,
                                .attribute = Circuit::Attribute::CleanAncilla,
                                .is_array = true
                        };
                    }
            )
            .def_static(
                    "dirty_ancilla",
                    [](std::string_view arg_name) {
                        return PyArgInfo{
                                .arg_name = std::string(arg_name),
                                .size = 1,
                                .attribute = Circuit::Attribute::DirtyAncilla,
                                .is_array = false
                        };
                    }
            )
            .def_static(
                    "dirty_ancillae",
                    [](std::string_view arg_name, std::size_t size) {
                        return PyArgInfo{
                                .arg_name = std::string(arg_name),
                                .size = size,
                                .attribute = Circuit::Attribute::DirtyAncilla,
                                .is_array = true
                        };
                    }
            )
            .def_static(
                    "catalyst",
                    [](std::string_view arg_name) {
                        return PyArgInfo{
                                .arg_name = std::string(arg_name),
                                .size = 1,
                                .attribute = Circuit::Attribute::Catalyst,
                                .is_array = false
                        };
                    }
            )
            .def_static(
                    "catalysts",
                    [](std::string_view arg_name, std::size_t size) {
                        return PyArgInfo{
                                .arg_name = std::string(arg_name),
                                .size = size,
                                .attribute = Circuit::Attribute::Catalyst,
                                .is_array = true
                        };
                    }
            )
            .def_static(
                    "input",
                    [](std::string_view arg_name) {
                        return PyArgInfo{
                                .arg_name = std::string(arg_name),
                                .size = 1,
                                .attribute = Circuit::Attribute::Input,
                                .is_array = false
                        };
                    }
            )
            .def_static(
                    "inputs",
                    [](std::string_view arg_name, std::size_t size) {
                        return PyArgInfo{
                                .arg_name = std::string(arg_name),
                                .size = size,
                                .attribute = Circuit::Attribute::Input,
                                .is_array = true
                        };
                    }
            )
            .def_static(
                    "output",
                    [](std::string_view arg_name) {
                        return PyArgInfo{
                                .arg_name = std::string(arg_name),
                                .size = 1,
                                .attribute = Circuit::Attribute::Output,
                                .is_array = false
                        };
                    }
            )
            .def_static(
                    "outputs",
                    [](std::string_view arg_name, std::size_t size) {
                        return PyArgInfo{
                                .arg_name = std::string(arg_name),
                                .size = size,
                                .attribute = Circuit::Attribute::Output,
                                .is_array = true
                        };
                    }
            )
            .def_ro("arg_name", &PyArgInfo::arg_name)
            .def_ro("size", &PyArgInfo::size)
            .def_ro("attribute", &PyArgInfo::attribute)
            .def_ro("is_array", &PyArgInfo::is_array)
            .def("type", [](const PyArgInfo& arg) { return arg.GetType(); })
            .def("__str__", [](const PyArgInfo& arg_info) {
                return fmt::format(
                        "arg_name = {}, type = {}, size = {}, attribute = {}, is_array = {}",
                        arg_info.arg_name,
                        arg_info.Type(),
                        arg_info.size,
                        arg_info.Attribute(),
                        arg_info.is_array
                );
            });

    nb::class_<PyArgument>(m, "Argument")
            .def(nb::init<>())
            .def("add",
                 [](PyArgument& arg, const PyArgInfo& info) {
                     arg.Add(info.arg_name,
                             info.GetType(),
                             info.size,
                             info.attribute,
                             info.is_array);
                 })
            .def("add_operate",
                 [](PyArgument& arg, std::string_view arg_name) {
                     arg.Add(arg_name, Circuit::Type::Qubit, 1, Circuit::Attribute::Operate, false);
                 })
            .def("add_operates",
                 [](PyArgument& arg, std::string_view arg_name, std::uint64_t size) {
                     arg.Add(arg_name,
                             Circuit::Type::Qubit,
                             size,
                             Circuit::Attribute::Operate,
                             true);
                 })
            .def("add_clean_ancilla",
                 [](PyArgument& arg, std::string_view arg_name) {
                     arg.Add(arg_name,
                             Circuit::Type::Qubit,
                             1,
                             Circuit::Attribute::CleanAncilla,
                             false);
                 })
            .def("add_clean_ancillae",
                 [](PyArgument& arg, std::string_view arg_name, std::uint64_t size) {
                     arg.Add(arg_name,
                             Circuit::Type::Qubit,
                             size,
                             Circuit::Attribute::CleanAncilla,
                             true);
                 })
            .def("add_dirty_ancilla",
                 [](PyArgument& arg, std::string_view arg_name) {
                     arg.Add(arg_name,
                             Circuit::Type::Qubit,
                             1,
                             Circuit::Attribute::DirtyAncilla,
                             false);
                 })
            .def("add_dirty_ancillae",
                 [](PyArgument& arg, std::string_view arg_name, std::uint64_t size) {
                     arg.Add(arg_name,
                             Circuit::Type::Qubit,
                             size,
                             Circuit::Attribute::DirtyAncilla,
                             true);
                 })
            .def("add_input",
                 [](PyArgument& arg, std::string_view arg_name) {
                     arg.Add(arg_name,
                             Circuit::Type::Register,
                             1,
                             Circuit::Attribute::Input,
                             false);
                 })
            .def("add_inputs",
                 [](PyArgument& arg, std::string_view arg_name, std::uint64_t size) {
                     arg.Add(arg_name,
                             Circuit::Type::Register,
                             size,
                             Circuit::Attribute::Input,
                             true);
                 })
            .def("add_output",
                 [](PyArgument& arg, std::string_view arg_name) {
                     arg.Add(arg_name,
                             Circuit::Type::Register,
                             1,
                             Circuit::Attribute::Output,
                             false);
                 })
            .def("add_outputs",
                 [](PyArgument& arg, std::string_view arg_name, std::uint64_t size) {
                     arg.Add(arg_name,
                             Circuit::Type::Register,
                             size,
                             Circuit::Attribute::Output,
                             true);
                 })
            .def("get",
                 [](const PyArgument& arg, std::string_view arg_name) {
                     return arg.PyGet(arg_name);
                 })
            .def("get_num_args", &PyArgument::GetNumArgs)
            .def("get_arg_names", [](const PyArgument& arg) { return arg.Keys(); })
            .def("view_arg_info",
                 [](const PyArgument& arg, std::string_view arg_name) {
                     return arg.GetArgInfo(arg_name);
                 })
            .def("get_operate", &PyArgument::GetOperate)
            .def("get_clean_ancilla", &PyArgument::GetCleanAncilla)
            .def("get_dirty_ancilla", &PyArgument::GetDirtyAncilla)
            .def("get_catalyst", &PyArgument::GetCatalyst)
            .def("get_input", &PyArgument::GetInput)
            .def("get_output", &PyArgument::GetOutput)
            .def("_get_start_and_size", &PyArgument::GetStartAndSize)
            .def("__getitem__", [](const PyArgument& arg, std::string_view arg_name) {
                return arg.PyGet(arg_name);
            });

    nb::class_<PyCircuit>(m, "Circuit")
            .def_rw("argument", &PyCircuit::argument)
            .def("_call_impl",
                 [](const PyCircuit& c,
                    const std::vector<std::size_t>& q,
                    const std::vector<std::size_t>& in,
                    const std::vector<std::size_t>& out) {
                     auto qubits = std::vector<ir::Qubit>();
                     qubits.reserve(q.size());
                     for (const auto x : q) {
                         qubits.emplace_back(x);
                     }

                     auto input_registers = std::vector<ir::Register>();
                     input_registers.reserve(in.size());
                     for (const auto x : in) {
                         input_registers.emplace_back(x);
                     }

                     auto output_registers = std::vector<ir::Register>();
                     output_registers.reserve(out.size());
                     for (const auto x : out) {
                         output_registers.emplace_back(x);
                     }

                     c.frontend->CallImpl(qubits, input_registers, output_registers);
                 })
            .def("_frontend_loaded_from_ir",
                 [](const PyCircuit& c) { return c.GetFrontend().LoadedFromIR(); })
            .def("has_frontend", [](const PyCircuit& c) { return c.HasFrontend(); })
            .def(
                    "get_frontend",
                    [](const PyCircuit& c) -> auto& { return c.GetFrontend(); },
                    nb::rv_policy::reference,
                    // Nurse = return value, Patient = c
                    // "c" should be kept alive at least until "return value" is
                    // freed by the garbage collector.
                    nb::keep_alive<0, 1>()
            )
            .def("has_ir", [](const PyCircuit& c) { return c.HasIR(); })
            .def(
                    "get_ir",
                    [](const PyCircuit& c) -> auto& { return c.GetIR(); },
                    nb::rv_policy::reference,
                    // Nurse = return value, Patient = c
                    // "c" should be kept alive at least until "return value" is
                    // freed by the garbage collector.
                    nb::keep_alive<0, 1>()
            )
            .def("has_mf", [](const PyCircuit& c) { return c.HasMF(); })
            .def(
                    "get_mf",
                    [](const PyCircuit& c) -> auto& { return c.GetMF(); },
                    nb::rv_policy::reference_internal,
                    // Nurse = return value, Patient = c
                    // "c" should be kept alive at least until "return value" is
                    // freed by the garbage collector.
                    nb::keep_alive<0, 1>()
            );
}
}  // namespace pyqret
