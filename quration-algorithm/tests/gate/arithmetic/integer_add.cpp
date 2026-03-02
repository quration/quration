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

//--------------------------------------------------//
// Test Add
//--------------------------------------------------//
TEST_P(Addition, TestAddUsingToffoliState) {
    using frontend::gate::AddGen;
    const auto [size, dst_value, src_value] = GetParam();
    const auto max = BigInt(1) << size;

    ir::IRContext context;
    auto* module = ir::Module::Create("add", context);
    auto builder = CircuitBuilder(module);
    auto gen = AddGen(&builder, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetValue(0, size, dst_value);
    simulator.SetValue(size, size, src_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    const BigInt sum = dst_value + src_value;
    const auto dst_expected = sum >= max ? BigInt(sum - max) : sum;
    EXPECT_EQ(dst_expected, state.GetValue(0, size));
    EXPECT_EQ(src_value, state.GetValue(size, size));
}
//--------------------------------------------------//
// Test AddBrentKung
//--------------------------------------------------//
TEST_P(Addition, TestAddBrentKung) {
    using frontend::gate::AddBrentKungGen;
    const auto [size, dst_value, src_value] = GetParam();
    const auto max = BigInt(1) << size;

    ir::IRContext context;
    auto* module = ir::Module::Create("add", context);
    auto builder = CircuitBuilder(module);
    auto gen = AddBrentKungGen(&builder, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetValue(0, size, dst_value);
    simulator.SetValue(size, size, src_value);
    simulator.SetValue(2 * size, 5 * size, 0);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    const BigInt sum = dst_value + src_value;
    const auto dst_expected = sum >= max ? BigInt(sum - max) : sum;
    EXPECT_EQ(dst_expected, state.GetValue(2 * size, size));
}
//--------------------------------------------------//
// Test AddCuccaro
//--------------------------------------------------//
TEST_P(Addition, TestAddCuccaroUsingToffoliState) {
    using frontend::gate::AddCuccaroGen;
    const auto [size, dst_value, src_value] = GetParam();
    const auto max = BigInt(1) << size;

    ir::IRContext context;
    auto* module = ir::Module::Create("cuccaro", context);
    auto builder = CircuitBuilder(module);
    auto gen = AddCuccaroGen(&builder, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetValue(0, size, dst_value);
    simulator.SetValue(size, size, src_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    const BigInt sum = dst_value + src_value;
    const auto dst_expected = sum >= max ? BigInt(sum - max) : sum;
    EXPECT_EQ(dst_expected, state.GetValue(0, size));
    EXPECT_EQ(src_value, state.GetValue(size, size));
    EXPECT_EQ(0, state.GetValue(2 * size, 1));
}
//--------------------------------------------------//
// Test AddCraig
//--------------------------------------------------//
TEST_P(Addition, TestAddCraigUsingToffoliState) {
    using frontend::gate::AddCraigGen;
    const auto [size, dst_value, src_value] = GetParam();
    const auto max = BigInt(1) << size;

    ir::IRContext context;
    auto* module = ir::Module::Create("craig", context);
    auto builder = CircuitBuilder(module);
    auto gen = AddCraigGen(&builder, size);
    auto* circuit = gen.Generate();

    auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 2;
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetValue(0, size, dst_value);
    simulator.SetValue(size, size, src_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    const BigInt sum = dst_value + src_value;
    const auto dst_expected = sum >= max ? BigInt(sum - max) : sum;
    EXPECT_EQ(dst_expected, state.GetValue(0, size));
    EXPECT_EQ(src_value, state.GetValue(size, size));
    EXPECT_EQ(0, state.GetValue(2 * size, size - 1));
}
//--------------------------------------------------//
// Test ControlledAddCraig
//--------------------------------------------------//
TEST_P(Addition, TestAddControlledCraigUsingToffoliStateCtrlTrue) {
    using frontend::gate::ControlledAddCraigGen;
    const auto [size, dst_value, src_value] = GetParam();
    const auto max = BigInt(1) << size;

    ir::IRContext context;
    auto* module = ir::Module::Create("craig", context);
    auto builder = CircuitBuilder(module);
    auto gen = ControlledAddCraigGen(&builder, size);
    auto* circuit = gen.Generate();

    auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 2;
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetAs1(0);  // ctrl on
    simulator.SetValue(1, size, dst_value);
    simulator.SetValue(1 + size, size, src_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    const BigInt sum = dst_value + src_value;
    const auto dst_expected = sum >= max ? BigInt(sum - max) : sum;
    EXPECT_EQ(1, state.GetValue(0, 1));
    EXPECT_EQ(dst_expected, state.GetValue(1, size));
    EXPECT_EQ(src_value, state.GetValue(1 + size, size));
    EXPECT_EQ(0, state.GetValue(1 + 2 * size, size));
}
TEST_P(Addition, TestAddControlledCraigUsingToffoliStateCtrlFalse) {
    using frontend::gate::ControlledAddCraigGen;
    const auto [size, dst_value, src_value] = GetParam();
    const auto max = BigInt(1) << size;

    ir::IRContext context;
    auto* module = ir::Module::Create("craig", context);
    auto builder = CircuitBuilder(module);
    auto gen = ControlledAddCraigGen(&builder, size);
    auto* circuit = gen.Generate();

    auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 2;
    auto simulator = Simulator(config, circuit->GetIR());
    // simulator.SetAs1(0);  // ctrl off
    simulator.SetValue(1, size, dst_value);
    simulator.SetValue(1 + size, size, src_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(0, state.GetValue(0, 1));
    EXPECT_EQ(dst_value, state.GetValue(1, size));
    EXPECT_EQ(src_value, state.GetValue(1 + size, size));
    EXPECT_EQ(0, state.GetValue(1 + 2 * size, size));
}
//--------------------------------------------------//
// Test AddGeneral
//--------------------------------------------------//
TEST_P(Addition, TestAddGeneralUsingToffoliState) {
    using frontend::gate::AddGeneralGen;
    const auto [size, dst_value, src_value] = GetParam();
    if (size < 2) {
        return;
    }
    const auto dst_size = size + 3;

    ir::IRContext context;
    auto* module = ir::Module::Create("add_general", context);
    auto builder = CircuitBuilder(module);
    auto gen = AddGeneralGen(&builder, dst_size, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetValue(0, dst_size, dst_value);
    simulator.SetValue(dst_size, size, src_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(dst_value + src_value, state.GetValue(0, dst_size));
    EXPECT_EQ(src_value, state.GetValue(dst_size, size));
}
//--------------------------------------------------//
// Test MultiControlledAddGeneral
//--------------------------------------------------//
TEST_P(Addition, TestMultiControlledAddGeneralUsingToffoliStateCtrlTrue) {
    using frontend::gate::MultiControlledAddGeneralGen;
    const auto [size, dst_value, src_value] = GetParam();
    if (size < 2) {
        return;
    }
    const auto dst_size = size + 3;

    ir::IRContext context;
    auto* module = ir::Module::Create("add_general", context);
    auto builder = CircuitBuilder(module);
    auto gen = MultiControlledAddGeneralGen(&builder, 1, dst_size, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetAs1(0);  // ctrl on
    simulator.SetValue(1, dst_size, dst_value);
    simulator.SetValue(1 + dst_size, size, src_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(1, state.GetValue(0, 1));
    EXPECT_EQ(dst_value + src_value, state.GetValue(1, dst_size));
    EXPECT_EQ(src_value, state.GetValue(1 + dst_size, size));
    EXPECT_EQ(0, state.GetValue(1 + dst_size + size, 1));
}
TEST_P(Addition, TestMultiControlledAddGeneralUsingToffoliStateCtrlFalse) {
    using frontend::gate::MultiControlledAddGeneralGen;
    const auto [size, dst_value, src_value] = GetParam();
    if (size < 2) {
        return;
    }
    const auto dst_size = size + 3;

    ir::IRContext context;
    auto* module = ir::Module::Create("add_general", context);
    auto builder = CircuitBuilder(module);
    auto gen = MultiControlledAddGeneralGen(&builder, 1, dst_size, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    // simulator.SetAs1(0);  // ctrl off
    simulator.SetValue(1, dst_size, dst_value);
    simulator.SetValue(1 + dst_size, size, src_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(0, state.GetValue(0, 1));
    EXPECT_EQ(dst_value, state.GetValue(1, dst_size));
    EXPECT_EQ(src_value, state.GetValue(1 + dst_size, size));
    EXPECT_EQ(0, state.GetValue(1 + dst_size + size, 1));
}
//--------------------------------------------------//
// Test AddImm
//--------------------------------------------------//
TEST_P(Addition, TestAddImmUsingToffoliState) {
    using frontend::gate::AddImmGen;
    const auto [size, dst_value, src_value] = GetParam();
    const auto max = BigInt(1) << size;

    ir::IRContext context;
    auto* module = ir::Module::Create("add_imm", context);
    auto builder = CircuitBuilder(module);
    auto gen = AddImmGen(&builder, src_value, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetValue(0, size, dst_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    const BigInt sum = dst_value + src_value;
    const auto dst_expected = sum >= max ? BigInt(sum - max) : sum;
    EXPECT_EQ(dst_expected, state.GetValue(0, size));
    EXPECT_EQ(0, state.GetValue(size, 1));
}
//--------------------------------------------------//
// Test MultiControlledAddImm
//--------------------------------------------------//
TEST_P(Addition, TestMultiControlledAddImmUsingToffoliStateCtrlTrue) {
    using frontend::gate::MultiControlledAddImmGen;
    const auto [size, dst_value, src_value] = GetParam();
    const auto max = BigInt(1) << size;
    const auto ctrl_size = std::size_t{1};

    ir::IRContext context;
    auto* module = ir::Module::Create("add_imm", context);
    auto builder = CircuitBuilder(module);
    auto gen = MultiControlledAddImmGen(&builder, src_value, ctrl_size, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetAs1(0);  // ctrl-on
    simulator.SetValue(1, size, dst_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    const BigInt sum = dst_value + src_value;
    const auto dst_expected = sum >= max ? BigInt(sum - max) : sum;
    EXPECT_EQ(dst_expected, state.GetValue(1, size));
    EXPECT_EQ(0, state.GetValue(1 + size, 1));
}
TEST_P(Addition, TestMultiControlledAddImmUsingToffoliStateCtrlFalse) {
    using frontend::gate::MultiControlledAddImmGen;
    const auto [size, dst_value, src_value] = GetParam();
    const auto max = BigInt(1) << size;
    const auto ctrl_size = std::size_t{1};

    ir::IRContext context;
    auto* module = ir::Module::Create("add_imm", context);
    auto builder = CircuitBuilder(module);
    auto gen = MultiControlledAddImmGen(&builder, src_value, ctrl_size, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    // simulator.SetAs1(0);  // ctrl-off
    simulator.SetValue(1, size, dst_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(dst_value, state.GetValue(1, size));
    EXPECT_EQ(0, state.GetValue(1 + size, 1));
}
//--------------------------------------------------//
// Test AddPiecewise
//--------------------------------------------------//
class PiecewiseAddition : public testing::TestWithParam<std::tuple<std::uint64_t, BigInt, BigInt>> {
};
INSTANTIATE_TEST_SUITE_P(
        LightCaseNormal,
        PiecewiseAddition,
        testing::Values(
                std::make_tuple(std::uint64_t{2}, BigInt(2), BigInt(1)),  // 2+=1(mod 2^2)
                std::make_tuple(std::uint64_t{5}, BigInt(10), BigInt(6))  // 10+=6(mod 2^5)
        )
);
INSTANTIATE_TEST_SUITE_P(
        LightCaseOverflow,
        PiecewiseAddition,
        testing::Values(
                std::make_tuple(std::size_t{2}, BigInt(2), BigInt(3)),  // 2+=3(mod 2^2)
                std::make_tuple(std::size_t{5}, BigInt(10), BigInt(26)),  // 10+=26(mod 2^5)
                std::make_tuple(std::size_t{5}, BigInt(16), BigInt(16))  // 16+=16(mod 2^5)
        )
);
bool CheckRoundingUpInLowerBitOperation(
        std::size_t lower_width,
        const BigInt& dst_value,
        const BigInt& src_value
) {
    const auto max_value = (BigInt{1} << lower_width);
    const auto dst_val_l = dst_value % max_value;
    const auto src_val_l = src_value % max_value;
    return (dst_val_l + src_val_l) >= max_value;
}
Int GetRegistersAsInt(
        const runtime::FullQuantumState& state,
        std::uint64_t start,
        std::uint64_t size
) {
    auto bool_array = std::vector<bool>();
    bool_array.reserve(size);
    for (auto i = std::uint64_t{0}; i < size; ++i) {
        bool_array.emplace_back(state.ReadRegister(start + i));
    }
    return BoolArrayAsInt(bool_array);
};
TEST_P(PiecewiseAddition, TestAddPiecewiseUsingFullQuantumState) {
    using frontend::gate::AddPiecewiseGen;
    constexpr auto NumSamples = std::size_t{100};
    const auto [size, dst_value, src_value] = GetParam();
    const auto max = BigInt(1) << size;
    const auto sum = dst_value + src_value;
    const auto dst_expected = sum >= max ? BigInt(sum - max) : sum;

    for (auto p = std::size_t{1}; p < size; ++p) {
        const auto do_stat_test = CheckRoundingUpInLowerBitOperation(p, dst_value, src_value);
        for (auto m = std::size_t{1}; m <= size - p; ++m) {
            ir::IRContext context;
            auto* module = ir::Module::Create("add_piecewise", context);
            auto builder = CircuitBuilder(module);
            auto gen = AddPiecewiseGen(&builder, size, m, p);
            auto* circuit = gen.Generate();

            const auto config = SimulatorConfig{
                    .state_type = SimulatorConfig::StateType::FullQuantum,
                    .use_qulacs = true
            };
            auto simulator = Simulator(config, circuit->GetIR());
            simulator.SetValue(0, size, dst_value);
            simulator.SetValue(size, size, src_value);
            simulator.RunAll();

            const auto& state = simulator.GetFullQuantumState();

            if ((p + m != size) && CheckRoundingUpInLowerBitOperation(p, dst_value, src_value)) {
                const auto out = GetRegistersAsInt(state, 0, m);
                const auto samples = state.Sampling(NumSamples);
                for (const auto& sample : samples) {
                    const auto dst_result = sample % max;
                    const auto src_result = sample / max;
                    if (out == 0) {
                        EXPECT_FALSE(dst_result == dst_expected);
                    } else {
                        EXPECT_EQ(dst_expected, dst_result);
                    }
                    EXPECT_EQ(src_value, src_result);
                }
            } else {
                const auto sample = state.Sampling(1)[0];
                const auto dst_result = sample % max;
                const auto src_result = sample / max;
                EXPECT_EQ(dst_expected, dst_result);
                EXPECT_EQ(src_value, src_result);
            }
        }
    }
}
