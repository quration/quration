/**
 * @file qret/base/type.h
 * @brief Classical value types used in quantum computer.
 * @details This file defines classical value types used in quantum computer and also defines the
 * conversion functions between them.
 */

#ifndef QRET_BASE_TYPE_H
#define QRET_BASE_TYPE_H

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/rational.hpp>
#include <fmt/core.h>
#include <fmt/ostream.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include "qret/qret_export.h"

namespace qret {
/**
 * @brief Integer type in quantum computer.
 */
using Int = std::uint64_t;
/**
 * @brief Arbitrary size integer type in quantum computer.
 */
using BigInt = boost::multiprecision::cpp_int;
/**
 * @brief Arbitrary size rational type in quantum computer.
 */
using BigFraction = boost::rational<BigInt>;

/**
 * @brief Convert a given integer to an equivalent big integer.
 */
QRET_EXPORT BigInt IntAsBigInt(Int x);
/**
 * @brief Produce a binary representation of a non-negative integer, using the little-endian
 * representation for the returned array.
 * @details Examples:
 * x = 7 -> {t, t, t}
 * x = 6 -> {f, t, t}
 * x = 10 -> {f, t, f, t}
 * x = 10, width = 6 -> {f, t, f, t, f, f}
 *
 * @param x non-negative integer
 * @param width the number of bits in the binary representation of `x`. If width == 0, the size of
 * returned array is bit size of x.
 * @return std::vector<bool> binary representation
 */
QRET_EXPORT std::vector<bool> IntAsBoolArray(Int x, std::size_t width = 0);
/**
 * @brief Produce a binary representation of a non-negative big integer, using the little-endian
 * representation for the returned array.
 * @param x non-negative big integer
 * @param width the number of bits in the binary representation of `x`. If width == 0, the size of
 * returned array is bit size of x.
 * @return std::vector<bool> binary representation
 */
QRET_EXPORT std::vector<bool> BigIntAsBoolArray(BigInt x, std::size_t width = 0);
/**
 * @brief Interpret bits in LSB-first (LSB0) order as a non-negative integer.
 * @details bool_array[0] is the least significant bit. This function does not depend on machine
 * endianness (byte order).
 *
 * @param bool_array bits in binary representation of number
 * @return Int non-negative integer
 * @throws std::runtime_error if bool_array.size() > 8 * sizeof(Int)
 *
 * @par Examples
 * Basic usage (LSB0 semantics):
 * @code{.cpp}
 * // [1,0,1] -> 1*2^0 + 0*2^1 + 1*2^2 = 5
 * std::vector<bool> a{true, false, true};
 * Int x = BoolArrayAsInt(a);
 * assert(x == Int{5});
 * @endcode
 *
 * @code{.cpp}
 * // Set bits at positions 1 and 5 (LSB0): value = 2 + 32 = 34
 * std::vector<bool> b{false, true, false, false, false, true};
 * Int y = BoolArrayAsInt(b);
 * assert(y == Int{34});
 * @endcode
 *
 * Edge cases:
 * @code{.cpp}
 * // All zeros -> 0
 * std::vector<bool> z(10, false);
 * assert(BoolArrayAsInt(z) == Int{0});
 * @endcode
 */
QRET_EXPORT Int BoolArrayAsInt(const std::vector<bool>& bool_array);
/**
 * @brief Interpret bits in LSB-first (LSB0) order as a non-negative big integer.
 * @details bool_array[0] is the least significant bit. This function does not depend on machine
 * endianness (byte order).
 *
 * @param bool_array bits in binary representation of number
 * @return BigInt non-negative big integer
 */
QRET_EXPORT BigInt BoolArrayAsBigInt(const std::vector<bool>& bool_array);
}  // namespace qret

/**
 * @brief Formatter specialization of qret::BigInt inherited from fmt::ostream_formatter.
 */
template <>
struct fmt::formatter<qret::BigInt> : fmt::ostream_formatter {};
/**
 * @brief Formatter specialization of qret::BigFraction inherited from fmt::ostream_formatter.
 */
template <>
struct fmt::formatter<qret::BigFraction> : fmt::ostream_formatter {};

#endif  // QRET_BASE_TYPE_H
