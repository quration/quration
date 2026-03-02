/**
 * @file tribool.h
 * @brief Defines a 3-valued logic (Tribool) system optimized for bitwise operations.
 *
 * @details
 * This module implements Kleene's 3-valued logic using a specific bitwise representation
 * to maximize efficiency on modern CPUs.
 *
 * The logic values are mapped as follows:
 * - **False**:   0 (binary `00`)
 * - **Unknown**: 1 (binary `01`)
 * - **True**:    3 (binary `11`)
 *
 * **Why this mapping?**
 * This encoding allows the standard bitwise hardware instructions AND (`&`) and OR (`|`)
 * to naturally satisfy the truth tables for 3-valued logic without any branching.
 * - False is "strong" in AND (00 & 01 -> 00).
 * - True is "strong" in OR (11 | 01 -> 11).
 *
 * XOR and NOT require special handling but remain efficient.
 */

#ifndef QRET_BASE_TRIBOOL_H
#define QRET_BASE_TRIBOOL_H

#include <cstdint>
#include <iostream>
#include <string>

#include "qret/base/string.h"

namespace qret {

/**
 * @enum Tribool
 * @brief Strongly typed enumeration for 3-valued logic.
 */
enum class Tribool : std::uint8_t {
    False = 0,  ///< Represents Logical False (0b00)
    Unknown = 1,  ///< Represents Unknown/Indeterminate state (0b01)
    True = 3  ///< Represents Logical True (0b11)
};

/**
 * @brief Casts a Tribool to its underlying integer representation.
 * @param t The Tribool value.
 * @return std::uint8_t The integer value (0, 1, or 3).
 */
constexpr std::uint8_t Val(Tribool t) {
    return static_cast<std::uint8_t>(t);
}

/**
 * @brief Casts an integer to a Tribool.
 * @note Assumes input is a valid Tribool value (0, 1, or 3).
 * @param v The integer value.
 * @return Tribool The corresponding Tribool value.
 */
constexpr Tribool ToTri(std::uint8_t v) {
    return static_cast<Tribool>(v);
}

/**
 * @brief Logical AND operator for Tribool.
 *
 * Uses bitwise AND directly:
 * - T & U -> 11 & 01 = 01 (Unknown)
 * - F & U -> 00 & 01 = 00 (False)
 *
 * @param lhs Left-hand side operand.
 * @param rhs Right-hand side operand.
 * @return Tribool Result of the AND operation.
 */
constexpr Tribool operator&(Tribool lhs, Tribool rhs) {
    return ToTri(Val(lhs) & Val(rhs));
}

/**
 * @brief Logical OR operator for Tribool.
 *
 * Uses bitwise OR directly:
 * - T | U -> 11 | 01 = 11 (True)
 * - F | U -> 00 | 01 = 01 (Unknown)
 *
 * @param lhs Left-hand side operand.
 * @param rhs Right-hand side operand.
 * @return Tribool Result of the OR operation.
 */
constexpr Tribool operator|(Tribool lhs, Tribool rhs) {
    return ToTri(Val(lhs) | Val(rhs));
}

/**
 * @brief Logical NOT operator for Tribool.
 *
 * Inverts the value:
 * - !False   -> True
 * - !True    -> False
 * - !Unknown -> Unknown
 *
 * @param t The operand to negate.
 * @return Tribool The negated value.
 */
constexpr Tribool operator!(Tribool t) {
    // Lookup table for negation:
    // Index 0 (False)   -> True (3)
    // Index 1 (Unknown) -> Unknown (1)
    // Index 2 (Unused)  -> Unknown (1) - fallback
    // Index 3 (True)    -> False (0)
    constexpr Tribool NotTable[4] =
            {Tribool::True, Tribool::Unknown, Tribool::Unknown, Tribool::False};
    return NotTable[Val(t)];
}

/**
 * @brief Logical XOR operator for Tribool.
 *
 * Standard bitwise XOR fails for 3-valued logic (e.g., 1^1=0).
 * This implementation explicitly handles the Unknown state.
 * If either operand is Unknown, the result is Unknown.
 *
 * @param lhs Left-hand side operand.
 * @param rhs Right-hand side operand.
 * @return Tribool Result of the XOR operation.
 */
constexpr Tribool operator^(Tribool lhs, Tribool rhs) {
    // Kleene logic: If any input is Unknown, the result of XOR is Unknown.
    if (lhs == Tribool::Unknown || rhs == Tribool::Unknown) {
        return Tribool::Unknown;
    }
    // Otherwise, standard XOR applies (0^0=0, 3^3=0, 0^3=3)
    return ToTri(Val(lhs) ^ Val(rhs));
}

inline std::string ToString(Tribool t) {
    switch (t) {
        case Tribool::False:
            return "False";
        case Tribool::True:
            return "True";
        case Tribool::Unknown:
            return "Unknown";
            break;
        default:
            return "Invalid(" + std::to_string(static_cast<std::int32_t>(Val(t))) + ")";
    }
}
inline Tribool TriboolFromString(const std::string& s) {
    const auto l = GetLowerString(s);
    if (l == "false") {
        return Tribool::False;
    } else if (l == "true") {
        return Tribool::True;
    }
    return Tribool::Unknown;
}

/**
 * @brief Stream insertion operator for printing Tribool values.
 * @param out The output stream.
 * @param t The Tribool value.
 * @return std::ostream& Reference to the output stream.
 */
inline std::ostream& operator<<(std::ostream& out, Tribool t) {
    out << ToString(t);
    return out;
}

}  // namespace qret

#endif  // QRET_BASE_TRIBOOL_H
