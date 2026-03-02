/**
 * @file qret/transforms/scalar/delete_consecutive_same_pauli.cpp
 * @brief Delete consecutive pauli.
 */

#include "qret/transforms/scalar/delete_consecutive_same_pauli.h"

#include "qret/base/cast.h"
#include "qret/ir/instruction.h"
#include "qret/ir/instructions.h"

namespace qret::ir {
namespace {
static auto X = RegisterPass<DeleteConsecutiveSamePauli>(
        "DeleteConsecutiveSamePauli",
        "ir::delete_consecutive_same_pauli"
);
}

namespace {
std::vector<Qubit> GetTargetQubits(Instruction* inst) {
    if (inst->IsMeasurement()) {
        return {Cast<MeasurementInst>(inst)->GetQubit()};
    } else if (inst->IsUnary()) {
        return {Cast<UnaryInst>(inst)->GetQubit()};
    } else if (inst->IsBinary()) {
        auto* binary_inst = Cast<BinaryInst>(inst);
        return {binary_inst->GetQubit0(), binary_inst->GetQubit1()};
    } else if (inst->IsTernary()) {
        auto* ternary_inst = Cast<TernaryInst>(inst);
        return {ternary_inst->GetQubit0(), ternary_inst->GetQubit1(), ternary_inst->GetQubit2()};
    } else if (inst->IsMultiControl()) {
        auto* multi_control_inst = Cast<MultiControlInst>(inst);
        auto ret = multi_control_inst->GetControl();
        ret.push_back(multi_control_inst->GetTarget());
        return ret;
    } else if (inst->IsCall()) {
        auto* call_inst = Cast<CallInst>(inst);
        return call_inst->GetOperate();
    }

    return {};
}
}  // namespace
bool DeleteConsecutiveSamePauli::RunOnFunction(Function& func) {
    auto changed = false;
    for (auto&& bb : func) {
        // Performs optimization within basic block
        // FIXME: Modify to enable optimization across basic blocks

        // Save the history of Pauli operations performed on qubits
        using Id = decltype(Qubit().id);
        std::map<Id, std::vector<Instruction*>> last_pauli_instructions;

        for (auto itr = bb.begin(); itr != bb.end();) {
            constexpr auto X = Opcode::Table::X;
            constexpr auto Y = Opcode::Table::Y;
            constexpr auto Z = Opcode::Table::Z;
            auto opcode = itr->GetOpcode().GetCode();
            if (opcode == X || opcode == Y || opcode == Z) {
                auto target = Cast<UnaryInst>(itr.GetPointer())->GetQubit();
                auto& pauli_history = last_pauli_instructions[target.id];
                if (!pauli_history.empty()
                    && pauli_history.back()->GetOpcode().GetCode() == opcode) {
                    EraseFromParent(pauli_history.back());
                    EraseFromParent(itr++.GetPointer());
                    pauli_history.pop_back();
                    changed = true;
                } else {
                    pauli_history.push_back(itr++.GetPointer());
                }
            } else {
                for (const auto& target : GetTargetQubits(itr++.GetPointer())) {
                    last_pauli_instructions[target.id].clear();
                }
            }
        }
    }
    return changed;
}
}  // namespace qret::ir
