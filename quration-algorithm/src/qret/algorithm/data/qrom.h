/**
 * @file qret/algorithm/data/qrom.h
 * @brief QROM, QROAM, and parallel QROM.
 */

#ifndef QRET_GATE_DATA_QROM_H
#define QRET_GATE_DATA_QROM_H

#include <fmt/core.h>

#include <span>

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
 * @brief Begins unary iteration.
 * @details If the value of `address` is equal to `begin`, set `aux[0] = 1`.
 * See fig5 and fig7 of https://arxiv.org/abs/1805.03662 for more information.
 *
 * Unary iteration from |L> to |L + M>
 * 1. UnaryIterBegin(L, address, aux);
 * 2. UnaryIteStep(L, address, aux);
 * 3. UnaryIteStep(L + 1, address, aux);
 * 4. ... (repeat) ...
 * 5. UnaryIterStep(L + M - 1, address, aux);
 * 6. UnaryIterEnd(L + M, address, aux);
 *
 * @param begin   begin < 2^n
 * @param address size = n >= 2
 * @param aux     size = n-1, must be clean at first
 */
QRET_EXPORT ir::CallInst*
UnaryIterBegin(const BigInt& begin, const Qubits& address, const Qubits& aux);
/**
 * @brief Steps unary iteration.
 * @details If the value of `address` is equal to `step+1`, set `aux[0] = 1`.
 * Steps unary iteration from `step` to `step + 1`
 *
 * UnaryIterStep(step, address, aux) can be called after UnaryIterBegin(step, address, aux) or
 * UnaryIterStep(step-1, address, aux).
 *
 * @param step    step < 2^n - 1
 * @param address size = n >= 2
 * @param aux     size = n-1
 */
QRET_EXPORT ir::CallInst*
UnaryIterStep(const BigInt& step, const Qubits& address, const Qubits& aux);
/**
 * @brief Ends unary iteration.
 * @details Ends unary iteration at `end`.
 * This function UnaryIterEnd(step, address, aux) can be called after UnaryIterBegin(end, address,
 * aux) or UnaryIterStep(end-1, address, aux).
 *
 * @param end     end < 2^n - 1
 * @param address size = n >= 2
 * @param aux     size = n-1, must be clean at end
 */
QRET_EXPORT ir::CallInst* UnaryIterEnd(const BigInt& end, const Qubits& address, const Qubits& aux);
/**
 * @brief Performs controlled UnaryIterBegin.
 *
 * @param begin   begin < 2^n
 * @param cs      size = c
 * @param address size = n,
 * @param aux     size = n+c-1, must be clean at first
 */
QRET_EXPORT ir::CallInst* MultiControlledUnaryIterBegin(
        const BigInt& begin,
        const Qubits& cs,
        const Qubits& address,
        const Qubits& aux
);
/**
 * @brief Performs controlled UnaryIterStep.
 *
 * @param step    step < 2^n - 1
 * @param cs      size = c
 * @param address size = n >= 2
 * @param aux     size = n-1
 */
QRET_EXPORT ir::CallInst* MultiControlledUnaryIterStep(
        const BigInt& step,
        const Qubits& cs,
        const Qubits& address,
        const Qubits& aux
);
/**
 * @brief Performs controlled UnaryIterEnd.
 *
 * @param end     end < 2^n - 1
 * @param cs      size = c
 * @param address size = n >= 2
 * @param aux     size = n-1, must be clean at end
 */
QRET_EXPORT ir::CallInst* MultiControlledUnaryIterEnd(
        const BigInt& end,
        const Qubits& cs,
        const Qubits& address,
        const Qubits& aux
);
/**
 * @brief Performs Quantum Read-Only Memory (QROM).
 * @details Implements fig7 of https://arxiv.org/abs/1805.03662
 *
 * @param address size = n >= 1
 * @param value   size = m
 * @param aux     size = n - 1
 * @param dict    size = m * dict_size, dict_size must be smaller than 2^n
 */
QRET_EXPORT ir::CallInst*
QROM(const Qubits& address, const Qubits& value, const CleanAncillae& aux, const Registers& dict);
/**
 * @brief Performs controlled QROM.
 * @details Implements fig7 of https://arxiv.org/abs/1805.03662
 *
 * @param cs      size = c
 * @param address size = n >= 1
 * @param value   size = m
 * @param aux     size = n + c - 1
 * @param dict    size = m * dict_size, dict_size must be smaller than 2^n
 */
QRET_EXPORT ir::CallInst* MultiControlledQROM(
        const Qubits& cs,
        const Qubits& address,
        const Qubits& value,
        const CleanAncillae& aux,
        const Registers& dict
);
/**
 * @brief Performs QROM where the written value is immediate.
 * @details Implements fig7 of https://arxiv.org/abs/1805.03662
 *
 * @param address size = n >= 1
 * @param value   size = m
 * @param aux     size = n - 1
 * @param dict    size of span must be smaller than 2^n, bit width of each element is m
 */
QRET_EXPORT ir::CallInst* QROMImm(
        const Qubits& address,
        const Qubits& value,
        const CleanAncillae& aux,
        std::span<const BigInt> dict
);
/**
 * @brief Performs controlled QROM where the written value is immediate.
 * @details Implements fig7 of https://arxiv.org/abs/1805.03662
 *
 * @param cs      size = c
 * @param address size = n >= 1
 * @param value   size = m
 * @param aux     size = n + c - 1
 * @param dict    size of span must be smaller than 2^n, bit width of each element is m
 */
QRET_EXPORT ir::CallInst* MultiControlledQROMImm(
        const Qubits& cs,
        const Qubits& address,
        const Qubits& value,
        const CleanAncillae& aux,
        std::span<const BigInt> dict
);
/**
 * @brief Performs measurement-based uncomputation of QROM.
 * @details
 * See fig5 of https://arxiv.org/abs/1902.02134 for more information.
 *
 * @param address size = n >= 1
 * @param value   size = m
 * @param aux     size = n - 1
 * @param dict    size = m * dict_size, dict_size must be smaller than 2^n
 */
QRET_EXPORT ir::CallInst* UncomputeQROM(
        const Qubits& address,
        const Qubits& value,
        const CleanAncillae& aux,
        const Registers& dict
);
/**
 * @brief Performs measurement-based uncomputation of QROM where the written value is immediate.
 * @details
 * See fig5 of https://arxiv.org/abs/1902.02134 for more information.
 *
 * @param address size = n >= 1
 * @param value   size = m
 * @param aux     size = n - 1
 * @param dict    size of span must be smaller than 2^n, bit width of each element is m
 */
QRET_EXPORT ir::CallInst* UncomputeQROMImm(
        const Qubits& address,
        const Qubits& value,
        const CleanAncillae& aux,
        std::span<const BigInt> dict
);
/**
 * @brief Performs advanced QROM (QROAM) assisted by clean ancillae.
 * @details
 * See fig4 of https://arxiv.org/abs/1902.02134 for more information.
 *
 * @param num_par   num_par < n-1, the number of threads = 2^num_par
 * @param address   size = n
 * @param value     size = m
 * @param duplicate size = (2^num_par - 1) * m, must be clean at first
 * @param aux       size = n - num_par - 1
 * @param dict      size = m * dict_size, dict_size must be smaller than 2^n
 */
QRET_EXPORT ir::CallInst* QROAMClean(
        std::size_t num_par,
        const Qubits& address,
        const Qubits& value,
        const Qubits& duplicate,
        const CleanAncillae& aux,
        const Registers& dict
);
/**
 * @brief Performs measurement-based uncomputation of QROAM assisted by clean ancillae.
 * @details
 * See fig6 of https://arxiv.org/abs/1902.02134 for more information.
 *
 * @param num_par   num_par < n-1, the number of threads = 2^num_par
 * @param address   size = n
 * @param value     size = m
 * @param duplicate size = 2^num_par, must be clean at first
 * @param aux       size = n - num_par - 1
 * @param dict      size = m * dict_size, dict_size must be smaller than 2^n
 */
QRET_EXPORT ir::CallInst* UncomputeQROAMClean(
        std::size_t num_par,
        const Qubits& address,
        const Qubits& value,
        const CleanAncillae& duplicate,
        const CleanAncillae& aux,
        const Registers& dict
);
/**
 * @brief Performs QROAM assisted by dirty ancillae.
 * @details
 * See fig4 of https://arxiv.org/abs/1902.02134 for more information.
 *
 * @param num_par   num_par < n-1, the number of threads = 2^num_par
 * @param address   size = n
 * @param value     size = m
 * @param duplicate size = (2^num_par - 1) * m
 * @param aux       size = n - num_par - 1
 * @param dict      size = m * dict_size, dict_size must be smaller than 2^n
 */
QRET_EXPORT ir::CallInst* QROAMDirty(
        std::size_t num_par,
        const Qubits& address,
        const Qubits& value,
        const DirtyAncillae& duplicate,
        const CleanAncillae& aux,
        const Registers& dict
);
/**
 * @brief Performs measurement-based uncomputation of QROAM assisted by dirty ancillae.
 * @details
 * See fig7 of https://arxiv.org/abs/1902.02134 for more information.
 *
 * @param num_par   num_par < n-1, the number of threads = 2^num_par
 * @param address   size = n
 * @param value     size = m
 * @param duplicate size = 2^num_par - 1
 * @param src       size = 1
 * @param aux       size = n - num_par - 1
 * @param dict      size = m * dict_size, dict_size must be smaller than 2^n
 */
QRET_EXPORT ir::CallInst* UncomputeQROAMDirty(
        std::size_t num_par,
        const Qubits& address,
        const Qubits& value,
        const DirtyAncillae& duplicate,
        const CleanAncilla& src,
        const CleanAncillae& aux,
        const Registers& dict
);
/**
 * @brief Parallel implementation of QROM.
 * @details Implements http://id.nii.ac.jp/1001/00218669/
 *
 * The difference between QROAM and QROMParallel:
 * * QROAM duplicates values
 * * QROMParallel duplicates addresses
 *
 * @param num_par num_par < n-1, the number of threads = 2^num_par
 * @param address           size = n >= 1
 * @param value             size = m
 * @param duplicate_address size = (2^num_par-1) * n
 * @param duplicate_aux     size = 2^num_par * (n-1)
 * @param dict              size = m * dict_size, dict_size must be smaller than 2^n
 */
QRET_EXPORT ir::CallInst* QROMParallel(
        std::size_t num_par,
        const Qubits& address,
        const Qubits& value,
        const CleanAncillae& duplicate_address,
        const CleanAncillae& duplicate_aux,
        const Registers& dict
);

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

/**
 * @brief Implementation of UnaryIterBegin and ControlledUnaryIterBegin.
 */
struct QRET_EXPORT UnaryIterBeginGen : CircuitGenerator {
    BigInt begin;
    std::size_t n;

    static inline const char* Name = "UnaryIterBegin";
    explicit UnaryIterBeginGen(CircuitBuilder* builder, const BigInt& begin, std::size_t n)
        : CircuitGenerator(builder)
        , begin{begin}
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{})", GetName(), begin, n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("address", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, n - 1, Attribute::Operate);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of UnaryIterStep and ControlledUnaryIterStep.
 */
struct QRET_EXPORT UnaryIterStepGen : CircuitGenerator {
    BigInt step;
    std::size_t n;

    static inline const char* Name = "UnaryIterStep";
    explicit UnaryIterStepGen(CircuitBuilder* builder, const BigInt& step, std::size_t n)
        : CircuitGenerator(builder)
        , step{step}
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{})", GetName(), step, n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("address", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, n - 1, Attribute::Operate);
    }
    Circuit* Generate() const override;
    void GenerateNaive() const;
    void GenerateSawtooth() const;
};
/**
 * @brief Implementation of UnaryIterEnd and ControlledUnaryIterEnd.
 */
struct QRET_EXPORT UnaryIterEndGen : CircuitGenerator {
    BigInt end;
    std::size_t n;

    static inline const char* Name = "UnaryIterEnd";
    explicit UnaryIterEndGen(CircuitBuilder* builder, const BigInt& end, std::size_t n)
        : CircuitGenerator(builder)
        , end{end}
        , n{n} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{})", GetName(), end, n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("address", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, n - 1, Attribute::Operate);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of QROM and MultiControlledQROM.
 */
struct QRET_EXPORT MultiControlledQROMGen : CircuitGenerator {
    std::size_t c;
    std::size_t n;
    std::size_t m;
    std::size_t dict_size;

    static inline const char* Name = "MultiControlledQROMImm";
    explicit MultiControlledQROMGen(
            CircuitBuilder* builder,
            std::size_t c,
            std::size_t n,
            std::size_t m,
            std::size_t dict_size
    )
        : CircuitGenerator(builder)
        , c{c}
        , n{n}
        , m{m}
        , dict_size{dict_size} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{},{},{})", GetName(), c, n, m, dict_size);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("cs", Type::Qubit, c, Attribute::Operate);
        arg.Add("address", Type::Qubit, n, Attribute::Operate);
        arg.Add("value", Type::Qubit, m, Attribute::Operate);
        arg.Add("aux", Type::Qubit, n + c - 1, Attribute::CleanAncilla);
        arg.Add("dict", Type::Register, dict_size * m, Attribute::Input);
    }
    Circuit* Generate() const override;
    void GenerateN1() const;
};
/**
 * @brief Implementation of QROMImm and MultiControlledQROMImm.
 */
struct QRET_EXPORT MultiControlledQROMImmGen : CircuitGenerator {
    std::size_t c;
    std::size_t n;
    std::size_t m;
    std::span<const BigInt> dict;

    static inline const char* Name = "MultiControlledQROMImm";
    explicit MultiControlledQROMImmGen(
            CircuitBuilder* builder,
            std::size_t c,
            std::size_t n,
            std::size_t m,
            std::span<const BigInt> dict
    )
        : CircuitGenerator(builder)
        , c{c}
        , n{n}
        , m{m}
        , dict{dict} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("cs", Type::Qubit, c, Attribute::Operate);
        arg.Add("address", Type::Qubit, n, Attribute::Operate);
        arg.Add("value", Type::Qubit, m, Attribute::Operate);
        arg.Add("aux", Type::Qubit, n + c - 1, Attribute::CleanAncilla);
    }
    Circuit* Generate() const override;
    void GenerateN1() const;
};
/**
 * @brief Implementation of UncomputeQROM.
 */
struct QRET_EXPORT UncomputeQROMGen : CircuitGenerator {
    std::size_t n;
    std::size_t m;
    std::size_t dict_size;

    static inline const char* Name = "UncomputeQROM";
    explicit UncomputeQROMGen(
            CircuitBuilder* builder,
            std::size_t n,
            std::size_t m,
            std::size_t dict_size
    )
        : CircuitGenerator(builder)
        , n{n}
        , m{m}
        , dict_size{dict_size} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{},{})", GetName(), n, m, dict_size);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("address", Type::Qubit, n, Attribute::Operate);
        arg.Add("value", Type::Qubit, m, Attribute::Operate);
        arg.Add("aux", Type::Qubit, n - 1, Attribute::CleanAncilla);
        arg.Add("dict", Type::Register, dict_size * m, Attribute::Input);
    }
    Circuit* Generate() const override;
    void GenerateN1() const;
};
/**
 * @brief Implementation of UncomputeQROMImm.
 */
struct QRET_EXPORT UncomputeQROMImmGen : CircuitGenerator {
    std::size_t n;
    std::size_t m;
    std::span<const BigInt> dict;

    static inline const char* Name = "UncomputeQROMImm";
    explicit UncomputeQROMImmGen(
            CircuitBuilder* builder,
            std::size_t n,
            std::size_t m,
            std::span<const BigInt> dict
    )
        : CircuitGenerator(builder)
        , n{n}
        , m{m}
        , dict{dict} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("address", Type::Qubit, n, Attribute::Operate);
        arg.Add("value", Type::Qubit, m, Attribute::Operate);
        arg.Add("aux", Type::Qubit, n - 1, Attribute::CleanAncilla);
    }
    Circuit* Generate() const override;
    void GenerateN1() const;
};
/**
 * @brief Implementation of QROAMClean.
 */
struct QRET_EXPORT QROAMCleanGen : CircuitGenerator {
    std::size_t num_par;
    std::size_t n;
    std::size_t m;
    std::size_t dict_size;

    static inline const char* Name = "QROAMClean";
    explicit QROAMCleanGen(
            CircuitBuilder* builder,
            std::size_t num_par,
            std::size_t n,
            std::size_t m,
            std::size_t dict_size
    )
        : CircuitGenerator(builder)
        , num_par{num_par}
        , n{n}
        , m{m}
        , dict_size{dict_size} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{},{},{})", GetName(), num_par, n, m, dict_size);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("address", Type::Qubit, n, Attribute::Operate);
        arg.Add("value", Type::Qubit, m, Attribute::Operate);
        arg.Add("duplicate",
                Type::Qubit,
                ((std::size_t{1} << num_par) - 1) * m,
                Attribute::Operate);
        arg.Add("aux", Type::Qubit, n - num_par - 1, Attribute::CleanAncilla);
        arg.Add("dict", Type::Register, dict_size * m, Attribute::Input);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of UncomputeQROAMClean.
 */
struct QRET_EXPORT UncomputeQROAMCleanGen : CircuitGenerator {
    std::size_t num_par;
    std::size_t n;
    std::size_t m;
    std::size_t dict_size;

    static inline const char* Name = "UncomputeQROAMClean";
    explicit UncomputeQROAMCleanGen(
            CircuitBuilder* builder,
            std::size_t num_par,
            std::size_t n,
            std::size_t m,
            std::size_t dict_size
    )
        : CircuitGenerator(builder)
        , num_par{num_par}
        , n{n}
        , m{m}
        , dict_size{dict_size} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{},{},{})", GetName(), num_par, n, m, dict_size);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("address", Type::Qubit, n, Attribute::Operate);
        arg.Add("value", Type::Qubit, m, Attribute::Operate);
        arg.Add("duplicate", Type::Qubit, std::size_t{1} << num_par, Attribute::CleanAncilla);
        arg.Add("aux", Type::Qubit, n - num_par - 1, Attribute::CleanAncilla);
        arg.Add("dict", Type::Register, dict_size * m, Attribute::Input);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of QROAMDirty.
 */
struct QRET_EXPORT QROAMDirtyGen : CircuitGenerator {
    std::size_t num_par;
    std::size_t n;
    std::size_t m;
    std::size_t dict_size;

    static inline const char* Name = "QROAMDirty";
    explicit QROAMDirtyGen(
            CircuitBuilder* builder,
            std::size_t num_par,
            std::size_t n,
            std::size_t m,
            std::size_t dict_size
    )
        : CircuitGenerator(builder)
        , num_par{num_par}
        , n{n}
        , m{m}
        , dict_size{dict_size} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{},{},{})", GetName(), num_par, n, m, dict_size);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("address", Type::Qubit, n, Attribute::Operate);
        arg.Add("value", Type::Qubit, m, Attribute::Operate);
        arg.Add("duplicate",
                Type::Qubit,
                ((std::size_t{1} << num_par) - 1) * m,
                Attribute::DirtyAncilla);
        arg.Add("aux", Type::Qubit, n - num_par - 1, Attribute::CleanAncilla);
        arg.Add("dict", Type::Register, dict_size * m, Attribute::Input);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of UncomputeQROAMDirty.
 */
struct QRET_EXPORT UncomputeQROAMDirtyGen : CircuitGenerator {
    std::size_t num_par;
    std::size_t n;
    std::size_t m;
    std::size_t dict_size;

    static inline const char* Name = "UncomputeQROAMDirty";
    explicit UncomputeQROAMDirtyGen(
            CircuitBuilder* builder,
            std::size_t num_par,
            std::size_t n,
            std::size_t m,
            std::size_t dict_size
    )
        : CircuitGenerator(builder)
        , num_par{num_par}
        , n{n}
        , m{m}
        , dict_size{dict_size} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{},{},{})", GetName(), num_par, n, m, dict_size);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("address", Type::Qubit, n, Attribute::Operate);
        arg.Add("value", Type::Qubit, m, Attribute::Operate);
        arg.Add("duplicate", Type::Qubit, (std::size_t{1} << num_par) - 1, Attribute::DirtyAncilla);
        arg.Add("src", Type::Qubit, 1, Attribute::CleanAncilla);
        arg.Add("aux", Type::Qubit, n - num_par - 1, Attribute::CleanAncilla);
        arg.Add("dict", Type::Register, dict_size * m, Attribute::Input);
    }
    Circuit* Generate() const override;
};
/**
 * @brief Implementation of QROMParallel.
 */
struct QRET_EXPORT QROMParallelGen : CircuitGenerator {
    std::size_t num_par;
    std::size_t n;
    std::size_t m;
    std::size_t dict_size;

    static inline const char* Name = "QROMParallel";
    explicit QROMParallelGen(
            CircuitBuilder* builder,
            std::size_t num_par,
            std::size_t n,
            std::size_t m,
            std::size_t dict_size
    )
        : CircuitGenerator(builder)
        , num_par{num_par}
        , n{n}
        , m{m}
        , dict_size{dict_size} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{},{},{})", GetName(), num_par, n, m, dict_size);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("address", Type::Qubit, n, Attribute::Operate);
        arg.Add("value", Type::Qubit, m, Attribute::Operate);
        arg.Add("duplicate_address",
                Type::Qubit,
                ((std::size_t{1} << num_par) - 1) * n,
                Attribute::CleanAncilla);
        arg.Add("duplicate_aux",
                Type::Qubit,
                (std::size_t{1} << num_par) * (n - 1),
                Attribute::CleanAncilla);
        arg.Add("dict", Type::Register, dict_size * m, Attribute::Input);
    }
    Circuit* Generate() const override;
};
}  // namespace qret::frontend::gate

#endif  // QRET_GATE_DATA_QROM_H
