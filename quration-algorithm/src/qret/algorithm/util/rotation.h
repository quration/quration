/**
 * @file qret/algorithm/util/rotation.h
 */

#ifndef QRET_GATE_UTIL_ROTATION_H
#define QRET_GATE_UTIL_ROTATION_H

#include <span>

#include "qret/frontend/argument.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/ir/instructions.h"
#include "qret/ir/value.h"
#include "qret/qret_export.h"

namespace qret::frontend::gate {
//--------------------------------------------------//
// Functions
//--------------------------------------------------//

/**
 * @brief Performs R1 gate.
 * @details R1 gate is defined as follows:
 *
 * \f[
 *   R1(\theta) =
 *   \begin{bmatrix}
 *     1 & 0 \\
 *     0 & e^{i\theta}
 *   \end{bmatrix}
 * \f]
 *
 * The name "R1" is referencing Q#:
 * https://learn.microsoft.com/en-us/qsharp/api/qsharp/microsoft.quantum.intrinsic.r1
 *
 * @param q target
 * @param theta
 * @param precision
 */
QRET_EXPORT ir::CallInst* R1(const Qubit& q, double theta, double precision = 1e-10);
/**
 * @brief Performs controlled R1 gate.
 * @details See
 * https://quantumcomputing.stackexchange.com/questions/2143/how-can-a-controlledry-be-made-from-cnots-and-rotations
 * for more information.
 *
 * @param c control
 * @param q target
 * @param theta
 * @param precision
 */
QRET_EXPORT ir::CallInst*
ControlledR1(const Qubit& c, const Qubit& q, double theta, double precision = 1e-10);
/**
 * @brief Performs controlled RX gate.
 *
 * @param c control
 * @param q target
 * @param theta
 * @param precision
 */
QRET_EXPORT ir::CallInst*
ControlledRX(const Qubit& c, const Qubit& q, double theta, double precision = 1e-10);
/**
 * @brief Performs controlled RY gate.
 *
 * @param c control
 * @param q target
 * @param theta
 * @param precision
 */
QRET_EXPORT ir::CallInst*
ControlledRY(const Qubit& c, const Qubit& q, double theta, double precision = 1e-10);
/**
 * @brief Performs controlled RZ gate.
 *
 * @param c control
 * @param q target
 * @param theta
 * @param precision
 */
QRET_EXPORT ir::CallInst*
ControlledRZ(const Qubit& c, const Qubit& q, double theta, double precision = 1e-10);
/**
 * @brief Performs multiplexed rotation using random.
 * @details Implements eq(1) of https://arxiv.org/abs/2110.13439.
 *
 * @param lookup_par
 * @param del_lookup_par
 * @param sub_bit_precision
 * @param seed
 * @param theta
 * @param q       size = 1
 * @param address size = n
 * @param aux     size = 1
 *                       + sub_bit_precision
 *                       + max(n - 1 - lookup_par, n - 1 - del_lookup_par)
 *                       + max((2^loopup_par - 1)*sub_bit_precision, 2^del_lookup_par)
 */
QRET_EXPORT ir::CallInst* RandomizedMultiplexedRotation(
        std::uint64_t lookup_par,
        std::uint64_t del_lookup_par,
        std::uint64_t sub_bit_precision,
        std::uint64_t seed,
        std::span<const ir::FloatWithPrecision> theta,
        const Qubit& q,
        const Qubits& address,
        const CleanAncillae& aux
);

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

/**
 * @brief Implementation of R1.
 */
struct QRET_EXPORT R1Gen : CircuitGenerator {
    double theta;
    double precision;

    static inline const char* Name = "R1";
    R1Gen(CircuitBuilder* builder, double theta, double precision)
        : CircuitGenerator(builder)
        , theta{theta}
        , precision{precision} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;  // do not cache
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 1, Attribute::Operate);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of ControlledR1, ControlledRX, ControlledRY, ControlledRZ.
 */
struct QRET_EXPORT ControlledRGen : CircuitGenerator {
    enum class RType : std::uint8_t { R1, RX, RY, RZ };
    RType type;
    double theta;
    double precision;

    static inline const char* Name = "ControlledRotation";
    ControlledRGen(CircuitBuilder* builder, RType type, double theta, double precision)
        : CircuitGenerator(builder)
        , type{type}
        , theta{theta}
        , precision{precision} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;  // do not cache
    void SetArgument(Argument& arg) const override {
        arg.Add("c", Type::Qubit, 1, Attribute::Operate);
        arg.Add("q", Type::Qubit, 1, Attribute::Operate);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of RandomizedMultiplexedRotation.
 */
struct QRET_EXPORT RandomizedMultiplexedRotationGen : CircuitGenerator {
    std::span<const ir::FloatWithPrecision> theta;
    std::uint64_t lookup_par;
    std::uint64_t del_lookup_par;
    std::uint64_t sub_bit_precision;
    std::uint64_t seed;
    std::uint64_t n;  // size of address
    std::uint64_t aux_size;

    static inline const char* Name = "RandomizedMultiplexedRotation";
    RandomizedMultiplexedRotationGen(
            CircuitBuilder* builder,
            std::span<const ir::FloatWithPrecision> theta,
            std::uint64_t lookup_par,
            std::uint64_t del_lookup_par,
            std::uint64_t sub_bit_precision,
            std::uint64_t seed,
            std::uint64_t n
    )
        : CircuitGenerator(builder)
        , theta{theta}
        , lookup_par{lookup_par}
        , del_lookup_par{del_lookup_par}
        , sub_bit_precision{sub_bit_precision}
        , seed{seed}
        , n{n}
        , aux_size{
                  1 + sub_bit_precision + std::max(n - 1 - lookup_par, n - 1 - del_lookup_par)
                  + std::max(
                          ((std::uint64_t{1} << lookup_par) - 1) * sub_bit_precision,
                          std::uint64_t{1} << del_lookup_par
                  )
          } {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;  // do not cache
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 1, Attribute::Operate);
        arg.Add("address", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, aux_size, Attribute::CleanAncilla);
    }
    Circuit* Generate() const override;
};
}  // namespace qret::frontend::gate

#endif  // QRET_GATE_UTIL_ROTATION_H
