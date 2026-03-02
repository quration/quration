#include "qret/target/sc_ls_fixed_v0/pipeline_state.h"

#include <gtest/gtest.h>

#include <fmt/format.h>

#include <cstddef>
#include <string>
#include <vector>

#include "qret/codegen/dummy.h"
#include "qret/codegen/machine_function.h"
#include "qret/codegen/machine_function_pass.h"
#include "qret/target/sc_ls_fixed_v0/lowering.h"
#include "qret/target/sc_ls_fixed_v0/mapping.h"
#include "qret/target/sc_ls_fixed_v0/routing.h"
#include "qret/target/sc_ls_fixed_v0/sc_ls_fixed_v0_target_machine.h"
#include "qret/target/sc_ls_fixed_v0/topology.h"
#include "qret/transforms/ipo/inliner.h"
#include "qret/transforms/scalar/decomposition.h"

#include "test_utils.h"

using namespace qret;
using namespace qret::sc_ls_fixed_v0;

namespace {
ir::Function* LoadAddCuccaroCircuit(std::size_t size, ir::IRContext& context) {
    const auto path = fmt::format("quration-core/tests/data/circuit/add_cuccaro_{}.json", size);
    const auto name = fmt::format("AddCuccaro({})", size);
    return tests::LoadCircuitFromJsonFile(path, name, context);
}

ScLsFixedV0MachineOption BuildMachineOption() {
    return ScLsFixedV0MachineOption{
            .magic_generation_period = 15,
            .maximum_magic_state_stock = 100,
            .entanglement_generation_period = 15,
            .maximum_entangled_state_stock = 100,
            .reaction_time = 1,
    };
}

std::vector<std::string> DumpProgram(const MachineFunction& mf) {
    auto out = std::vector<std::string>{};
    for (const auto& bb : mf) {
        for (const auto& inst : bb) {
            out.emplace_back(inst->ToString());
        }
    }
    return out;
}

MachineFunction
BuildCompiledMachineFunction(ir::Function& circuit, const ScLsFixedV0TargetMachine& target) {
    auto mf = MachineFunction(&target);
    mf.SetIR(&circuit);
    Lowering().RunOnMachineFunction(mf);
    Mapping().RunOnMachineFunction(mf);
    Routing().RunOnMachineFunction(mf);
    return mf;
}
}  // namespace

TEST(PipelineState, SaveAndLoadWithoutManager) {
    ir::IRContext context;
    auto* circuit = LoadAddCuccaroCircuit(3, context);
    ASSERT_NE(nullptr, circuit);
    ir::DecomposeInst().RunOnFunction(*circuit);
    ir::InlinerPass().RunOnFunction(*circuit);

    auto topology = Topology::FromYAML(LoadFile("quration-core/tests/data/topology/plane.yaml"));
    const auto target = ScLsFixedV0TargetMachine(topology, BuildMachineOption());
    const auto original_mf = BuildCompiledMachineFunction(*circuit, target);

    const auto state = BuildPipelineState(original_mf);
    const auto file = std::string("pipeline_state_plane.json");
    SavePipelineState(file, state);

    const auto loaded_state = LoadPipelineState(file);
    auto loaded_mf = MachineFunction(&target);
    ApplyPipelineState(loaded_state, loaded_mf);

    EXPECT_EQ(DumpProgram(original_mf), DumpProgram(loaded_mf));
    EXPECT_EQ(state.program.size(), loaded_state.program.size());
    EXPECT_TRUE(loaded_state.parameter.target.has_value());
}

TEST(PipelineState, SaveAndLoadWithManager) {
    ir::IRContext context;
    auto* circuit = LoadAddCuccaroCircuit(3, context);
    ASSERT_NE(nullptr, circuit);
    ir::DecomposeInst().RunOnFunction(*circuit);
    ir::InlinerPass().RunOnFunction(*circuit);

    auto topology = Topology::FromYAML(LoadFile("quration-core/tests/data/topology/grid.yaml"));
    auto target = ScLsFixedV0TargetMachine::New(topology, BuildMachineOption());

    auto original_mf = BuildCompiledMachineFunction(*circuit, *target);
    auto compile_info = std::unique_ptr<ScLsFixedV0CompileInfo>(new ScLsFixedV0CompileInfo());
    compile_info->runtime = 123;
    original_mf.InitializeCompileInfo(std::move(compile_info));

    auto manager = MFPassManager{};
    manager.AddPass(std::unique_ptr<DummyPass>(new DummyPass("custom_pass")));
    manager.Run(original_mf);
    ASSERT_TRUE(original_mf.HasCompileInfo());

    const auto state = BuildPipelineState(manager, target, original_mf);
    const auto file = std::string("pipeline_state_grid.json");
    SavePipelineState(file, state);

    const auto loaded_state = LoadPipelineState(file);
    auto target_after_load =
            ScLsFixedV0TargetMachine::New(topology, ScLsFixedV0MachineOption{.reaction_time = 42});
    auto loaded_manager = MFPassManager{};
    auto loaded_mf = MachineFunction(target_after_load.get());
    ApplyPipelineState(loaded_state, loaded_manager, target_after_load, loaded_mf);

    EXPECT_EQ(DumpProgram(original_mf), DumpProgram(loaded_mf));
    EXPECT_TRUE(loaded_mf.HasCompileInfo());
    EXPECT_EQ(123, static_cast<const ScLsFixedV0CompileInfo*>(loaded_mf.GetCompileInfo())->runtime);
    EXPECT_EQ(state.opt.passes.size(), loaded_manager.GetAnalysis().run_order.size());
    EXPECT_EQ(
            target->machine_option.reaction_time,
            target_after_load->machine_option.reaction_time
    );
}
