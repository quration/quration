/**
 * @file qret/algorithm/util/write_register.h
 */

#ifndef QRET_GATE_UTIL_WRITE_REGISTER_H
#define QRET_GATE_UTIL_WRITE_REGISTER_H

#include "qret/base/type.h"
#include "qret/frontend/argument.h"
#include "qret/ir/instructions.h"
#include "qret/qret_export.h"

namespace qret::frontend::gate {
//--------------------------------------------------//
// Functions
//--------------------------------------------------//

/**
 * @brief Write a value to registers.
 * @details Set register values as follows:
 * 1. out[0] = 0-th bit of imm
 * 2. out[1] = 1-st bit of imm
 * 3. ... (repeat)
 *
 * out[i] = (imm >> i) & 1
 *
 * @param imm
 * @param out size = n
 */
QRET_EXPORT ir::CallCFInst* WriteRegisters(const BigInt& imm, const Registers& out);
}  // namespace qret::frontend::gate

#endif  // QRET_GATE_UTIL_WRITE_REGISTER_H
