#define FMT_HEADER_ONLY

#include <fmt/format.h>

#include <fstream>
#include <iostream>

#include "qret/base/json.h"
#include "qret/base/log.h"
#include "qret/ir/context.h"
#include "qret/ir/json.h"
#include "qret/ir/module.h"
#include "qret/pass.h"
#include "qret/runtime/full_quantum_state.h"
#include "qret/version.h"

int main() {
    qret::Logger::EnableConsoleOutput();
    qret::Logger::EnableColorfulOutput();
    LOG_INFO("QRET VERSION: {}.{}.{}", QRET_VERSION_MAJOR, QRET_VERSION_MINOR, QRET_VERSION_PATCH);
    LOG_INFO("Qulacs is {}", (qret::runtime::CanUseQulacs() ? "available" : "unavailable"));

    const auto size = std::size_t{5};
    const auto dst_value = 10;
    const auto src_value = 17;

    const auto json_path = std::string(QRET_TEST_DATA_DIR) + "/circuit/add_craig_5.json";
    auto ifs = std::ifstream(json_path);
    auto j = qret::Json::parse(ifs);
    qret::ir::IRContext context;
    qret::ir::LoadJson(j, context);
    const auto* func = context.owned_module.back()->GetFunction("AddCraig(5)");
    if (func == nullptr) {
        LOG_ERROR("function 'AddCraig(5)' not found in {}", json_path);
        return 1;
    }

    // Dump instructions
    for (const auto& bb : *func) {
        std::cout << bb.GetName() << std::endl;
        for (const auto& inst : bb) {
            std::cout << "  ";
            inst.Print(std::cout);
            std::cout << std::endl;
        }
    }

    const auto* registry = qret::PassRegistry::GetPassRegistry();
    LOG_INFO("Passes registered in registry:");
    for (const auto& [key, pass_info] : *registry) {
        LOG_INFO("  {}", key);
    }
    if (registry->Contains(std::string("sc_ls_fixed_v0::mapping"))) {
        LOG_INFO("sc_ls_fixed_v0::mapping is available");
    } else {
        LOG_WARN("sc_ls_fixed_v0::mapping is unavailable");
        return 1;
    }

    return 0;
}
