#include "qret/algorithm/preparation/arbitrary.h"

#include <gtest/gtest.h>

#include <random>

#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/intrinsic.h"
#include "qret/runtime/simulator.h"

using namespace qret;
using frontend::CircuitBuilder, runtime::SimulatorConfig, runtime::Simulator;

//--------------------------------------------------//
// Test PrepareArbitraryState
//--------------------------------------------------//
TEST(TestPrepareArbitraryState, UsingFullQuantumState) {
    using frontend::gate::PrepareArbitraryStateGen;
    const auto size = std::size_t{5};
    const auto mask = (std::size_t{1} << size) - 1;
    const auto precision = std::size_t{10};
    auto probs = std::vector<double>((std::size_t{1} << size) - 5);
    {
        auto engine = std::mt19937_64{0};
        auto dist = std::uniform_real_distribution<>(0.1, 1.0);
        std::generate(probs.begin(), probs.end(), [&engine, &dist]() { return dist(engine); });
        const auto sum = std::accumulate(probs.begin(), probs.end(), 0.0);
        std::for_each(probs.begin(), probs.end(), [sum](auto& x) { x /= sum; });
    }

    ir::IRContext context;
    auto* module = ir::Module::Create("arbitrary", context);
    auto builder = CircuitBuilder(module);
    auto gen = PrepareArbitraryStateGen(&builder, probs, size, precision);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{
            .state_type = SimulatorConfig::StateType::FullQuantum,
            .use_qulacs = true
    };
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.RunAll();

    const auto& state = simulator.GetFullQuantumState();
    const auto eps = 0.01;
    const auto* state_vec = state.GetStateVector();
    auto reduced = std::vector<double>(std::size_t{1} << size);
    for (auto i = std::size_t{0}; i < (std::size_t{1} << size); ++i) {
        reduced[i & mask] += std::norm(state_vec[i]);
    }
    for (auto i = std::size_t{0}; i < std::size_t{1} << size; ++i) {
        if (i < probs.size()) {
            EXPECT_NEAR(probs[i], reduced[i], eps);
        } else {
            EXPECT_NEAR(0.0, reduced[i], eps);
        }
    }
}
