/**
 * @file qret/math/integer.cpp
 * @brief Integer.
 */

#include "qret/math/integer.h"

namespace qret::math {
std::size_t BitSizeI(Int x) {
    auto ret = std::size_t{0};
    while (x > std::uint64_t{0}) {
        x >>= 1;
        ret++;
    }
    return ret;
}
std::size_t BitSizeL(BigInt x) {
    auto ret = std::size_t{0};
    while (x > BigInt{0}) {
        x >>= 1;
        ret++;
    }
    return ret;
}
std::size_t NumOfFactor2I(Int x) {
    auto ret = std::size_t{0};
    while ((x & Int{1}) == Int{0}) {
        x >>= 1;
        ret++;
    }
    return ret;
}
std::size_t NumOfFactor2L(BigInt x) {
    auto ret = std::size_t{0};
    while ((x & BigInt{1}) == BigInt{0}) {
        x >>= 1;
        ret++;
    }
    return ret;
}
BigInt ModPow(const BigInt& x, const BigInt& exp, const BigInt& N) {
    if (exp == BigInt{0}) {
        return BigInt{1};
    } else if (exp == BigInt{1}) {
        return x % N;
    } else if ((exp & 1) == 0) {
        const auto tmp = ModPow(x, exp >> 1, N);
        return tmp * tmp % N;
    } else {
        // exp % 2 == 1
        const auto tmp = ModPow(x, exp >> 1, N);
        return (tmp * tmp % N) * x % N;
    }
}
std::pair<BigInt, BigInt> ExtGCD(const BigInt& x, const BigInt& y) {
    // Return (a, b) s,t, a * x + b * y = 1.
    if (x <= 0 || y <= 0) {
        throw std::runtime_error(
                "input number is not positive\nx = " + x.str() + ", y = " + y.str()
        );
    }
    if (x == BigInt{1}) {
        return {1, 0};
    } else if (y == BigInt{1}) {
        return {0, 1};
    } else {
        const auto [a, b] = ExtGCD(y % x, x);
        return {b - a * (y / x), a};
    }
}
BigInt ModularMultiplicativeInverse(const BigInt& x, const BigInt& mod) {
    // Assert GCD(x, mod) == 1
    const auto [a, _] = ExtGCD(x, mod);
    if (a < BigInt{0}) {
        return mod - ((-a) % mod);
    } else {
        return a % mod;
    }
}
}  // namespace qret::math
