/**
 * @file qret/target/sc_ls_fixed_v0/constants.h
 * @brief Constants.
 */

#ifndef QRET_TARGET_SC_LS_FIXED_V0_CONSTANTS_H
#define QRET_TARGET_SC_LS_FIXED_V0_CONSTANTS_H

#include <cstdint>

#include "qret/target/sc_ls_fixed_v0/symbol.h"

namespace qret::sc_ls_fixed_v0 {
inline constexpr auto AsmHeaderSchemaVersion = "0.1";

/**
 * @brief The number of reserved csymbols.
 * @details
 *
 * - CSymbol{0} : always 0
 * - CSymbol{1} : always 1
 * - CSymbol{2} - CSymbol{9} : reserved for future use.
 */
inline constexpr auto NumReservedCSymbols = std::uint64_t{10};
/**
 * @brief C0 is always 0.
 */
inline constexpr auto C0 = CSymbol{0};
/**
 * @brief C1 is always 1.
 */
inline constexpr auto C1 = CSymbol{1};
}  // namespace qret::sc_ls_fixed_v0

#endif  // QRET_TARGET_SC_LS_FIXED_V0_CONSTANTS_H
