/**
 * @file qret/transforms/scalar/delete_opt_hint.cpp
 * @brief Delete optimization hint.
 */

#include "qret/transforms/scalar/delete_opt_hint.h"

#include "qret/ir/instruction.h"

namespace qret::ir {
namespace {
static auto X = RegisterPass<DeleteOptHint>("DeleteOptHint", "ir::delete_opt_hint");
}

bool DeleteOptHint::RunOnFunction(Function& func) {
    auto changed = false;
    for (auto&& bb : func) {
        auto itr = bb.begin();
        while (itr != bb.end()) {
            if (itr->GetOpcode().IsOptHint()) {
                auto* ptr = itr.GetPointer();
                ++itr;
                EraseFromParent(ptr);
                changed = true;
            } else {
                ++itr;
            }
        }
    }
    return changed;
}
}  // namespace qret::ir
