/**
 * @file qret/target/compile_format.cpp
 * @brief Common compile formats for target backends.
 */

#include "qret/target/compile_format.h"

#include <fmt/format.h>

#include <stdexcept>
#include <string_view>

#include "qret/base/string.h"

namespace qret {
CompileFormat CompileFormatFromString(std::string_view sv) {
    const auto lower = GetLowerString(sv);
    if (lower == "ir") {
        return CompileFormat::IR;
    }
    if (lower == "openqasm2") {
        return CompileFormat::OPENQASM2;
    }
    if (lower == "sc_ls_fixed_v0") {
        return CompileFormat::SC_LS_FIXED_V0;
    }
    throw std::runtime_error(fmt::format("Unknown compile format: {}", sv));
}

std::string ToString(const CompileFormat format) {
    switch (format) {
        case CompileFormat::IR:
            return "ir";
        case CompileFormat::OPENQASM2:
            return "openqasm2";
        case CompileFormat::SC_LS_FIXED_V0:
            return "sc_ls_fixed_v0";
        default:
            break;
    }
    throw std::runtime_error("Unknown compile format");
}
}  // namespace qret
