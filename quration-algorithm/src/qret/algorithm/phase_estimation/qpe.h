/**
 * @file qret/algorithm/phase_estimation/qpe.h
 * @brief QPE.
 */

#ifndef QRET_ALGORITHM_PHASE_ESTIMATION_QPE_H
#define QRET_ALGORITHM_PHASE_ESTIMATION_QPE_H

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "qret/base/type.h"
#include "qret/frontend/argument.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/ir/instructions.h"
#include "qret/math/integer.h"
#include "qret/math/pauli.h"
#include "qret/qret_export.h"

namespace qret::frontend::gate {
//--------------------------------------------------//
// Functions
//--------------------------------------------------//

/**
 * @brief Performs PREPARE.
 * @details See fig11 of https://arxiv.org/abs/1805.03662 for more information.
 *
 * Correspondence between fig11 and params:
 * * \f$\log L = n\f$
 * * \f$\mu    = m\f$
 *
 * @param alias_sampling  pair of (alternated index, switch weight), keep weight = 2^m - switch
 * weight, size = the number of pauli terms < =2^n
 * @param index           size = n
 * @param alternate_index size = n
 * @param random          size = m, m is sub_bit_precision
 * @param switch_weight   size = m
 * @param cmp             size = 1
 * @param aux             size = n+3
 */
QRET_EXPORT ir::CallInst* PREPARE(
        std::span<const std::pair<std::uint32_t, BigInt>> alias_sampling,
        const Qubits& index,
        const Qubits& alternate_index,
        const Qubits& random,
        const Qubits& switch_weight,
        const Qubit& cmp,
        const CleanAncillae& aux
);

using PauliArray = std::vector<std::vector<math::Pauli>>;

/**
 * @brief Performs SELECT.
 * @details See fig5 and fig7 of https://arxiv.org/abs/1805.03662 for more information.
 *
 * @param paulis            list of Pauli string (size = L)
 * @param address           size = Bit size of L - 1
 * @param tgt               size = length of a Pauli string
 * @param aux               size = address size - 1
 */
QRET_EXPORT ir::CallInst* SELECT(
        std::span<const PauliArray> paulis,
        const Qubits& address,
        const Qubits& tgt,
        const CleanAncillae& aux
);

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

/**
 * @brief Implementation of PREPARE.
 */
struct QRET_EXPORT PREPAREGen : CircuitGenerator {
    std::span<const std::pair<std::uint32_t, BigInt>> alias_sampling;
    std::size_t n;
    std::size_t m;

    static inline const char* Name = "PREPARE";
    explicit PREPAREGen(
            CircuitBuilder* builder,
            std::span<const std::pair<std::uint32_t, BigInt>> alias_sampling,
            std::size_t n,
            std::size_t m
    )
        : CircuitGenerator(builder)
        , alias_sampling{alias_sampling}
        , n{n}
        , m{m} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("index", Type::Qubit, n, Attribute::Operate);
        arg.Add("alternate_index", Type::Qubit, n, Attribute::Operate);
        arg.Add("random", Type::Qubit, m, Attribute::Operate);
        arg.Add("switch_weight", Type::Qubit, m, Attribute::Operate);
        arg.Add("cmp", Type::Qubit, 1, Attribute::Operate);
        arg.Add("aux", Type::Qubit, n + 3, Attribute::CleanAncilla);
    }
    Circuit* Generate() const override;
};

/**
 * @brief Implementation of SELECT.
 */
struct QRET_EXPORT SELECTGen : CircuitGenerator {
    PauliArray pauli_strings;
    std::size_t system_size;
    std::size_t aux_size;
    std::size_t address_size;

    static inline const char* Name = "SELECT";
    explicit SELECTGen(CircuitBuilder* builder, const PauliArray& pauli_strings)
        : CircuitGenerator(builder)
        , pauli_strings{pauli_strings}
        , system_size(pauli_strings[0].size())
        , aux_size(math::BitSizeI(pauli_strings.size() - 1) - 1)
        , address_size(math::BitSizeI(pauli_strings.size() - 1)) {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("address", Type::Qubit, address_size, Attribute::Operate);
        arg.Add("system", Type::Qubit, system_size, Attribute::Operate);
        arg.Add("aux", Type::Qubit, aux_size, Attribute::CleanAncilla);
    }
    Circuit* Generate() const override;
};

/**
 * @brief Implementation of QPE.
 */
struct QRET_EXPORT PhaseEstimationGen : CircuitGenerator {
    // size of paulis and alias sampling is the number of terms in lcu-hamiltonian
    std::span<const PauliArray> paulis;
    std::span<const std::pair<std::uint32_t, BigInt>> alias_sampling;
    std::size_t hadamard_size;
    std::size_t system_size;
    std::size_t n;  // BitSizeI(paulis.size() - 1)
    std::size_t m;  // precision of PREPARE

    static inline const char* Name = "PhaseEstimation";
    explicit PhaseEstimationGen(
            CircuitBuilder* builder,
            std::span<const PauliArray> paulis,
            std::span<const std::pair<std::uint32_t, BigInt>> alias_sampling,
            std::size_t hadamard_size,
            std::size_t system_size,
            std::size_t n,
            std::size_t m
    )
        : CircuitGenerator(builder)
        , paulis{paulis}
        , alias_sampling{alias_sampling}
        , hadamard_size{hadamard_size}
        , system_size{system_size}
        , n{n}
        , m{m} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;  // Do not cache
    void SetArgument(Argument& arg) const override {
        arg.Add("system", Type::Qubit, system_size, Attribute::Operate);
        arg.Add("hadamard", Type::Qubit, hadamard_size, Attribute::Operate);
        arg.Add("index", Type::Qubit, n, Attribute::Operate);
        arg.Add("alternate_index", Type::Qubit, n, Attribute::Operate);
        arg.Add("random", Type::Qubit, m, Attribute::Operate);
        arg.Add("switch_weight", Type::Qubit, m, Attribute::Operate);
        arg.Add("cmp", Type::Qubit, 1, Attribute::Operate);
        arg.Add("aux", Type::Qubit, n + 3, Attribute::CleanAncilla);
        arg.Add("measure_system", Type::Register, system_size, Attribute::Output);
        arg.Add("measure_hadamard", Type::Register, hadamard_size, Attribute::Output);
        arg.Add("measure_index", Type::Register, n, Attribute::Output);
    }
    frontend::Circuit* Generate() const override;
};
}  // namespace qret::frontend::gate

#endif  // QRET_ALGORITHM_PHASE_ESTIMATION_QPE_H
