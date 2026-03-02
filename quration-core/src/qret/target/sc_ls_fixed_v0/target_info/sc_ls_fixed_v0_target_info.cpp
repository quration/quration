/**
 * @file qret/target/sc_ls_fixed_v0/target_info/sc_ls_fixed_v0_target_info.cpp
 * @brief SC_LS_FIXED_V0 target implementation.
 */

#include "qret/target/sc_ls_fixed_v0/target_info/sc_ls_fixed_v0_target_info.h"

#include "qret/target/sc_ls_fixed_v0/sc_ls_fixed_v0_compile_backend.h"
#include "qret/target/target_enum.h"
#include "qret/target/target_registry.h"

namespace qret::sc_ls_fixed_v0 {
Target* GetSC_LS_FIXED_V0Target() {
    static Target ScLsFixedV0TargetObj;
    return &ScLsFixedV0TargetObj;
}
namespace {
ScLsFixedV0CompileBackend& GetScLsFixedV0CompileBackend() {
    static auto backend = ScLsFixedV0CompileBackend();
    return backend;
}

const auto kScLsFixedV0Registered = []() {
    auto* registry = TargetRegistry::GetTargetRegistry();
    registry->RegisterTarget(
            GetSC_LS_FIXED_V0Target(),
            TargetEnum::SCLSFixedV0().ToString(),
            "SC_LS_FIXED_V0 target"
    );
    registry->RegisterCompileBackend(&GetScLsFixedV0CompileBackend());
    return true;
}();
}  // namespace
}  // namespace qret::sc_ls_fixed_v0
