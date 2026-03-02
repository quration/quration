#include "qret/runtime/clifford_state.h"

#include <gtest/gtest.h>

#include <stdexcept>

#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/intrinsic.h"
#include "qret/runtime/simulator.h"

using namespace qret;
using namespace qret::runtime;

TEST(CliffordState, OneQubit) {
    std::mt19937_64 mt(std::random_device{}());
    auto state = CliffordState(mt(), 1);
    state.Measure(0, 0);
    EXPECT_EQ(state.ReadRegister(0), 0);
    state.X(0);
    state.Measure(0, 1);
    EXPECT_EQ(state.ReadRegister(1), 1);
    state.Y(0);
    state.Measure(0, 2);
    EXPECT_EQ(state.ReadRegister(2), 0);
    for (std::size_t i = 0; i < 10; ++i) {
        state.H(0);
        EXPECT_NEAR(state.Calc1Prob(0), 0.5, state.Eps);
        state.Measure(0, 3);
        if (state.ReadRegister(3)) {
            EXPECT_TRUE(state.Calc0Prob(0) < state.Eps);
            EXPECT_EQ(state.GetStabilizerGroup()[0].ToString(), "-1 Z");
        } else {
            EXPECT_TRUE(state.Calc1Prob(0) < state.Eps);
            EXPECT_EQ(state.GetStabilizerGroup()[0].ToString(), "+1 Z");
        }
        state.Measure(0, 4);
        EXPECT_EQ(state.ReadRegister(4), state.ReadRegister(3));
    }
    EXPECT_THROW(state.X(1), std::out_of_range);
    EXPECT_THROW(state.CX(0, 0), std::invalid_argument);
}
TEST(CliffordState, TwoQubits) {
    std::mt19937_64 mt(std::random_device{}());
    auto state = CliffordState(mt(), 2);
    state.Measure(0, 0);
    EXPECT_EQ(state.ReadRegister(0), 0);
    state.X(0);
    state.Measure(0, 1);
    EXPECT_EQ(state.ReadRegister(1), 1);
    state.CX(1, 0);
    state.Measure(0, 2);
    state.Measure(1, 3);
    EXPECT_EQ(state.ReadRegister(2), 1);
    EXPECT_EQ(state.ReadRegister(3), 1);
    state.H(0);
    state.CX(1, 0);
    EXPECT_NEAR(state.Calc1Prob(0), 0.5, state.Eps);
    EXPECT_NEAR(state.Calc1Prob(1), 0.5, state.Eps);
    state.Measure(0, 4);
    state.Measure(1, 5);
    EXPECT_NE(state.ReadRegister(4), state.ReadRegister(5));
}
TEST(CliffordState, ThreeQubits) {
    std::mt19937_64 mt(std::random_device{}());
    auto state = CliffordState(mt(), 3);
    state.H(0);
    state.CX(2, 0);
    EXPECT_TRUE(state.IsClean(1));
    EXPECT_FALSE(state.IsClean(0));
    EXPECT_FALSE(state.IsClean(2));
    state.Measure(0, 0);
    state.Measure(2, 1);
    EXPECT_EQ(state.ReadRegister(0), state.ReadRegister(1));
    EXPECT_THROW(state.H(4), std::out_of_range);
}
TEST(CliffordState, GetNumSuperpositions) {
    std::mt19937_64 mt(std::random_device{}());
    auto state = CliffordState(mt(), 4);
    EXPECT_EQ(state.GetNumSuperpositions(), 1);
    state.H(0);
    EXPECT_EQ(state.GetNumSuperpositions(), 2);
    state.H(2);
    EXPECT_EQ(state.GetNumSuperpositions(), 4);
    state.H(0);
    EXPECT_EQ(state.GetNumSuperpositions(), 2);
}
class TeleportZ1Gen : public frontend::CircuitGenerator {
private:
    static inline const char* name = "TeleportX0";

public:
    explicit TeleportZ1Gen(frontend::CircuitBuilder* builder)
        : frontend::CircuitGenerator(builder) {}
    std::string GetName() const override {
        return name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 3, Attribute::Operate);
        arg.Add("r", Type::Register, 3, Attribute::Output);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto q = GetQubits("q");
        auto r = GetRegisters("r");
        frontend::gate::X(q[0]);
        frontend::gate::H(q[1]);
        frontend::gate::CX(q[2], q[1]);

        frontend::gate::CX(q[1], q[0]);
        frontend::gate::H(q[0]);
        frontend::gate::Measure(q[0], r[0]);
        frontend::gate::Measure(q[1], r[1]);
        frontend::control_flow::If(r[1]);
        frontend::gate::X(q[2]);
        frontend::control_flow::EndIf(r[1]);
        frontend::control_flow::If(r[0]);
        frontend::gate::Z(q[2]);
        frontend::control_flow::EndIf(r[0]);

        frontend::gate::Measure(q[2], r[2]);
        return EndCircuitDefinition();
    }
};
TEST(CliffordState, Simulator) {
    auto context = qret::ir::IRContext();
    auto* module = qret::ir::Module::Create("module", context);
    auto builder = frontend::CircuitBuilder(module);
    auto gen = TeleportZ1Gen(&builder);
    auto* f_circuit = gen.Generate();
    auto* circuit = f_circuit->GetIR();
    auto simulator =
            Simulator(SimulatorConfig{.state_type = SimulatorConfig::StateType::Clifford}, circuit);
    simulator.RunAll();
    auto state = simulator.GetCliffordState();
    ASSERT_EQ(state.ReadRegister(2), true);
}
