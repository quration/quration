/**
 * @file qret/transforms/ipo/inliner.cpp
 * @brief Inline Call instruction.
 */

#include "qret/transforms/ipo/inliner.h"

#include <cmath>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include "qret/base/cast.h"
#include "qret/ir/basic_block.h"
#include "qret/ir/function.h"
#include "qret/ir/instruction.h"
#include "qret/ir/instructions.h"
#include "qret/ir/value.h"

namespace qret::ir {
namespace {
static auto X = RegisterPass<InlinerPass>("InlinerPass", "ir::inliner");
static auto Y = RegisterPass<RecursiveInlinerPass>("RecursiveInlinerPass", "ir::recursive_inliner");
}  // namespace

namespace {
struct InlineInfo {
    //        prev
    //          |
    //        entry
    //        .....
    //  (inline expansion of callee)
    //        .....
    //        exit
    //          |
    //        next
    //
    // entry == exit if callee has only one BB.

    BasicBlock* prev = nullptr;
    BasicBlock* next = nullptr;
    BasicBlock* entry = nullptr;
    BasicBlock* exit = nullptr;

    std::unordered_map<BasicBlock*, BasicBlock*> bb_map = {};  //!< Map from callee bb to caller bb.

    const std::vector<Qubit>& operate;
    const std::vector<Register>& input;
    const std::vector<Register>& output;
    std::size_t r_tmp_offset = 0;

    BasicBlock* GetCallerBB(BasicBlock* callee_bb) const {
        if (callee_bb == nullptr) {
            return nullptr;
        }
        auto itr = bb_map.find(callee_bb);
        if (itr == bb_map.end()) {
            throw std::logic_error(
                    fmt::format("bb_map does not contain BB of name '{}'", callee_bb->GetName())
            );
        }
        return itr->second;
    }
    Qubit GetCallerQubit(const Qubit& callee_q) const {
        return operate[callee_q.id];
    }
    Register GetCallerRegister(const Register callee_r) const {
        if (callee_r.id < input.size()) {
            return input[callee_r.id];
        }
        if (callee_r.id < input.size() + output.size()) {
            return output[callee_r.id - input.size()];
        }
        return Register{r_tmp_offset + (callee_r.id - input.size() - output.size())};
    }
};

InlineInfo CalcInlineInfo(Function& func, const CallInst& inst) {
    auto info = InlineInfo{
            .operate = inst.GetOperate(),
            .input = inst.GetInput(),
            .output = inst.GetOutput()
    };
    info.r_tmp_offset = func.GetNumRegisters() + func.GetNumTmpRegisters();

    return info;
}

void InlineInstruction(BasicBlock* bb, const InlineInfo& info, const Instruction* inst) {
    const auto convert_q = [&info](const std::vector<Qubit>& callee_qs) {
        auto ret = std::vector<Qubit>{};
        ret.reserve(callee_qs.size());
        for (const auto& callee_q : callee_qs) {
            ret.emplace_back(info.GetCallerQubit(callee_q));
        }
        return ret;
    };
    const auto convert_r = [&info](const std::vector<Register>& callee_rs) {
        auto ret = std::vector<Register>{};
        ret.reserve(callee_rs.size());
        for (const auto& callee_r : callee_rs) {
            ret.emplace_back(info.GetCallerRegister(callee_r));
        }
        return ret;
    };

    if (const auto* tmp = DynCast<MeasurementInst>(inst)) {
        MeasurementInst::Create(
                info.GetCallerQubit(tmp->GetQubit()),
                info.GetCallerRegister(tmp->GetRegister()),
                bb
        );
    } else if (const auto* tmp = DynCast<ParametrizedRotationInst>(inst)) {
        ParametrizedRotationInst::Create(
                inst->GetOpcode(),
                info.GetCallerQubit(tmp->GetQubit()),
                tmp->GetTheta(),
                bb
        );
    } else if (const auto* tmp = DynCast<UnaryInst>(inst)) {
        UnaryInst::Create(inst->GetOpcode(), info.GetCallerQubit(tmp->GetQubit()), bb);
    } else if (const auto* tmp = DynCast<BinaryInst>(inst)) {
        BinaryInst::Create(
                inst->GetOpcode(),
                info.GetCallerQubit(tmp->GetQubit0()),
                info.GetCallerQubit(tmp->GetQubit1()),
                bb
        );
    } else if (const auto* tmp = DynCast<TernaryInst>(inst)) {
        TernaryInst::Create(
                inst->GetOpcode(),
                info.GetCallerQubit(tmp->GetQubit0()),
                info.GetCallerQubit(tmp->GetQubit1()),
                info.GetCallerQubit(tmp->GetQubit2()),
                bb
        );
    } else if (const auto* tmp = DynCast<MultiControlInst>(inst)) {
        MultiControlInst::Create(
                inst->GetOpcode(),
                info.GetCallerQubit(tmp->GetTarget()),
                convert_q(tmp->GetControl()),
                bb
        );
    } else if (const auto* tmp = DynCast<GlobalPhaseInst>(inst)) {
        GlobalPhaseInst::Create(tmp->GetTheta(), bb);
    } else if (const auto* tmp = DynCast<CallInst>(inst)) {
        CallInst::Create(
                tmp->GetCallee(),
                convert_q(tmp->GetOperate()),
                convert_r(tmp->GetInput()),
                convert_r(tmp->GetOutput()),
                bb
        );
    } else if (const auto* tmp = DynCast<CallCFInst>(inst)) {
        CallCFInst::Create(
                tmp->GetFunction(),
                convert_r(tmp->GetInput()),
                convert_r(tmp->GetOutput()),
                bb
        );
    } else if (const auto* tmp = DynCast<DiscreteDistInst>(inst)) {
        DiscreteDistInst::Create(tmp->GetWeights(), convert_r(tmp->GetRegisters()), bb);
    } else if (const auto* tmp = DynCast<BranchInst>(inst)) {
        if (tmp->IsConditional()) {
            BranchInst::Create(
                    info.GetCallerBB(tmp->GetTrueBB()),
                    info.GetCallerBB(tmp->GetFalseBB()),
                    info.GetCallerRegister(tmp->GetCondition()),
                    bb
            );
        } else {
            BranchInst::Create(info.GetCallerBB(tmp->GetTrueBB()), bb);
        }
    } else if (const auto* tmp = DynCast<SwitchInst>(inst)) {
        auto case_bb = std::map<std::uint64_t, BasicBlock*>{};
        for (const auto& [key, bb] : tmp->GetCaseBB()) {
            case_bb[key] = info.GetCallerBB(bb);
        }
        SwitchInst::Create(
                convert_r(tmp->GetValue()),
                info.GetCallerBB(tmp->GetDefaultBB()),
                case_bb,
                bb
        );
    } else if (DynCast<ReturnInst>(inst) != nullptr) {
        // NOTE: Convert Return inst to unconditional BranchInst
        BranchInst::Create(info.next, bb);
    } else if (const auto* tmp = DynCast<CleanInst>(inst)) {
        CleanInst::Create(info.GetCallerQubit(tmp->GetQubit()), bb);
    } else if (const auto* tmp = DynCast<CleanProbInst>(inst)) {
        CleanProbInst::Create(info.GetCallerQubit(tmp->GetQubit()), tmp->GetCleanProb(), bb);
    } else if (const auto* tmp = DynCast<DirtyInst>(inst)) {
        DirtyInst::Create(inst->GetOpcode(), info.GetCallerQubit(tmp->GetQubit()), bb);
    } else {
        auto msg = std::stringstream{};
        inst->Print(msg);
        throw std::logic_error("Unknown instruction: " + msg.str());
    }
}
}  // namespace

bool InlineCallInst(CallInst* call, bool merge_trivial) {
    if (call == nullptr) {
        return false;
    }
    auto* const bb = call->GetParent();
    if (bb == nullptr) {
        return false;
    }
    auto* const func = bb->GetParent();
    if (func == nullptr) {
        return false;
    }
    auto* const callee = call->GetCallee();
    if (callee == nullptr) {
        return false;
    }

    const auto inline_prefix =
            fmt::format("__inline_{}_{}", func->GetNumBBs(), call->GetCallee()->GetName());

    // Create InlineInfo.
    auto info = CalcInlineInfo(*func, *call);

    // Split BasicBlock into two parts:
    // Before call instruction and After call instruction.
    info.prev = bb;
    info.next = bb->Split(call, fmt::format("{}_next", inline_prefix));

    // Create callee BB in the func.
    for (auto& callee_bb : *callee) {
        auto* new_bb = info.bb_map[&callee_bb] =
                BasicBlock::Create(fmt::format("{}_{}", inline_prefix, callee_bb.GetName()), func);
        if (callee_bb.IsEntryBlock()) {
            info.entry = new_bb;
        }
        if (callee_bb.IsExitBlock()) {
            info.exit = new_bb;
        }
    }

    // Pour callee instructions into BB.
    for (auto& callee_bb : *callee) {
        auto* bb = info.GetCallerBB(&callee_bb);
        for (auto succ = callee_bb.succ_begin(); succ != callee_bb.succ_end(); ++succ) {
            bb->AddEdge(info.GetCallerBB(*succ));
        }
        for (auto& callee_inst : callee_bb) {
            InlineInstruction(bb, info, &callee_inst);
        }
    }

    // Increase the number of temporal registers.
    func->AddNumTmpRegisters(callee->GetNumTmpRegisters());

    // Set terminators correctly.
    // Now: prev --> next, Change to: prev --> entry
    if (info.prev->Empty()) {
        throw std::logic_error("BB has no instructions");
    }
    if (auto* inst = DynCast<BranchInst>(info.prev->GetTerminator())) {
        inst->ReplaceBB(info.next, info.entry);
    } else {
        throw std::logic_error("Terminator of non exit BB is not BranchInst");
    }

    // Set BB dependencies.
    {
        info.prev->ClearSuccessors();
        info.next->ClearPredecessors();

        info.prev->AddEdge(info.entry);
        info.exit->AddEdge(info.next);
    }

    // Delete call instruction.
    RemoveFromParent(call);

    if (merge_trivial) {
        MergeBasicBlockIntoOnlyPred(info.entry);
        MergeBasicBlockIntoOnlyPred(info.next);
    }

    return true;
}

bool InlinerPass::RunOnFunction(Function& func) {
    // Get the list of call instructions.
    auto call_inst_list = std::vector<CallInst*>{};
    for (auto& bb : func) {
        for (auto& inst : bb) {
            if (auto* call_inst = DynCast<CallInst>(&inst)) {
                call_inst_list.emplace_back(call_inst);
            }
        }
    }

    if (call_inst_list.empty()) {
        return false;
    }

    for (auto* call_inst : call_inst_list) {
        InlineCallInst(call_inst, merge_trivial);
    }

    return true;
}
bool RecursiveInlinerPass::RunOnFunction(Function& func) {
    auto changed = false;

    auto inliner = InlinerPass();
    inliner.merge_trivial = merge_trivial;
    while (inliner.RunOnFunction(func)) {
        changed = true;
    }

    return changed;
}
}  // namespace qret::ir
