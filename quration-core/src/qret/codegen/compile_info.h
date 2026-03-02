/**
 * @file qret/codegen/compile_info.h
 * @brief Information of compilation.
 * @details This file defines a data class that accumulates information at compile time.
 */

#ifndef QRET_CODEGEN_COMPILE_INFO_H
#define QRET_CODEGEN_COMPILE_INFO_H

#include <string>

#include "qret/base/json.h"
#include "qret/qret_export.h"

namespace qret {
struct QRET_EXPORT CompileInfo {
    virtual ~CompileInfo() = default;

    virtual ::qret::Json Json() const = 0;
    virtual std::string Markdown() const = 0;
};
}  // namespace qret

#endif  // QRET_TARGET_COMPILE_INFO_H
