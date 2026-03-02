/**
 * @file qret/transforms/scalar/ignore_global_phase.h
 * @brief Delete GlobalPhase instructions.
 */

#ifndef QRET_TRANSFORMS_SCALAR_IGNORE_GLOBAL_PHASE_H
#define QRET_TRANSFORMS_SCALAR_IGNORE_GLOBAL_PHASE_H

#include <unordered_set>

#include "qret/ir/function.h"
#include "qret/ir/function_pass.h"
#include "qret/qret_export.h"

namespace qret::ir {
/**
 * @brief Delete RotateGlobalPhase instructions.
 */
class QRET_EXPORT IgnoreGlobalPhase : public FunctionPass {
public:
    static inline char ID = 0;
    IgnoreGlobalPhase()
        : FunctionPass(&ID) {}

    bool RunOnFunction(Function& func) override;

private:
    bool recursive_ = true;
    std::unordered_set<const Function*> done_ = {};
};
}  // namespace qret::ir

#endif  // QRET_TRANSFORMS_SCALAR_IGNORE_GLOBAL_PHASE_H
