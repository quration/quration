#include "qret/algorithm/transform/qft.h"

#include <gtest/gtest.h>

#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/functor.h"
#include "qret/frontend/intrinsic.h"
#include "qret/runtime/simulator.h"

using namespace qret;
using frontend::CircuitGenerator, frontend::CircuitBuilder, runtime::SimulatorConfig,
        runtime::Simulator;

//--------------------------------------------------//
// Test FourierTransform
//--------------------------------------------------//
struct TestFourierTransformGen : public CircuitGenerator {
    std::uint64_t size;

    static inline const char* Name = "TestFourierTransform";
    explicit TestFourierTransformGen(CircuitBuilder* builder, std::uint64_t size)
        : CircuitGenerator(builder)
        , size{size} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("tgt", Type::Qubit, size, Attribute::Operate);
        arg.Add("out", Type::Register, size, Attribute::Output);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto tgt = GetQubits(0);
        auto out = GetRegisters(1);

        for (const auto& q : tgt) {
            frontend::gate::H(q);
        }
        frontend::gate::FourierTransform(tgt);
        for (auto i = std::size_t{0}; i < size; ++i) {
            frontend::gate::Measure(tgt[i], out[i]);
        }

        return EndCircuitDefinition();
    }
};
TEST(TestFourierTransform, UsingFullQuantumState) {
    const auto size = std::size_t{10};

    ir::IRContext context;
    auto* module = ir::Module::Create("qft", context);
    auto builder = CircuitBuilder(module);
    auto gen = TestFourierTransformGen(&builder, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{
            .state_type = SimulatorConfig::StateType::FullQuantum,
            .use_qulacs = true
    };
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.RunAll();

    const auto& state = simulator.GetState();
    for (auto i = std::size_t{0}; i < size; ++i) {
        EXPECT_EQ(0, state.ReadRegister(i));
    }
}
struct TestAdjFourierTransformGen : public CircuitGenerator {
    std::uint64_t size;

    static inline const char* Name = "TestAdjFourierTransform";
    explicit TestAdjFourierTransformGen(CircuitBuilder* builder, std::uint64_t size)
        : CircuitGenerator(builder)
        , size{size} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("tgt", Type::Qubit, size, Attribute::Operate);
        arg.Add("out", Type::Register, size, Attribute::Output);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto tgt = GetQubits(0);
        auto out = GetRegisters(1);

        frontend::Adjoint(frontend::gate::FourierTransform)(tgt);
        for (const auto& q : tgt) {
            frontend::gate::H(q);
        }
        for (auto i = std::size_t{0}; i < size; ++i) {
            frontend::gate::Measure(tgt[i], out[i]);
        }

        return EndCircuitDefinition();
    }
};
TEST(TestAdjFourierTransform, UsingFullQuantumState) {
    const auto size = std::size_t{10};

    ir::IRContext context;
    auto* module = ir::Module::Create("qft", context);
    auto builder = CircuitBuilder(module);
    auto gen = TestAdjFourierTransformGen(&builder, size);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{
            .state_type = SimulatorConfig::StateType::FullQuantum,
            .use_qulacs = true
    };
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.RunAll();

    const auto& state = simulator.GetState();
    for (auto i = std::size_t{0}; i < size; ++i) {
        EXPECT_EQ(0, state.ReadRegister(i));
    }
}
