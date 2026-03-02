#include <iostream>
#include <ostream>
#include <string>

#include "qret/analysis/printer.h"
#include "qret/frontend/argument.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/intrinsic.h"
#include "qret/transforms/external/external_pass.h"

using namespace qret;

namespace {
class SampleGen : frontend::CircuitGenerator {
public:
    static inline const char* Name = "SampleCircuit";
    explicit SampleGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator{builder} {}

    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return Name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("x", Type::Qubit, 3, Attribute::Operate);
        arg.Add("r", Type::Register, 1, Attribute::Output);
    }
    frontend::Circuit* Generate() const override {
        if (IsCached()) {
            return GetCachedCircuit();
        }
        BeginCircuitDefinition();

        const auto x = GetQubits(0);
        const auto r = GetRegister(1);

        frontend::gate::CCX(x[0], x[1], x[2]);
        frontend::gate::Measure(x[0], r);

        return EndCircuitDefinition();
    }
};

void Decompose() {
    ir::IRContext context;
    auto* module = ir::Module::Create("SampleModule", context);
    auto builder = frontend::CircuitBuilder(module);
    auto gen = SampleGen(&builder);
    auto* circuit = gen.Generate()->GetIR();

    std::cout << "Before decomposition:" << std::endl;
    std::cout << ToString(*circuit);

    std::cout << "----------------------------------------" << std::endl;
    std::cout << "Run decomposition." << std::endl;
    auto external_pass = ir::ExternalPass(
            // pass name
            "decompose_toffoli",
            // cmd
            "python quration-core/examples/external-pass/decompose.py in.json out.json "
            "SampleCircuit",
            // input
            "in.json",
            // output
            "out.json",
            // runner (unused)
            ""
    );
    external_pass.RunOnFunction(*circuit);
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "After decomposition:" << std::endl;
    std::cout << ToString(*circuit);
}
}  // namespace

int main() {
    Decompose();
    return 0;
}
