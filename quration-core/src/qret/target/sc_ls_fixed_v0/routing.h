/**
 * @file qret/target/sc_ls_fixed_v0/routing.h
 * @brief Routing.
 */

#ifndef QRET_TARGET_SC_LS_FIXED_V0_ROUTING_H
#define QRET_TARGET_SC_LS_FIXED_V0_ROUTING_H

#include "qret/codegen/machine_function.h"
#include "qret/codegen/machine_function_pass.h"
#include "qret/qret_export.h"
#include "qret/target/sc_ls_fixed_v0/beat.h"
#include "qret/target/sc_ls_fixed_v0/sc_ls_fixed_v0_target_machine.h"

namespace qret::sc_ls_fixed_v0 {
struct QRET_EXPORT Routing : public MachineFunctionPass {
    static inline char ID = 0;
    Routing()
        : MachineFunctionPass(&ID) {}

    bool RunOnMachineFunction(MachineFunction& mf) override;
};

inline Beat AllowedMaxIdleBeats(
        const ScLsFixedV0MachineOption& option,
        Beat relax_factor = 2,
        Beat relax_offset = 10
) {
    const auto expected = option.use_magic_state_cultivation
            ? 1.0 / std::max(0.001, option.prob_magic_state_creation)
            : 1.0;
    const auto mul = std::max(Beat{1}, static_cast<Beat>(std::ceil(expected)));
    const auto mx = std::max(
            {option.reaction_time,
             mul * option.magic_generation_period,
             option.entanglement_generation_period}
    );

    // Relax allowed limit.
    return relax_factor * (mx + relax_offset);
}
}  // namespace qret::sc_ls_fixed_v0

#endif  // QRET_TARGET_SC_LS_FIXED_V0_ROUTING_H
