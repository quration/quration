#include <gtest/gtest.h>

#include <iostream>

#include "qret/base/type.h"
#include "qret/ir/function.h"
#include "qret/runtime/simulator.h"

#include "test_utils.h"

using namespace qret;
using runtime::Simulator, runtime::SimulatorConfig;

TEST(Adjoint, MAJ) {
    ir::IRContext context;
    tests::LoadContextFromJsonFile(
            "quration-core/tests/data/circuit/functor_adjoint.json",
            context
    );
    const auto& module = *context.owned_module.back();

    const auto* circuit = module.GetCircuit("Adjoint__000000000__");
    ASSERT_NE(nullptr, circuit);
    std::cout << "Adjoint" << std::endl;
    for (const auto& inst : *circuit->GetEntryBB()) {
        std::cout << "  ";
        inst.Print(std::cout);
        std::cout << std::endl;
    }

    std::cout << "__adjoint__MAJ" << std::endl;
    const auto* adjoint = module.GetCircuit("__adjoint__MAJ");
    ASSERT_NE(nullptr, adjoint);
    for (const auto& inst : *adjoint->GetEntryBB()) {
        std::cout << "  ";
        inst.Print(std::cout);
        std::cout << std::endl;
    }
}

TEST(Controlled, MAJ) {
    ir::IRContext context;
    tests::LoadContextFromJsonFile(
            "quration-core/tests/data/circuit/functor_controlled.json",
            context
    );
    const auto& module = *context.owned_module.back();
    const auto* circuit = module.GetCircuit("Controlled__000000000__");
    ASSERT_NE(nullptr, circuit);
    std::cout << "Controlled" << std::endl;
    for (const auto& inst : *circuit->GetEntryBB()) {
        std::cout << "  ";
        inst.Print(std::cout);
        std::cout << std::endl;
    }

    // Check
    {
        std::cout << "__controlled__MAJ" << std::endl;
        const auto* controlled = module.GetCircuit("__controlled__MAJ");
        ASSERT_NE(nullptr, controlled);
        EXPECT_EQ(4, controlled->GetNumQubits());
        for (const auto& inst : *controlled->GetEntryBB()) {
            std::cout << "  ";
            inst.Print(std::cout);
            std::cout << std::endl;
        }
    }
    {
        std::cout << "__controlled____controlled__MAJ" << std::endl;
        const auto* controlled_twice = module.GetCircuit("__controlled____controlled__MAJ");
        ASSERT_NE(nullptr, controlled_twice);
        EXPECT_EQ(5, controlled_twice->GetNumQubits());
        for (const auto& inst : *controlled_twice->GetEntryBB()) {
            std::cout << "  ";
            inst.Print(std::cout);
            std::cout << std::endl;
        }
    }
}

TEST(TestPivotFlip, UsingToffoliState) {
    const auto dst_size = std::size_t{6};
    const auto src_size = std::size_t{5};
    const BigInt dst_max = 1 << dst_size;
    const BigInt src_max = 1 << src_size;

    ir::IRContext context;
    const auto* circuit = tests::LoadCircuitFromJsonFile(
            "quration-core/tests/data/circuit/functor_pivot_flip_6_5.json",
            "PivotFlipUsingControlledFunctor(6,5)",
            context
    );
    ASSERT_NE(nullptr, circuit);

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};

    for (auto i = 0; i < 10; ++i) {
        for (auto j = 0; j < 10; ++j) {
            const auto dst_value = (1893 * i + 9348) % dst_max;
            const auto src_value = (3981 * j + 8439) % src_max;

            auto simulator = Simulator(config, circuit);
            simulator.SetValue(0, dst_size, dst_value);
            simulator.SetValue(dst_size, src_size, src_value);
            simulator.RunAll();

            const auto& state = simulator.GetToffoliState();
            const auto dst_expected =
                    dst_value < src_value ? BigInt(src_value - 1 - dst_value) : dst_value;
            EXPECT_EQ(dst_expected, state.GetValue(0, dst_size)) << dst_value << ", " << src_value;
            EXPECT_EQ(src_value, state.GetValue(dst_size, src_size));
            EXPECT_EQ(0, state.GetValue(dst_size + src_size, 2));
        }
    }
}
