/**
 * @file qret/algorithm/preparation/uniform.cpp
 * @brief Prepare uniform superposition.
 */

#include "qret/algorithm/preparation/uniform.h"

#include <stdexcept>

#include "qret/algorithm/arithmetic/boolean.h"
#include "qret/algorithm/arithmetic/integer.h"
#include "qret/algorithm/util/array.h"
#include "qret/algorithm/util/util.h"
#include "qret/frontend/argument.h"
#include "qret/frontend/attribute.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/intrinsic.h"
#include "qret/ir/instructions.h"

namespace qret::frontend::gate {
//--------------------------------------------------//
// Functions
//--------------------------------------------------//

ir::CallInst*
PrepareUniformSuperposition(const BigInt& N, const Qubits& tgt, const CleanAncillae& aux) {
    auto gen = PrepareUniformSuperpositionGen(tgt.GetBuilder(), N);
    if (tgt.Size() < gen.n) {
        throw std::runtime_error(
                fmt::format("size of `tgt` must be larger than or equal to {}", gen.n)
        );
    }
    if (aux.Size() < gen.m) {
        throw std::runtime_error(
                fmt::format("size of `aux` must be larger than or equal to {}", gen.m)
        );
    }
    auto* circuit = gen.Generate();
    return (*circuit)(tgt.Range(0, gen.n), aux.Range(0, gen.m));
}
ir::CallInst* ControlledPrepareUniformSuperposition(
        const BigInt& N,
        const Qubit& c,
        const Qubits& tgt,
        const CleanAncillae& aux
) {
    auto gen = ControlledPrepareUniformSuperpositionGen(tgt.GetBuilder(), N);
    if (tgt.Size() < gen.n) {
        throw std::runtime_error(
                fmt::format("size of `tgt` must be larger than or equal to {}", gen.n)
        );
    }
    if (aux.Size() < gen.m) {
        throw std::runtime_error(
                fmt::format("size of `aux` must be larger than or equal to {}", gen.m)
        );
    }
    auto* circuit = gen.Generate();
    return (*circuit)(c, tgt.Range(0, gen.n), aux.Range(0, gen.m));
}

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

Circuit* PrepareUniformSuperpositionGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto tgt = GetQubits(0);  // size = n
    auto tmp_aux = GetQubits(1);  // size = m

    const auto lo_size = math::NumOfFactor2L(N);
    const auto hi_size = n - lo_size;
    const BigInt L = N >> lo_size;

    auto lo = tgt.Range(0, lo_size);
    auto hi = tgt.Range(lo_size, hi_size);

    // Create uniform superposition for lower part
    for (const auto& q : lo) {
        H(q);
    }

    if (hi_size == 0) {
        return EndCircuitDefinition();
    } else if (hi_size == 1) {
        assert(0 && "size of hi part is not equal to 1");
        return EndCircuitDefinition();
    }

    // Create uniform superposition for lower part
    assert(hi_size >= 2);
    assert(L >= 3);
    assert(m == hi_size + 3);
    const auto aux = tmp_aux.Range(0, hi_size);
    const auto aux_cmp = tmp_aux.Range(hi_size, 3);
    const auto numerator = (BigInt{1} << (math::BitSizeL(L - 1) - 1)) - L;
    const auto theta = -std::acos(-static_cast<double>(numerator) / static_cast<double>(L));
    // 1. H all
    for (const auto& q : hi) {
        H(q);
    }
    // 2. compare
    {
        const auto dst = aux + aux_cmp[0];
        const auto src = hi + aux_cmp[1];
        ApplyXToEachIfImm(L, dst);
        for (const auto& q : src) {
            X(q);
        }
        AddCuccaro(dst, src, aux_cmp[2]);
        X(aux_cmp[0]);
    }
    // 3. rotate
    RZ(aux_cmp[0], theta);
    // 4. uncompute the previous comparison
    {
        const auto dst = aux + aux_cmp[0];
        const auto src = hi + aux_cmp[1];
        X(aux_cmp[0]);
        SubCuccaro(dst, src, aux_cmp[2]);
        for (const auto& q : src) {
            X(q);
        }
        ApplyXToEachIfImm(L, dst);
    }
    // add opt-hint
    for (const auto& q : tmp_aux) {
        MARK_AS_CLEAN(q);
    }
    // 5. H all
    for (const auto& q : hi) {
        H(q);
    }
    // 6. multi-controlled X
    {
        for (const auto& q : hi) {
            X(q);
        }
        TemporalAnd(aux[0], hi[0], hi[1]);
        for (auto i = std::size_t{2}; i < hi_size; ++i) {
            TemporalAnd(aux[i - 1], hi[i], aux[i - 2]);
        }
    }
    // 7. rotate
    RZ(aux[hi_size - 2], theta);
    // 8. uncompute the previous multi-controlled X
    {
        for (auto i = hi_size - 1; i >= 2; --i) {
            UncomputeTemporalAnd(aux[i - 1], hi[i], aux[i - 2]);
        }
        UncomputeTemporalAnd(aux[0], hi[0], hi[1]);
        for (const auto& q : hi) {
            X(q);
        }
    }
    // 9. H all
    for (const auto& q : hi) {
        H(q);
    }

    return EndCircuitDefinition();
}
Circuit* ControlledPrepareUniformSuperpositionGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto c = GetQubit(0);  // size = 1
    auto tgt = GetQubits(1);  // size = n
    auto tmp_aux = GetQubits(2);  // size = m

    const auto lo_size = math::NumOfFactor2L(N);
    const auto hi_size = n - lo_size;
    const BigInt L = N >> lo_size;

    auto lo = tgt.Range(0, lo_size);
    auto hi = tgt.Range(lo_size, hi_size);

    // Create uniform superposition for lower part controlled by c
    {
        H(tmp_aux[0]);
        T(tmp_aux[0]);
        for (const auto& q : lo) {
            ControlledH(c, q, tmp_aux[0], tmp_aux[1]);
        }
        TDag(tmp_aux[0]);
        H(tmp_aux[0]);
    }

    if (hi_size == 0) {
        return EndCircuitDefinition();
    } else if (hi_size == 1) {
        assert(0 && "size of hi part is not equal to 1");
        return EndCircuitDefinition();
    }

    // Create uniform superposition for lower part
    assert(hi_size >= 2);
    assert(L >= 3);
    assert(m == hi_size + 3);
    const auto aux = tmp_aux.Range(0, hi_size);
    const auto aux_cmp = tmp_aux.Range(hi_size, 3);
    const auto numerator = (BigInt{1} << (math::BitSizeL(L - 1) - 1)) - L;
    const auto theta = -std::acos(-static_cast<double>(numerator) / static_cast<double>(L));
    // 1. H all controlled by c
    {
        H(tmp_aux[0]);
        T(tmp_aux[0]);
        for (const auto& q : hi) {
            ControlledH(c, q, tmp_aux[0], tmp_aux[1]);
        }
        TDag(tmp_aux[0]);
        H(tmp_aux[0]);
    }
    // 2. compare
    {
        const auto dst = aux + aux_cmp[0];
        const auto src = hi + aux_cmp[1];
        ApplyXToEachIfImm(L, dst);
        for (const auto& q : src) {
            X(q);
        }
        AddCuccaro(dst, src, aux_cmp[2]);
        X(aux_cmp[0]);
    }
    // 3. rotate
    RZ(aux_cmp[0], theta);
    // 4. uncompute the previous comparison
    {
        const auto dst = aux + aux_cmp[0];
        const auto src = hi + aux_cmp[1];
        X(aux_cmp[0]);
        SubCuccaro(dst, src, aux_cmp[2]);
        for (const auto& q : src) {
            X(q);
        }
        ApplyXToEachIfImm(L, dst);
    }
    // add opt-hint
    for (const auto& q : tmp_aux) {
        MARK_AS_CLEAN(q);
    }
    // 5. H all
    for (const auto& q : hi) {
        H(q);
    }
    // 6. multi-controlled X
    {
        for (const auto& q : hi) {
            X(q);
        }
        TemporalAnd(aux[0], hi[0], hi[1]);
        for (auto i = std::size_t{2}; i < hi_size; ++i) {
            TemporalAnd(aux[i - 1], hi[i], aux[i - 2]);
        }
        TemporalAnd(aux_cmp[0], aux[hi_size - 2], c);
    }
    // 7. rotate
    RZ(aux_cmp[0], theta);
    // 8. uncompute the previous multi-controlled X
    {
        UncomputeTemporalAnd(aux_cmp[0], aux[hi_size - 2], c);
        for (auto i = hi_size - 1; i >= 2; --i) {
            UncomputeTemporalAnd(aux[i - 1], hi[i], aux[i - 2]);
        }
        UncomputeTemporalAnd(aux[0], hi[0], hi[1]);
        for (const auto& q : hi) {
            X(q);
        }
    }
    // 9. H all
    for (const auto& q : hi) {
        H(q);
    }

    return EndCircuitDefinition();
}
}  // namespace qret::frontend::gate
