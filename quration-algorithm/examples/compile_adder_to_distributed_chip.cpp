#include "qret/algorithm/arithmetic/integer.h"
#include "qret/base/log.h"
#include "qret/base/option.h"
#include "qret/base/string.h"
#include "qret/codegen/machine_function.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/intrinsic.h"
#include "qret/target/sc_ls_fixed_v0/calc_compile_info.h"
#include "qret/target/sc_ls_fixed_v0/lowering.h"
#include "qret/target/sc_ls_fixed_v0/mapping.h"
#include "qret/target/sc_ls_fixed_v0/routing.h"
#include "qret/target/sc_ls_fixed_v0/sc_ls_fixed_v0_target_machine.h"
#include "qret/target/sc_ls_fixed_v0/topology.h"
#include "qret/transforms/scalar/decomposition.h"

using namespace qret;
using namespace qret::sc_ls_fixed_v0;

void Run(const std::size_t size, const std::string& topology_path) {
    std::cout << "-----------------------------------------------------------" << std::endl;
    std::cout << fmt::format("size = {}, path = {}", size, topology_path) << std::endl;

    ir::IRContext context;
    auto* module = ir::Module::Create("lowering", context);
    auto builder = frontend::CircuitBuilder(module);
    auto gen = frontend::gate::AddCuccaroGen(&builder, size);
    auto* circuit = gen.Generate();
    auto* ir = circuit->GetIR();
    ir::DecomposeInst().RunOnFunction(*ir);

    auto topology = Topology::FromYAML(LoadFile(topology_path));
    const auto target = ScLsFixedV0TargetMachine(
            topology,
            ScLsFixedV0MachineOption{
                    .type = ScLsFixedV0MachineType::DistributedDim2,
                    .magic_generation_period = 15,
                    .maximum_magic_state_stock = 10,
                    .entanglement_generation_period = 100,
                    .maximum_entangled_state_stock = 10,
                    .reaction_time = 15,
            }
    );
    auto mf = MachineFunction(&target);
    mf.SetIR(ir);
    InitCompileInfo().RunOnMachineFunction(mf);
    Lowering().RunOnMachineFunction(mf);
    Mapping().RunOnMachineFunction(mf);

    Routing().RunOnMachineFunction(mf);
    CompileInfoWithoutTopology().RunOnMachineFunction(mf);
    CompileInfoWithTopology().RunOnMachineFunction(mf);
    DumpCompileInfo().RunOnMachineFunction(mf);
}

int main() {
    // Set global variables.
    // Logger::EnableConsoleOutput();
    Logger::DisableConsoleOutput();
    Logger::DisableColorfulOutput();
    Logger::SetLogLevel(LogLevel::Debug);

    // 0: Map based on topology file, 1: Auto
    std::get<Option<std::uint32_t>*>(
            OptionStorage::GetOptionStorage()->At("sc_ls_fixed_v0-mapping-algorithm")
    )
            ->SetValue(std::uint32_t{0});

    // 0: Greedy, 1: Random, 2: METIS
    std::get<Option<std::uint32_t>*>(
            OptionStorage::GetOptionStorage()->At("sc_ls_fixed_v0-partition-algorithm")
    )
            ->SetValue(std::uint32_t{1});

    // 0: EnoughSpaceSoft, 1: EnoughSpaceHard
    std::get<Option<std::uint32_t>*>(
            OptionStorage::GetOptionStorage()->At("sc_ls_fixed_v0-find-place-algorithm")
    )
            ->SetValue(std::uint32_t{1});

    for (const auto size : {5, 10, 15, 20, 25, 30}) {
        Run(size, "quration-core/examples/data/topology/dist_line.yaml");
        Run(size, "quration-core/examples/data/topology/dist_circle.yaml");
        Run(size, "quration-core/examples/data/topology/dist_all.yaml");
    }
    std::get<Option<std::string>*>(
            OptionStorage::GetOptionStorage()->At("sc_ls_fixed_v0_dump_compile_info_to_json")
    )
            ->SetValue("adder_dist_line_10.json");
    Run(10, "quration-core/examples/data/topology/dist_line.yaml");
    std::get<Option<std::string>*>(
            OptionStorage::GetOptionStorage()->At("sc_ls_fixed_v0_dump_compile_info_to_json")
    )
            ->SetValue("adder_dist_circle_10.json");
    Run(10, "quration-core/examples/data/topology/dist_circle.yaml");
    std::get<Option<std::string>*>(
            OptionStorage::GetOptionStorage()->At("sc_ls_fixed_v0_dump_compile_info_to_json")
    )
            ->SetValue("adder_dist_all_10.json");
    Run(10, "quration-core/examples/data/topology/dist_all.yaml");

    return 0;
}
