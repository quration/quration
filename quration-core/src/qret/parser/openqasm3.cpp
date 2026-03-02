/**
 * @file qret/parser/openqasm3.cpp
 * @brief OpenQASM3 parser (compatibility subset).
 */

#include "qret/parser/openqasm3.h"

#include <stdexcept>

namespace qret::openqasm3 {
namespace {
Program ValidateOpenQASM3(Program&& program) {
    if (!program.IsQASM3()) {
        throw std::runtime_error("OpenQASM3 parser expects OPENQASM 3.x header");
    }
    return std::move(program);
}
}  // namespace

bool CanParseOpenQASM3() {
    return qret::openqasm2::CanParseOpenQASM2();
}
Program ParseOpenQASM3File(std::string_view file) {
    return ValidateOpenQASM3(qret::openqasm2::ParseOpenQASM2File(file));
}
Program ParseOpenQASM3(const std::string& qasm) {
    return ValidateOpenQASM3(qret::openqasm2::ParseOpenQASM2(qasm));
}
}  // namespace qret::openqasm3
