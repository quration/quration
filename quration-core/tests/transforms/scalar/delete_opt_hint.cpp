#include "qret/transforms/scalar/delete_opt_hint.h"

#include <gtest/gtest.h>

#include "qret/frontend/argument.h"
#include "qret/frontend/attribute.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/intrinsic.h"
#include "qret/ir/function.h"

using namespace qret;

struct TestOptHintGen : public frontend::CircuitGenerator {
    static inline const char* Name = "TestOptHint";
    explicit TestOptHintGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 1, Attribute::Operate);
        arg.Add("r", Type::Register, 1, Attribute::Output);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto q = GetQubit(0);
        auto r = GetRegister(1);

        frontend::attribute::MarkAsClean(q);

        frontend::gate::X(q);
        frontend::gate::H(q);
        frontend::gate::Measure(q, r);

        frontend::control_flow::If(r);
        frontend::gate::X(q);
        frontend::control_flow::Else(r);
        frontend::attribute::MarkAsDirtyBegin(q);
        frontend::attribute::MarkAsDirtyEnd(q);
        frontend::control_flow::EndIf(r);

        frontend::attribute::MarkAsClean(q);

        return EndCircuitDefinition();
    }
};

bool HasOptHint(const ir::Function& func) {
    for (const auto& bb : func) {
        for (const auto& inst : bb) {
            if (inst.IsOptHint()) {
                return true;
            }
        }
    }
    return false;
}

TEST(DeleteOptHint, DeleteOptHint) {
    ir::IRContext context;
    auto* module = ir::Module::Create("delete_opt_hint", context);
    auto builder = frontend::CircuitBuilder(module);
    auto gen = TestOptHintGen(&builder);
    auto* func = gen.Generate();

    auto* ir = func->GetIR();

    EXPECT_TRUE(HasOptHint(*ir));
    ir::DeleteOptHint().RunOnFunction(*ir);
    EXPECT_FALSE(HasOptHint(*ir));
}
