/**
 * @file qret/frontend/openqasm2.h
 * @brief Build circuit from OpenQASM2 AST.
 * @details This file defines a builder of circuit from OpenQASM2 AST.
 */

#ifndef QRET_FRONTEND_OPENQASM2_H
#define QRET_FRONTEND_OPENQASM2_H

#include <string_view>

#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/parser/openqasm2.h"
#include "qret/qret_export.h"

namespace qret::frontend {
/**
 * @brief Build circuit from AST.
 * @details TODO: Write conversion mechanism based on InsertU().
 *
 * @param ast AST
 * @param builder builder
 * @param entry_name name of the entry circuit
 * @return Circuit* built circuit
 */
QRET_EXPORT Circuit* BuildCircuitFromAST(
        const openqasm2::Program& ast,
        CircuitBuilder& builder,
        std::string_view entry_name = "main"
);
}  // namespace qret::frontend

#endif  // QRET_FRONTEND_OPENQASM2_H
