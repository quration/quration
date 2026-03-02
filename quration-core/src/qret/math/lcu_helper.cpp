/**
 * @file qret/math/lcu_helper.cpp
 * @brief Helper functions related to linear combinations of unitaries (LCU).
 */

#include "qret/math/lcu_helper.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

#include "qret/base/type.h"

namespace qret::math {
std::vector<BigInt> DiscretizeProbabilityDistribution(
        std::vector<double> unnormalized_probabilities,
        std::uint32_t sub_bit_precision
) {
    const auto n = unnormalized_probabilities.size();
    const auto bin_count = (BigInt{1} << sub_bit_precision) * n;

    // Prefix sum
    auto sum = 0.0;
    for (auto i = std::size_t{0}; i < n; ++i) {
        if (unnormalized_probabilities[i] < 0.0) {
            throw std::runtime_error("probability is non-negative");
        }
        sum += unnormalized_probabilities[i];
        unnormalized_probabilities[i] = sum;
    }
    const auto& cumulative = unnormalized_probabilities;

    if (std::abs(sum) < std::numeric_limits<double>::epsilon()) {
        throw std::runtime_error("sum of probabilities is not zero");
    }

    // Conversion: double -> BigInt
    auto ret = std::vector<BigInt>(n);
    for (auto i = std::size_t{0}; i < n; ++i) {
        const auto v = (static_cast<double>(bin_count) * (cumulative[i] / sum)) + 0.5;
        ret[i] = BigInt{std::floor(v)};
    }

    // Calculate difference of cumulative
    auto acc = ret[0];
    for (auto i = std::size_t{1}; i < n; ++i) {
        const auto tmp = ret[i];
        ret[i] -= acc;
        acc = tmp;
    }
    return ret;
}

std::vector<std::pair<std::uint32_t, BigInt>> PreprocessForEfficientRouletteSelection(
        std::vector<BigInt> discretized_probabilities
) {
    if (discretized_probabilities.empty()) {
        throw std::runtime_error("input vector is empty");
    }
    for (const auto& prob : discretized_probabilities) {
        if (prob < 0) {
            throw std::runtime_error("probability must be positive");
        }
    }
    const auto n = discretized_probabilities.size();
    const auto sum = std::accumulate(
            discretized_probabilities.begin(),
            discretized_probabilities.end(),
            BigInt{0}
    );
    const auto target_weight = sum / n;
    if (sum == 0) {
        throw std::runtime_error("sum of discretized probabilities must be positive");
    }
    if (sum != n * target_weight) {
        throw std::runtime_error(
                "sum of discretized_probabilities must be a multiple of "
                "discretized_probabilities.size()."
        );
    }
    auto ret = std::vector<std::pair<std::uint32_t, BigInt>>(n);
    for (auto i = std::size_t{0}; i < n; ++i) {
        ret[i] = {static_cast<std::uint32_t>(i), BigInt{0}};
    }

    // Scan for needy items and donors. First pass will handle all initially-needy items. Second
    // pass will handle any remaining items that started as donors but become needy due to
    // over-donation (though some may also be handled during the first pass).
    // See https://qiita.com/kaityo256/items/1656597198cbfeb7328c for an intuitive understanding of
    // Walker's alias method.
    auto donor_position = std::size_t{0};
    for (auto _ = 0; _ < 2; _++) {
        for (auto i = std::size_t{0}; i < n; ++i) {
            // Is this a needy item?
            if (discretized_probabilities[i] >= target_weight) {
                continue;
            }

            // Find a donor
            while (discretized_probabilities[donor_position] <= target_weight) {
                donor_position++;
            }

            // Donate
            const auto donated = target_weight - discretized_probabilities[i];
            discretized_probabilities[donor_position] -= donated;
            ret[i] = {static_cast<std::uint32_t>(donor_position), donated};

            // Needy item has been repaired. Remove it from consideration
            discretized_probabilities[i] = target_weight;
        }
    }

    return ret;
}

std::vector<std::pair<std::uint32_t, BigInt>> PreprocessLCUCoefficientsForReversibleSampling(
        const std::vector<double>& lcu_coefficients,
        std::uint32_t sub_bit_precision
) {
    return PreprocessForEfficientRouletteSelection(
            DiscretizeProbabilityDistribution(lcu_coefficients, sub_bit_precision)
    );
}
}  // namespace qret::math
