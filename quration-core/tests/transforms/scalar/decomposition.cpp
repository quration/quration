#include "qret/transforms/scalar/decomposition.h"

#include <gtest/gtest.h>

#include <fmt/format.h>

#include "qret/base/gridsynth.h"
#include "qret/base/type.h"
#include "qret/runtime/simulator.h"

#include "test_utils.h"

using namespace qret;
using ir::DecomposeInst;
using runtime::SimulatorConfig, runtime::Simulator;

namespace {
std::string AddCuccaroPath(std::size_t size) {
    return fmt::format("quration-core/tests/data/circuit/add_cuccaro_{}.json", size);
}
std::string AddCuccaroName(std::size_t size) {
    return fmt::format("AddCuccaro({})", size);
}
std::string AddCraigPath(std::size_t size) {
    return fmt::format("quration-core/tests/data/circuit/add_craig_{}.json", size);
}
std::string AddCraigName(std::size_t size) {
    return fmt::format("AddCraig({})", size);
}
std::string UniformPath(std::uint32_t n) {
    return fmt::format("quration-core/tests/data/circuit/uniform/prepare_uniform_{}.json", n);
}
std::string UniformName(std::uint32_t n) {
    return fmt::format("PrepareUniformSuperposition({})", n);
}
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

// Define test cases
class Addition : public testing::TestWithParam<std::tuple<std::uint64_t, BigInt, BigInt>> {};
INSTANTIATE_TEST_SUITE_P(
        LightCaseNormal,
        Addition,
        testing::Values(
                std::make_tuple(std::uint64_t{1}, BigInt(0), BigInt(1)),  // 0+=1(mod 2^1)
                std::make_tuple(std::uint64_t{2}, BigInt(2), BigInt(1)),  // 2+=1(mod 2^2)
                std::make_tuple(std::uint64_t{5}, BigInt(10), BigInt(6))  // 10+=6(mod 2^5)
        )
);
INSTANTIATE_TEST_SUITE_P(
        HeavyCaseNormal,
        Addition,
        testing::Values(
                std::make_tuple(std::uint64_t{1000}, BigInt(42789), BigInt(2347)),
                std::make_tuple(std::uint64_t{1000}, BigInt(1) << 900, BigInt(2478390))
        )
);
INSTANTIATE_TEST_SUITE_P(
        LightCaseOverflow,
        Addition,
        testing::Values(
                std::make_tuple(std::size_t{1}, BigInt(1), BigInt(1)),  // 1+=1(mod 2^1)
                std::make_tuple(std::size_t{2}, BigInt(2), BigInt(3)),  // 2+=3(mod 2^2)
                std::make_tuple(std::size_t{5}, BigInt(10), BigInt(26)),  // 10+=26(mod 2^5)
                std::make_tuple(std::size_t{5}, BigInt(16), BigInt(16))  // 16+=16(mod 2^5)
        )
);
INSTANTIATE_TEST_SUITE_P(
        HeavyCaseOverflow,
        Addition,
        testing::Values(
                std::make_tuple(std::size_t{1000}, (BigInt(1) << 1000) - 1, BigInt(2347)),
                std::make_tuple(std::size_t{1000}, BigInt(1) << 999, BigInt(1) << 999)
        )
);

TEST_P(Addition, DecomposeAddCuccaro) {
    const auto [size, dst_value, src_value] = GetParam();
    const auto max = BigInt(1) << size;

    ir::IRContext context;
    auto* circuit =
            tests::LoadCircuitFromJsonFile(AddCuccaroPath(size), AddCuccaroName(size), context);
    ASSERT_NE(nullptr, circuit);
    DecomposeInst().RunOnFunction(*circuit);

    auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 8;
    auto simulator = Simulator(config, circuit);
    simulator.SetValue(0, size, dst_value);
    simulator.SetValue(size, size, src_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    const BigInt sum = dst_value + src_value;
    const auto dst_expected = sum >= max ? BigInt(sum - max) : sum;
    EXPECT_EQ(dst_expected, state.GetValue(0, size));
    EXPECT_EQ(src_value, state.GetValue(size, size));
}
TEST_P(Addition, DecomposeAddCraig) {
    const auto [size, dst_value, src_value] = GetParam();
    const auto max = BigInt(1) << size;

    ir::IRContext context;
    auto* circuit = tests::LoadCircuitFromJsonFile(AddCraigPath(size), AddCraigName(size), context);
    ASSERT_NE(nullptr, circuit);
    DecomposeInst().RunOnFunction(*circuit);

    auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 2;
    auto simulator = Simulator(config, circuit);
    simulator.SetValue(0, size, dst_value);
    simulator.SetValue(size, size, src_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    const BigInt sum = dst_value + src_value;
    const auto dst_expected = sum >= max ? BigInt(sum - max) : sum;
    EXPECT_EQ(dst_expected, state.GetValue(0, size));
    EXPECT_EQ(src_value, state.GetValue(size, size));
}

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
TEST(Prepare, DecomposePrepareUniformSuperposition) {
    constexpr auto Eps = 0.00000001;
    constexpr auto NumSamples = std::size_t{100000};

    if (!IsGridSynthRunnable()) {
        GTEST_SKIP() << "Skip GridSynth test";
    }

    for (auto N = 2; N <= 32; ++N) {
        ir::IRContext context;
        auto* circuit = tests::LoadCircuitFromJsonFile(UniformPath(N), UniformName(N), context);
        ASSERT_NE(nullptr, circuit);
        DecomposeInst().RunOnFunction(*circuit);

        const auto config = SimulatorConfig{
                .state_type = SimulatorConfig::StateType::FullQuantum,
                .use_qulacs = true
        };
        auto simulator = Simulator(config, circuit);
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

TEST_P(Rotation, DecomposeRTarget0) {
    static constexpr auto Eps = 0.0001;
    const auto [theta, precision] = GetParam();

    if (!IsGridSynthRunnable()) {
        GTEST_SKIP() << "Skip GridSynth test";
    }

    for (auto pauli : {'X', 'Y', 'Z', '1'}) {
        ir::IRContext context;
        auto* circuit = tests::LoadCircuitFromJsonFile(
                RotationPath(pauli, theta, precision),
                "TestR__000000000__",
                context
        );
        ASSERT_NE(nullptr, circuit);
        DecomposeInst().RunOnFunction(*circuit);

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
        } else if (pauli == 'Z') {
            e0 = {std::cos(-theta / 2), std::sin(-theta / 2)};
            e1 = 0.0;
        } else if (pauli == '1') {
            e0 = 1.0;
            e1 = 0.0;
        }
    }
}
TEST_P(Rotation, DecomposeRTarget1) {
    static constexpr auto Eps = 0.0001;
    const auto [theta, precision] = GetParam();

    if (!IsGridSynthRunnable()) {
        GTEST_SKIP() << "Skip GridSynth test";
    }

    for (auto pauli : {'X', 'Y', 'Z', '1'}) {
        ir::IRContext context;
        auto* circuit = tests::LoadCircuitFromJsonFile(
                RotationPath(pauli, theta, precision),
                "TestR__000000000__",
                context
        );
        ASSERT_NE(nullptr, circuit);
        DecomposeInst().RunOnFunction(*circuit);

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
        } else if (pauli == 'Z') {
            e0 = 0.0;
            e1 = {std::cos(theta / 2), std::sin(theta / 2)};
        } else if (pauli == '1') {
            e0 = 0.0;
            e1 = {std::cos(theta), std::sin(theta)};
        }
    }
}
