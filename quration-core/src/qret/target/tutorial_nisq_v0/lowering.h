/**
 * @file qret/target/tutorial_nisq_v0/lowering.h
 * @brief Lower qret IR into TUTORIAL_NISQ_V0 machine instructions.
 */

#ifndef QRET_TARGET_TUTORIAL_NISQ_V0_LOWERING_H
#define QRET_TARGET_TUTORIAL_NISQ_V0_LOWERING_H

#include "qret/codegen/machine_function.h"
#include "qret/qret_export.h"

namespace qret::tutorial_nisq_v0 {
/**
 * @brief Minimal lowering pass from IR to tutorial machine instruction stream.
 * @details
 * - Requires `MachineFunction::GetIR()` to be non-null.
 * - Rejects control-flow structure beyond a single basic block.
 * - Converts IR `Measurement` to tutorial ISA `MZ`.
 */
class QRET_EXPORT TutorialNisqV0Lowering {
public:
    [[nodiscard]] bool RunOnMachineFunction(qret::MachineFunction& mf) const;
};
}  // namespace qret::tutorial_nisq_v0

#endif  // QRET_TARGET_TUTORIAL_NISQ_V0_LOWERING_H
