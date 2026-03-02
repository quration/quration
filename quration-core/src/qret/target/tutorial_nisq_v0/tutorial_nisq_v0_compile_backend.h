/**
 * @file qret/target/tutorial_nisq_v0/tutorial_nisq_v0_compile_backend.h
 * @brief Compile backend for TUTORIAL_NISQ_V0 target.
 */

#ifndef QRET_TARGET_TUTORIAL_NISQ_V0_TUTORIAL_NISQ_V0_COMPILE_BACKEND_H
#define QRET_TARGET_TUTORIAL_NISQ_V0_TUTORIAL_NISQ_V0_COMPILE_BACKEND_H

#include "qret/target/target_compile_backend.h"

namespace qret::tutorial_nisq_v0 {
/**
 * @brief Compile backend for tutorial target.
 * @details
 * - Input format: IR or OpenQASM2
 * - Internal lowering: IR -> MachineFunction(TUTORIAL_NISQ_V0 instructions)
 * - Output: assembly text emitted by target AsmPrinter
 *
 * This is intentional for tutorial_nisq_v0: compile output is assembly text.
 * In contrast, production backends such as SC_LS_FIXED_V0 use JSON as
 * the primary compile artifact.
 */
class TutorialNisqV0CompileBackend : public qret::TargetCompileBackend {
public:
    [[nodiscard]] std::string_view TargetName() const override;
    void AddCompileOptions(qret::CompileOptionRegistrar& registrar) const override;
    [[nodiscard]] bool Supports(CompileFormat source) const override;
    [[nodiscard]] bool
    Compile(const CompileRequest& request, const qret::CompileOptionReader& options) const override;
};
}  // namespace qret::tutorial_nisq_v0

#endif  // QRET_TARGET_TUTORIAL_NISQ_V0_TUTORIAL_NISQ_V0_COMPILE_BACKEND_H
