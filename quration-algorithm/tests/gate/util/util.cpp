#include "qret/algorithm/util/util.h"

#include <gtest/gtest.h>

#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/intrinsic.h"
#include "qret/runtime/simulator.h"

using namespace qret;

//--------------------------------------------------//
// Test ControlledSwap
//--------------------------------------------------//
struct TestControlledSwapGen : public frontend::CircuitGenerator {
    bool x;
    bool y;
    bool z;

    static inline const char* Name = "TestControlledSwap";
    explicit TestControlledSwapGen(frontend::CircuitBuilder* builder, bool x, bool y, bool z)
        : CircuitGenerator(builder)
        , x{x}
        , y{y}
        , z{z} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("c", Type::Qubit, 1, Attribute::Operate);
        arg.Add("lhs", Type::Qubit, 1, Attribute::Operate);
        arg.Add("rhs", Type::Qubit, 1, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 1, Attribute::CleanAncilla);
        arg.Add("out", Type::Register, 3, Attribute::Output);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto c = GetQubit(0);
        auto lhs = GetQubit(1);
        auto rhs = GetQubit(2);
        auto aux = GetQubit(3);
        auto out = GetRegisters(4);

        if (x) {
            frontend::gate::X(c);
        }
        if (y) {
            frontend::gate::X(lhs);
        }
        if (z) {
            frontend::gate::X(rhs);
        }
        frontend::gate::ControlledSwap(c, lhs, rhs, aux);
        frontend::gate::Measure(c, out[0]);
        frontend::gate::Measure(lhs, out[1]);
        frontend::gate::Measure(rhs, out[2]);

        return EndCircuitDefinition();
    }
};
class TestControlledSwap : public testing::TestWithParam<std::tuple<bool, bool, bool>> {};
TEST_P(TestControlledSwap, FullQuantum) {
    const auto [x, y, z] = GetParam();

    ir::IRContext context;
    auto* module = ir::Module::Create("util", context);
    auto builder = frontend::CircuitBuilder(module);
    auto gen = TestControlledSwapGen(&builder, x, y, z);
    auto* circuit = gen.Generate();

    const auto config = runtime::SimulatorConfig{
            .state_type = runtime::SimulatorConfig::StateType::FullQuantum,
            .use_qulacs = true
    };
    auto simulator = runtime::Simulator(config, circuit->GetIR());
    simulator.RunAll();
    const auto& state = simulator.GetFullQuantumState();

    EXPECT_EQ(x, state.ReadRegister(0));
    if (x) {
        EXPECT_EQ(z, state.ReadRegister(1));
        EXPECT_EQ(y, state.ReadRegister(2));
    } else {
        EXPECT_EQ(y, state.ReadRegister(1));
        EXPECT_EQ(z, state.ReadRegister(2));
    }
}
INSTANTIATE_TEST_SUITE_P(
        Exhaustive,
        TestControlledSwap,
        testing::Combine(testing::Bool(), testing::Bool(), testing::Bool())
);
