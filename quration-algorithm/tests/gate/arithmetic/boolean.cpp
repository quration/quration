#include "qret/algorithm/arithmetic/boolean.h"

#include <gtest/gtest.h>

#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/intrinsic.h"
#include "qret/runtime/simulator.h"

using namespace qret;
using frontend::CircuitBuilder, runtime::SimulatorConfig, runtime::Simulator;

namespace {
bool MAJ(bool x, bool y, bool z) {
    auto tmp = static_cast<std::int32_t>(x) + static_cast<std::int32_t>(y)
            + static_cast<std::int32_t>(z);
    return tmp > std::int32_t{1};
}
}  // namespace

// Define test cases
class Boolean : public testing::TestWithParam<std::tuple<bool, bool, bool>> {};
INSTANTIATE_TEST_SUITE_P(
        Exhaustive,
        Boolean,
        testing::Combine(testing::Bool(), testing::Bool(), testing::Bool())
);

//--------------------------------------------------//
// Test TemporalAnd
//--------------------------------------------------//
TEST_P(Boolean, TestTemporalAndUsingToffoliState) {
    using frontend::gate::TemporalAndGen;
    const auto [_, y, z] = GetParam();

    ir::IRContext context;
    auto* module = ir::Module::Create("TemporalAND", context);
    auto builder = CircuitBuilder(module);
    auto gen = TemporalAndGen(&builder);
    auto* circuit = gen.Generate();

    auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 2;
    auto simulator = Simulator(config, circuit->GetIR());
    // if (x) { simulator.SetAs1(0); }
    if (y) {
        simulator.SetAs1(1);
    }
    if (z) {
        simulator.SetAs1(2);
    }
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(y & z, state.GetValue(0, 1));
    EXPECT_EQ(y, state.GetValue(1, 1));
    EXPECT_EQ(z, state.GetValue(2, 1));
}
//--------------------------------------------------//
// Test UncomputeTemporalAnd
//--------------------------------------------------//
TEST_P(Boolean, TestUncomputeTemporalAndUsingToffoliState) {
    using frontend::gate::UncomputeTemporalAndGen;
    const auto [_, y, z] = GetParam();

    ir::IRContext context;
    auto* module = ir::Module::Create("UncomputeTemporalAND", context);
    auto builder = CircuitBuilder(module);
    auto gen = UncomputeTemporalAndGen(&builder);
    auto* circuit = gen.Generate();

    auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 2;
    auto simulator = Simulator(config, circuit->GetIR());
    if (y && z) {
        simulator.SetAs1(0);
    }
    if (y) {
        simulator.SetAs1(1);
    }
    if (z) {
        simulator.SetAs1(2);
    }
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(0, state.GetValue(0, 1));
    EXPECT_EQ(y, state.GetValue(1, 1));
    EXPECT_EQ(z, state.GetValue(2, 1));
}
//--------------------------------------------------//
// Test RT3
//--------------------------------------------------//
TEST_P(Boolean, TestRT3UsingToffoliState) {
    using frontend::gate::RT3Gen;
    const auto [x, y, z] = GetParam();

    ir::IRContext context;
    auto* module = ir::Module::Create("RT3", context);
    auto builder = CircuitBuilder(module);
    auto gen = RT3Gen(&builder);
    auto* circuit = gen.Generate();

    auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 2;
    auto simulator = Simulator(config, circuit->GetIR());
    if (x) {
        simulator.SetAs1(0);
    }
    if (y) {
        simulator.SetAs1(1);
    }
    if (z) {
        simulator.SetAs1(2);
    }
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(x ^ (y & z), state.GetValue(0, 1));
    EXPECT_EQ(y, state.GetValue(1, 1));
    EXPECT_EQ(z, state.GetValue(2, 1));
}
//--------------------------------------------------//
// Test IRT3
//--------------------------------------------------//
TEST_P(Boolean, TestIRT3UsingToffoliState) {
    using frontend::gate::IRT3Gen;
    const auto [x, y, z] = GetParam();

    ir::IRContext context;
    auto* module = ir::Module::Create("IRT3", context);
    auto builder = CircuitBuilder(module);
    auto gen = IRT3Gen(&builder);
    auto* circuit = gen.Generate();

    auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 2;
    auto simulator = Simulator(config, circuit->GetIR());
    if (x) {
        simulator.SetAs1(0);
    }
    if (y) {
        simulator.SetAs1(1);
    }
    if (z) {
        simulator.SetAs1(2);
    }
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(x ^ (y & z), state.GetValue(0, 1));
    EXPECT_EQ(y, state.GetValue(1, 1));
    EXPECT_EQ(z, state.GetValue(2, 1));
}
//--------------------------------------------------//
// Test RT4
//--------------------------------------------------//
TEST_P(Boolean, TestRT4UsingToffoliState) {
    using frontend::gate::RT4Gen;
    const auto [x, y, z] = GetParam();

    ir::IRContext context;
    auto* module = ir::Module::Create("RT4", context);
    auto builder = CircuitBuilder(module);
    auto gen = RT4Gen(&builder);
    auto* circuit = gen.Generate();

    auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 2;
    auto simulator = Simulator(config, circuit->GetIR());
    if (x) {
        simulator.SetAs1(0);
    }
    if (y) {
        simulator.SetAs1(1);
    }
    if (z) {
        simulator.SetAs1(2);
    }
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(x ^ (y & z), state.GetValue(0, 1));
    EXPECT_EQ(y, state.GetValue(1, 1));
    EXPECT_EQ(z, state.GetValue(2, 1));
}
//--------------------------------------------------//
// Test IRT4
//--------------------------------------------------//
TEST_P(Boolean, TestIRT4UsingToffoliState) {
    using frontend::gate::IRT4Gen;
    const auto [x, y, z] = GetParam();

    ir::IRContext context;
    auto* module = ir::Module::Create("IRT4", context);
    auto builder = CircuitBuilder(module);
    auto gen = IRT4Gen(&builder);
    auto* circuit = gen.Generate();

    auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 2;
    auto simulator = Simulator(config, circuit->GetIR());
    if (x) {
        simulator.SetAs1(0);
    }
    if (y) {
        simulator.SetAs1(1);
    }
    if (z) {
        simulator.SetAs1(2);
    }
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(x ^ (y & z), state.GetValue(0, 1));
    EXPECT_EQ(y, state.GetValue(1, 1));
    EXPECT_EQ(z, state.GetValue(2, 1));
}
//--------------------------------------------------//
// Test MAJ
//--------------------------------------------------//
TEST_P(Boolean, TestMAJUsingToffoliState) {
    using frontend::gate::MAJGen;
    const auto [x, y, z] = GetParam();

    ir::IRContext context;
    auto* module = ir::Module::Create("MAJ", context);
    auto builder = CircuitBuilder(module);
    auto gen = MAJGen(&builder);
    auto* circuit = gen.Generate();

    auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 2;
    auto simulator = Simulator(config, circuit->GetIR());
    if (x) {
        simulator.SetAs1(0);
    }
    if (y) {
        simulator.SetAs1(1);
    }
    if (z) {
        simulator.SetAs1(2);
    }
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(MAJ(x, y, z), state.GetValue(0, 1));
    EXPECT_EQ(x ^ y, state.GetValue(1, 1));
    EXPECT_EQ(x ^ z, state.GetValue(2, 1));
}
//--------------------------------------------------//
// Test UMA
//--------------------------------------------------//
TEST_P(Boolean, TestUMAUsingToffoliState) {
    using frontend::gate::UMAGen;
    const auto [x, y, z] = GetParam();

    ir::IRContext context;
    auto* module = ir::Module::Create("UMA", context);
    auto builder = CircuitBuilder(module);
    auto gen = UMAGen(&builder);
    auto* circuit = gen.Generate();

    auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 2;
    auto simulator = Simulator(config, circuit->GetIR());
    if (MAJ(x, y, z)) {
        simulator.SetAs1(0);
    }
    if (x ^ y) {
        simulator.SetAs1(1);
    }
    if (x ^ z) {
        simulator.SetAs1(2);
    }
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(x, state.GetValue(0, 1));
    EXPECT_EQ(y, state.GetValue(1, 1));
    EXPECT_EQ(x ^ y ^ z, state.GetValue(2, 1));
}
