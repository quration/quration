#include <gtest/gtest.h>

#include "qret/algorithm/arithmetic/integer.h"
#include "qret/algorithm/util/array.h"
#include "qret/base/type.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/intrinsic.h"
#include "qret/runtime/simulator.h"

using namespace qret;
using frontend::CircuitGenerator, frontend::CircuitBuilder, runtime::SimulatorConfig,
        runtime::Simulator;

// Define test cases
class Subtraction : public testing::TestWithParam<std::tuple<std::uint64_t, BigInt, BigInt>> {};
INSTANTIATE_TEST_SUITE_P(
        LightCaseNormal,
        Subtraction,
        testing::Values(
                std::make_tuple(std::size_t{1}, BigInt(1), BigInt(1)),  // 1-=1(mod 2^1)
                std::make_tuple(std::size_t{2}, BigInt(2), BigInt(1)),  // 2-=1(mod 2^2)
                std::make_tuple(std::size_t{5}, BigInt(10), BigInt(6))  // 10-=6(mod 2^5)
        )
);
INSTANTIATE_TEST_SUITE_P(
        HeavyCaseNormal,
        Subtraction,
        testing::Values(
                std::make_tuple(std::size_t{1000}, BigInt(42789), BigInt(2347)),
                std::make_tuple(std::size_t{1000}, BigInt(1) << 900, BigInt(2478390))
        )
);
INSTANTIATE_TEST_SUITE_P(
        LightCaseOverflow,
        Subtraction,
        testing::Values(
                std::make_tuple(std::size_t{1}, BigInt(0), BigInt(1)),  // 0-=1(mod 2^1)
                std::make_tuple(std::size_t{2}, BigInt(2), BigInt(3)),  // 2-=3(mod 2^2)
                std::make_tuple(std::size_t{5}, BigInt(10), BigInt(26)),  // 10-=26(mod 2^5)
                std::make_tuple(std::size_t{5}, BigInt(16), BigInt(17))  // 16-=17(mod 2^5)
        )
);
INSTANTIATE_TEST_SUITE_P(
        HeavyCaseOverflow,
        Subtraction,
        testing::Values(
                std::make_tuple(std::size_t{1000}, BigInt(2347), (BigInt(1) << 1000) - 1),
                std::make_tuple(std::size_t{1000}, BigInt(1) << 998, BigInt(1) << 999)
        )
);

//--------------------------------------------------//
// Test Sub
//--------------------------------------------------//
struct TestSubGen : public CircuitGenerator {
    std::uint64_t size;
    BigInt src_value;
    BigInt dst_value;

    static inline const char* Name = "TestSub";
    explicit TestSubGen(
            CircuitBuilder* builder,
            std::uint64_t size,
            const BigInt& dst_value,
            const BigInt& src_value
    )
        : CircuitGenerator(builder)
        , size{size}
        , dst_value{dst_value}
        , src_value{src_value} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("dst", Type::Qubit, size, Attribute::Operate);
        arg.Add("src", Type::Qubit, size, Attribute::Operate);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto dst = GetQubits(0);
        auto src = GetQubits(1);

        frontend::gate::ApplyXToEachIfImm(dst_value, dst);
        frontend::gate::ApplyXToEachIfImm(src_value, src);
        frontend::gate::Sub(dst, src);

        return EndCircuitDefinition();
    }
};
TEST_P(Subtraction, TestSubUsingToffoliState) {
    const auto [size, dst_value, src_value] = GetParam();
    const auto max = BigInt(1) << size;

    ir::IRContext context;
    auto* module = ir::Module::Create("sub", context);
    auto builder = CircuitBuilder(module);
    auto gen = TestSubGen(&builder, size, dst_value, src_value);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    const auto dst_expected = dst_value >= src_value ? BigInt{dst_value - src_value}
                                                     : BigInt{max + dst_value - src_value};
    EXPECT_EQ(dst_expected, state.GetValue(0, size));
    EXPECT_EQ(src_value, state.GetValue(size, size));
}
//--------------------------------------------------//
// Test SubCuccaro
//--------------------------------------------------//
struct TestSubCuccaroGen : public CircuitGenerator {
    std::uint64_t size;
    BigInt src_value;
    BigInt dst_value;

    static inline const char* Name = "TestSubCuccaro";
    explicit TestSubCuccaroGen(
            CircuitBuilder* builder,
            std::uint64_t size,
            const BigInt& dst_value,
            const BigInt& src_value
    )
        : CircuitGenerator(builder)
        , size{size}
        , dst_value{dst_value}
        , src_value{src_value} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("dst", Type::Qubit, size, Attribute::Operate);
        arg.Add("src", Type::Qubit, size, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 1, Attribute::CleanAncilla);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto dst = GetQubits(0);
        auto src = GetQubits(1);
        auto aux = GetQubit(2);

        frontend::gate::ApplyXToEachIfImm(dst_value, dst);
        frontend::gate::ApplyXToEachIfImm(src_value, src);
        frontend::gate::SubCuccaro(dst, src, aux);

        return EndCircuitDefinition();
    }
};
TEST_P(Subtraction, TestSubCuccaroUsingToffoliState) {
    const auto [size, dst_value, src_value] = GetParam();
    const auto max = BigInt(1) << size;

    ir::IRContext context;
    auto* module = ir::Module::Create("sub", context);
    auto builder = CircuitBuilder(module);
    auto gen = TestSubCuccaroGen(&builder, size, dst_value, src_value);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    const auto dst_expected = dst_value >= src_value ? BigInt{dst_value - src_value}
                                                     : BigInt{max + dst_value - src_value};
    EXPECT_EQ(dst_expected, state.GetValue(0, size));
    EXPECT_EQ(src_value, state.GetValue(size, size));
    EXPECT_EQ(0, state.GetValue(2 * size, 1));
}
//--------------------------------------------------//
// Test SubCraig
//--------------------------------------------------//
struct TestSubCraigGen : public CircuitGenerator {
    std::uint64_t size;
    BigInt src_value;
    BigInt dst_value;

    static inline const char* Name = "TestSubCraig";
    explicit TestSubCraigGen(
            CircuitBuilder* builder,
            std::uint64_t size,
            const BigInt& dst_value,
            const BigInt& src_value
    )
        : CircuitGenerator(builder)
        , size{size}
        , dst_value{dst_value}
        , src_value{src_value} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("dst", Type::Qubit, size, Attribute::Operate);
        arg.Add("src", Type::Qubit, size, Attribute::Operate);
        arg.Add("aux", Type::Qubit, size - 1, Attribute::CleanAncilla);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto dst = GetQubits(0);
        auto src = GetQubits(1);
        auto aux = GetQubits(2);

        frontend::gate::ApplyXToEachIfImm(dst_value, dst);
        frontend::gate::ApplyXToEachIfImm(src_value, src);
        frontend::gate::SubCraig(dst, src, aux);

        return EndCircuitDefinition();
    }
};
TEST_P(Subtraction, TestSubCraigUsingToffoliState) {
    const auto [size, dst_value, src_value] = GetParam();
    const auto max = BigInt(1) << size;

    ir::IRContext context;
    auto* module = ir::Module::Create("sub", context);
    auto builder = CircuitBuilder(module);
    auto gen = TestSubCraigGen(&builder, size, dst_value, src_value);
    auto* circuit = gen.Generate();

    auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 2;
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    const auto dst_expected = dst_value >= src_value ? BigInt{dst_value - src_value}
                                                     : BigInt{max + dst_value - src_value};
    EXPECT_EQ(dst_expected, state.GetValue(0, size));
    EXPECT_EQ(src_value, state.GetValue(size, size));
    EXPECT_EQ(0, state.GetValue(2 * size, size - 1));
}
//--------------------------------------------------//
// Test ControlledSubCraig
//--------------------------------------------------//
struct TestControlledSubCraigGen : public CircuitGenerator {
    bool ctrl;
    std::uint64_t size;
    BigInt src_value;
    BigInt dst_value;

    static inline const char* Name = "TestControlledSubCraig";
    explicit TestControlledSubCraigGen(
            CircuitBuilder* builder,
            bool ctrl,
            std::uint64_t size,
            const BigInt& dst_value,
            const BigInt& src_value
    )
        : CircuitGenerator(builder)
        , ctrl{ctrl}
        , size{size}
        , dst_value{dst_value}
        , src_value{src_value} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("c", Type::Qubit, 1, Attribute::Operate);
        arg.Add("dst", Type::Qubit, size, Attribute::Operate);
        arg.Add("src", Type::Qubit, size, Attribute::Operate);
        arg.Add("aux", Type::Qubit, size, Attribute::CleanAncilla);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto c = GetQubit(0);
        auto dst = GetQubits(1);
        auto src = GetQubits(2);
        auto aux = GetQubits(3);

        if (ctrl) {
            frontend::gate::X(c);
        }
        frontend::gate::ApplyXToEachIfImm(dst_value, dst);
        frontend::gate::ApplyXToEachIfImm(src_value, src);
        frontend::gate::ControlledSubCraig(c, dst, src, aux);

        return EndCircuitDefinition();
    }
};
TEST_P(Subtraction, TestSubControlledCraigUsingToffoliStateCtrlTrue) {
    const auto [size, dst_value, src_value] = GetParam();
    const auto max = BigInt(1) << size;

    ir::IRContext context;
    auto* module = ir::Module::Create("sub", context);
    auto builder = CircuitBuilder(module);
    auto gen = TestControlledSubCraigGen(&builder, true, size, dst_value, src_value);
    auto* circuit = gen.Generate();

    auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 2;
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    const auto dst_expected = dst_value >= src_value ? BigInt{dst_value - src_value}
                                                     : BigInt{max + dst_value - src_value};
    EXPECT_EQ(1, state.GetValue(0, 1));
    EXPECT_EQ(dst_expected, state.GetValue(1, size));
    EXPECT_EQ(src_value, state.GetValue(1 + size, size));
    EXPECT_EQ(0, state.GetValue(1 + 2 * size, size));
}
TEST_P(Subtraction, TestSubControlledCraigUsingToffoliStateCtrlFalse) {
    const auto [size, dst_value, src_value] = GetParam();
    const auto max = BigInt(1) << size;

    ir::IRContext context;
    auto* module = ir::Module::Create("sub", context);
    auto builder = CircuitBuilder(module);
    auto gen = TestControlledSubCraigGen(&builder, false, size, dst_value, src_value);
    auto* circuit = gen.Generate();

    auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 2;
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(0, state.GetValue(0, 1));
    EXPECT_EQ(dst_value, state.GetValue(1, size));
    EXPECT_EQ(src_value, state.GetValue(1 + size, size));
    EXPECT_EQ(0, state.GetValue(1 + 2 * size, size));
}
//--------------------------------------------------//
// Test SubGeneral
//--------------------------------------------------//
struct TestSubGeneralGen : public CircuitGenerator {
    std::uint64_t dst_size;
    std::uint64_t src_size;
    BigInt src_value;
    BigInt dst_value;

    static inline const char* Name = "TestSubGeneral";
    explicit TestSubGeneralGen(
            CircuitBuilder* builder,
            std::uint64_t dst_size,
            std::uint64_t src_size,
            const BigInt& dst_value,
            const BigInt& src_value
    )
        : CircuitGenerator(builder)
        , dst_size{dst_size}
        , src_size{src_size}
        , dst_value{dst_value}
        , src_value{src_value} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("dst", Type::Qubit, dst_size, Attribute::Operate);
        arg.Add("src", Type::Qubit, src_size, Attribute::Operate);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto dst = GetQubits(0);
        auto src = GetQubits(1);

        frontend::gate::ApplyXToEachIfImm(dst_value, dst);
        frontend::gate::ApplyXToEachIfImm(src_value, src);
        frontend::gate::SubGeneral(dst, src);

        return EndCircuitDefinition();
    }
};
TEST_P(Subtraction, TestSubGeneralUsingToffoliState) {
    const auto [size, dst_value, src_value] = GetParam();
    if (size < 2) {
        return;
    }
    const auto dst_size = size + 3;
    const auto max = BigInt(1) << dst_size;

    ir::IRContext context;
    auto* module = ir::Module::Create("sub", context);
    auto builder = CircuitBuilder(module);
    auto gen = TestSubGeneralGen(&builder, dst_size, size, dst_value, src_value);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    const auto dst_expected = dst_value >= src_value ? BigInt{dst_value - src_value}
                                                     : BigInt{max + dst_value - src_value};
    EXPECT_EQ(dst_expected, state.GetValue(0, dst_size));
    EXPECT_EQ(src_value, state.GetValue(dst_size, size));
}
//--------------------------------------------------//
// Test MultiControlledSubGeneral
//--------------------------------------------------//
struct TestMultiControlledSubGeneralGen : public CircuitGenerator {
    bool ctrl;
    std::uint64_t dst_size;
    std::uint64_t src_size;
    BigInt src_value;
    BigInt dst_value;

    static inline const char* Name = "TestMultiControlledSubGeneral";
    explicit TestMultiControlledSubGeneralGen(
            CircuitBuilder* builder,
            bool ctrl,
            std::uint64_t dst_size,
            std::uint64_t src_size,
            const BigInt& dst_value,
            const BigInt& src_value
    )
        : CircuitGenerator(builder)
        , ctrl{ctrl}
        , dst_size{dst_size}
        , src_size{src_size}
        , dst_value{dst_value}
        , src_value{src_value} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("c", Type::Qubit, 1, Attribute::Operate);
        arg.Add("dst", Type::Qubit, dst_size, Attribute::Operate);
        arg.Add("src", Type::Qubit, src_size, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 1, Attribute::DirtyAncilla);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto c = GetQubit(0);
        auto dst = GetQubits(1);
        auto src = GetQubits(2);
        auto aux = GetQubit(3);

        if (ctrl) {
            frontend::gate::X(c);
        }
        frontend::gate::ApplyXToEachIfImm(dst_value, dst);
        frontend::gate::ApplyXToEachIfImm(src_value, src);
        frontend::gate::MultiControlledSubGeneral(c, dst, src, aux);

        return EndCircuitDefinition();
    }
};
TEST_P(Subtraction, TestMultiControlledSubGeneralUsingToffoliStateCtrlTrue) {
    const auto [size, dst_value, src_value] = GetParam();
    if (size < 2) {
        return;
    }
    const auto dst_size = size + 3;
    const auto max = BigInt(1) << dst_size;

    ir::IRContext context;
    auto* module = ir::Module::Create("sub", context);
    auto builder = CircuitBuilder(module);
    auto gen =
            TestMultiControlledSubGeneralGen(&builder, true, dst_size, size, dst_value, src_value);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    const auto dst_expected = dst_value >= src_value ? BigInt{dst_value - src_value}
                                                     : BigInt{max + dst_value - src_value};
    EXPECT_EQ(1, state.GetValue(0, 1));
    EXPECT_EQ(dst_expected, state.GetValue(1, dst_size));
    EXPECT_EQ(src_value, state.GetValue(1 + dst_size, size));
    EXPECT_EQ(0, state.GetValue(1 + dst_size + size, 1));
}
TEST_P(Subtraction, TestMultiControlledSubGeneralUsingToffoliStateCtrlFalse) {
    const auto [size, dst_value, src_value] = GetParam();
    if (size < 2) {
        return;
    }
    const auto dst_size = size + 3;
    const auto max = BigInt(1) << dst_size;

    ir::IRContext context;
    auto* module = ir::Module::Create("sub", context);
    auto builder = CircuitBuilder(module);
    auto gen =
            TestMultiControlledSubGeneralGen(&builder, false, dst_size, size, dst_value, src_value);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(0, state.GetValue(0, 1));
    EXPECT_EQ(dst_value, state.GetValue(1, dst_size));
    EXPECT_EQ(src_value, state.GetValue(1 + dst_size, size));
    EXPECT_EQ(0, state.GetValue(1 + dst_size + size, 1));
}
