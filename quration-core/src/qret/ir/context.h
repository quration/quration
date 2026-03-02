/**
 * @file qret/ir/context.h
 * @brief Context.
 */

#ifndef QRET_IR_CONTEXT_H
#define QRET_IR_CONTEXT_H

#include <memory>
#include <vector>

#include "qret/qret_export.h"

namespace qret::ir {
// Forward declarations
class Module;

/**
 * @brief Context of qret IR.
 * @details This class owns and manages the core "global" data of qret's core infrastructure.
 */
class QRET_EXPORT IRContext {
public:
    IRContext() = default;
    IRContext(const IRContext&) = delete;
    IRContext& operator=(const IRContext&) = delete;

    /**
     * @brief List of modules instantiated in this context, and which will be automatically deleted
     * if this context is deleted
     */
    std::vector<std::unique_ptr<Module>> owned_module;

    // TODO: global constants
};
}  // namespace qret::ir

#endif  // QRET_IR_CONTEXT_H
