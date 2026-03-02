/**
 * @file qret/math/fraction.cpp
 * @brief Fraction.
 */

#include "qret/math/fraction.h"

namespace qret::math {
ContinuedFraction ToContinuedFraction(const BigFraction& fraction) {
    const auto is_negative = fraction < 0;
    auto ret = std::vector<BigInt>();
    BigInt numer = boost::multiprecision::abs(fraction.numerator());
    BigInt denom = boost::multiprecision::abs(fraction.denominator());
    assert(denom != 0);

    auto q = BigInt{0};
    auto r = BigInt{0};

    // Integer part of fraction
    boost::multiprecision::divide_qr(numer, denom, q, r);
    if (is_negative) {
        if (r == 0) {
            ret.emplace_back(-q);
            numer = -r;
        } else {
            ret.emplace_back(-(q + 1));
            numer = denom - r;
        }
    } else {
        ret.emplace_back(q);
        numer = r;
    }

    // Decimal point part of fraction
    while (!numer.is_zero()) {
        assert(0 < numer);
        assert(numer < denom);
        boost::multiprecision::divide_qr(denom, numer, q, r);
        ret.emplace_back(q);
        denom = numer;
        numer = r;
    }

    return ContinuedFraction{ret};
}

BigFraction FromContinuedFraction(const ContinuedFraction& continued_fraction) {
    return FromContinuedFraction(continued_fraction.begin(), continued_fraction.end());
}

BigFraction ContinuedFractionConvergentL(BigFraction fraction, const BigInt& denominator_bound) {
    if (denominator_bound <= 1) {
        throw std::runtime_error("denominator_bound should be larger than 1.");
    }
    if (fraction.denominator() <= denominator_bound) {
        return fraction;
    }

    auto continued_fraction = math::ToContinuedFraction(fraction);
    auto candidates = std::vector<std::pair<BigFraction, BigFraction>>();
    const auto push = [&candidates, fraction, &denominator_bound](const BigFraction& x) -> bool {
        if (x.denominator() > denominator_bound) {
            return false;
        }
        const auto diff = x - fraction;
        if (diff < 0) {
            candidates.emplace_back(-diff, x);
        } else {
            candidates.emplace_back(diff, x);
        }
        return true;
    };
    // Search for rational numbers close to `fraction` while descending the Stern-Brocot tree
    for (auto depth = std::size_t{1}; depth < continued_fraction.Size(); ++depth) {
        const auto begin = continued_fraction.begin();
        const auto end = continued_fraction.begin() + static_cast<std::int64_t>(depth) + 1;
        if (continued_fraction[depth] == 1) {
            const auto success = push(math::FromContinuedFraction(begin, end));
            if (!success) {
                break;
            }
        } else {
            const auto s1 = push(math::FromContinuedFraction(begin, end));
            continued_fraction[depth]++;
            const auto s2 = push(math::FromContinuedFraction(begin, end));
            continued_fraction[depth]--;

            if (!s1 && !s2) {
                // Binary search to find the very edge where the denominator does not exceed `bound`
                // There is a value (`lo` <= `value` < `hi`) where the denominator is greater than
                // `bound`.
                auto lo = BigInt{2};
                auto hi = continued_fraction[depth];
                while (hi - lo > 1) {
                    auto mid = (lo + hi) / 2;
                    continued_fraction[depth] = mid;
                    const auto x = math::FromContinuedFraction(begin, end);
                    if (x.denominator() > denominator_bound) {
                        hi = mid;
                    } else {
                        lo = mid;
                    }
                }
                continued_fraction[depth] = lo;
                push(math::FromContinuedFraction(begin, end));
                break;
            }
        }
    }
    std::sort(candidates.begin(), candidates.end());
    return candidates.front().second;
}
}  // namespace qret::math
