#include "qret/algorithm/util/rotation.h"

#include <gtest/gtest.h>

#include <numbers>

#include "qret/base/type.h"
#include "qret/frontend/argument.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/intrinsic.h"
#include "qret/ir/value.h"
#include "qret/runtime/simulator.h"

using namespace qret;
using frontend::CircuitBuilder, runtime::SimulatorConfig, runtime::Simulator;

//--------------------------------------------------//
// Test RandomizedMultiplexedRotation
//--------------------------------------------------//
TEST(TestRandomizedMultiplexedRotation, UsingFullQuantumState) {
    using qret::frontend::gate::RandomizedMultiplexedRotationGen;
    const auto num_samples = std::size_t{10};
    const auto address_size = std::size_t{3};
    const auto sub_bit_precision = std::size_t{5};
    // radian
    const auto theta = std::vector<ir::FloatWithPrecision>{
            {0.0},
            {1.0},
            {1.5},
            {2.0},
            {std::numbers::pi * 7.0 / 32.0},
            {std::numbers::pi / 2.0},
            {std::numbers::pi},
            {1.5 * std::numbers::pi}
    };
    const auto near = [](const double lhs, const double rhs) {
        const auto eps = 0.000000001;
        return std::abs(lhs - rhs) < eps;
    };

    ir::IRContext context;
    auto* module = ir::Module::Create("rotation", context);
    auto builder = CircuitBuilder(module);

    for (auto i = std::size_t{0}; i < theta.size(); ++i) {
        auto sum = std::complex<double>(0.0, 0.0);
        for (auto _ = std::size_t{0}; _ < num_samples; ++_) {
            auto gen = RandomizedMultiplexedRotationGen(
                    &builder,
                    theta,
                    1,
                    1,
                    sub_bit_precision,
                    314 * _,
                    address_size
            );
            auto* circuit = gen.Generate();

            const auto config = SimulatorConfig{
                    .state_type = SimulatorConfig::StateType::FullQuantum,
                    .use_qulacs = true
            };
            auto simulator = Simulator(config, circuit->GetIR());
            simulator.SetValue(1, address_size, i);
            simulator.RunAll();

            const auto& state = simulator.GetFullQuantumState();
            const auto result = state.GetStateVector()[2 * i];
            const auto arg = std::arg(result) / std::numbers::pi;
            EXPECT_NEAR(1.0, std::abs(result), 0.0000001);
            EXPECT_TRUE(
                    near(std::pow(2.0, sub_bit_precision) * arg, 0.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 1.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 2.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 3.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 4.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 5.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 6.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 7.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 8.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 9.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 10.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 11.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 12.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 13.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 14.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 15.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 16.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 17.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 18.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 19.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 20.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 21.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 22.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 23.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 24.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 25.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 26.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 27.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 28.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 29.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 30.0)
                    || near(std::pow(2.0, sub_bit_precision) * arg, 31.0)
            ) << std::pow(2.0, sub_bit_precision) * arg;
            sum += result;
        }
        const auto expected = std::complex<double>(
                std::cos(theta[i].value / 2.0),
                std::sin(theta[i].value / 2.0)
        );
        const auto ave = sum / static_cast<double>(num_samples);
        std::cout << expected << ", " << ave << ", abs-diff: " << std::abs(expected - ave)
                  << std::endl;
        // const auto eps = std::pow(0.5, 2 * sub_bit_precision);
        // EXPECT_LT(std::abs(expected - ave), eps);
        EXPECT_LT(std::abs(expected - ave), 0.2);
    }
}
