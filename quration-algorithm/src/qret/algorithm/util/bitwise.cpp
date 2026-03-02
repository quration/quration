/**
 * @file qret/algorithm/util/bitwise.cpp
 */

#include "qret/algorithm/util/bitwise.h"

#include "qret/algorithm/util/multi_control.h"
#include "qret/frontend/argument.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/intrinsic.h"
#include "qret/ir/instructions.h"

namespace qret::frontend::gate {
//--------------------------------------------------//
// Functions
//--------------------------------------------------//

ir::CallInst* MultiControlledBitOrderRotation(
        std::size_t len,
        const Qubits& cs,
        const Qubits& tgt,
        const DirtyAncillae& aux
) {
    auto gen = MultiControlledBitOrderRotationGen(
            cs.GetBuilder(),
            len,
            cs.Size(),
            tgt.Size(),
            aux.Size()
    );
    auto* circuit = gen.Generate();
    return (*circuit)(cs, tgt, aux);
}
ir::CallInst*
MultiControlledBitOrderReversal(const Qubits& cs, const Qubits& tgt, const DirtyAncillae& aux) {
    auto gen =
            MultiControlledBitOrderReversalGen(cs.GetBuilder(), cs.Size(), tgt.Size(), aux.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(cs, tgt, aux);
}

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

Circuit* MultiControlledBitOrderRotationGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto cs = GetQubits(0);
    auto tgt = GetQubits(1);
    auto aux = GetQubits(2);

    MultiControlledBitOrderReversal(cs, tgt.Range(0, len), tgt.Range(len, n - len) + aux);
    MultiControlledBitOrderReversal(cs, tgt.Range(len, n - len), tgt.Range(0, len) + aux);
    MultiControlledBitOrderReversal(cs, tgt, aux);

    return EndCircuitDefinition();
}
Circuit* MultiControlledBitOrderReversalGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto cs = GetQubits(0);
    auto tgt = GetQubits(1);
    auto aux = GetQubits(2);

    if (n <= 1) {
        // do nothing
        return EndCircuitDefinition();
    } else if (c == 0 && n == 2) {
        CX(tgt[0], tgt[1]);
        CX(tgt[1], tgt[0]);
        CX(tgt[0], tgt[1]);
        return EndCircuitDefinition();
    } else if (c == 1 && n == 2) {
        CX(tgt[0], tgt[1]);
        CCX(tgt[1], tgt[0], cs[0]);
        CX(tgt[0], tgt[1]);
        return EndCircuitDefinition();
    }

    if (n % 2 == 0) {
        // n is even number
        const auto k = (n / 2) - 1;
        auto lo_mid = tgt[k];
        auto hi_mid = tgt[k + 1];
        // controlled swap of lo_mid and hi_mid
        CX(hi_mid, lo_mid);
        MCMX(cs + hi_mid, lo_mid, tgt.Range(0, k) + tgt.Range(k + 2, k) + aux);
        CX(hi_mid, lo_mid);
        // same as n is odd
        MultiControlledBitOrderReversal(
                cs,
                tgt.Range(0, k) + lo_mid + tgt.Range(k + 2, k),
                aux + hi_mid
        );
    } else {
        // n is odd number
        const auto half = n / 2;
        auto mid = tgt[half];
        for (auto i = std::size_t{0}; i < half; ++i) {
            CX(tgt[i], tgt[n - 1 - i]);
        }
        MCMX(cs, mid, tgt.Range(0, half) + tgt.Range(half + 1, half) + aux);
        for (auto i = std::size_t{0}; i < half; ++i) {
            CCX(tgt[n - 1 - i], tgt[i], mid);
        }
        MCMX(cs, mid, tgt.Range(0, half) + tgt.Range(half + 1, half) + aux);
        for (auto i = std::size_t{0}; i < half; ++i) {
            CCX(tgt[n - 1 - i], tgt[i], mid);
        }
        for (auto i = std::size_t{0}; i < half; ++i) {
            CX(tgt[i], tgt[n - 1 - i]);
        }
    }

    return EndCircuitDefinition();
}
}  // namespace qret::frontend::gate
