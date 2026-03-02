/**
 * @file qret/algorithm/preparation/arbitrary.cpp
 * @brief Prepare arbitrary state.
 */

#include "qret/algorithm/preparation/arbitrary.h"

#include <numbers>
#include <stdexcept>

#include "qret/algorithm/data/qrom.h"
#include "qret/algorithm/util/rotation.h"
#include "qret/base/type.h"
#include "qret/frontend/argument.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/intrinsic.h"
#include "qret/ir/instructions.h"
#include "qret/math/float.h"

namespace qret::frontend::gate {
//--------------------------------------------------//
// Functions
//--------------------------------------------------//

ir::CallInst* PrepareArbitraryState(
        std::span<const double> probs,
        const Qubits& tgt,
        const Qubits& theta,
        const CleanAncillae& aux
) {
    auto gen = PrepareArbitraryStateGen(tgt.GetBuilder(), probs, tgt.Size(), theta.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(tgt, theta, aux);
}

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

Circuit* PrepareArbitraryStateGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto tgt = GetQubits(0);  // size = n
    auto theta = GetQubits(1);  // size = m
    auto aux = GetQubits(2);  // size = n-1

    auto cumulative = std::vector<double>((std::size_t{1} << n) + 1, 0.0);
    for (auto i = std::size_t{0}; i < (std::size_t{1} << n); ++i) {
        if (i < probs.size()) {
            cumulative[i + 1] = cumulative[i] + probs[i];
        } else {
            cumulative[i + 1] = cumulative[i];
        }
    }
    // Sum of probabilities is 1.0
    if (std::abs(cumulative.back() - 1.0) > 0.0001) {
        throw std::runtime_error("sum of probabilities is not 1");
    }
    const auto get_angle = [](double p0, double p1) {
        const auto eps = 0.0000001;
        if (std::abs(p0) < eps && std::abs(p1) < eps) {
            return 0.0;
        }
        return std::acos(std::sqrt(p0 / (p0 + p1)));
    };

    // Handle the first qubit specially
    {
        const auto p0 = cumulative[std::size_t{1} << (n - 1)];
        const auto p1 = 1.0 - p0;
        const auto angle = get_angle(p0, p1);
        RY(tgt[n - 1], -2.0 * angle);
    }

    // Rotate from the upper digits
    for (auto j = std::size_t{1}; j < n; ++j) {
        const auto i = n - 1 - j;
        const auto tmp_dict_size = std::size_t{1} << j;

        // Calculate rotation angles
        auto angle_dict = std::vector<BigInt>(tmp_dict_size);
        const auto tmp_num_items = std::size_t{1} << i;
        for (auto k = std::size_t{0}; k < tmp_dict_size; ++k) {
            const auto x = 2 * k * tmp_num_items;
            const auto y = x + tmp_num_items;
            const auto z = y + tmp_num_items;
            const auto p0 = cumulative[y] - cumulative[x];
            const auto p1 = cumulative[z] - cumulative[y];
            const auto angle = get_angle(p0, p1) / std::numbers::pi;
            angle_dict[k] = BoolArrayAsBigInt(math::DiscretizeDouble(m, angle));
        }

        // Write rotation angles to `theta` qubits
        QROMImm(tgt.Range(i + 1, j), theta, aux.Range(0, j - 1), angle_dict);

        // Do Pauli-Y rotation controlled by `theta` qubits
        for (auto k = std::size_t{0}; k < m; ++k) {
            const auto angle = -std::pow(0.5, k) * std::numbers::pi;
            ControlledRY(theta[k], tgt[i], angle);
        }

        // Uncompute the previous QROM
        UncomputeQROMImm(tgt.Range(i + 1, j), theta, aux.Range(0, j - 1), angle_dict);
    }

    return EndCircuitDefinition();
}
}  // namespace qret::frontend::gate
