/**
 * @file qret/algorithm/util/reset.h
 */

#ifndef QRET_GATE_UTIL_RESET_H
#define QRET_GATE_UTIL_RESET_H

#include "qret/frontend/argument.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/ir/instructions.h"
#include "qret/qret_export.h"

namespace qret::frontend::gate {
//--------------------------------------------------//
// Functions
//--------------------------------------------------//

/**
 * @brief Resets a qubit.
 *
 * @param q size = 1
 */
QRET_EXPORT ir::CallInst* Reset(const Qubit& q);

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

/**
 * @brief Implementation of Reset.
 */
struct QRET_EXPORT ResetGen : CircuitGenerator {
    static inline const char* Name = "Reset";
    explicit ResetGen(CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return Name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 1, Attribute::Operate);
    }
    Circuit* Generate() const override;
};
}  // namespace qret::frontend::gate

#endif  // QRET_GATE_UTIL_RESET_H
