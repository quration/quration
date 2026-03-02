/**
 * @file qret/transforms/scalar/ignore_global_phase.cpp
 * @brief Delete GlobalPhase instructions.
 */

#include "qret/transforms/scalar/ignore_global_phase.h"

#include "qret/base/cast.h"
#include "qret/ir/instruction.h"
#include "qret/ir/instructions.h"

namespace qret::ir {
namespace {
static auto X = RegisterPass<IgnoreGlobalPhase>("IgnoreGlobalPhase", "ir::ignore_global_phase");
}

bool IgnoreGlobalPhase::RunOnFunction(Function& func) {
    auto changed = false;
    for (auto&& bb : func) {
        auto itr = bb.begin();
        while (itr != bb.end()) {
            if (itr->GetOpcode().GetCode() == Opcode::Table::GlobalPhase) {
                auto* ptr = itr.GetPointer();
                ++itr;
                EraseFromParent(ptr);
                changed = true;
            } else if (itr->IsCall() && recursive_) {
                // Applies pass recursively
                auto* inst = Cast<CallInst>(itr.GetPointer());
                auto* callee = inst->GetCallee();
                if (!done_.contains(callee)) {
                    changed |= RunOnFunction(*callee);
                    done_.insert(callee);
                }
                ++itr;
            } else {
                ++itr;
            }
        }
    }
    return changed;
}
}  // namespace qret::ir
