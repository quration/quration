/**
 * @file qret/target/sc_ls_fixed_v0/runtime_simulation_pruning.h
 * @brief Delete PROBABILITY_HINT inst.
 */

#ifndef QRET_TARGET_SC_LS_FIXED_V0_RUNTIME_SIMULATION_PRUNING_H
#define QRET_TARGET_SC_LS_FIXED_V0_RUNTIME_SIMULATION_PRUNING_H

#include <optional>

#include "qret/codegen/machine_function.h"
#include "qret/codegen/machine_function_pass.h"
#include "qret/qret_export.h"

namespace qret::sc_ls_fixed_v0 {
struct QRET_EXPORT RuntimeSimulationPruning : public MachineFunctionPass {
    static inline char ID = 0;
    explicit RuntimeSimulationPruning(std::optional<std::uint64_t> seed = std::nullopt)
        : MachineFunctionPass(&ID)
        , seed{seed} {}

    bool RunOnMachineFunction(MachineFunction& mf) override;

    std::optional<std::uint64_t> seed;
};
}  // namespace qret::sc_ls_fixed_v0

#endif  // QRET_TARGET_SC_LS_FIXED_V0_RUNTIME_SIMULATION_PRUNING_H
