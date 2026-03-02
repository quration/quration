#include "qret/algorithm/util/multi_control.h"

#include <gtest/gtest.h>

#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/intrinsic.h"
#include "qret/runtime/simulator.h"

using namespace qret;
using frontend::CircuitBuilder, runtime::SimulatorConfig, runtime::Simulator;

// Tuple: control_size, target_size, aux_size, control_value
class MultiControl
    : public testing::TestWithParam<std::tuple<std::size_t, std::size_t, std::size_t, BigInt>> {};
INSTANTIATE_TEST_SUITE_P(
        LightCaseNoX,
        MultiControl,
        testing::Values(
                std::make_tuple(std::size_t{1}, std::size_t{5}, std::size_t{0}, 0),
                std::make_tuple(std::size_t{2}, std::size_t{5}, std::size_t{0}, 1),
                std::make_tuple(std::size_t{3}, std::size_t{5}, std::size_t{0}, 2),
                std::make_tuple(std::size_t{3}, std::size_t{1}, std::size_t{1}, 2),
                std::make_tuple(std::size_t{4}, std::size_t{5}, std::size_t{0}, 4),
                std::make_tuple(std::size_t{4}, std::size_t{1}, std::size_t{1}, 5)
        )
);
INSTANTIATE_TEST_SUITE_P(
        LightCaseDoX,
        MultiControl,
        testing::Values(
                std::make_tuple(std::size_t{1}, std::size_t{5}, std::size_t{0}, 1),
                std::make_tuple(std::size_t{2}, std::size_t{5}, std::size_t{0}, 3),
                std::make_tuple(std::size_t{3}, std::size_t{5}, std::size_t{0}, 7),
                std::make_tuple(std::size_t{3}, std::size_t{1}, std::size_t{1}, 7),
                std::make_tuple(std::size_t{4}, std::size_t{5}, std::size_t{0}, 15),
                std::make_tuple(std::size_t{4}, std::size_t{1}, std::size_t{1}, 15)
        )
);
TEST_P(MultiControl, TestMCMXUsingToffoliState) {
    using frontend::gate::MCMXGen;
    const auto [control_size, target_size, aux_size, control_value] = GetParam();

    ir::IRContext context;
    auto* module = ir::Module::Create("multi_control", context);
    auto builder = CircuitBuilder(module);
    auto gen = MCMXGen(&builder, control_size, target_size, aux_size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetValue(0, control_size, control_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    const auto target_expected = control_value == (BigInt(1) << control_size) - 1
            ? (BigInt(1) << target_size) - 1
            : BigInt();
    EXPECT_EQ(control_value, state.GetValue(0, control_size));
    EXPECT_EQ(target_expected, state.GetValue(control_size, target_size));
}
