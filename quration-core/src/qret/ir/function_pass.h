/**
 * @file qret/ir/function_pass.h
 * @brief Function pass.
 */

#ifndef QRET_IR_FUNCTION_PASS_H
#define QRET_IR_FUNCTION_PASS_H

#include "qret/ir/function.h"
#include "qret/pass.h"

namespace qret::ir {
/**
 * @brief This class is used to implement most global optimizations.
 */
class FunctionPass : public Pass {
public:
    explicit FunctionPass(PassIDType pass_id)
        : Pass(pass_id, PassKind::Function) {}
    ~FunctionPass() override = default;

    /**
     * @brief Get what kind of pass manager can manage this pass
     */
    [[nodiscard]] PassManagerType GetPassManagerType() const override {
        return PassManagerType::FunctionPassManager;
    }

    /**
     * @brief Virtual method overridden by subclasses to do the per-func processing of the pass.
     *
     * @return true method changed \p func
     * @return false method didn't change \p func
     */
    virtual bool RunOnFunction(Function& func) = 0;
};
}  // namespace qret::ir

#endif  // QRET_IR_FUNCTION_PASS_H
