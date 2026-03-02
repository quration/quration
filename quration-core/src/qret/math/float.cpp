/**
 * @file qret/math/float.cpp
 * @brief Utility math function with float.
 */

#include "qret/math/float.h"

#include <cmath>
#include <stdexcept>

namespace qret::math {
std::uint32_t CalculateSubBitPrecision(std::uint32_t n, double epsilon) {
    return std::max(0u, static_cast<std::uint32_t>(std::ceil(-std::log2(epsilon * n))));
}
std::vector<bool> DiscretizeDouble(std::size_t precision, double x) {
    if (x < 0.0 || 1.0 <= x) {
        throw std::runtime_error("input double should be in the range [0, 1)");
    }
    auto ret = std::vector<bool>(precision, false);
    for (auto i = std::size_t{0}; i < precision; ++i) {
        x *= 2.0;
        if (x >= 1.0) {
            ret[i] = true;
            x -= 1.0;
        }
    }
    if (x >= 0.5) {
        ret.back() = true;
    }
    return ret;
}
std::tuple<double, std::vector<bool>> DiscretizeDoubleWithError(std::size_t precision, double x) {
    if (x < 0.0 || 1.0 <= x) {
        throw std::runtime_error("input double should be in the range [0, 1)");
    }
    auto ret = std::vector<bool>(precision, false);
    for (auto i = std::size_t{0}; i < precision; ++i) {
        x *= 2.0;
        if (x >= 1.0) {
            ret[i] = true;
            x -= 1.0;
        }
    }
    // if (x >= 0.5) { ret.back() = true; }
    return {x, ret};
}
}  // namespace qret::math
