/**
 * @file qret/target/sc_ls_fixed_v0/lowering.h
 * @brief Lowering.
 */

#ifndef QRET_TARGET_SC_LS_FIXED_V0_LOWERING_H
#define QRET_TARGET_SC_LS_FIXED_V0_LOWERING_H

#include "qret/codegen/machine_function.h"
#include "qret/codegen/machine_function_pass.h"
#include "qret/qret_export.h"

namespace qret::sc_ls_fixed_v0 {
struct QRET_EXPORT Lowering : public MachineFunctionPass {
    static inline char ID = 0;
    Lowering()
        : MachineFunctionPass(&ID) {}

    bool RunOnMachineFunction(MachineFunction& mf) override;
};
}  // namespace qret::sc_ls_fixed_v0

#endif  // QRET_TARGET_SC_LS_FIXED_V0_LOWERING_H
