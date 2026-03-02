/**
 * @file qret/target/tutorial_nisq_v0/target_info/tutorial_nisq_v0_target_info.h
 * @brief TUTORIAL_NISQ_V0 target registration.
 */

#ifndef QRET_TARGET_TUTORIAL_NISQ_V0_TARGET_INFO_TUTORIAL_NISQ_V0_TARGET_INFO_H
#define QRET_TARGET_TUTORIAL_NISQ_V0_TARGET_INFO_TUTORIAL_NISQ_V0_TARGET_INFO_H

#include "qret/qret_export.h"

namespace qret {
class Target;
namespace tutorial_nisq_v0 {
QRET_EXPORT Target* GetTutorialNisqV0Target();
}  // namespace tutorial_nisq_v0
}  // namespace qret

#endif  // QRET_TARGET_TUTORIAL_NISQ_V0_TARGET_INFO_TUTORIAL_NISQ_V0_TARGET_INFO_H
