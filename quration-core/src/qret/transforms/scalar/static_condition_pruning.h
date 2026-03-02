/**
 * @file qret/transforms/scalar/static_condition_pruning.h
 * @brief Static control-flow pruning pass for branch/switch instructions.
 *
 * The pass keeps a compile-time abstract state per basic block and rewrites
 * conditional terminators only when their guard is fully statically known.
 * It is intentionally conservative: if a predicate could vary at runtime after
 * CFG merge or side-effectful classical updates, the condition remains.
 */

#ifndef QRET_TRANSFORMS_SCALAR_STATIC_CONDITION_PRUNING_H
#define QRET_TRANSFORMS_SCALAR_STATIC_CONDITION_PRUNING_H

#include <optional>

#include "qret/ir/function.h"
#include "qret/ir/function_pass.h"
#include "qret/qret_export.h"

namespace qret::ir {
/**
 * @brief Compile-time pruning of control-flow for proven static predicates.
 *
 * The pass scans each basic block with an abstract register state
 * (register -> True / False / Dynamic), where:
 * - True/False: the register value is known at block entry.
 * - Dynamic: the value is not safe to use for compile-time control-flow reduction.
 *
 * On a static condition match, branch/switch terminators are rewritten into
 * unconditional control-flow edges to the proven target.
 */
class QRET_EXPORT StaticConditionPruningPass : public FunctionPass {
public:
    static inline char ID = 0;
    // Default constructible pass with no explicit seed; when seed is empty,
    // the implementation falls back to its registered option value.
    StaticConditionPruningPass()
        : FunctionPass(&ID) {}

    // Runs the pass on a function and returns true iff the CFG terminators
    // were rewritten in this invocation.
    bool RunOnFunction(Function& func) override;

    // Optional deterministic seed used by discrete-distribution folding inside
    // the static pass. Empty means "use option/default seed".
    std::optional<std::uint64_t> seed;
};
}  // namespace qret::ir

#endif  // QRET_TRANSFORMS_SCALAR_STATIC_CONDITION_PRUNING_H
