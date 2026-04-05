#include "qret/target/sc_ls_fixed_v0/lowering.h"

#include <gtest/gtest.h>

#include <fmt/format.h>

#include <iostream>

#include "qret/base/cast.h"
#include "qret/base/string.h"
#include "qret/codegen/machine_function.h"
#include "qret/target/sc_ls_fixed_v0/instruction.h"
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
}  // namespace

TEST(Lowering, Plane) {
    const auto size = std::size_t{3};

    ir::IRContext context;
    auto* circuit = LoadAddCuccaroCircuit(size, context);
    ASSERT_NE(nullptr, circuit);
    ir::DecomposeInst().RunOnFunction(*circuit);
    ir::InlinerPass().RunOnFunction(*circuit);

    auto topology = Topology::FromYAML(LoadFile("quration-core/tests/data/topology/plane.yaml"));
    const auto target = ScLsFixedV0TargetMachine(topology, ScLsFixedV0MachineOption());
    auto mf = MachineFunction(&target);
    mf.SetIR(circuit);
    Lowering().RunOnMachineFunction(mf);

    for (const auto& mbb : mf) {
        for (const auto& minst : mbb) {
            std::cout << minst->ToString() << std::endl;
        }
    }
}
TEST(Lowering, CXLowersToCnot) {
    const auto size = std::size_t{3};

    ir::IRContext context;
    auto* circuit = LoadAddCuccaroCircuit(size, context);
    ASSERT_NE(nullptr, circuit);
    ir::DecomposeInst().RunOnFunction(*circuit);
    ir::InlinerPass().RunOnFunction(*circuit);

    auto topology = Topology::FromYAML(LoadFile("quration-core/tests/data/topology/plane.yaml"));
    const auto target = ScLsFixedV0TargetMachine(topology, ScLsFixedV0MachineOption());
    auto mf = MachineFunction(&target);
    mf.SetIR(circuit);
    Lowering().RunOnMachineFunction(mf);

    auto has_cnot = false;
    for (const auto& mbb : mf) {
        for (const auto& minst : mbb) {
            has_cnot = has_cnot
                    || (DynCast<Cnot>(static_cast<const ScLsInstructionBase*>(minst.get()))
                        != nullptr);
        }
    }
    EXPECT_TRUE(has_cnot);
}
TEST(Lowering, Grid) {
    const auto size = std::size_t{3};

    ir::IRContext context;
    auto* circuit = LoadAddCuccaroCircuit(size, context);
    ASSERT_NE(nullptr, circuit);
    ir::DecomposeInst().RunOnFunction(*circuit);
    ir::InlinerPass().RunOnFunction(*circuit);

    auto topology = Topology::FromYAML(LoadFile("quration-core/tests/data/topology/grid.yaml"));
    const auto target = ScLsFixedV0TargetMachine(topology, ScLsFixedV0MachineOption());
    auto mf = MachineFunction(&target);
    mf.SetIR(circuit);
    Lowering().RunOnMachineFunction(mf);

    for (const auto& mbb : mf) {
        for (const auto& minst : mbb) {
            std::cout << minst->ToString() << std::endl;
        }
    }
}
TEST(Lowering, Distribute) {
    const auto size = std::size_t{3};

    ir::IRContext context;
    auto* circuit = LoadAddCuccaroCircuit(size, context);
    ASSERT_NE(nullptr, circuit);
    ir::DecomposeInst().RunOnFunction(*circuit);
    ir::InlinerPass().RunOnFunction(*circuit);

    auto topology =
            Topology::FromYAML(LoadFile("quration-core/tests/data/topology/distribute.yaml"));
    const auto target = ScLsFixedV0TargetMachine(topology, ScLsFixedV0MachineOption());
    auto mf = MachineFunction(&target);
    mf.SetIR(circuit);
    Lowering().RunOnMachineFunction(mf);

    for (const auto& mbb : mf) {
        for (const auto& minst : mbb) {
            std::cout << minst->ToString() << std::endl;
        }
    }
}
