/**
 * @file qret/target/sc_ls_fixed_v0/sc_ls_fixed_v0_compile_backend.h
 * @brief Compile backend for SC_LS_FIXED_V0 target.
 */

#ifndef QRET_TARGET_SC_LS_FIXED_V0_SC_LS_FIXED_V0_COMPILE_BACKEND_H
#define QRET_TARGET_SC_LS_FIXED_V0_SC_LS_FIXED_V0_COMPILE_BACKEND_H

#include "qret/target/target_compile_backend.h"

namespace qret::sc_ls_fixed_v0 {
class ScLsFixedV0CompileBackend : public qret::TargetCompileBackend {
public:
    [[nodiscard]] std::string_view TargetName() const override;
    void AddCompileOptions(qret::CompileOptionRegistrar& registrar) const override;
    [[nodiscard]] bool Supports(CompileFormat source) const override;
    [[nodiscard]] bool
    Compile(const CompileRequest& request, const qret::CompileOptionReader& options) const override;
};
}  // namespace qret::sc_ls_fixed_v0

#endif  // QRET_TARGET_SC_LS_FIXED_V0_SC_LS_FIXED_V0_COMPILE_BACKEND_H
