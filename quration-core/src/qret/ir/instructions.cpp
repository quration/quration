/**
 * @file qret/ir/instructions.cpp
 * @brief Define each instruction class.
 */

#include "qret/ir/instructions.h"

#include <limits>
#include <stdexcept>
#include <utility>

#include "qret/ir/basic_block.h"
#include "qret/ir/function.h"
#include "qret/ir/value.h"

namespace qret::ir {
MeasurementInst*
MeasurementInst::Create(const Qubit& q, const Register& r, Instruction* insert_before) {
    if (insert_before == nullptr) {
        return nullptr;
    }

    auto inst = std::unique_ptr<MeasurementInst>(new MeasurementInst(q, r));
    auto* ret = inst.get();
    InsertBefore(std::move(inst), insert_before);
    return ret;
}
MeasurementInst*
MeasurementInst::Create(const Qubit& q, const Register& r, BasicBlock* insert_at_end) {
    if (insert_at_end == nullptr) {
        return nullptr;
    }

    auto inst = std::unique_ptr<MeasurementInst>(new MeasurementInst(q, r));
    auto* ret = inst.get();
    InsertAtEnd(std::move(inst), insert_at_end);
    return ret;
}
void MeasurementInst::Print(std::ostream& out) const {
    GetOpcode().Print(out);
    out << " ";
    GetQubit().PrintAsOperand(out);
    out << " ";
    GetRegister().PrintAsOperand(out);
}
UnaryInst* UnaryInst::Create(Opcode opcode, const Qubit& q, Instruction* insert_before) {
    if (insert_before == nullptr) {
        return nullptr;
    }

    auto inst = std::unique_ptr<UnaryInst>(new UnaryInst(opcode, q));
    auto* ret = inst.get();
    InsertBefore(std::move(inst), insert_before);
    return ret;
}
UnaryInst* UnaryInst::Create(Opcode opcode, const Qubit& q, BasicBlock* insert_at_end) {
    if (insert_at_end == nullptr) {
        return nullptr;
    }

    auto inst = std::unique_ptr<UnaryInst>(new UnaryInst(opcode, q));
    auto* ret = inst.get();
    InsertAtEnd(std::move(inst), insert_at_end);
    return ret;
}
void UnaryInst::Print(std::ostream& out) const {
    GetOpcode().Print(out);
    out << " ";
    GetQubit().PrintAsOperand(out);
}
ParametrizedRotationInst* ParametrizedRotationInst::Create(
        Opcode opcode,
        const Qubit& q,
        const FloatWithPrecision& theta,
        Instruction* insert_before
) {
    if (insert_before == nullptr) {
        return nullptr;
    }

    auto inst = std::unique_ptr<ParametrizedRotationInst>(
            new ParametrizedRotationInst(opcode, q, theta)
    );
    auto* ret = inst.get();
    InsertBefore(std::move(inst), insert_before);
    return ret;
}
ParametrizedRotationInst* ParametrizedRotationInst::Create(
        Opcode opcode,
        const Qubit& q,
        const FloatWithPrecision& theta,
        BasicBlock* insert_at_end
) {
    if (insert_at_end == nullptr) {
        return nullptr;
    }

    auto inst = std::unique_ptr<ParametrizedRotationInst>(
            new ParametrizedRotationInst(opcode, q, theta)
    );
    auto* ret = inst.get();
    InsertAtEnd(std::move(inst), insert_at_end);
    return ret;
}
void ParametrizedRotationInst::Print(std::ostream& out) const {
    GetOpcode().Print(out);
    out << " ";
    GetQubit().PrintAsOperand(out);
    out << " ";
    // TODO: Implement Print() with FloatWithPrecision
    out << GetTheta().value << " " << GetTheta().precision;
}
BinaryInst*
BinaryInst::Create(Opcode opcode, const Qubit& q0, const Qubit& q1, Instruction* insert_before) {
    if (insert_before == nullptr) {
        return nullptr;
    }

    auto inst = std::unique_ptr<BinaryInst>(new BinaryInst(opcode, q0, q1));
    auto* ret = inst.get();
    InsertBefore(std::move(inst), insert_before);
    return ret;
}
BinaryInst*
BinaryInst::Create(Opcode opcode, const Qubit& q0, const Qubit& q1, BasicBlock* insert_at_end) {
    if (insert_at_end == nullptr) {
        return nullptr;
    }

    auto inst = std::unique_ptr<BinaryInst>(new BinaryInst(opcode, q0, q1));
    auto* ret = inst.get();
    InsertAtEnd(std::move(inst), insert_at_end);
    return ret;
}
void BinaryInst::Print(std::ostream& out) const {
    GetOpcode().Print(out);
    out << " ";
    GetQubit0().PrintAsOperand(out);
    out << " ";
    GetQubit1().PrintAsOperand(out);
}
TernaryInst* TernaryInst::Create(
        Opcode opcode,
        const Qubit& q0,
        const Qubit& q1,
        const Qubit& q2,
        Instruction* insert_before
) {
    if (insert_before == nullptr) {
        return nullptr;
    }

    auto inst = std::unique_ptr<TernaryInst>(new TernaryInst(opcode, q0, q1, q2));
    auto* ret = inst.get();
    InsertBefore(std::move(inst), insert_before);
    return ret;
}
TernaryInst* TernaryInst::Create(
        Opcode opcode,
        const Qubit& q0,
        const Qubit& q1,
        const Qubit& q2,
        BasicBlock* insert_at_end
) {
    if (insert_at_end == nullptr) {
        return nullptr;
    }

    auto inst = std::unique_ptr<TernaryInst>(new TernaryInst(opcode, q0, q1, q2));
    auto* ret = inst.get();
    InsertAtEnd(std::move(inst), insert_at_end);
    return ret;
}
void TernaryInst::Print(std::ostream& out) const {
    GetOpcode().Print(out);
    out << " ";
    GetQubit0().PrintAsOperand(out);
    out << " ";
    GetQubit1().PrintAsOperand(out);
    out << " ";
    GetQubit2().PrintAsOperand(out);
}
MultiControlInst* MultiControlInst::Create(
        Opcode opcode,
        const Qubit& t,
        const std::vector<Qubit>& c,
        Instruction* insert_before
) {
    if (insert_before == nullptr) {
        return nullptr;
    }

    auto inst = std::unique_ptr<MultiControlInst>(new MultiControlInst(opcode, t, c));
    auto* ret = inst.get();
    InsertBefore(std::move(inst), insert_before);
    return ret;
}
MultiControlInst* MultiControlInst::Create(
        Opcode opcode,
        const Qubit& t,
        const std::vector<Qubit>& c,
        BasicBlock* insert_at_end
) {
    if (insert_at_end == nullptr) {
        return nullptr;
    }

    auto inst = std::unique_ptr<MultiControlInst>(new MultiControlInst(opcode, t, c));
    auto* ret = inst.get();
    InsertAtEnd(std::move(inst), insert_at_end);
    return ret;
}
void MultiControlInst::Print(std::ostream& out) const {
    GetOpcode().Print(out);
    out << " ";
    GetTarget().PrintAsOperand(out);
    for (const auto& c : GetControl()) {
        out << " ";
        c.PrintAsOperand(out);
    }
}
GlobalPhaseInst*
GlobalPhaseInst::Create(const FloatWithPrecision& theta, Instruction* insert_before) {
    if (insert_before == nullptr) {
        return nullptr;
    }

    auto inst = std::unique_ptr<GlobalPhaseInst>(new GlobalPhaseInst(theta));
    auto* ret = inst.get();
    InsertBefore(std::move(inst), insert_before);
    return ret;
}
GlobalPhaseInst*
GlobalPhaseInst::Create(const FloatWithPrecision& theta, BasicBlock* insert_at_end) {
    if (insert_at_end == nullptr) {
        return nullptr;
    }

    auto inst = std::unique_ptr<GlobalPhaseInst>(new GlobalPhaseInst(theta));
    auto* ret = inst.get();
    InsertAtEnd(std::move(inst), insert_at_end);
    return ret;
}
void GlobalPhaseInst::Print(std::ostream& out) const {
    GetOpcode().Print(out);
    out << " " << GetTheta().value << " " << GetTheta().precision;
}
CallInst* CallInst::Create(
        Function* callee,
        const std::vector<Qubit>& operate,
        const std::vector<Register>& input,
        const std::vector<Register>& output,
        Instruction* insert_before
) {
    if (insert_before == nullptr) {
        return nullptr;
    }

    auto inst = std::unique_ptr<CallInst>(new CallInst(callee, operate, input, output));
    auto* ret = inst.get();
    InsertBefore(std::move(inst), insert_before);
    return ret;
}
CallInst* CallInst::Create(
        Function* callee,
        const std::vector<Qubit>& operate,
        const std::vector<Register>& input,
        const std::vector<Register>& output,
        BasicBlock* insert_at_end
) {
    if (insert_at_end == nullptr) {
        return nullptr;
    }

    auto inst = std::unique_ptr<CallInst>(new CallInst(callee, operate, input, output));
    auto* ret = inst.get();
    InsertAtEnd(std::move(inst), insert_at_end);
    return ret;
}
void CallInst::Print(std::ostream& out) const {
    GetOpcode().Print(out);
    const char* sep = "";
    out << " @";
    out << callee_->GetName();
    out << "({";
    for (const auto& q : GetOperate()) {
        out << std::exchange(sep, " ");
        q.PrintAsOperand(out);
    }
    out << "},{";
    sep = "";
    for (const auto& r : GetInput()) {
        out << std::exchange(sep, " ");
        r.PrintAsOperand(out);
    }
    out << "},{";
    sep = "";
    for (const auto& r : GetOutput()) {
        out << std::exchange(sep, " ");
        r.PrintAsOperand(out);
    }
    out << "})";
}
CallCFInst* CallCFInst::Create(
        const PortableAnnotatedFunction& function,
        const std::vector<Register>& input,
        const std::vector<Register>& output,
        Instruction* insert_before
) {
    if (insert_before == nullptr) {
        return nullptr;
    }

    auto inst = std::unique_ptr<CallCFInst>(new CallCFInst(function, input, output));
    auto* ret = inst.get();
    InsertBefore(std::move(inst), insert_before);
    return ret;
}
CallCFInst* CallCFInst::Create(
        const PortableAnnotatedFunction& function,
        const std::vector<Register>& input,
        const std::vector<Register>& output,
        BasicBlock* insert_at_end
) {
    if (insert_at_end == nullptr) {
        return nullptr;
    }

    auto inst = std::unique_ptr<CallCFInst>(new CallCFInst(function, input, output));
    auto* ret = inst.get();
    InsertAtEnd(std::move(inst), insert_at_end);
    return ret;
}
void CallCFInst::Print(std::ostream& out) const {
    GetOpcode().Print(out);
    const char* sep = "";
    out << " @";
    out << function_.Name();
    out << "({";
    for (const auto& r : GetInput()) {
        out << std::exchange(sep, " ");
        r.PrintAsOperand(out);
    }
    out << "},{";
    sep = "";
    for (const auto& r : GetOutput()) {
        out << std::exchange(sep, " ");
        r.PrintAsOperand(out);
    }
    out << "})";
}
namespace {
void CheckCRandInput(const std::vector<double>& weights, const std::vector<Register>& registers) {
    auto sum = 0.0;
    for (const auto w : weights) {
        if (w < 0.0) {
            throw std::runtime_error("Negative found in CRand weights.");
        }
        sum += w;
    }
    if (sum < std::numeric_limits<double>::epsilon()) {
        throw std::runtime_error("Sum of weights must be positive.");
    }
    const auto min_size =
            static_cast<std::size_t>(std::ceil(std::log2(static_cast<double>(weights.size()))));
    if (registers.size() < min_size) {
        throw std::runtime_error(
                fmt::format(
                        "Size of 'registers' ({}) must be larger than 'ceil(log2(weights.size()))' "
                        "({}).",
                        registers.size(),
                        min_size
                )
        );
    }
}
}  // namespace
DiscreteDistInst* DiscreteDistInst::Create(
        const std::vector<double>& weights,
        const std::vector<Register>& registers,
        Instruction* insert_before
) {
    if (insert_before == nullptr) {
        return nullptr;
    }

    CheckCRandInput(weights, registers);

    auto inst = std::unique_ptr<DiscreteDistInst>(new DiscreteDistInst(weights, registers));
    auto* ret = inst.get();
    InsertBefore(std::move(inst), insert_before);
    return ret;
}
DiscreteDistInst* DiscreteDistInst::Create(
        const std::vector<double>& weights,
        const std::vector<Register>& registers,
        BasicBlock* insert_at_end
) {
    if (insert_at_end == nullptr) {
        return nullptr;
    }

    CheckCRandInput(weights, registers);

    auto inst = std::unique_ptr<DiscreteDistInst>(new DiscreteDistInst(weights, registers));
    auto* ret = inst.get();
    InsertAtEnd(std::move(inst), insert_at_end);
    return ret;
}
void DiscreteDistInst::Print(std::ostream& out) const {
    GetOpcode().Print(out);
    const char* sep = "";
    out << " {";
    for (const auto& w : GetWeights()) {
        out << std::exchange(sep, ",");
        out << w;
    }
    out << "}";
    for (const auto& r : GetRegisters()) {
        out << ' ';
        r.PrintAsOperand(out);
    }
}
BranchInst* BranchInst::Create(BasicBlock* if_true, BasicBlock* insert_at_end) {
    if (insert_at_end == nullptr) {
        return nullptr;
    }

    auto inst = std::unique_ptr<BranchInst>(new BranchInst(if_true, nullptr, Register::Null()));
    auto* ret = inst.get();
    InsertAtEnd(std::move(inst), insert_at_end);
    return ret;
}
BranchInst* BranchInst::Create(
        BasicBlock* if_true,
        BasicBlock* if_false,
        const Register& cond,
        BasicBlock* insert_at_end
) {
    if (insert_at_end == nullptr) {
        return nullptr;
    }

    auto inst = std::unique_ptr<BranchInst>(new BranchInst(if_true, if_false, cond));
    auto* ret = inst.get();
    InsertAtEnd(std::move(inst), insert_at_end);
    return ret;
}
void BranchInst::Print(std::ostream& out) const {
    GetOpcode().Print(out);
    if (IsUnconditional()) {
        out << " %";
        out << GetTrueBB()->GetName();
    } else {
        out << " ";
        GetCondition().PrintAsOperand(out);
        out << " %";
        out << GetTrueBB()->GetName();
        out << " %";
        out << GetFalseBB()->GetName();
    }
}
SwitchInst* SwitchInst::Create(
        const std::vector<Register>& value,
        BasicBlock* default_bb,
        const std::map<std::uint64_t, BasicBlock*>& case_bb,
        BasicBlock* insert_at_end
) {
    if (insert_at_end == nullptr) {
        return nullptr;
    }

    auto inst = std::unique_ptr<SwitchInst>(new SwitchInst(value, default_bb, case_bb));
    auto* ret = inst.get();
    InsertAtEnd(std::move(inst), insert_at_end);
    return ret;
}
void SwitchInst::Print(std::ostream& out) const {
    GetOpcode().Print(out);

    // Print value
    out << " [";
    auto sep = std::string("");
    for (const auto r : GetValue()) {
        out << std::exchange(sep, " ");
        r.PrintAsOperand(out);
    }
    out << ']';

    // Print default
    out << " %";
    out << GetDefaultBB()->GetName();

    // Print cases
    out << " [";
    sep = "";
    for (const auto& [key, bb] : GetCaseBB()) {
        out << std::exchange(sep, " ") << key << ": %" << bb->GetName();
    }
    out << ']';
}
ReturnInst* ReturnInst::Create(BasicBlock* insert_at_end) {
    if (insert_at_end == nullptr) {
        return nullptr;
    }

    auto inst = std::unique_ptr<ReturnInst>(new ReturnInst());
    auto* ret = inst.get();
    InsertAtEnd(std::move(inst), insert_at_end);
    return ret;
}
void ReturnInst::Print(std::ostream& out) const {
    GetOpcode().Print(out);
}
CleanInst* CleanInst::Create(const Qubit& q, Instruction* insert_before) {
    if (insert_before == nullptr) {
        return nullptr;
    }

    auto inst = std::unique_ptr<CleanInst>(new CleanInst(q));
    auto* ret = inst.get();
    InsertBefore(std::move(inst), insert_before);
    return ret;
}
CleanInst* CleanInst::Create(const Qubit& q, BasicBlock* insert_at_end) {
    if (insert_at_end == nullptr) {
        return nullptr;
    }

    auto inst = std::unique_ptr<CleanInst>(new CleanInst(q));
    auto* ret = inst.get();
    InsertAtEnd(std::move(inst), insert_at_end);
    return ret;
}
void CleanInst::Print(std::ostream& out) const {
    GetOpcode().Print(out);
    out << " ";
    GetQubit().PrintAsOperand(out);
}
CleanProbInst* CleanProbInst::Create(
        const Qubit& q,
        const FloatWithPrecision& clean_prob,
        Instruction* insert_before
) {
    if (insert_before == nullptr) {
        return nullptr;
    }

    auto inst = std::unique_ptr<CleanProbInst>(new CleanProbInst(q, clean_prob));
    auto* ret = inst.get();
    InsertBefore(std::move(inst), insert_before);
    return ret;
}
CleanProbInst* CleanProbInst::Create(
        const Qubit& q,
        const FloatWithPrecision& clean_prob,
        BasicBlock* insert_at_end
) {
    if (insert_at_end == nullptr) {
        return nullptr;
    }

    auto inst = std::unique_ptr<CleanProbInst>(new CleanProbInst(q, clean_prob));
    auto* ret = inst.get();
    InsertAtEnd(std::move(inst), insert_at_end);
    return ret;
}
void CleanProbInst::Print(std::ostream& out) const {
    GetOpcode().Print(out);
    out << " ";
    GetQubit().PrintAsOperand(out);
}
DirtyInst* DirtyInst::Create(Opcode opcode, const Qubit& q, Instruction* insert_before) {
    if (insert_before == nullptr) {
        return nullptr;
    }

    auto inst = std::unique_ptr<DirtyInst>(new DirtyInst(opcode, q));
    auto* ret = inst.get();
    InsertBefore(std::move(inst), insert_before);
    return ret;
}
DirtyInst* DirtyInst::Create(Opcode opcode, const Qubit& q, BasicBlock* insert_at_end) {
    if (insert_at_end == nullptr) {
        return nullptr;
    }

    auto inst = std::unique_ptr<DirtyInst>(new DirtyInst(opcode, q));
    auto* ret = inst.get();
    InsertAtEnd(std::move(inst), insert_at_end);
    return ret;
}
void DirtyInst::Print(std::ostream& out) const {
    GetOpcode().Print(out);
    out << " ";
    GetQubit().PrintAsOperand(out);
}
}  // namespace qret::ir
