/**
 * @file qret/error.h
 * @brief Error code in QRET library.
 */

#ifndef QRET_ERROR_H
#define QRET_ERROR_H

#include <cstdint>

namespace qret::error {
// IR
static constexpr std::uint32_t InvalidIR = 100;

// SC_LS_FIXED_V0
static constexpr std::uint32_t ReAllocateSymbol = 1000;
static constexpr std::uint32_t UseUnallocatedSymbol = 1001;
}  // namespace qret::error

#endif  // QRET_ERROR_H
