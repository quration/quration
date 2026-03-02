#include "qret/transforms/scalar/static_condition_pruning.h"

#include <gtest/gtest.h>

#include "qret/base/cast.h"
#include "qret/base/log.h"
#include "qret/base/portable_function.h"
#include "qret/frontend/argument.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/intrinsic.h"
#include "qret/ir/function.h"

using namespace qret;

/*
 * Test matrix:
 *  - PruningBranch / PruningSwitch / PruningSwitch2 / PruningDefault:
 *    confirm straightforward unconditional redirection cases.
 *  - PruningClassical / PruningMeasurement / PruningMerge:
 *    confirm side-effectful classical updates and merge dynamics.
 *  - MergeInconsistentStaticPredicatesMustRemainConditional:
 *    regression test for the predecessor-merge bug.
 *
 * Each test encodes one "expected pass behavior" and acts as a compact
 * executable specification for this compile-time optimization.
 */

enum class Mode : std::uint8_t {
    BranchInst,
    SwitchInst,
    SwitchInst2,
    Default,
    CallCFInst,
    Measurement,
    MergeInconsistentPred,
    Merge,
};

struct TestPruningGen : public frontend::CircuitGenerator {
    static inline const char* Name = "TestPruning";
    explicit TestPruningGen(frontend::CircuitBuilder* builder, Mode mode)
        : CircuitGenerator(builder)
        , mode_{mode} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 1, Attribute::Operate);
        arg.Add("r", Type::Register, 5, Attribute::Output);
    }

    void GenerateBranch() const {
        auto q = GetQubit(0);
        auto r = GetRegisters(1);

        // Single conditional branch where r[0] is fixed by a deterministic
        // distribution and should be pruned to unconditional flow.
        frontend::gate::DiscreteDistribution({1.0, 0.0, 0.0, 0.0, 0.0}, r);

        frontend::control_flow::If(r[0]);
        frontend::gate::X(q);  // Prune this branch (will be erased)
        frontend::control_flow::Else(r[0]);
        frontend::gate::Y(q);
        frontend::control_flow::EndIf(r[0]);

        frontend::gate::H(q);
    }

    void GenerateSwitch() const {
        auto q = GetQubit(0);
        auto r = GetRegisters(1);

        // Deterministic 4-bit switch value should make only one case executable.
        frontend::gate::DiscreteDistribution({0.0, 0.0, 0.0, 1.0, 0.0}, r);

        // r[0] = r[1] = 1
        // r[2] = r[3] = r[4] = 0

        frontend::control_flow::Switch(r.Range(0, 4));
        frontend::control_flow::Case(r.Range(0, 4), 0);
        frontend::gate::X(q);  // Prune this branch (will be erased)
        frontend::control_flow::Case(r.Range(0, 4), 3);
        frontend::gate::Y(q);
        frontend::control_flow::Default(r.Range(0, 4));
        frontend::gate::Z(q);  // Prune this branch (will be erased)
        frontend::control_flow::EndSwitch(r.Range(0, 4));

        frontend::gate::H(q);
    }

    void GenerateSwitch2() const {
        auto q = GetQubit(0);
        auto r = GetRegisters(1);

        // Covers narrower value-range switch (2 bits) with a dominant deterministic pattern.
        frontend::gate::DiscreteDistribution({0.0, 0.0, 0.0, 1.0, 0.0}, r);

        // r[0] = r[1] = 1
        // r[2] = r[3] = r[4] = 0

        frontend::control_flow::Switch(r.Range(1, 2));
        frontend::control_flow::Case(r.Range(1, 2), 1);
        frontend::gate::X(q);
        frontend::control_flow::Case(r.Range(1, 2), 3);
        frontend::gate::Y(q);  // Prune this branch (will be erased)
        frontend::control_flow::Default(r.Range(1, 2));
        frontend::gate::Z(q);  // Prune this branch (will be erased)
        frontend::control_flow::EndSwitch(r.Range(1, 2));

        frontend::gate::H(q);
    }

    void GenerateDefault() const {
        auto q = GetQubit(0);
        auto r = GetRegisters(1);

        // Ensure default branch remains or disappears depending on known switch value.
        frontend::gate::DiscreteDistribution({0.0, 0.0, 0.0, 0.0, 1.0}, r);

        frontend::control_flow::Switch(r.Range(0, 4));
        frontend::control_flow::Case(r.Range(0, 4), 0);
        frontend::gate::X(q);  // Prune this branch (will be erased)
        frontend::control_flow::Case(r.Range(0, 4), 3);
        frontend::gate::Y(q);  // Prune this branch (will be erased)
        frontend::control_flow::Default(r.Range(0, 4));
        frontend::gate::Z(q);
        frontend::control_flow::EndSwitch(r.Range(0, 4));

        frontend::gate::H(q);
    }

    static PortableAnnotatedFunction CopyFunc() {
        auto builder = PortableFunctionBuilder();
        auto input = builder.AddBoolArrayInputVariable("input", 1);
        auto output = builder.AddBoolArrayOutputVariable("output", 2);
        output[0] = input[0];
        output[1] = input[0];
        return PortableAnnotatedFunction(builder.Compile("copy"));
    }

    void GenerateClassical() const {
        auto q = GetQubit(0);
        auto r = GetRegisters(1);

        // r[0..1] from a deterministic distribution are then transformed by a
        // pure classical function to verify static inference propagates through CallCF.
        frontend::gate::DiscreteDistribution({0.0, 0.0, 0.0, 0.0, 1.0}, r);

        frontend::gate::CallCF(TestPruningGen::CopyFunc(), r.Range(2, 1), r.Range(0, 2));

        frontend::control_flow::Switch(r.Range(0, 4));
        frontend::control_flow::Case(r.Range(0, 4), 0);
        frontend::gate::X(q);  // Prune this branch (will be erased)
        frontend::control_flow::Case(r.Range(0, 4), 3);
        frontend::gate::Y(q);  // Prune this branch (will be erased)
        frontend::control_flow::Case(r.Range(0, 4), 7);
        frontend::gate::Z(q);
        frontend::control_flow::Default(r.Range(0, 4));
        frontend::gate::S(q);  // Prune this branch (will be erased)
        frontend::control_flow::EndSwitch(r.Range(0, 4));

        frontend::gate::H(q);
    }

    void GenerateMeasurement() const {
        auto q = GetQubit(0);
        auto r = GetRegisters(1);

        // Measurement writes dynamic value into r[3], so switch on r should not be resolved.
        frontend::gate::DiscreteDistribution({0.0, 0.0, 0.0, 0.0, 1.0}, r);
        frontend::gate::Measure(q, r[3]);

        // r[3] is dynamic, so cannot prune.

        frontend::control_flow::Switch(r.Range(0, 4));
        frontend::control_flow::Case(r.Range(0, 4), 0);
        frontend::gate::X(q);
        frontend::control_flow::Case(r.Range(0, 4), 3);
        frontend::gate::Y(q);
        frontend::control_flow::Case(r.Range(0, 4), 7);
        frontend::gate::Z(q);
        frontend::control_flow::Default(r.Range(0, 4));
        frontend::gate::S(q);
        frontend::control_flow::EndSwitch(r.Range(0, 4));

        frontend::gate::H(q);
    }

    static PortableAnnotatedFunction NotFunc() {
        auto builder = PortableFunctionBuilder();
        auto input = builder.AddBoolArrayInputVariable("input", 1);
        auto output = builder.AddBoolArrayOutputVariable("output", 1);
        output[0] = !input[0];
        return PortableAnnotatedFunction(builder.Compile("not"));
    }

    void GenerateMerge() const {
        auto q = GetQubit(0);
        auto r = GetRegisters(1);

        // Exercises merge-flow reasoning where a branch writes and another branch does not,
        // causing r[1] to become dynamic after merge.
        frontend::gate::DiscreteDistribution({0.0, 0.0, 0.0, 0.0, 1.0}, r.Range(0, 3));

        // r[0], r[1], r[2] are static
        // r[0] = r[1] = 0, r[2] = 1

        frontend::control_flow::If(r[3]);

        frontend::gate::CallCF(TestPruningGen::NotFunc(), r.Range(0, 1), r.Range(1, 1));
        // r[0] = 0, r[1] = r[2] = 1

        frontend::control_flow::Else(r[3]);

        // r[0] = r[1] = 0, r[2] = 1

        frontend::control_flow::EndIf(r[3]);

        // r[0] = 0, r[2] = 1
        // r[1] is dynamic

        frontend::control_flow::If(r[0]);
        frontend::gate::X(q);  // Prune this branch (will be erased)
        frontend::control_flow::Else(r[0]);
        frontend::gate::Y(q);
        frontend::control_flow::EndIf(r[0]);

        frontend::control_flow::If(r[1]);
        frontend::gate::Z(q);
        frontend::control_flow::Else(r[1]);
        frontend::gate::S(q);
        frontend::control_flow::EndIf(r[1]);

        frontend::gate::H(q);
    }

    void GenerateMergeInconsistentPred() const {
        auto q = GetQubit(0);
        auto r = GetRegisters(1);

        // r0 is dynamic (input-dependent), so both CFG branches after the merge
        // can be reached depending on run-time data.
        frontend::control_flow::If(r[0]);

        // True branch sets r1 deterministically.
        // This alone should never make r1 globally static after the merge because
        // r1 is uninitialized/unknown in the false branch.
        frontend::gate::DiscreteDistribution({0.0, 1.0}, r.Range(1, 1));
        frontend::gate::X(q);

        frontend::control_flow::Else(r[0]);

        // False branch does not write r1.
        frontend::gate::Y(q);

        frontend::control_flow::EndIf(r[0]);

        // The key regression here: if merge logic is wrong and only checks intersection
        // of predecessor values, it may incorrectly treat r1 as static and prune this branch.
        frontend::control_flow::If(r[1]);
        frontend::gate::Z(q);
        frontend::control_flow::Else(r[1]);
        frontend::gate::H(q);
        frontend::control_flow::EndIf(r[1]);
    }

    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        switch (mode_) {
            case Mode::BranchInst:
                GenerateBranch();
                break;
            case Mode::SwitchInst:
                GenerateSwitch();
                break;
            case Mode::SwitchInst2:
                GenerateSwitch2();
                break;
            case Mode::Default:
                GenerateDefault();
                break;
            case Mode::CallCFInst:
                GenerateClassical();
                break;
            case Mode::Measurement:
                GenerateMeasurement();
                break;
            case Mode::Merge:
                GenerateMerge();
                break;
            case Mode::MergeInconsistentPred:
                GenerateMergeInconsistentPred();
                break;
            default:
                break;
        }
        return EndCircuitDefinition();
    }

private:
    Mode mode_;
};

void PrintCircuit(const ir::Function& func) {
    std::cout << "==================================" << std::endl;
    for (const auto& bb : func) {
        std::cout << "%" << bb.GetName() << std::endl;
        for (auto itr = bb.pred_begin(); itr != bb.pred_end(); ++itr) {
            std::cout << "  pred: %" << (*itr)->GetName() << "--->%" << bb.GetName() << std::endl;
        }
        for (auto itr = bb.succ_begin(); itr != bb.succ_end(); ++itr) {
            std::cout << "  succ: %" << bb.GetName() << "--->%" << (*itr)->GetName() << std::endl;
        }
        for (const auto& inst : bb) {
            std::cout << "    ";
            inst.Print(std::cout);
            std::cout << std::endl;
        }
    }
}

TEST(StaticConditionPruning, PruningBranch) {
    ir::IRContext context;
    auto* module = ir::Module::Create("delete_opt_hint", context);
    auto builder = frontend::CircuitBuilder(module);
    auto gen = TestPruningGen(&builder, Mode::BranchInst);
    auto* func = gen.Generate();

    auto* ir = func->GetIR();

    EXPECT_TRUE(ir::StaticConditionPruningPass().RunOnFunction(*ir));

    PrintCircuit(*ir);
}

TEST(StaticConditionPruning, PruningSwitch) {
    Logger::EnableConsoleOutput();
    Logger::SetLogLevel(LogLevel::Debug);

    ir::IRContext context;
    auto* module = ir::Module::Create("delete_opt_hint", context);
    auto builder = frontend::CircuitBuilder(module);
    auto gen = TestPruningGen(&builder, Mode::SwitchInst);
    auto* func = gen.Generate();

    auto* ir = func->GetIR();

    EXPECT_TRUE(ir::StaticConditionPruningPass().RunOnFunction(*ir));

    PrintCircuit(*ir);
}

TEST(StaticConditionPruning, PruningSwitch2) {
    Logger::EnableConsoleOutput();
    Logger::SetLogLevel(LogLevel::Debug);

    ir::IRContext context;
    auto* module = ir::Module::Create("delete_opt_hint", context);
    auto builder = frontend::CircuitBuilder(module);
    auto gen = TestPruningGen(&builder, Mode::SwitchInst2);
    auto* func = gen.Generate();

    auto* ir = func->GetIR();

    EXPECT_TRUE(ir::StaticConditionPruningPass().RunOnFunction(*ir));

    PrintCircuit(*ir);
}

TEST(StaticConditionPruning, PruningDefault) {
    ir::IRContext context;
    auto* module = ir::Module::Create("delete_opt_hint", context);
    auto builder = frontend::CircuitBuilder(module);
    auto gen = TestPruningGen(&builder, Mode::Default);
    auto* func = gen.Generate();

    auto* ir = func->GetIR();

    EXPECT_TRUE(ir::StaticConditionPruningPass().RunOnFunction(*ir));

    PrintCircuit(*ir);
}

TEST(StaticConditionPruning, PruningClassical) {
    ir::IRContext context;
    auto* module = ir::Module::Create("delete_opt_hint", context);
    auto builder = frontend::CircuitBuilder(module);
    auto gen = TestPruningGen(&builder, Mode::CallCFInst);
    auto* func = gen.Generate();

    auto* ir = func->GetIR();

    EXPECT_TRUE(ir::StaticConditionPruningPass().RunOnFunction(*ir));

    PrintCircuit(*ir);
}

TEST(StaticConditionPruning, PruningMeasurement) {
    ir::IRContext context;
    auto* module = ir::Module::Create("delete_opt_hint", context);
    auto builder = frontend::CircuitBuilder(module);
    auto gen = TestPruningGen(&builder, Mode::Measurement);
    auto* func = gen.Generate();

    auto* ir = func->GetIR();

    EXPECT_FALSE(ir::StaticConditionPruningPass().RunOnFunction(*ir));

    PrintCircuit(*ir);
}

TEST(StaticConditionPruning, PruningMerge) {
    ir::IRContext context;
    auto* module = ir::Module::Create("delete_opt_hint", context);
    auto builder = frontend::CircuitBuilder(module);
    auto gen = TestPruningGen(&builder, Mode::Merge);
    auto* func = gen.Generate();

    auto* ir = func->GetIR();

    EXPECT_TRUE(ir::StaticConditionPruningPass().RunOnFunction(*ir));

    PrintCircuit(*ir);
}

TEST(StaticConditionPruning, MergeInconsistentStaticPredicatesMustRemainConditional) {
    ir::IRContext context;
    auto* module = ir::Module::Create("delete_opt_hint", context);
    auto builder = frontend::CircuitBuilder(module);
    auto gen = TestPruningGen(&builder, Mode::MergeInconsistentPred);
    auto* func = gen.Generate();

    auto* ir = func->GetIR();

    // Expected result: the second if (r[1]) cannot be proven at compile time
    // because r1 is dynamic at merge point. Static pruning must not rewrite it
    // into an unconditional branch.
    EXPECT_FALSE(ir::StaticConditionPruningPass().RunOnFunction(*ir));

    auto conditional_count = std::size_t{0};
    for (const auto& bb : *ir) {
        if (const auto* branch = DynCast<ir::BranchInst>(bb.GetTerminator())) {
            // Each remaining conditional terminator is counted as evidence that
            // dynamic branches were not incorrectly removed.
            conditional_count += branch->IsConditional() ? 1 : 0;
        }
    }
    EXPECT_EQ(conditional_count, 2);
}
