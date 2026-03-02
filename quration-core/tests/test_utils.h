#pragma once

#include <fstream>
#include <string>

#include "qret/base/json.h"
#include "qret/ir/context.h"
#include "qret/ir/function.h"
#include "qret/ir/json.h"
#include "qret/ir/module.h"

namespace qret::tests {
inline Json LoadJsonFromFile(const std::string& path) {
    auto ifs = std::ifstream(path);
    return Json::parse(ifs);
}

inline void LoadContextFromJsonFile(const std::string& path, ir::IRContext& context) {
    auto j = LoadJsonFromFile(path);
    ir::LoadJson(j, context);
}

inline ir::Function* LoadFunctionFromJsonFile(
        const std::string& path,
        const std::string& function_name,
        ir::IRContext& context
) {
    LoadContextFromJsonFile(path, context);
    const auto* module = context.owned_module.back().get();
    return module->GetFunction(function_name);
}

inline ir::Function* LoadCircuitFromJsonFile(
        const std::string& path,
        const std::string& function_name,
        ir::IRContext& context
) {
    return LoadFunctionFromJsonFile(path, function_name, context);
}
}  // namespace qret::tests
