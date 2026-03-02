/**
 * @file qret/algorithm/arithmetic/modular.cpp
 * @brief Modular integer quantum circuits.
 */

#include "qret/algorithm/arithmetic/modular.h"

#include <span>

#include "qret/algorithm/arithmetic/integer.h"
#include "qret/algorithm/data/qrom.h"
#include "qret/algorithm/util/bitwise.h"
#include "qret/algorithm/util/multi_control.h"
#include "qret/algorithm/util/util.h"
#include "qret/frontend/argument.h"
#include "qret/frontend/attribute.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/functor.h"
#include "qret/frontend/intrinsic.h"
#include "qret/ir/instructions.h"
#include "qret/math/integer.h"

namespace qret::frontend::gate {
//--------------------------------------------------//
// Functions
//--------------------------------------------------//

ir::CallInst* ModNeg(const BigInt& mod, const Qubits& tgt, const DirtyAncillae& aux) {
    auto gen = MultiControlledModNegGen(tgt.GetBuilder(), mod, 0, tgt.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(tgt.Range(0, 0), tgt, aux);
}
ir::CallInst* MultiControlledModNeg(
        const BigInt& mod,
        const Qubits& cs,
        const Qubits& tgt,
        const DirtyAncillae& aux
) {
    auto gen = MultiControlledModNegGen(tgt.GetBuilder(), mod, cs.Size(), tgt.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(cs, tgt, aux);
}
ir::CallInst*
ModAdd(const BigInt& mod, const Qubits& dst, const Qubits& src, const DirtyAncillae& aux) {
    auto gen = ModAddGen(dst.GetBuilder(), mod, dst.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(dst, src, aux);
}
ir::CallInst* MultiControlledModAdd(
        const BigInt& mod,
        const Qubits& cs,
        const Qubits& dst,
        const Qubits& src,
        const DirtyAncillae& aux
) {
    auto gen = MultiControlledModAddGen(dst.GetBuilder(), mod, cs.Size(), dst.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(cs, dst, src, aux.Range(0, gen.m));
}
ir::CallInst*
ModAddImm(const BigInt& mod, const BigInt& imm, const Qubits& tgt, const DirtyAncillae& aux) {
    auto gen = ModAddImmGen(tgt.GetBuilder(), mod, imm, tgt.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(tgt, aux);
}
ir::CallInst* MultiControlledModAddImm(
        const BigInt& mod,
        const BigInt& imm,
        const Qubits& cs,
        const Qubits& tgt,
        const DirtyAncillae& aux
) {
    auto gen = MultiControlledModAddImmGen(tgt.GetBuilder(), mod, imm, cs.Size(), tgt.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(cs, tgt, aux);
}
ir::CallInst*
ModSub(const BigInt& mod, const Qubits& dst, const Qubits& src, const DirtyAncillae& aux) {
    return Adjoint(ModAdd)(mod, dst, src, aux);
}
ir::CallInst* MultiControlledModSub(
        const BigInt& mod,
        const Qubits& cs,
        const Qubits& dst,
        const Qubits& src,
        const DirtyAncillae& aux
) {
    return Adjoint(MultiControlledModAdd)(mod, cs, dst, src, aux);
}
ir::CallInst*
ModSubImm(const BigInt& mod, const BigInt& imm, const Qubits& tgt, const DirtyAncillae& aux) {
    return Adjoint(ModAddImm)(mod, imm, tgt, aux);
}
ir::CallInst* MultiControlledModSubImm(
        const BigInt& mod,
        const BigInt& imm,
        const Qubits& cs,
        const Qubits& tgt,
        const DirtyAncillae& aux
) {
    return Adjoint(MultiControlledModAddImm)(mod, imm, cs, tgt, aux);
}
ir::CallInst* ModDoubling(const BigInt& mod, const Qubits& tgt, const DirtyAncilla& aux) {
    auto gen = MultiControlledModDoublingGen(tgt.GetBuilder(), mod, 0, tgt.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(tgt.Range(0, 0), tgt, aux);
}
ir::CallInst* MultiControlledModDoubling(
        const BigInt& mod,
        const Qubits& cs,
        const Qubits& tgt,
        const DirtyAncilla& aux
) {
    auto gen = MultiControlledModDoublingGen(tgt.GetBuilder(), mod, cs.Size(), tgt.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(cs, tgt, aux);
}
ir::CallInst*
ModBiMulImm(const BigInt& mod, const BigInt& imm, const Qubits& lhs, const Qubits& rhs) {
    auto gen = MultiControlledModBiMulImmGen(lhs.GetBuilder(), mod, imm, 0, lhs.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(lhs.Range(0, 0), lhs, rhs);
}
ir::CallInst* MultiControlledModBiMulImm(
        const BigInt& mod,
        const BigInt& imm,
        const Qubits& cs,
        const Qubits& lhs,
        const Qubits& rhs
) {
    auto gen = MultiControlledModBiMulImmGen(lhs.GetBuilder(), mod, imm, cs.Size(), lhs.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(cs, lhs, rhs);
}
ir::CallInst*
ModScaledAdd(const BigInt& mod, const BigInt& scale, const Qubits& dst, const Qubits& src) {
    auto gen = MultiControlledModScaledAddGen(dst.GetBuilder(), mod, scale, 0, dst.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(dst.Range(0, 0), dst, src);
}
ir::CallInst* ModScaledAddW(
        std::size_t window_size,
        const BigInt& mod,
        const BigInt& scale,
        const Qubits& dst,
        const Qubits& src,
        const CleanAncillae& table,
        const CleanAncillae& aux
) {
    auto gen = ModScaledAddWGen(dst.GetBuilder(), window_size, mod, scale, dst.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(dst, src, table, aux);
}
ir::CallInst* MultiControlledModScaledAdd(
        const BigInt& mod,
        const BigInt& scale,
        const Qubits& cs,
        const Qubits& dst,
        const Qubits& src
) {
    auto gen = MultiControlledModScaledAddGen(dst.GetBuilder(), mod, scale, cs.Size(), dst.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(cs, dst, src);
}
ir::CallInst* ModExpW(
        std::size_t mul_window_size,
        std::size_t exp_window_size,
        const BigInt& mod,
        const BigInt& base,
        const Qubits& tgt,
        const Qubits& exp,
        const CleanAncillae& table,
        const CleanAncillae& tmp,
        const CleanAncillae& aux
) {
    auto gen = ModExpWGen(
            tgt.GetBuilder(),
            mul_window_size,
            exp_window_size,
            mod,
            base,
            tgt.Size(),
            exp.Size()
    );
    auto* circuit = gen.Generate();
    return (*circuit)(tgt, exp, table, tmp, aux);
}

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

Circuit* MultiControlledModNegGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto cs = GetQubits(0);  // size = c
    auto tgt = GetQubits(1);  // size = n
    auto aux = GetQubits(2);  // size = 2

    Adjoint(Inc)(tgt, aux);
    MultiControlledPivotFlipImm(mod - 1, cs, tgt, aux);
    Inc(tgt, aux);

    return EndCircuitDefinition();
}
Circuit* ModAddGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto dst = GetQubits(0);  // size = n
    auto src = GetQubits(1);  // size = n
    auto aux = GetQubits(2);  // size = 2

    for (const auto& q : src) {
        X(q);
    }
    AddImm(mod + 1, src, dst[0]);
    PivotFlip(dst, src, aux);
    PivotFlipImm(mod, dst, src.Range(0, 2));
    for (const auto& q : src) {
        X(q);
    }
    AddImm(mod + 1, src, dst[0]);
    PivotFlip(dst, src, aux);

    return EndCircuitDefinition();
}
Circuit* MultiControlledModAddGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto cs = GetQubits(0);  // size = c
    auto dst = GetQubits(1);  // size = n
    auto src = GetQubits(2);  // size = n
    auto aux = GetQubits(3);  // size = m

    MCMX(cs, src, dst + aux);
    MultiControlledAddImm(mod + 1, cs, src, dst[0]);
    PivotFlip(dst, src, (aux + cs).Range(0, 2));
    MultiControlledPivotFlipImm(mod, cs, dst, src.Range(0, 2));
    MCMX(cs, src, dst + aux);
    MultiControlledAddImm(mod + 1, cs, src, dst[0]);
    PivotFlip(dst, src, (aux + cs).Range(0, 2));

    return EndCircuitDefinition();
}
Circuit* ModAddImmGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto tgt = GetQubits(0);  // size = n
    auto aux = GetQubits(1);  // size = 2

    PivotFlipImm(mod - imm, tgt, aux);
    PivotFlipImm(mod, tgt, aux);
    PivotFlipImm(imm, tgt, aux);

    return EndCircuitDefinition();
}
Circuit* MultiControlledModAddImmGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto cs = GetQubits(0);  // size = c
    auto tgt = GetQubits(1);  // size = n
    auto aux = GetQubits(2);  // size = 2

    MultiControlledPivotFlipImm(mod - imm, cs, tgt, aux);
    MultiControlledPivotFlipImm(mod, cs, tgt, aux);
    MultiControlledPivotFlipImm(imm, cs, tgt, aux);

    return EndCircuitDefinition();
}
Circuit* MultiControlledModDoublingGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto cs = GetQubits(0);  // size = c
    auto tgt = GetQubits(1);  // size = n
    auto aux = GetQubit(2);  // size = 1

    const auto R = mod / 2 + 1;  // mod must be odd

    MultiControlledSubImm(R, cs, tgt, aux);
    MultiControlledAddImm(R, cs + tgt[n - 1], tgt.Range(0, n - 1), aux);
    MCMX(cs, tgt[n - 1], tgt.Range(0, n - 1) + aux);
    MultiControlledBitOrderRotation(n - 1, cs, tgt, aux);

    return EndCircuitDefinition();
}
Circuit* MultiControlledModBiMulImmGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto cs = GetQubits(0);  // size = c
    auto lhs = GetQubits(1);  // size = n
    auto rhs = GetQubits(2);  // size = n

    const auto inverse = math::ModularMultiplicativeInverse(imm, mod);

    MultiControlledModScaledAdd(mod, imm, cs, rhs, lhs);
    Adjoint(MultiControlledModScaledAdd)(mod, inverse, cs, lhs, rhs);
    MultiControlledModScaledAdd(mod, imm, cs, rhs, lhs);
    MultiControlledBitOrderRotation(n, cs, lhs + rhs, cs.Range(0, 0));
    MultiControlledModNeg(mod, cs, rhs, lhs.Range(0, 2));

    return EndCircuitDefinition();
}
Circuit* MultiControlledModScaledAddGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto cs = GetQubits(0);  // size = c
    auto dst = GetQubits(1);  // size = n >= 3
    auto src = GetQubits(2);  // size = n

    for (auto i = std::size_t{0}; i < n - 1; ++i) {
        Adjoint(ModDoubling)(mod, dst, src[0]);
    }
    for (auto j = std::size_t{0}; j < n - 1; ++j) {
        const auto i = n - 1 - j;
        MultiControlledModAddImm(
                mod,
                scale,
                cs + src[i],
                dst,
                (src.Range(0, i) + src.Range(i + 1, n - i - 1)).Range(0, 2)
        );
        ModDoubling(mod, dst, src[0]);
    }
    MultiControlledModAddImm(mod, scale, cs + src[0], dst, src.Range(1, 2));

    return EndCircuitDefinition();
}
Circuit* ModScaledAddWGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto dst = GetQubits(0);  // size = n
    auto src = GetQubits(1);  // size = n
    auto table = GetQubits(2);  // size = n
    auto aux = GetQubits(3);  // size = max(2, n-1)

    const auto num_windows = (n + window_size - 1) / window_size;
    const auto dict_size = std::size_t{1} << window_size;

    auto dict_impl = std::vector<BigInt>(dict_size);
    auto dict = std::span(dict_impl);

    for (auto i = std::size_t{0}; i < num_windows; ++i) {
        const auto lo = i * window_size;
        const auto hi = std::min((i + 1) * window_size, n);
        const auto tmp_size = hi - lo;
        const auto tmp_dict_size = std::size_t{1} << tmp_size;
        assert(tmp_size >= 1);

        // Create a table for each loop
        for (auto k = std::size_t{0}; k < dict_size; ++k) {
            dict_impl[k] = BigInt(k) * scale * (BigInt{1} << lo) % mod;
        }

        QROMImm(src.Range(lo, tmp_size),
                table,
                aux.Range(0, tmp_size - 1),
                dict.subspan(0, tmp_dict_size));
        ModAdd(mod, dst, table, aux.Range(0, 2));
        UncomputeQROMImm(
                src.Range(lo, tmp_size),
                table,
                aux.Range(0, tmp_size - 1),
                dict.subspan(0, tmp_dict_size)
        );
    }

    return EndCircuitDefinition();
}
Circuit* ModExpWGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto tgt = GetQubits(0);  // size = n
    auto exp = GetQubits(1);  // size = m
    auto table = GetQubits(2);  // size = n
    auto tmp = GetQubits(3);  // size = n
    auto aux = GetQubits(4);  // size = mul_window_size + exp_window_size - 1

    const auto num_mul_windows = (n + mul_window_size - 1) / mul_window_size;
    const auto num_exp_windows = (m + exp_window_size - 1) / exp_window_size;

    auto dict_impl = std::vector<BigInt>(std::size_t{1} << (exp_window_size + mul_window_size));
    auto dict = std::span(dict_impl);

    for (auto i = std::size_t{0}; i < num_exp_windows; ++i) {
        const auto exp_lo = i * exp_window_size;
        const auto exp_hi = std::min((i + 1) * exp_window_size, m);
        const auto exp_size = exp_hi - exp_lo;

        // tmp += tgt * base^e mod N
        // (tgt, tmp) : (x, 0) -> (x, x * k^e)
        for (auto j = std::size_t{0}; j < num_mul_windows; ++j) {
            const auto mul_lo = j * mul_window_size;
            const auto mul_hi = std::min((j + 1) * mul_window_size, n);
            const auto mul_size = mul_hi - mul_lo;

            // Create table
            const auto address_size = exp_size + mul_size;
            const auto dict_size = std::size_t{1} << address_size;
            for (auto k = std::uint64_t{0}; k < dict_size; ++k) {
                const auto exp_val = k % (std::uint64_t{1} << exp_size);
                const auto mul_val = k >> exp_size;
                const auto kes = math::ModPow(base, (std::uint64_t{1} << exp_lo) * exp_val, mod);
                const auto value = ((kes * mul_val % mod) * (std::uint64_t{1} << mul_lo)) % mod;
                dict_impl[k] = value;
                assert(value < mod);
            }

            auto address = exp.Range(exp_lo, exp_size) + tgt.Range(mul_lo, mul_size);
            QROMImm(address, table, aux.Range(0, address_size - 1), dict.subspan(0, dict_size));
            ModAdd(mod, tmp, table, aux.Range(0, 2));
            UncomputeQROMImm(
                    address,
                    table,
                    aux.Range(0, address_size - 1),
                    dict.subspan(0, dict_size)
            );
        }

        // tgt -= tmp * inv(base^e) mod N
        // (tgt, tmp) : (x, x * k_e) -> (0, x * k_e)
        for (auto j = std::size_t{0}; j < num_mul_windows; ++j) {
            const auto mul_lo = j * mul_window_size;
            const auto mul_hi = std::min((j + 1) * mul_window_size, n);
            const auto mul_size = mul_hi - mul_lo;

            // Create table
            const auto address_size = exp_size + mul_size;
            const auto dict_size = std::size_t{1} << address_size;
            for (auto k = std::uint64_t{0}; k < dict_size; ++k) {
                const auto exp_val = k % (std::uint64_t{1} << exp_size);
                const auto mul_val = k >> exp_size;
                const auto kes = math::ModPow(base, (std::uint64_t{1} << exp_lo) * exp_val, mod);
                const auto kes_inv = math::ModularMultiplicativeInverse(kes, mod);
                assert(1 == (kes * kes_inv) % mod);
                const auto value = ((kes_inv * mul_val % mod) * (std::uint64_t{1} << mul_lo)) % mod;
                dict_impl[k] = value;
                assert(value < mod);
            }

            auto address = exp.Range(exp_lo, exp_size) + tmp.Range(mul_lo, mul_size);
            QROMImm(address, table, aux.Range(0, address_size - 1), dict.subspan(0, dict_size));
            ModSub(mod, tgt, table, aux.Range(0, 2));
            UncomputeQROMImm(
                    address,
                    table,
                    aux.Range(0, address_size - 1),
                    dict.subspan(0, dict_size)
            );
        }

        /// \todo (FIXME) Implement with variable renaming instead of qubit swap

        // Swap tmp <--> tgt.
        // (tgt, tmp) : (0, x * k_e) -> (x * k_e, 0)
        for (auto j = std::size_t{0}; j < n; ++j) {
            Swap(tgt[j], tmp[j]);
        }

        for (const auto& q : tmp) {
            MARK_AS_CLEAN(q);
        }
    }

    return EndCircuitDefinition();
}
}  // namespace qret::frontend::gate
