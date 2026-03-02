/**
 * @file qret/target/tutorial_nisq_v0/target_info/tutorial_nisq_v0_target_info.cpp
 * @brief TUTORIAL_NISQ_V0 target registration.
 */

#include "qret/target/tutorial_nisq_v0/target_info/tutorial_nisq_v0_target_info.h"

#include "qret/target/target_registry.h"
#include "qret/target/tutorial_nisq_v0/tutorial_nisq_v0_compile_backend.h"

namespace qret::tutorial_nisq_v0 {
Target* GetTutorialNisqV0Target() {
    static Target tutorial_target;
    return &tutorial_target;
}

namespace {
TutorialNisqV0CompileBackend& GetTutorialNisqV0CompileBackend() {
    static auto backend = TutorialNisqV0CompileBackend();
    return backend;
}

const auto kTutorialNisqV0Registered = []() {
    auto* registry = TargetRegistry::GetTargetRegistry();
    registry->RegisterTarget(GetTutorialNisqV0Target(), "tutorial_nisq_v0", "Tutorial NISQ target");
    registry->RegisterCompileBackend(&GetTutorialNisqV0CompileBackend());
    return true;
}();
}  // namespace
}  // namespace qret::tutorial_nisq_v0
