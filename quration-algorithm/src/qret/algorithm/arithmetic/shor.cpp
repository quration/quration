/**
 * @file qret/algorithm/arithmetic/shor.cpp
 * @brief Shor quantum circuits.
 */

#include "qret/algorithm/arithmetic/shor.h"

#include <cmath>
#include <numbers>
#include <vector>

#include "qret/algorithm/arithmetic/modular.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/control_flow.h"
#include "qret/frontend/intrinsic.h"

namespace qret::frontend::gate {
frontend::Circuit* PeriodFinderGen::Generate() const {
    BeginCircuitDefinition();
    auto caux = GetQubits(0);
    auto daux = GetQubits(1);
    auto out = GetRegisters(2);

    auto lhs = caux.Range(0, n);
    auto rhs = daux + caux[n];
    auto ctrl = caux[n + 1];

    auto multipliers = std::vector<BigInt>(depth);
    multipliers.back() = coprime;
    for (std::int32_t i = static_cast<std::int32_t>(depth) - 2; i >= 0; --i) {
        const auto idx = static_cast<std::size_t>(i);
        multipliers[idx] = (multipliers[idx + 1] * multipliers[idx + 1]) % mod;
    }
    X(lhs[0]);

    for (auto i = std::size_t{0}; i < depth; ++i) {
        const auto& mul = multipliers[i];

        H(ctrl);
        MultiControlledModBiMulImm(mod, mul, ctrl, lhs, rhs);
        H(ctrl);

        for (auto j = std::size_t{0}; j < i; ++j) {
            const auto theta = -std::pow(0.5, i - j) * std::numbers::pi;
            control_flow::If(out[j]);
            RX(ctrl, theta);
            control_flow::EndIf(out[j]);
        }

        // Measurement
        Measure(ctrl, out[i]);
        control_flow::If(out[i]);
        X(ctrl);
        control_flow::EndIf(out[i]);
    }

    return EndCircuitDefinition();
}
}  // namespace qret::frontend::gate
