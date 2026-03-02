/**
 * @file qret/runtime/simulator.cpp
 * @brief Simulator.
 */

#include "qret/runtime/simulator.h"

#include <fmt/core.h>

#include <cassert>
#include <iterator>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string_view>

#include "qret/base/cast.h"
#include "qret/base/log.h"
#include "qret/base/type.h"
#include "qret/ir/basic_block.h"
#include "qret/ir/instruction.h"
#include "qret/ir/instructions.h"
#include "qret/ir/metadata.h"
#include "qret/runtime/clifford_state.h"
#include "qret/runtime/full_quantum_state.h"
#include "qret/runtime/quantum_state.h"
#include "qret/runtime/toffoli_state.h"

namespace qret::runtime {
using ir::Opcode, ir::Function, ir::Instruction, ir::MeasurementInst, ir::UnaryInst,
        ir::ParametrizedRotationInst, ir::BinaryInst, ir::TernaryInst, ir::MultiControlInst,
        ir::GlobalPhaseInst, ir::CleanInst;
namespace {
void RunMeasurementInst(
        QuantumState* state,
        const MeasurementInst* inst,
        const std::vector<std::uint64_t>& qmap,
        const std::vector<std::uint64_t>& rmap
) {
    const auto q = qmap[inst->GetQubit().id];
    const auto r = rmap[inst->GetRegister().id];
    state->Measure(q, r);
}
void RunUnaryInst(
        QuantumState* state,
        const UnaryInst* inst,
        const std::vector<std::uint64_t>& qmap
) {
    const auto q = qmap[inst->GetQubit().id];
    switch (inst->GetOpcode().GetCode()) {
        case Opcode::Table::X:
            state->X(q);
            return;
        case Opcode::Table::Y:
            state->Y(q);
            return;
        case Opcode::Table::Z:
            state->Z(q);
            return;
        case Opcode::Table::H:
            state->H(q);
            return;
        case Opcode::Table::S:
            state->S(q);
            return;
        case Opcode::Table::SDag:
            state->SDag(q);
            return;
        case Opcode::Table::T:
            state->T(q);
            return;
        case Opcode::Table::TDag:
            state->TDag(q);
            return;
        default:
            return;
    }
}
void RunRotationInst(
        QuantumState* state,
        const ParametrizedRotationInst* inst,
        const std::vector<std::uint64_t>& qmap
) {
    const auto q = qmap[inst->GetQubit().id];
    const auto& theta = inst->GetTheta();
    switch (inst->GetOpcode().GetCode()) {
        case Opcode::Table::RX:
            state->RX(q, theta.value);
            return;
        case Opcode::Table::RY:
            state->RY(q, theta.value);
            return;
        case Opcode::Table::RZ:
            state->RZ(q, theta.value);
            return;
        default:
            return;
    }
}
void RunBinaryInst(
        QuantumState* state,
        const BinaryInst* inst,
        const std::vector<std::uint64_t>& qmap
) {
    const auto q0 = qmap[inst->GetQubit0().id];
    const auto q1 = qmap[inst->GetQubit1().id];
    switch (inst->GetOpcode().GetCode()) {
        case Opcode::Table::CX:
            state->CX(q0, q1);
            return;
        case Opcode::Table::CY:
            state->CY(q0, q1);
            return;
        case Opcode::Table::CZ:
            state->CZ(q0, q1);
            return;
        default:
            return;
    }
}
void RunTernaryInst(
        QuantumState* state,
        const TernaryInst* inst,
        const std::vector<std::uint64_t>& qmap
) {
    const auto q0 = qmap[inst->GetQubit0().id];
    const auto q1 = qmap[inst->GetQubit1().id];
    const auto q2 = qmap[inst->GetQubit2().id];
    switch (inst->GetOpcode().GetCode()) {
        case Opcode::Table::CCX:
            state->CCX(q0, q1, q2);
            return;
        case Opcode::Table::CCY:
            state->CCY(q0, q1, q2);
            return;
        case Opcode::Table::CCZ:
            state->CCZ(q0, q1, q2);
            return;
        default:
            return;
    }
}
void RunMultiControlInst(
        QuantumState* state,
        const MultiControlInst* inst,
        const std::vector<std::uint64_t>& qmap
) {
    const auto t = qmap[inst->GetTarget().id];
    auto c = std::vector<std::uint64_t>();
    for (const auto& con : inst->GetControl()) {
        c.emplace_back(qmap[con.id]);
    }
    state->MCX(t, c);
}
std::optional<std::string> RunInst(
        QuantumState* state,
        const Instruction* inst,
        const SimulatorConfig& config,
        const std::vector<std::uint64_t>& qmap,
        const std::vector<std::uint64_t>& rmap
) {
    if (inst->IsMeasurement()) {
        RunMeasurementInst(state, Cast<MeasurementInst>(inst), qmap, rmap);
    } else if (inst->IsUnary()) {
        const auto code = inst->GetOpcode().GetCode();
        if (code == Opcode::Table::RX || code == Opcode::Table::RY || code == Opcode::Table::RZ) {
            RunRotationInst(state, Cast<ParametrizedRotationInst>(inst), qmap);
        } else {
            RunUnaryInst(state, Cast<UnaryInst>(inst), qmap);
        }
    } else if (inst->IsBinary()) {
        RunBinaryInst(state, Cast<BinaryInst>(inst), qmap);
    } else if (inst->IsTernary()) {
        RunTernaryInst(state, Cast<TernaryInst>(inst), qmap);
    } else if (inst->IsMultiControl()) {
        RunMultiControlInst(state, Cast<MultiControlInst>(inst), qmap);
    } else if (inst->IsStateInvariant()) {
        if (inst->GetOpcode() == Opcode::Table::GlobalPhase) {
            state->RotateGlobalPhase(Cast<GlobalPhaseInst>(inst)->GetTheta().value);
        }
    } else if (inst->IsClassical()) {
        const auto* classical_inst = Cast<ir::CallCFInst>(inst);
        const auto& function = classical_inst->GetFunction();
        const auto& input = classical_inst->GetInput();
        const auto& output = classical_inst->GetOutput();
        auto in = std::vector<bool>(input.size());
        std::transform(input.begin(), input.end(), in.begin(), [&rmap](const auto& r) {
            return rmap[r.id];
        });
        auto out = std::vector<bool>(output.size());
        function(in, out);
        for (auto i = std::size_t{0}; i < output.size(); ++i) {
            const auto r = rmap[output[i].id];
            state->WriteRegister(r, out[i]);
        }
    } else if (inst->IsRandom()) {
        const auto* dist_inst = Cast<ir::DiscreteDistInst>(inst);
        const auto& weights = dist_inst->GetWeights();
        const auto& registers = dist_inst->GetRegisters();

        auto dist = std::discrete_distribution<std::size_t>(weights.begin(), weights.end());
        const auto value = dist(state->GetMutEngine());

        for (auto i = std::size_t{0}; i < registers.size(); ++i) {
            const auto r = rmap[registers[i].id];
            state->WriteRegister(r, ((value >> i) & 1) == 1);
        }
    } else if (inst->IsOptHint()) {
        if (inst->GetOpcode().GetCode() == Opcode::Table::Clean) {
            const auto* clean_inst = Cast<CleanInst>(inst);
            const auto q = qmap[clean_inst->GetQubit().id];
            if (!state->IsClean(q)) {
                auto ss = std::stringstream();
                clean_inst->GetQubit().Print(ss);
                ss << " is not clean";
                return ss.str();
            }
        } else {
            // Do nothing
            // std::cerr << "currently unable to run this opt-hint inst: ";
            // inst->Print(std::cerr);
            // std::cerr << std::endl;
        }
    } else {
        auto ss = std::stringstream();
        inst->Print(ss);
        throw std::runtime_error("unable to run this inst: " + ss.str());
    }
    if (config.IsToffoli() && state->GetNumSuperpositions() > config.max_superpositions) {
        return fmt::format(
                "the number of superpositions(={}) exceeds the limits(={})",
                state->GetNumSuperpositions(),
                config.max_superpositions
        );
    }
    return std::nullopt;
}
}  // namespace
Simulator::Simulator(const SimulatorConfig& config, const Function* func)
    : config_{config}
    , func_{func} {
    using StateType = SimulatorConfig::StateType;

    // Debug
    // std::cerr << "Num qubits        = " << func->GetNumQubits() << std::endl;
    // std::cerr << "Num registers     = " << func->GetNumRegisters() << std::endl;
    // std::cerr << "Num tmp registers = " << func->GetNumTmpRegisters() << std::endl;

    switch (config.state_type) {
        case StateType::Toffoli: {
            state_ = std::unique_ptr<ToffoliState>(
                    new ToffoliState(config.seed, func->GetNumQubits())
            );
            break;
        }
        case StateType::FullQuantum: {
            state_ = std::unique_ptr<FullQuantumState>(new FullQuantumState(
                    config.seed,
                    func->GetNumQubits(),
                    config.use_qulacs,
                    config.save_operation_matrix
            ));
            break;
        }
        case StateType::Clifford: {
            state_ = std::unique_ptr<CliffordState>(
                    new CliffordState(config.seed, func->GetNumQubits())
            );
            break;
        }
        default:
            throw std::runtime_error("unimplemented state type");
    }
    const auto& arg = func->GetArgument();
    for (const auto& [name, size] : arg.qubits) {
        state_->AddQubit(name, size);
    }
    for (const auto& [name, size] : arg.registers) {
        state_->AddRegister(name, size);
    }
    lc_.pc = func->GetEntryBB()->cbegin();
    lc_.qmap.resize(func->GetNumQubits());
    std::iota(lc_.qmap.begin(), lc_.qmap.end(), 0);
    lc_.rmap.resize(func->GetNumRegisters() + func->GetNumTmpRegisters());
    std::iota(lc_.rmap.begin(), lc_.rmap.end(), 0);
    unique_register_id_ = lc_.rmap.size();
}
const ToffoliState& Simulator::GetToffoliState() const {
    if (config_.state_type != SimulatorConfig::StateType::Toffoli) {
        throw std::runtime_error("set Toffoli in SimulatorConfig");
    }
    return *static_cast<const ToffoliState*>(state_.get());
}
const FullQuantumState& Simulator::GetFullQuantumState() const {
    if (config_.state_type != SimulatorConfig::StateType::FullQuantum) {
        throw std::runtime_error("set FullQuantum in SimulatorConfig");
    }
    return *static_cast<const FullQuantumState*>(state_.get());
}
const CliffordState& Simulator::GetCliffordState() const {
    if (config_.state_type != SimulatorConfig::StateType::Clifford) {
        throw std::runtime_error("set Clifford in SimulatorConfig");
    }
    return *static_cast<const CliffordState*>(state_.get());
}
bool Simulator::Finished() const {
    return call_stack_.empty() && lc_.pc->GetOpcode().GetCode() == Opcode::Table::Return;
}
void Simulator::SetAs1(std::uint64_t q) {
    state_->X(q);
}
void Simulator::SetValue(std::uint64_t start, std::uint64_t size, const BigInt& value) {
    for (auto i = std::uint64_t{0}; i < size; ++i) {
        const auto x = (value >> i) & 1;
        if (x == 1) {
            state_->X(start + i);
        }
    }
}
void Simulator::SetValueByName(std::string_view name, const BigInt& value) {
    const auto find_qubit = [name, this]() {
        const auto& argument = func_->GetArgument();
        bool find = false;
        auto start = std::uint64_t{0};
        auto size = std::uint64_t{0};
        for (const auto& [tmp_name, tmp_size] : argument.qubits) {
            if (name == tmp_name) {
                find = true;
                size = tmp_size;
                break;
            } else {
                start += tmp_size;
            }
        }
        return std::make_tuple(find, start, size);
    };
    const auto find_register = [name, this]() {
        const auto& argument = func_->GetArgument();
        bool find = false;
        auto start = std::uint64_t{0};
        auto size = std::uint64_t{0};
        for (const auto& [tmp_name, tmp_size] : argument.registers) {
            if (name == tmp_name) {
                find = true;
                size = tmp_size;
                break;
            } else {
                start += tmp_size;
            }
        }
        return std::make_tuple(find, start, size);
    };

    {
        const auto [find, start, size] = find_qubit();
        if (find) {
            SetValue(start, size, value);
            return;
        }
    }

    {
        const auto [find, start, size] = find_register();
        if (find) {
            state_->WriteRegisters(start, size, value);
            return;
        }
    }

    throw std::runtime_error("Failed to set value. Unknown name: " + std::string(name));
}
Simulator::iterator Simulator::Step() {
    if (Finished()) {
        throw std::runtime_error("finished");
    }
    const auto ret = PeekInst();
    if (PeekInst()->IsCall()) {
        StepInto();
        StepOut();
    } else {
        const auto error_msg =
                RunInst(state_.get(), PeekInst().GetPointer(), config_, lc_.qmap, lc_.rmap);
        if (error_msg) {
            auto ss = std::stringstream();
            ss << error_msg.value();
            ss << " ";
            PrintCallStack(ss);
            throw std::runtime_error(ss.str());
        }
        ToNextInstruction();
    }
    return ret;
}
Simulator::iterator Simulator::StepInto() {
    if (Finished()) {
        throw std::runtime_error("finished");
    }
    if (!PeekInst()->IsCall()) {
        throw std::runtime_error("not CallInst");
    }
    const auto ret = PeekInst();
    ToNextInstruction();
    return ret;
}
Simulator::iterator Simulator::StepOut() {
    if (Finished()) {
        throw std::runtime_error("finished");
    }
    const auto ret = PeekInst();
    const auto cur_depth = GetCallDepth();
    if (cur_depth == 0) {
        RunAll();
    } else {
        while (GetCallDepth() > cur_depth - 1) {
            Step();
        }
    }
    return ret;
}
Simulator::iterator Simulator::JumpToNextBreakPoint() {
    while (!Finished()) {
        if (PointsToCallInst()) {
            StepInto();
        } else {
            Step();
        }
        auto tmp = PeekInst();
        if (tmp->HasMetadata(ir::MDKind::BreakPoint)) {
            break;
        }
    }
    return PeekInst();
}
Simulator::iterator Simulator::JumpTo(std::string_view name) {
    while (true) {
        auto tmp = JumpToNextBreakPoint();
        if (tmp->IsTerminator()) {
            break;
        } else {
            const auto* md_tmp = PeekInst()->GetMetadata(ir::MDKind::BreakPoint);
            const auto* di = static_cast<const ir::DIBreakPoint*>(md_tmp);
            if (di->GetName() == name) {
                break;
            }
        }
    }
    return PeekInst();
}
void Simulator::RunAll() {
    while (!Finished()) {
        Step();
    }
}
void Simulator::PrintCallStack(std::ostream& out) const {
    out << "\n\n";
    out << "Call stack:\n";
    for (auto j = std::size_t{0}; j <= GetCallDepth(); ++j) {
        const auto i = GetCallDepth() - j;
        const auto& lc = i == GetCallDepth() ? lc_ : call_stack_[i];
        out << fmt::format("#{:<3}", j);
        lc.pc->Print(out);
        out << "    ";
        lc.pc->PrintMetadata(out);
        out << "\n";
    }
}
void Simulator::ToNextInstruction() {
    if (Finished()) {
        throw std::runtime_error("finished");
    }
    if (PeekInst()->IsCall()) {
        const auto* inst = Cast<ir::CallInst>(PeekInst().GetPointer());
        StepIntoImpl(inst);
    } else {
        std::advance(lc_.pc, 1);
    }

    while (PeekInst()->IsTerminator()) {
        const auto code = PeekInst()->GetOpcode().GetCode();
        if (code == Opcode::Table::Branch) {
            const auto* inst = Cast<ir::BranchInst>(PeekInst().GetPointer());
            ir::BasicBlock* next_bb = nullptr;
            if (inst->IsUnconditional()) {
                next_bb = inst->GetTrueBB();
            } else {
                const auto& cond = inst->GetCondition();
                const auto result = state_->ReadRegister(lc_.rmap[cond.id]);
                next_bb = result ? inst->GetTrueBB() : inst->GetFalseBB();
            }
            LOG_DEBUG("Jump to %{} from %{}", next_bb->GetName(), inst->GetParent()->GetName());
            lc_.pc = next_bb->cbegin();
        } else if (code == Opcode::Table::Switch) {
            const auto* inst = Cast<ir::SwitchInst>(PeekInst().GetPointer());
            const auto& value_register = inst->GetValue();
            auto value_list = std::vector<bool>{};
            for (const auto r : value_register) {
                value_list.emplace_back(state_->ReadRegister(lc_.rmap[r.id]));
            }
            const auto value = BoolArrayAsInt(value_list);
            auto* const next_bb = (inst->GetCaseBB().contains(value))
                    ? (inst->GetCaseBB()).at(value)
                    : inst->GetDefaultBB();
            LOG_DEBUG(
                    "Jump to %{} from %{} (value: {})",
                    next_bb->GetName(),
                    inst->GetParent()->GetName(),
                    value
            );
            lc_.pc = next_bb->cbegin();
        } else if (code == Opcode::Table::Return) {
            // simulator ends if the depth of call is zero
            // the last state of simulator points to the Return instruction
            if (GetCallDepth() == 0) {
                break;
            }

            StepOutImpl();
        } else {
            assert(0 && "unreachable");
        }
    }
}
void Simulator::StepIntoImpl(const ir::CallInst* inst) {
    call_stack_.push_back(lc_);
    lc_.pc = inst->GetCallee()->GetEntryBB()->cbegin();

    const auto& operate = inst->GetOperate();
    lc_.qmap.resize(operate.size());
    for (auto i = std::size_t{0}; i < operate.size(); ++i) {
        lc_.qmap[i] = call_stack_.back().qmap[operate[i].id];
    }

    const auto& input = inst->GetInput();
    const auto& output = inst->GetOutput();
    const auto num_tmp_registers = inst->GetCallee()->GetNumTmpRegisters();
    lc_.rmap.resize(input.size() + output.size() + num_tmp_registers);
    for (auto i = std::size_t{0}; i < input.size(); ++i) {
        lc_.rmap[i] = call_stack_.back().rmap[input[i].id];
    }
    for (auto i = std::size_t{0}; i < output.size(); ++i) {
        lc_.rmap[i + input.size()] = call_stack_.back().rmap[output[i].id];
    }
    for (auto i = std::size_t{0}; i < num_tmp_registers; ++i) {
        lc_.rmap[i + input.size() + output.size()] = GetUniqueRegisterId();
    }
}
void Simulator::StepOutImpl() {
    lc_ = call_stack_.back();
    std::advance(lc_.pc, 1);
    call_stack_.pop_back();
}
}  // namespace qret::runtime
