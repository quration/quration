/**
 * @file qret/target/target_compile_backend.h
 * @brief Interface of compile backend for each target.
 */

#ifndef QRET_TARGET_TARGET_COMPILE_BACKEND_H
#define QRET_TARGET_TARGET_COMPILE_BACKEND_H

#include <string_view>

#include "qret/qret_export.h"
#include "qret/target/compile_backend_context.h"
#include "qret/target/compile_request.h"

namespace qret {
/**
 * @brief Compile backend interface for each target.
 * @details This interface is intentionally independent from cmd/boost types.
 */
class QRET_EXPORT TargetCompileBackend {
public:
    virtual ~TargetCompileBackend() = default;

    [[nodiscard]] virtual std::string_view TargetName() const = 0;
    virtual void AddCompileOptions(CompileOptionRegistrar& registrar) const = 0;
    [[nodiscard]] virtual bool Supports(CompileFormat source) const = 0;
    [[nodiscard]] virtual bool
    Compile(const CompileRequest& request, const CompileOptionReader& options) const = 0;
};
}  // namespace qret

#endif  // QRET_TARGET_TARGET_COMPILE_BACKEND_H
