/**
 * @file qret/algorithm/transform/qft.h
 * @brief QFT.
 */

#ifndef QRET_ALGORITHM_TRANSFORM_QFT_H
#define QRET_ALGORITHM_TRANSFORM_QFT_H

#include <fmt/core.h>

#include "qret/frontend/argument.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/ir/instructions.h"
#include "qret/qret_export.h"

namespace qret::frontend::gate {
//--------------------------------------------------//
// Functions
//--------------------------------------------------//

/**
 * @brief Performs QFT.
 *
 * @param tgt  size = n
 */
QRET_EXPORT ir::CallInst* FourierTransform(const Qubits& tgt);

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

/**
 * @brief Implementation of FourierTransform.
 */
struct QRET_EXPORT FourierTransformGen : CircuitGenerator {
    std::size_t n;

    static inline const char* Name = "FourierTransform";
    explicit FourierTransformGen(CircuitBuilder* builder, std::size_t n)
        : CircuitGenerator(builder)
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({})", GetName(), n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("tgt", Type::Qubit, n, Attribute::Operate);
    }
    Circuit* Generate() const override;
};
}  // namespace qret::frontend::gate

#endif  // QRET_ALGORITHM_TRANSFORM_QFT_H
