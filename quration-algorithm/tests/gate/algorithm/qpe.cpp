#include "qret/algorithm/phase_estimation/qpe.h"

#include <gtest/gtest.h>

#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/intrinsic.h"
#include "qret/math/integer.h"
#include "qret/math/lcu_helper.h"
#include "qret/runtime/simulator.h"

using namespace qret;
using frontend::CircuitBuilder, runtime::SimulatorConfig, runtime::Simulator;

//--------------------------------------------------//
// Test PREPARE
//--------------------------------------------------//
TEST(TestPREPARE, UsingFullQuantumState) {
    using frontend::gate::PREPAREGen;
    const auto lcu_coefficients = std::vector<double>{0.1, 0.2, 0.3, 0.4, 0.5, 0.6};
    const auto size = math::BitSizeI(lcu_coefficients.size() - 1);
    const auto mask = (std::size_t{1} << size) - 1;
    const auto sub_bit_precision = 5;
    const auto alias_sampling = math::PreprocessLCUCoefficientsForReversibleSampling(
            lcu_coefficients,
            sub_bit_precision
    );

    ir::IRContext context;
    auto* module = ir::Module::Create("PREPARE", context);
    auto builder = CircuitBuilder(module);
    auto gen = PREPAREGen(&builder, alias_sampling, size, sub_bit_precision);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{
            .state_type = SimulatorConfig::StateType::FullQuantum,
            .use_qulacs = true
    };
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.RunAll();
    const auto& state = simulator.GetFullQuantumState();

    const auto eps = 0.2;
    const auto* state_vec = state.GetStateVector();
    auto reduced = std::vector<double>(1 << size);
    for (auto i = std::size_t{0}; i < (std::size_t{1} << circuit->GetIR()->GetNumQubits()); ++i) {
        reduced[i & mask] += std::norm(state_vec[i]);
    }
    for (auto i = std::size_t{1}; i < lcu_coefficients.size(); ++i) {
        const auto exp_ratio = lcu_coefficients[i] / lcu_coefficients[0];
        const auto act_ratio = reduced[i] / reduced[0];
        EXPECT_NEAR(exp_ratio, act_ratio, eps);
    }
}
