/**
 * @file qret/algorithm/arithmetic/shor.h
 * @brief Shor.
 */

#ifndef QRET_ALGORITHM_ARITHMETIC_SHOR_H
#define QRET_ALGORITHM_ARITHMETIC_SHOR_H

#include "qret/base/type.h"
#include "qret/frontend/argument.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/math/integer.h"
#include "qret/qret_export.h"

namespace qret::frontend::gate {

/**
 * @brief Implementation of PeriodFinder.
 * @details Implements fig4 of https://arxiv.org/abs/1706.07884
 */
struct QRET_EXPORT PeriodFinderGen : CircuitGenerator {
    qret::BigInt mod;
    qret::BigInt coprime;
    std::size_t depth;
    std::size_t n;  // n = BitSizeL(mod)

    static inline const char* Name = "PeriodFinder";
    explicit PeriodFinderGen(
            CircuitBuilder* builder,
            const BigInt& mod,
            const BigInt& coprime,
            std::size_t depth
    )
        : CircuitGenerator(builder)
        , mod{mod}
        , coprime{coprime}
        , depth{depth}
        , n{math::BitSizeL(mod)} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;  // Do not cache
    void SetArgument(Argument& arg) const override {
        arg.Add("caux", Type::Qubit, n + 2, Attribute::Operate);
        arg.Add("daux", Type::Qubit, n - 1, Attribute::Operate);
        arg.Add("out", Type::Register, depth, Attribute::Output);
    }
    frontend::Circuit* Generate() const override;
};
}  // namespace qret::frontend::gate

#endif  // QRET_ALGORITHM_ARITHMETIC_SHOR_H
