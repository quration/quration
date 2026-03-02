/**
 * @file qret/algorithm/preparation/arbitrary.h
 * @brief Prepare arbitrary state.
 */

#ifndef QRET_GATE_PREPARATION_ARBITRARY_H
#define QRET_GATE_PREPARATION_ARBITRARY_H

#include <span>

#include "qret/frontend/argument.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/ir/instructions.h"
#include "qret/qret_export.h"

namespace qret::frontend::gate {
//--------------------------------------------------//
// Functions
//--------------------------------------------------//

/**
 * @brief Prepares arbitrary state.
 * @details See https://arxiv.org/abs/1812.00954 for more information.
 *
 * @param probs size of span must be smaller than 2^n, sum is 1.0
 * @param tgt   size = n
 * @param theta size = m
 * @param aux   size = n-1
 */
QRET_EXPORT ir::CallInst* PrepareArbitraryState(
        std::span<const double> probs,
        const Qubits& tgt,
        const Qubits& theta,
        const CleanAncillae& aux
);

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

/**
 * @brief Implementation of PrepareArbitraryState.
 */
struct QRET_EXPORT PrepareArbitraryStateGen : public CircuitGenerator {
    std::span<const double> probs;
    std::size_t n;  // size of tgt
    std::size_t m;  // size of theta

    static inline const char* Name = "PrepareArbitraryState";
    explicit PrepareArbitraryStateGen(
            CircuitBuilder* builder,
            std::span<const double> probs,
            std::size_t n,
            std::size_t m
    )
        : CircuitGenerator(builder)
        , probs{probs}
        , n{n}
        , m{m} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("tgt", Type::Qubit, n, Attribute::Operate);
        arg.Add("theta", Type::Qubit, m, Attribute::Operate);
        arg.Add("aux", Type::Qubit, n - 1, Attribute::CleanAncilla);
    }
    Circuit* Generate() const override;
};
}  // namespace qret::frontend::gate

#endif  // QRET_GATE_PREPARATION_ARBITRARY_H
