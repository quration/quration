/**
 * @file qret/frontend/attribute.h
 * @brief Attributes of qubits.
 * @details This file implements methods to set attributes on qubits.
 */

#ifndef QRET_FRONTEND_ATTRIBUTE_H
#define QRET_FRONTEND_ATTRIBUTE_H

#include <string_view>

#include "qret/frontend/argument.h"
#include "qret/qret_export.h"

namespace qret::frontend::attribute {
QRET_EXPORT void MarkAsClean(const Qubit& q);
QRET_EXPORT void MarkAsClean(const Qubit& q, std::string_view file, std::uint32_t line);
QRET_EXPORT void SetCleanProb(const Qubit& q, double clean_prob, double precision);
QRET_EXPORT void SetCleanProb(
        const Qubit& q,
        double clean_prob,
        double precision,
        std::string_view file,
        std::uint32_t line
);
QRET_EXPORT void MarkAsDirtyBegin(const Qubit& q);
QRET_EXPORT void MarkAsDirtyBegin(const Qubit& q, std::string_view file, std::uint32_t line);
QRET_EXPORT void MarkAsDirtyEnd(const Qubit& q);
QRET_EXPORT void MarkAsDirtyEnd(const Qubit& q, std::string_view file, std::uint32_t line);
}  // namespace qret::frontend::attribute

/**
 * @brief Marks a qubit q is clean.
 * @details Defining a qubit q to be clean means that the state of qubit is |0>.
 */
#define MARK_AS_CLEAN(q) ::qret::frontend::attribute::MarkAsClean((q), __FILE__, __LINE__);

/**
 * @brief Sets the probability of observing |0> when measuring a qubit.
 */
#define SET_CLEAN_PROB(q, clean_prob, precision)                         \
    ::qret::frontend::attribute::SetCleanProb(                           \
            (q),                                                         \
            (::qret::ir::FloatWithPrecision{(clean_prob), (precision)}), \
            __FILE__,                                                    \
            __LINE__                                                     \
    );

/**
 * @brief Marks the initiation of using a qubit as a dirty ancilla.
 */
#define MARK_AS_DIRTY_BEGIN(q) \
    ::qret::frontend::attribute::MarkAsDirtyBegin((q), __FILE__, __LINE__);

/**
 * @brief Marks the completion of using a qubit as a dirty ancilla.
 */
#define MARK_AS_DIRTY_END(q) ::qret::frontend::attribute::MarkAsDirtyEnd((q), __FILE__, __LINE__);

#endif  // QRET_FRONTEND_ATTRIBUTE_H
