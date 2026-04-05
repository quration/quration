#include <gtest/gtest.h>

#include <fmt/format.h>

#include <fstream>
#include <string_view>

#include "qret/base/log.h"
#include "qret/base/option.h"
#include "qret/base/string.h"
#include "qret/codegen/machine_function.h"
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

void SetRandomMappingOptions(std::uint64_t seed) {
    std::get<Option<std::uint32_t>*>(
            OptionStorage::GetOptionStorage()->At("sc_ls_fixed_v0-partition-algorithm")
    )
            ->SetValue(std::uint32_t{1});
    std::get<Option<std::uint64_t>*>(
            OptionStorage::GetOptionStorage()->At("sc_ls_fixed_v0-partition-seed")
    )
            ->SetValue(seed);
    std::get<Option<std::uint32_t>*>(
            OptionStorage::GetOptionStorage()->At("sc_ls_fixed_v0-find-place-algorithm")
    )
            ->SetValue(std::uint32_t{1});
}

void WriteTextFile(std::string_view path, std::string_view text) {
    auto ofs = std::ofstream(std::string(path));
    ASSERT_TRUE(ofs.good());
    ofs << text;
}
}  // namespace

TEST(PrintAssembly, Plane) {
    for (const auto seed : tests::LoadScLsFixedV0PartitionSeeds()) {
        SCOPED_TRACE(fmt::format("partition_seed={}", seed));

        const auto size = std::size_t{3};

        Logger::EnableConsoleOutput();
        Logger::SetLogLevel(LogLevel::Debug);
        SetRandomMappingOptions(seed);

        ir::IRContext context;
        auto* circuit = LoadAddCuccaroCircuit(size, context);
        ASSERT_NE(nullptr, circuit);
        ir::DecomposeInst().RunOnFunction(*circuit);
        ir::InlinerPass().RunOnFunction(*circuit);

        auto topology = Topology::FromYAML(LoadFile("quration-core/tests/data/topology/plane.yaml"));
        const auto target = ScLsFixedV0TargetMachine(
                topology,
                ScLsFixedV0MachineOption{
                        .magic_generation_period = 15,
                        .maximum_magic_state_stock = 100,
                        .entanglement_generation_period = 15,
                        .maximum_entangled_state_stock = 100,
                        .reaction_time = 1,
                }
        );
        auto mf = MachineFunction(&target);
        mf.SetIR(circuit);
        Lowering().RunOnMachineFunction(mf);
        Mapping().RunOnMachineFunction(mf);
        Routing().RunOnMachineFunction(mf);

        const auto* target_obj =
                qret::TargetRegistry::GetTargetRegistry()->GetTarget("sc_ls_fixed_v0");
        ASSERT_TRUE(target_obj->HasAsmPrinter());
        auto streamer = target_obj->CreateAsmStreamer();
        auto asm_printer = target_obj->CreateAsmPrinter(std::move(streamer));
        asm_printer->RunOnMachineFunction(mf);
        WriteTextFile("output_plane.asm", asm_printer->GetStreamer()->ToString());
    }
}
TEST(PrintAssembly, Grid) {
    for (const auto seed : tests::LoadScLsFixedV0PartitionSeeds()) {
        SCOPED_TRACE(fmt::format("partition_seed={}", seed));

        const auto size = std::size_t{3};

        Logger::EnableConsoleOutput();
        Logger::SetLogLevel(LogLevel::Debug);
        SetRandomMappingOptions(seed);

        ir::IRContext context;
        auto* circuit = LoadAddCuccaroCircuit(size, context);
        ASSERT_NE(nullptr, circuit);
        ir::DecomposeInst().RunOnFunction(*circuit);
        ir::InlinerPass().RunOnFunction(*circuit);

        auto topology = Topology::FromYAML(LoadFile("quration-core/tests/data/topology/grid.yaml"));
        const auto target = ScLsFixedV0TargetMachine(
                topology,
                ScLsFixedV0MachineOption{
                        .magic_generation_period = 15,
                        .maximum_magic_state_stock = 100,
                        .entanglement_generation_period = 15,
                        .maximum_entangled_state_stock = 100,
                        .reaction_time = 1,
                }
        );
        auto mf = MachineFunction(&target);
        mf.SetIR(circuit);
        Lowering().RunOnMachineFunction(mf);
        Mapping().RunOnMachineFunction(mf);
        Routing().RunOnMachineFunction(mf);

        const auto* target_obj =
                qret::TargetRegistry::GetTargetRegistry()->GetTarget("sc_ls_fixed_v0");
        ASSERT_TRUE(target_obj->HasAsmPrinter());
        auto streamer = target_obj->CreateAsmStreamer();
        auto asm_printer = target_obj->CreateAsmPrinter(std::move(streamer));
        asm_printer->RunOnMachineFunction(mf);
        WriteTextFile("output_grid.asm", asm_printer->GetStreamer()->ToString());
    }
}
TEST(PrintAssembly, Distribute) {
    for (const auto seed : tests::LoadScLsFixedV0PartitionSeeds()) {
        SCOPED_TRACE(fmt::format("partition_seed={}", seed));

        const auto size = std::size_t{3};

        Logger::EnableConsoleOutput();
        Logger::EnableColorfulOutput();
        Logger::SetLogLevel(LogLevel::Debug);
        SetRandomMappingOptions(seed);

        ir::IRContext context;
        auto* circuit = LoadAddCuccaroCircuit(size, context);
        ASSERT_NE(nullptr, circuit);
        ir::DecomposeInst().RunOnFunction(*circuit);
        ir::InlinerPass().RunOnFunction(*circuit);

        auto topology =
                Topology::FromYAML(LoadFile("quration-core/tests/data/topology/distribute.yaml"));
        const auto target = ScLsFixedV0TargetMachine(
                topology,
                ScLsFixedV0MachineOption{
                        .type = ScLsFixedV0MachineType::DistributedDim2,
                        .magic_generation_period = 15,
                        .maximum_magic_state_stock = 100,
                        .entanglement_generation_period = 15,
                        .maximum_entangled_state_stock = 100,
                        .reaction_time = 15,
                }
        );
        auto mf = MachineFunction(&target);
        mf.SetIR(circuit);
        Lowering().RunOnMachineFunction(mf);
        Mapping().RunOnMachineFunction(mf);
        Routing().RunOnMachineFunction(mf);

        const auto* target_obj =
                qret::TargetRegistry::GetTargetRegistry()->GetTarget("sc_ls_fixed_v0");
        ASSERT_TRUE(target_obj->HasAsmPrinter());
        auto streamer = target_obj->CreateAsmStreamer();
        auto asm_printer = target_obj->CreateAsmPrinter(std::move(streamer));
        asm_printer->RunOnMachineFunction(mf);
        WriteTextFile("output_distribute.asm", asm_printer->GetStreamer()->ToString());
    }
}
