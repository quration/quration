/**
 * @file qret/algorithm/util/util.h
 */

#ifndef QRET_GATE_UTIL_UTIL_H
#define QRET_GATE_UTIL_UTIL_H

#include "qret/frontend/argument.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/ir/instructions.h"
#include "qret/qret_export.h"

namespace qret::frontend::gate {
//--------------------------------------------------//
// Functions
//--------------------------------------------------//

/**
 * @brief Performs controlled H gate using catalysis |T>.
 * @details See fig17 of https://arxiv.org/abs/2011.03494 for more information.
 *
 * @param c control
 * @param q target
 * @param catalysis |T>
 * @param aux clean ancillae
 */
QRET_EXPORT ir::CallInst*
ControlledH(const Qubit& c, const Qubit& q, const Qubit& catalysis, const CleanAncilla& aux);
/**
 * @brief Performs swap gate using three CNOT.
 *
 * @param lhs
 * @param rhs
 */
QRET_EXPORT ir::CallInst* Swap(const Qubit& lhs, const Qubit& rhs);
/**
 * @brief Performs controlled swap gate.
 * @details See
 * https://quantumcomputing.stackexchange.com/questions/33774/whats-a-good-cliffordt-circuit-for-a-controlled
 * controlled swap-gate for more information
 *
 * @param c control
 * @param lhs
 * @param rhs
 * @param aux
 */
QRET_EXPORT ir::CallInst*
ControlledSwap(const Qubit& c, const Qubit& lhs, const Qubit& rhs, const CleanAncilla& aux);

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

/**
 * @brief Implementation of ControlledH.
 */
struct QRET_EXPORT ControlledHGen : CircuitGenerator {
    static inline const char* Name = "ControlledH";
    explicit ControlledHGen(CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return Name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("c", Type::Qubit, 1, Attribute::Operate);
        arg.Add("q", Type::Qubit, 1, Attribute::Operate);
        arg.Add("catalysis", Type::Qubit, 1, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 1, Attribute::CleanAncilla);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of Swap.
 */
struct QRET_EXPORT SwapGen : CircuitGenerator {
    static inline const char* Name = "Swap";
    explicit SwapGen(CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return Name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("lhs", Type::Qubit, 1, Attribute::Operate);
        arg.Add("rhs", Type::Qubit, 1, Attribute::Operate);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of ControlledSwap.
 */
struct QRET_EXPORT ControlledSwapGen : CircuitGenerator {
    static inline const char* Name = "ControlledSwap";
    explicit ControlledSwapGen(CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return Name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("c", Type::Qubit, 1, Attribute::Operate);
        arg.Add("lhs", Type::Qubit, 1, Attribute::Operate);
        arg.Add("rhs", Type::Qubit, 1, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 1, Attribute::CleanAncilla);
    }
    Circuit* Generate() const override;
};
}  // namespace qret::frontend::gate

#endif  // QRET_GATE_UTIL_UTIL_H
