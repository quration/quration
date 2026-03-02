/**
 * @file qret/analysis/circuit_drawer.h
 * @brief Quantikz-based quantum func drawer (LaTeX emitter).
 * @details This header defines a lightweight utility for generating LaTeX code using the
 * **quantikz** package. It maintains a column-major buffer of per-wire tokens
 * (strings) and guarantees non-overlapping placements by tracking the next
 * available "column" for a contiguous span of wires.
 *
 * ### Concepts
 *
 * - **Wires**: The first `n_qubits` rows are quantum wires; the next
 *   `n_registers` rows are classical wires.
 * - **Columns**: Each logical time step/gate occupies one column. Columns are
 *   created lazily and re-used when there is no vertical overlap.
 * - **Placement**: Multi-wire gates compute an enclosing span `[min_wire, max_wire]`
 *   and occupy a single shared column; scalar gates occupy one wire in one column.
 *
 * ### Output
 *
 * `Print()` emits a complete `quantikz` environment, with `\qw` for quantum wires
 * and `\cw` for classical wires. The caller is responsible for including:
 *
 * @code{.tex}
 * \usepackage{quantikz}
 * @endcode
 *
 * ### Indexing
 * - Quantum wires: `q \in [0, n_qubits)`.
 * - Classical registers: `r \in [0, n_registers)`, addressed as row `n_qubits + r`.
 *
 * @note Bounds are not checked in most mutators for performance; invalid indices
 *       lead to undefined behavior.
 * @see https://ctan.org/pkg/quantikz for the quantikz package.
 */

#ifndef QRET_ANALYSIS_CIRCUIT_DRAWER_H
#define QRET_ANALYSIS_CIRCUIT_DRAWER_H

#include <fmt/format.h>

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

#include "qret/qret_export.h"

namespace qret {
/**
 * @brief Class for drawing quantum circuits using quantikz.
 * @details Construct with the number of quantum wires and classical registers. Use
 * gate methods (e.g., `H`, `CX`, `Measure`, etc.) to populate the internal
 * buffer, then call `Print()` to emit LaTeX.
 *
 * **Placement policy**
 * - The drawer packs gates to the left by reusing the earliest compatible
 *   column whose vertical span is not already occupied.
 * - Multi-control gates (`CCX`, `MCX`, etc.) add appropriate `\ctrl{}` and
 *   `\control \qw` tokens for endpoint and intermediate controls respectively.
 *
 * **Label handling**
 * - Box labels are escaped for LaTeX special characters via `EscapeLabel()`.
 * - Rotation angles are formatted as multiples of `\pi` with small rational
 *   denominators when possible (`AngleToString()`).
 *
 * @warning The class does not enforce wire bounds at runtime except where noted.
 *          Ensure all indices are valid.
 */
class QRET_EXPORT CircuitDrawer {
public:
    CircuitDrawer(std::size_t n_qubits, std::size_t n_registers)
        : n_qubits_(n_qubits)
        , n_registers_(n_registers)
        , content_()
        , next_place_(n_qubits + n_registers) {}

    std::ostream& Print(std::ostream& out, const std::vector<std::string>& lsticks = {}) const;

    void Barrier(std::string_view label);
    void Measure(std::size_t q, std::size_t r);
    void I(std::size_t q);
    void X(std::size_t q);
    void Y(std::size_t q);
    void Z(std::size_t q);
    void H(std::size_t q);
    void S(std::size_t q);
    void SDag(std::size_t q);
    void T(std::size_t q);
    void TDag(std::size_t q);
    void RX(std::size_t q, double theta);
    void RY(std::size_t q, double theta);
    void RZ(std::size_t q, double theta);
    void CX(std::size_t t, std::size_t c);
    void CY(std::size_t t, std::size_t c);
    void CZ(std::size_t t, std::size_t c);
    void CCX(std::size_t t, std::size_t c0, std::size_t c1);
    void CCY(std::size_t t, std::size_t c0, std::size_t c1);
    void CCZ(std::size_t t, std::size_t c0, std::size_t c1);
    void MCX(std::size_t t, const std::vector<std::size_t>& c);
    void RotateGlobalPhase(double theta);
    void Call(
            std::string_view name,
            const std::vector<std::size_t>& op,
            const std::vector<std::size_t>& in,
            const std::vector<size_t>& out
    );
    void ClassicalFunction(
            std::string_view name,
            const std::vector<std::size_t>& in,
            const std::vector<size_t>& out
    );
    void CRand(
            const std::vector<double>& weights,
            const std::vector<std::size_t>& regs,
            std::size_t num_per_line = std::size_t{4}
    );

private:
    void NewColumn();
    std::size_t VacantColumnId(std::size_t l, std::size_t r);
    std::vector<std::string>& FindVacantColumnAndOccupy(std::size_t l, std::size_t r);
    std::string AngleToString(double angle);
    std::string EscapeLabel(std::string_view label);

    std::size_t n_qubits_;
    std::size_t n_registers_;
    std::vector<std::vector<std::string>> content_;  // col major
    std::vector<std::size_t> next_place_;
};
}  // namespace qret

#endif  // QRET_ANALYSIS_CIRCUIT_DRAWER_H
