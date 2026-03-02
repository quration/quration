/**
 * @file qret/target/target_registry.cpp
 * @brief Target registration.
 */

#include "qret/target/target_registry.h"

#include <iostream>
#include <string_view>

#include "qret/base/string.h"
#include "qret/target/target_compile_backend.h"

namespace qret {
TargetRegistry* TargetRegistry::GetTargetRegistry() {
    static TargetRegistry TargetRegistryObj;
    return &TargetRegistryObj;
}

const Target* TargetRegistry::GetTarget(std::string_view name) const {
    const auto key = GetLowerString(name);
    const auto itr = target_map_.find(key);
    if (itr == target_map_.end()) {
        std::cerr << "target '" << name << "' is not registered" << std::endl;
        return nullptr;
    }
    return itr->second;
}

const Target* TargetRegistry::GetTarget(TargetEnum name) const {
    return GetTarget(name.ToString());
}

const TargetCompileBackend* TargetRegistry::GetCompileBackend(std::string_view target_name) const {
    const auto key = GetLowerString(target_name);
    const auto it = compile_backend_map_.find(key);
    if (it == compile_backend_map_.end()) {
        std::cerr << "target '" << target_name << "' is not registered for compile" << std::endl;
        return nullptr;
    }
    return it->second;
}

void TargetRegistry::RegisterTarget(
        Target* target,
        std::string_view name,
        std::string_view short_desc
) {
    const auto key = GetLowerString(name);
    target_map_[key] = target;
    target->name_ = name;
    target->short_desc_ = short_desc;
}

void TargetRegistry::RegisterCompileBackend(const TargetCompileBackend* backend) {
    const auto key = GetLowerString(backend->TargetName());
    compile_backend_map_[key] = backend;
}
}  // namespace qret
