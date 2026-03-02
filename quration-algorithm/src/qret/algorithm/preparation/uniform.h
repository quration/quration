/**
 * @file qret/algorithm/preparation/uniform.h
 * @brief Prepare uniform superposition.
 */

#ifndef QRET_GATE_PREPARATION_UNIFORM_H
#define QRET_GATE_PREPARATION_UNIFORM_H

#include <fmt/core.h>

#include "qret/base/type.h"
#include "qret/frontend/argument.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/ir/instructions.h"
#include "qret/math/integer.h"
#include "qret/qret_export.h"

namespace qret::frontend::gate {
//--------------------------------------------------//
// Functions
//--------------------------------------------------//

/**
 * @brief Creates a uniform superposition over states that encode 0 through `N` - 1.
 * @details Use amplitude amplification technique.
 * See fig12 of https://arxiv.org/abs/1805.03662 for more information.
 *
 * @param N   upper bound, N = 2^k * L (L is odd number)
 * @param tgt size = n >= BitSizeL(N-1)
 * @param aux size = m >= 3 + BitSizeL(L)
 */
QRET_EXPORT ir::CallInst*
PrepareUniformSuperposition(const BigInt& N, const Qubits& tgt, const CleanAncillae& aux);
/**
 * @brief Performs controlled creation of a uniform superposition over states that encode 0 through
 * `N` - 1.
 * @details Use amplitude amplification technique.
 * See fig12 of https://arxiv.org/abs/1805.03662 for more information.
 *
 * @param N   upper bound, N = 2^k * L (L is odd number)
 * @param c   size = 1
 * @param tgt size = n >= BitSizeL(N-1)
 * @param aux size = m >= 3 + BitSizeL(L)
 */
QRET_EXPORT ir::CallInst* ControlledPrepareUniformSuperposition(
        const BigInt& N,
        const Qubit& c,
        const Qubits& tgt,
        const CleanAncillae& aux
);

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

/**
 * @brief Implementation of PrepareUniformSuperposition.
 */
struct QRET_EXPORT PrepareUniformSuperpositionGen : public CircuitGenerator {
    static std::size_t CalcSizeOfAux(BigInt N) {
        while ((N & 1) == 0) {
            N >>= 1;
        }
        return math::BitSizeL(N) + 3;
    }

    BigInt N;
    std::size_t n;  // size of tgt
    std::size_t m;  // size of aux

    static inline const char* Name = "PrepareUniformSuperposition";
    explicit PrepareUniformSuperpositionGen(CircuitBuilder* builder, const BigInt& N)
        : CircuitGenerator(builder)
        , N{N}
        , n{math::BitSizeL(N - 1)}
        , m{CalcSizeOfAux(N)} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({})", GetName(), N);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("tgt", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, m, Attribute::CleanAncilla);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of ControlledPrepareUniformSuperposition.
 */
struct QRET_EXPORT ControlledPrepareUniformSuperpositionGen : public CircuitGenerator {
    static std::size_t CalcSizeOfAux(BigInt N) {
        while ((N & 1) == 0) {
            N >>= 1;
        }
        return math::BitSizeL(N) + 3;
    }

    BigInt N;
    std::size_t n;  // size of tgt
    std::size_t m;  // size of aux

    static inline const char* Name = "ControlledPrepareUniformSuperposition";
    explicit ControlledPrepareUniformSuperpositionGen(CircuitBuilder* builder, const BigInt& N)
        : CircuitGenerator(builder)
        , N{N}
        , n{math::BitSizeL(N - 1)}
        , m{CalcSizeOfAux(N)} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({})", GetName(), N);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("c", Type::Qubit, 1, Attribute::Operate);
        arg.Add("tgt", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, m, Attribute::CleanAncilla);
    }
    Circuit* Generate() const override;
};
}  // namespace qret::frontend::gate

#endif  // QRET_GATE_PREPARATION_UNIFORM_H
