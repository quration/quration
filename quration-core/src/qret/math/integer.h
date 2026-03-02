/**
 * @file qret/math/integer.h
 * @brief Integer.
 */

#ifndef QRET_MATH_INTEGER_H
#define QRET_MATH_INTEGER_H

#include <cstddef>
#include <utility>

#include "qret/base/type.h"
#include "qret/qret_export.h"

namespace qret::math {
/**
 * @brief Calculate bit size of integer.
 * @details Examples:
 * * 15 (1111)  -> 4
 * * 16 (10000) -> 5
 *
 * @param x integer
 * @return std::size_t bit size
 */
QRET_EXPORT std::size_t BitSizeI(Int x);

/**
 * @brief Calculate bit size of large integer.
 * @details Examples:
 * * 15 (1111)  -> 4
 * * 16 (10000) -> 5
 *
 * @param x large integer
 * @return std::size_t bit size
 */
QRET_EXPORT std::size_t BitSizeL(BigInt x);

/**
 * @brief Count the number of times divisible by 2.
 *
 * @param x integer
 * @return std::size_t
 */
QRET_EXPORT std::size_t NumOfFactor2I(Int x);

/**
 * @brief Count the number of times divisible by 2.
 *
 * @param x large integer
 * @return std::size_t
 */
QRET_EXPORT std::size_t NumOfFactor2L(BigInt x);

/**
 * @brief Calculate remainder of power of integer.
 *
 * @param x large integer
 * @param exp exponent
 * @param N mod
 * @return BigInt x^exp (mod N)
 */
QRET_EXPORT BigInt ModPow(const BigInt& x, const BigInt& exp, const BigInt& N);

/**
 * @brief Calculate extended Euclidean algorithm.
 * @remark Requirement: GCD(x, mod) = 1.
 *
 * @param x positive large integer
 * @param y positive large integer
 * @return std::pair<BigInt, BigInt> return (a, b) s,t, a * x + b * y = 1.
 */
QRET_EXPORT std::pair<BigInt, BigInt> ExtGCD(const BigInt& x, const BigInt& y);

/**
 * @brief Calculate modular multiplicative inverse.
 * @remark Requirement: GCD(x, mod) = 1.
 *
 * @param x positive large integer
 * @param mod mod
 * @return BigInt return i s,t, i * x = 1 (mod N)
 */
QRET_EXPORT BigInt ModularMultiplicativeInverse(const BigInt& x, const BigInt& mod);
}  // namespace qret::math

#endif  // QRET_MATH_INTEGER_H
