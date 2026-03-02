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

//--------------------------------------------------//
// Test BiFlip
//--------------------------------------------------//
TEST(TestBiFlip, UsingToffoliState) {
    using frontend::gate::BiFlipGen;
    const auto dst_size = std::size_t{8};
    const auto src_size = std::size_t{5};
    const BigInt dst_max = 1 << dst_size;
    const BigInt src_max = 1 << src_size;

    ir::IRContext context;
    auto* module = ir::Module::Create("biflip", context);
    auto builder = CircuitBuilder(module);
    auto gen = BiFlipGen(&builder, dst_size, src_size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};

    for (auto i = 0; i < 10; ++i) {
        for (auto j = 0; j < 10; ++j) {
            const auto dst_value = (1893 * i + 9348) % dst_max;
            const auto src_value = (3981 * j + 8439) % src_max;

            auto simulator = Simulator(config, circuit->GetIR());
            simulator.SetValue(0, dst_size, dst_value);
            simulator.SetValue(dst_size, src_size, src_value);
            simulator.RunAll();

            const auto& state = simulator.GetToffoliState();
            const auto dst_expected = dst_value < src_value
                    ? BigInt(src_value - 1 - dst_value)
                    : BigInt(dst_max - 1 - dst_value + src_value);
            EXPECT_EQ(dst_expected, state.GetValue(0, dst_size));
            EXPECT_EQ(src_value, state.GetValue(dst_size, src_size));
        }
    }
}
//--------------------------------------------------//
// Test MultiControlledBiFlip
//--------------------------------------------------//
TEST(TestMultiControlledBiFlip, UsingToffoliStateCtrlTrue) {
    using frontend::gate::MultiControlledBiFlipGen;
    const auto dst_size = std::size_t{8};
    const auto src_size = std::size_t{5};
    const BigInt dst_max = 1 << dst_size;
    const BigInt src_max = 1 << src_size;

    ir::IRContext context;
    auto* module = ir::Module::Create("biflip", context);
    auto builder = CircuitBuilder(module);
    auto gen = MultiControlledBiFlipGen(&builder, 1, dst_size, src_size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};

    for (auto i = 0; i < 10; ++i) {
        for (auto j = 0; j < 10; ++j) {
            const auto dst_value = (1893 * i + 9348) % dst_max;
            const auto src_value = (3981 * j + 8439) % src_max;

            auto simulator = Simulator(config, circuit->GetIR());
            simulator.SetAs1(0);  // ctrl-on
            simulator.SetValue(1, dst_size, dst_value);
            simulator.SetValue(1 + dst_size, src_size, src_value);
            simulator.RunAll();

            const auto& state = simulator.GetToffoliState();
            const auto dst_expected = dst_value < src_value
                    ? BigInt(src_value - 1 - dst_value)
                    : BigInt(dst_max - 1 - dst_value + src_value);
            EXPECT_EQ(1, state.GetValue(0, 1));
            EXPECT_EQ(dst_expected, state.GetValue(1, dst_size));
            EXPECT_EQ(src_value, state.GetValue(1 + dst_size, src_size));
            EXPECT_EQ(0, state.GetValue(1 + dst_size + src_size, 1));
        }
    }
}
TEST(TestMultiControlledBiFlip, UsingToffoliStateCtrlFalse) {
    using frontend::gate::MultiControlledBiFlipGen;
    const auto dst_size = std::size_t{8};
    const auto src_size = std::size_t{5};
    const BigInt dst_max = 1 << dst_size;
    const BigInt src_max = 1 << src_size;

    ir::IRContext context;
    auto* module = ir::Module::Create("biflip", context);
    auto builder = CircuitBuilder(module);
    auto gen = MultiControlledBiFlipGen(&builder, 1, dst_size, src_size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};

    for (auto i = 0; i < 10; ++i) {
        for (auto j = 0; j < 10; ++j) {
            const auto dst_value = (1893 * i + 9348) % dst_max;
            const auto src_value = (3981 * j + 8439) % src_max;

            auto simulator = Simulator(config, circuit->GetIR());
            // simulator.SetAs1(0);  // ctrl-off
            simulator.SetValue(1, dst_size, dst_value);
            simulator.SetValue(1 + dst_size, src_size, src_value);
            simulator.RunAll();

            const auto& state = simulator.GetToffoliState();
            EXPECT_EQ(0, state.GetValue(0, 1));
            EXPECT_EQ(dst_value, state.GetValue(1, dst_size));
            EXPECT_EQ(src_value, state.GetValue(1 + dst_size, src_size));
            EXPECT_EQ(0, state.GetValue(1 + dst_size + src_size, 1));
        }
    }
}
//--------------------------------------------------//
// Test BiFlipImm
//--------------------------------------------------//
TEST(TestBiFlipImm, UsingToffoliState) {
    using frontend::gate::BiFlipImmGen;
    const auto size = std::size_t{10};
    const BigInt max = 1 << size;

    ir::IRContext context;
    auto* module = ir::Module::Create("biflip", context);
    auto builder = CircuitBuilder(module);

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};

    for (auto i = 0; i < 10; ++i) {
        for (auto j = 0; j < 10; ++j) {
            const auto dst_value = (1893 * i + 9348) % max;
            const auto src_value = (3981 * j + 8439) % max;

            auto gen = BiFlipImmGen(&builder, src_value, size);
            auto* circuit = gen.Generate();

            auto simulator = Simulator(config, circuit->GetIR());
            simulator.SetValue(0, size, dst_value);
            simulator.RunAll();

            const auto& state = simulator.GetToffoliState();
            const auto dst_expected = dst_value < src_value
                    ? BigInt(src_value - 1 - dst_value)
                    : BigInt(max - 1 - dst_value + src_value);
            EXPECT_EQ(dst_expected, state.GetValue(0, size));
            EXPECT_EQ(0, state.GetValue(size, 1));
        }
    }
}
//--------------------------------------------------//
// Test MultiControlledControlledBiFlipImm
//--------------------------------------------------//
TEST(TestMultiControlledBiFlipImm, UsingToffoliStateCtrlTrue) {
    using frontend::gate::MultiControlledBiFlipImmGen;
    const auto size = std::size_t{10};
    const BigInt max = 1 << size;

    ir::IRContext context;
    auto* module = ir::Module::Create("biflip", context);
    auto builder = CircuitBuilder(module);

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};

    for (auto i = 0; i < 10; ++i) {
        for (auto j = 0; j < 10; ++j) {
            const auto dst_value = (1893 * i + 9348) % max;
            const auto src_value = (3981 * j + 8439) % max;

            auto gen = MultiControlledBiFlipImmGen(&builder, src_value, 1, size);
            auto* circuit = gen.Generate();

            auto simulator = Simulator(config, circuit->GetIR());
            simulator.SetAs1(0);  // ctrl-on
            simulator.SetValue(1, size, dst_value);
            simulator.RunAll();

            const auto& state = simulator.GetToffoliState();
            const auto dst_expected = dst_value < src_value
                    ? BigInt(src_value - 1 - dst_value)
                    : BigInt(max - 1 - dst_value + src_value);
            EXPECT_EQ(1, state.GetValue(0, 1));
            EXPECT_EQ(dst_expected, state.GetValue(1, size));
            EXPECT_EQ(0, state.GetValue(1 + size, 1));
        }
    }
}
TEST(TestMultiControlledBiFlipImm, UsingToffoliStateCtrlFalse) {
    using frontend::gate::MultiControlledBiFlipImmGen;
    const auto size = std::size_t{10};
    const BigInt max = 1 << size;

    ir::IRContext context;
    auto* module = ir::Module::Create("biflip", context);
    auto builder = CircuitBuilder(module);

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};

    for (auto i = 0; i < 10; ++i) {
        for (auto j = 0; j < 10; ++j) {
            const auto dst_value = (1893 * i + 9348) % max;
            const auto src_value = (3981 * j + 8439) % max;

            auto gen = MultiControlledBiFlipImmGen(&builder, src_value, 1, size);
            auto* circuit = gen.Generate();

            auto simulator = Simulator(config, circuit->GetIR());
            // simulator.SetAs1(0);  // ctrl-off
            simulator.SetValue(1, size, dst_value);
            simulator.RunAll();

            const auto& state = simulator.GetToffoliState();
            EXPECT_EQ(0, state.GetValue(0, 1));
            EXPECT_EQ(dst_value, state.GetValue(1, size));
            EXPECT_EQ(0, state.GetValue(1 + size, 1));
        }
    }
}
//--------------------------------------------------//
// Test PivotFlip
//--------------------------------------------------//
TEST(TestPivotFlip, UsingToffoliState) {
    using frontend::gate::PivotFlipGen;
    const auto dst_size = std::size_t{6};
    const auto src_size = std::size_t{5};
    const BigInt dst_max = 1 << dst_size;
    const BigInt src_max = 1 << src_size;

    ir::IRContext context;
    auto* module = ir::Module::Create("pivot_flip", context);
    auto builder = CircuitBuilder(module);
    auto gen = PivotFlipGen(&builder, dst_size, src_size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};

    for (auto i = 0; i < 10; ++i) {
        for (auto j = 0; j < 10; ++j) {
            const auto dst_value = (1893 * i + 9348) % dst_max;
            const auto src_value = (3981 * j + 8439) % src_max;

            auto simulator = Simulator(config, circuit->GetIR());
            simulator.SetValue(0, dst_size, dst_value);
            simulator.SetValue(dst_size, src_size, src_value);
            simulator.RunAll();

            const auto& state = simulator.GetToffoliState();
            const auto dst_expected =
                    dst_value < src_value ? BigInt(src_value - 1 - dst_value) : dst_value;
            EXPECT_EQ(dst_expected, state.GetValue(0, dst_size)) << dst_value << ", " << src_value;
            EXPECT_EQ(src_value, state.GetValue(dst_size, src_size));
            EXPECT_EQ(0, state.GetValue(dst_size + src_size, 2));
        }
    }
}
//--------------------------------------------------//
// Test MultiControlledPivotFlip
//--------------------------------------------------//
TEST(TestMultiControlledPivotFlip, UsingToffoliStateCtrlTrue) {
    using frontend::gate::MultiControlledPivotFlipGen;
    const auto dst_size = std::size_t{8};
    const auto src_size = std::size_t{5};
    const BigInt dst_max = 1 << dst_size;
    const BigInt src_max = 1 << src_size;

    ir::IRContext context;
    auto* module = ir::Module::Create("pivot_flip", context);
    auto builder = CircuitBuilder(module);
    auto gen = MultiControlledPivotFlipGen(&builder, 1, dst_size, src_size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};

    for (auto i = 0; i < 10; ++i) {
        for (auto j = 0; j < 10; ++j) {
            const auto dst_value = (1893 * i + 9348) % dst_max;
            const auto src_value = (3981 * j + 8439) % src_max;

            auto simulator = Simulator(config, circuit->GetIR());
            simulator.SetAs1(0);  // ctrl-on
            simulator.SetValue(1, dst_size, dst_value);
            simulator.SetValue(1 + dst_size, src_size, src_value);
            simulator.RunAll();

            const auto& state = simulator.GetToffoliState();
            const auto dst_expected =
                    dst_value < src_value ? BigInt(src_value - 1 - dst_value) : dst_value;
            EXPECT_EQ(1, state.GetValue(0, 1));
            EXPECT_EQ(dst_expected, state.GetValue(1, dst_size)) << dst_value << ", " << src_value;
            EXPECT_EQ(src_value, state.GetValue(1 + dst_size, src_size));
            EXPECT_EQ(0, state.GetValue(1 + dst_size + src_size, 2));
        }
    }
}
TEST(TestMultiControlledPivotFlip, UsingToffoliStateCtrlFalse) {
    using frontend::gate::MultiControlledPivotFlipGen;
    const auto dst_size = std::size_t{8};
    const auto src_size = std::size_t{5};
    const BigInt dst_max = 1 << dst_size;
    const BigInt src_max = 1 << src_size;

    ir::IRContext context;
    auto* module = ir::Module::Create("pivot_flip", context);
    auto builder = CircuitBuilder(module);
    auto gen = MultiControlledPivotFlipGen(&builder, 1, dst_size, src_size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};

    for (auto i = 0; i < 10; ++i) {
        for (auto j = 0; j < 10; ++j) {
            const auto dst_value = (1893 * i + 9348) % dst_max;
            const auto src_value = (3981 * j + 8439) % src_max;

            auto simulator = Simulator(config, circuit->GetIR());
            // simulator.SetAs1(0);  // ctrl-off
            simulator.SetValue(1, dst_size, dst_value);
            simulator.SetValue(1 + dst_size, src_size, src_value);
            simulator.RunAll();

            const auto& state = simulator.GetToffoliState();
            EXPECT_EQ(0, state.GetValue(0, 1));
            EXPECT_EQ(dst_value, state.GetValue(1, dst_size)) << dst_value << ", " << src_value;
            EXPECT_EQ(src_value, state.GetValue(1 + dst_size, src_size));
            EXPECT_EQ(0, state.GetValue(1 + dst_size + src_size, 2));
        }
    }
}
//--------------------------------------------------//
// Test PivotFlipImm
//--------------------------------------------------//
TEST(TestPivotFlipImm, UsingToffoliState) {
    using frontend::gate::PivotFlipImmGen;
    const auto size = std::size_t{10};
    const BigInt max = 1 << size;

    ir::IRContext context;
    auto* module = ir::Module::Create("pivot_flip", context);
    auto builder = CircuitBuilder(module);

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};

    for (auto i = 0; i < 10; ++i) {
        for (auto j = 0; j < 10; ++j) {
            const auto dst_value = (1893 * i + 9348) % max;
            const auto src_value = (3981 * j + 8439) % max;

            auto gen = PivotFlipImmGen(&builder, src_value, size);
            auto* circuit = gen.Generate();

            auto simulator = Simulator(config, circuit->GetIR());
            simulator.SetValue(0, size, dst_value);
            simulator.RunAll();

            const auto& state = simulator.GetToffoliState();
            const auto dst_expected =
                    dst_value < src_value ? BigInt(src_value - 1 - dst_value) : dst_value;
            EXPECT_EQ(dst_expected, state.GetValue(0, size)) << dst_value << ", " << src_value;
            EXPECT_EQ(0, state.GetValue(size, 2));
        }
    }
}
//--------------------------------------------------//
// Test MultiControlledControlledPivotFlipImm
//--------------------------------------------------//
TEST(TestMultiControlledPivotFlipImm, UsingToffoliStateCtrlTrue) {
    using frontend::gate::MultiControlledPivotFlipImmGen;
    const auto size = std::size_t{10};
    const BigInt max = 1 << size;

    ir::IRContext context;
    auto* module = ir::Module::Create("pivot_flip", context);
    auto builder = CircuitBuilder(module);

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};

    for (auto i = 0; i < 10; ++i) {
        for (auto j = 0; j < 10; ++j) {
            const auto dst_value = (1893 * i + 9348) % max;
            const auto src_value = (3981 * j + 8439) % max;

            auto gen = MultiControlledPivotFlipImmGen(&builder, src_value, 1, size);
            auto* circuit = gen.Generate();

            auto simulator = Simulator(config, circuit->GetIR());
            simulator.SetAs1(0);  // ctrl-on
            simulator.SetValue(1, size, dst_value);
            simulator.RunAll();

            const auto& state = simulator.GetToffoliState();
            const auto dst_expected =
                    dst_value < src_value ? BigInt(src_value - 1 - dst_value) : dst_value;
            EXPECT_EQ(1, state.GetValue(0, 1));
            EXPECT_EQ(dst_expected, state.GetValue(1, size));
            EXPECT_EQ(0, state.GetValue(1 + size, 2));
        }
    }
}
TEST(TestMultiControlledPivotFlipImm, UsingToffoliStateCtrlFalse) {
    using frontend::gate::MultiControlledPivotFlipImmGen;
    const auto size = std::size_t{10};
    const BigInt max = 1 << size;

    ir::IRContext context;
    auto* module = ir::Module::Create("pivot_flip", context);
    auto builder = CircuitBuilder(module);

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};

    for (auto i = 0; i < 10; ++i) {
        for (auto j = 0; j < 10; ++j) {
            const auto dst_value = (1893 * i + 9348) % max;
            const auto src_value = (3981 * j + 8439) % max;

            auto gen = MultiControlledPivotFlipImmGen(&builder, src_value, 1, size);
            auto* circuit = gen.Generate();

            auto simulator = Simulator(config, circuit->GetIR());
            // simulator.SetAs1(0);  // ctrl-off
            simulator.SetValue(1, size, dst_value);
            simulator.RunAll();

            const auto& state = simulator.GetToffoliState();
            EXPECT_EQ(0, state.GetValue(0, 1));
            EXPECT_EQ(dst_value, state.GetValue(1, size));
            EXPECT_EQ(0, state.GetValue(1 + size, 2));
        }
    }
}
