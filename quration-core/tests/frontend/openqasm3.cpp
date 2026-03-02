#include "qret/parser/openqasm3.h"

#include <gtest/gtest.h>

#include "qret/frontend/builder.h"
#include "qret/frontend/openqasm2.h"
#include "qret/ir/context.h"
#include "qret/ir/module.h"
#include "qret/runtime/simulator.h"

namespace qret {
runtime::Simulator
SimulateOpenQASM3(const qret::openqasm2::Program& program, const runtime::SimulatorConfig& config) {
    ir::IRContext context;
    auto* module = ir::Module::Create("simulate_openqasm3", context);
    auto builder = qret::frontend::CircuitBuilder(module);
    auto* circuit = qret::frontend::BuildCircuitFromAST(program, builder);

    auto simulator = runtime::Simulator(config, circuit->GetIR());
    simulator.RunAll();
    return simulator;
}
}  // namespace qret

TEST(OpenQASM3, ParseAndRunWithStdGates) {
    if (!qret::openqasm3::CanParseOpenQASM3()) {
        GTEST_SKIP() << "Skip OpenQASM3 test";
    }

    const auto program =
            qret::openqasm3::ParseOpenQASM3File("quration-core/tests/data/OpenQASM3/x.qasm");

    const auto config =
            qret::runtime::SimulatorConfig{qret::runtime::SimulatorConfig::StateType::Toffoli};
    const auto simulator = qret::SimulateOpenQASM3(program, config);
    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(1, state.ReadRegister(0));
}
