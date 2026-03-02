/**
 * @file qret/target/tutorial_nisq_v0/tutorial_nisq_v0_target_machine.h
 * @brief Target machine for TUTORIAL_NISQ_V0.
 */

#ifndef QRET_TARGET_TUTORIAL_NISQ_V0_TUTORIAL_NISQ_V0_TARGET_MACHINE_H
#define QRET_TARGET_TUTORIAL_NISQ_V0_TUTORIAL_NISQ_V0_TARGET_MACHINE_H

#include <memory>
#include <string>
#include <string_view>

#include "qret/target/target_machine.h"

namespace qret::tutorial_nisq_v0 {
struct TutorialNisqV0TargetMachine : public qret::TargetMachine {
    static inline auto Name = std::string("TUTORIAL_NISQ_V0Machine");

    [[nodiscard]] std::string_view GetName() const override {
        return Name;
    }

    static std::unique_ptr<TutorialNisqV0TargetMachine> New() {
        return std::unique_ptr<TutorialNisqV0TargetMachine>(new TutorialNisqV0TargetMachine());
    }
};
}  // namespace qret::tutorial_nisq_v0

#endif  // QRET_TARGET_TUTORIAL_NISQ_V0_TUTORIAL_NISQ_V0_TARGET_MACHINE_H
