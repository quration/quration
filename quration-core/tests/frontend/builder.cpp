#include "qret/frontend/builder.h"

#include <gtest/gtest.h>

#include <fstream>

#include "qret/analysis/visualizer.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/intrinsic.h"
#include "qret/ir/context.h"
#include "qret/ir/json.h"

using namespace qret;

TEST(CircuitBuilder, LoadModule) {
    auto ifs = std::ifstream("quration-core/tests/data/ir/boolean.json");
    auto json = Json::parse(ifs);

    // Load boolean module.
    qret::ir::IRContext context;
    ir::LoadJson(json, context);
    auto* boolean_module = context.owned_module.front().get();

    auto* module = qret::ir::Module::Create("builder", context);
    auto builder = qret::frontend::CircuitBuilder(module);
    builder.LoadModule(boolean_module);

    EXPECT_TRUE(builder.Contains("TemporalAnd"));
    EXPECT_TRUE(builder.Contains("UncomputeTemporalAnd"));
}

struct MyCraigAdder : public frontend::CircuitGenerator {
    std::size_t n;
    frontend::Circuit& temporal_and;
    frontend::Circuit& uncompute_temporal_and;

    static inline const char* Name = "AddCraig";
    explicit MyCraigAdder(
            frontend::CircuitBuilder* builder,
            std::size_t n,
            frontend::Circuit& temporal_and,
            frontend::Circuit& uncompute_temporal_and
    )
        : CircuitGenerator(builder)
        , n{n}
        , temporal_and(temporal_and)
        , uncompute_temporal_and(uncompute_temporal_and) {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({})", GetName(), n);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("dst", Type::Qubit, n, Attribute::Operate);
        arg.Add("src", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, n - 1, Attribute::CleanAncilla);
    }
    frontend::Circuit* Generate() const override;
};
frontend::Circuit* MyCraigAdder::Generate() const {
    using frontend::gate::CX;
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto dst = GetQubits(0);
    auto src = GetQubits(1);
    auto aux = GetQubits(2);

    if (n == 1) {
        frontend::gate::CX(dst[0], src[0]);
        return EndCircuitDefinition();
    }

    assert(n >= 2);

    temporal_and(aux[0], src[0], dst[0]);
    for (auto i = std::size_t{1}; i < n - 1; ++i) {
        CX(src[i], aux[i - 1]);
        CX(dst[i], aux[i - 1]);
        temporal_and(aux[i], src[i], dst[i]);
        CX(aux[i], aux[i - 1]);
    }
    CX(dst[n - 1], aux[n - 2]);
    for (auto j = std::size_t{1}; j < n - 1; ++j) {
        const auto i = n - 1 - j;
        CX(aux[i], aux[i - 1]);
        uncompute_temporal_and(aux[i], src[i], dst[i]);
        CX(src[i], aux[i - 1]);
    }
    uncompute_temporal_and(aux[0], src[0], dst[0]);
    for (auto i = std::size_t{0}; i < n; ++i) {
        CX(dst[i], src[i]);
    }

    return EndCircuitDefinition();
}
TEST(CircuitBuilder, ConstructCraigAdderUsingLoadModule) {
    auto ifs = std::ifstream("quration-core/tests/data/ir/boolean.json");
    auto json = Json::parse(ifs);

    // Load boolean module.
    qret::ir::IRContext context;
    ir::LoadJson(json, context);
    auto* boolean_module = context.owned_module.front().get();

    auto* module = qret::ir::Module::Create("craig", context);
    auto builder = qret::frontend::CircuitBuilder(module);
    builder.LoadModule(boolean_module);

    ASSERT_TRUE(builder.Contains("TemporalAnd"));
    ASSERT_TRUE(builder.Contains("UncomputeTemporalAnd"));

    auto& temporal_and = *builder.GetCircuit("TemporalAnd");
    auto& uncompute_temporal_and = *builder.GetCircuit("UncomputeTemporalAnd");

    auto gen = MyCraigAdder(&builder, 5, temporal_and, uncompute_temporal_and);
    auto* adder = gen.Generate();

    for (const auto& bb : *adder->GetIR()) {
        for (const auto& inst : bb) {
            inst.Print(std::cout);
            std::cout << std::endl;
        }
    }

    std::cout << Json(*module).dump(4) << std::endl;
}

struct NestedBranchGen : public frontend::CircuitGenerator {
    static inline const char* Name = "NestedBranchGen";
    explicit NestedBranchGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}", GetName());
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("tgt", Type::Qubit, 10, Attribute::Operate);
        arg.Add("reg", Type::Register, 10, Attribute::Output);
    }
    frontend::Circuit* Generate() const override {
        if (IsCached()) {
            return GetCachedCircuit();
        }
        BeginCircuitDefinition();

        auto tgt = GetQubits("tgt");
        auto reg = GetRegisters("reg");

        frontend::gate::X(tgt[0]);

        frontend::control_flow::If(reg[0], "reg_0");
        {
            frontend::gate::X(tgt[1]);

            frontend::control_flow::If(reg[1], "reg_1");
            {
                frontend::gate::X(tgt[2]);

                frontend::control_flow::If(reg[2]);
                {
                    //
                    frontend::gate::X(tgt[3]);
                }
                frontend::control_flow::EndIf(reg[2]);

                frontend::gate::X(tgt[4]);
            }
            frontend::control_flow::Else(reg[1]);
            {
                //
                frontend::gate::X(tgt[5]);
            }
            frontend::control_flow::EndIf(reg[1]);

            frontend::gate::X(tgt[6]);
        }
        frontend::control_flow::EndIf(reg[0]);

        frontend::gate::X(tgt[7]);

        return EndCircuitDefinition();
    }
};
TEST(CircuitBuilder, NestedBranch) {
    qret::ir::IRContext context;
    auto* module = qret::ir::Module::Create("nested_branch", context);
    auto builder = qret::frontend::CircuitBuilder(module);

    auto gen = NestedBranchGen(&builder);
    auto* nested_branch = gen.Generate();

    for (const auto& bb : *nested_branch->GetIR()) {
        std::cout << '%' << bb.GetName() << std::endl;
        for (const auto& inst : bb) {
            std::cout << "  ";
            inst.Print(std::cout);
            std::cout << std::endl;
        }
    }

    std::cout << Json(*module).dump(4) << std::endl;

    std::cout << GenCFG(*nested_branch->GetIR()) << std::endl;
}

struct NestedSwitchGen : public frontend::CircuitGenerator {
    static inline const char* Name = "NestedSwitchGen";
    explicit NestedSwitchGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}", GetName());
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("tgt", Type::Qubit, 10, Attribute::Operate);
        arg.Add("reg", Type::Register, 10, Attribute::Output);
    }
    frontend::Circuit* Generate() const override {
        if (IsCached()) {
            return GetCachedCircuit();
        }
        BeginCircuitDefinition();

        auto tgt = GetQubits("tgt");
        auto reg = GetRegisters("reg");

        frontend::gate::X(tgt[0]);

        frontend::control_flow::Switch(reg.Range(0, 3), "reg_0-3");
        {
            frontend::control_flow::Case(reg.Range(0, 3), 1);
            frontend::gate::X(tgt[1]);

            frontend::control_flow::Switch(reg.Range(3, 3), "reg_3-6");
            {
                frontend::control_flow::Case(reg.Range(3, 3), 1);
                frontend::gate::X(tgt[1]);

                frontend::control_flow::Case(reg.Range(3, 3), 7);
                frontend::gate::X(tgt[2]);

                frontend::control_flow::Default(reg.Range(3, 3));
                frontend::gate::X(tgt[3]);
            }
            frontend::control_flow::EndSwitch(reg.Range(3, 3));

            frontend::gate::X(tgt[4]);

            frontend::control_flow::Case(reg.Range(0, 3), 3);
            frontend::gate::X(tgt[5]);

            frontend::control_flow::Case(reg.Range(0, 3), 6);
            frontend::gate::X(tgt[6]);

            frontend::control_flow::Default(reg.Range(0, 3));
            frontend::gate::X(tgt[7]);
        }
        frontend::control_flow::EndSwitch(reg.Range(0, 3));

        frontend::gate::X(tgt[7]);

        return EndCircuitDefinition();
    }
};
TEST(CircuitBuilder, NestedSwitch) {
    qret::ir::IRContext context;
    auto* module = qret::ir::Module::Create("nested_branch", context);
    auto builder = qret::frontend::CircuitBuilder(module);

    auto gen = NestedSwitchGen(&builder);
    auto* nested_switch = gen.Generate();

    for (const auto& bb : *nested_switch->GetIR()) {
        std::cout << '%' << bb.GetName() << std::endl;
        for (const auto& inst : bb) {
            std::cout << "  ";
            inst.Print(std::cout);
            std::cout << std::endl;
        }
    }

    std::cout << Json(*module).dump(4) << std::endl;

    std::cout << GenCFG(*nested_switch->GetIR()) << std::endl;
}
