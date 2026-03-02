/**
 * @file qret/analysis/visualizer.cpp
 * @brief Visualize a func in various ways.
 */

#include "qret/analysis/visualizer.h"

#include <fmt/format.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <ranges>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "qret/analysis/circuit_drawer.h"
#include "qret/analysis/compute_graph.h"
#include "qret/analysis/dot.h"
#include "qret/base/cast.h"
#include "qret/ir/basic_block.h"
#include "qret/ir/function.h"
#include "qret/ir/instruction.h"
#include "qret/ir/instructions.h"
#include "qret/ir/value.h"

namespace qret {
namespace {
std::string EscapeRecord(std::string_view s) {
    std::string out;
    out.reserve(s.size() * 2);
    for (char c : s) {
        switch (c) {
            case '{':
            case '}':
            case '|':
            case '<':
            case '>':
            case '\"':
                out.push_back('\\');
                [[fallthrough]];
            default:
                out.push_back(c);
        }
    }
    return out;
}

void PrintSwitch(const ir::SwitchInst& inst, std::ostream& out) {
    out << "&nbsp;&nbsp;";
    inst.GetOpcode().Print(out);

    // Print value
    out << " [";
    auto sep = std::string("");
    for (const auto r : inst.GetValue()) {
        out << std::exchange(sep, " ");
        r.PrintAsOperand(out);
    }
    out << ']';

    out << "\\l&nbsp;&nbsp;&nbsp;&nbsp;";

    // Print default
    out << "%";
    out << inst.GetDefaultBB()->GetName();

    // Print cases
    out << "\\l&nbsp;&nbsp;&nbsp;&nbsp;";
    out << "[";
    for (const auto& [key, bb] : inst.GetCaseBB()) {
        out << "\\l&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";
        out << key << ": %" << bb->GetName();
    }
    out << "\\l&nbsp;&nbsp;&nbsp;&nbsp;";
    out << ']';
}
}  // namespace

std::string GenCFG(const ir::Function& func) {
    auto dot = DotGraph(func.GetName());

    auto bb_idx = std::unordered_map<const ir::BasicBlock*, std::size_t>{};
    bb_idx.reserve(func.GetNumBBs());

    // Add nodes.
    for (const auto& bb : func) {
        const auto idx = bb_idx.size();
        bb_idx.insert({&bb, idx});

        std::stringstream instructions_ss;
        for (const auto& inst : bb) {
            if (inst.GetOpcode() == ir::Opcode::Table::Switch) {
                PrintSwitch(*Cast<ir::SwitchInst>(&inst), instructions_ss);
                instructions_ss << "\\l";
            } else {
                instructions_ss << "&nbsp;&nbsp;";
                inst.Print(instructions_ss);
                instructions_ss << "\\l";
            }
        }

        const auto node_name = fmt::format("n{}", idx);
        const auto instruction_str = EscapeRecord(instructions_ss.str());

        // Treat branching terminator as special.
        const auto* terminator = bb.GetTerminator();
        if (terminator != nullptr && terminator->GetOpcode() == ir::Opcode::Table::Branch) {
            const auto* inst = Cast<ir::BranchInst>(terminator);
            if (inst->IsConditional()) {
                dot.AddNode(
                        node_name,
                        {.label = fmt::format(
                                 "{{<main> %{}:\\l{}|{{<true> T|<false> F}}}}",
                                 bb.GetName(),
                                 instruction_str
                         ),
                         .shape = "record"}
                );
            }
        } else if (terminator != nullptr && terminator->GetOpcode() == ir::Opcode::Table::Switch) {
            const auto* inst = Cast<ir::SwitchInst>(terminator);
            auto case_str = std::string();
            for (const auto& [key, case_bb] : inst->GetCaseBB()) {
                case_str += fmt::format("|<case{}> Case{}", key, key);
            }
            dot.AddNode(
                    node_name,
                    {.label = fmt::format(
                             "{{<main> %{}:\\l{}|{{<default> D{}}}}}",
                             bb.GetName(),
                             instruction_str,
                             case_str
                     ),
                     .shape = "record"}
            );
        }

        // Add node if not branching.
        if (!dot.HasNode(node_name)) {
            dot.AddNode(
                    node_name,
                    {.label = fmt::format("{{<main> %{}:\\l{}}}", bb.GetName(), instruction_str),
                     .shape = "record"}
            );
        }
    }

    // Add edges.
    for (const auto& bb : func) {
        const auto* terminator = bb.GetTerminator();
        if (terminator == nullptr) {
            continue;
        }

        if (terminator->GetOpcode() == ir::Opcode::Table::Branch) {
            const auto* inst = Cast<ir::BranchInst>(terminator);
            if (inst->IsConditional()) {
                dot.AddDirectedEdge(
                        fmt::format("n{}:true:s", bb_idx.at(&bb)),
                        fmt::format("n{}:main:n", bb_idx.at(inst->GetTrueBB()))
                );
                dot.AddDirectedEdge(
                        fmt::format("n{}:false:s", bb_idx.at(&bb)),
                        fmt::format("n{}:main:n", bb_idx.at(inst->GetFalseBB()))
                );
            } else {
                dot.AddDirectedEdge(
                        fmt::format("n{}:main:s", bb_idx.at(&bb)),
                        fmt::format("n{}:main:n", bb_idx.at(inst->GetTrueBB()))
                );
            }
        } else if (terminator->GetOpcode() == ir::Opcode::Table::Switch) {
            const auto* inst = Cast<ir::SwitchInst>(terminator);
            dot.AddDirectedEdge(
                    fmt::format("n{}:default:s", bb_idx.at(&bb)),
                    fmt::format("n{}:main:n", bb_idx.at(inst->GetDefaultBB()))
            );
            for (const auto& [key, case_bb] : inst->GetCaseBB()) {
                dot.AddDirectedEdge(
                        fmt::format("n{}:case{}:s", bb_idx.at(&bb), key),
                        fmt::format("n{}:main:n", bb_idx.at(case_bb))
                );
            }
        }
    }

    return (std::stringstream{} << dot).str();
}

std::string GenCallGraph(const ir::Function& func, bool display_num_calls) {
    auto call =
            std::map<const ir::Function*, std::map<const ir::Function*, std::size_t>>{{&func, {}}};
    auto circuit_idx = std::map<const ir::Function*, std::size_t>{{&func, 0}};

    auto dfs_stack = std::stack<const ir::Function*>{};
    dfs_stack.push(&func);
    while (!dfs_stack.empty()) {
        const auto* src = dfs_stack.top();
        dfs_stack.pop();
        for (const auto& bb : *src) {
            for (const auto& inst : bb) {
                if (inst.GetOpcode().GetCode() == ir::Opcode::Table::Call) {
                    const auto* cinst = Cast<ir::CallInst>(&inst);
                    const auto* dst = cinst->GetCallee();
                    if (!call.contains(dst)) {
                        call.insert({dst, {}});
                        circuit_idx.insert({dst, circuit_idx.size()});
                        dfs_stack.push(dst);
                    }
                    call.at(src)[dst]++;
                }
            }
        }
    }

    auto dot = DotGraph(func.GetName());
    for (const auto& [src, dst_map] : call) {
        dot.AddNode(
                fmt::format("n{}", circuit_idx.at(src)),
                {.label = std::string(src->GetName()), .shape = "record"}
        );
        for (const auto& [dst, num_calls] : dst_map) {
            if (display_num_calls) {
                dot.AddDirectedEdge(
                        fmt::format("n{}", circuit_idx.at(src)),
                        fmt::format("n{}", circuit_idx.at(dst)),
                        std::to_string(num_calls)
                );
            } else {
                dot.AddDirectedEdge(
                        fmt::format("n{}", circuit_idx.at(src)),
                        fmt::format("n{}", circuit_idx.at(dst))
                );
            }
        }
    }
    return (std::stringstream{} << dot).str();
}
std::string GenLaTeX(const ir::Function& func) {
    auto drawer = CircuitDrawer(func.GetNumQubits(), func.GetNumRegisters());

    for (const auto& bb : func) {
        drawer.Barrier(bb.GetName());
        for (const auto& inst : bb) {
            const auto opcode = inst.GetOpcode().GetCode();
            if (opcode == ir::Opcode::Table::Measurement) {
                const auto* i = Cast<ir::MeasurementInst>(&inst);
                drawer.Measure(i->GetQubit().id, i->GetRegister().id);
            }
            if (opcode == ir::Opcode::Table::I) {
                const auto* i = Cast<ir::UnaryInst>(&inst);
                drawer.I(i->GetQubit().id);
            }
            if (opcode == ir::Opcode::Table::X) {
                const auto* i = Cast<ir::UnaryInst>(&inst);
                drawer.X(i->GetQubit().id);
            }
            if (opcode == ir::Opcode::Table::Y) {
                const auto* i = Cast<ir::UnaryInst>(&inst);
                drawer.Y(i->GetQubit().id);
            }
            if (opcode == ir::Opcode::Table::Z) {
                const auto* i = Cast<ir::UnaryInst>(&inst);
                drawer.Z(i->GetQubit().id);
            }
            if (opcode == ir::Opcode::Table::H) {
                const auto* i = Cast<ir::UnaryInst>(&inst);
                drawer.H(i->GetQubit().id);
            }
            if (opcode == ir::Opcode::Table::S) {
                const auto* i = Cast<ir::UnaryInst>(&inst);
                drawer.S(i->GetQubit().id);
            }
            if (opcode == ir::Opcode::Table::SDag) {
                const auto* i = Cast<ir::UnaryInst>(&inst);
                drawer.SDag(i->GetQubit().id);
            }
            if (opcode == ir::Opcode::Table::T) {
                const auto* i = Cast<ir::UnaryInst>(&inst);
                drawer.T(i->GetQubit().id);
            }
            if (opcode == ir::Opcode::Table::TDag) {
                const auto* i = Cast<ir::UnaryInst>(&inst);
                drawer.TDag(i->GetQubit().id);
            }
            if (opcode == ir::Opcode::Table::RX) {
                const auto* i = Cast<ir::ParametrizedRotationInst>(&inst);
                drawer.RX(i->GetQubit().id, i->GetTheta().value);
            }
            if (opcode == ir::Opcode::Table::RY) {
                const auto* i = Cast<ir::ParametrizedRotationInst>(&inst);
                drawer.RY(i->GetQubit().id, i->GetTheta().value);
            }
            if (opcode == ir::Opcode::Table::RZ) {
                const auto* i = Cast<ir::ParametrizedRotationInst>(&inst);
                drawer.RZ(i->GetQubit().id, i->GetTheta().value);
            }
            if (opcode == ir::Opcode::Table::CX) {
                const auto* i = Cast<ir::BinaryInst>(&inst);
                drawer.CX(i->GetQubit0().id, i->GetQubit1().id);
            }
            if (opcode == ir::Opcode::Table::CY) {
                const auto* i = Cast<ir::BinaryInst>(&inst);
                drawer.CY(i->GetQubit0().id, i->GetQubit1().id);
            }
            if (opcode == ir::Opcode::Table::CZ) {
                const auto* i = Cast<ir::BinaryInst>(&inst);
                drawer.CZ(i->GetQubit0().id, i->GetQubit1().id);
            }
            if (opcode == ir::Opcode::Table::CCX) {
                const auto* i = Cast<ir::TernaryInst>(&inst);
                drawer.CCX(i->GetQubit0().id, i->GetQubit1().id, i->GetQubit2().id);
            }
            if (opcode == ir::Opcode::Table::CCY) {
                const auto* i = Cast<ir::TernaryInst>(&inst);
                drawer.CCY(i->GetQubit0().id, i->GetQubit1().id, i->GetQubit2().id);
            }
            if (opcode == ir::Opcode::Table::CCZ) {
                const auto* i = Cast<ir::TernaryInst>(&inst);
                drawer.CCZ(i->GetQubit0().id, i->GetQubit1().id, i->GetQubit2().id);
            }
            if (opcode == ir::Opcode::Table::MCX) {
                const auto* i = Cast<ir::MultiControlInst>(&inst);
                const auto& control = i->GetControl();
                auto control_id = std::vector<std::size_t>(control.size());
                std::ranges::transform(control, control_id.begin(), [&](ir::Qubit q) {
                    return q.id;
                });
                drawer.MCX(i->GetTarget().id, control_id);
            }
            if (opcode == ir::Opcode::Table::GlobalPhase) {
                const auto* i = Cast<ir::GlobalPhaseInst>(&inst);
                drawer.RotateGlobalPhase(i->GetTheta().value);
            }
            if (opcode == ir::Opcode::Table::Call) {
                const auto* i = Cast<ir::CallInst>(&inst);
                const auto& operate = i->GetOperate();
                auto operate_id = std::vector<std::size_t>(operate.size());
                std::ranges::transform(operate, operate_id.begin(), [&](ir::Qubit q) {
                    return q.id;
                });
                const auto& input = i->GetInput();
                auto input_id = std::vector<std::size_t>(input.size());
                std::ranges::transform(input, input_id.begin(), [&](ir::Register r) {
                    return r.id;
                });
                const auto& output = i->GetOutput();
                auto output_id = std::vector<std::size_t>(output.size());
                std::ranges::transform(output, output_id.begin(), [&](ir::Register r) {
                    return r.id;
                });
                drawer.Call(i->GetCallee()->GetName(), operate_id, input_id, output_id);
            }
            if (opcode == ir::Opcode::Table::CallCF) {
                const auto* i = Cast<ir::CallCFInst>(&inst);
                const auto& input = i->GetInput();
                auto input_id = std::vector<std::size_t>(input.size());
                std::ranges::transform(input, input_id.begin(), [&](ir::Register r) {
                    return r.id;
                });
                const auto& output = i->GetOutput();
                auto output_id = std::vector<std::size_t>(output.size());
                std::ranges::transform(output, output_id.begin(), [&](ir::Register r) {
                    return r.id;
                });
                drawer.ClassicalFunction(i->GetFunction().Name(), input_id, output_id);
            }
            if (opcode == ir::Opcode::Table::DiscreteDistribution) {
                const auto* i = Cast<ir::DiscreteDistInst>(&inst);
                const auto& registers = i->GetRegisters();
                auto registers_id = std::vector<std::size_t>(registers.size());
                std::ranges::transform(registers, registers_id.begin(), [&](ir::Register r) {
                    return r.id;
                });
                drawer.CRand(i->GetWeights(), registers_id);
            }
            if (opcode == ir::Opcode::Table::Branch) {
                const auto* i = Cast<ir::BranchInst>(&inst);
                if (i->IsConditional()) {
                    throw std::runtime_error(
                            "Error ir::Function::GenLaTeX: Circuit with conditional BranchInst is "
                            "not "
                            "supported."
                    );
                }
            }
            if (opcode == ir::Opcode::Table::Switch) {
                throw std::runtime_error(
                        "Error ir::Function::GenLaTeX: Circuit with SwitchInst is not supported."
                );
            }
        }
    }

    auto lsticks = std::vector<std::string>{};
    lsticks.reserve(func.GetNumQubits() + func.GetNumRegisters() + func.GetNumTmpRegisters());
    for (const auto& [name, size] : func.GetArgument().qubits) {
        for (const auto i : std::views::iota(std::size_t{0}, size)) {
            lsticks.emplace_back(fmt::format("{}[{}]", name, i));
        }
    }
    for (const auto& [name, size] : func.GetArgument().registers) {
        for (const auto i : std::views::iota(std::size_t{0}, size)) {
            lsticks.emplace_back(fmt::format("{}[{}]", name, i));
        }
    }
    if (const auto num_tmps = func.GetNumTmpRegisters(); num_tmps > 0) {
        lsticks.emplace_back(fmt::format("{}[{}]", "tmp", num_tmps));
    }

    auto ss = std::stringstream{};
    drawer.Print(ss, lsticks);
    return ss.str();
}

std::string GenComputeGraph(const ir::Function& func) {
    using NodeId = ComputeGraph::NodeId;
    using Position = ComputeGraph::Position;
    using ArgKind = ComputeGraph::ArgKind;
    using Region = ComputeGraph::Region;

    auto graph = ComputeGraph{};
    auto qubit_positions = std::vector<Position>{};
    qubit_positions.reserve(func.GetNumQubits());
    for (const auto& [label, size] : func.GetArgument().qubits) {
        NodeId node_id = graph.AddInit(label, ArgKind::Qubit, size);
        for (const auto arg_id : std::views::iota(std::size_t{0}, size)) {
            qubit_positions.emplace_back(node_id, 0, arg_id);
        }
    }

    const auto update_qubit = [&graph, &qubit_positions](std::uint64_t id, Position next_position) {
        graph.AddEdge(qubit_positions[id], next_position);
        qubit_positions[id] = next_position;
    };

    for (const auto& bb : func) {
        for (const auto& inst : bb) {
            if (const auto* i = DynCast<ir::MeasurementInst>(&inst)) {
                const auto id = graph.AddInstruction("measure", {{"target", ArgKind::Qubit, 1}});
                update_qubit(i->GetQubit().id, {id, 0, 0});
            } else if (const auto* i = DynCast<ir::UnaryInst>(&inst)) {
                const auto label = [&]() -> std::string_view {
                    const auto opcode = i->GetOpcode().GetCode();
                    if (opcode == ir::Opcode::Table::I) {
                        return "I";
                    } else if (opcode == ir::Opcode::Table::X) {
                        return "X";
                    } else if (opcode == ir::Opcode::Table::Y) {
                        return "Y";
                    } else if (opcode == ir::Opcode::Table::Z) {
                        return "Z";
                    } else if (opcode == ir::Opcode::Table::H) {
                        return "H";
                    } else if (opcode == ir::Opcode::Table::S) {
                        return "S";
                    } else if (opcode == ir::Opcode::Table::SDag) {
                        return "SDag";
                    } else if (opcode == ir::Opcode::Table::T) {
                        return "T";
                    } else if (opcode == ir::Opcode::Table::TDag) {
                        return "TDag";
                    } else if (opcode == ir::Opcode::Table::RX) {
                        return "RX";
                    } else if (opcode == ir::Opcode::Table::RY) {
                        return "RY";
                    } else if (opcode == ir::Opcode::Table::RZ) {
                        return "RZ";
                    }
                    assert(false && "unreachable");
                    return "";
                }();
                const auto id = graph.AddInstruction(label, {{"target", ArgKind::Qubit, 1}});
                update_qubit(i->GetQubit().id, {id, 0, 0});
            } else if (const auto* i = DynCast<ir::BinaryInst>(&inst)) {
                const auto label = [&]() -> std::string_view {
                    const auto opcode = i->GetOpcode().GetCode();
                    if (opcode == ir::Opcode::Table::CX) {
                        return "CX";
                    } else if (opcode == ir::Opcode::Table::CY) {
                        return "CY";
                    } else if (opcode == ir::Opcode::Table::CZ) {
                        return "CZ";
                    }
                    assert(false && "unreachable");
                    return "";
                }();
                const auto id = graph.AddInstruction(
                        label,
                        {{"target", ArgKind::Qubit, 1}, {"control", ArgKind::Qubit, 1}}
                );
                update_qubit(i->GetQubit0().id, {id, 0, 0});
                update_qubit(i->GetQubit1().id, {id, 1, 0});
            } else if (const auto* i = DynCast<ir::TernaryInst>(&inst)) {
                const auto label = [&]() -> std::string_view {
                    const auto opcode = i->GetOpcode().GetCode();
                    if (opcode == ir::Opcode::Table::CCX) {
                        return "CCX";
                    } else if (opcode == ir::Opcode::Table::CCY) {
                        return "CCY";
                    } else if (opcode == ir::Opcode::Table::CCZ) {
                        return "CCZ";
                    }
                    assert(false && "unreachable");
                    return "";
                }();
                const auto id = graph.AddInstruction(
                        label,
                        {{"target", ArgKind::Qubit, 1}, {"control", ArgKind::Qubit, 2}}
                );
                update_qubit(i->GetQubit0().id, {id, 0, 0});
                update_qubit(i->GetQubit1().id, {id, 1, 0});
                update_qubit(i->GetQubit2().id, {id, 1, 1});
            } else if (const auto* i = DynCast<ir::MultiControlInst>(&inst)) {
                const auto& controls = i->GetControl();
                const auto id = graph.AddInstruction(
                        "MCX",
                        {{"target", ArgKind::Qubit, 1},
                         {"control", ArgKind::Qubit, controls.size()}}
                );

                update_qubit(i->GetTarget().id, {id, 0, 0});
                for (const auto arg_id : std::views::iota(std::size_t{0}, controls.size())) {
                    update_qubit(controls[arg_id].id, {id, 1, arg_id});
                }
            } else if (const auto* i = DynCast<ir::CallInst>(&inst)) {
                const auto& operate = i->GetOperate();
                const auto* callee = i->GetCallee();
                const auto& argument = callee->GetArgument();
                auto regions = std::vector<Region>{};
                for (const auto& [label, size] : argument.qubits) {
                    regions.emplace_back(label, ArgKind::Qubit, size);
                }
                const auto id = graph.AddInstruction(callee->GetName(), regions);
                auto qubit_id = std::size_t{0};
                for (const auto region_id :
                     std::views::iota(std::size_t{0}, argument.qubits.size())) {
                    const auto size = argument.qubits[region_id].second;
                    for (const auto arg_id : std::views::iota(std::size_t{0}, size)) {
                        update_qubit(operate[qubit_id++].id, {id, region_id, arg_id});
                    }
                }
            } else if (const auto* i = DynCast<ir::CallCFInst>(&inst)) {
                (void)i;
            } else if (const auto* i = DynCast<ir::DiscreteDistInst>(&inst)) {
                (void)i;
            } else if (const auto* i = DynCast<ir::BranchInst>(&inst)) {
                if (i->IsConditional()) {
                    throw std::runtime_error(
                            "Error GenComputeGraph: Circuit with conditional BranchInst is not "
                            "supported."
                    );
                }
            } else if (const auto* i = DynCast<ir::SwitchInst>(&inst)) {
                (void)i;
                throw std::runtime_error(
                        "Error GenComputeGraph: Circuit with conditional SwitchInst is not "
                        "supported."
                );
            }
        }
    }

    const auto dot = graph.BuildAsDot();
    return (std::ostringstream{} << dot).str();
}
}  // namespace qret
