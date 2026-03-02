/**
 * @file qret/math/fraction.h
 * @brief Fraction.
 */

#ifndef QRET_MATH_FRACTION_H
#define QRET_MATH_FRACTION_H

#include <stdexcept>
#include <vector>

#include "qret/base/type.h"
#include "qret/qret_export.h"

namespace qret::math {
/**
 * @brief Container of partial quotients for a simple continued fraction.
 *
 * @verbatim
 *                  1
 * a[0] + ---------------------
 *                   1
 *         a[1] + ------------
 *                 a[2] + ...
 * @endverbatim
 *
 */
struct ContinuedFraction {
    /**
     * @brief Partial quotients.
     */
    std::vector<BigInt> a;

    auto begin() {
        return a.begin();
    }
    auto begin() const {
        return a.begin();
    }
    auto end() {
        return a.end();
    }
    auto end() const {
        return a.end();
    }

    std::size_t Size() const {
        return a.size();
    }

    BigInt& operator[](const std::size_t index) {
        return a[index];
    }
    const BigInt& operator[](const std::size_t index) const {
        return a[index];
    }

    ContinuedFraction(const std::vector<BigInt>& a)  // NOLINT
        : a{a} {}
};

/**
 * @brief Convert a rational number to its (finite) simple continued fraction.
 * @details Examples:
 *
 * * 7 / 5 --> (1, 2, 2)
 *     * 7 / 5 = `1` + 1 / (`2` + (1 / `2`))
 * * 3 / 7 --> (0, 2, 3)
 *     * 3 / 7 = `0` + 1 / (`2` + (1 / `3`))
 *
 * @code {.cpp}
 * auto cf1 = ToContinuedFraction(BigFraction{7,5}); // [1,2,2]
 * auto cf2 = ToContinuedFraction(BigFraction{3,7}); // [0,2,3]
 * @endcode
 *
 * @param fraction rational number
 * @return std::vector<BigInt> continued fraction
 */
QRET_EXPORT ContinuedFraction ToContinuedFraction(const BigFraction& fraction);

/**
 * @brief Convert a simple continued fraction to a rational number.
 * @details Non-positive elements are not allowed except for the first term.
 *
 * @param continued_fraction continued fraction
 * @return BigFraction rational number
 */
QRET_EXPORT BigFraction FromContinuedFraction(const ContinuedFraction& continued_fraction);

/**
 * @brief Convert a simple continued fraction to a rational number.
 * @details Non-positive elements are not allowed except for the first term.
 *
 * Examples:
 *
 * * (1, 2, 2) --> 7 / 5
 *     * `1` + 1 / (`2` + (1 / `2`)) = 7 / 5
 * * (0, 2, 3) --> 3 / 7
 *     * `0` + 1 / (`2` + (1 / `3`)) = 3 / 7
 *
 * @param begin [begin, end) is a region of continued fraction
 * @param end [begin, end) is a region of continued fraction
 * @return BigFraction rational number
 */
template <typename Iterator>
BigFraction FromContinuedFraction(Iterator begin, Iterator end) {
    for (auto itr = begin; itr != end; ++itr) {
        if (itr == begin) {
            continue;
        }
        if (*itr <= 0) {
            throw std::runtime_error("non-positive element in continued fraction");
        }
    }

    auto itr = --end;
    auto numer = *itr;
    auto denom = BigInt{1};
    while (itr != begin) {
        itr--;
        const auto tmp = numer;
        numer = *itr * numer + denom;
        denom = tmp;
    }
    return BigFraction(numer, denom);
}

/**
 * @brief Finds the continued fraction convergent closest to fraction with the denominator less or
 * equal to denominator_bound.
 * @details Algorithm is based on Stern-Brocot tree.
 *
 * @param fraction
 * @param denominator_bound
 * @return BigFraction continued fraction closest to fraction with the denominator less or equal to
 * denominator_bound
 */
QRET_EXPORT BigFraction
ContinuedFractionConvergentL(BigFraction fraction, const BigInt& denominator_bound);
}  // namespace qret::math

#endif  // QRET_MATH_FRACTION_H
