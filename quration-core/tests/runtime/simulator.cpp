#include "qret/runtime/simulator.h"

#include <gtest/gtest.h>

#include "qret/base/type.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/intrinsic.h"

using namespace qret;

struct CRandGen : public frontend::CircuitGenerator {
    static inline const char* Name = "CRand";
    explicit CRandGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("c", Type::Register, 10, Attribute::Output);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        const auto c = GetRegisters(0);
        frontend::gate::DiscreteDistribution({0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0}, c.Range(2, 3));
        return EndCircuitDefinition();
    }
};

TEST(Simulator, DiscreteDistInst) {
    static constexpr auto NumSimulations = 10000;

    ir::IRContext context;
    auto* module = ir::Module::Create("crand", context);
    auto builder = frontend::CircuitBuilder(module);
    auto gen = CRandGen(&builder);
    auto* circuit = gen.Generate();

    auto hist = std::unordered_map<std::size_t, int>{};
    for (auto i = 0; i < NumSimulations; ++i) {
        auto config = runtime::SimulatorConfig{
                .state_type = runtime::SimulatorConfig::StateType::Toffoli,
                .seed = static_cast<std::uint64_t>(i)
        };
        auto simulator = runtime::Simulator(config, circuit->GetIR());
        simulator.RunAll();

        const auto val = BoolArrayAsInt(simulator.GetState().ReadRegisters(2, 3));
        EXPECT_TRUE(val == 1 || val == 3 || val == 4 || val == 6);

        if (hist.contains(val)) {
            hist[val] += 1;
        } else {
            hist[val] = 1;
        }
    }

    EXPECT_EQ(4, hist.size());
    for (const auto& [_, num] : hist) {
        EXPECT_LT(NumSimulations / 5, num);
        EXPECT_LT(num, NumSimulations / 3);
    }
}
