/**
 * @file qret/target/sc_ls_fixed_v0/external_pass.cpp
 * @brief Pass to run commands defined externally.
 */

#include "qret/target/sc_ls_fixed_v0/external_pass.h"

#include <fmt/format.h>

#include <exception>

#include "qret/base/log.h"
#include "qret/base/system.h"
#include "qret/target/sc_ls_fixed_v0/pipeline_state.h"

namespace qret::sc_ls_fixed_v0 {
namespace {
static auto X = RegisterPass<ExternalPass>("ExternalPass", "sc_ls_fixed_v0::external");
}

bool ExternalPass::RunOnMachineFunction(MachineFunction& mf) {
    bool changed = false;

    const auto input_state = BuildPipelineState(mf);
    SavePipelineState(input_, input_state);

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

    const auto output_state = LoadPipelineState(output_);
    ApplyPipelineState(output_state, mf);

    return changed = true;
}

}  // namespace qret::sc_ls_fixed_v0
