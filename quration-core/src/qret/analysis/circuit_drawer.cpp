/**
 * @file qret/analysis/circuit_drawer.cpp
 * @brief Quantikz-based quantum func drawer (LaTeX emitter).
 */

#include "qret/analysis/circuit_drawer.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <numbers>
#include <ostream>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace qret {
std::ostream&
CircuitDrawer::Print(std::ostream& out, const std::vector<std::string>& lsticks) const {
    out << "\\begin{quantikz}[transparent]\n";
    const auto n_columns = content_.size();
    for (const auto i : std::views::iota(std::size_t{0}, n_qubits_ + n_registers_)) {
        if (i < lsticks.size()) {
            out << fmt::format("    \\lstick{{{}}} & ", lsticks[i]);
        } else if (i < n_qubits_) {
            out << "    \\qw & ";
        } else {
            out << "    \\cw & ";
        }
        for (const auto j : std::views::iota(std::size_t{0}, n_columns)) {
            out << content_[j][i] << " & ";
        }
        if (i < n_qubits_) {
            out << "\\qw";
        } else {
            out << "\\cw";
        }
        if (i < n_qubits_ + n_registers_ - 1) {
            out << " \\\\\n";
        } else {
            out << "\n";
        }
    }
    out << "\\end{quantikz}";
    return out;
}

void CircuitDrawer::Barrier(std::string_view label) {
    auto& col = FindVacantColumnAndOccupy(0, n_qubits_ + n_registers_ - 1);
    col[0] += fmt::format(" \\slice{{{}}}", label);
}

void CircuitDrawer::Measure(std::size_t q, std::size_t r) {
    auto& col = FindVacantColumnAndOccupy(q, n_qubits_ + r);
    col[q] = "\\meter{}";
    col[n_qubits_ + r] = fmt::format(
            "\\cwbend{{{}}} \\cw",
            static_cast<int>(q) - static_cast<int>(n_qubits_ + r)
    );
}

void CircuitDrawer::I(std::size_t q) {
    auto& col = FindVacantColumnAndOccupy(q, q);
    col[q] = "\\gate{I}";
}

void CircuitDrawer::X(std::size_t q) {
    auto& col = FindVacantColumnAndOccupy(q, q);
    col[q] = "\\targ{}";
}

void CircuitDrawer::Y(std::size_t q) {
    auto& col = FindVacantColumnAndOccupy(q, q);
    col[q] = "\\gate{Y}";
}

void CircuitDrawer::Z(std::size_t q) {
    auto& col = FindVacantColumnAndOccupy(q, q);
    col[q] = "\\gate{Z}";
}

void CircuitDrawer::H(std::size_t q) {
    auto& col = FindVacantColumnAndOccupy(q, q);
    col[q] = "\\gate{H}";
}

void CircuitDrawer::S(std::size_t q) {
    auto& col = FindVacantColumnAndOccupy(q, q);
    col[q] = "\\gate{S}";
}

void CircuitDrawer::SDag(std::size_t q) {
    auto& col = FindVacantColumnAndOccupy(q, q);
    col[q] = "\\gate{S^\\dagger}";
}

void CircuitDrawer::T(std::size_t q) {
    auto& col = FindVacantColumnAndOccupy(q, q);
    col[q] = "\\gate{T}";
}

void CircuitDrawer::TDag(std::size_t q) {
    auto& col = FindVacantColumnAndOccupy(q, q);
    col[q] = "\\gate{T^\\dagger}";
}

void CircuitDrawer::RX(std::size_t q, double theta) {
    auto& col = FindVacantColumnAndOccupy(q, q);
    col[q] = fmt::format("\\gate{{RX({})}}", AngleToString(theta));
}

void CircuitDrawer::RY(std::size_t q, double theta) {
    auto& col = FindVacantColumnAndOccupy(q, q);
    col[q] = fmt::format("\\gate{{RY({})}}", AngleToString(theta));
}

void CircuitDrawer::RZ(std::size_t q, double theta) {
    auto& col = FindVacantColumnAndOccupy(q, q);
    col[q] = fmt::format("\\gate{{RZ({})}}", AngleToString(theta));
}

void CircuitDrawer::CX(std::size_t t, std::size_t c) {
    const auto [mn, mx] = std::minmax(t, c);
    auto& col = FindVacantColumnAndOccupy(mn, mx);
    col[t] = "\\targ{}";
    col[c] = fmt::format("\\ctrl{{{}}}", static_cast<int>(t) - static_cast<int>(c));
}

void CircuitDrawer::CY(std::size_t t, std::size_t c) {
    const auto [mn, mx] = std::minmax(t, c);
    auto& col = FindVacantColumnAndOccupy(mn, mx);
    col[t] = "\\gate{Y}";
    col[c] = fmt::format("\\ctrl{{{}}}", static_cast<int>(t) - static_cast<int>(c));
}

void CircuitDrawer::CZ(std::size_t t, std::size_t c) {
    const auto [mn, mx] = std::minmax(t, c);
    auto& col = FindVacantColumnAndOccupy(mn, mx);
    col[t] = "\\gate{Z}";
    col[c] = fmt::format("\\ctrl{{{}}}", static_cast<int>(t) - static_cast<int>(c));
}

void CircuitDrawer::CCX(std::size_t t, std::size_t c0, std::size_t c1) {
    auto targets = std::array{t, c0, c1};
    std::ranges::sort(targets);
    auto& col = FindVacantColumnAndOccupy(targets[0], targets[2]);
    col[t] = "\\targ{}";
    for (const auto idx : {std::size_t{0}, std::size_t{2}}) {
        if (targets[idx] != t) {
            col[targets[idx]] = fmt::format(
                    "\\ctrl{{{}}}",
                    static_cast<int>(t) - static_cast<int>(targets[idx])
            );
        }
    }
    if (targets[1] != t) {
        col[targets[1]] = "\\control \\qw";
    }
}

void CircuitDrawer::CCY(std::size_t t, std::size_t c0, std::size_t c1) {
    auto targets = std::array{t, c0, c1};
    std::ranges::sort(targets);
    auto& col = FindVacantColumnAndOccupy(targets[0], targets[2]);
    col[t] = "\\gate{Y}";
    for (const auto idx : {std::size_t{0}, std::size_t{2}}) {
        if (targets[idx] != t) {
            col[targets[idx]] = fmt::format(
                    "\\ctrl{{{}}}",
                    static_cast<int>(t) - static_cast<int>(targets[idx])
            );
        }
    }
    if (targets[1] != t) {
        col[targets[1]] = "\\control \\qw";
    }
}

void CircuitDrawer::CCZ(std::size_t t, std::size_t c0, std::size_t c1) {
    auto targets = std::array{t, c0, c1};
    std::ranges::sort(targets);
    auto& col = FindVacantColumnAndOccupy(targets[0], targets[2]);
    col[t] = "\\gate{Z}";
    for (const auto idx : {std::size_t{0}, std::size_t{2}}) {
        if (targets[idx] != t) {
            col[targets[idx]] = fmt::format(
                    "\\ctrl{{{}}}",
                    static_cast<int>(t) - static_cast<int>(targets[idx])
            );
        }
    }
    if (targets[1] != t) {
        col[targets[1]] = "\\control \\qw";
    }
}

void CircuitDrawer::MCX(std::size_t t, const std::vector<std::size_t>& c) {
    auto targets = c;
    targets.push_back(t);
    std::ranges::sort(targets);
    auto& col = FindVacantColumnAndOccupy(targets.front(), targets.back());
    col[t] = "\\targ{}";
    for (const auto q : {targets.front(), targets.back()}) {
        if (q != t) {
            col[q] = fmt::format("\\ctrl{{{}}}", static_cast<int>(t) - static_cast<int>(q));
        }
    }
    for (const auto q : std::span(targets).subspan(1, targets.size() - 2)) {
        if (q != t) {
            col[q] = "\\control \\qw";
        }
    }
    if (targets[1] != t) {
        col[targets[1]] = "\\control \\qw";
    }
}

void CircuitDrawer::RotateGlobalPhase(double theta) {
    auto& col = FindVacantColumnAndOccupy(0, 0);
    col[0] = fmt::format("\\phase{{{}}}", AngleToString(theta));
}

void CircuitDrawer::Call(
        std::string_view name,
        const std::vector<std::size_t>& op,
        const std::vector<std::size_t>& in,
        const std::vector<size_t>& out
) {
    std::string escaped_name = EscapeLabel(name);
    const auto [op_mn, op_mx] = std::ranges::minmax_element(op);
    const auto [in_mn, in_mx] = std::ranges::minmax_element(in);
    const auto [out_mn, out_mx] = std::ranges::minmax_element(out);
    const auto mn = op_mn != op.end() ? *op_mn
            : in_mn != in.end()       ? n_qubits_ + *out_mn
            : out_mn != out.end()     ? n_qubits_ + *in_mn
                                      : n_qubits_ + std::min(*in_mn, *out_mn);
    const auto mx = in_mx == in.end() && out_mx == out.end() ? *op_mx
            : in_mx == in.end()                              ? n_qubits_ + *out_mx
            : out_mx == out.end()                            ? n_qubits_ + *in_mx
                                  : n_qubits_ + std::max(*in_mx, *out_mx);
    auto& col = FindVacantColumnAndOccupy(mn, mx);
    auto wires_str = std::string("{");
    wires_str.reserve((2 * (mx - mn)) + 3);
    for (const auto i : std::views::iota(std::size_t{0}, mx - mn + 1)) {
        if (i != 0) {
            wires_str += ',';
        }
        wires_str += std::to_string(i + 1);
    }
    wires_str += '}';
    col[mn] = fmt::format(
            "\\gate[wires={},nwires={}][{:.2f}cm]{{{}}} {}",
            mx - mn + 1,
            wires_str,
            (static_cast<double>(name.length()) * .25) + 2.,
            escaped_name,
            mn < n_qubits_ ? "\\qw" : "\\cw"
    );
    for (const auto i : std::views::iota(std::size_t{0}, op.size())) {
        col[op[i]] += fmt::format(" \\gateinput{{op[{}]}}", i);
    }
    for (const auto i : std::views::iota(std::size_t{0}, in.size())) {
        col[n_qubits_ + in[i]] += fmt::format(" \\gateinput{{in[{}]}}", i);
    }
    for (const auto i : std::views::iota(std::size_t{0}, out.size())) {
        col[n_qubits_ + out[i]] += fmt::format(" \\gateoutput{{out[{}]}}", i);
    }
}

void CircuitDrawer::ClassicalFunction(
        std::string_view name,
        const std::vector<std::size_t>& in,
        const std::vector<size_t>& out
) {
    const auto escaped_name = EscapeLabel(name);
    const auto [in_mn, in_mx] = std::ranges::minmax_element(in);
    const auto [out_mn, out_mx] = std::ranges::minmax_element(out);
    const auto mn = in_mn == in.end() ? *out_mn
                                      : (out_mn == out.end() ? *in_mn : std::min(*in_mn, *out_mn));
    const auto mx = in_mx == in.end() ? *out_mx
                                      : (out_mx == out.end() ? *in_mx : std::max(*in_mx, *out_mx));
    auto& col = FindVacantColumnAndOccupy(n_qubits_ + mn, n_qubits_ + mx);

    auto wires_str = std::string("{");
    wires_str.reserve((2 * (mx - mn)) + 3);
    for (const auto i : std::views::iota(std::size_t{0}, mx - mn + 1)) {
        if (i != 0) {
            wires_str += ',';
        }
        wires_str += std::to_string(i + 1);
    }
    wires_str += '}';

    col[n_qubits_ + mn] = fmt::format(
            "\\gate[wires={},nwires={}][{:.2f}cm]{{{}}} \\cw",
            mx - mn + 1,
            wires_str,
            (static_cast<double>(name.length()) * 0.25) + 2.0,
            escaped_name
    );
    for (const auto i : std::views::iota(std::size_t{0}, in.size())) {
        col[n_qubits_ + in[i]] += fmt::format(" \\gateinput{{in[{}]}}", i);
    }
    for (const auto i : std::views::iota(std::size_t{0}, out.size())) {
        col[n_qubits_ + out[i]] += fmt::format(" \\gateoutput{{out[{}]}}", i);
    }
}

void CircuitDrawer::CRand(
        const std::vector<double>& weights,
        const std::vector<std::size_t>& regs,
        const std::size_t num_per_line
) {
    if (num_per_line == 0) {
        throw std::runtime_error("'num_per_line' must be positive.");
    }

    const auto [min_itr, max_itr] = std::ranges::minmax_element(regs);
    const auto mn = *min_itr;
    const auto mx = *max_itr;
    auto& col = FindVacantColumnAndOccupy(n_qubits_ + mn, n_qubits_ + mx);

    auto wires_str = std::string("{");
    wires_str.reserve((2 * (mx - mn)) + 3);
    for (const auto i : std::views::iota(std::size_t{0}, mx - mn + 1)) {
        if (i != 0) {
            wires_str += ',';
        }
        wires_str += std::to_string(i + 1);
    }
    wires_str += '}';

    auto box_str = std::string(R"(\shortstack{\text{CRand} \\ )");
    auto max_size = std::size_t{5};
    for (auto i = std::size_t{0}; i < weights.size();) {
        auto box_line = std::string();
        for (auto j = std::size_t{0}; j < num_per_line; ++j) {
            if (i < weights.size()) {
                const auto w = weights[i++];
                box_line += fmt::format("{:.2f}", w);
            }
            if (i != weights.size()) {
                box_line += ",";
            }
        }

        max_size = std::max(max_size, box_line.size());
        box_str += " \\\\ " + box_line;
    }
    box_str += "}";

    col[n_qubits_ + mn] = fmt::format(
            "\\gate[wires={},nwires={}][{:.2f}cm]{{{}}} \\cw",
            mx - mn + 1,
            wires_str,
            (static_cast<double>(max_size) * 0.20) + 1.0,
            box_str
    );
    for (const auto r : std::views::iota(std::size_t{0}, regs.size())) {
        col[n_qubits_ + regs[r]] += fmt::format(" \\gateinput{{regs[{}]}}", r);
    }
}

void CircuitDrawer::NewColumn() {
    auto new_column = std::vector<std::string>(n_qubits_ + n_registers_);
    const auto span = std::span(new_column);
    std::ranges::fill(span.subspan(0, n_qubits_), "\\qw");
    std::ranges::fill(span.subspan(n_qubits_, n_registers_), "\\cw");
    content_.push_back(new_column);
}

std::size_t CircuitDrawer::VacantColumnId(std::size_t l, std::size_t r) {
    const auto c = *std::ranges::max_element(std::span(next_place_).subspan(l, r - l + 1));
    if (c == content_.size()) {
        NewColumn();
    }
    return c;
}

std::vector<std::string>& CircuitDrawer::FindVacantColumnAndOccupy(std::size_t l, std::size_t r) {
    const auto c = VacantColumnId(l, r);
    std::ranges::fill(std::span(next_place_).subspan(l, r - l + 1), c + 1);
    return content_[c];
}

std::string CircuitDrawer::AngleToString(double angle) {
    if (std::abs(angle) < 1e-9) {
        return "0";
    }
    angle /= std::numbers::pi;
    for (auto d = uint32_t{1}; d <= 32; d++) {
        auto m = static_cast<int64_t>(std::round(angle * d));
        if (std::abs(angle - (static_cast<double>(m) / d)) < 1e-9) {
            if (d == 1) {
                if (m == 1) {
                    return "\\pi";
                }
                if (m == -1) {
                    return "-\\pi";
                }
                return fmt::format("{}\\pi", m);
            }
            return fmt::format("{}\\frac{{{}}}{{{}}}\\pi", m < 0 ? "-" : "", std::abs(m), d);
        }
    }
    return fmt::format("{}\\pi", angle);
}

std::string CircuitDrawer::EscapeLabel(std::string_view label) {
    auto ret = std::string{};
    ret.reserve(label.size());
    for (char c : label) {
        switch (c) {
            case '&':
            case '%':
            case '$':
            case '#':
            case '_':
            case '{':
            case '}':
                ret += '\\';
                ret += c;
                break;
            case '~':
                ret += "\\textasciitilde\n";
                break;
            case '^':
                ret += "\\textasciicircum\n";
                break;
            case '\\':
                ret += "\\textbackslash\n";
                break;
            default:
                ret += c;
        }
    }
    return ret;
}

}  // namespace qret
