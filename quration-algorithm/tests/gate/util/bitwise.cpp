#include "qret/algorithm/util/bitwise.h"

#include <gtest/gtest.h>

#include "qret/base/type.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/intrinsic.h"
#include "qret/runtime/simulator.h"

using namespace qret;
using frontend::CircuitBuilder, runtime::SimulatorConfig, runtime::Simulator;

// Define test cases
// len, control_size, target_size, target_value
class BitOrderRotation
    : public testing::TestWithParam<std::tuple<std::size_t, std::size_t, std::size_t, BigInt>> {};
INSTANTIATE_TEST_SUITE_P(
        LightCase,
        BitOrderRotation,
        testing::Values(
                std::make_tuple(std::size_t{1}, std::size_t{3}, std::size_t{2}, BigInt(1)),
                std::make_tuple(std::size_t{1}, std::size_t{3}, std::size_t{2}, BigInt(2)),
                std::make_tuple(std::size_t{1}, std::size_t{4}, std::size_t{2}, BigInt(1)),
                std::make_tuple(std::size_t{1}, std::size_t{4}, std::size_t{2}, BigInt(2)),
                std::make_tuple(std::size_t{1}, std::size_t{3}, std::size_t{5}, BigInt(5)),
                std::make_tuple(std::size_t{1}, std::size_t{3}, std::size_t{5}, BigInt(11)),
                std::make_tuple(std::size_t{2}, std::size_t{3}, std::size_t{5}, BigInt(5)),
                std::make_tuple(std::size_t{2}, std::size_t{3}, std::size_t{5}, BigInt(11)),
                std::make_tuple(std::size_t{1}, std::size_t{3}, std::size_t{6}, BigInt(5)),
                std::make_tuple(std::size_t{1}, std::size_t{3}, std::size_t{6}, BigInt(27)),
                std::make_tuple(std::size_t{2}, std::size_t{3}, std::size_t{6}, BigInt(5)),
                std::make_tuple(std::size_t{2}, std::size_t{3}, std::size_t{6}, BigInt(27))
        )
);
// control_size, target_size, target_value
class BitOrderReversal
    : public testing::TestWithParam<std::tuple<std::size_t, std::size_t, BigInt>> {};
INSTANTIATE_TEST_SUITE_P(
        LightCase,
        BitOrderReversal,
        testing::Values(
                std::make_tuple(std::size_t{3}, std::size_t{2}, BigInt(1)),
                std::make_tuple(std::size_t{3}, std::size_t{2}, BigInt(2)),
                std::make_tuple(std::size_t{4}, std::size_t{2}, BigInt(1)),
                std::make_tuple(std::size_t{4}, std::size_t{2}, BigInt(2)),
                std::make_tuple(std::size_t{3}, std::size_t{5}, BigInt(5)),
                std::make_tuple(std::size_t{3}, std::size_t{5}, BigInt(11)),
                std::make_tuple(std::size_t{3}, std::size_t{5}, BigInt(5)),
                std::make_tuple(std::size_t{3}, std::size_t{5}, BigInt(11)),
                std::make_tuple(std::size_t{3}, std::size_t{6}, BigInt(5)),
                std::make_tuple(std::size_t{3}, std::size_t{6}, BigInt(27)),
                std::make_tuple(std::size_t{3}, std::size_t{6}, BigInt(5)),
                std::make_tuple(std::size_t{3}, std::size_t{6}, BigInt(27))
        )
);

//--------------------------------------------------//
// Test MultiControlledBitOrderRotation
//--------------------------------------------------//
TEST_P(BitOrderRotation, TestMultiControlledBitOrderRotationGenUsingToffoliStateCtrlTrue) {
    using frontend::gate::MultiControlledBitOrderRotationGen;
    const auto [len, control_size, target_size, target_value] = GetParam();
    const auto aux_size = control_size >= 2 && target_size == 2 ? std::size_t{1} : std::size_t{0};

    ir::IRContext context;
    auto* module = ir::Module::Create("bitwise", context);
    auto builder = CircuitBuilder(module);
    auto gen =
            MultiControlledBitOrderRotationGen(&builder, len, control_size, target_size, aux_size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    for (auto i = std::size_t{0}; i < control_size; ++i) {
        simulator.SetAs1(i);
    }  // ctrl-on
    simulator.SetValue(control_size, target_size, target_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    const auto get = [&target_value](std::size_t i) { return (target_value >> i) & 1; };
    for (auto i = std::size_t{0}; i < target_size; ++i) {
        const auto expected = i < target_size - len ? get(i + len) : get(i - (target_size - len));
        EXPECT_EQ(expected, state.GetValue(control_size + i, 1));
    }
}
TEST_P(BitOrderRotation, TestMultiControlledBitOrderRotationGenUsingToffoliStateCtrlFalse) {
    using frontend::gate::MultiControlledBitOrderRotationGen;
    const auto [len, control_size, target_size, target_value] = GetParam();
    const auto aux_size = control_size >= 2 && target_size == 2 ? std::size_t{1} : std::size_t{0};

    ir::IRContext context;
    auto* module = ir::Module::Create("bitwise", context);
    auto builder = CircuitBuilder(module);
    auto gen =
            MultiControlledBitOrderRotationGen(&builder, len, control_size, target_size, aux_size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    // for (auto i = std::size_t{0}; i < control_size; ++i) { simulator.SetAs1(i); }  // ctrl-off
    simulator.SetValue(control_size, target_size, target_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    const auto get = [&target_value](std::size_t i) { return (target_value >> i) & 1; };
    for (auto i = std::size_t{0}; i < target_size; ++i) {
        EXPECT_EQ(get(i), state.GetValue(control_size + i, 1));
    }
}
//--------------------------------------------------//
// Test MultiControlledBitOrderReversal
//--------------------------------------------------//
TEST_P(BitOrderReversal, TestMultiControlledBitOrderReversalGenUsingToffoliStateCtrlTrue) {
    using frontend::gate::MultiControlledBitOrderReversalGen;
    const auto [control_size, target_size, target_value] = GetParam();
    const auto aux_size = control_size >= 2 && target_size == 2 ? std::size_t{1} : std::size_t{0};

    ir::IRContext context;
    auto* module = ir::Module::Create("bitwise", context);
    auto builder = CircuitBuilder(module);
    auto gen = MultiControlledBitOrderReversalGen(&builder, control_size, target_size, aux_size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    for (auto i = std::size_t{0}; i < control_size; ++i) {
        simulator.SetAs1(i);
    }  // ctrl-on
    simulator.SetValue(control_size, target_size, target_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    const auto get = [&target_value](std::size_t i) { return (target_value >> i) & 1; };
    for (auto i = std::size_t{0}; i < target_size; ++i) {
        const auto expected = get(target_size - 1 - i);
        EXPECT_EQ(expected, state.GetValue(control_size + i, 1));
    }
}
TEST_P(BitOrderReversal, TestMultiControlledBitOrderReversalGenUsingToffoliStateCtrlFalse) {
    using frontend::gate::MultiControlledBitOrderReversalGen;
    const auto [control_size, target_size, target_value] = GetParam();
    const auto aux_size = control_size >= 2 && target_size == 2 ? std::size_t{1} : std::size_t{0};

    ir::IRContext context;
    auto* module = ir::Module::Create("bitwise", context);
    auto builder = CircuitBuilder(module);
    auto gen = MultiControlledBitOrderReversalGen(&builder, control_size, target_size, aux_size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    // for (auto i = std::size_t{0}; i < control_size; ++i) { simulator.SetAs1(i); }  // ctrl-false
    simulator.SetValue(control_size, target_size, target_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    const auto get = [&target_value](std::size_t i) { return (target_value >> i) & 1; };
    for (auto i = std::size_t{0}; i < target_size; ++i) {
        EXPECT_EQ(get(i), state.GetValue(control_size + i, 1));
    }
}
