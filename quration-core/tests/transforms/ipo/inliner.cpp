#include "qret/transforms/ipo/inliner.h"

#include <gtest/gtest.h>

#include <iostream>

#include "qret/analysis/visualizer.h"

#include "test_utils.h"

using namespace qret;
using ir::InlinerPass, ir::RecursiveInlinerPass;

TEST(Inliner, SingleBBNotMerge) {
    ir::IRContext context;
    auto* circuit = tests::LoadCircuitFromJsonFile(
            "quration-core/tests/data/circuit/inliner_test.json",
            "Test__000000000__",
            context
    );
    ASSERT_NE(nullptr, circuit);

    std::cout << "==========================" << std::endl;
    std::cout << GenCFG(*circuit) << std::endl;

    auto inliner = InlinerPass();
    inliner.merge_trivial = false;
    inliner.RunOnFunction(*circuit);

    std::cout << "==========================" << std::endl;
    for (const auto& bb : *circuit) {
        std::cout << "BB: " << bb.GetName() << std::endl;
        for (const auto& inst : bb) {
            std::cout << "  ";
            inst.Print(std::cout);
            std::cout << std::endl;
        }
    }

    std::cout << "==========================" << std::endl;
    std::cout << GenCFG(*circuit) << std::endl;
}
TEST(Inliner, SingleBBMerge) {
    ir::IRContext context;
    auto* circuit = tests::LoadCircuitFromJsonFile(
            "quration-core/tests/data/circuit/inliner_test.json",
            "Test__000000000__",
            context
    );
    ASSERT_NE(nullptr, circuit);
    auto inliner = InlinerPass();
    inliner.merge_trivial = true;
    inliner.RunOnFunction(*circuit);

    for (const auto& bb : *circuit) {
        std::cout << "BB: " << bb.GetName() << std::endl;
        for (const auto& inst : bb) {
            std::cout << "  ";
            inst.Print(std::cout);
            std::cout << std::endl;
        }
    }

    std::cout << "==========================" << std::endl;
    std::cout << GenCFG(*circuit) << std::endl;
}
TEST(RecursiveInliner, SingleBBNotMerge) {
    ir::IRContext context;
    auto* circuit = tests::LoadCircuitFromJsonFile(
            "quration-core/tests/data/circuit/inliner_test.json",
            "Test__000000000__",
            context
    );
    ASSERT_NE(nullptr, circuit);
    auto inliner = RecursiveInlinerPass();
    inliner.merge_trivial = false;
    inliner.RunOnFunction(*circuit);

    for (const auto& bb : *circuit) {
        std::cout << "BB: " << bb.GetName() << std::endl;
        for (const auto& inst : bb) {
            std::cout << "  ";
            inst.Print(std::cout);
            std::cout << std::endl;
        }
    }

    std::cout << "==========================" << std::endl;
    std::cout << GenCFG(*circuit) << std::endl;
}
TEST(RecursiveInliner, SingleBBMerge) {
    ir::IRContext context;
    auto* circuit = tests::LoadCircuitFromJsonFile(
            "quration-core/tests/data/circuit/inliner_test.json",
            "Test__000000000__",
            context
    );
    ASSERT_NE(nullptr, circuit);
    auto inliner = RecursiveInlinerPass();
    inliner.merge_trivial = true;
    inliner.RunOnFunction(*circuit);

    for (const auto& bb : *circuit) {
        std::cout << "BB: " << bb.GetName() << std::endl;
        for (const auto& inst : bb) {
            std::cout << "  ";
            inst.Print(std::cout);
            std::cout << std::endl;
        }
    }

    std::cout << "==========================" << std::endl;
    std::cout << GenCFG(*circuit) << std::endl;
}
