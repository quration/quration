/**
 * @file qret/target/sc_ls_fixed_v0/validation.h
 * @brief Validate SC_LS_FIXED_V0 machine function.
 */

#ifndef QRET_TARGET_SC_LS_FIXED_V0_VALIDATION_H
#define QRET_TARGET_SC_LS_FIXED_V0_VALIDATION_H

#include "qret/codegen/machine_function.h"
#include "qret/qret_export.h"

namespace qret::sc_ls_fixed_v0 {
QRET_EXPORT void Validate(const MachineFunction& mf);
}

#endif  // QRET_TARGET_SC_LS_FIXED_V0_VALIDATION_H
