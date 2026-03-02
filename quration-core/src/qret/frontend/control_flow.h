/**
 * @file qret/frontend/control_flow.h
 * @brief Manipulators of control flow in quantum circuits.
 * @details This file implements functions that manipulate control flow in quantum circuits.
 */

#ifndef QRET_FRONTEND_CONTROL_FLOW_H
#define QRET_FRONTEND_CONTROL_FLOW_H

#include "qret/frontend/argument.h"
#include "qret/qret_export.h"

namespace qret::frontend::control_flow {
/**
 * @brief Insert a branching instruction into the circuit currently being defined in the
 * CircuitBuilder.
 * @details If you call If(r), you must always call EndIf(r).
 * How to use:
 *
 * Case1:
 * @code{.cpp}
 * If(r);
 * // define a circuit for the case when r == 1
 * Else(r);
 * // define a circuit for the case when r == 0
 * EndIf(r);
 * @endcode
 *
 * Case2:
 * @code {.cpp}
 * If(r);
 * // define a circuit for the case when r == 1
 * EndIf(r);
 * @endcode
 *
 * @param cond condition register
 */
QRET_EXPORT void If(const Register& cond);
QRET_EXPORT void If(const Register& cond, std::string branch_name);
/**
 * @brief Insert a branching instruction into the circuit.
 * @details See `If` documents for more details.
 *
 * @param cond condition register
 */
QRET_EXPORT void Else(const Register& cond);
/**
 * @brief Insert a branching instruction into the circuit.
 * @details See `If` documents for more details.
 *
 * @param cond condition register
 */
QRET_EXPORT void EndIf(const Register& cond);

/**
 * @brief Begin a multi-way branch on the given register value.
 * @details Starts a switch block. Within a switch block you must add two or more `Case(value, key)`
 * branches, exactly one `Default(value)` branch, and terminate the block with `EndSwitch(value)`.
 * Keys in `Case` must be unique and fit within the bit-width of `value`.
 *
 * How to use:
 *
 * Case1 (typical):
 * @code{.cpp}
 * Switch(v);                 // v encodes the branch key
 *   Case(v, 0);              // when v == 0
 *     // ...
 *   Case(v, 1);              // when v == 1
 *     // ...
 *   Default(v);              // otherwise
 *     // ...
 * EndSwitch(v);
 * @endcode
 *
 * Case2 (more cases, nested allowed):
 * @code{.cpp}
 * Switch(v, "outer");
 *   Case(v, 3) {  }
 *   Case(v, 7) {
 *     // You may nest another Switch inside a Case block
 *     Switch(w, "inner");
 *       Case(w, 1) {  }
 *       Case(w, 2) {  }
 *       Default(w) { }
 *     EndSwitch(w);
 *   }
 *   Default(v) {  }
 * EndSwitch(v);
 * @endcode
 *
 * @pre Call `Switch(value)` before any corresponding `Case(value, ...)` / `Default(value)` /
 * `EndSwitch(value)`.
 * @pre Call at least two `Case(value, key)` before `Default(value)`.
 * @pre Call exactly one `Default(value)` before `EndSwitch(value)`.
 * @post Control flow continues after `EndSwitch(value)`.
 *
 * @note Keys are compared against the bit pattern of `value` interpreted as an unsigned integer.
 * The key must be representable with the total bit-width of `value`; otherwise this function
 * sequence will fail at run time.
 * @warning Calling `Case` after `Default`, omitting `Default`, using duplicate keys, or mismatching
 * the `value` object across the block is an undefined behavior.
 *
 * @param value register(s) whose bits form the branch key.
 */
QRET_EXPORT void Switch(const Registers& value);
/**
 * @brief Begin a multi-way branch on the given register value, with a label.
 * @details Same as `Switch(value)`, but attaches a human-readable label to the
 *          switch block (useful for debugging, tracing, or IR dumps).
 *
 * @param value register(s) whose bits form the branch key.
 * @param branch_name optional label for this switch block (should be unique
 *                    among sibling blocks for readability).
 */
QRET_EXPORT void Switch(const Registers& value, std::string branch_name);
/**
 * @brief Add a case arm to the current switch block.
 * @details Introduces a branch that is taken when `value` equals `key`. May be called multiple
 * times (keys must be unique). Must appear after the matching `Switch(value)` and before
 * `Default(value)`.
 *
 * @param value the same register(s) passed to the corresponding `Switch`.
 * @param key   non-negative integer key to match; must fit within the bit-width of `value` and be
 * unique among cases in this block.
 */
QRET_EXPORT void Case(const Registers& value, std::uint64_t key);
/**
 * @brief Add the default arm to the current switch block.
 * @details Introduces the fallback branch taken when none of the `Case` keys match. Exactly one
 * `Default(value)` is required in each switch block, and it must appear before `EndSwitch(value)`.
 *
 * @param value the same register(s) passed to the corresponding `Switch`.
 */
QRET_EXPORT void Default(const Registers& value);
/**
 * @brief End the current switch block.
 * @details Closes the switch construct that was opened by `Switch(value)`. This must be called
 * after adding at least two `Case(value, key)` arms and exactly one `Default(value)`.
 *
 * @param value the same register(s) passed to the corresponding `Switch`.
 */
QRET_EXPORT void EndSwitch(const Registers& value);
}  // namespace qret::frontend::control_flow

#endif  // QRET_FRONTEND_CONTROL_FLOW_H
