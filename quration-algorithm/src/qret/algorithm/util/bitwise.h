/**
 * @file qret/algorithm/util/bitwise.h
 */

#ifndef QRET_GATE_UTIL_BITWISE_H
#define QRET_GATE_UTIL_BITWISE_H

#include "qret/frontend/argument.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/ir/instructions.h"
#include "qret/qret_export.h"

namespace qret::frontend::gate {
//--------------------------------------------------//
// Functions
//--------------------------------------------------//

/**
 * @brief Performs controlled shift of the lower `len` qubits to the higher positions.
 * @details See fig22 of https://arxiv.org/abs/1706.07884 for more information.
 * Diagram of bit order rotation:
 *
 * @verbatim
 * lower `len` part         ------\ /---------
 *                                 X
 * higher `n` - `len` part  ------/ \---------
 * @endverbatim
 *
 * Math of bit order rotation: new_tgt = tgt.Range(len, n - len) + tgt(0, len)
 *
 * We need dirty ancillae to run multi-controlled swap.
 *
 * @param len 0 < len < n
 * @param cs  size = c
 * @param tgt size = n
 * @param aux size = m, if n == 2 and c >= 2, we need aux: m = 1, otherwise m = 0
 */
QRET_EXPORT ir::CallInst* MultiControlledBitOrderRotation(
        std::size_t len,
        const Qubits& cs,
        const Qubits& tgt,
        const DirtyAncillae& aux
);
/**
 * @brief Performs controlled reversal of the order of qubits.
 * @details See fig23 of https://arxiv.org/abs/1706.07884 for more information.
 *
 * We need dirty ancillae to run multi-controlled swap.
 *
 * @param cs  size = c
 * @param tgt size = n
 * @param aux size = m, if n = 2 and c >= 2, we need aux: m = 1; otherwise m = 0
 */
QRET_EXPORT ir::CallInst*
MultiControlledBitOrderReversal(const Qubits& cs, const Qubits& tgt, const DirtyAncillae& aux);

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

/**
 * @brief Implementation of MultiControlledBitOrderRotation.
 */
struct QRET_EXPORT MultiControlledBitOrderRotationGen : CircuitGenerator {
    std::size_t len;
    std::size_t c;
    std::size_t n;
    std::size_t m;

    static inline const char* Name = "MultiControlledBitOrderRotation";
    explicit MultiControlledBitOrderRotationGen(
            CircuitBuilder* builder,
            std::size_t len,
            std::size_t c,
            std::size_t n,
            std::size_t m
    )
        : CircuitGenerator(builder)
        , len{len}
        , c{c}
        , n{n}
        , m{m} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{},{})", GetName(), len, c, n, m);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("cs", Type::Qubit, c, Attribute::Operate);
        arg.Add("tgt", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, m, Attribute::DirtyAncilla);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of MultiControlledBitOrderReversal.
 */
struct QRET_EXPORT MultiControlledBitOrderReversalGen : CircuitGenerator {
    std::size_t c;
    std::size_t n;
    std::size_t m;

    static inline const char* Name = "MultiControlledBitOrderReversal";
    explicit MultiControlledBitOrderReversalGen(
            CircuitBuilder* builder,
            std::size_t c,
            std::size_t n,
            std::size_t m
    )
        : CircuitGenerator(builder)
        , c{c}
        , n{n}
        , m{m} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{},{})", GetName(), c, n, m);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("cs", Type::Qubit, c, Attribute::Operate);
        arg.Add("tgt", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, m, Attribute::DirtyAncilla);
    }
    Circuit* Generate() const override;
};
}  // namespace qret::frontend::gate

#endif  // QRET_GATE_UTIL_BITWISE_H
