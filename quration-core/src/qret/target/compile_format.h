/**
 * @file qret/target/compile_format.h
 * @brief Common compile formats for target backends.
 */

#ifndef QRET_TARGET_COMPILE_FORMAT_H
#define QRET_TARGET_COMPILE_FORMAT_H

#include <cstdint>
#include <string>
#include <string_view>

namespace qret {
enum class CompileFormat : std::uint8_t {
    IR,
    OPENQASM2,
    SC_LS_FIXED_V0,
};

CompileFormat CompileFormatFromString(std::string_view sv);
std::string ToString(CompileFormat format);
}  // namespace qret

#endif  // QRET_TARGET_COMPILE_FORMAT_H
