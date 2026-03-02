/**
 * @file qret/algorithm/arithmetic/integer.h
 * @brief Integer quantum circuits.
 */

#ifndef QRET_GATE_ARITHMETIC_INTEGER_H
#define QRET_GATE_ARITHMETIC_INTEGER_H

#include <fmt/core.h>

#include <stdexcept>

#include "qret/base/type.h"
#include "qret/frontend/argument.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/ir/instructions.h"
#include "qret/qret_export.h"

namespace qret::frontend::gate {
//--------------------------------------------------//
// Functions
//--------------------------------------------------//

// Increment

/**
 * @brief Performs increment.
 * See fig19 and fig20 of https://arxiv.org/abs/1706.07884 for more information.
 *
 * @param tgt size = n
 * @param aux size = m (If n >= 4, then m >= 1)
 */
QRET_EXPORT ir::CallInst* Inc(const Qubits& tgt, const DirtyAncillae& aux);
/**
 * @brief Performs controlled increment.
 * See fig20 of https://arxiv.org/abs/1706.07884 for more information.
 *
 * @param cs  size = c
 * @param tgt size = n
 * @param aux size = m >= 1
 */
QRET_EXPORT ir::CallInst*
MultiControlledInc(const Qubits& cs, const Qubits& tgt, const DirtyAncillae& aux);

// Addition

/**
 * @brief Performs addition without using auxiliary qubits.
 * @details Calculate dst += src
 * See fig15 of https://arxiv.org/abs/1706.07884 for more information.
 *
 * @param dst size = n
 * @param src size = n
 */
QRET_EXPORT ir::CallInst* Add(const Qubits& dst, const Qubits& src);
/**
 * @brief Performs addition using Brent-Kung's Carry-Lookahead Adder (CLA).
 *
 * @param lhs size = n
 * @param rhs size = n
 * @param dst size = n
 * @param aux size = 4n
 */
QRET_EXPORT ir::CallInst*
AddBrentKung(const Qubits& lhs, const Qubits& rhs, const Qubits& dst, const CleanAncillae& aux);
/**
 * @brief Performs addition using Cuccaro's Ripple-Carry Adder (RCA).
 * @details Calculate dst += src
 * See fig4 of https://arxiv.org/abs/quant-ph/0410184 for more information.
 *
 * @param dst size = n
 * @param src size = n
 * @param aux size = 1
 */
QRET_EXPORT ir::CallInst* AddCuccaro(const Qubits& dst, const Qubits& src, const CleanAncilla& aux);
/**
 * @brief Performs addition using Craig's RCA.
 * @details Calculate dst += src
 * See fig1 of https://arxiv.org/abs/1709.06648 for more information.
 *
 * @param dst size = n
 * @param src size = n
 * @param aux size = n-1
 */
QRET_EXPORT ir::CallInst* AddCraig(const Qubits& dst, const Qubits& src, const CleanAncillae& aux);
/**
 * @brief Performs controlled addition using Craig's RCA.
 * @details Calculate dst += src if `c` is |1>.
 * See fig4 of https://arxiv.org/abs/1709.06648 for more information.
 *
 * @param c   size = 1
 * @param dst size = n
 * @param src size = n
 * @param aux size = n
 */
QRET_EXPORT ir::CallInst*
ControlledAddCraig(const Qubit& c, const Qubits& dst, const Qubits& src, const CleanAncillae& aux);
/**
 * @brief Performs immediate value addition.
 * @details Calculate tgt += imm
 * See fig16 of https://arxiv.org/abs/1706.07884 for more information.
 *
 * @param imm immediate value, smaller than 2^n
 * @param tgt size = n
 * @param aux size = 1
 */
QRET_EXPORT ir::CallInst* AddImm(const BigInt& imm, const Qubits& tgt, const DirtyAncilla& aux);
/**
 * @brief Performs controlled immediate value addition.
 * @details Calculate tgt += imm controlled by cs
 * See fig16 and fig18 of https://arxiv.org/abs/1706.07884 for more information.
 *
 * @param imm immediate value, smaller than 2^n
 * @param cs  size = c
 * @param tgt size = n
 * @param aux size = 1
 */
QRET_EXPORT ir::CallInst* MultiControlledAddImm(
        const BigInt& imm,
        const Qubits& cs,
        const Qubits& tgt,
        const DirtyAncilla& aux
);
/**
 * @brief Performs general addition without using auxiliary qubits.
 * @details Calculate dst += src
 * See fig17 of https://arxiv.org/abs/1706.07884 for more information.
 *
 * @param dst size = n (n >= m)
 * @param src size = m (m >= 2)
 */
QRET_EXPORT ir::CallInst* AddGeneral(const Qubits& dst, const Qubits& src);
/**
 * @brief Performs controlled general addition.
 * @details Calculate dst += src controlled by cs
 * See fig18 of https://arxiv.org/abs/1706.07884 for more information.
 *
 * @param cs  size = c
 * @param dst size = n (n >= m)
 * @param src size = m (m >= 2)
 * @param aux size = 1
 */
QRET_EXPORT ir::CallInst* MultiControlledAddGeneral(
        const Qubits& cs,
        const Qubits& dst,
        const Qubits& src,
        const DirtyAncilla& aux
);
/**
 * @brief Computes the final carry of immediate addition.
 * @details Calculate whether "tgt += imm" overflows. If it does, apply X to the `carry` qubit.
 * See fig2 of https://arxiv.org/abs/1611.07995 for more information.
 *
 * @param imm immediate value, smaller than 2^n
 * @param tgt   size = n
 * @param carry size = 1
 * @param aux   size = n
 */
QRET_EXPORT ir::CallInst* FinalCarryOfAddImm(
        const BigInt& imm,
        const Qubits& tgt,
        const Qubit& carry,
        const DirtyAncillae& aux
);
/**
 * @brief Performs approximate piecewise addition.
 * @details Calculate dst += src.
 * This circuit can reduce the asymptotic depth of addition to O(lg lg n)
 * (n: size of `dst` and `src`) by dividing qubits into lower and upper bits during addition.
 * The carry from lower to upper bits uses additional auxiliary qubits
 * which size is `m` and initialized to the |+> state.
 * Note that if carry occurs, the addition will fail with probability 1/2^m.
 * If all aux measurement results are |0>, the addition may have failed.
 * See fig2 and fig3 of https://arxiv.org/abs/1905.08488 for more information.
 *
 * @param p   size of lower bits, (0, n)
 * @param dst size = n
 * @param src size = n
 * @param aux size = m + 1 + max(n - p - m, m), 0 < m <= n-p
 * @param out size = m
 */
QRET_EXPORT ir::CallInst* AddPiecewise(
        size_t p,
        const Qubits& dst,
        const Qubits& src,
        const CleanAncillae& aux,
        const Registers& out
);

// Subtraction

/**
 * @brief Performs subtraction without using auxiliary qubits (adjoint of Add).
 * @details Calculate dst -= src
 *
 * @param dst size = n
 * @param src size = n
 */
QRET_EXPORT ir::CallInst* Sub(const Qubits& dst, const Qubits& src);
/**
 * @brief Performs subtraction using adjoint of Brent-Kung's CLA.
 *
 * @param lhs size = n
 * @param rhs size = n
 * @param dst size = n
 * @param aux size = 4n
 */
QRET_EXPORT ir::CallInst*
SubBrentKung(const Qubits& lhs, const Qubits& rhs, const Qubits& dst, const CleanAncillae& aux);
/**
 * @brief Performs subtraction using adjoint of Cuccaro's RCA.
 * @details Calculate dst -= src
 *
 * @param dst size = n
 * @param src size = n
 * @param aux size = 1
 */
QRET_EXPORT ir::CallInst* SubCuccaro(const Qubits& dst, const Qubits& src, const CleanAncilla& aux);
/**
 * @brief Performs subtraction using adjoint of Craig's RCA.
 * @details Calculate dst -= src
 *
 * @param dst size = n
 * @param src size = n
 * @param aux size = n-1
 */
QRET_EXPORT ir::CallInst* SubCraig(const Qubits& dst, const Qubits& src, const CleanAncillae& aux);
/**
 * @brief Performs controlled subtraction using adjoint of Craig's RCA.
 * @details Calculate dst -= src if `c` is |1>.
 *
 * @param c   size = 1
 * @param dst size = n
 * @param src size = n
 * @param aux size = n
 */
QRET_EXPORT ir::CallInst*
ControlledSubCraig(const Qubit& c, const Qubits& dst, const Qubits& src, const CleanAncillae& aux);
/**
 * @brief Performs immediate value subtraction (adjoint of AddImm).
 * @details Calculate tgt -= imm
 *
 * @param imm immediate value, smaller than 2^n
 * @param tgt size = n
 * @param aux size = 1
 */
QRET_EXPORT ir::CallInst* SubImm(const BigInt& imm, const Qubits& tgt, const DirtyAncilla& aux);
/**
 * @brief Performs controlled immediate value subtraction (adjoint of MultiControlledAddImm).
 * @details Calculate tgt -= imm controlled by cs
 *
 * @param imm immediate value, smaller than 2^n
 * @param cs  size = c
 * @param tgt size = n
 * @param aux size = 1
 */
QRET_EXPORT ir::CallInst* MultiControlledSubImm(
        const BigInt& imm,
        const Qubits& cs,
        const Qubits& tgt,
        const DirtyAncilla& aux
);
/**
 * @brief Performs general subtraction without using auxiliary qubits (adjoint of AddGeneral).
 * @details Calculate dst -= src
 *
 * @param dst size = n (n >= m)
 * @param src size = m (m >= 2)
 */
QRET_EXPORT ir::CallInst* SubGeneral(const Qubits& dst, const Qubits& src);
/**
 * @brief Performs controlled general subtraction (adjoint of MultiControlledSubGeneral).
 * @details Calculate dst -= src controlled by cs
 *
 * @param cs  size = c
 * @param dst size = n (n >= m)
 * @param src size = m (m >= 2)
 * @param aux 1
 */
QRET_EXPORT ir::CallInst* MultiControlledSubGeneral(
        const Qubits& cs,
        const Qubits& dst,
        const Qubits& src,
        const DirtyAncilla& aux
);

// Multiplication

/**
 * @brief Performs multiplication optimized with windowing.
 * @details Calculates tgt *= mul
 * See `times_equal` of https://arxiv.org/abs/1905.07682 for more information.
 *
 * @param window_size < n
 * @param mul   odd number
 * @param tgt   size = n
 * @param table size = n
 * @param aux   size = n - 1
 */
QRET_EXPORT ir::CallInst* MulW(
        std::size_t window_size,
        const BigInt& mul,
        const Qubits& tgt,
        const CleanAncillae& table,
        const CleanAncillae& aux
);

// Scaled addition

/**
 * @brief Performs scaled addition.
 * @details Calculates dst += scale * src
 * See fig6 of https://arxiv.org/abs/1706.07884 for more information.
 *
 * @param scale
 * @param dst size = n
 * @param src size = n
 * @param aux size = n - 1
 */
QRET_EXPORT ir::CallInst*
ScaledAdd(const BigInt& scale, const Qubits& dst, const Qubits& src, const CleanAncillae& aux);
/**
 * @brief Performs scaled addition optimized with windowing.
 * @details Calculates dst += scale * src
 * See `plus_equal_product` of https://arxiv.org/abs/1905.07682 for more information.
 *
 * @param window_size < n
 * @param scale
 * @param dst   size = n
 * @param src   size = n
 * @param table size = n
 * @param aux   size = n - 1
 */
QRET_EXPORT ir::CallInst* ScaledAddW(
        std::size_t window_size,
        const BigInt& scale,
        const Qubits& dst,
        const Qubits& src,
        const CleanAncillae& table,
        const CleanAncillae& aux
);

// Comparison

/**
 * @brief Performs comparison `equal to` (==).
 *
 * @param lhs size = n
 * @param rhs size = n
 * @param cmp size = 1
 * @param aux size = n-1
 */
QRET_EXPORT ir::CallInst*
EqualTo(const Qubits& lhs, const Qubits& rhs, const Qubit& cmp, const CleanAncillae& aux);
/**
 * @brief Performs comparison `equal to` (==) with immediate value.
 *
 * @param imm immediate value, smaller than 2^n
 * @param tgt size = n
 * @param cmp size = 1
 * @param aux size = n-1
 */
QRET_EXPORT ir::CallInst*
EqualToImm(const BigInt& imm, const Qubits& tgt, const Qubit& cmp, const CleanAncillae& aux);
/**
 * @brief Performs comparison `less than` (<).
 * @details If lhs < rhs, flips `cmp`.
 *
 * Algorithm:
 * 1. Computes lhs -= rhs by adding an extra most significant bit
 *     * Let `cmp` be the most significant bit of `lhs`
 * 2. Computes lhs += rhs to uncompute the previous subtraction
 *
 * @param lhs size = n
 * @param rhs size = n
 * @param cmp size = 1
 * @param aux size = 2
 */
QRET_EXPORT ir::CallInst*
LessThan(const Qubits& lhs, const Qubits& rhs, const Qubit& cmp, const CleanAncillae& aux);
/**
 * @brief Performs comparison `less than or equal to` (<=).
 * @details If lhs <= rhs, flips `cmp`.
 *
 * Algorithm:
 * 1. Computes lhs -= (rhs+1) by adding an extra most significant bit
 *     * Let `cmp` be the most significant bit of `lhs`
 * 2. Computes lhs += rhs and lhs += 1 to uncompute the previous subtraction
 *     * addition and increment
 *
 * @param lhs size = n
 * @param rhs size = n
 * @param cmp size = 1
 * @param aux size = 2
 */
QRET_EXPORT ir::CallInst*
LessThanOrEqualTo(const Qubits& lhs, const Qubits& rhs, const Qubit& cmp, const CleanAncillae& aux);
/**
 * @brief Performs comparison `less than` (<) with immediate value.
 * @details If tgt < imm, flips `cmp`
 * See fig14 of https://arxiv.org/abs/1706.07884 for more information.
 *
 * @param imm immediate value
 * @param tgt size = n
 * @param cmp size = 1
 * @param aux size = 1
 */
QRET_EXPORT ir::CallInst*
LessThanImm(const BigInt& imm, const Qubits& tgt, const Qubit& cmp, const DirtyAncilla& aux);
/**
 * @brief Performs comparison `less than` (<) with immediate value controlled by qubits.
 * @details If tgt < imm, flips `cmp` controlled by qubits `cs`.
 * See fig14 of https://arxiv.org/abs/1706.07884 for more information.
 *
 * @param imm immediate value
 * @param cs  size = c
 * @param tgt size = n
 * @param cmp size = 1
 * @param aux size = 1
 */
QRET_EXPORT ir::CallInst* MultiControlledLessThanImm(
        const BigInt& imm,
        const Qubits& cs,
        const Qubits& tgt,
        const Qubit& cmp,
        const DirtyAncilla& aux
);

// Flip operation

/**
 * @brief Flips both [0 .. src-1] and [src .. max] sections at the same time.
 * @details See section2.5 (fig8) of https://arxiv.org/abs/1706.07884 for more information.
 *
 * * max = 2^n - 1
 *
 * @param dst size = n >= m
 * @param src size = m
 */
QRET_EXPORT ir::CallInst* BiFlip(const Qubits& dst, const Qubits& src);
/**
 * @brief Performs controlled flip of both [0 .. src-1] and [src .. max] sections at the same time.
 * @details See section2.5 (fig8) of https://arxiv.org/abs/1706.07884 for more information.
 *
 * * max = 2^n - 1
 *
 * @param cs  size = c
 * @param dst size = n >= m
 * @param src size = m
 * @param aux size = 1
 */
QRET_EXPORT ir::CallInst* MultiControlledBiFlip(
        const Qubits& cs,
        const Qubits& dst,
        const Qubits& src,
        const DirtyAncilla& aux
);
/**
 * @brief Flips both [0 .. imm-1] and [imm .. max] sections at the same time.
 * @details See section2.5 (fig9) of https://arxiv.org/abs/1706.07884 for more information.
 *
 * @param imm immediate value
 * @param tgt size = n
 * @param aux size = 1
 */
QRET_EXPORT ir::CallInst* BiFlipImm(const BigInt& imm, const Qubits& tgt, const DirtyAncilla& aux);
/**
 * @brief Performs controlled flip of both [0 .. imm-1] and [imm .. max] sections at the same time.
 * @details See section2.5 (fig9) of https://arxiv.org/abs/1706.07884 for more information.
 *
 * @param imm immediate value
 * @param cs  size = c
 * @param tgt size = n
 * @param aux size = 1
 */
QRET_EXPORT ir::CallInst* MultiControlledBiFlipImm(
        const BigInt& imm,
        const Qubits& cs,
        const Qubits& tgt,
        const DirtyAncilla& aux
);
/**
 * @brief Flips [0 .. src-1].
 * @details See fig8 of https://arxiv.org/abs/1706.07884 for more information.
 *
 * Uses "toggle-controlled" technique to implement pivot flip.
 *
 * @param dst size = n >= m
 * @param src size = m
 * @param aux size = 2
 */
QRET_EXPORT ir::CallInst* PivotFlip(const Qubits& dst, const Qubits& src, const DirtyAncillae& aux);
/**
 * @brief Performs a controlled flip of [0 .. src-1].
 * @details See fig8 of https://arxiv.org/abs/1706.07884 for more information.
 *
 * Uses "toggle-controlled" technique to implement pivot flip.
 *
 * @param cs  size = c
 * @param dst size = n >= m
 * @param src size = m
 * @param aux size = 2
 */
QRET_EXPORT ir::CallInst* MultiControlledPivotFlip(
        const Qubits& cs,
        const Qubits& dst,
        const Qubits& src,
        const DirtyAncillae& aux
);
/**
 * @brief Flips [0 .. imm-1].
 * @details See fig9 of https://arxiv.org/abs/1706.07884 for more information.
 *
 * @param imm immediate value
 * @param tgt size = n
 * @param aux size = 2
 */
QRET_EXPORT ir::CallInst*
PivotFlipImm(const BigInt& imm, const Qubits& tgt, const DirtyAncillae& aux);
/**
 * @brief Performs a controlled flip of [0 .. imm-1].
 * @details See fig9 of https://arxiv.org/abs/1706.07884 for more information.
 *
 * @param imm immediate value
 * @param cs  size = c
 * @param tgt size = n
 * @param aux size = 2
 */
QRET_EXPORT ir::CallInst* MultiControlledPivotFlipImm(
        const BigInt& imm,
        const Qubits& cs,
        const Qubits& tgt,
        const DirtyAncillae& aux
);

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

/**
 * @brief Implementation of Inc.
 */
struct QRET_EXPORT IncGen : CircuitGenerator {
    std::size_t n;
    std::size_t m;

    static inline const char* Name = "Inc";
    explicit IncGen(CircuitBuilder* builder, std::size_t n, std::size_t m)
        : CircuitGenerator(builder)
        , n{n}
        , m{m} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{})", GetName(), n, m);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("tgt", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, m, Attribute::DirtyAncilla);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of MultiControlledInc.
 */
struct QRET_EXPORT MultiControlledIncGen : CircuitGenerator {
    std::size_t c;
    std::size_t n;
    std::size_t m;

    static inline const char* Name = "MultiControlledInc";
    explicit MultiControlledIncGen(
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
/**
 * @brief Implementation of Add and Sub.
 */
struct QRET_EXPORT AddGen : CircuitGenerator {
    std::size_t n;

    static inline const char* Name = "Add";
    explicit AddGen(CircuitBuilder* builder, std::size_t n)
        : CircuitGenerator(builder)
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({})", GetName(), n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("dst", Type::Qubit, n, Attribute::Operate);
        arg.Add("src", Type::Qubit, n, Attribute::Operate);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of AddBrentKung and SubBrentKung.
 * @details see https://arxiv.org/abs/2405.02523 and section 2 of
 * https://ipsj.ixsq.nii.ac.jp/ej/?action=repository_uri&item_id=190923&file_id=1&file_no=1 for more
 * information.
 */
struct QRET_EXPORT AddBrentKungGen : CircuitGenerator {
    std::size_t n;

    static inline const char* Name = "AddBrentKung";
    explicit AddBrentKungGen(CircuitBuilder* builder, std::size_t n)
        : CircuitGenerator(builder)
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({})", GetName(), n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("lhs", Type::Qubit, n, Attribute::Operate);
        arg.Add("rhs", Type::Qubit, n, Attribute::Operate);
        arg.Add("dst", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 4 * n, Attribute::CleanAncilla);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of AddCuccaro and SubCuccaro.
 */
struct QRET_EXPORT AddCuccaroGen : CircuitGenerator {
    std::size_t n;

    static inline const char* Name = "AddCuccaro";
    explicit AddCuccaroGen(CircuitBuilder* builder, std::size_t n)
        : CircuitGenerator(builder)
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({})", GetName(), n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("dst", Type::Qubit, n, Attribute::Operate);
        arg.Add("src", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 1, Attribute::CleanAncilla);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of AddCraig and SubCraig.
 */
struct QRET_EXPORT AddCraigGen : CircuitGenerator {
    std::size_t n;

    static inline const char* Name = "AddCraig";
    explicit AddCraigGen(CircuitBuilder* builder, std::size_t n)
        : CircuitGenerator(builder)
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({})", GetName(), n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("dst", Type::Qubit, n, Attribute::Operate);
        arg.Add("src", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, n - 1, Attribute::CleanAncilla);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of ControlledAddCraig and ControlledSubCraig.
 */
struct QRET_EXPORT ControlledAddCraigGen : CircuitGenerator {
    std::size_t n;

    static inline const char* Name = "ControlledAddCraig";
    explicit ControlledAddCraigGen(CircuitBuilder* builder, std::size_t n)
        : CircuitGenerator(builder)
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({})", GetName(), n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("c", Type::Qubit, 1, Attribute::Operate);
        arg.Add("dst", Type::Qubit, n, Attribute::Operate);
        arg.Add("src", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, n, Attribute::CleanAncilla);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of AddImm and SubImm.
 */
struct QRET_EXPORT AddImmGen : CircuitGenerator {
    BigInt imm;
    std::size_t n;

    static inline const char* Name = "AddImm";
    explicit AddImmGen(CircuitBuilder* builder, const BigInt& imm, std::size_t n)
        : CircuitGenerator(builder)
        , imm{imm}
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{})", GetName(), imm, n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("tgt", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 1, Attribute::DirtyAncilla);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of MultiControlledAddImm and MultiControlledSubImm.
 */
struct QRET_EXPORT MultiControlledAddImmGen : CircuitGenerator {
    BigInt imm;
    std::size_t c;
    std::size_t n;

    static inline const char* Name = "MultiControlledAddImm";
    explicit MultiControlledAddImmGen(
            CircuitBuilder* builder,
            const BigInt& imm,
            std::size_t c,
            std::size_t n
    )
        : CircuitGenerator(builder)
        , imm{imm}
        , c{c}
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{},{})", GetName(), imm, c, n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("cs", Type::Qubit, c, Attribute::Operate);
        arg.Add("tgt", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 1, Attribute::DirtyAncilla);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of AddGeneral and SubGeneral.
 */
struct QRET_EXPORT AddGeneralGen : CircuitGenerator {
    std::size_t n;
    std::size_t m;

    static inline const char* Name = "AddGeneral";
    explicit AddGeneralGen(CircuitBuilder* builder, std::size_t n, std::size_t m)
        : CircuitGenerator(builder)
        , n{n}
        , m{m} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{})", GetName(), n, m);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("dst", Type::Qubit, n, Attribute::Operate);
        arg.Add("src", Type::Qubit, m, Attribute::Operate);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of MultiControlledAddGeneral and MultiControlledSubGeneral.
 */
struct QRET_EXPORT MultiControlledAddGeneralGen : CircuitGenerator {
    std::size_t c;
    std::size_t n;
    std::size_t m;

    static inline const char* Name = "MultiControlledAddGeneral";
    explicit MultiControlledAddGeneralGen(
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
        return fmt::format("{}({},{})", GetName(), c, n, m);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("cs", Type::Qubit, c, Attribute::Operate);
        arg.Add("dst", Type::Qubit, n, Attribute::Operate);
        arg.Add("src", Type::Qubit, m, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 1, Attribute::DirtyAncilla);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of FinalCarryOfAddImm.
 */
struct QRET_EXPORT FinalCarryOfAddImmGen : CircuitGenerator {
    BigInt imm;
    std::size_t n;

    static inline const char* Name = "FinalCarryOfAddImm";
    explicit FinalCarryOfAddImmGen(CircuitBuilder* builder, const BigInt& imm, std::size_t n)
        : CircuitGenerator(builder)
        , imm{imm}
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{})", GetName(), imm, n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("tgt", Type::Qubit, n, Attribute::Operate);
        arg.Add("carry", Type::Qubit, 1, Attribute::Operate);
        arg.Add("aux", Type::Qubit, n, Attribute::DirtyAncilla);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of AddPiecewise.
 */
struct QRET_EXPORT AddPiecewiseGen : CircuitGenerator {
    std::size_t n;
    std::size_t m;  // (0, n - p]
    std::size_t p;  // (0, n)

    static inline const char* Name = "AddPiecewise";
    explicit AddPiecewiseGen(CircuitBuilder* builder, std::size_t n, std::size_t m, std::size_t p)
        : CircuitGenerator(builder)
        , n{n}
        , m{m}
        , p{p} {
        if (p == 0 || p >= n) {
            throw std::runtime_error("parameter `p` must be in the range of (0, n)");
        };
        if (m == 0 || m > n - p) {
            throw std::runtime_error("parameter `m` must be in the range of (0, n-p]");
        };
    }
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{},{})", GetName(), n, m, p);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("dst", Type::Qubit, n, Attribute::Operate);
        arg.Add("src", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, m + 1 + std::max(n - p - m, m), Attribute::CleanAncilla);
        arg.Add("out", Type::Register, m, Attribute::Output);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of MulW.
 */
struct QRET_EXPORT MulWGen : CircuitGenerator {
    std::size_t window_size;
    BigInt mul;
    std::size_t n;

    static inline const char* Name = "MulW";
    explicit MulWGen(
            CircuitBuilder* builder,
            std::size_t window_size,
            const BigInt& mul,
            std::size_t n
    )
        : CircuitGenerator(builder)
        , window_size{window_size}
        , mul{mul}
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{},{})", GetName(), window_size, mul, n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("tgt", Type::Qubit, n, Attribute::Operate);
        arg.Add("table", Type::Qubit, n, Attribute::CleanAncilla);
        arg.Add("aux", Type::Qubit, n - 1, Attribute::CleanAncilla);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of ScaledAdd.
 */
struct QRET_EXPORT ScaledAddGen : CircuitGenerator {
    BigInt scale;
    std::size_t n;

    static inline const char* Name = "ScaledAdd";
    explicit ScaledAddGen(CircuitBuilder* builder, const BigInt& scale, std::size_t n)
        : CircuitGenerator(builder)
        , scale{scale}
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{})", GetName(), scale, n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("dst", Type::Qubit, n, Attribute::Operate);
        arg.Add("src", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, n - 1, Attribute::CleanAncilla);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of ScaledAddW.
 */
struct QRET_EXPORT ScaledAddWGen : CircuitGenerator {
    std::size_t window_size;
    BigInt scale;
    std::size_t n;

    static inline const char* Name = "ScaledAddW";
    explicit ScaledAddWGen(
            CircuitBuilder* builder,
            std::size_t window_size,
            const BigInt& scale,
            std::size_t n
    )
        : CircuitGenerator(builder)
        , window_size{window_size}
        , scale{scale}
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{},{})", GetName(), window_size, scale, n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("dst", Type::Qubit, n, Attribute::Operate);
        arg.Add("src", Type::Qubit, n, Attribute::Operate);
        arg.Add("table", Type::Qubit, n, Attribute::CleanAncilla);
        arg.Add("aux", Type::Qubit, n - 1, Attribute::CleanAncilla);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of EqualTo.
 */
struct QRET_EXPORT EqualToGen : CircuitGenerator {
    std::size_t n;

    static inline const char* Name = "EqualTo";
    explicit EqualToGen(CircuitBuilder* builder, std::size_t n)
        : CircuitGenerator(builder)
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({})", GetName(), n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("lhs", Type::Qubit, n, Attribute::Operate);
        arg.Add("rhs", Type::Qubit, n, Attribute::Operate);
        arg.Add("cmp", Type::Qubit, 1, Attribute::Operate);
        arg.Add("aux", Type::Qubit, n - 1, Attribute::CleanAncilla);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of EqualToImm.
 */
struct QRET_EXPORT EqualToImmGen : CircuitGenerator {
    BigInt imm;
    std::size_t n;

    static inline const char* Name = "EqualToImm";
    explicit EqualToImmGen(CircuitBuilder* builder, const BigInt& imm, std::size_t n)
        : CircuitGenerator(builder)
        , imm{imm}
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{})", GetName(), imm, n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("tgt", Type::Qubit, n, Attribute::Operate);
        arg.Add("cmp", Type::Qubit, 1, Attribute::Operate);
        arg.Add("aux", Type::Qubit, n - 1, Attribute::CleanAncilla);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of LessThan.
 */
struct QRET_EXPORT LessThanGen : CircuitGenerator {
    std::size_t n;

    static inline const char* Name = "LessThan";
    explicit LessThanGen(CircuitBuilder* builder, std::size_t n)
        : CircuitGenerator(builder)
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({})", GetName(), n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("lhs", Type::Qubit, n, Attribute::Operate);
        arg.Add("rhs", Type::Qubit, n, Attribute::Operate);
        arg.Add("cmp", Type::Qubit, 1, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 2, Attribute::CleanAncilla);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of LessThanOrEqualTo.
 */
struct QRET_EXPORT LessThanOrEqualToGen : CircuitGenerator {
    std::size_t n;

    static inline const char* Name = "LessThanOrEqualTo";
    explicit LessThanOrEqualToGen(CircuitBuilder* builder, std::size_t n)
        : CircuitGenerator(builder)
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({})", GetName(), n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("lhs", Type::Qubit, n, Attribute::Operate);
        arg.Add("rhs", Type::Qubit, n, Attribute::Operate);
        arg.Add("cmp", Type::Qubit, 1, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 2, Attribute::CleanAncilla);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of LessThanImm.
 */
struct QRET_EXPORT LessThanImmGen : CircuitGenerator {
    BigInt imm;
    std::size_t n;

    static inline const char* Name = "LessThanImm";
    explicit LessThanImmGen(CircuitBuilder* builder, const BigInt& imm, std::size_t n)
        : CircuitGenerator(builder)
        , imm{imm}
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{})", GetName(), imm, n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("tgt", Type::Qubit, n, Attribute::Operate);
        arg.Add("cmp", Type::Qubit, 1, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 1, Attribute::DirtyAncilla);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of MultiControlledLessThanImm.
 */
struct QRET_EXPORT MultiControlledLessThanImmGen : CircuitGenerator {
    BigInt imm;
    std::size_t c;
    std::size_t n;

    static inline const char* Name = "MultiControlledLessThanImm";
    explicit MultiControlledLessThanImmGen(
            CircuitBuilder* builder,
            const BigInt& imm,
            std::size_t c,
            std::size_t n
    )
        : CircuitGenerator(builder)
        , imm{imm}
        , c{c}
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{},{})", GetName(), imm, c, n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("cs", Type::Qubit, c, Attribute::Operate);
        arg.Add("tgt", Type::Qubit, n, Attribute::Operate);
        arg.Add("cmp", Type::Qubit, 1, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 1, Attribute::DirtyAncilla);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of BiFlip.
 */
struct QRET_EXPORT BiFlipGen : CircuitGenerator {
    std::size_t n;
    std::size_t m;

    static inline const char* Name = "BiFlip";
    explicit BiFlipGen(CircuitBuilder* builder, std::size_t n, std::size_t m)
        : CircuitGenerator(builder)
        , n{n}
        , m{m} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{})", GetName(), n, m);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("dst", Type::Qubit, n, Attribute::Operate);
        arg.Add("src", Type::Qubit, m, Attribute::Operate);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of MultiControlledBiFlip.
 */
struct QRET_EXPORT MultiControlledBiFlipGen : CircuitGenerator {
    std::size_t c;
    std::size_t n;
    std::size_t m;

    static inline const char* Name = "MultiControlledBiFlip";
    explicit MultiControlledBiFlipGen(
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
        arg.Add("dst", Type::Qubit, n, Attribute::Operate);
        arg.Add("src", Type::Qubit, m, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 1, Attribute::DirtyAncilla);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of BiFlipImm.
 */
struct QRET_EXPORT BiFlipImmGen : CircuitGenerator {
    BigInt imm;
    std::size_t n;

    static inline const char* Name = "BiFlipImm";
    explicit BiFlipImmGen(CircuitBuilder* builder, const BigInt& imm, std::size_t n)
        : CircuitGenerator(builder)
        , imm{imm}
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({}{})", GetName(), imm, n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("tgt", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 1, Attribute::DirtyAncilla);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of MultiControlledBiFlipImm.
 */
struct QRET_EXPORT MultiControlledBiFlipImmGen : CircuitGenerator {
    BigInt imm;
    std::size_t c;
    std::size_t n;

    static inline const char* Name = "MultiControlledBiFlipImm";
    explicit MultiControlledBiFlipImmGen(
            CircuitBuilder* builder,
            const BigInt& imm,
            std::size_t c,
            std::size_t n
    )
        : CircuitGenerator(builder)
        , imm{imm}
        , c{c}
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{},{})", GetName(), imm, c, n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("cs", Type::Qubit, c, Attribute::Operate);
        arg.Add("tgt", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 1, Attribute::DirtyAncilla);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of PivotFlip.
 */
struct QRET_EXPORT PivotFlipGen : CircuitGenerator {
    std::size_t n;
    std::size_t m;

    static inline const char* Name = "PivotFlip";
    explicit PivotFlipGen(CircuitBuilder* builder, std::size_t n, std::size_t m)
        : CircuitGenerator(builder)
        , n{n}
        , m{m} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{})", GetName(), n, m);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("dst", Type::Qubit, n, Attribute::Operate);
        arg.Add("src", Type::Qubit, m, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 2, Attribute::DirtyAncilla);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of MultiControlledPivotFlip.
 */
struct QRET_EXPORT MultiControlledPivotFlipGen : CircuitGenerator {
    std::size_t c;
    std::size_t n;
    std::size_t m;

    static inline const char* Name = "MultiControlledPivotFlip";
    explicit MultiControlledPivotFlipGen(
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
        arg.Add("dst", Type::Qubit, n, Attribute::Operate);
        arg.Add("src", Type::Qubit, m, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 2, Attribute::DirtyAncilla);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of PivotFlipImm.
 */
struct QRET_EXPORT PivotFlipImmGen : CircuitGenerator {
    BigInt imm;
    std::size_t n;

    static inline const char* Name = "PivotFlipImm";
    explicit PivotFlipImmGen(CircuitBuilder* builder, const BigInt& imm, std::size_t n)
        : CircuitGenerator(builder)
        , imm{imm}
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{})", GetName(), imm, n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("tgt", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 2, Attribute::DirtyAncilla);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of MultiControlledPivotFlipImm.
 */
struct QRET_EXPORT MultiControlledPivotFlipImmGen : CircuitGenerator {
    BigInt imm;
    std::size_t c;
    std::size_t n;

    static inline const char* Name = "MultiControlledPivotFlipImm";
    explicit MultiControlledPivotFlipImmGen(
            CircuitBuilder* builder,
            const BigInt& imm,
            std::size_t c,
            std::size_t n
    )
        : CircuitGenerator(builder)
        , imm{imm}
        , c{c}
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{},{})", GetName(), imm, c, n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("cs", Type::Qubit, c, Attribute::Operate);
        arg.Add("tgt", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 2, Attribute::DirtyAncilla);
    }
    Circuit* Generate() const override;
};
}  // namespace qret::frontend::gate

#endif  // QRET_GATE_ARITHMETIC_INTEGER_H
