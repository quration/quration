#include "qret/algorithm/arithmetic/modular.h"

#include <gtest/gtest.h>

#include "qret/base/type.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/intrinsic.h"
#include "qret/math/integer.h"
#include "qret/runtime/simulator.h"

using namespace qret;
using frontend::CircuitBuilder, runtime::SimulatorConfig, runtime::Simulator;

// Define test cases
// size, mod, dst_value, src_value
class Modular : public testing::TestWithParam<std::tuple<std::uint64_t, BigInt, BigInt, BigInt>> {};
INSTANTIATE_TEST_SUITE_P(
        LightCase,
        Modular,
        testing::Values(
                std::make_tuple(std::uint64_t{2}, BigInt(3), BigInt(2), BigInt(1)),
                std::make_tuple(std::uint64_t{5}, BigInt(7), BigInt(10), BigInt(6)),
                std::make_tuple(std::uint64_t{5}, BigInt(17), BigInt(10), BigInt(6)),
                std::make_tuple(std::uint64_t{5}, BigInt(17), BigInt(10), BigInt(10)),
                std::make_tuple(std::uint64_t{5}, BigInt(16), BigInt(10), BigInt(6)),
                std::make_tuple(std::uint64_t{5}, BigInt(16), BigInt(20), BigInt(19)),
                std::make_tuple(std::uint64_t{5}, BigInt(29), BigInt(20), BigInt(19))
        )
);
INSTANTIATE_TEST_SUITE_P(
        HeavyCase,
        Modular,
        testing::Values(
                std::make_tuple(std::uint64_t{100}, BigInt(9999999), BigInt(42789), BigInt(2347)),
                std::make_tuple(
                        std::uint64_t{100},
                        BigInt(9999999),
                        BigInt(1) << 90,
                        BigInt(2478390)
                )
        )
);

// size, mod, multiplier, dst_value, src_value
class ModularScaledAddition
    : public testing::TestWithParam<std::tuple<std::uint64_t, BigInt, BigInt, BigInt, BigInt>> {};
INSTANTIATE_TEST_SUITE_P(
        LightCase,
        ModularScaledAddition,
        testing::Values(
                std::make_tuple(std::uint64_t{5}, BigInt(7), BigInt(4), BigInt(10), BigInt(6)),
                std::make_tuple(std::uint64_t{5}, BigInt(17), BigInt(4), BigInt(10), BigInt(6)),
                std::make_tuple(std::uint64_t{5}, BigInt(17), BigInt(4), BigInt(10), BigInt(10)),
                std::make_tuple(std::uint64_t{5}, BigInt(29), BigInt(4), BigInt(20), BigInt(19))
        )
);
INSTANTIATE_TEST_SUITE_P(
        HeavyCase,
        ModularScaledAddition,
        testing::Values(
                std::make_tuple(
                        std::uint64_t{20},
                        BigInt(100019),
                        BigInt(42789),
                        BigInt(42789),
                        BigInt(2347)
                ),
                std::make_tuple(
                        std::uint64_t{20},
                        BigInt(100019),
                        BigInt(42789),
                        BigInt(1) << 19,
                        BigInt(2478390)
                )
        )
);

// total_window_size, mul_size, exp_size, base, mod, tgt_value, exp_value
class WindowedExponentiation
    : public testing::TestWithParam<std::tuple<
              std::size_t,
              std::tuple<std::size_t, std::size_t, BigInt, BigInt, BigInt, BigInt>>> {};
INSTANTIATE_TEST_SUITE_P(
        LightCase,
        WindowedExponentiation,
        testing::Combine(
                testing::Range(std::size_t{4}, std::size_t{6}),  // total_window_size
                testing::Values(
                        std::make_tuple(
                                std::size_t{2},  // mul_size
                                std::size_t{5},  // exp_size
                                BigInt(1),  // base
                                BigInt(3),  // mod
                                BigInt(2),  // tgt_value
                                BigInt(5)  // exp_value
                        ),  // 2x=1^5(mod 3)
                        std::make_tuple(
                                std::size_t{3},  // mul_size
                                std::size_t{5},  // exp_size
                                BigInt(3),  // base
                                BigInt(7),  // mod
                                BigInt(2),  // tgt_value
                                BigInt(9)  // exp_value
                        ),  // 2x=3^9(mod 7)
                        std::make_tuple(
                                std::size_t{20},  // mul_size
                                std::size_t{20},  // exp_size
                                BigInt(2491),  // base
                                BigInt(9973),  // mod
                                BigInt(2890),  // tgt_value
                                BigInt(4291)  // exp_value
                        )
                )
        )
);

//--------------------------------------------------//
// Test ModNeg
//--------------------------------------------------//
TEST_P(Modular, TestModNegUsingToffoliState) {
    using frontend::gate::MultiControlledModNegGen;
    auto [size, mod, tmp_value, _] = GetParam();
    const auto value = tmp_value % mod;

    ir::IRContext context;
    auto* module = ir::Module::Create("mod", context);
    auto builder = CircuitBuilder(module);
    auto gen = MultiControlledModNegGen(&builder, mod, 0, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetValue(0, size, value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ((mod - value) % mod, state.GetValue(0, size));
    EXPECT_EQ(0, state.GetValue(size, 2));
}
//--------------------------------------------------//
// Test MultiControlledModNeg
//--------------------------------------------------//
TEST_P(Modular, TestMultiControlledModNegUsingToffoliStateCtrlTrue) {
    using frontend::gate::MultiControlledModNegGen;
    auto [size, mod, tmp_value, _] = GetParam();
    const auto ctrl_size = std::size_t{2};
    const auto value = tmp_value % mod;

    ir::IRContext context;
    auto* module = ir::Module::Create("mod", context);
    auto builder = CircuitBuilder(module);
    auto gen = MultiControlledModNegGen(&builder, mod, ctrl_size, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    for (auto i = std::size_t{0}; i < ctrl_size; ++i) {
        simulator.SetAs1(i);  // ctrl-on
    }
    simulator.SetValue(ctrl_size, size, value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    for (auto i = std::size_t{0}; i < ctrl_size; ++i) {
        EXPECT_EQ(1, state.GetValue(i, 1));
    }
    EXPECT_EQ((mod - value) % mod, state.GetValue(ctrl_size, size));
    EXPECT_EQ(0, state.GetValue(ctrl_size + size, 2));
}
TEST_P(Modular, TestMultiControlledModNegUsingToffoliStateCtrlFalse) {
    using frontend::gate::MultiControlledModNegGen;
    auto [size, mod, tmp_value, _] = GetParam();
    const auto ctrl_size = std::size_t{2};
    const auto value = tmp_value % mod;

    ir::IRContext context;
    auto* module = ir::Module::Create("mod", context);
    auto builder = CircuitBuilder(module);
    auto gen = MultiControlledModNegGen(&builder, mod, ctrl_size, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    // for (auto i = std::size_t{0}; i < ctrl_size; ++i) {
    //     simulator.SetAs1(i);  // ctrl-off
    // }
    simulator.SetValue(ctrl_size, size, value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    for (auto i = std::size_t{0}; i < ctrl_size; ++i) {
        EXPECT_EQ(0, state.GetValue(i, 1));
    }
    EXPECT_EQ(value, state.GetValue(ctrl_size, size));
    EXPECT_EQ(0, state.GetValue(ctrl_size + size, 2));
}
//--------------------------------------------------//
// Test ModAdd
//--------------------------------------------------//
TEST_P(Modular, TestModAddUsingToffoliState) {
    using frontend::gate::ModAddGen;
    auto [size, mod, tmp_dst_value, tmp_src_value] = GetParam();
    const auto dst_value = tmp_dst_value % mod;
    const auto src_value = tmp_src_value % mod;

    ir::IRContext context;
    auto* module = ir::Module::Create("mod", context);
    auto builder = CircuitBuilder(module);
    auto gen = ModAddGen(&builder, mod, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetValue(0, size, dst_value);
    simulator.SetValue(size, size, src_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ((dst_value + src_value) % mod, state.GetValue(0, size));
    EXPECT_EQ(src_value, state.GetValue(size, size));
}
//--------------------------------------------------//
// Test MultiControlledModAdd
//--------------------------------------------------//
TEST_P(Modular, TestMultiControlledModAddUsingToffoliStateCtrlTrue) {
    using frontend::gate::MultiControlledModAddGen;
    auto [size, mod, tmp_dst_value, tmp_src_value] = GetParam();
    const auto ctrl_size = std::size_t{1};
    const auto dst_value = tmp_dst_value % mod;
    const auto src_value = tmp_src_value % mod;

    ir::IRContext context;
    auto* module = ir::Module::Create("mod", context);
    auto builder = CircuitBuilder(module);
    auto gen = MultiControlledModAddGen(&builder, mod, ctrl_size, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    for (auto i = std::size_t{0}; i < ctrl_size; ++i) {
        simulator.SetAs1(i);  // ctrl-on
    }
    simulator.SetValue(ctrl_size, size, dst_value);
    simulator.SetValue(ctrl_size + size, size, src_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    for (auto i = std::size_t{0}; i < ctrl_size; ++i) {
        EXPECT_EQ(1, state.GetValue(i, 1));
    }
    EXPECT_EQ((dst_value + src_value) % mod, state.GetValue(ctrl_size, size));
    EXPECT_EQ(src_value, state.GetValue(ctrl_size + size, size));
    for (auto i = std::size_t{0}; i < gen.m; ++i) {
        EXPECT_EQ(0, state.GetValue(ctrl_size + 2 * size + i, 1));
    }
}
TEST_P(Modular, TestMultiControlledModAddUsingToffoliStateCtrlFalse) {
    using frontend::gate::MultiControlledModAddGen;
    auto [size, mod, tmp_dst_value, tmp_src_value] = GetParam();
    const auto ctrl_size = std::size_t{1};
    const auto dst_value = tmp_dst_value % mod;
    const auto src_value = tmp_src_value % mod;

    ir::IRContext context;
    auto* module = ir::Module::Create("mod", context);
    auto builder = CircuitBuilder(module);
    auto gen = MultiControlledModAddGen(&builder, mod, ctrl_size, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    // for (auto i = std::size_t{0}; i < ctrl_size; ++i) {
    //     simulator.SetAs1(i);  // ctrl-off
    // }
    simulator.SetValue(ctrl_size, size, dst_value);
    simulator.SetValue(ctrl_size + size, size, src_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    for (auto i = std::size_t{0}; i < ctrl_size; ++i) {
        EXPECT_EQ(0, state.GetValue(i, 1));
    }
    EXPECT_EQ(dst_value, state.GetValue(ctrl_size, size));
    EXPECT_EQ(src_value, state.GetValue(ctrl_size + size, size));
    for (auto i = std::size_t{0}; i < gen.m; ++i) {
        EXPECT_EQ(0, state.GetValue(ctrl_size + 2 * size + i, 1));
    }
}
//--------------------------------------------------//
// Test ModAddImm
//--------------------------------------------------//
TEST_P(Modular, TestModAddImmUsingToffoliState) {
    using frontend::gate::ModAddImmGen;
    auto [size, mod, tmp_tgt_value, tmp_imm_value] = GetParam();
    const auto tgt_value = tmp_tgt_value % mod;
    const auto imm_value = tmp_imm_value % mod;

    ir::IRContext context;
    auto* module = ir::Module::Create("mod", context);
    auto builder = CircuitBuilder(module);
    auto gen = ModAddImmGen(&builder, mod, imm_value, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetValue(0, size, tgt_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ((tgt_value + imm_value) % mod, state.GetValue(0, size));
}
//--------------------------------------------------//
// Test MultiControlledModAddImm
//--------------------------------------------------//
TEST_P(Modular, TestMultiControlledModAddImmUsingToffoliStateCtrlTrue) {
    using frontend::gate::MultiControlledModAddImmGen;
    auto [size, mod, tmp_tgt_value, tmp_imm_value] = GetParam();
    const auto tgt_value = tmp_tgt_value % mod;
    const auto imm_value = tmp_imm_value % mod;

    ir::IRContext context;
    auto* module = ir::Module::Create("mod", context);
    auto builder = CircuitBuilder(module);
    auto gen = MultiControlledModAddImmGen(&builder, mod, imm_value, 1, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetAs1(0);  // ctrl-on
    simulator.SetValue(1, size, tgt_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ((tgt_value + imm_value) % mod, state.GetValue(1, size));
}
TEST_P(Modular, TestMultiControlledModAddImmUsingToffoliStateCtrlFalse) {
    using frontend::gate::MultiControlledModAddImmGen;
    auto [size, mod, tmp_tgt_value, tmp_imm_value] = GetParam();
    const auto tgt_value = tmp_tgt_value % mod;
    const auto imm_value = tmp_imm_value % mod;

    ir::IRContext context;
    auto* module = ir::Module::Create("mod", context);
    auto builder = CircuitBuilder(module);
    auto gen = MultiControlledModAddImmGen(&builder, mod, imm_value, 1, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    // simulator.SetAs1(0);  // ctrl-off
    simulator.SetValue(1, size, tgt_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(tgt_value, state.GetValue(1, size));
}
//--------------------------------------------------//
// Test ModDoubling
//--------------------------------------------------//
TEST_P(Modular, TestModDoublingUsingToffoliState) {
    using frontend::gate::MultiControlledModDoublingGen;
    auto [size, mod, tmp_tgt_value, _] = GetParam();
    if (mod % 2 == 0) {
        return;
    }
    const auto tgt_value = tmp_tgt_value % mod;

    ir::IRContext context;
    auto* module = ir::Module::Create("mod", context);
    auto builder = CircuitBuilder(module);
    auto gen = MultiControlledModDoublingGen(&builder, mod, 0, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetValue(0, size, tgt_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ((2 * tgt_value) % mod, state.GetValue(0, size));
}
//--------------------------------------------------//
// Test MultiControlledModDoubling
//--------------------------------------------------//
TEST_P(Modular, TestMultiControlledModDoublingUsingToffoliStateCtrlTrue) {
    using frontend::gate::MultiControlledModDoublingGen;
    auto [size, mod, tmp_tgt_value, _] = GetParam();
    if (mod % 2 == 0) {
        return;
    }
    const auto ctrl_size = std::size_t{1};
    const auto tgt_value = tmp_tgt_value % mod;

    ir::IRContext context;
    auto* module = ir::Module::Create("mod", context);
    auto builder = CircuitBuilder(module);
    auto gen = MultiControlledModDoublingGen(&builder, mod, ctrl_size, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    for (auto i = std::size_t{0}; i < ctrl_size; ++i) {
        simulator.SetAs1(i);  // ctrl-on
    }
    simulator.SetValue(ctrl_size, size, tgt_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    for (auto i = std::size_t{0}; i < ctrl_size; ++i) {
        EXPECT_EQ(1, state.GetValue(i, 1));
    }
    EXPECT_EQ((2 * tgt_value) % mod, state.GetValue(ctrl_size, size));
}
TEST_P(Modular, TestMultiControlledModDoublingUsingToffoliStateCtrlFalse) {
    using frontend::gate::MultiControlledModDoublingGen;
    auto [size, mod, tmp_tgt_value, _] = GetParam();
    if (mod % 2 == 0) {
        return;
    }
    const auto ctrl_size = std::size_t{1};
    const auto tgt_value = tmp_tgt_value % mod;

    ir::IRContext context;
    auto* module = ir::Module::Create("mod", context);
    auto builder = CircuitBuilder(module);
    auto gen = MultiControlledModDoublingGen(&builder, mod, ctrl_size, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    // for (auto i = std::size_t{0}; i < ctrl_size; ++i) {
    //     simulator.SetAs1(i);  // ctrl-off
    // }
    simulator.SetValue(ctrl_size, size, tgt_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    for (auto i = std::size_t{0}; i < ctrl_size; ++i) {
        EXPECT_EQ(0, state.GetValue(i, 1));
    }
    EXPECT_EQ(tgt_value, state.GetValue(ctrl_size, size));
}
//--------------------------------------------------//
// Test ModBiMulImm
//--------------------------------------------------//
TEST_P(ModularScaledAddition, TestModBiMulImmUsingToffoliState) {
    using frontend::gate::MultiControlledModBiMulImmGen;
    auto [size, mod, mul, tmp_lhs_value, tmp_rhs_value] = GetParam();
    if (mod % 2 == 0) {
        return;
    }
    const auto lhs_value = tmp_lhs_value % mod;
    const auto rhs_value = tmp_rhs_value % mod;

    ir::IRContext context;
    auto* module = ir::Module::Create("mod", context);
    auto builder = CircuitBuilder(module);
    auto gen = MultiControlledModBiMulImmGen(&builder, mod, mul, 0, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetValue(0, size, lhs_value);
    simulator.SetValue(size, size, rhs_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ((lhs_value * mul) % mod, state.GetValue(0, size));
    EXPECT_EQ(rhs_value, mul * state.GetValue(size, size) % mod);
}
//--------------------------------------------------//
// Test MultiControlledModBiMulImm
//--------------------------------------------------//
TEST_P(ModularScaledAddition, TestMultiControlledModBiMulImmUsingToffoliStateCtrlTrue) {
    using frontend::gate::MultiControlledModBiMulImmGen;
    auto [size, mod, mul, tmp_lhs_value, tmp_rhs_value] = GetParam();
    if (mod % 2 == 0) {
        return;
    }
    const auto ctrl_size = std::size_t{1};
    const auto lhs_value = tmp_lhs_value % mod;
    const auto rhs_value = tmp_rhs_value % mod;

    ir::IRContext context;
    auto* module = ir::Module::Create("mod", context);
    auto builder = CircuitBuilder(module);
    auto gen = MultiControlledModBiMulImmGen(&builder, mod, mul, ctrl_size, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetAs1(0);  // ctrl-on
    simulator.SetValue(ctrl_size, size, lhs_value);
    simulator.SetValue(ctrl_size + size, size, rhs_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(1, state.GetValue(0, 1));
    EXPECT_EQ((lhs_value * mul) % mod, state.GetValue(ctrl_size, size));
    EXPECT_EQ(rhs_value, mul * state.GetValue(ctrl_size + size, size) % mod);
}
TEST_P(ModularScaledAddition, TestMultiControlledModBiMulImmUsingToffoliStateCtrlFalse) {
    using frontend::gate::MultiControlledModBiMulImmGen;
    auto [size, mod, mul, tmp_lhs_value, tmp_rhs_value] = GetParam();
    if (mod % 2 == 0) {
        return;
    }
    const auto ctrl_size = std::size_t{1};
    const auto lhs_value = tmp_lhs_value % mod;
    const auto rhs_value = tmp_rhs_value % mod;

    ir::IRContext context;
    auto* module = ir::Module::Create("mod", context);
    auto builder = CircuitBuilder(module);
    auto gen = MultiControlledModBiMulImmGen(&builder, mod, mul, ctrl_size, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    // simulator.SetAs1(0);  // ctrl-off
    simulator.SetValue(ctrl_size, size, lhs_value);
    simulator.SetValue(ctrl_size + size, size, rhs_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(0, state.GetValue(0, 1));
    EXPECT_EQ(lhs_value, state.GetValue(ctrl_size, size));
    EXPECT_EQ(rhs_value, state.GetValue(ctrl_size + size, size));
}
//--------------------------------------------------//
// Test ModScaledAdd
//--------------------------------------------------//
TEST_P(ModularScaledAddition, TestModScaledAddUsingToffoliState) {
    using frontend::gate::MultiControlledModScaledAddGen;
    auto [size, mod, tmp_mul, tmp_dst_value, tmp_src_value] = GetParam();
    const auto scale = tmp_mul % mod;
    const auto dst_value = tmp_dst_value % mod;
    const auto src_value = tmp_src_value % mod;

    ir::IRContext context;
    auto* module = ir::Module::Create("mod", context);
    auto builder = CircuitBuilder(module);
    auto gen = MultiControlledModScaledAddGen(&builder, mod, scale, 0, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetValue(0, size, dst_value);
    simulator.SetValue(size, size, src_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ((dst_value + scale * src_value) % mod, state.GetValue(0, size));
    EXPECT_EQ(src_value, state.GetValue(size, size));
}
//--------------------------------------------------//
// Test ModScaledAddW
//--------------------------------------------------//
TEST_P(ModularScaledAddition, TestModScaledAddWUsingToffoliState) {
    using frontend::gate::ModScaledAddWGen;
    const auto window_size = std::size_t{3};
    auto [size, mod, tmp_mul, tmp_dst_value, tmp_src_value] = GetParam();
    const auto scale = tmp_mul % mod;
    const auto dst_value = tmp_dst_value % mod;
    const auto src_value = tmp_src_value % mod;

    ir::IRContext context;
    auto* module = ir::Module::Create("mod", context);
    auto builder = CircuitBuilder(module);
    auto gen = ModScaledAddWGen(&builder, window_size, mod, scale, size);
    auto* circuit = gen.Generate();

    auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 8;
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetValue(0, size, dst_value);
    simulator.SetValue(size, size, src_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ((dst_value + scale * src_value) % mod, state.GetValue(0, size));
    EXPECT_EQ(src_value, state.GetValue(size, size));
}
//--------------------------------------------------//
// Test ModExpW
//--------------------------------------------------//
TEST_P(WindowedExponentiation, TestModExpWUsingToffoliState) {
    using frontend::gate::ModExpWGen;
    const auto [total_window_size, tmp] = GetParam();
    const auto mul_window_size = (total_window_size + 1) / 2;
    const auto exp_window_size = total_window_size - mul_window_size;
    const auto [mul_size, exp_size, base, mod, tgt_value, exp_value] = tmp;

    ir::IRContext context;
    auto* module = ir::Module::Create("mod", context);
    auto builder = CircuitBuilder(module);
    auto gen =
            ModExpWGen(&builder, mul_window_size, exp_window_size, mod, base, mul_size, exp_size);
    auto* circuit = gen.Generate();

    auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 8;
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.SetValue(0, mul_size, tgt_value);
    simulator.SetValue(mul_size, exp_size, exp_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ((tgt_value * math::ModPow(base, exp_value, mod)) % mod, state.GetValue(0, mul_size));
    EXPECT_EQ(exp_value, state.GetValue(mul_size, exp_size));
}
