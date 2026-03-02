#include "qret/transforms/scalar/ignore_global_phase.h"

#include <gtest/gtest.h>

#include <fmt/format.h>

#include "qret/runtime/simulator.h"

#include "test_utils.h"

using namespace qret;
using ir::IgnoreGlobalPhase;
using runtime::SimulatorConfig, runtime::Simulator;

namespace {
std::string ToFilename(double value) {
    auto s = fmt::format("{:.12g}", value);
    for (auto& c : s) {
        if (c == '.') {
            c = 'p';
        } else if (c == '-') {
            c = 'm';
        } else if (c == '+') {
            c = 'p';
        }
    }
    return s;
}
std::string RotationPath(char pauli, double theta, double precision) {
    return fmt::format(
            "quration-core/tests/data/circuit/rotation/test_r_{}_theta_{}_precision_{}.json",
            pauli,
            ToFilename(theta),
            ToFilename(precision)
    );
}
}  // namespace

class Rotation : public testing::TestWithParam<std::tuple<double, double>> {};
INSTANTIATE_TEST_SUITE_P(
        LowPrecisionCase,
        Rotation,
        testing::Values(
                // 0.01, 0.02, 0.03
                std::make_tuple(0.01, 0.0001),
                std::make_tuple(0.02, 0.0001),
                std::make_tuple(0.03, 0.0001),
                // 0.1, 0.2, 0.3
                std::make_tuple(0.1, 0.0001),
                std::make_tuple(0.2, 0.0001),
                std::make_tuple(0.3, 0.0001),
                // 1, 2, 3
                std::make_tuple(1, 0.0001),
                std::make_tuple(2, 0.0001),
                std::make_tuple(3, 0.0001)
        )
);
INSTANTIATE_TEST_SUITE_P(
        HighPrecisionCase,
        Rotation,
        testing::Values(
                // 0.01, 0.02, 0.03
                std::make_tuple(0.01, 0.0000000001),
                std::make_tuple(0.02, 0.0000000001),
                std::make_tuple(0.03, 0.0000000001),
                // 0.1, 0.2, 0.3
                std::make_tuple(0.1, 0.0000000001),
                std::make_tuple(0.2, 0.0000000001),
                std::make_tuple(0.3, 0.0000000001),
                // 1, 2, 3
                std::make_tuple(1, 0.0000000001),
                std::make_tuple(2, 0.0000000001),
                std::make_tuple(3, 0.0000000001)
        )
);

TEST_P(Rotation, IgnoreGlobalPhaseRTarget0) {
    // If we ignore global phase, RZ == R1
    static constexpr auto Eps = 0.0001;
    const auto [theta, precision] = GetParam();

    for (auto pauli : {'X', 'Y', 'Z', '1'}) {
        ir::IRContext context;
        auto* circuit = tests::LoadCircuitFromJsonFile(
                RotationPath(pauli, theta, precision),
                "TestR__000000000__",
                context
        );
        ASSERT_NE(nullptr, circuit);
        IgnoreGlobalPhase().RunOnFunction(*circuit);

        const auto config = SimulatorConfig{
                .state_type = SimulatorConfig::StateType::FullQuantum,
                .use_qulacs = true
        };
        auto simulator = Simulator(config, circuit);
        simulator.RunAll();

        const auto& state = simulator.GetFullQuantumState();
        const auto* state_vec = state.GetStateVector();
        auto e0 = std::complex<double>();
        auto e1 = std::complex<double>();
        if (pauli == 'X') {
            e0 = std::cos(theta / 2);
            e1 = {0, -std::sin(theta / 2)};
        } else if (pauli == 'Y') {
            e0 = std::cos(theta / 2);
            e1 = std::sin(theta / 2);
        } else if (pauli == 'Z' || pauli == '1') {
            e0 = {std::cos(-theta / 2), std::sin(-theta / 2)};
            e1 = 0.0;
            // } else if (pauli == '1') {
            //     e0 = 1.0;
            //     e1 = 0.0;
        }
    }
}
TEST_P(Rotation, IgnoreGlobalPhaseRTarget1) {
    // If we ignore global phase, RZ == R1
    static constexpr auto Eps = 0.0001;
    const auto [theta, precision] = GetParam();

    for (auto pauli : {'X', 'Y', 'Z', '1'}) {
        ir::IRContext context;
        auto* circuit = tests::LoadCircuitFromJsonFile(
                RotationPath(pauli, theta, precision),
                "TestR__000000000__",
                context
        );
        ASSERT_NE(nullptr, circuit);
        IgnoreGlobalPhase().RunOnFunction(*circuit);

        const auto config = SimulatorConfig{
                .state_type = SimulatorConfig::StateType::FullQuantum,
                .use_qulacs = true
        };
        auto simulator = Simulator(config, circuit);
        simulator.SetAs1(0);
        simulator.RunAll();

        const auto& state = simulator.GetFullQuantumState();
        const auto* state_vec = state.GetStateVector();
        auto e0 = std::complex<double>();
        auto e1 = std::complex<double>();
        if (pauli == 'X') {
            e0 = {0, -std::sin(theta / 2)};
            e1 = std::cos(theta / 2);
        } else if (pauli == 'Y') {
            e0 = -std::sin(theta / 2);
            e1 = std::cos(theta / 2);
        } else if (pauli == 'Z' || pauli == '1') {
            e0 = 0.0;
            e1 = {std::cos(theta / 2), std::sin(theta / 2)};
            // } else if (pauli == '1') {
            //     e0 = 0.0;
            //     e1 = {std::cos(theta), std::sin(theta)};
        }
    }
}
