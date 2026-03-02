#include <gtest/gtest.h>

#include "qret/algorithm/arithmetic/integer.h"
#include "qret/base/type.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/intrinsic.h"
#include "qret/runtime/full_quantum_state.h"
#include "qret/runtime/simulator.h"

using namespace qret;
using frontend::CircuitBuilder, runtime::SimulatorConfig, runtime::Simulator;

// Define test cases
// size, scale, dst_value, src_value
class ScaledAddition
    : public testing::TestWithParam<std::tuple<std::uint64_t, BigInt, BigInt, BigInt>> {};
INSTANTIATE_TEST_SUITE_P(
        LightCase,
        ScaledAddition,
        testing::Values(
                std::make_tuple(
                        std::uint64_t{5},
                        BigInt(3),
                        BigInt(2),
                        BigInt(1)
                ),  // 2+=3*1(mod 2^5)
                std::make_tuple(
                        std::uint64_t{5},
                        BigInt(3),
                        BigInt(10),
                        BigInt(6)
                )  // 10+=3*6(mod 2^5)
        )
);
INSTANTIATE_TEST_SUITE_P(
        HeavyCase,
        ScaledAddition,
        testing::Values(
                std::make_tuple(std::uint64_t{31}, BigInt(15), BigInt(42789), BigInt(2347)),
                std::make_tuple(std::uint64_t{31}, BigInt(15), BigInt(1) << 29, BigInt(2478390))
        )
);

//--------------------------------------------------//
// Test ScaledAdd
//--------------------------------------------------//
TEST_P(ScaledAddition, TestScaledAddUsingToffoliState) {
    using frontend::gate::ScaledAddGen;
    const auto [size, scale, dst_value, src_value] = GetParam();
    const auto max = BigInt(1) << size;

    ir::IRContext context;
    auto* module = ir::Module::Create("scale_add", context);
    auto builder = CircuitBuilder(module);
    auto gen = ScaledAddGen(&builder, scale, size);
    auto* circuit = gen.Generate();

    auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 2;
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetValue(0, size, dst_value);
    simulator.SetValue(size, size, src_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    const BigInt sum = dst_value + scale * src_value;
    EXPECT_EQ(sum % max, state.GetValue(0, size));
    EXPECT_EQ(src_value, state.GetValue(size, size));
}
//--------------------------------------------------//
// Test ScaledAddW
//--------------------------------------------------//
TEST_P(ScaledAddition, TestScaledAddWUsingToffoliState) {
    using frontend::gate::ScaledAddWGen;
    const auto [size, scale, dst_value, src_value] = GetParam();
    const auto num_windows = std::size_t{3};
    const auto max = BigInt(1) << size;

    ir::IRContext context;
    auto* module = ir::Module::Create("scale_add", context);
    auto builder = CircuitBuilder(module);
    auto gen = ScaledAddWGen(&builder, num_windows, scale, size);
    auto* circuit = gen.Generate();

    auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 8;
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetValue(0, size, dst_value);
    simulator.SetValue(size, size, src_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    const BigInt sum = dst_value + scale * src_value;
    EXPECT_EQ(sum % max, state.GetValue(0, size));
    EXPECT_EQ(src_value, state.GetValue(size, size));
}
