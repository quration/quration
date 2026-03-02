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
// target_size, aux_size, target_value
class Increment : public testing::TestWithParam<std::tuple<std::size_t, std::size_t, BigInt>> {};
INSTANTIATE_TEST_SUITE_P(
        LightCaseNormal,
        Increment,
        testing::Values(
                std::make_tuple(std::size_t{1}, std::size_t{0}, BigInt(0)),
                std::make_tuple(std::size_t{2}, std::size_t{0}, BigInt(1)),
                std::make_tuple(std::size_t{3}, std::size_t{0}, BigInt(5)),
                std::make_tuple(std::size_t{5}, std::size_t{1}, BigInt(6))
        )
);
INSTANTIATE_TEST_SUITE_P(
        HeavyCaseNormal,
        Increment,
        testing::Values(
                std::make_tuple(std::uint64_t{1000}, std::size_t{1}, BigInt(2347)),
                std::make_tuple(std::uint64_t{1000}, std::size_t{1}, BigInt(2478390)),
                std::make_tuple(std::uint64_t{1001}, std::size_t{1}, BigInt(2347)),
                std::make_tuple(std::uint64_t{1001}, std::size_t{1}, BigInt(2478390))
        )
);
INSTANTIATE_TEST_SUITE_P(
        LightCaseOverflow,
        Increment,
        testing::Values(
                std::make_tuple(std::size_t{1}, std::size_t{0}, BigInt(0)),
                std::make_tuple(std::size_t{2}, std::size_t{0}, BigInt(1)),
                std::make_tuple(std::size_t{3}, std::size_t{0}, BigInt(5)),
                std::make_tuple(std::size_t{5}, std::size_t{1}, BigInt(6))
        )
);
INSTANTIATE_TEST_SUITE_P(
        HeavyCaseOverflow,
        Increment,
        testing::Values(
                std::make_tuple(std::uint64_t{1000}, std::size_t{1}, (BigInt(1) << 1000) - 1),
                std::make_tuple(std::uint64_t{1001}, std::size_t{1}, (BigInt(1) << 1001) - 1)
        )
);

//--------------------------------------------------//
// Test Inc
//--------------------------------------------------//
TEST_P(Increment, TestIncUsingToffoliState) {
    using frontend::gate::IncGen;
    const auto [target_size, aux_size, target_value] = GetParam();
    const auto max = BigInt(1) << target_size;

    ir::IRContext context;
    auto* module = ir::Module::Create("inc", context);
    auto builder = CircuitBuilder(module);
    auto gen = IncGen(&builder, target_size, aux_size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetValue(0, target_size, target_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    const auto target_expected = target_value + 1 == max ? BigInt(0) : BigInt(target_value + 1);
    EXPECT_EQ(target_expected, state.GetValue(0, target_size));
    EXPECT_EQ(0, state.GetValue(target_size, aux_size));
}
