/**
 * @file qret/frontend/qir.h
 * @brief Build circuit from QIR (.ll text).
 */

#ifndef QRET_FRONTEND_QIR_H
#define QRET_FRONTEND_QIR_H

#include <string_view>

#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/qret_export.h"

namespace qret::frontend {
/**
 * @brief Build a circuit from QIR (.ll) text.
 * @details Supports the static-allocation subset of QIR: direct measurement-based conditional
 * branching and record_output values (e.g. if/else), values derived from classical expressions
 * (`icmp`/`add`/`phi`/`select`/... chains, synthesized into a `CallCFInst`) as branch conditions
 * or record_output values, and multi-way `switch`. Dynamic qubit allocation and backward
 * branches/loops are not supported.
 *
 * @param qir_text QIR text
 * @param builder builder
 * @param entry_name name of the entry function within `qir_text`. If empty (the default), the
 * function whose attribute group carries the `entry_point` attribute is used; if none does
 * (hand-written QIR without it), falls back to a function named "main".
 * @return Circuit* built circuit
 */
QRET_EXPORT Circuit* BuildCircuitFromQIR(
        std::string_view qir_text,
        CircuitBuilder& builder,
        std::string_view entry_name = ""
);
/**
 * @brief Build a circuit from a QIR (.ll) file. See BuildCircuitFromQIR() for details.
 */
QRET_EXPORT Circuit* BuildCircuitFromQIRFile(
        std::string_view path,
        CircuitBuilder& builder,
        std::string_view entry_name = ""
);
}  // namespace qret::frontend

#endif  // QRET_FRONTEND_QIR_H
