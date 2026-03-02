/**
 * @file qret/transforms/scalar/delete_consecutive_same_pauli.h
 * @brief Delete consecutive pauli.
 */

#ifndef QRET_TRANSFORMS_SCALAR_DELETE_CONSECUTIVE_SAME_PAULI_H
#define QRET_TRANSFORMS_SCALAR_DELETE_CONSECUTIVE_SAME_PAULI_H

#include "qret/ir/function_pass.h"
#include "qret/qret_export.h"

namespace qret::ir {
/**
 * @brief Performs func optimization by removing Pauli operators when applying the same Pauli
 * operator consecutively to a qubit.
 */
class QRET_EXPORT DeleteConsecutiveSamePauli : public FunctionPass {
public:
    static inline char ID = 0;
    DeleteConsecutiveSamePauli()
        : FunctionPass(&ID) {}

    bool RunOnFunction(Function& func) override;
};
}  // namespace qret::ir

#endif  // QRET_TRANSFORMS_SCALAR_DELETE_CONSECUTIVE_SAME_PAULI_H
