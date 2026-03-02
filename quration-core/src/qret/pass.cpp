/**
 * @file qret/pass.cpp
 * @brief Base class for passes.
 */

#include "qret/pass.h"

#include <string_view>

#include "qret/codegen/machine_function_pass.h"

namespace qret {
std::string_view Pass::GetPassName() const {
    const auto* const id = GetPassId();
    const auto* const pi = PassRegistry::GetPassRegistry()->GetPassInfo(id);
    if (pi != nullptr) {
        return pi->GetPassName();
    }
    return "unnamed pass: implement Pass::GetPassName()";
}
std::string_view Pass::GetPassArgument() const {
    const auto* const id = GetPassId();
    const auto* const pi = PassRegistry::GetPassRegistry()->GetPassInfo(id);
    if (pi != nullptr) {
        return pi->GetPassArgument();
    }
    return "no arg pass: implement Pass::GetPassArgument()";
}
const PassInfo* Pass::GetPassInfo() const {
    return PassRegistry::GetPassRegistry()->GetPassInfo(pass_id_);
}
PassRegistry* PassRegistry::GetPassRegistry() {
    static PassRegistry PassRegistryObj;
    return &PassRegistryObj;
}
const PassInfo* PassRegistry::GetPassInfo(PassIDType pass_id) const {
    const auto itr = pass_info_map_.find(pass_id);
    return itr == pass_info_map_.end() ? nullptr : itr->second;
}
const PassInfo* PassRegistry::GetPassInfo(std::string_view arg) const {
    const auto itr = pass_info_string_map_.find(arg);
    return itr == pass_info_string_map_.end() ? nullptr : itr->second;
}
bool PassRegistry::Contains(PassIDType pass_id) const {
    return pass_info_map_.contains(pass_id);
}
bool PassRegistry::Contains(std::string_view arg) const {
    return pass_info_string_map_.contains(arg);
}
void PassRegistry::RegisterPass(const PassInfo& pi) {
    pass_info_map_.emplace(pi.GetPassID(), &pi);
    pass_info_string_map_.emplace(pi.GetPassArgument(), &pi);
}
const OptimizationLevel OptimizationLevel::O0 = {
        /*SpeedLevel*/ 0,
        /*SizeLevel*/ 0
};
const OptimizationLevel OptimizationLevel::O1 = {
        /*SpeedLevel*/ 1,
        /*SizeLevel*/ 0
};
const OptimizationLevel OptimizationLevel::O2 = {
        /*SpeedLevel*/ 2,
        /*SizeLevel*/ 0
};
const OptimizationLevel OptimizationLevel::O3 = {
        /*SpeedLevel*/ 3,
        /*SizeLevel*/ 0
};
const OptimizationLevel OptimizationLevel::Os = {
        /*SpeedLevel*/ 2,
        /*SizeLevel*/ 1
};
const OptimizationLevel OptimizationLevel::Oz = {
        /*SpeedLevel*/ 2,
        /*SizeLevel*/ 2
};
MFPassManager PassBuilder::BuildMFDefaultPipeline(const OptimizationLevel&) const {
    auto manager = MFPassManager();
    const auto add_pass = [&manager](std::string_view arg) {
        const auto* pi = PassRegistry::GetPassRegistry()->GetPassInfo(arg);
        manager.AddPass((pi->GetNormalCtor())());
    };
    add_pass("sc_ls_fixed_v0::mapping");
    add_pass("sc_ls_fixed_v0::routing");
    return manager;
}
}  // namespace qret
