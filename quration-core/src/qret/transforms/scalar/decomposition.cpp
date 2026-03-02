/**
 * @file qret/transforms/scalar/decomposition.cpp
 * @brief Decompose into lower level instructions.
 */

#include "qret/transforms/scalar/decomposition.h"

#include <numbers>
#include <stdexcept>

#include "qret/base/cast.h"
#include "qret/base/gridsynth.h"
#include "qret/ir/instruction.h"
#include "qret/ir/instructions.h"

namespace qret::ir {
namespace {
static auto X = RegisterPass<DecomposeInst>("DecomposeInst", "ir::decompose_inst");
}

Instruction* DecomposeRotation(ParametrizedRotationInst* const inst) {
    static constexpr auto W = std::numbers::pi / 4;

    const auto q = inst->GetQubit();
    const auto theta = inst->GetTheta();
    const auto code = inst->GetOpcode().GetCode();
    Instruction* ret = nullptr;
    // Prefix for RX and RY
    if (code == Opcode::Table::RX) {
        UnaryInst::CreateHInst(q, inst);
    } else if (code == Opcode::Table::RY) {
        UnaryInst::CreateSDagInst(q, inst);
        UnaryInst::CreateHInst(q, inst);
    }
    // Decomposes RZ using GridSynth
    {
        auto str = std::string("");
        auto exit = std::int32_t{0};
        ApproximateRzWithCliffordT(theta.value, str, exit, theta.precision);
        if (exit != 0) {
            throw std::runtime_error("failed to run GridSynth");
        }
        for (auto itr = str.rbegin(); itr != str.rend(); ++itr) {
            switch (*itr) {
                case 'X':
                    ret = UnaryInst::CreateXInst(q, inst);
                    break;
                case 'H':
                    ret = UnaryInst::CreateHInst(q, inst);
                    break;
                case 'S':
                    ret = UnaryInst::CreateSInst(q, inst);
                    break;
                case 'T':
                    ret = UnaryInst::CreateTInst(q, inst);
                    break;
                case 'W':
                    ret = GlobalPhaseInst::Create({W, 0}, inst);
                    break;
                case 'I':
                    ret = UnaryInst::CreateIInst(q, inst);
                    break;
                default:
                    throw std::runtime_error(
                            std::string("unknown char in the output of GridSynth: ") + *itr
                    );
            }
        }
    }
    // Suffix for RX and RY
    if (code == Opcode::Table::RX) {
        ret = UnaryInst::CreateHInst(q, inst);
    } else if (code == Opcode::Table::RY) {
        UnaryInst::CreateHInst(q, inst);
        ret = UnaryInst::CreateSInst(q, inst);
    }
    EraseFromParent(inst);
    return ret;
}
Instruction* DecomposeBinary(BinaryInst* inst) {
    const auto q0 = inst->GetQubit0();
    const auto q1 = inst->GetQubit1();
    const auto code = inst->GetOpcode().GetCode();
    Instruction* ret = nullptr;
    if (code == Opcode::Table::CX) {
        ret = inst;
    } else if (code == Opcode::Table::CY) {
        UnaryInst::CreateSDagInst(q0, inst);
        BinaryInst::CreateCXInst(q0, q1, inst);
        ret = UnaryInst::CreateSInst(q0, inst);
        EraseFromParent(inst);
    } else if (code == Opcode::Table::CZ) {
        UnaryInst::CreateHInst(q0, inst);
        BinaryInst::CreateCXInst(q0, q1, inst);
        ret = UnaryInst::CreateHInst(q0, inst);
        EraseFromParent(inst);
    }
    return ret;
}
Instruction* DecomposeTernary(TernaryInst* inst) {
    const auto q0 = inst->GetQubit0();
    const auto q1 = inst->GetQubit1();
    const auto q2 = inst->GetQubit2();
    const auto code = inst->GetOpcode().GetCode();
    Instruction* ret = nullptr;
    // Prefix for CCY and CCX
    if (code == Opcode::Table::CCY) {
        UnaryInst::CreateSDagInst(q0, inst);
    } else if (code == Opcode::Table::CCZ) {
        UnaryInst::CreateHInst(q0, inst);
    }
    // Decomposes CCX (ref: https://en.wikipedia.org/wiki/Toffoli_gate)
    {
        UnaryInst::CreateHInst(q0, inst);
        BinaryInst::CreateCXInst(q0, q1, inst);
        UnaryInst::CreateTDagInst(q0, inst);
        BinaryInst::CreateCXInst(q0, q2, inst);
        UnaryInst::CreateTInst(q0, inst);
        BinaryInst::CreateCXInst(q0, q1, inst);
        UnaryInst::CreateTDagInst(q0, inst);
        BinaryInst::CreateCXInst(q0, q2, inst);
        UnaryInst::CreateTInst(q0, inst);
        UnaryInst::CreateTInst(q1, inst);
        UnaryInst::CreateHInst(q0, inst);
        BinaryInst::CreateCXInst(q1, q2, inst);
        UnaryInst::CreateTDagInst(q1, inst);
        UnaryInst::CreateTInst(q2, inst);
        ret = BinaryInst::CreateCXInst(q1, q2, inst);
    }
    // Suffix for CCY and CCZ
    if (code == Opcode::Table::CCY) {
        ret = UnaryInst::CreateSInst(q0, inst);
    } else if (code == Opcode::Table::CCZ) {
        ret = UnaryInst::CreateHInst(q0, inst);
    }
    EraseFromParent(inst);
    return ret;
}
bool DecomposeInst::RunOnFunction(Function& func) {
    auto changed = false;
    for (auto&& bb : func) {
        auto itr = bb.begin();
        while (itr != bb.end()) {
            // Decompose following instructions
            if (itr->IsParametrizedRotation()) {
                // RX, RY, RZ
                auto* inst = Cast<ParametrizedRotationInst>(itr.GetPointer());
                auto* last = DecomposeRotation(inst);
                itr = bb.GetIteratorFromPointer(last);
                changed = true;
            } else if (itr->IsBinary() && itr->GetOpcode().GetCode() != Opcode::Table::CX) {
                // CY, CZ
                auto* inst = Cast<BinaryInst>(itr.GetPointer());
                auto* last = DecomposeBinary(inst);
                itr = bb.GetIteratorFromPointer(last);
                changed = true;
            } else if (itr->IsTernary()) {
                // CCX, CCY, CCZ
                auto* inst = Cast<TernaryInst>(itr.GetPointer());
                auto* last = DecomposeTernary(inst);
                itr = bb.GetIteratorFromPointer(last);
                changed = true;
            } else if (itr->IsCall() && recursive_) {
                // Applies pass recursively
                auto* inst = Cast<CallInst>(itr.GetPointer());
                auto* callee = inst->GetCallee();
                if (!done_.contains(callee)) {
                    changed |= RunOnFunction(*callee);
                    done_.insert(callee);
                }
            }

            // go on to the next instruction
            ++itr;
        }
    }
    return changed;
}
}  // namespace qret::ir
