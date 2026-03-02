/**
 * @file qret/algorithm/arithmetic/boolean.h
 * @brief Boolean quantum circuits.
 */

#ifndef QRET_GATE_ARITHMETIC_BOOLEAN_H
#define QRET_GATE_ARITHMETIC_BOOLEAN_H

#include "qret/frontend/argument.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/ir/instructions.h"
#include "qret/qret_export.h"

namespace qret::frontend::gate {
//--------------------------------------------------//
// Functions
//--------------------------------------------------//

/**
 * @brief Performs temporal logical AND.
 * @details Calculate t = c0 & c1. t must be clean at first.
 * See fig3 of https://arxiv.org/abs/1709.06648 for more information.
 *
 * Different implementation from the current one: fig4 of https://arxiv.org/abs/1805.03662
 */
QRET_EXPORT ir::CallInst* TemporalAnd(const Qubit& t, const Qubit& c0, const Qubit& c1);
/**
 * @brief Performs measurement based uncomputation of temporal logical AND.
 * @details Calculate t = c0 & c1, by measuring a qubit `t`.
 * See fig3 of https://arxiv.org/abs/1709.06648 for more information.
 */
QRET_EXPORT ir::CallInst* UncomputeTemporalAnd(const Qubit& t, const Qubit& c0, const Qubit& c1);
/**
 * @brief Performs a toffoli-like operation, while adding a relative phase.
 * @details Calculate t ^= (c0 & c1), while adding a relative phase.
 * If you use IRT3 to uncompute RT3, the relative phase disappears.
 * RT3 and IRT3 is called "paired toffoli".
 * See fig3 of https://arxiv.org/abs/2101.04764 for more information.
 */
QRET_EXPORT ir::CallInst* RT3(const Qubit& t, const Qubit& c0, const Qubit& c1);
/**
 * @brief Performs a toffoli-like operation, while adding a relative phase.
 * @details Calculate t ^= (c0 & c1), while adding a relative phase.
 * If you use RT3 to uncompute IRT3, the relative phase disappears.
 * RT3 and IRT3 is called "paired toffoli".
 * See fig3 of https://arxiv.org/abs/2101.04764 for more information.
 */
QRET_EXPORT ir::CallInst* IRT3(const Qubit& t, const Qubit& c0, const Qubit& c1);
/**
 * @brief Performs a toffoli-like operation, while adding a relative phase.
 * @details Calculate t ^= (c0 & c1), while adding a relative phase.
 * If you use RT4 to uncompute IRT4, the relative phase disappears.
 * RT4 and IRT4 is called "paired toffoli".
 * See fig4 of https://arxiv.org/abs/2101.04764 for more information.
 */
QRET_EXPORT ir::CallInst* RT4(const Qubit& t, const Qubit& c0, const Qubit& c1);
/**
 * @brief Performs a toffoli-like operation, while adding a relative phase.
 * @details Calculate t ^= (c0 & c1), while adding a relative phase.
 * If you use RT4 to uncompute IRT4, the relative phase disappears.
 * RT4 and IRT4 is called "paired toffoli".
 * See fig4 of https://arxiv.org/abs/2101.04764 for more information.
 */
QRET_EXPORT ir::CallInst* IRT4(const Qubit& t, const Qubit& c0, const Qubit& c1);
/**
 * @brief Performs a in-place majorization.
 * @details Calculates the following transformation:
 * |xyz> -> |MAJ(x,y,z)>|x+y>|x+z>
 * See fig 3 of https://arxiv.org/abs/quant-ph/0410184 for more information.
 */
QRET_EXPORT ir::CallInst* MAJ(const Qubit& x, const Qubit& y, const Qubit& z);
/**
 * @brief Performs a in-place unmajorization.
 * @details Calculates the following transformation:
 * |MAJ(x,y,z)>|x+y>|x+z> -> |xyu> where \f$u = x \oplus y \oplus z \f$.
 * See fig 3 of https://arxiv.org/abs/quant-ph/0410184 for more information.
 */
QRET_EXPORT ir::CallInst* UMA(const Qubit& x, const Qubit& y, const Qubit& z);

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

/**
 * @brief Implementation of TemporalAnd.
 */
struct QRET_EXPORT TemporalAndGen : CircuitGenerator {
    static inline const char* Name = "TemporalAnd";
    explicit TemporalAndGen(CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return Name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("t", Type::Qubit, 1, Attribute::Operate);
        arg.Add("c0", Type::Qubit, 1, Attribute::Operate);
        arg.Add("c1", Type::Qubit, 1, Attribute::Operate);
    }
    Circuit* Generate() const override;
    Circuit* GenerateAdjoint() const override;
};
/**
 * @brief Implementation of UncomputeTemporalAnd.
 */
struct QRET_EXPORT UncomputeTemporalAndGen : CircuitGenerator {
    static inline const char* Name = "UncomputeTemporalAnd";
    explicit UncomputeTemporalAndGen(CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return Name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("t", Type::Qubit, 1, Attribute::Operate);
        arg.Add("c0", Type::Qubit, 1, Attribute::Operate);
        arg.Add("c1", Type::Qubit, 1, Attribute::Operate);
    }
    Circuit* Generate() const override;
    Circuit* GenerateAdjoint() const override;
};
/**
 * @brief Implementation of RT3.
 */
struct QRET_EXPORT RT3Gen : CircuitGenerator {
    static inline const char* Name = "RT3";
    explicit RT3Gen(CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return Name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("t", Type::Qubit, 1, Attribute::Operate);
        arg.Add("c0", Type::Qubit, 1, Attribute::Operate);
        arg.Add("c1", Type::Qubit, 1, Attribute::Operate);
    }
    Circuit* Generate() const override;
    Circuit* GenerateAdjoint() const override;
};
/**
 * @brief Implementation of IRT3.
 */
struct QRET_EXPORT IRT3Gen : CircuitGenerator {
    static inline const char* Name = "IRT3";
    explicit IRT3Gen(CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return Name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("t", Type::Qubit, 1, Attribute::Operate);
        arg.Add("c0", Type::Qubit, 1, Attribute::Operate);
        arg.Add("c1", Type::Qubit, 1, Attribute::Operate);
    }
    Circuit* Generate() const override;
    Circuit* GenerateAdjoint() const override;
};
/**
 * @brief Implementation of RT4.
 */
struct QRET_EXPORT RT4Gen : CircuitGenerator {
    static inline const char* Name = "RT4";
    explicit RT4Gen(CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return Name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("t", Type::Qubit, 1, Attribute::Operate);
        arg.Add("c0", Type::Qubit, 1, Attribute::Operate);
        arg.Add("c1", Type::Qubit, 1, Attribute::Operate);
    }
    Circuit* Generate() const override;
    Circuit* GenerateAdjoint() const override;
};
/**
 * @brief Implementation of IRT4.
 */
struct QRET_EXPORT IRT4Gen : CircuitGenerator {
    static inline const char* Name = "IRT4";
    explicit IRT4Gen(CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return Name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("t", Type::Qubit, 1, Attribute::Operate);
        arg.Add("c0", Type::Qubit, 1, Attribute::Operate);
        arg.Add("c1", Type::Qubit, 1, Attribute::Operate);
    }
    Circuit* Generate() const override;
    Circuit* GenerateAdjoint() const override;
};
/**
 * @brief Implementation of MAJ.
 */
struct QRET_EXPORT MAJGen : CircuitGenerator {
    static inline const char* Name = "MAJ";
    explicit MAJGen(CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return Name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("x", Type::Qubit, 1, Attribute::Operate);
        arg.Add("y", Type::Qubit, 1, Attribute::Operate);
        arg.Add("z", Type::Qubit, 1, Attribute::Operate);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of UMA.
 */
struct QRET_EXPORT UMAGen : CircuitGenerator {
    static inline const char* Name = "UMA";
    explicit UMAGen(CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return Name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("x", Type::Qubit, 1, Attribute::Operate);
        arg.Add("y", Type::Qubit, 1, Attribute::Operate);
        arg.Add("z", Type::Qubit, 1, Attribute::Operate);
    }
    Circuit* Generate() const override;
};
}  // namespace qret::frontend::gate

#endif  // QRET_GATE_ARITHMETIC_BOOLEAN_H
