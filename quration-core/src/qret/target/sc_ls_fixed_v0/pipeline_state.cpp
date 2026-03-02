/**
 * @file qret/target/sc_ls_fixed_v0/pipeline_state.cpp
 * @brief SC_LS_FIXED_V0 pipeline state.
 */

#include "qret/target/sc_ls_fixed_v0/pipeline_state.h"

#include <fmt/format.h>

#include <fstream>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <vector>

#include "qret/base/log.h"
#include "qret/base/time.h"
#include "qret/codegen/dummy.h"
#include "qret/codegen/machine_function.h"
#include "qret/codegen/machine_function_pass.h"
#include "qret/target/sc_ls_fixed_v0/instruction.h"
#include "qret/version.h"

namespace qret::sc_ls_fixed_v0 {
namespace {
void BuildProgramJson(const MachineFunction& mf, std::vector<Json>& out) {
    out.clear();
    for (const auto& bb : mf) {
        for (const auto& tmp : bb) {
            const auto& inst = *static_cast<const ScLsInstructionBase*>(tmp.get());
            out.emplace_back(inst.ToJson());
        }
    }
}

void ApplyProgramJson(const std::vector<Json>& program, MachineFunction& mf) {
    auto& bb = mf.AddBlock();
    for (const auto& inst : program) {
        bb.EmplaceBack(ScLsInstructionBase::FromJson(inst));
    }
}

void LoadPassHistory(
        const std::vector<std::tuple<std::string, double>>& passes,
        MFPassManager& manager
) {
    for (const auto& [pass_name, elapsed_ms] : passes) {
        const auto* registry = qret::PassRegistry::GetPassRegistry();
        MachineFunctionPass* pass = nullptr;
        if (registry->Contains(pass_name)) {
            const auto* pi = registry->GetPassInfo(pass_name);
            pass = static_cast<MachineFunctionPass*>(manager.AddPass(pi->GetNormalCtor()()));
        } else {
            pass = static_cast<MachineFunctionPass*>(
                    manager.AddPass(std::unique_ptr<DummyPass>(new DummyPass(pass_name)))
            );
        }
        auto& analysis = manager.GetMutAnalysis();
        analysis.run_order.emplace_back(pass);
        analysis.elapsed_ms[pass] = MFAnalysis::Time(elapsed_ms);
    }
}
}  // namespace

void to_json(Json& j, const ScLsFixedV0PipelineState& state) {
    // metadata
    j["metadata"]["format"] = "SC_LS_FIXED_V0";
    j["metadata"]["schema_version"] = PipelineStateJsonSchemaVersion;
    j["metadata"]["qret_version"] =
            fmt::format("{}.{}.{}", QRET_VERSION_MAJOR, QRET_VERSION_MINOR, QRET_VERSION_PATCH),
    j["metadata"]["created_at"] = fmt::format("{}", GetCurrentTime());

    // info
    j["info"] = Json();
    if (!state.info.description.empty()) {
        j["info"]["description"] = state.info.description;
    }

    // parameter
    j["parameter"] = Json();
    if (state.parameter.target.has_value()) {
        j["parameter"]["target"] = *state.parameter.target;
    }

    // opt
    j["opt"] = Json();
    if (!state.opt.passes.empty()) {
        auto& passes = j["opt"]["passes"] = Json::array();
        for (const auto& [pass_name, elapsed_ms] : state.opt.passes) {
            passes.emplace_back(Json({pass_name, elapsed_ms}));
        }
    }
    if (state.opt.compile_info.has_value()) {
        j["opt"]["compile_info"] = state.opt.compile_info.value();
    }

    // program
    j["program"] = Json::array();
    for (const auto& inst : state.program) {
        j["program"].emplace_back(inst);
    }
}

void from_json(const Json& j, ScLsFixedV0PipelineState& state) {
    state = {};

    // info
    if (j.contains("info") && j["info"].contains("description")) {
        state.info.description = j["info"]["description"].template get<std::string>();
    }

    // parameter
    if (j.contains("parameter") && j["parameter"].contains("target")) {
        state.parameter.target = j["parameter"]["target"].template get<ScLsFixedV0TargetMachine>();
    }

    // opt
    if (j.contains("opt") && j["opt"].contains("passes")) {
        const auto& passes = j["opt"]["passes"];
        for (const auto& pass : passes) {
            state.opt.passes.emplace_back(
                    pass[0].template get<std::string>(),
                    pass[1].template get<double>()
            );
        }
    }
    if (j.contains("opt") && j["opt"].contains("compile_info")) {
        state.opt.compile_info = j["opt"]["compile_info"].template get<ScLsFixedV0CompileInfo>();
    }

    // program
    if (j.contains("program")) {
        for (const auto& inst : j["program"]) {
            state.program.emplace_back(inst);
        }
    }
}

ScLsFixedV0PipelineState LoadPipelineState(const std::string& input) {
    LOG_INFO("Loading SC_LS_FIXED_V0 pipeline state from JSON file: {}", input);

    auto ifs = std::ifstream(input);
    if (!ifs.good()) {
        throw std::runtime_error(fmt::format("failed to open: {}", input));
    }
    auto j = Json::parse(ifs);
    return j.template get<ScLsFixedV0PipelineState>();
}

void SavePipelineState(const std::string& output, const ScLsFixedV0PipelineState& state) {
    LOG_INFO("Saving SC_LS_FIXED_V0 pipeline state to JSON file: {}", output);

    auto ofs = std::ofstream(output);
    if (!ofs.good()) {
        throw std::runtime_error(fmt::format("failed to open: {}", output));
    }
    ofs << Json(state) << std::endl;
}

ScLsFixedV0PipelineState BuildPipelineState(
        const MFPassManager& manager,
        const std::unique_ptr<ScLsFixedV0TargetMachine>& target,
        const MachineFunction& mf
) {
    auto state = ScLsFixedV0PipelineState{};

    if (target != nullptr) {
        state.parameter.target = *target;
    }

    const auto& analysis = manager.GetAnalysis();
    for (const auto& pass : analysis.run_order) {
        const auto it = analysis.elapsed_ms.find(pass);
        if (it == analysis.elapsed_ms.end()) {
            continue;
        }
        state.opt.passes.emplace_back(pass->GetPassArgument(), it->second.count());
    }

    if (mf.HasCompileInfo()) {
        state.opt.compile_info = *static_cast<const ScLsFixedV0CompileInfo*>(mf.GetCompileInfo());
    }

    BuildProgramJson(mf, state.program);
    return state;
}

ScLsFixedV0PipelineState BuildPipelineState(const MachineFunction& mf) {
    auto state = ScLsFixedV0PipelineState{};

    state.info.description =
            "This is a SC_LS_FIXED_V0 pipeline state file. Do not change 'parameter' field.";

    const auto* target = static_cast<const ScLsFixedV0TargetMachine*>(mf.GetTarget());
    if (target != nullptr) {
        state.parameter.target = *target;
    }

    if (mf.HasCompileInfo()) {
        state.opt.compile_info = *static_cast<const ScLsFixedV0CompileInfo*>(mf.GetCompileInfo());
    }

    BuildProgramJson(mf, state.program);
    return state;
}

void ApplyPipelineState(
        const ScLsFixedV0PipelineState& state,
        MFPassManager& manager,
        std::unique_ptr<ScLsFixedV0TargetMachine>& target,
        MachineFunction& mf
) {
    mf.Clear();
    mf.InitializeCompileInfo(std::unique_ptr<qret::CompileInfo>{});

    if (state.parameter.target.has_value()) {
        LOG_INFO("Load parameter");
        *target = *state.parameter.target;
    }
    if (!state.opt.passes.empty()) {
        LOG_INFO("Load passes");
    }
    LoadPassHistory(state.opt.passes, manager);

    if (state.opt.compile_info.has_value()) {
        LOG_INFO("Load compile info");
        mf.InitializeCompileInfo(
                std::unique_ptr<ScLsFixedV0CompileInfo>(
                        new ScLsFixedV0CompileInfo(*state.opt.compile_info)
                )
        );
    }

    LOG_INFO("Load program");
    ApplyProgramJson(state.program, mf);
    mf.SetTarget(target.get());
}

void ApplyPipelineState(const ScLsFixedV0PipelineState& state, MachineFunction& mf) {
    mf.Clear();
    mf.InitializeCompileInfo(std::unique_ptr<qret::CompileInfo>{});

    if (state.opt.compile_info.has_value()) {
        LOG_INFO("Load compile info");
        mf.InitializeCompileInfo(
                std::unique_ptr<ScLsFixedV0CompileInfo>(
                        new ScLsFixedV0CompileInfo(*state.opt.compile_info)
                )
        );
    }

    LOG_INFO("Load program");
    ApplyProgramJson(state.program, mf);
}
}  // namespace qret::sc_ls_fixed_v0
