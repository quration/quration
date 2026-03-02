/**
 * @file qret/transforms/scalar/delete_opt_hint.h
 * @brief Delete optimization hint.
 */

#ifndef QRET_TRANSFORMS_SCALAR_DELETE_OPT_HINT_H
#define QRET_TRANSFORMS_SCALAR_DELETE_OPT_HINT_H

#include "qret/ir/function.h"
#include "qret/ir/function_pass.h"
#include "qret/qret_export.h"

namespace qret::ir {
/**
 * @brief Delete optimization hint instructions.
 */
class QRET_EXPORT DeleteOptHint : public FunctionPass {
public:
    static inline char ID = 0;
    DeleteOptHint()
        : FunctionPass(&ID) {}

    bool RunOnFunction(Function& func) override;
};
}  // namespace qret::ir

#endif  // QRET_TRANSFORMS_SCALAR_DELETE_OPT_HINT_H
