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
// size, mul, value
class Multiplication : public testing::TestWithParam<std::tuple<std::uint64_t, BigInt, BigInt>> {};
INSTANTIATE_TEST_SUITE_P(
        LightCase,
        Multiplication,
        testing::Values(
                std::make_tuple(std::uint64_t{5}, BigInt(3), BigInt(1)),
                std::make_tuple(std::uint64_t{5}, BigInt(3), BigInt(6))
        )
);
INSTANTIATE_TEST_SUITE_P(
        HeavyCase,
        Multiplication,
        testing::Values(
                std::make_tuple(std::uint64_t{31}, BigInt(15), BigInt(2347)),
                std::make_tuple(std::uint64_t{31}, BigInt(15), BigInt(1) << 29)
        )
);

//--------------------------------------------------//
// Test MulW
//--------------------------------------------------//
TEST_P(Multiplication, TestMulWUsingToffoliState) {
    using frontend::gate::MulWGen;
    const auto window_size = std::size_t{3};
    const auto [size, mul, value] = GetParam();
    const auto max = BigInt(1) << size;

    ir::IRContext context;
    auto* module = ir::Module::Create("mul", context);
    auto builder = CircuitBuilder(module);
    auto gen = MulWGen(&builder, window_size, mul, size);
    auto* circuit = gen.Generate();

    auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 8;
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetValue(0, size, value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ((mul * value) % max, state.GetValue(0, size));
}
