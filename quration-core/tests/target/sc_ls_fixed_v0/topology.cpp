#include "qret/target/sc_ls_fixed_v0/topology.h"

#include <gtest/gtest.h>

#include <stdexcept>

#include "qret/base/json.h"
#include "qret/base/log.h"
#include "qret/base/string.h"

using namespace qret;
using namespace qret::sc_ls_fixed_v0;

TEST(Topology, LoadPlane) {
    auto topology = Topology::FromYAML(LoadFile("quration-core/tests/data/topology/plane.yaml"));
    EXPECT_EQ(1, topology->NumGrids());

    const auto& grid = topology->GetGrid(3);
    EXPECT_TRUE(grid.IsPlane());
    EXPECT_EQ(10, grid.GetMaxX());
    EXPECT_EQ(10, grid.GetMaxY());
    EXPECT_EQ(3, grid.GetMinZ());
    EXPECT_EQ(4, grid.GetMaxZ());

    const auto& plane = grid.GetPlane(3);
    EXPECT_EQ(10, plane.GetMaxX());
    EXPECT_EQ(10, plane.GetMaxY());
}
TEST(Topology, LoadGrid) {
    auto topology = Topology::FromYAML(LoadFile("quration-core/tests/data/topology/grid.yaml"));
    EXPECT_EQ(1, topology->NumGrids());

    const auto& grid = topology->GetGrid(4);
    EXPECT_FALSE(grid.IsPlane());
    EXPECT_EQ(5, grid.GetMaxX());
    EXPECT_EQ(5, grid.GetMaxY());
    EXPECT_EQ(2, grid.GetMinZ());
    EXPECT_EQ(6, grid.GetMaxZ());
}
TEST(Topology, LoadDistribute) {
    auto topology =
            Topology::FromYAML(LoadFile("quration-core/tests/data/topology/distribute.yaml"));
    EXPECT_EQ(4, topology->NumGrids());
}
TEST(Topology, SerializePlaneToJson) {
    auto topology = Topology::FromYAML(LoadFile("quration-core/tests/data/topology/plane.yaml"));
    std::cout << Json(*topology) << std::endl;
    const auto json = Json(*topology);
    const auto topology2 = Topology::FromJSON(json);
    EXPECT_EQ(json, Json(*topology2));
}
TEST(Topology, SerializeGridToJson) {
    auto topology = Topology::FromYAML(LoadFile("quration-core/tests/data/topology/grid.yaml"));
    std::cout << Json(*topology) << std::endl;
    const auto json = Json(*topology);
    const auto topology2 = Topology::FromJSON(json);
    EXPECT_EQ(json, Json(*topology2));
}
TEST(Topology, SerializeDistributeToJson) {
    auto topology =
            Topology::FromYAML(LoadFile("quration-core/tests/data/topology/distribute.yaml"));
    std::cout << Json(*topology) << std::endl;
    const auto json = Json(*topology);
    const auto topology2 = Topology::FromJSON(json);
    EXPECT_EQ(json, Json(*topology2));
}
TEST(InvalidTopology, NonPositiveSize) {
    Logger::EnableConsoleOutput();

    EXPECT_THROW(
            Topology::FromYAML(LoadFile("quration-core/tests/data/topology/invalid_size.yaml")),
            std::runtime_error
    );
}
TEST(InvalidTopology, ZOverlap) {
    Logger::EnableConsoleOutput();

    EXPECT_THROW(
            Topology::FromYAML(
                    LoadFile("quration-core/tests/data/topology/invalid_z_overlap.yaml")
            ),
            std::runtime_error
    );
}
TEST(InvalidTopology, QSymbolOverlap) {
    Logger::EnableConsoleOutput();

    EXPECT_THROW(
            Topology::FromYAML(
                    LoadFile("quration-core/tests/data/topology/invalid_q_overlap.yaml")
            ),
            std::runtime_error
    );
}
TEST(InvalidTopology, MSymbolOverlap) {
    Logger::EnableConsoleOutput();

    EXPECT_THROW(
            Topology::FromYAML(
                    LoadFile("quration-core/tests/data/topology/invalid_m_overlap.yaml")
            ),
            std::runtime_error
    );
}
TEST(InvalidTopology, ESymbolOverlap) {
    Logger::EnableConsoleOutput();

    EXPECT_THROW(
            Topology::FromYAML(
                    LoadFile("quration-core/tests/data/topology/invalid_e_overlap.yaml")
            ),
            std::runtime_error
    );
}
TEST(InvalidTopology, ESymbolNotPaired) {
    Logger::EnableConsoleOutput();

    EXPECT_THROW(
            Topology::FromYAML(LoadFile("quration-core/tests/data/topology/invalid_e_pair.yaml")),
            std::runtime_error
    );
}
TEST(InvalidTopology, ESymbolPairSameGrid) {
    Logger::EnableConsoleOutput();

    EXPECT_THROW(
            Topology::FromYAML(
                    LoadFile("quration-core/tests/data/topology/invalid_e_pair_same_grid.yaml")
            ),
            std::runtime_error
    );
}
