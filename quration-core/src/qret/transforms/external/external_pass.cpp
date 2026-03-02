/**
 * @file qret/transforms/external/external_pass.cpp
 * @brief Pass to run commands defined externally.
 */

#include "qret/transforms/external/external_pass.h"

#include <fmt/format.h>

#include <exception>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "qret/base/json.h"
#include "qret/base/log.h"
#include "qret/base/system.h"
#include "qret/ir/function.h"
#include "qret/ir/json.h"

namespace qret::ir {
namespace {
static auto X = RegisterPass<ExternalPass>("ExternalPass", "ir::external");
}

void SaveCircuit(const std::string& path, Function& func) {
    auto j = Json(*func.GetParent());
    auto ofs = std::ofstream(path);
    if (!ofs.good()) {
        throw std::runtime_error(fmt::format("Failed to open {}", path));
    }
    ofs << j;
    ofs.close();
}
void LoadCircuit(const std::string& path, Function& func) {
    auto ifs = std::ifstream(path);
    if (!ifs.good()) {
        throw std::runtime_error(fmt::format("Failed to open {}", path));
    }
    auto j = Json::parse(ifs);

    func.Clear();
    auto cmap = CircuitMap();
    auto* mod = func.GetParent();
    for (auto& c : *mod) {
        cmap[std::string(c.GetName())] = &c;
    }
    for (const auto& tmp : j["circuit_list"]) {
        if (tmp["name"].template get<std::string>() == func.GetName()) {
            LoadCircuit(tmp, cmap);
        }
    }
}
bool ExternalPass::RunOnFunction(Function& func) {
    bool changed = false;

    SaveCircuit(input_, func);

    // Run external pass
    try {
        auto out = std::string();
        auto exit = std::int32_t{0};
        RunCommand(cmd_, out, exit);
        if (exit != 0) {
            LOG_ERROR("failed to run '{}' (returncode: {})", cmd_, exit);
            if (!out.empty()) {
                LOG_ERROR("log: {}", out);
            }
            return changed = false;
        }
    } catch (std::exception& ex) {
        LOG_ERROR("exception '{}' occurred when running {}", ex.what(), cmd_);
        return changed = false;
    }

    LoadCircuit(output_, func);

    return changed = true;
}
}  // namespace qret::ir
