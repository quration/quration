/**
 * @file qret/math/float.h
 * @brief Utility math function with float.
 */

#ifndef QRET_MATH_FLOAT_H
#define QRET_MATH_FLOAT_H

#include <cstddef>
#include <cstdint>
#include <tuple>
#include <vector>

#include "qret/qret_export.h"

namespace qret::math {
/**
 * @brief Calculate sub bit precision to achieve given error tolerance.
 * @note Implemented this function based on OpenFermion.
 *
 * @param n size of list.
 * @param epsilon Absolute error tolerance
 * @return std::uint32_t the exponent mu such that denominator = n * 2^{mu}
 */
QRET_EXPORT std::uint32_t CalculateSubBitPrecision(std::uint32_t n, double epsilon);

/**
 * @brief Discretize floating number to the given precision.
 *
 * @param precision
 * @param x floating number, 0 <= x < 1
 * @return std::vector<bool> discretized number
        x ~ v[0] / 2^1 + v[1] / 2^2 + ... + v[n-1] / 2^{n}
 */
QRET_EXPORT std::vector<bool> DiscretizeDouble(std::size_t precision, double x);

/**
 * @brief Discretize floating number to the given precision and get discretize error.
 *
 * @param precision
 * @param x floating number, 0 <= x < 1
 * @return std::tuple<double, std::vector<bool>> discretized number with error
 *      x = v[0] / 2^1 + v[1] / 2^2 + ... + v[n-1] / 2^{n} + 2^{n} * error
 *      (0 <= error < 1)
 */
QRET_EXPORT std::tuple<double, std::vector<bool>>
DiscretizeDoubleWithError(std::size_t precision, double x);
}  // namespace qret::math

#endif  // QRET_MATH_FLOAT_H
