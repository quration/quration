#include "qret/analysis/compute_graph.h"

#include <gtest/gtest.h>

#include <iostream>

#include "qret/analysis/visualizer.h"

#include "test_utils.h"

using namespace qret;

TEST(ComputeGraph, Generate) {
    auto context = ir::IRContext();
    const auto* circuit = tests::LoadCircuitFromJsonFile(
            "quration-core/tests/data/circuit/compute_graph.json",
            "CircGen__000000000__",
            context
    );
    ASSERT_NE(nullptr, circuit);
    const auto graph = GenComputeGraph(*circuit);
    std::cout << graph << std::endl;
}
