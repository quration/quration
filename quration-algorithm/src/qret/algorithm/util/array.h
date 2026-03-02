/**
 * @file qret/algorithm/util/array.h
 */

#ifndef QRET_GATE_UTIL_ARRAY_H
#define QRET_GATE_UTIL_ARRAY_H

#include <fmt/core.h>

#include "qret/base/type.h"
#include "qret/frontend/argument.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/ir/instructions.h"
#include "qret/qret_export.h"

namespace qret::frontend::gate {
//--------------------------------------------------//
// Functions
//--------------------------------------------------//

/**
 * @brief Applies Pauli-X to all qubits.
 *
 * @param tgt size = n
 */
QRET_EXPORT ir::CallInst* ApplyXToEach(const Qubits& tgt);
/**
 * @brief Applies Pauli-X to qubits if the corresponding register is 1.
 * @details Applies Pauli-X to tgt[i] if bools[i] == 1
 *
 * @param tgt   size = n
 * @param bools size = n
 */
QRET_EXPORT ir::CallInst* ApplyXToEachIf(const Qubits& tgt, const Registers& bools);
/**
 * @brief Applies CNOT to qubits if the corresponding bit is 1.
 * @details Applies CNOT to (c, tgt[i]) if bools[i] == 1
 *
 * @param c     size = 1
 * @param tgt   size = n
 * @param bools size = n
 */
QRET_EXPORT ir::CallInst*
ApplyCXToEachIf(const Qubit& c, const Qubits& tgt, const Registers& bools);

// immediate value

/**
 * @brief Applies Pauli-X to qubits if the corresponding bit is 1.
 * @details Applies Pauli-X to tgt[i] if (imm>>i) & 1 == 1
 *
 * @param imm condition
 * @param tgt size = n
 */
QRET_EXPORT ir::CallInst* ApplyXToEachIfImm(const BigInt& imm, const Qubits& tgt);
/**
 * @brief Applies CNOT to qubits if the corresponding bit is 1.
 * @details Applies CNOT to (c, tgt[i]) if (imm>>i) & 1 == 1
 *
 * @param imm condition
 * @param c   size = 1
 * @param tgt size = n
 */
QRET_EXPORT ir::CallInst* ApplyCXToEachIfImm(const BigInt& imm, const Qubit& c, const Qubits& tgt);

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

/**
 * @brief Implementation of ApplyXToEach.
 */
struct QRET_EXPORT ApplyXToEachGen : CircuitGenerator {
    std::uint64_t size;

    static inline const char* Name = "ApplyXToEach";
    ApplyXToEachGen(CircuitBuilder* builder, std::uint64_t size)
        : CircuitGenerator(builder)
        , size{size} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({})", GetName(), size);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("tgt", Type::Qubit, size, Attribute::Operate);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of ApplyXToEachIf.
 */
struct QRET_EXPORT ApplyXToEachIfGen : CircuitGenerator {
    std::uint64_t size;

    static inline const char* Name = "ApplyXToEachIf";
    ApplyXToEachIfGen(CircuitBuilder* builder, std::uint64_t size)
        : CircuitGenerator(builder)
        , size{size} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({})", GetName(), size);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("tgt", Type::Qubit, size, Attribute::Operate);
        arg.Add("bools", Type::Register, size, Attribute::Input);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of ApplyCXToEachIf.
 */
struct QRET_EXPORT ApplyCXToEachIfGen : CircuitGenerator {
    std::uint64_t size;

    static inline const char* Name = "ApplyCXToEachIf";
    ApplyCXToEachIfGen(CircuitBuilder* builder, std::uint64_t size)
        : CircuitGenerator(builder)
        , size{size} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({})", GetName(), size);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("c", Type::Qubit, 1, Attribute::Operate);
        arg.Add("tgt", Type::Qubit, size, Attribute::Operate);
        arg.Add("bools", Type::Register, size, Attribute::Input);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of ApplyXToEachIfImm.
 */
struct QRET_EXPORT ApplyXToEachIfImmGen : CircuitGenerator {
    BigInt imm;
    std::uint64_t size;

    static inline const char* Name = "ApplyXToEachIfImm";
    ApplyXToEachIfImmGen(CircuitBuilder* builder, const BigInt& imm, std::uint64_t size)
        : CircuitGenerator(builder)
        , imm{imm}
        , size{size} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{})", GetName(), imm, size);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("tgt", Type::Qubit, size, Attribute::Operate);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of ApplyCXToEachIfImm.
 */
struct QRET_EXPORT ApplyCXToEachIfImmGen : CircuitGenerator {
    BigInt imm;
    std::uint64_t size;

    static inline const char* Name = "ApplyCXToEachIfImm";
    ApplyCXToEachIfImmGen(CircuitBuilder* builder, const BigInt& imm, std::uint64_t size)
        : CircuitGenerator(builder)
        , imm{imm}
        , size{size} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{})", GetName(), imm, size);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("c", Type::Qubit, 1, Attribute::Operate);
        arg.Add("tgt", Type::Qubit, size, Attribute::Operate);
    }
    Circuit* Generate() const override;
};
}  // namespace qret::frontend::gate

#endif  // QRET_GATE_UTIL_ARRAY_H
