/**
 * @file qret/parser/openqasm3.h
 * @brief OpenQASM3 parser (compatibility subset).
 * @details This parser accepts only an OpenQASM2-compatible subset of OpenQASM3 syntax.
 */

#ifndef QRET_PARSER_OPENQASM3_H
#define QRET_PARSER_OPENQASM3_H

#include <string>
#include <string_view>

#include "qret/parser/openqasm2.h"
#include "qret/qret_export.h"

namespace qret::openqasm3 {
using Program = qret::openqasm2::Program;

QRET_EXPORT bool CanParseOpenQASM3();
QRET_EXPORT Program ParseOpenQASM3File(std::string_view file);
QRET_EXPORT Program ParseOpenQASM3(const std::string& qasm);
}  // namespace qret::openqasm3

#endif  // QRET_PARSER_OPENQASM3_H
