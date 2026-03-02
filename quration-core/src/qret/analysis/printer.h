/**
 * @file qret/analysis/printer.h
 * @brief Print a func in pretty way.
 */

#ifndef QRET_ANALYSIS_PRINT_H
#define QRET_ANALYSIS_PRINT_H

#include <string>

#include "qret/ir/basic_block.h"
#include "qret/ir/function.h"
#include "qret/ir/instruction.h"
#include "qret/qret_export.h"

namespace qret {
/**
 * @brief Get the string of bb.
 */
QRET_EXPORT std::string ToString(const ir::BasicBlock& bb, std::uint32_t inst_indent = 2);

/**
 * @brief Get the string of func.
 */
QRET_EXPORT std::string ToString(
        const ir::Function& func,
        std::uint32_t bb_indent = 2,
        std::uint32_t inst_indent = 4,
        bool show_bb_dep = false
);
}  // namespace qret

#endif  // QRET_ANALYSIS_PRINT_H
