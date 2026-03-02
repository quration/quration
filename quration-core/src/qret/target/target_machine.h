/**
 * @file qret/target/target_machine.h
 * @brief Target information.
 * @details This file defines the TargetMachine class.
 */

#ifndef QRET_TARGET_TARGET_MACHINE_H
#define QRET_TARGET_TARGET_MACHINE_H

#include <string_view>

#include "qret/qret_export.h"
#include "qret/target/target_registry.h"

namespace qret {
/**
 * @brief Primary interface to the complete machine description for the target machine. All
 * target-specific information should be accessible through this interface.
 */
class QRET_EXPORT TargetMachine {
public:
    virtual ~TargetMachine() = default;

    [[nodiscard]] virtual std::string_view GetName() const = 0;

    //     const Target& GetTarget() const { return target_; }
    //
    // protected:
    //     explicit TargetMachine(const Target& target) : target_{target} {}
    //
    //     const Target& target_;
};
}  // namespace qret

#endif  // QRET_TARGET_TARGET_MACHINE_H
