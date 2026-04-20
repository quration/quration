#include "qret/target/sc_ls_fixed_v0/search_grid.h"

#include <gtest/gtest.h>

#include <queue>
#include <set>
#include <unordered_set>
#include <utility>

#include "qret/base/string.h"
#include "qret/target/sc_ls_fixed_v0/state.h"
#include "qret/target/sc_ls_fixed_v0/topology.h"

using namespace qret;
using namespace qret::sc_ls_fixed_v0;

namespace {
bool PathTurns(const std::list<Coord3D>& path) {
    if (path.size() < 3) {
        return false;
    }

    auto prev = path.begin();
    auto curr = std::next(prev);
    auto next = std::next(curr);
    auto prev_axis = (prev->x != curr->x) ? 'X' : 'Y';
    for (; next != path.end(); ++prev, ++curr, ++next) {
        const auto axis = (curr->x != next->x) ? 'X' : 'Y';
        if (axis != prev_axis) {
            return true;
        }
        prev_axis = axis;
    }
    return false;
}
}  // namespace

TEST(SearchRoute, FindCnotRoute2DProducesBentRoute) {
    const auto topology =
            Topology::FromYAML(LoadFile("quration-core/tests/data/topology/plane.yaml"));
    const auto option = ScLsFixedV0MachineOption{};
    auto buffer = QuantumStateBuffer::New(*topology, option, 4);

    buffer->PutQubit(0, QSymbol{0}, Coord3D{1, 1, 3}, 0);
    buffer->PutQubit(0, QSymbol{1}, Coord3D{4, 3, 3}, 0);

    auto& plane_0 = buffer->GetQuantumState(0).GetGrid(3).GetPlane(3);
    auto& plane_1 = buffer->GetQuantumState(1).GetGrid(3).GetPlane(3);
    const auto route = SearchRoute::FindCnotRoute2D(
            plane_0,
            plane_1,
            QSymbol{0},
            QSymbol{1},
            SearchRoute::LS_Z_BOUNDARY,
            SearchRoute::LS_X_BOUNDARY
    );

    ASSERT_TRUE(route.has_value());
    EXPECT_EQ(route->src, Coord3D(1, 1, 3));
    EXPECT_EQ(route->dst, Coord3D(4, 3, 3));
    EXPECT_TRUE(PathTurns(route->logical_path));
}

TEST(SearchRoute, FindCnotRoute2DUsesTwoBeatAvailability) {
    const auto topology =
            Topology::FromYAML(LoadFile("quration-core/tests/data/topology/plane.yaml"));
    const auto option = ScLsFixedV0MachineOption{};
    auto buffer = QuantumStateBuffer::New(*topology, option, 4);

    buffer->PutQubit(0, QSymbol{0}, Coord3D{1, 1, 3}, 0);
    buffer->PutQubit(0, QSymbol{1}, Coord3D{4, 3, 3}, 0);

    auto& plane_0 = buffer->GetQuantumState(0).GetGrid(3).GetPlane(3);
    auto& plane_1 = buffer->GetQuantumState(1).GetGrid(3).GetPlane(3);
    const auto allowed = std::set<std::pair<std::int32_t, std::int32_t>>{
            {1, 1},
            {4, 3},
            {2, 1},
            {3, 1},
            {4, 1},
            {4, 2},
    };
    for (auto x = 0; x < plane_0.GetTopology().GetMaxX(); ++x) {
        for (auto y = 0; y < plane_0.GetTopology().GetMaxY(); ++y) {
            if (allowed.contains({x, y})) {
                continue;
            }
            plane_0.GetNode({x, y}).is_available = false;
            plane_1.GetNode({x, y}).is_available = false;
        }
    }

    const auto route = SearchRoute::FindCnotRoute2D(
            plane_0,
            plane_1,
            QSymbol{0},
            QSymbol{1},
            SearchRoute::LS_Z_BOUNDARY,
            SearchRoute::LS_X_BOUNDARY
    );
    ASSERT_TRUE(route.has_value());
    EXPECT_EQ(
            route->logical_path,
            std::list<Coord3D>({
                    Coord3D{1, 1, 3},
                    Coord3D{2, 1, 3},
                    Coord3D{3, 1, 3},
                    Coord3D{4, 1, 3},
                    Coord3D{4, 2, 3},
                    Coord3D{4, 3, 3},
            })
    );

    plane_1.GetNode({4, 1}).is_available = false;
    const auto blocked = SearchRoute::FindCnotRoute2D(
            plane_0,
            plane_1,
            QSymbol{0},
            QSymbol{1},
            SearchRoute::LS_Z_BOUNDARY,
            SearchRoute::LS_X_BOUNDARY
    );
    EXPECT_FALSE(blocked.has_value());
}

TEST(SearchRoute, FindCnotRoute2DRejectsCrossPlaneEndpoints) {
    const auto topology =
            Topology::FromYAML(LoadFile("quration-core/tests/data/topology/grid.yaml"));
    const auto option = ScLsFixedV0MachineOption{};
    auto buffer = QuantumStateBuffer::New(*topology, option, 4);

    buffer->PutQubit(0, QSymbol{0}, Coord3D{1, 1, 2}, 0);
    buffer->PutQubit(0, QSymbol{1}, Coord3D{4, 3, 4}, 0);

    auto& plane_0 = buffer->GetQuantumState(0).GetGrid(2).GetPlane(2);
    auto& plane_1 = buffer->GetQuantumState(1).GetGrid(2).GetPlane(2);
    const auto route = SearchRoute::FindCnotRoute2D(
            plane_0,
            plane_1,
            QSymbol{0},
            QSymbol{1},
            SearchRoute::LS_Z_BOUNDARY,
            SearchRoute::LS_X_BOUNDARY
    );

    EXPECT_FALSE(route.has_value());
}

// Returns true if all cells in `path` form a single connected component.
bool IsAncillaPathConnected(const std::list<Coord3D>& path) {
    if (path.empty()) {
        return false;
    }
    auto remaining = std::unordered_set<Coord3D>(path.begin(), path.end());
    auto queue = std::queue<Coord3D>{};
    auto start = *remaining.begin();
    remaining.erase(start);
    queue.push(start);
    while (!queue.empty()) {
        const auto cur = queue.front();
        queue.pop();
        for (const auto& nb :
             {cur + Coord2D::Left(),
              cur + Coord2D::Right(),
              cur + Coord2D::Up(),
              cur + Coord2D::Down()}) {
            const auto it = remaining.find(nb);
            if (it != remaining.end()) {
                queue.push(*it);
                remaining.erase(it);
            }
        }
    }
    return remaining.empty();
}

TEST(SearchRoute, FindAncillaProducesConnectedSetWhenQubitIsBridge) {
    const auto topology =
            Topology::FromYAML(LoadFile("quration-core/tests/data/topology/plane.yaml"));
    const auto option = ScLsFixedV0MachineOption{};
    auto buffer = QuantumStateBuffer::New(*topology, option, 1);

    buffer->PutQubit(0, QSymbol{0}, Coord3D{8, 8, 3}, 0);
    buffer->PutQubit(0, QSymbol{1}, Coord3D{8, 6, 3}, 0);
    buffer->PutQubit(0, QSymbol{2}, Coord3D{8, 4, 3}, 0);

    auto& plane = buffer->GetQuantumState(0).GetGrid(3).GetPlane(3);

    const auto result = SearchRoute::FindAncilla(
            plane,
            {QSymbol{0}, QSymbol{1}, QSymbol{2}},
            {Pauli::X(), Pauli::X(), Pauli::X()},
            {},
            false
    );

    ASSERT_TRUE(result.has_value());

    EXPECT_TRUE(IsAncillaPathConnected(result->ancilla));

    // No qubit cell must appear in the ancilla list.
    const auto qubit_places = std::vector<Coord3D>{
            Coord3D{8, 8, 3},
            Coord3D{8, 6, 3},
            Coord3D{8, 4, 3},
    };
    for (const auto& coord : result->ancilla) {
        for (const auto& q_place : qubit_places) {
            EXPECT_NE(coord, q_place)
                    << "qubit cell " << q_place << " appeared in the ancilla path";
        }
    }
}
