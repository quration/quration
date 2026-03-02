#include <gtest/gtest.h>

#include <stdexcept>

#include "qret/frontend/attribute.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/intrinsic.h"
#include "qret/ir/instructions.h"
#include "qret/runtime/simulator.h"

using namespace qret;

ir::CallInst* A(const frontend::Qubit& q);
ir::CallInst* B(const frontend::Qubit& q);
ir::CallInst* C(const frontend::Qubit& q);

struct AGen : public frontend::CircuitGenerator {
    static inline const char* Name = "A";
    explicit AGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 1, Attribute::Operate);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto q = GetQubit(0);
        MARK_AS_CLEAN(q);
        return EndCircuitDefinition();
    }
};
struct BGen : public frontend::CircuitGenerator {
    static inline const char* Name = "B";
    explicit BGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 1, Attribute::Operate);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto q = GetQubit(0);
        A(q);
        return EndCircuitDefinition();
    }
};
struct CGen : public frontend::CircuitGenerator {
    static inline const char* Name = "C";
    explicit CGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 1, Attribute::Operate);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto q = GetQubit(0);
        frontend::gate::H(q);
        B(q);
        return EndCircuitDefinition();
    }
};

ir::CallInst* A(const frontend::Qubit& q) {
    return (*AGen(q.GetBuilder()).Generate())(q);
}
ir::CallInst* B(const frontend::Qubit& q) {
    return (*BGen(q.GetBuilder()).Generate())(q);
}
ir::CallInst* C(const frontend::Qubit& q) {
    return (*CGen(q.GetBuilder()).Generate())(q);
}

TEST(CallStack, AssertCleanError) {
    ir::IRContext context;
    auto* module = ir::Module::Create("call_stack", context);
    auto builder = frontend::CircuitBuilder(module);
    auto gen = CGen(&builder);
    auto* circuit = gen.Generate();

    auto config = runtime::SimulatorConfig{runtime::SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 2;
    auto simulator = runtime::Simulator(config, circuit->GetIR());
    EXPECT_THROW(simulator.RunAll(), std::runtime_error);
}
TEST(CallStack, MaxSuperpositionsError) {
    ir::IRContext context;
    auto* module = ir::Module::Create("call_stack", context);
    auto builder = frontend::CircuitBuilder(module);
    auto gen = CGen(&builder);
    auto* circuit = gen.Generate();

    const auto config = runtime::SimulatorConfig{runtime::SimulatorConfig::StateType::Toffoli};
    auto simulator = runtime::Simulator(config, circuit->GetIR());
    EXPECT_THROW(simulator.RunAll(), std::runtime_error);
}
TEST(CallStack, AssertCleanErrorMessage) {
    ir::IRContext context;
    auto* module = ir::Module::Create("call_stack", context);
    auto builder = frontend::CircuitBuilder(module);
    auto gen = CGen(&builder);
    auto* circuit = gen.Generate();

    auto config = runtime::SimulatorConfig{runtime::SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 2;
    auto simulator = runtime::Simulator(config, circuit->GetIR());
    try {
        simulator.RunAll();
    } catch (std::runtime_error& ex) {
        const auto& message = std::string(ex.what());
        EXPECT_TRUE(message.starts_with("q0 is not clean"));
    }
}
TEST(CallStack, MaxSuperpositionsErrorMessage) {
    ir::IRContext context;
    auto* module = ir::Module::Create("call_stack", context);
    auto builder = frontend::CircuitBuilder(module);
    auto gen = CGen(&builder);
    auto* circuit = gen.Generate();

    const auto config = runtime::SimulatorConfig{runtime::SimulatorConfig::StateType::Toffoli};
    auto simulator = runtime::Simulator(config, circuit->GetIR());
    try {
        simulator.RunAll();
    } catch (std::runtime_error& ex) {
        const auto& message = std::string(ex.what());
        EXPECT_TRUE(message.starts_with("the number of superpositions(=2) exceeds the limits(=1)"));
    }
}
