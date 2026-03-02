/**
 * @file qret/transforms/scalar/decomposition.h
 * @brief Decompose into lower level instructions.
 */

#ifndef QRET_TRANSFORMS_SCALAR_DECOMPOSITION_H
#define QRET_TRANSFORMS_SCALAR_DECOMPOSITION_H

#include <unordered_set>

#include "qret/ir/function.h"
#include "qret/ir/function_pass.h"
#include "qret/qret_export.h"

namespace qret::ir {
/**
 * @brief Decompose instructions to more trivial instructions.
 * @details Decompose following instructions to the combinations of the trivial instructions.
 *
 * * RX: Decomposes RX into H RZ H and approximate RZ using GridSynth.
 * * RY: Decomposes RY into S H RZ H SDag and approximates RZ using GridSynth.
 * * RZ: Uses GridSynth to approximate z-rotation with {H,S,T,X,W} gates, then converts W to
 * RotateGlobalPhase inst
 * * CY: CY = S CX SDag
 * * CZ: CZ = H CX S
 * * CCX: Decomposes toffoli gate into the combination of {H,T,TDag,CX}, which uses 7 T-gates.
 *     * TODO: if the target of a toffoli gate is clean before toffoli, decomposes into TemporalAnd,
 * which uses 4 T-gates.
 *     * TODO: if the target of a toffoli gate becomes clean after toffoli, decomposes into
 * UncomputeTemporalAnd, which uses 0 T-gates.
 * * CCY: CCY = S CCX SDag
 * * CCZ: CCZ = H CCX H
 *
 */
class QRET_EXPORT DecomposeInst : public FunctionPass {
public:
    static inline char ID = 0;
    DecomposeInst()
        : FunctionPass(&ID) {}

    bool RunOnFunction(Function& func) override;

private:
    bool recursive_ = true;
    std::unordered_set<const Function*> done_ = {};
};
}  // namespace qret::ir

#endif  // QRET_TRANSFORMS_SCALAR_DECOMPOSITION_H
