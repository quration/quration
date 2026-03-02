/**
 * @file qret/target/sc_ls_fixed_v0/pipeline_state.h
 * @brief SC_LS_FIXED_V0 pipeline state.
 */

#ifndef QRET_TARGET_SC_LS_FIXED_V0_PIPELINE_STATE_H
#define QRET_TARGET_SC_LS_FIXED_V0_PIPELINE_STATE_H

#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "qret/base/json.h"
#include "qret/codegen/machine_function.h"
#include "qret/codegen/machine_function_pass.h"
#include "qret/qret_export.h"
#include "qret/target/sc_ls_fixed_v0/compile_info.h"
#include "qret/target/sc_ls_fixed_v0/sc_ls_fixed_v0_target_machine.h"

namespace qret::sc_ls_fixed_v0 {
inline constexpr auto PipelineStateJsonSchemaVersion = "0.1";

/**
 * @brief Public DTO of SC_LS_FIXED_V0 pipeline state.
 */
struct QRET_EXPORT ScLsFixedV0PipelineState {
    struct Info {
        std::string description;
    };
    struct Parameter {
        std::optional<ScLsFixedV0TargetMachine> target;
    };
    struct Opt {
        std::vector<std::tuple<std::string, double>> passes;
        std::optional<ScLsFixedV0CompileInfo> compile_info;
    };

    Info info = {};
    Parameter parameter = {};
    Opt opt = {};
    std::vector<Json> program = {};
};

QRET_EXPORT void to_json(Json& j, const ScLsFixedV0PipelineState& state);
QRET_EXPORT void from_json(const Json& j, ScLsFixedV0PipelineState& state);

QRET_EXPORT ScLsFixedV0PipelineState LoadPipelineState(const std::string& input);
QRET_EXPORT void
SavePipelineState(const std::string& output, const ScLsFixedV0PipelineState& state);

QRET_EXPORT ScLsFixedV0PipelineState BuildPipelineState(
        const MFPassManager& manager,
        const std::unique_ptr<ScLsFixedV0TargetMachine>& target,
        const MachineFunction& mf
);
QRET_EXPORT ScLsFixedV0PipelineState BuildPipelineState(const MachineFunction& mf);

QRET_EXPORT void ApplyPipelineState(
        const ScLsFixedV0PipelineState& state,
        MFPassManager& manager,
        std::unique_ptr<ScLsFixedV0TargetMachine>& target,
        MachineFunction& mf
);
QRET_EXPORT void ApplyPipelineState(const ScLsFixedV0PipelineState& state, MachineFunction& mf);
}  // namespace qret::sc_ls_fixed_v0

#endif  // QRET_TARGET_SC_LS_FIXED_V0_PIPELINE_STATE_H
