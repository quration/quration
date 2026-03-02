#include "frontend/ir.h"

#include <nanobind/make_iterator.h>
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/unique_ptr.h>
#include <nanobind/stl/vector.h>

#include <fmt/format.h>

#include <fstream>
#include <sstream>
#include <stdexcept>

#include "qret/analysis/visualizer.h"
#include "qret/ir/basic_block.h"
#include "qret/ir/context.h"
#include "qret/ir/function.h"
#include "qret/ir/instruction.h"
#include "qret/ir/instructions.h"
#include "qret/ir/json.h"
#include "qret/ir/module.h"

#include "frontend/pycircuit.h"

namespace pyqret {
namespace ir = qret::ir;
namespace nb = nanobind;

void BindIR(nanobind::module_& m) {
    nb::class_<ir::IRContext>(m, "Context").def(nb::init<>());

    nb::class_<ir::Module>(m, "Module")
            .def_static(
                    "Create",
                    [](std::string_view name, ir::IRContext& context) {
                        return ir::Module::Create(name, context);
                    },
                    nb::rv_policy::reference,
                    // Nurse = return value, Patient = context
                    // "context" should be kept alive at least until "return value" is freed by the
                    // garbage collector.
                    nb::keep_alive<0, 2>()
            )
            .def("get_name", &ir::Module::GetName)
            .def("contains",
                 [](const ir::Module& module, const std::string& name) {
                     const auto* ptr = module.GetFunction(name);
                     return ptr != nullptr;
                 })
            .def("get_function",
                 [](const ir::Module& module, const std::string& name) {
                     auto* func = module.GetFunction(name);
                     if (func == nullptr) {
                         throw std::runtime_error(
                                 fmt::format(
                                         "Module does not contain the function of name {}",
                                         name
                                 )
                         );
                     }
                     auto ret = PyCircuit{PyArgument::FromIR(*func), nullptr, func, std::nullopt};
                     return ret;
                 })
            .def("get_circuit",
                 [](const ir::Module& module, const std::string& name) {
                     auto* func = module.GetFunction(name);
                     if (func == nullptr) {
                         throw std::runtime_error(
                                 fmt::format(
                                         "Module does not contain the function of name {}",
                                         name
                                 )
                         );
                     }
                     auto ret = PyCircuit{PyArgument::FromIR(*func), nullptr, func, std::nullopt};
                     return ret;
                 })
            .def("dump",
                 [](const ir::Module& module, const std::string& file) {
                     auto ofs = std::ofstream{file};
                     if (!ofs.good()) {
                         throw std::runtime_error(fmt::format("Failed to load: {}", file));
                     }
                     ofs << qret::Json(module) << std::endl;
                 })
            .def("dumps", [](const ir::Module& module) { return qret::Json(module).dump(); })
            .def("_load",
                 [](ir::Module& module, const std::string& file) {
                     auto ifs = std::ifstream{file};
                     if (!ifs.good()) {
                         throw std::runtime_error(fmt::format("Failed to load: {}", file));
                     }
                     auto j = qret::Json::parse(ifs);
                     qret::ir::LoadJsonImpl(j, module);
                 })
            .def("_loads",
                 [](ir::Module& module, const std::string& json) {
                     auto j = qret::Json::parse(json);
                     qret::ir::LoadJsonImpl(j, module);
                 })
            .def("get_function_list",
                 [](const ir::Module& module) {
                     auto ret = std::vector<std::string_view>{};
                     for (const auto& func : module) {
                         ret.push_back(func.GetName());
                     }
                     return ret;
                 })
            .def("get_circuit_list", [](const ir::Module& module) {
                auto ret = std::vector<std::string_view>{};
                for (const auto& func : module) {
                    ret.push_back(func.GetName());
                }
                return ret;
            });

    // Instruction
    using Table = ir::Opcode::Table;
    nb::enum_<Table>(m, "Opcode", "Opcode of an ir instruction.")
            .value("Measurement", Table::Measurement)
            .value("I", Table::I)
            .value("X", Table::X)
            .value("Y", Table::Y)
            .value("Z", Table::Z)
            .value("H", Table::H)
            .value("S", Table::S)
            .value("SDag", Table::SDag)
            .value("T", Table::T)
            .value("TDag", Table::TDag)
            .value("RX", Table::RX)
            .value("RY", Table::RY)
            .value("RZ", Table::RZ)
            .value("CX", Table::CX)
            .value("CY", Table::CY)
            .value("CZ", Table::CZ)
            .value("CCX", Table::CCX)
            .value("CCY", Table::CCY)
            .value("CCZ", Table::CCZ)
            .value("MCX", Table::MCX)
            .value("GlobalPhase", Table::GlobalPhase)
            .value("Call", Table::Call)
            .value("CallCF", Table::CallCF)
            .value("DiscreteDistribution", Table::DiscreteDistribution)
            .value("Branch", Table::Branch)
            .value("Switch", Table::Switch)
            .value("Return", Table::Return)
            .value("Clean", Table::Clean)
            .value("CleanProb", Table::CleanProb)
            .value("DirtyBegin", Table::DirtyBegin)
            .value("DirtyEnd", Table::DirtyEnd);

    nb::class_<ir::Instruction>(m, "Instruction", "Base class of instructions in IR")
            .def("get_opcode",
                 [](const ir::Instruction& inst) { return inst.GetOpcode().GetCode(); })
            .def("is_measurement", &ir::Instruction::IsMeasurement)
            .def("is_unary", &ir::Instruction::IsUnary)
            .def("is_parametrized_rotation", &ir::Instruction::IsParametrizedRotation)
            .def("is_binary", &ir::Instruction::IsBinary)
            .def("is_ternary", &ir::Instruction::IsTernary)
            .def("is_multi_control", &ir::Instruction::IsMultiControl)
            .def("is_state_invariant", &ir::Instruction::IsStateInvariant)
            .def("is_call", &ir::Instruction::IsCall)
            .def("is_classical", &ir::Instruction::IsClassical)
            .def("is_random", &ir::Instruction::IsRandom)
            .def("is_terminator", &ir::Instruction::IsTerminator)
            .def("is_opt_hint", &ir::Instruction::IsOptHint)
            .def("is_gate", &ir::Instruction::IsGate)
            .def("is_clifford", &ir::Instruction::IsClifford)
            .def("get_metadata",
                 [](const ir::Instruction& inst) {
                     auto ss = std::stringstream();
                     inst.PrintMetadata(ss);
                     return ss.str();
                 })
            .def("__str__", [](const ir::Instruction& inst) {
                auto ss = std::stringstream();
                inst.Print(ss);
                return ss.str();
            });

    // Instructions
    nb::class_<ir::MeasurementInst, ir::Instruction>(m, "MeasurementInst")
            .def("__str__", [](const ir::MeasurementInst& inst) {
                auto ss = std::stringstream();
                inst.Print(ss);
                return ss.str();
            });
    nb::class_<ir::UnaryInst, ir::Instruction>(m, "UnaryInst")
            .def("__str__", [](const ir::UnaryInst& inst) {
                auto ss = std::stringstream();
                inst.Print(ss);
                return ss.str();
            });
    nb::class_<ir::ParametrizedRotationInst, ir::UnaryInst>(m, "ParametrizedRotationInst")
            .def("__str__", [](const ir::ParametrizedRotationInst& inst) {
                auto ss = std::stringstream();
                inst.Print(ss);
                return ss.str();
            });
    nb::class_<ir::BinaryInst, ir::Instruction>(m, "BinaryInst")
            .def("__str__", [](const ir::BinaryInst& inst) {
                auto ss = std::stringstream();
                inst.Print(ss);
                return ss.str();
            });
    nb::class_<ir::TernaryInst, ir::Instruction>(m, "TernaryInst")
            .def("__str__", [](const ir::TernaryInst& inst) {
                auto ss = std::stringstream();
                inst.Print(ss);
                return ss.str();
            });
    nb::class_<ir::MultiControlInst, ir::Instruction>(m, "MultiControlInst")
            .def("__str__", [](const ir::MultiControlInst& inst) {
                auto ss = std::stringstream();
                inst.Print(ss);
                return ss.str();
            });
    nb::class_<ir::GlobalPhaseInst, ir::Instruction>(m, "GlobalPhaseInst")
            .def("__str__", [](const ir::GlobalPhaseInst& inst) {
                auto ss = std::stringstream();
                inst.Print(ss);
                return ss.str();
            });
    nb::class_<ir::CallInst, ir::Instruction>(m, "CallInst")
            .def("__str__", [](const ir::CallInst& inst) {
                auto ss = std::stringstream();
                inst.Print(ss);
                return ss.str();
            });
    nb::class_<ir::CallCFInst, ir::Instruction>(m, "CallCFInst")
            .def("__str__", [](const ir::CallCFInst& inst) {
                auto ss = std::stringstream();
                inst.Print(ss);
                return ss.str();
            });
    nb::class_<ir::DiscreteDistInst, ir::Instruction>(m, "DiscreteDistInst")
            .def("__str__", [](const ir::DiscreteDistInst& inst) {
                auto ss = std::stringstream();
                inst.Print(ss);
                return ss.str();
            });
    nb::class_<ir::BranchInst, ir::Instruction>(m, "BranchInst")
            .def("__str__", [](const ir::BranchInst& inst) {
                auto ss = std::stringstream();
                inst.Print(ss);
                return ss.str();
            });
    nb::class_<ir::SwitchInst, ir::Instruction>(m, "SwitchInst")
            .def("__str__", [](const ir::SwitchInst& inst) {
                auto ss = std::stringstream();
                inst.Print(ss);
                return ss.str();
            });
    nb::class_<ir::ReturnInst, ir::Instruction>(m, "ReturnInst")
            .def("__str__", [](const ir::ReturnInst& inst) {
                auto ss = std::stringstream();
                inst.Print(ss);
                return ss.str();
            });
    nb::class_<ir::CleanInst, ir::Instruction>(m, "CleanInst")
            .def("__str__", [](const ir::CleanInst& inst) {
                auto ss = std::stringstream();
                inst.Print(ss);
                return ss.str();
            });
    nb::class_<ir::CleanProbInst, ir::Instruction>(m, "CleanProbInst")
            .def("__str__", [](const ir::CleanProbInst& inst) {
                auto ss = std::stringstream();
                inst.Print(ss);
                return ss.str();
            });
    nb::class_<ir::DirtyInst, ir::Instruction>(m, "DirtyInst")
            .def("__str__", [](const ir::DirtyInst& inst) {
                auto ss = std::stringstream();
                inst.Print(ss);
                return ss.str();
            });

    // BB
    nb::class_<ir::BasicBlock>(m, "BasicBlock")
            .def("name", &ir::BasicBlock::GetName)
            .def("num_instructions", &ir::BasicBlock::Size)
            .def("num_predecessors", &ir::BasicBlock::NumPredecessors)
            .def("num_successors", &ir::BasicBlock::NumSuccessors)
            .def("is_entry_block", &ir::BasicBlock::IsEntryBlock)
            .def("is_unitary", &ir::BasicBlock::IsUnitary)
            .def("does_measurement", &ir::BasicBlock::DoesMeasurement)
            .def(
                    "__iter__",
                    [](const ir::BasicBlock& bb) {
                        return nb::make_iterator<nb::rv_policy::reference_internal>(
                                nb::type<ir::Instruction>(),
                                "BBIterator",
                                bb.begin(),
                                bb.end()
                        );
                    },
                    // Nurse = return value, Patient = bb
                    // "bb" should be kept alive at least until "return value" is
                    // freed by the garbage collector.
                    nb::keep_alive<0, 1>()
            );

    // Function
    nb::class_<ir::Function>(m, "IRFunction")
            .def("name", &ir::Function::GetName)
            .def(
                    "get_entry_bb",
                    [](const ir::Function& func) -> auto& { return *func.GetEntryBB(); },
                    nb::rv_policy::reference,
                    nb::keep_alive<0, 1>()
            )
            .def("num_instructions", &ir::Function::GetInstructionCount)
            .def("num_qubits", &ir::Function::GetNumQubits)
            .def("num_registers", &ir::Function::GetNumRegisters)
            .def("num_tmp_registers", &ir::Function::GetNumTmpRegisters)
            .def("is_unitary", &ir::Function::IsUnitary)
            .def("does_measurement", &ir::Function::DoesMeasurement)
            .def("gen_cfg", [](const ir::Function& c) { return qret::GenCFG(c); })
            .def("gen_call_graph",
                 [](const ir::Function& c, bool display_num_calls) {
                     return qret::GenCallGraph(c, display_num_calls);
                 })
            .def("gen_latex", [](const ir::Function& c) { return qret::GenLaTeX(c); })
            .def("gen_compute_graph",
                 [](const ir::Function& c) { return qret::GenComputeGraph(c); })
            .def(
                    "__iter__",
                    [](const ir::Function& c) {
                        return nb::make_iterator<nb::rv_policy::reference_internal>(
                                nb::type<ir::Instruction>(),
                                "IRFunctionIterator",
                                c.begin(),
                                c.end()
                        );
                    },
                    // Nurse = return value, Patient = c
                    // "c" should be kept alive at least until "return value"
                    // is freed by the garbage collector.
                    nb::keep_alive<0, 1>()
            );
}
}  // namespace pyqret
