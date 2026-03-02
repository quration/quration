#include "qret/target/sc_ls_fixed_v0/search_chip_comm.h"

#include <gtest/gtest.h>

#include <stdexcept>

#include "qret/base/string.h"
#include "qret/target/sc_ls_fixed_v0/topology.h"

using namespace qret;
using namespace qret::sc_ls_fixed_v0;

TEST(Topology, LoadDistributeNeighbor) {
    // z = 0, 2, 4, 6
    // 0 <---> 4 <---> 6
    const auto topology =
            Topology::FromYAML(LoadFile("quration-core/tests/data/topology/search_chip_comm.yaml"));

    auto searcher = SearchChipRoute(*topology);
    const auto graph = searcher.Search({0, 4});
    ASSERT_TRUE(graph.has_value());
    EXPECT_EQ(graph->NumNodes(), 2);
    EXPECT_EQ(graph->NumEdges(), 1);
    EXPECT_TRUE(graph->HasEdge(0, 4));
    EXPECT_FALSE(graph->HasEdge(4, 6));
}
TEST(Topology, LoadDistributeNotNeighbor) {
    // z = 0, 2, 4, 6
    // 0 <---> 4 <---> 6
    const auto topology =
            Topology::FromYAML(LoadFile("quration-core/tests/data/topology/search_chip_comm.yaml"));

    auto searcher = SearchChipRoute(*topology);
    const auto graph = searcher.Search({0, 6});
    ASSERT_TRUE(graph.has_value());
    EXPECT_EQ(graph->NumNodes(), 3);
    EXPECT_EQ(graph->NumEdges(), 2);
    EXPECT_TRUE(graph->HasEdge(0, 4));
    EXPECT_TRUE(graph->HasEdge(4, 6));
}
TEST(Topology, LoadDistributeConnectedAll) {
    // z = 0, 2, 4, 6
    // 0 <---> 4 <---> 6
    const auto topology =
            Topology::FromYAML(LoadFile("quration-core/tests/data/topology/search_chip_comm.yaml"));

    auto searcher = SearchChipRoute(*topology);
    const auto graph = searcher.Search({0, 4, 6});
    ASSERT_TRUE(graph.has_value());
    EXPECT_EQ(graph->NumNodes(), 3);
    EXPECT_EQ(graph->NumEdges(), 2);
    EXPECT_TRUE(graph->HasEdge(0, 4));
    EXPECT_TRUE(graph->HasEdge(4, 6));
}
TEST(Topology, LoadDistributeFail) {
    const auto topology =
            Topology::FromYAML(LoadFile("quration-core/tests/data/topology/search_chip_comm.yaml"));
    EXPECT_EQ(4, topology->NumGrids());

    auto searcher = SearchChipRoute(*topology);
    const auto graph = searcher.Search({0, 2});
    ASSERT_FALSE(graph.has_value());
}
TEST(Topology, LoadDistributeError) {
    const auto topology =
            Topology::FromYAML(LoadFile("quration-core/tests/data/topology/search_chip_comm.yaml"));
    EXPECT_EQ(4, topology->NumGrids());

    auto searcher = SearchChipRoute(*topology);
    EXPECT_THROW(searcher.Search({1, 3}), std::out_of_range);
}
