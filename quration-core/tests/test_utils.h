#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

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

inline std::vector<std::uint64_t> LoadSeedListFromEnv(
        const char* env_name,
        const std::uint64_t default_seed
) {
    const auto* raw = std::getenv(env_name);
    if (raw == nullptr || raw[0] == '\0') {
        return {default_seed};
    }

    auto seeds = std::vector<std::uint64_t>();
    auto ss = std::stringstream(raw);
    auto item = std::string();
    while (std::getline(ss, item, ',')) {
        item.erase(
                std::remove_if(item.begin(), item.end(), [](unsigned char c) { return std::isspace(c); }),
                item.end()
        );
        if (item.empty()) {
            continue;
        }
        seeds.emplace_back(static_cast<std::uint64_t>(std::stoull(item)));
    }
    if (seeds.empty()) {
        return {default_seed};
    }
    return seeds;
}

inline std::vector<std::uint64_t> LoadScLsFixedV0PartitionSeeds() {
    return LoadSeedListFromEnv("QRET_SC_LS_FIXED_V0_TEST_SEEDS", 314);
}
}  // namespace qret::tests
