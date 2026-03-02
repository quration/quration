/**
 * @file qret/algorithm/arithmetic/modular.h
 * @brief Modular integer quantum circuits.
 */

#ifndef QRET_GATE_ARITHMETIC_MODULAR_H
#define QRET_GATE_ARITHMETIC_MODULAR_H

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

// Modular negation

/**
 * @brief Performs modular negation.
 * @details Calculates tgt = (mod - tgt) % mod
 * See fig13 of https://arxiv.org/abs/1706.07884 for more information.
 *
 * Algorithm:
 * 1. Decrement `tgt`
 * 2. Pivot flip tgt under mod - 1
 * 3. Increment `tgt`
 *
 * @param mod
 * @param tgt size = n
 * @param aux size = 2
 */
QRET_EXPORT ir::CallInst* ModNeg(const BigInt& mod, const Qubits& tgt, const DirtyAncillae& aux);
/**
 * @brief Performs controlled modular negation.
 * @details Calculates tgt = (mod - tgt) % mod controlled by `cs`
 * See fig13 of https://arxiv.org/abs/1706.07884 for more information.
 *
 * @param mod
 * @param cs  size = c
 * @param tgt size = n
 * @param aux size = 2
 */
QRET_EXPORT ir::CallInst* MultiControlledModNeg(
        const BigInt& mod,
        const Qubits& cs,
        const Qubits& tgt,
        const DirtyAncillae& aux
);

// Modular addition

/**
 * @brief Performs modular addition.
 * @details Calculate dst = (dst + src) % mod
 * See fig11 of https://arxiv.org/abs/1706.07884 for more information.
 *
 * @param mod
 * @param dst size = n >= 2
 * @param src size = n
 * @param aux size = 2
 */
QRET_EXPORT ir::CallInst*
ModAdd(const BigInt& mod, const Qubits& dst, const Qubits& src, const DirtyAncillae& aux);
/**
 * @brief Performs controlled modular addition.
 * @details Calculate dst = (dst + src) % mod controlled by `cs`
 * See fig11 of https://arxiv.org/abs/1706.07884 for more information.
 *
 * @param mod
 * @param cs  size = c
 * @param dst size = n >= 2
 * @param src size = n
 * @param aux size = 2 - c (if c >= 2, size of aux is zero)
 */
QRET_EXPORT ir::CallInst* MultiControlledModAdd(
        const BigInt& mod,
        const Qubits& cs,
        const Qubits& dst,
        const Qubits& src,
        const DirtyAncillae& aux
);
/**
 * @brief Performs modular immediate addition.
 * @details Calculate tgt = (tgt + imm) % mod
 * See fig9 of https://arxiv.org/abs/1706.07884 for more information.
 *
 * @param mod
 * @param imm
 * @param tgt size = n >= 2
 * @param aux size = 2
 */
QRET_EXPORT ir::CallInst*
ModAddImm(const BigInt& mod, const BigInt& imm, const Qubits& tgt, const DirtyAncillae& aux);
/**
 * @brief Performs controlled modular immediate addition.
 * @details Calculate tgt = (tgt + imm) % mod controlled by `cs`
 * See fig9 of https://arxiv.org/abs/1706.07884 for more information.
 *
 * @param mod
 * @param imm
 * @param cs  size = c
 * @param tgt size = n >= 2
 * @param aux size = 2
 */
QRET_EXPORT ir::CallInst* MultiControlledModAddImm(
        const BigInt& mod,
        const BigInt& imm,
        const Qubits& cs,
        const Qubits& tgt,
        const DirtyAncillae& aux
);

// Modular subtraction

/**
 * @brief Performs modular subtraction (adjoint of ModAdd).
 * @details Calculate dst = (dst - src) % mod
 *
 * @param mod
 * @param dst size = n >= 2
 * @param src size = n
 * @param aux size = 2
 */
QRET_EXPORT ir::CallInst*
ModSub(const BigInt& mod, const Qubits& dst, const Qubits& src, const DirtyAncillae& aux);
/**
 * @brief Performs controlled modular subtraction (adjoint of MultiControlledModAdd).
 * @details Calculate dst = (dst - src) % mod controlled by `cs`
 *
 * @param mod
 * @param cs  size = c
 * @param dst size = n >= 2
 * @param src size = n
 * @param aux size = 2 - c (if c >= 2, size of aux is zero)
 */
QRET_EXPORT ir::CallInst* MultiControlledModSub(
        const BigInt& mod,
        const Qubits& cs,
        const Qubits& dst,
        const Qubits& src,
        const DirtyAncillae& aux
);
/**
 * @brief Performs modular immediate subtraction (adjoint of ModAddImm).
 * @details Calculate tgt = (tgt - imm) % mod
 *
 * @param mod
 * @param imm
 * @param tgt size = n >= 2
 * @param aux size = 2
 */
QRET_EXPORT ir::CallInst*
ModSubImm(const BigInt& mod, const BigInt& imm, const Qubits& tgt, const DirtyAncillae& aux);
/**
 * @brief Performs controlled modular immediate subtraction (adjoint of MultiControlledModAddImm).
 * @details Calculate tgt = (tgt - imm) % mod controlled by `cs`
 *
 * @param mod
 * @param imm
 * @param cs  size = c
 * @param tgt size = n >= 2
 * @param aux size = 2
 */
QRET_EXPORT ir::CallInst* MultiControlledModSubImm(
        const BigInt& mod,
        const BigInt& imm,
        const Qubits& cs,
        const Qubits& tgt,
        const DirtyAncillae& aux
);

// Modular doubling

/**
 * @brief Performs modular doubling.
 * @details Calculates tgt = (2 * tgt) % mod
 * See fig7 of https://arxiv.org/abs/1706.07884 for more information.
 *
 * @param mod odd number
 * @param tgt size = n >= 3
 * @param aux size = 1
 */
QRET_EXPORT ir::CallInst*
ModDoubling(const BigInt& mod, const Qubits& tgt, const DirtyAncilla& aux);
/**
 * @brief Performs controlled modular doubling.
 * @details Calculates tgt = (2 * tgt) % mod controlled by `cs`
 * See fig7 of https://arxiv.org/abs/1706.07884 for more information.
 *
 * @param mod odd number
 * @param cs  size = c
 * @param tgt size = n >= 3
 * @param aux size = 1
 */
QRET_EXPORT ir::CallInst* MultiControlledModDoubling(
        const BigInt& mod,
        const Qubits& cs,
        const Qubits& tgt,
        const DirtyAncilla& aux
);

// Modular bi-multiplication

/**
 * @brief Performs immediate bi multiplication.
 * @details Calculates lhs = (lhs * imm) % mod and rhs = (rhs * (multiplicative inverse of `imm`
 * modulo `mod`)) % mod at the same time.
 * See fig5 of https://arxiv.org/abs/1706.07884 for more
 * information.
 *
 * @param mod odd number
 * @param imm imm must have multiplicative inverse modulo `mod`
 * @param lhs size = n
 * @param rhs size = n
 */
QRET_EXPORT ir::CallInst*
ModBiMulImm(const BigInt& mod, const BigInt& imm, const Qubits& lhs, const Qubits& rhs);
/**
 * @brief Performs controlled immediate bi multiplication.
 * @details Calculates lhs = (lhs * imm) % mod and rhs = (rhs * (multiplicative inverse of `imm`
 * modulo `mod`)) % mod at the same time controlled by `cs` .
 * See fig5 of https://arxiv.org/abs/1706.07884 for more
 * information.
 *
 * @param mod odd number
 * @param imm imm must have multiplicative inverse modulo `mod`
 * @param cs  size = c
 * @param lhs size = n
 * @param rhs size = n
 */
QRET_EXPORT ir::CallInst* MultiControlledModBiMulImm(
        const BigInt& mod,
        const BigInt& imm,
        const Qubits& cs,
        const Qubits& lhs,
        const Qubits& rhs
);

// Modular scaled addition

/**
 * @brief Performs modular scaled addition.
 * @details Calculates dst = (dst + scale * src) % mod
 * See fig6 of https://arxiv.org/abs/1706.07884 for more information.
 *
 * @param mod   odd number
 * @param scale scale < mod, coprime to mod
 * @param dst   size = n >= 3
 * @param src   size = n
 */
QRET_EXPORT ir::CallInst*
ModScaledAdd(const BigInt& mod, const BigInt& scale, const Qubits& dst, const Qubits& src);
/**
 * @brief Performs modular scaled addition optimized with windowing.
 * @details Calculates dst = (dst + scale * src) % mod
 * See `plus_equal_product_mod` of https://arxiv.org/abs/1905.07682 for more information.
 *
 * @param window_size < n
 * @param mod   odd number
 * @param scale scale < mod, coprime to mod
 * @param dst   size=n
 * @param src   size=n
 * @param table size=n
 * @param aux   size=max(n-1,2)
 */
QRET_EXPORT ir::CallInst* ModScaledAddW(
        std::size_t window_size,
        const BigInt& mod,
        const BigInt& scale,
        const Qubits& dst,
        const Qubits& src,
        const CleanAncillae& table,
        const CleanAncillae& aux
);
/**
 * @brief Performs controlled modular scaled addition.
 * @details Calculates dst = (dst + scale * src) % mod controlled by `cs`
 * See fig6 of https://arxiv.org/abs/1706.07884 for more information.
 *
 * @param mod   odd number
 * @param scale scale < mod, coprime to mod
 * @param cs    size = c
 * @param dst   size = n >= 3
 * @param src   size = n
 */
QRET_EXPORT ir::CallInst* MultiControlledModScaledAdd(
        const BigInt& mod,
        const BigInt& scale,
        const Qubits& cs,
        const Qubits& dst,
        const Qubits& src
);

// Modular exponentiation

/**
 * @brief Performs modular exponentiation optimized with windowing.
 * @details Calculates tgt = tgt * base^exp % mod
 * See `times_equal_exp_mod` of https://arxiv.org/abs/1905.07682 for more information.
 *
 * @param mul_window_size < n
 * @param exp_window_size < m
 * @param mod   odd number
 * @param base  base must have multiplicative inverse modulo `mod`
 * @param tgt   size = n
 * @param exp   size = m
 * @param table size = n
 * @param tmp   size = n
 * @param aux   size = mul_window_size + exp_window_size - 1 (>=2)
 */
QRET_EXPORT ir::CallInst* ModExpW(
        std::size_t mul_window_size,
        std::size_t exp_window_size,
        const BigInt& mod,
        const BigInt& base,
        const Qubits& tgt,
        const Qubits& exp,
        const CleanAncillae& table,
        const CleanAncillae& tmp,
        const CleanAncillae& aux
);

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

/**
 * @brief Implementation of ModNeg and MultiControlledModNeg.
 */
struct QRET_EXPORT MultiControlledModNegGen : CircuitGenerator {
    BigInt mod;
    std::size_t c;
    std::size_t n;

    static inline const char* Name = "MultiControlledModNeg";
    explicit MultiControlledModNegGen(
            CircuitBuilder* builder,
            const BigInt& mod,
            std::size_t c,
            std::size_t n
    )
        : CircuitGenerator(builder)
        , mod{mod}
        , c{c}
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{},{})", GetName(), mod, c, n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("cs", Type::Qubit, c, Attribute::Operate);
        arg.Add("tgt", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 2, Attribute::Operate);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of ModAdd and ModSub.
 */
struct QRET_EXPORT ModAddGen : CircuitGenerator {
    BigInt mod;
    std::size_t n;

    static inline const char* Name = "ModAdd";
    explicit ModAddGen(CircuitBuilder* builder, const BigInt& mod, std::size_t n)
        : CircuitGenerator(builder)
        , mod{mod}
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{})", GetName(), mod, n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("dst", Type::Qubit, n, Attribute::Operate);
        arg.Add("src", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 2, Attribute::Operate);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of MultiControlledModAdd and MultiControlledModSub.
 */
struct QRET_EXPORT MultiControlledModAddGen : CircuitGenerator {
    BigInt mod;
    std::size_t c;
    std::size_t n;
    std::size_t m;  // size of aux, m = max(2 - c, 0)

    static inline const char* Name = "MultiControlledModAdd";
    explicit MultiControlledModAddGen(
            CircuitBuilder* builder,
            const BigInt& mod,
            std::size_t c,
            std::size_t n
    )
        : CircuitGenerator(builder)
        , mod{mod}
        , c{c}
        , n{n}
        , m{c >= 2 ? 0 : 2 - c} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{},{})", GetName(), mod, c, n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("cs", Type::Qubit, c, Attribute::Operate);
        arg.Add("dst", Type::Qubit, n, Attribute::Operate);
        arg.Add("src", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, m, Attribute::Operate);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of ModAddImm and ModSubImm.
 */
struct QRET_EXPORT ModAddImmGen : CircuitGenerator {
    BigInt mod;
    BigInt imm;
    std::size_t n;

    static inline const char* Name = "ModAddImm";
    explicit ModAddImmGen(
            CircuitBuilder* builder,
            const BigInt& mod,
            const BigInt& imm,
            std::size_t n
    )
        : CircuitGenerator(builder)
        , mod{mod}
        , imm{imm}
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{},{})", GetName(), mod, imm, n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("tgt", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 2, Attribute::Operate);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of MultiControlledModAddImm and MultiControlledModSubImm.
 */
struct QRET_EXPORT MultiControlledModAddImmGen : CircuitGenerator {
    BigInt mod;
    BigInt imm;
    std::size_t c;
    std::size_t n;

    static inline const char* Name = "MultiControlledModAddImm";
    explicit MultiControlledModAddImmGen(
            CircuitBuilder* builder,
            const BigInt& mod,
            const BigInt& imm,
            std::size_t c,
            std::size_t n
    )
        : CircuitGenerator(builder)
        , mod{mod}
        , imm{imm}
        , c{c}
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{},{},{})", GetName(), mod, imm, c, n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("cs", Type::Qubit, c, Attribute::Operate);
        arg.Add("tgt", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 2, Attribute::Operate);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of ModDoubling and MultiControlledModDoubling.
 */
struct QRET_EXPORT MultiControlledModDoublingGen : CircuitGenerator {
    BigInt mod;
    std::size_t c;
    std::size_t n;

    static inline const char* Name = "MultiControlledModDoubling";
    explicit MultiControlledModDoublingGen(
            CircuitBuilder* builder,
            const BigInt& mod,
            std::size_t c,
            std::size_t n
    )
        : CircuitGenerator(builder)
        , mod{mod}
        , c{c}
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{},{})", GetName(), mod, c, n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("cs", Type::Qubit, c, Attribute::Operate);
        arg.Add("tgt", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 1, Attribute::Operate);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of ModBiMulImm and MultiControlledModBiMulImm.
 */
struct QRET_EXPORT MultiControlledModBiMulImmGen : CircuitGenerator {
    BigInt mod;
    BigInt imm;
    std::size_t c;
    std::size_t n;

    static inline const char* Name = "MultiControlledModBiMulImm";
    explicit MultiControlledModBiMulImmGen(
            CircuitBuilder* builder,
            const BigInt& mod,
            const BigInt& imm,
            std::size_t c,
            std::size_t n
    )
        : CircuitGenerator(builder)
        , mod{mod}
        , imm{imm}
        , c{c}
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{},{},{})", GetName(), mod, imm, c, n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("cs", Type::Qubit, c, Attribute::Operate);
        arg.Add("lhs", Type::Qubit, n, Attribute::Operate);
        arg.Add("rhs", Type::Qubit, n, Attribute::Operate);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of ModScaledAdd and MultiControlledModScaledAdd.
 */
struct QRET_EXPORT MultiControlledModScaledAddGen : CircuitGenerator {
    BigInt mod;
    BigInt scale;
    std::size_t c;
    std::size_t n;

    static inline const char* Name = "MultiControlledModScaledAdd";
    explicit MultiControlledModScaledAddGen(
            CircuitBuilder* builder,
            const BigInt& mod,
            const BigInt& scale,
            std::size_t c,
            std::size_t n
    )
        : CircuitGenerator(builder)
        , mod{mod}
        , scale{scale}
        , c{c}
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{},{},{})", GetName(), mod, scale, c, n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("cs", Type::Qubit, c, Attribute::Operate);
        arg.Add("dst", Type::Qubit, n, Attribute::Operate);
        arg.Add("src", Type::Qubit, n, Attribute::Operate);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of ModScaledAdd.
 */
struct QRET_EXPORT ModScaledAddWGen : CircuitGenerator {
    std::size_t window_size;
    BigInt mod;
    BigInt scale;
    std::size_t n;

    static inline const char* Name = "ModScaledAddW";
    explicit ModScaledAddWGen(
            CircuitBuilder* builder,
            std::size_t window_size,
            const BigInt& mod,
            const BigInt& scale,
            std::size_t n
    )
        : CircuitGenerator(builder)
        , window_size{window_size}
        , mod{mod}
        , scale{scale}
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{},{},{})", GetName(), window_size, mod, scale, n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("dst", Type::Qubit, n, Attribute::Operate);
        arg.Add("src", Type::Qubit, n, Attribute::Operate);
        arg.Add("table", Type::Qubit, n, Attribute::CleanAncilla);
        arg.Add("aux", Type::Qubit, std::max(std::size_t{2}, n - 1), Attribute::CleanAncilla);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of ModExpW.
 */
struct QRET_EXPORT ModExpWGen : CircuitGenerator {
    std::size_t mul_window_size;
    std::size_t exp_window_size;
    BigInt mod;
    BigInt base;
    std::size_t n;  // size of tgt
    std::size_t m;  // size of exp

    static inline const char* Name = "ModExpW";
    explicit ModExpWGen(
            CircuitBuilder* builder,
            std::size_t mul_window_size,
            std::size_t exp_window_size,
            const BigInt& mod,
            const BigInt& base,
            std::size_t n,
            std::size_t m
    )
        : CircuitGenerator(builder)
        , mul_window_size{mul_window_size}
        , exp_window_size{exp_window_size}
        , mod{mod}
        , base{base}
        , n{n}
        , m{m} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format(
                "{}({},{},{},{},{},{})",
                GetName(),
                mul_window_size,
                exp_window_size,
                mod,
                base,
                n,
                m
        );
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("tgt", Type::Qubit, n, Attribute::Operate);
        arg.Add("exp", Type::Qubit, m, Attribute::Operate);
        arg.Add("table", Type::Qubit, n, Attribute::CleanAncilla);
        arg.Add("tmp", Type::Qubit, n, Attribute::CleanAncilla);
        arg.Add("aux", Type::Qubit, mul_window_size + exp_window_size - 1, Attribute::CleanAncilla);
    }
    Circuit* Generate() const override;
};
}  // namespace qret::frontend::gate

#endif  // QRET_GATE_ARITHMETIC_MODULAR_H
