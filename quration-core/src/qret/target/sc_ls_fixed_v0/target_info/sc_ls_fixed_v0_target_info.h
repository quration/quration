/**
 * @file qret/target/sc_ls_fixed_v0/target_info/sc_ls_fixed_v0_target_info.h
 * @brief SC_LS_FIXED_V0 target implementation.
 * @details This file provides a getter of SC_LS_FIXED_V0 target.
 */

#ifndef QRET_TARGET_SC_LS_FIXED_V0_TARGET_INFO_SC_LS_FIXED_V0_TARGET_INFO_H
#define QRET_TARGET_SC_LS_FIXED_V0_TARGET_INFO_SC_LS_FIXED_V0_TARGET_INFO_H

#include "qret/qret_export.h"

namespace qret {
class Target;
namespace sc_ls_fixed_v0 {
QRET_EXPORT Target* GetSC_LS_FIXED_V0Target();
}  // namespace sc_ls_fixed_v0
}  // namespace qret

#endif  // QRET_TARGET_SC_LS_FIXED_V0_TARGET_INFO_SC_LS_FIXED_V0_TARGET_INFO_H
