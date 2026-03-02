#include "qret/algorithm/preparation/uniform.h"

#include <gtest/gtest.h>

#include "qret/base/type.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/intrinsic.h"
#include "qret/runtime/simulator.h"

using namespace qret;
using frontend::CircuitBuilder, runtime::SimulatorConfig, runtime::Simulator;

template <typename T>
std::map<T, std::uint32_t> CalcFreqMap(const std::vector<T>& vec) {
    auto mp = std::map<T, std::uint32_t>();
    for (const auto& x : vec) {
        if (mp.contains(x)) {
            mp[x]++;
        } else {
            mp[x] = 1;
        }
    }
    return mp;
}

//--------------------------------------------------//
// Test PrepareUniformSuperposition
//--------------------------------------------------//
TEST(TestPrepareUniformSuperposition, UsingFullQuantumState) {
    using frontend::gate::PrepareUniformSuperpositionGen;
    constexpr auto Eps = 0.00000001;
    constexpr auto NumSamples = std::size_t{100000};

    for (auto N = 17; N <= 32; ++N) {
        ir::IRContext context;
        auto* module = ir::Module::Create("uniform", context);
        auto builder = CircuitBuilder(module);
        auto gen = PrepareUniformSuperpositionGen(&builder, N);
        auto* circuit = gen.Generate();

        const auto config = SimulatorConfig{
                .state_type = SimulatorConfig::StateType::FullQuantum,
                .use_qulacs = true
        };
        auto simulator = Simulator(config, circuit->GetIR());
        simulator.RunAll();

        const auto& state = simulator.GetFullQuantumState();

        // test entropy
        const auto x = static_cast<double>(N);
        EXPECT_NEAR(std::log(x), state.GetEntropy(), Eps);

        // test sampling results
        const auto samples = state.Sampling(NumSamples);
        const auto mp = CalcFreqMap(samples);
        const auto min = NumSamples / N / 2;
        const auto max = NumSamples / N * 2;
        EXPECT_EQ(N, mp.size());
        for (const auto& [key, value] : mp) {
            EXPECT_GT(N, key);  // N > key
            // TODO: test value is about NumSamples/N in more correct way
            EXPECT_GT(max, value);
            EXPECT_GT(value, min);
        }
    }
}
//--------------------------------------------------//
// Test ControlledPrepareUniformSuperposition
//--------------------------------------------------//
TEST(TestControlledPrepareUniformSuperposition, UsingFullQuantumStateCtrlTrue) {
    using frontend::gate::ControlledPrepareUniformSuperpositionGen;
    constexpr auto Eps = 0.00000001;
    constexpr auto NumSamples = std::size_t{100000};

    for (auto N = 17; N <= 32; ++N) {
        ir::IRContext context;
        auto* module = ir::Module::Create("uniform", context);
        auto builder = CircuitBuilder(module);
        auto gen = ControlledPrepareUniformSuperpositionGen(&builder, N);
        auto* circuit = gen.Generate();

        const auto config = SimulatorConfig{
                .state_type = SimulatorConfig::StateType::FullQuantum,
                .use_qulacs = true
        };
        auto simulator = Simulator(config, circuit->GetIR());
        simulator.SetAs1(0);  // ctrl-on
        simulator.RunAll();

        const auto& state = simulator.GetFullQuantumState();

        // test entropy
        const auto x = static_cast<double>(N);
        EXPECT_NEAR(std::log(x), state.GetEntropy(), Eps);

        // test sampling results
        const auto samples = state.Sampling(NumSamples);
        const auto mp = CalcFreqMap(samples);
        const auto min = NumSamples / N / 2;
        const auto max = NumSamples / N * 2;
        EXPECT_EQ(N, mp.size());
        for (const auto& [tmp_key, value] : mp) {
            EXPECT_EQ(1, tmp_key % 2);
            const auto key = tmp_key / 2;
            EXPECT_GT(N, key);  // N > key
            // TODO: test value is about NumSamples/N in more correct way
            EXPECT_GT(max, value);
            EXPECT_GT(value, min);
        }
    }
}
TEST(TestControlledPrepareUniformSuperposition, UsingFullQuantumStateCtrlFalse) {
    using frontend::gate::ControlledPrepareUniformSuperpositionGen;
    constexpr auto Eps = 0.00000001;
    constexpr auto NumSamples = std::size_t{100000};

    for (auto N = 17; N <= 32; ++N) {
        ir::IRContext context;
        auto* module = ir::Module::Create("uniform", context);
        auto builder = CircuitBuilder(module);
        auto gen = ControlledPrepareUniformSuperpositionGen(&builder, N);
        auto* circuit = gen.Generate();

        const auto config = SimulatorConfig{
                .state_type = SimulatorConfig::StateType::FullQuantum,
                .use_qulacs = true
        };
        auto simulator = Simulator(config, circuit->GetIR());
        // simulator.SetAs1(0);  // ctrl-off
        simulator.RunAll();

        const auto& state = simulator.GetFullQuantumState();

        // test entropy
        EXPECT_NEAR(0, state.GetEntropy(), Eps);

        // test zero probability
        for (auto i = 0; i < gen.n; ++i) {
            EXPECT_TRUE(state.IsClean(1 + i));
        }
    }
}
