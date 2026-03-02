/**
 * @file qret/math/lcu_helper.h
 * @brief Helper functions related to linear combinations of unitaries (LCU).
 */

#ifndef QRET_MATH_LCU_HELPER_H
#define QRET_MATH_LCU_HELPER_H

#include <cstdint>
#include <utility>
#include <vector>

#include "qret/base/type.h"
#include "qret/qret_export.h"

namespace qret::math {
/**
 * @brief Approximates probabilities with integers over a common denominator.
 * @note Implemented this function based on OpenFermion.
 *
 * @param unnormalized_probabilities a list of non-negative doubles
 * @param sub_bit_precision the exponent mu such that denominator = n * 2^{mu} where n =
 * unnormalized_probabilities.size()
 * @return std::vector<BigInt> a list of numerators for each probability
 */
QRET_EXPORT std::vector<BigInt> DiscretizeProbabilityDistribution(
        std::vector<double> unnormalized_probabilities,
        std::uint32_t sub_bit_precision
);

/**
 * @brief Preprocess of Walker's alias method.
 * @note Implemented this function based on OpenFermion.
 * @details See
 * https://github.com/quantumlib/OpenFermion/blob/788481753c798a72c5cb3aa9f2aa9da3ce3190b0/src/openfermion/circuits/lcu_util.py#L97C6-L97C6
 * for more information.
 *
 * @param discretized_probabilities a list of probabilities approximated by integer numerators
 * @return std::vector<std::uint32_t, BigInt> n = discretized_probabilities.size()
 * * first:  An alternate index for each index from 0 to n - 1
 * * second: Indicates how often one should switch to alternates[i] instead of staying at index i
 */
QRET_EXPORT std::vector<std::pair<std::uint32_t, BigInt>> PreprocessForEfficientRouletteSelection(
        std::vector<BigInt> discretized_probabilities
);

/**
 * @brief Prepares data used to perform efficient reversible roulette selection.
 * @note Implemented this function based on OpenFermion.
 *
 * @param lcu_coefficients a list of non-negative doubles
 * @param sub_bit_precision the exponent mu such that denominator = n * 2^{mu} where n =
 * lcu_coefficients.size()
 * @return std::vector<std::pair<std::uint32_t, BigInt>> n = lcu_coefficients.size()
 * * first:  An alternate index for each index from 0 to n - 1
 * * second: Indicates how often one should switch to alternates[i] instead of staying at index i
 */
QRET_EXPORT std::vector<std::pair<std::uint32_t, BigInt>>
PreprocessLCUCoefficientsForReversibleSampling(
        const std::vector<double>& lcu_coefficients,
        std::uint32_t sub_bit_precision
);
}  // namespace qret::math

#endif  // QRET_MATH_LCU_HELPER_H
