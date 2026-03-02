/**
 * @file qret/base/type.cpp
 * @brief Classical value types used in quantum computer.
 */

#include "qret/base/type.h"

#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace qret {
BigInt IntAsBigInt(const Int x) {
    return BigInt{x};
}
std::vector<bool> IntAsBoolArray(Int x, std::size_t width) {
    auto ret = std::vector<bool>();
    while (x > Int{0}) {
        if (x % Int{2} == Int{1}) {
            ret.emplace_back(true);
        } else {
            ret.emplace_back(false);
        }
        x >>= Int{1};

        if (width != 0 && ret.size() == width) {
            return ret;
        }
    }
    if (width != std::size_t{0}) {
        const auto size = ret.size();
        for (auto i = size; i < width; ++i) {
            ret.emplace_back(false);
        }
    }
    return ret;
}
std::vector<bool> BigIntAsBoolArray(BigInt x, std::size_t width) {
    auto ret = std::vector<bool>();
    while (x > BigInt{0}) {
        if (x % BigInt{2} == BigInt{1}) {
            ret.emplace_back(true);
        } else {
            ret.emplace_back(false);
        }
        x >>= Int{1};

        if (width != 0 && ret.size() == width) {
            return ret;
        }
    }
    if (width != std::size_t{0}) {
        const auto size = ret.size();
        for (auto i = size; i < width; ++i) {
            ret.emplace_back(false);
        }
    }
    return ret;
}
Int BoolArrayAsInt(const std::vector<bool>& bool_array) {
    if (bool_array.size() > 8 * sizeof(Int)) {
        throw std::runtime_error("size of bool array is larger than bit-width of Int");
    }
    auto ret = Int{0};
    for (auto i = std::size_t{0}; i < bool_array.size(); ++i) {
        if (bool_array[i]) {
            ret |= (Int{1} << i);
        }
    }
    return ret;
}
BigInt BoolArrayAsBigInt(const std::vector<bool>& bool_array) {
    auto ret = BigInt(0);
    for (auto i = std::size_t{0}; i < bool_array.size(); ++i) {
        if (bool_array[i]) {
            ret |= (BigInt{1} << i);
        }
    }
    return ret;
}
};  // namespace qret
