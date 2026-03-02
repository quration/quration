/**
 * @file qret/algorithm/arithmetic/integer.cpp
 * @brief Integer quantum circuits.
 */

#include "qret/algorithm/arithmetic/integer.h"

#include <deque>
#include <stdexcept>
#include <tuple>

#include "qret/algorithm/arithmetic/boolean.h"
#include "qret/algorithm/data/qrom.h"
#include "qret/algorithm/util/multi_control.h"
#include "qret/frontend/argument.h"
#include "qret/frontend/attribute.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/functor.h"
#include "qret/frontend/intrinsic.h"
#include "qret/ir/instructions.h"

namespace qret::frontend::gate {
//--------------------------------------------------//
// Functions
//--------------------------------------------------//

ir::CallInst* Inc(const Qubits& tgt, const DirtyAncillae& aux) {
    if (tgt.Size() >= 4 && aux.Size() < 1) {
        throw std::runtime_error(
                "size of `aux` must not be zero if the size of `tgt` is larger than 3"
        );
    }
    auto gen = IncGen(tgt.GetBuilder(), tgt.Size(), aux.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(tgt, aux);
}
ir::CallInst* MultiControlledInc(const Qubits& cs, const Qubits& tgt, const DirtyAncillae& aux) {
    if (aux.Size() < 1) {
        throw std::runtime_error("size of `aux` must not be zero");
    }
    auto gen = MultiControlledIncGen(cs.GetBuilder(), cs.Size(), tgt.Size(), aux.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(cs, tgt, aux);
}
ir::CallInst* Add(const Qubits& dst, const Qubits& src) {
    auto gen = AddGen(dst.GetBuilder(), dst.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(dst, src);
}
ir::CallInst*
AddBrentKung(const Qubits& lhs, const Qubits& rhs, const Qubits& dst, const CleanAncillae& aux) {
    auto gen = AddBrentKungGen(dst.GetBuilder(), dst.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(lhs, rhs, dst, aux);
}
ir::CallInst* AddCuccaro(const Qubits& dst, const Qubits& src, const CleanAncilla& aux) {
    auto gen = AddCuccaroGen(dst.GetBuilder(), dst.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(dst, src, aux);
}
ir::CallInst* AddCraig(const Qubits& dst, const Qubits& src, const CleanAncillae& aux) {
    auto gen = AddCraigGen(dst.GetBuilder(), dst.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(dst, src, aux);
}
ir::CallInst*
ControlledAddCraig(const Qubit& c, const Qubits& dst, const CleanAncillae& src, const Qubits& aux) {
    auto gen = ControlledAddCraigGen(dst.GetBuilder(), dst.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(c, dst, src, aux);
}
ir::CallInst* AddImm(const BigInt& imm, const Qubits& tgt, const DirtyAncilla& aux) {
    auto gen = AddImmGen(tgt.GetBuilder(), imm, tgt.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(tgt, aux);
}
ir::CallInst* MultiControlledAddImm(
        const BigInt& imm,
        const Qubits& cs,
        const Qubits& tgt,
        const DirtyAncilla& aux
) {
    auto gen = MultiControlledAddImmGen(tgt.GetBuilder(), imm, cs.Size(), tgt.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(cs, tgt, aux);
}
ir::CallInst* AddGeneral(const Qubits& dst, const Qubits& src) {
    if (src.Size() < 2) {
        throw std::runtime_error("size of `src` must be larger than 1");
    }
    if (dst.Size() < src.Size()) {
        throw std::runtime_error("size of `dst` must be larger than or equal to the size of `src`");
    }
    auto gen = AddGeneralGen(dst.GetBuilder(), dst.Size(), src.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(dst, src);
}
ir::CallInst* MultiControlledAddGeneral(
        const Qubits& cs,
        const Qubits& dst,
        const Qubits& src,
        const DirtyAncilla& aux
) {
    if (src.Size() < 2) {
        throw std::runtime_error("size of `src` must be larger than 1");
    }
    if (dst.Size() < src.Size()) {
        throw std::runtime_error("size of `dst` must be larger than or equal to the size of `src`");
    }
    auto gen = MultiControlledAddGeneralGen(dst.GetBuilder(), cs.Size(), dst.Size(), src.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(cs, dst, src, aux);
}
ir::CallInst* FinalCarryOfAddImm(
        const BigInt& imm,
        const Qubits& tgt,
        const Qubit& carry,
        const DirtyAncillae& aux
) {
    auto gen = FinalCarryOfAddImmGen(tgt.GetBuilder(), imm, tgt.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(tgt, carry, aux);
}
ir::CallInst* AddPiecewise(
        size_t p,
        const Qubits& dst,
        const Qubits& src,
        const CleanAncillae& aux,
        const Registers& out
) {
    if (dst.Size() != src.Size()) {
        throw std::runtime_error("size of `dst` must be equal to the size of `src`");
    };
    if (p == 0 || p >= dst.Size()) {
        throw std::runtime_error(
                "parameter `p` must be in the range of (0, n). (n: size of `dst`)"
        );
    };
    if (out.Size() == 0 || out.Size() > dst.Size() - p) {
        throw std::runtime_error(
                "size of `out` must be in the range of (0, n-p]. (n: size of `dst`)"
        );
    };
    auto gen = AddPiecewiseGen(dst.GetBuilder(), dst.Size(), out.Size(), p);
    auto* circuit = gen.Generate();
    return (*circuit)(dst, src, aux, out);
}
ir::CallInst* Sub(const Qubits& dst, const Qubits& src) {
    return Adjoint(Add)(dst, src);
}
ir::CallInst*
SubBrentKung(const Qubits& lhs, const Qubits& rhs, const Qubits& dst, const CleanAncillae& aux) {
    return Adjoint(AddBrentKung)(lhs, rhs, dst, aux);
}
ir::CallInst* SubCuccaro(const Qubits& dst, const Qubits& src, const CleanAncilla& aux) {
    return Adjoint(AddCuccaro)(dst, src, aux);
}
ir::CallInst* SubCraig(const Qubits& dst, const Qubits& src, const CleanAncillae& aux) {
    return Adjoint(AddCraig)(dst, src, aux);
}
ir::CallInst*
ControlledSubCraig(const Qubit& c, const Qubits& dst, const Qubits& src, const CleanAncillae& aux) {
    return Adjoint(ControlledAddCraig)(c, dst, src, aux);
}
ir::CallInst* SubImm(const BigInt& imm, const Qubits& tgt, const DirtyAncilla& aux) {
    return Adjoint(AddImm)(imm, tgt, aux);
}
ir::CallInst* MultiControlledSubImm(
        const BigInt& imm,
        const Qubits& cs,
        const Qubits& tgt,
        const DirtyAncilla& aux
) {
    return Adjoint(MultiControlledAddImm)(imm, cs, tgt, aux);
}
ir::CallInst* SubGeneral(const Qubits& dst, const Qubits& src) {
    return Adjoint(AddGeneral)(dst, src);
}
ir::CallInst* MultiControlledSubGeneral(
        const Qubits& cs,
        const Qubits& dst,
        const Qubits& src,
        const DirtyAncilla& aux
) {
    return Adjoint(MultiControlledAddGeneral)(cs, dst, src, aux);
}
ir::CallInst* MulW(
        std::size_t window_size,
        const BigInt& mul,
        const Qubits& tgt,
        const CleanAncillae& table,
        const CleanAncillae& aux
) {
    auto gen = MulWGen(tgt.GetBuilder(), window_size, mul, tgt.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(tgt, table, aux);
}
ir::CallInst*
ScaledAdd(const BigInt& scale, const Qubits& dst, const Qubits& src, const CleanAncillae& aux) {
    auto gen = ScaledAddGen(dst.GetBuilder(), scale, dst.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(dst, src, aux);
}
ir::CallInst* ScaledAddW(
        std::size_t window_size,
        const BigInt& scale,
        const Qubits& dst,
        const Qubits& src,
        const CleanAncillae& table,
        const CleanAncillae& aux
) {
    auto gen = ScaledAddWGen(dst.GetBuilder(), window_size, scale, dst.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(dst, src, table, aux);
}
ir::CallInst*
EqualTo(const Qubits& lhs, const Qubits& rhs, const Qubit& cmp, const CleanAncillae& aux) {
    auto gen = EqualToGen(lhs.GetBuilder(), lhs.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(lhs, rhs, cmp, aux);
}
ir::CallInst*
EqualToImm(const BigInt& imm, const Qubits& tgt, const Qubit& cmp, const CleanAncillae& aux) {
    auto gen = EqualToImmGen(tgt.GetBuilder(), imm, tgt.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(tgt, cmp, aux);
}
ir::CallInst*
LessThan(const Qubits& lhs, const Qubits& rhs, const Qubit& cmp, const CleanAncillae& aux) {
    auto gen = LessThanGen(lhs.GetBuilder(), lhs.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(lhs, rhs, cmp, aux);
}
ir::CallInst* LessThanOrEqualTo(
        const Qubits& lhs,
        const Qubits& rhs,
        const Qubit& cmp,
        const CleanAncillae& aux
) {
    auto gen = LessThanOrEqualToGen(lhs.GetBuilder(), lhs.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(lhs, rhs, cmp, aux);
}
ir::CallInst*
LessThanImm(const BigInt& imm, const Qubits& tgt, const Qubit& cmp, const DirtyAncilla& aux) {
    auto gen = LessThanImmGen(tgt.GetBuilder(), imm, tgt.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(tgt, cmp, aux);
}
ir::CallInst* MultiControlledLessThanImm(
        const BigInt& imm,
        const Qubits& cs,
        const Qubits& tgt,
        const Qubit& cmp,
        const DirtyAncilla& aux
) {
    auto gen = MultiControlledLessThanImmGen(tgt.GetBuilder(), imm, cs.Size(), tgt.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(cs, tgt, cmp, aux);
}
ir::CallInst* BiFlip(const Qubits& dst, const Qubits& src) {
    auto gen = BiFlipGen(dst.GetBuilder(), dst.Size(), src.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(dst, src);
}
ir::CallInst* MultiControlledBiFlip(
        const Qubits& cs,
        const Qubits& dst,
        const Qubits& src,
        const DirtyAncilla& aux
) {
    auto gen = MultiControlledBiFlipGen(dst.GetBuilder(), cs.Size(), dst.Size(), src.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(cs, dst, src, aux);
}
ir::CallInst* BiFlipImm(const BigInt& imm, const Qubits& tgt, const DirtyAncilla& aux) {
    auto gen = BiFlipImmGen(tgt.GetBuilder(), imm, tgt.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(tgt, aux);
}
ir::CallInst* MultiControlledBiFlipImm(
        const BigInt& imm,
        const Qubits& cs,
        const Qubits& tgt,
        const DirtyAncilla& aux
) {
    auto gen = MultiControlledBiFlipImmGen(tgt.GetBuilder(), imm, cs.Size(), tgt.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(cs, tgt, aux);
}
ir::CallInst* PivotFlip(const Qubits& dst, const Qubits& src, const DirtyAncillae& aux) {
    auto gen = PivotFlipGen(dst.GetBuilder(), dst.Size(), src.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(dst, src, aux);
}
ir::CallInst* MultiControlledPivotFlip(
        const Qubits& cs,
        const Qubits& dst,
        const Qubits& src,
        const DirtyAncillae& aux
) {
    auto gen = MultiControlledPivotFlipGen(dst.GetBuilder(), cs.Size(), dst.Size(), src.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(cs, dst, src, aux);
}
ir::CallInst* PivotFlipImm(const BigInt& imm, const Qubits& tgt, const DirtyAncillae& aux) {
    auto gen = PivotFlipImmGen(tgt.GetBuilder(), imm, tgt.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(tgt, aux);
}
ir::CallInst* MultiControlledPivotFlipImm(
        const BigInt& imm,
        const Qubits& cs,
        const Qubits& tgt,
        const DirtyAncillae& aux
) {
    auto gen = MultiControlledPivotFlipImmGen(tgt.GetBuilder(), imm, cs.Size(), tgt.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(cs, tgt, aux);
}

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

Circuit* IncGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto tgt = GetQubits(0);
    auto aux = GetQubits(1);

    if (n == 0) {
        return EndCircuitDefinition();
    } else if (n == 1) {
        X(tgt[0]);
        return EndCircuitDefinition();
    } else if (n == 2) {
        CX(tgt[1], tgt[0]);
        X(tgt[0]);
        return EndCircuitDefinition();
    } else if (n == 3) {
        CCX(tgt[2], tgt[1], tgt[0]);
        CX(tgt[1], tgt[0]);
        X(tgt[0]);
        return EndCircuitDefinition();
    }
    assert(n >= 4);

    if (m >= n) {
        // If there is a lot of dirty qubits
        Sub(tgt, aux);
        for (auto i = std::size_t{0}; i < n; ++i) {
            X(aux[i]);
        }
        Sub(tgt, aux);
        for (auto i = std::size_t{0}; i < n; ++i) {
            X(aux[i]);
        }
    } else {
        // Fig20 of 1706.07884
        if (n % 2 == 0) {
            // n is even number
            MultiControlledInc(tgt[0], tgt.Range(1, n - 1), aux);
            X(tgt[0]);
        } else {
            // n is odd number
            const auto half = (n + 1) / 2;
            const auto lo = tgt.Range(0, half);
            const auto hi = aux[0] + tgt.Range(half, n - half);
            assert(lo.Size() == hi.Size() && "n must be odd");

            Sub(hi, lo);
            MCMX(lo, hi, aux.Range(1, m - 1));
            Add(hi, lo);
            MCMX(lo, hi, aux.Range(1, m - 1));
            Sub(lo, hi);
            for (const auto& x : tgt) {
                X(x);
            }
            X(aux[0]);
            Add(lo, hi);
            for (const auto& x : tgt) {
                X(x);
            }
            X(aux[0]);
        }
    }

    return EndCircuitDefinition();
}
Circuit* MultiControlledIncGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto cs = GetQubits(0);
    auto tgt = GetQubits(1);
    auto aux = GetQubits(2);

    if (n == 0) {
        return EndCircuitDefinition();
    } else if (n == 1) {
        MCMX(cs, tgt, aux);
        return EndCircuitDefinition();
    }
    assert(n >= 2);

    if (n % 2 == 0) {
        // n is even number
        MultiControlledInc(cs + tgt[0], tgt.Range(1, n - 1), aux);
        MCMX(cs, tgt[0], tgt.Range(1, n - 1) + aux);
    } else {
        // n is odd number
        const auto half = (n + 1) / 2;
        const auto lo = tgt.Range(0, half);
        const auto hi = aux[0] + tgt.Range(half, n - half);
        assert(lo.Size() == hi.Size() && "n must be odd");

        Sub(hi, lo);
        MCMX(cs + lo, hi, aux.Range(1, m - 1));
        Add(hi, lo);
        MCMX(cs + lo, hi, aux.Range(1, m - 1));
        Sub(lo, hi);
        MCMX(cs, lo + hi, aux.Range(1, m - 1));
        Add(lo, hi);
        MCMX(cs, lo + hi, aux.Range(1, m - 1));
    }

    return EndCircuitDefinition();
}
Circuit* AddGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto dst = GetQubits(0);
    auto src = GetQubits(1);

    const auto cswap = [](const Qubit& c, const Qubit& lhs, const Qubit& rhs) {
        CX(lhs, rhs);
        CCX(rhs, lhs, c);
        CX(lhs, rhs);
    };

    if (n == 1) {
        CX(dst[0], src[0]);
        return EndCircuitDefinition();
    }

    assert(n >= 2);

    for (auto i = std::size_t{0}; i < n; ++i) {
        CX(dst[i], src[n - 1]);
    }
    for (auto i = std::size_t{0}; i < n - 1; ++i) {
        CX(src[i], src[n - 1]);
    }
    for (auto i = std::size_t{0}; i < n - 1; ++i) {
        CX(dst[i], src[n - 1]);
        cswap(dst[i], src[i], src[n - 1]);
    }
    CX(dst[n - 1], src[n - 1]);
    for (auto j = std::size_t{0}; j < n - 1; ++j) {
        const auto i = n - 2 - j;
        cswap(dst[i], src[i], src[n - 1]);
        CX(dst[i], src[i]);
    }
    for (auto i = std::size_t{0}; i < n; ++i) {
        CX(dst[i], src[n - 1]);
    }
    for (auto i = std::size_t{0}; i < n - 1; ++i) {
        CX(src[i], src[n - 1]);
    }

    return EndCircuitDefinition();
}
Circuit* AddBrentKungGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto lhs = GetQubits(0);
    auto rhs = GetQubits(1);
    auto dst = GetQubits(2);
    auto aux = GetQubits(3);

    for (std::size_t i = 0; i < n; i++) {
        attribute::MarkAsClean(dst[i]);
    }

    for (std::size_t i = 0; i < n - 1; i++) {
        CCX(dst[i + 1], lhs[i], rhs[i]);
    }
    for (std::size_t i = 0; i < n; i++) {
        CX(rhs[i], lhs[i]);
    }

    std::size_t next_ind = 0;
    std::vector<std::deque<std::size_t>> p_inds(n);  // indexes of P_{i,j}
    std::deque<std::tuple<std::size_t, bool, std::size_t, bool, std::size_t>>
            p_ccx;  // save CXX gates which calculate P_{i,j} for uncomputation.
    std::size_t n_power2 = 1;
    while (n_power2 * 2 < n) {
        n_power2 *= 2;
    }
    for (std::size_t i = 1; i <= n / 2; i *= 2) {
        for (std::size_t j = i - 1; i + j < n; j += 2 * i) {
            if (i + j + 1 < n) {
                CCX(dst[i + j + 1],
                    p_inds[i + j].empty() ? rhs[i + j] : aux[p_inds[i + j].back()],
                    dst[j + 1]);
            }
            CCX(aux[next_ind],
                p_inds[j].empty() ? rhs[j] : aux[p_inds[j].back()],
                p_inds[i + j].empty() ? rhs[i + j] : aux[p_inds[i + j].back()]);
            p_ccx.emplace_back(
                    next_ind,
                    p_inds[j].empty(),
                    p_inds[j].empty() ? j : p_inds[j].back(),
                    p_inds[i + j].empty(),
                    p_inds[i + j].empty() ? i + j : p_inds[i + j].back()
            );
            p_inds[i + j].push_back(next_ind);
            next_ind++;
        }
    }
    for (std::size_t i = n_power2; i > 0; i /= 2) {
        for (std::size_t j = (i * 2) - 1; i + j < n; j += 2 * i) {
            if (i + j + 1 < n) {
                CCX(dst[i + j + 1],
                    p_inds[i + j].empty() ? rhs[i + j] : aux[p_inds[i + j].back()],
                    dst[j + 1]);
            }
            CCX(aux[next_ind],
                p_inds[j].empty() ? rhs[j] : aux[p_inds[j].back()],
                p_inds[i + j].empty() ? rhs[i + j] : aux[p_inds[i + j].back()]);
            p_ccx.emplace_back(
                    next_ind,
                    p_inds[j].empty(),
                    p_inds[j].empty() ? j : p_inds[j].back(),
                    p_inds[i + j].empty(),
                    p_inds[i + j].empty() ? i + j : p_inds[i + j].back()
            );
            p_inds[i + j].push_back(next_ind);
            next_ind++;
        }
    }

    while (!p_ccx.empty()) {
        auto [ind0, flag1, ind1, flag2, ind2] = p_ccx.back();
        p_ccx.pop_back();
        CCX(aux[ind0], flag1 ? rhs[ind1] : aux[ind1], flag2 ? rhs[ind2] : aux[ind2]);
    }

    for (std::size_t i = 0; i < n; i++) {
        CX(dst[i], rhs[i]);
        CX(rhs[i], lhs[i]);
    }

    return EndCircuitDefinition();
}
Circuit* AddCuccaroGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto dst = GetQubits(0);
    auto src = GetQubits(1);
    auto aux = GetQubit(2);

    if (n == 1) {
        CX(dst[0], src[0]);
        return EndCircuitDefinition();
    }

    assert(n >= 2);

    MAJ(src[0], aux, dst[0]);
    for (auto i = std::size_t{1}; i < n - 1; ++i) {
        MAJ(src[i], src[i - 1], dst[i]);
    }
    CX(dst[n - 1], src[n - 2]);
    CX(dst[n - 1], src[n - 1]);
    for (auto i = std::size_t{1}; i < n - 1; ++i) {
        const auto j = n - 1 - i;
        UMA(src[j], src[j - 1], dst[j]);
    }
    UMA(src[0], aux, dst[0]);

    return EndCircuitDefinition();
}
Circuit* AddCraigGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto dst = GetQubits(0);
    auto src = GetQubits(1);
    auto aux = GetQubits(2);

    if (n == 1) {
        CX(dst[0], src[0]);
        return EndCircuitDefinition();
    }

    assert(n >= 2);

    TemporalAnd(aux[0], src[0], dst[0]);
    for (auto i = std::size_t{1}; i < n - 1; ++i) {
        CX(src[i], aux[i - 1]);
        CX(dst[i], aux[i - 1]);
        TemporalAnd(aux[i], src[i], dst[i]);
        CX(aux[i], aux[i - 1]);
    }
    CX(dst[n - 1], aux[n - 2]);
    for (auto j = std::size_t{1}; j < n - 1; ++j) {
        const auto i = n - 1 - j;
        CX(aux[i], aux[i - 1]);
        UncomputeTemporalAnd(aux[i], src[i], dst[i]);
        CX(src[i], aux[i - 1]);
    }
    UncomputeTemporalAnd(aux[0], src[0], dst[0]);
    for (auto i = std::size_t{0}; i < n; ++i) {
        CX(dst[i], src[i]);
    }

    return EndCircuitDefinition();
}
Circuit* ControlledAddCraigGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto c = GetQubit(0);
    auto dst = GetQubits(1);
    auto src = GetQubits(2);
    auto aux = GetQubits(3);

    auto tmp_aux = aux[n - 1];

    if (n == 1) {
        TemporalAnd(tmp_aux, c, src[0]);
        CX(dst[0], tmp_aux);
        UncomputeTemporalAnd(tmp_aux, c, src[0]);
        return EndCircuitDefinition();
    }

    assert(n >= 2);

    TemporalAnd(aux[0], src[0], dst[0]);
    for (auto i = std::size_t{1}; i < n - 1; ++i) {
        CX(src[i], aux[i - 1]);
        CX(dst[i], aux[i - 1]);
        TemporalAnd(aux[i], src[i], dst[i]);
        CX(aux[i], aux[i - 1]);
    }
    CX(src[n - 1], aux[n - 2]);
    CX(dst[n - 1], aux[n - 2]);
    TemporalAnd(tmp_aux, c, src[n - 1]);
    CX(dst[n - 1], tmp_aux);
    UncomputeTemporalAnd(tmp_aux, c, src[n - 1]);
    CX(dst[n - 1], aux[n - 2]);
    CX(src[n - 1], aux[n - 2]);
    for (auto j = std::size_t{1}; j < n - 1; ++j) {
        const auto i = n - 1 - j;
        CX(aux[i], aux[i - 1]);
        UncomputeTemporalAnd(aux[i], src[i], dst[i]);
        TemporalAnd(tmp_aux, c, src[i]);
        CX(dst[i], tmp_aux);
        UncomputeTemporalAnd(tmp_aux, c, src[i]);
        CX(dst[i], aux[i - 1]);
        CX(src[i], aux[i - 1]);
    }
    UncomputeTemporalAnd(aux[0], src[0], dst[0]);

    // Calculate the least significant bit
    TemporalAnd(tmp_aux, c, src[0]);
    CX(dst[0], tmp_aux);
    UncomputeTemporalAnd(tmp_aux, c, src[0]);

    return EndCircuitDefinition();
}
Circuit* AddImmGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto tgt = GetQubits(0);  // size = n
    auto aux = GetQubit(1);  // size = 1

    if (n == 0) {
        return EndCircuitDefinition();
    } else if (n == 1) {
        if (imm == 1) {
            X(tgt[0]);
        }
        return EndCircuitDefinition();
    } else if (n == 2) {
        if (imm == 1) {
            CX(tgt[1], tgt[0]);
            X(tgt[0]);
        } else if (imm == 2) {
            X(tgt[1]);
        } else if (imm == 3) {
            CX(tgt[1], tgt[0]);
            X(tgt[0]);
            X(tgt[1]);
        }
        return EndCircuitDefinition();
    }

    const auto half = n / 2;
    const auto lo = tgt.Range(0, half);
    const auto hi = tgt.Range(half, n - half);
    const auto lo_imm = imm & ((BigInt(1) << half) - 1);
    const auto hi_imm = imm >> half;

    MultiControlledInc(aux, hi, lo);
    for (const auto& q : hi) {
        CX(q, aux);
    }
    FinalCarryOfAddImm(lo_imm, lo, aux, hi.Range(0, half));
    MultiControlledInc(aux, hi, lo);
    FinalCarryOfAddImm(lo_imm, lo, aux, hi.Range(0, half));
    for (const auto& q : hi) {
        CX(q, aux);
    }
    AddImm(lo_imm, lo, aux);
    AddImm(hi_imm, hi, aux);

    return EndCircuitDefinition();
}
Circuit* MultiControlledAddImmGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto cs = GetQubits(0);  // size = c
    auto tgt = GetQubits(1);  // size = n
    auto aux = GetQubit(2);  // size = 1

    if (c == 0) {
        AddImm(imm, tgt, aux);
        return EndCircuitDefinition();
    } else if (n == 0) {
        // do nothing
        return EndCircuitDefinition();
    } else if (n == 1) {
        if (imm == 1) {
            MCMX(cs, tgt, aux);
        }
        return EndCircuitDefinition();
    }
    assert(c >= 1);
    assert(n >= 2);

    // Use control as dirty ancillae
    AddImm(imm, aux + tgt, cs[0]);
    MCMX(cs, aux + tgt, cs.Range(0, 0));  // no aux is needed
    Adjoint(AddImm)(imm, aux + tgt, cs[0]);
    MCMX(cs, aux + tgt, cs.Range(0, 0));  // no aux is needed

    return EndCircuitDefinition();
}
Circuit* AddGeneralGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto dst = GetQubits(0);  // size = n >= m
    auto src = GetQubits(1);  // size = m >= 2

    assert(n >= m);
    assert(m >= 2);

    const auto cswap = [](const Qubit& c, const Qubit& lhs, const Qubit& rhs) {
        CX(lhs, rhs);
        CCX(rhs, lhs, c);
        CX(lhs, rhs);
    };

    if (n == m) {
        Add(dst, src);
        return EndCircuitDefinition();
    }

    Adjoint(MultiControlledInc)(src[m - 1], dst, src.Range(0, m - 1));
    MultiControlledInc(src[m - 1], dst.Range(m - 1, n - m + 1), src.Range(0, m - 1));
    for (auto i = std::size_t{0}; i < m - 1; ++i) {
        CX(dst[i], src[m - 1]);
        cswap(dst[i], src[i], src[m - 1]);
    }
    MultiControlledInc(src[m - 1], dst.Range(m - 1, n - m + 1), src.Range(0, m - 1));
    for (auto j = std::size_t{0}; j < m - 1; ++j) {
        const auto i = m - 2 - j;
        cswap(dst[i], src[i], src[m - 1]);
        CX(dst[i], src[i]);
    }

    return EndCircuitDefinition();
}
Circuit* MultiControlledAddGeneralGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto cs = GetQubits(0);  // size = c
    auto dst = GetQubits(1);  // size = n >= m
    auto src = GetQubits(2);  // size = m >= 2
    auto aux = GetQubit(3);  // size = 1

    AddGeneral(aux + dst, src);
    MCMX(cs, aux + dst, src);
    Adjoint(AddGeneral)(aux + dst, src);
    MCMX(cs, aux + dst, src);

    return EndCircuitDefinition();
}
Circuit* FinalCarryOfAddImmGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto tgt = GetQubits(0);  // size = n
    auto carry = GetQubit(1);  // size = 1
    auto aux = GetQubits(2);  // size = n
    const auto is_bit_set = [this](std::size_t i) { return ((imm >> i) & 1) == 1; };

    // compute
    CX(carry, aux[n - 1]);
    for (auto i = n - 1; i >= 1; --i) {
        if (is_bit_set(i)) {
            CX(aux[i], tgt[i]);
            X(tgt[i]);
        }
        CCX(aux[i], tgt[i], aux[i - 1]);
    }
    if (is_bit_set(0)) {
        CX(aux[0], tgt[0]);
    }
    for (auto i = std::size_t{1}; i < n; ++i) {
        CCX(aux[i], tgt[i], aux[i - 1]);
    }
    CX(carry, aux[n - 1]);
    // uncompute
    for (auto i = n - 1; i >= 1; --i) {
        CCX(aux[i], tgt[i], aux[i - 1]);
    }
    if (is_bit_set(0)) {
        CX(aux[0], tgt[0]);
    }
    for (auto i = std::size_t{1}; i < n; ++i) {
        CCX(aux[i], tgt[i], aux[i - 1]);
        if (is_bit_set(i)) {
            X(tgt[i]);
            CX(aux[i], tgt[i]);
        }
    }

    return EndCircuitDefinition();
}
Circuit* AddPiecewiseGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    if (n == 0) {
        return EndCircuitDefinition();
    }

    auto dst = GetQubits(0);
    auto src = GetQubits(1);
    auto aux = GetQubits(2);
    auto out = GetRegisters(3);

    if (n == 1) {
        CX(dst[0], src[0]);
        return EndCircuitDefinition();
    }

    const auto hw = n - p;
    const auto dst_l = dst.Range(0, p);
    const auto src_l = src.Range(0, p);
    const auto dst_h = dst.Range(p, hw);
    const auto src_h = src.Range(p, hw);
    const auto aux_c = aux[0];
    const auto aux_m = aux.Range(1, m);
    const auto aux_pad = aux.Range(m + 1, std::max(hw - m, m));
    const auto aux_h = aux_pad.Range(0, hw - m);
    const auto aux_l = aux_pad.Range(0, m);

    for (auto i = std::size_t{0}; i < m; ++i) {
        H(aux_m[i]);
    }
    SubCuccaro(dst_h, aux_m + aux_h, aux_c);
    AddCuccaro(dst_l + aux_m, src_l + aux_l, aux_c);
    AddCuccaro(dst_h, src_h, aux_c);
    AddCuccaro(dst_h, aux_m + aux_h, aux_c);
    for (auto i = std::size_t{0}; i < m; ++i) {
        Measure(aux_m[i], out[i]);
        control_flow::If(out[i]);
        { X(aux_m[i]); }
        control_flow::EndIf(out[i]);
    }

    return EndCircuitDefinition();
}
Circuit* MulWGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto tgt = GetQubits(0);  // size = n
    auto table = GetQubits(1);  // size = n
    auto aux = GetQubits(2);  // size = n-1

    const auto num_windows = (n + window_size - 1) / window_size;
    const auto dict_size = std::size_t{1} << window_size;

    auto dict_impl = std::vector<BigInt>(dict_size);
    for (auto i = std::size_t{0}; i < dict_size; ++i) {
        dict_impl[i] = (i * mul) >> window_size;
    }
    auto dict = std::span(dict_impl);

    for (auto j = std::size_t{0}; j < num_windows; ++j) {
        const auto i = num_windows - 1 - j;
        const auto lo = i * window_size;
        const auto hi = std::min((i + 1) * window_size, n);
        const auto tmp_size = hi - lo;
        const auto tmp_dict_size = std::size_t{1} << tmp_size;
        assert(tmp_size >= 1);

        // 1. Update outside window
        if (i != num_windows - 1) {
            QROMImm(tgt.Range(lo, tmp_size),
                    table,
                    aux.Range(0, tmp_size - 1),
                    dict.subspan(0, tmp_dict_size));
            AddCraig(tgt.Range(hi, n - hi), table.Range(0, n - hi), aux.Range(0, n - hi - 1));
            UncomputeQROMImm(
                    tgt.Range(lo, tmp_size),
                    table,
                    aux.Range(0, tmp_size - 1),
                    dict.subspan(0, tmp_dict_size)
            );
        }

        // 2. Update inside window
        if (window_size > 1) {
            MulW(std::size_t{1},
                 mul,
                 tgt.Range(lo, tmp_size),
                 table.Range(0, tmp_size),
                 aux.Range(0, tmp_size - 1));
        }
    }

    return EndCircuitDefinition();
}
Circuit* ScaledAddGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto dst = GetQubits(0);  // size = n
    auto src = GetQubits(1);  // size = n
    auto aux = GetQubits(2);  // size = n-1

    for (auto i = std::size_t{0}; i < n; ++i) {
        if (((scale >> i) & 1) == 1) {
            AddCraig(dst.Range(i, n - i), src.Range(0, n - i), aux.Range(0, n - i - 1));
        }
    }

    return EndCircuitDefinition();
}
Circuit* ScaledAddWGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto dst = GetQubits(0);  // size = n
    auto src = GetQubits(1);  // size = n
    auto table = GetQubits(2);  // size = n
    auto aux = GetQubits(3);  // size = n-1

    const auto num_windows = (n + window_size - 1) / window_size;

    const auto dict_size = std::size_t{1} << window_size;
    auto dict_impl = std::vector<BigInt>();
    dict_impl.reserve(dict_size);
    for (auto i = std::size_t{0}; i < dict_size; ++i) {
        dict_impl.emplace_back(scale * i);
    }
    auto dict = std::span(dict_impl);

    for (auto i = std::size_t{0}; i < num_windows; ++i) {
        const auto lo = i * window_size;
        const auto hi = std::min((i + 1) * window_size, n);
        const auto tmp_size = hi - lo;
        assert(tmp_size >= 1);

        QROMImm(src.Range(lo, tmp_size),
                table,
                aux.Range(0, tmp_size - 1),
                dict.subspan(0, std::size_t{1} << tmp_size));
        AddCraig(dst.Range(lo, n - lo), table.Range(0, n - lo), aux.Range(0, n - lo - 1));
        UncomputeQROMImm(
                src.Range(lo, tmp_size),
                table,
                aux.Range(0, tmp_size - 1),
                dict.subspan(0, std::size_t{1} << tmp_size)
        );
    }

    return EndCircuitDefinition();
}
Circuit* EqualToGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto lhs = GetQubits(0);
    auto rhs = GetQubits(1);
    auto cmp = GetQubit(2);
    auto aux = GetQubits(3);

    if (n == 0) {
        return EndCircuitDefinition();
    } else if (n == 1) {
        X(lhs[0]);
        CX(rhs[0], lhs[0]);
        CX(cmp, rhs[0]);
        CX(rhs[0], lhs[0]);
        X(lhs[0]);
        return EndCircuitDefinition();
    }
    assert(n >= 2);

    // computation
    for (auto i = std::size_t{0}; i < n; ++i) {
        X(lhs[i]);
        CX(rhs[i], lhs[i]);
    }
    TemporalAnd(aux[n - 2], rhs[n - 2], rhs[n - 1]);
    for (auto j = std::size_t{0}; j < n - 2; ++j) {
        const auto i = n - 3 - j;
        TemporalAnd(aux[i], aux[i + 1], rhs[i]);
    }
    CX(cmp, aux[0]);
    // uncomputation
    for (auto i = std::size_t{0}; i < n - 2; ++i) {
        UncomputeTemporalAnd(aux[i], aux[i + 1], rhs[i]);
    }
    UncomputeTemporalAnd(aux[n - 2], rhs[n - 2], rhs[n - 1]);
    for (auto i = std::size_t{0}; i < n; ++i) {
        CX(rhs[i], lhs[i]);
        X(lhs[i]);
    }

    return EndCircuitDefinition();
}
Circuit* EqualToImmGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto tgt = GetQubits(0);  // size = n
    auto cmp = GetQubit(1);  // size = 1
    auto aux = GetQubits(2);  // size = n-1

    if (n == 0) {
        return EndCircuitDefinition();
    } else if (n == 1) {
        if (imm == 0) {
            X(tgt[0]);
            CX(cmp, tgt[0]);
            X(tgt[0]);
        } else if (imm == 1) {
            CX(cmp, tgt[0]);
        }
        return EndCircuitDefinition();
    }
    assert(n >= 2);

    const auto is_bit_set = [this](std::size_t i) { return ((imm >> i) & 1) == 1; };
    // compute
    for (auto i = std::size_t{0}; i < n; ++i) {
        if (!is_bit_set(i)) {
            X(tgt[i]);
        }
    }
    TemporalAnd(aux[n - 2], tgt[n - 2], tgt[n - 1]);
    for (auto j = std::size_t{0}; j < n - 2; ++j) {
        const auto i = n - 3 - j;
        TemporalAnd(aux[i], aux[i + 1], tgt[i]);
    }
    CX(cmp, aux[0]);
    // uncompute
    for (auto i = std::size_t{0}; i < n - 2; ++i) {
        UncomputeTemporalAnd(aux[i], aux[i + 1], tgt[i]);
    }
    UncomputeTemporalAnd(aux[n - 2], tgt[n - 2], tgt[n - 1]);
    for (auto i = std::size_t{0}; i < n; ++i) {
        if (!is_bit_set(i)) {
            X(tgt[i]);
        }
    }

    return EndCircuitDefinition();
}
Circuit* LessThanGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto lhs = GetQubits(0);
    auto rhs = GetQubits(1);
    auto cmp = GetQubit(2);
    auto aux = GetQubits(3);

    // lhs -= rhs, size = n + 1
    SubCuccaro(lhs + cmp, rhs + aux[0], aux[1]);
    // lhs += rhs, size = n
    AddCuccaro(lhs, rhs, aux[0]);

    return EndCircuitDefinition();
}
Circuit* LessThanOrEqualToGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto lhs = GetQubits(0);
    auto rhs = GetQubits(1);
    auto cmp = GetQubit(2);
    auto aux = GetQubits(3);

    // lhs -= (rhs + 1), size = n + 1
    {
        for (const auto& q : rhs + aux[0]) {
            X(q);
        }
        AddCuccaro(lhs + cmp, rhs + aux[0], aux[1]);
        for (const auto& q : rhs + aux[0]) {
            X(q);
        }
    }

    // lhs += rhs, size = n
    AddCuccaro(lhs, rhs, aux[0]);

    // lhs += 1, size = n
    {
        for (const auto& q : rhs) {
            X(q);
        }
        SubCuccaro(lhs, rhs, aux[0]);
        for (const auto& q : rhs) {
            X(q);
        }
        SubCuccaro(lhs, rhs, aux[0]);
    }

    return EndCircuitDefinition();
}
Circuit* LessThanImmGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto tgt = GetQubits(0);
    auto cmp = GetQubit(1);
    auto aux = GetQubit(2);

    // tgt -= imm, size = n + 1
    SubImm(imm, tgt + cmp, aux);
    // tgt += imm, size = n
    AddImm(imm, tgt, aux);

    return EndCircuitDefinition();
}
Circuit* MultiControlledLessThanImmGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto cs = GetQubits(0);
    auto tgt = GetQubits(1);
    auto cmp = GetQubit(2);
    auto aux = GetQubit(3);

    // tgt -= imm, size = n + 1
    MultiControlledSubImm(imm, cs, tgt + cmp, aux);
    // tgt += imm, size = n
    MultiControlledAddImm(imm, cs, tgt, aux);

    return EndCircuitDefinition();
}
Circuit* BiFlipGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto dst = GetQubits(0);  // size = n >= m
    auto src = GetQubits(1);  // size = m

    SubGeneral(dst, src);
    for (const auto& q : dst) {
        X(q);
    }

    return EndCircuitDefinition();
}
Circuit* MultiControlledBiFlipGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto cs = GetQubits(0);  // size = c
    auto dst = GetQubits(1);  // size = n >= m
    auto src = GetQubits(2);  // size = m
    auto aux = GetQubit(3);  // size = 1

    MultiControlledSubGeneral(cs, dst, src, aux);
    MCMX(cs, dst, src);

    return EndCircuitDefinition();
}
Circuit* BiFlipImmGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto tgt = GetQubits(0);  // size = n
    auto aux = GetQubit(1);  // size = 1

    SubImm(imm, tgt, aux);
    for (const auto& q : tgt) {
        X(q);
    }

    return EndCircuitDefinition();
}
Circuit* MultiControlledBiFlipImmGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto cs = GetQubits(0);  // size = c
    auto tgt = GetQubits(1);  // size = n
    auto aux = GetQubit(2);  // size = 1

    MultiControlledSubImm(imm, cs, tgt, aux);
    MCMX(cs, tgt, aux);

    return EndCircuitDefinition();
}
Circuit* PivotFlipGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto dst = GetQubits(0);  // size = n >= m
    auto src = GetQubits(1);  // size = m
    auto aux = GetQubits(2);  // size = 2

    // less than
    {
        SubGeneral(dst + aux[0], src);
        AddGeneral(dst, src);
    }
    // controlled bi flip
    {
        MultiControlledSubGeneral(aux[0], dst, src, aux[1]);
        MCMX(aux[0], dst, src + aux[1]);
    }
    // less than
    {
        SubGeneral(dst + aux[0], src);
        AddGeneral(dst, src);
    }
    // controlled bi flip
    {
        MultiControlledSubGeneral(aux[0], dst, src, aux[1]);
        MCMX(aux[0], dst, src + aux[1]);
    }

    return EndCircuitDefinition();
}
Circuit* MultiControlledPivotFlipGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto cs = GetQubits(0);  // size = c
    auto dst = GetQubits(1);  // size = n >= m
    auto src = GetQubits(2);  // size = m
    auto aux = GetQubits(3);  // size = 2

    // less than
    {
        SubGeneral(dst + aux[0], src);
        AddGeneral(dst, src);
    }
    // controlled bi flip
    {
        MultiControlledSubGeneral(cs + aux[0], dst, src, aux[1]);
        MCMX(cs + aux[0], dst, src + aux[1]);
    }
    // less than
    {
        SubGeneral(dst + aux[0], src);
        AddGeneral(dst, src);
    }
    // controlled bi flip
    {
        MultiControlledSubGeneral(cs + aux[0], dst, src, aux[1]);
        MCMX(cs + aux[0], dst, src);
    }

    return EndCircuitDefinition();
}
Circuit* PivotFlipImmGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto tgt = GetQubits(0);  // size = n
    auto aux = GetQubits(1);  // size = 2

    LessThanImm(imm, tgt, aux[0], aux[1]);
    MultiControlledSubImm(imm, aux[0], tgt, aux[1]);
    MCMX(aux[0], tgt, aux[1]);

    LessThanImm(imm, tgt, aux[0], aux[1]);
    MultiControlledSubImm(imm, aux[0], tgt, aux[1]);
    MCMX(aux[0], tgt, aux[1]);

    return EndCircuitDefinition();
}
Circuit* MultiControlledPivotFlipImmGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto cs = GetQubits(0);  // size = c
    auto tgt = GetQubits(1);  // size = n
    auto aux = GetQubits(2);  // size = 2

    LessThanImm(imm, tgt, aux[0], aux[1]);
    MultiControlledSubImm(imm, cs + aux[0], tgt, aux[1]);
    MCMX(cs + aux[0], tgt, aux[1]);

    LessThanImm(imm, tgt, aux[0], aux[1]);
    MultiControlledSubImm(imm, cs + aux[0], tgt, aux[1]);
    MCMX(cs + aux[0], tgt, aux[1]);

    return EndCircuitDefinition();
}
}  // namespace qret::frontend::gate
