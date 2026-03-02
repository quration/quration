/**
 * @file qret/transforms/ipo/inliner.h
 * @brief Inline Call instruction.
 */

#ifndef QRET_TRANSFORMS_IPO_INLINER_H
#define QRET_TRANSFORMS_IPO_INLINER_H

#include "qret/ir/function.h"
#include "qret/ir/function_pass.h"
#include "qret/ir/instructions.h"
#include "qret/qret_export.h"

namespace qret::ir {
/**
 * @brief Inlines a call instruction.
 * @details This function attempts to replace the given **call instruction** with the
 * **sequence of instructions from the callee func** directly at the call site.
 * It returns `false` if any of the following are true:
 * - The `CallInst` object itself (`call`) is `nullptr`.
 * - The parent basic block of the `CallInst` (`call->GetParent()`) is `nullptr`.
 * - The parent func of the basic block (`bb->GetParent()`) is `nullptr`.
 * - The callee func retrieved from the `CallInst` (`call->GetCallee()`) is `nullptr`.
 *
 * In all other cases where the necessary components are valid, the inlining operation is performed.
 *
 * @param call The CallInst object to be inlined.
 * @param merge_trivial If true, merges trivial basic blocks (those containing only a branch or a
 * single instruction) after inlining to optimize the control flow.
 * @return true if the inlining was successfully performed.
 * @return false otherwise due to a `nullptr` dependency.
 */
QRET_EXPORT bool InlineCallInst(CallInst* call, bool merge_trivial);

/**
 * @brief Performs non-recursive inlining of all call instructions.
 *
 * @details This pass iterates through a func's functions and inlines **all** call instructions
 * encountered. It does *not* recursively inline calls found within the newly inlined func body.
 */
class QRET_EXPORT InlinerPass : public FunctionPass {
public:
    static inline char ID = 0;
    bool merge_trivial = true;  //!< If true, merges trivial basic blocks after inlining.

    InlinerPass()
        : FunctionPass(&ID) {}

    bool RunOnFunction(Function& func) override;
};

/**
 * @brief Performs recursive inlining of all call instructions.
 *
 * @details This pass is similar to `InlinerPass`, but it recursively inlines calls. If a func is
 * inlined and its body contains further call instructions, those nested calls will **also** be
 * inlined. This enables deep inlining of function call chains.
 */
class QRET_EXPORT RecursiveInlinerPass : public FunctionPass {
public:
    static inline char ID = 0;
    bool merge_trivial = true;  //!< If true, merges trivial basic blocks after inlining.
    RecursiveInlinerPass()
        : FunctionPass(&ID) {}

    bool RunOnFunction(Function& func) override;
};
}  // namespace qret::ir

#endif  // QRET_TRANSFORMS_IPO_INLINER_H
