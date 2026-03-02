#include <gtest/gtest.h>

#include "qret/algorithm/arithmetic/integer.h"
#include "qret/base/type.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/intrinsic.h"
#include "qret/runtime/simulator.h"

using namespace qret;
using frontend::CircuitBuilder, runtime::SimulatorConfig, runtime::Simulator;

// Define test cases
class Comparison : public testing::TestWithParam<std::tuple<std::uint64_t, BigInt, BigInt>> {};
INSTANTIATE_TEST_SUITE_P(
        LightCase,
        Comparison,
        testing::Values(
                std::make_tuple(std::size_t{1}, BigInt(0), BigInt(0)),
                std::make_tuple(std::size_t{1}, BigInt(0), BigInt(1)),
                std::make_tuple(std::size_t{1}, BigInt(1), BigInt(0)),
                std::make_tuple(std::size_t{1}, BigInt(1), BigInt(1)),
                std::make_tuple(std::size_t{2}, BigInt(2), BigInt(1)),
                std::make_tuple(std::size_t{2}, BigInt(1), BigInt(2)),
                std::make_tuple(std::size_t{5}, BigInt(10), BigInt(6)),
                std::make_tuple(std::size_t{5}, BigInt(6), BigInt(10)),
                std::make_tuple(std::size_t{5}, BigInt(10), BigInt(10))
        )
);
INSTANTIATE_TEST_SUITE_P(
        HeavyCase,
        Comparison,
        testing::Values(
                std::make_tuple(std::size_t{1000}, BigInt(42789), BigInt(2347)),
                std::make_tuple(std::size_t{1000}, BigInt(2347), BigInt(42789)),
                std::make_tuple(std::size_t{1000}, BigInt(42789), BigInt(42789)),
                std::make_tuple(std::size_t{1000}, (BigInt(1) << 900) + 1, BigInt(1) << 900)
        )
);

//--------------------------------------------------//
// Test EqualTo
//--------------------------------------------------//
TEST_P(Comparison, TestEqualToUsingToffoliState) {
    using frontend::gate::EqualToGen;
    const auto [size, lhs_value, rhs_value] = GetParam();
    const auto max = BigInt(1) << size;

    ir::IRContext context;
    auto* module = ir::Module::Create("compare", context);
    auto builder = CircuitBuilder(module);
    auto gen = EqualToGen(&builder, size);
    auto* circuit = gen.Generate();

    auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 2;
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetValue(0, size, lhs_value);
    simulator.SetValue(size, size, rhs_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(lhs_value, state.GetValue(0, size));
    EXPECT_EQ(rhs_value, state.GetValue(size, size));
    EXPECT_EQ(lhs_value == rhs_value, state.GetValue(2 * size, 1));
    EXPECT_EQ(0, state.GetValue(2 * size + 1, size - 1));
}
//--------------------------------------------------//
// Test EqualToImm
//--------------------------------------------------//
TEST_P(Comparison, TestEqualToImmUsingToffoliState) {
    using frontend::gate::EqualToImmGen;
    const auto [size, lhs_value, rhs_value] = GetParam();
    const auto max = BigInt(1) << size;

    ir::IRContext context;
    auto* module = ir::Module::Create("compare", context);
    auto builder = CircuitBuilder(module);
    auto gen = EqualToImmGen(&builder, rhs_value, size);
    auto* circuit = gen.Generate();

    auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 2;
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetValue(0, size, lhs_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(lhs_value, state.GetValue(0, size));
    EXPECT_EQ(lhs_value == rhs_value, state.GetValue(size, 1));
    EXPECT_EQ(0, state.GetValue(size + 1, size - 1));
}
//--------------------------------------------------//
// Test LessThan
//--------------------------------------------------//
TEST_P(Comparison, TestLessThanUsingToffoliState) {
    using frontend::gate::LessThanGen;
    const auto [size, lhs_value, rhs_value] = GetParam();
    const auto max = BigInt(1) << size;

    ir::IRContext context;
    auto* module = ir::Module::Create("compare", context);
    auto builder = CircuitBuilder(module);
    auto gen = LessThanGen(&builder, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetValue(0, size, lhs_value);
    simulator.SetValue(size, size, rhs_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(lhs_value, state.GetValue(0, size));
    EXPECT_EQ(rhs_value, state.GetValue(size, size));
    EXPECT_EQ(lhs_value < rhs_value, state.GetValue(2 * size, 1));
    EXPECT_EQ(0, state.GetValue(2 * size + 1, 2));
}
//--------------------------------------------------//
// Test LessThanOrEqualTo
//--------------------------------------------------//
TEST_P(Comparison, TestLessThanOrEqualToUsingToffoliState) {
    using frontend::gate::LessThanOrEqualToGen;
    const auto [size, lhs_value, rhs_value] = GetParam();
    const auto max = BigInt(1) << size;

    ir::IRContext context;
    auto* module = ir::Module::Create("compare", context);
    auto builder = CircuitBuilder(module);
    auto gen = LessThanOrEqualToGen(&builder, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetValue(0, size, lhs_value);
    simulator.SetValue(size, size, rhs_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(lhs_value, state.GetValue(0, size));
    EXPECT_EQ(rhs_value, state.GetValue(size, size));
    EXPECT_EQ(lhs_value <= rhs_value, state.GetValue(2 * size, 1));
    EXPECT_EQ(0, state.GetValue(2 * size + 1, 2));
}
//--------------------------------------------------//
// Test LessThanImm
//--------------------------------------------------//
TEST_P(Comparison, TestLessThanImmUsingToffoliState) {
    using frontend::gate::LessThanImmGen;
    const auto [size, lhs_value, rhs_value] = GetParam();
    const auto max = BigInt(1) << size;

    ir::IRContext context;
    auto* module = ir::Module::Create("compare", context);
    auto builder = CircuitBuilder(module);
    auto gen = LessThanImmGen(&builder, rhs_value, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetValue(0, size, lhs_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(lhs_value, state.GetValue(0, size));
    EXPECT_EQ(lhs_value < rhs_value, state.GetValue(size, 1));
    EXPECT_EQ(0, state.GetValue(size + 1, 1));
}
//--------------------------------------------------//
// Test MultiControlledLessThanImm
//--------------------------------------------------//
TEST_P(Comparison, TestMultiControlledLessThanImmUsingToffoliStateCtrlTrue) {
    using frontend::gate::MultiControlledLessThanImmGen;
    const auto [size, lhs_value, rhs_value] = GetParam();
    const auto max = BigInt(1) << size;

    ir::IRContext context;
    auto* module = ir::Module::Create("compare", context);
    auto builder = CircuitBuilder(module);
    auto gen = MultiControlledLessThanImmGen(&builder, rhs_value, 1, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetAs1(0);  // ctrl-on
    simulator.SetValue(1, size, lhs_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(1, state.GetValue(0, 1));
    EXPECT_EQ(lhs_value, state.GetValue(1, size));
    EXPECT_EQ(lhs_value < rhs_value, state.GetValue(1 + size, 1));
    EXPECT_EQ(0, state.GetValue(1 + size + 1, 1));
}
TEST_P(Comparison, TestMultiControlledLessThanImmUsingToffoliStateCtrlFalse) {
    using frontend::gate::MultiControlledLessThanImmGen;
    const auto [size, lhs_value, rhs_value] = GetParam();
    const auto max = BigInt(1) << size;

    ir::IRContext context;
    auto* module = ir::Module::Create("compare", context);
    auto builder = CircuitBuilder(module);
    auto gen = MultiControlledLessThanImmGen(&builder, rhs_value, 1, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    // simulator.SetAs1(0);  // ctrl-off
    simulator.SetValue(1, size, lhs_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(0, state.GetValue(0, 1));
    EXPECT_EQ(lhs_value, state.GetValue(1, size));
    EXPECT_EQ(0, state.GetValue(1 + size + 1, 1));
}
