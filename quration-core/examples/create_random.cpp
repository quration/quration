#include <fmt/core.h>

#include <fstream>

#include "qret/frontend/argument.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/control_flow.h"
#include "qret/frontend/intrinsic.h"
#include "qret/ir/context.h"
#include "qret/ir/json.h"

using namespace qret;

struct RotGen : public frontend::CircuitGenerator {
    std::int32_t p;

    static inline const char* Name = "Rot";
    explicit RotGen(frontend::CircuitBuilder* builder, std::int32_t p)
        : CircuitGenerator(builder)
        , p{p} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({})", GetName(), p);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 1, Attribute::Operate);
    }
    frontend::Circuit* Generate() const override {
        if (IsCached()) {
            return GetCachedCircuit();
        }
        BeginCircuitDefinition();

        const auto q = GetQubit(0);

        switch (p) {
            case 0:
                frontend::gate::S(q);
                break;
            case 1:
                frontend::gate::T(q);
                break;
            case 2:
                frontend::gate::H(q);
                break;
            default:
                frontend::gate::RZ(q, 0.1, 0.0001);
                break;
        }

        return EndCircuitDefinition();
    }
};
void Rot(frontend::Qubit q, std::int32_t p) {
    auto* circuit = RotGen(q.GetBuilder(), p).Generate();
    (*circuit)(q);
}

struct RandomGen : public frontend::CircuitGenerator {
    static inline const char* Name = "Random";
    explicit RandomGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return Name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 1, Attribute::Operate);
        arg.Add("c", Type::Register, 3, Attribute::Output);
    }
    frontend::Circuit* Generate() const override {
        if (IsCached()) {
            return GetCachedCircuit();
        }
        BeginCircuitDefinition();

        const auto q = GetQubit(0);
        const auto c = GetRegisters(1);

        for (auto i = 0; i < 100; ++i) {
            // Set random value in c.
            frontend::gate::DiscreteDistribution({0.1, 0.3, 0.5, 0.05, 0.05}, c);

            frontend::control_flow::Switch(c);
            frontend::control_flow::Case(c, 0);
            Rot(q, 0);
            frontend::control_flow::Case(c, 1);
            Rot(q, 1);
            frontend::control_flow::Case(c, 2);
            Rot(q, 2);
            frontend::control_flow::Default(c);
            Rot(q, 3);
            frontend::control_flow::EndSwitch(c);
        }

        return EndCircuitDefinition();
    }
};

int main() {
    // create circuit
    qret::ir::IRContext context;
    auto* module = qret::ir::Module::Create("random", context);
    auto builder = qret::frontend::CircuitBuilder(module);
    auto gen = RandomGen(&builder);
    gen.Generate();

    // serialization
    auto j = qret::Json();
    j = *builder.GetModule();

    // write json to file
    auto ofs = std::ofstream("random.json");
    ofs << std::setw(4) << j;

    return 0;
}
