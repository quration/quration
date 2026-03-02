/**
 * @file qret/algorithm/util/multi_control.cpp
 */

#include "qret/algorithm/util/multi_control.h"

#include "qret/frontend/intrinsic.h"
#include "qret/ir/instructions.h"

namespace qret::frontend::gate {
//--------------------------------------------------//
// Functions
//--------------------------------------------------//

ir::CallInst* MCMX(const Qubits& cs, const Qubits& tgt, const DirtyAncillae& aux) {
    auto gen = MCMXGen(cs.GetBuilder(), cs.Size(), tgt.Size(), aux.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(cs, tgt, aux);
}

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

Circuit* MCMXGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto cs = GetQubits(0);  // size = c
    auto tgt = GetQubits(1);  // size = n
    auto aux = GetQubits(2);  // size = m

    if (n == 0) {
        return EndCircuitDefinition();
    }
    assert(n >= 1);

    const auto multi_cnot = [](const Qubits& qs) {
        for (auto i = std::size_t{1}; i < qs.Size(); ++i) {
            CX(qs[i], qs[0]);
        }
    };
    if (c == 0) {
        for (const auto& q : tgt) {
            X(q);
        }
        return EndCircuitDefinition();
    } else if (c == 1) {
        for (const auto& q : tgt) {
            CX(q, cs[0]);
        }
        return EndCircuitDefinition();
    } else if (c == 2) {
        multi_cnot(tgt);
        CCX(tgt[0], cs[0], cs[1]);
        multi_cnot(tgt);
        return EndCircuitDefinition();
    }
    assert(c >= 3);

    multi_cnot(tgt);
    const auto dirty = tgt.Range(1, n - 1) + aux;
    if (dirty.Size() >= c - 2) {
        // If there is a lot of dirty ancillae
        CCX(tgt[0], cs[c - 1], dirty[c - 3]);
        for (auto i = c - 2; i >= 2; --i) {
            CCX(dirty[i - 1], cs[i], dirty[i - 2]);
        }
        CCX(dirty[0], cs[0], cs[1]);
        for (auto i = std::size_t{2}; i <= c - 2; ++i) {
            CCX(dirty[i - 1], cs[i], dirty[i - 2]);
        }
        CCX(tgt[0], cs[c - 1], dirty[c - 3]);
        for (auto i = c - 2; i >= 2; --i) {
            CCX(dirty[i - 1], cs[i], dirty[i - 2]);
        }
        CCX(dirty[0], cs[0], cs[1]);
        for (auto i = std::size_t{2}; i <= c - 2; ++i) {
            CCX(dirty[i - 1], cs[i], dirty[i - 2]);
        }
    } else {
        // If there are few ancillae
        // Run MCX twice by using half of `c` as aux
        const auto half = (c + 1) / 2;
        const auto cs_lo = cs.Range(0, half);
        const auto cs_hi = cs.Range(half, c - half);
        MCMX(cs_lo, dirty[0], cs_hi + dirty.Range(1, dirty.Size() - 1));
        MCMX(cs_hi + dirty[0], tgt[0], cs_lo + dirty.Range(1, dirty.Size() - 1));
        MCMX(cs_lo, dirty[0], cs_hi + dirty.Range(1, dirty.Size() - 1));
        MCMX(cs_hi + dirty[0], tgt[0], cs_lo + dirty.Range(1, dirty.Size() - 1));
    }
    multi_cnot(tgt);

    return EndCircuitDefinition();
}
}  // namespace qret::frontend::gate
