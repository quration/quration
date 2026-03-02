/**
 * @file qret/base/string.cpp
 * @brief Utility functions for strings.
 */

#include "qret/base/string.h"

#include <fmt/format.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace qret {
namespace {
inline char Lower(char c) {
    if ('A' <= c && c <= 'Z') {
        return c - 'A' + 'a';
    }
    return c;
}
}  // namespace
std::string GetLowerString(std::string_view sv) {
    auto ret = std::string();
    ret.reserve(sv.size());
    std::transform(sv.begin(), sv.end(), std::back_inserter(ret), [](const char c) {
        return Lower(c);
    });
    return ret;
}
std::vector<std::string> SplitString(std::string_view sv, char separator) {
    if (sv.empty()) {
        return {""};
    }

    auto ret = std::vector<std::string>();
    auto first_time = true;
    auto prev = sv.begin();  // NOLINT
    while (prev < sv.end()) {
        if (!first_time) {
            prev++;
        }
        auto tmp = std::find(prev, sv.end(), separator);  // NOLINT
        ret.emplace_back(prev, tmp);
        prev = tmp;
        first_time = false;
    }
    return ret;
}
std::string LoadFile(const std::string& filepath) {
    auto ifs = std::ifstream(filepath.data());
    if (!ifs.good()) {
        throw std::runtime_error(fmt::format("failed to open: {}", filepath));
    }

    auto ss = std::stringstream();
    ss << ifs.rdbuf();
    return ss.str();
}
}  // namespace qret
