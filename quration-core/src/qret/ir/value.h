/**
 * @file qret/ir/value.h
 * @brief Value.
 */

#ifndef QRET_IR_VALUE_H
#define QRET_IR_VALUE_H

#include <cstdint>
#include <limits>
#include <ostream>

namespace qret::ir {
struct Qubit {
    static Qubit Null() {
        return {std::numeric_limits<std::uint64_t>::max()};
    }
    [[nodiscard]] constexpr std::strong_ordering operator<=>(const Qubit&) const noexcept = default;
    void Print(std::ostream& out) const {
        out << 'q' << id;
    }
    void PrintAsOperand(std::ostream& out) const {
        out << "@q" << id;
    }
    std::uint64_t id;
};

struct Register {
    static Register Null() {
        return {std::numeric_limits<std::uint64_t>::max()};
    }
    [[nodiscard]] constexpr std::strong_ordering operator<=>(const Register&) const noexcept =
            default;
    void Print(std::ostream& out) const {
        out << 'r' << id;
    }
    void PrintAsOperand(std::ostream& out) const {
        out << "@r" << id;
    }
    std::uint64_t id;
};

/**
 * @brief Floating point with precision.
 */
struct FloatWithPrecision {
    void Print(std::ostream& out) const {
        out << value << '(' << precision << ')';
    }
    void PrintAsOperand(std::ostream& out) const {
        out << value << '(' << precision << ')';
    }
    double value;
    double precision;
};
}  // namespace qret::ir

#endif  // QRET_IR_VALUE_H
