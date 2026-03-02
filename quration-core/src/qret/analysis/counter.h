/**
 * @file qret/analysis/counter.h
 * @brief Count the number of instructions in a func.
 */

#ifndef QRET_ANALYSIS_COUNT_H
#define QRET_ANALYSIS_COUNT_H

#include <map>
#include <ostream>

#include "qret/ir/basic_block.h"
#include "qret/ir/function.h"
#include "qret/ir/instruction.h"
#include "qret/qret_export.h"

namespace qret {
struct QRET_EXPORT InstCount {
    std::size_t num_bbs = 0;
    std::size_t num_instructions = 0;

    /**
     * @brief Counter. Key: (opcode, pointer to callee if opcode is Call, otherwise nullptr), Value:
     * the number of instructions
     */
    std::map<std::pair<ir::Opcode::Table, const ir::Function*>, std::size_t> counter = {};

    /**
     * @brief Print InstCount class.
     *
     * @param out
     * @param ignore_callee If true, ignore the callee and count as a call instruction.
     */
    void Print(std::ostream& out, bool ignore_callee = false) const;
};

/**
 * @brief Count the number of instructions in a bb.
 */
QRET_EXPORT InstCount CountInst(const ir::BasicBlock& bb);

/**
 * @brief Count the number of instructions in a func.
 */
QRET_EXPORT InstCount CountInst(const ir::Function& func);
}  // namespace qret

#endif  // QRET_ANALYSIS_COUNT_H
