/**
 * @file qret/algorithm/transform/qft.cpp
 * @brief QFT.
 */

#include "qret/algorithm/transform/qft.h"

#include <cmath>
#include <numbers>

#include "qret/algorithm/util/rotation.h"
#include "qret/frontend/argument.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/intrinsic.h"

namespace qret::frontend::gate {
//--------------------------------------------------//
// Functions
//--------------------------------------------------//

ir::CallInst* FourierTransform(const Qubits& tgt) {
    auto gen = FourierTransformGen(tgt.GetBuilder(), tgt.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(tgt);
}

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

Circuit* FourierTransformGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto tgt = GetQubits(0);  // size = n

    for (auto i = std::size_t{0}; i < n; ++i) {
        H(tgt[i]);
        for (auto j = std::size_t{1}; j < n - i; ++j) {
            const auto theta = std::pow(0.5, j) * std::numbers::pi;
            ControlledR1(tgt[i + j], tgt[i], theta);
        }
    }

    return EndCircuitDefinition();
}
}  // namespace qret::frontend::gate
