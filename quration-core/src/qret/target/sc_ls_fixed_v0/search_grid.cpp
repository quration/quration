/**
 * @file qret/target/sc_ls_fixed_v0/search_grid.cpp
 * @brief Search grid in chip.
 */

#include "qret/target/sc_ls_fixed_v0/search_grid.h"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <queue>
#include <set>
#include <stdexcept>
#include <vector>

#include "qret/base/graph.h"
#include "qret/target/sc_ls_fixed_v0/geometry.h"
#include "qret/target/sc_ls_fixed_v0/pauli.h"
#include "qret/target/sc_ls_fixed_v0/state.h"
#include "qret/target/sc_ls_fixed_v0/symbol.h"
#include "qret/target/sc_ls_fixed_v0/topology.h"

namespace qret::sc_ls_fixed_v0 {
namespace {
bool IsMoveCostUnreachable(std::uint64_t cost) {
    return MoveCostAccessor::CostX0(cost) == MoveCostAccessor::Inf
            && MoveCostAccessor::CostX1(cost) == MoveCostAccessor::Inf
            && MoveCostAccessor::CostZ0(cost) == MoveCostAccessor::Inf
            && MoveCostAccessor::CostZ1(cost) == MoveCostAccessor::Inf;
}

std::optional<std::pair<MSymbol, Coord2D>>
FindBestAdjacentMagicFactory(QuantumPlane& plane, QuantumState& state, const Coord2D& place) {
    const auto& topology = plane.GetTopology();
    auto best = std::optional<std::pair<MSymbol, Coord2D>>{};
    auto best_stock = std::uint64_t{0};
    for (const auto& neighbor : GetNeighbors(place)) {
        if (topology.OutOfPlane(neighbor)) {
            continue;
        }

        const auto& node = plane.GetNode(neighbor);
        if (node.type != QuantumStateBufferInternal::Type::MagicFactory) {
            continue;
        }

        const auto msymbol = MSymbol{node.symbol};
        if (!state.IsAvailable(msymbol)) {
            continue;
        }

        const auto stock = state.GetState(msymbol).magic_state_count;
        if (!best.has_value() || stock > best_stock
            || (stock == best_stock && msymbol.Id() < best->first.Id())) {
            best = {msymbol, neighbor};
            best_stock = stock;
        }
    }
    return best;
}

enum class CnotAxis : std::uint8_t {
    X = 0,
    Y = 1,
};

struct CnotState {
    Coord2D place;
    CnotAxis edge_from;
    bool turned;
};

struct CnotSearchData {
    std::int32_t z = 0;
    Coord3D dst;
    const ScLsPlane* topology = nullptr;
    std::vector<std::uint64_t> costs;
    std::vector<std::int64_t> next;
};

constexpr auto CnotStateCountPerCoord = std::size_t{4};
constexpr auto InvalidCnotStateIndex = std::int64_t{-1};
constexpr auto UnreachableCnotCost = std::numeric_limits<std::uint64_t>::max();

CnotAxis AxisOfStep(const Coord2D& src, const Coord2D& dst) {
    const auto diff = dst - src;
    assert(std::abs(diff.x) + std::abs(diff.y) == 1);
    return diff.x == 0 ? CnotAxis::Y : CnotAxis::X;
}

std::vector<std::pair<Coord2D, CnotAxis>> GetBoundaryCells(
        const ScLsPlane& topology,
        const Coord2D& place,
        std::uint32_t dir,
        std::uint32_t boundaries
) {
    auto ret = std::vector<std::pair<Coord2D, CnotAxis>>{};
    const auto append = [&ret, &topology, &place](CnotAxis axis) {
        const auto& neighbors =
                axis == CnotAxis::X ? GetHorizontalNeighbors(place) : GetVerticalNeighbors(place);
        for (const auto& neighbor : neighbors) {
            if (!topology.OutOfPlane(neighbor)) {
                ret.emplace_back(neighbor, axis);
            }
        }
    };

    if ((boundaries == SearchRoute::LS_X_BOUNDARY && dir == 1)
        || (boundaries == SearchRoute::LS_Z_BOUNDARY && dir == 0)
        || boundaries == SearchRoute::LS_ANY_BOUNDARY) {
        append(CnotAxis::X);
    }
    if ((boundaries == SearchRoute::LS_X_BOUNDARY && dir == 0)
        || (boundaries == SearchRoute::LS_Z_BOUNDARY && dir == 1)
        || boundaries == SearchRoute::LS_ANY_BOUNDARY) {
        append(CnotAxis::Y);
    }

    return ret;
}

std::size_t
CnotStateIndex(const ScLsPlane& topology, const Coord2D& place, CnotAxis edge_from, bool turned) {
    const auto coord_index = static_cast<std::size_t>(place.y * topology.GetMaxX() + place.x);
    assert(coord_index < static_cast<std::size_t>(topology.GetMaxX())
                   * static_cast<std::size_t>(topology.GetMaxY()));
    return (coord_index * CnotStateCountPerCoord)
            + (edge_from == CnotAxis::Y ? std::size_t{2} : std::size_t{0})
            + (turned ? std::size_t{1} : std::size_t{0});
}

CnotState DecodeCnotState(const ScLsPlane& topology, std::size_t index) {
    const auto state = index % CnotStateCountPerCoord;
    const auto coord_index = index / CnotStateCountPerCoord;
    const auto x =
            static_cast<std::int32_t>(coord_index % static_cast<std::size_t>(topology.GetMaxX()));
    const auto y =
            static_cast<std::int32_t>(coord_index / static_cast<std::size_t>(topology.GetMaxX()));
    return {
            .place = {x, y},
            .edge_from = state >= 2 ? CnotAxis::Y : CnotAxis::X,
            .turned = (state % 2) == 1,
    };
}

bool IsFreeAncillaForLatency(QuantumPlane& plane_0, QuantumPlane& plane_1, const Coord2D& place) {
    const auto& topology = plane_0.GetTopology();
    if (topology.OutOfPlane(place)) {
        return false;
    }
    return plane_0.GetNode(place).IsFreeAncilla() && plane_1.GetNode(place).IsFreeAncilla();
}

CnotSearchData BuildCnotSearchData(
        QuantumPlane& plane_0,
        QuantumPlane& plane_1,
        QSymbol q_dst,
        std::uint32_t boundaries_dst
) {
    const auto& topology = plane_0.GetTopology();
    auto& state = plane_0.GetParent().GetParent();
    const auto& dst = state.GetPlace(q_dst);
    const auto dir_dst = state.GetDir(q_dst);
    const auto num_states = static_cast<std::size_t>(topology.GetMaxX())
            * static_cast<std::size_t>(topology.GetMaxY()) * CnotStateCountPerCoord;

    auto ret = CnotSearchData{
            .z = topology.GetZ(),
            .dst = dst,
            .topology = &topology,
            .costs = std::vector<std::uint64_t>(num_states, UnreachableCnotCost),
            .next = std::vector<std::int64_t>(num_states, InvalidCnotStateIndex),
    };
    auto queue = std::queue<std::size_t>{};

    for (const auto& [boundary, axis] :
         GetBoundaryCells(topology, dst.XY(), dir_dst, boundaries_dst)) {
        if (!IsFreeAncillaForLatency(plane_0, plane_1, boundary)) {
            continue;
        }
        const auto index = CnotStateIndex(topology, boundary, axis, false);
        if (ret.costs[index] != UnreachableCnotCost) {
            continue;
        }
        ret.costs[index] = 1;
        queue.emplace(index);
    }

    while (!queue.empty()) {
        const auto current_index = queue.front();
        queue.pop();
        const auto current = DecodeCnotState(topology, current_index);
        const auto current_cost = ret.costs[current_index];

        for (const auto& prev_place : GetNeighbors(current.place)) {
            if (!IsFreeAncillaForLatency(plane_0, plane_1, prev_place)) {
                continue;
            }

            const auto prev_axis = AxisOfStep(prev_place, current.place);
            const auto prev_turned = current.turned || (prev_axis != current.edge_from);
            const auto prev_index = CnotStateIndex(topology, prev_place, prev_axis, prev_turned);
            if (ret.costs[prev_index] != UnreachableCnotCost) {
                continue;
            }

            ret.costs[prev_index] = current_cost + 1;
            ret.next[prev_index] = static_cast<std::int64_t>(current_index);
            queue.emplace(prev_index);
        }
    }

    return ret;
}

std::optional<std::pair<std::uint64_t, std::size_t>>
FindBestCnotStateAtPlace(const CnotSearchData& data, const Coord2D& place, CnotAxis source_axis) {
    auto best = std::optional<std::pair<std::uint64_t, std::size_t>>{};
    for (const auto edge_from : {CnotAxis::X, CnotAxis::Y}) {
        for (const auto turned : {false, true}) {
            const auto index = CnotStateIndex(*data.topology, place, edge_from, turned);
            const auto cost = data.costs[index];
            if (cost == UnreachableCnotCost) {
                continue;
            }
            if (!(turned || source_axis != edge_from)) {
                continue;
            }
            if (!best.has_value() || cost < best->first
                || (cost == best->first && index < best->second)) {
                best = {{cost, index}};
            }
        }
    }
    return best;
}

std::optional<std::pair<std::uint64_t, std::size_t>> FindBestCnotStartState(
        QuantumPlane& plane_0,
        QuantumPlane& plane_1,
        const Coord2D& src_place,
        std::uint32_t src_dir,
        std::uint32_t boundaries_src,
        const CnotSearchData& data
) {
    auto best = std::optional<std::pair<std::uint64_t, std::size_t>>{};
    for (const auto& [boundary, source_axis] :
         GetBoundaryCells(*data.topology, src_place, src_dir, boundaries_src)) {
        if (!IsFreeAncillaForLatency(plane_0, plane_1, boundary)) {
            continue;
        }
        const auto candidate = FindBestCnotStateAtPlace(data, boundary, source_axis);
        if (!candidate.has_value()) {
            continue;
        }
        if (!best.has_value() || candidate->first < best->first
            || (candidate->first == best->first && candidate->second < best->second)) {
            best = candidate;
        }
    }
    return best;
}

std::list<Coord3D> TraceBackPathOfCnot(const CnotSearchData& data, std::size_t start_index) {
    auto ret = std::list<Coord3D>{};
    auto current_index = static_cast<std::int64_t>(start_index);
    while (current_index != InvalidCnotStateIndex) {
        const auto current =
                DecodeCnotState(*data.topology, static_cast<std::size_t>(current_index));
        ret.emplace_back(current.place.x, current.place.y, data.z);
        current_index = data.next[static_cast<std::size_t>(current_index)];
    }
    ret.emplace_back(data.dst);
    return ret;
}
}  // namespace

void SearchHelper::SetCostsOfFreeAncillae(QuantumState& state, std::uint64_t cost) {
    for (auto&& grid : state) {
        SetCostsOfFreeAncillae(grid, cost);
    }
}
void SearchHelper::SetCostsOfFreeAncillae(QuantumGrid& grid, std::uint64_t cost) {
    for (auto&& plane : grid) {
        SetCostsOfFreeAncillae(plane, cost);
    }
}
void SearchHelper::SetCostsOfFreeAncillae(QuantumPlane& plane, std::uint64_t cost) {
    for (auto&& node : plane) {
        if (node.IsFreeAncilla()) {
            node.SetCost(cost);
        }
    }
}
std::queue<Coord2D> SearchRoute::InitializeQueueAndPlane2D(
        QuantumPlane& plane,
        const Coord2D& place_src,
        std::uint32_t dir_src,
        std::uint32_t boundaries_src
) {
    // dir=0  dir=1
    // ============
    //   X      Z
    //  ZQZ    XQX
    //   X      Z
    auto queue = std::queue<Coord2D>();
    const auto& topology = plane.GetTopology();
    if ((boundaries_src == LS_X_BOUNDARY && dir_src == 1)
        || (boundaries_src == LS_Z_BOUNDARY && dir_src == 0) || boundaries_src == LS_ANY_BOUNDARY) {
        for (const auto& place : GetHorizontalNeighbors(place_src)) {
            if (!topology.OutOfPlane(place) && plane.GetNode(place).IsFreeAncilla()) {
                queue.emplace(place);
                plane.GetNode(place).SetCost(1);
            }
        }
    }
    if ((boundaries_src == LS_X_BOUNDARY && dir_src == 0)
        || (boundaries_src == LS_Z_BOUNDARY && dir_src == 1) || boundaries_src == LS_ANY_BOUNDARY) {
        for (const auto& place : GetVerticalNeighbors(place_src)) {
            if (!topology.OutOfPlane(place) && plane.GetNode(place).IsFreeAncilla()) {
                queue.emplace(place);
                plane.GetNode(place).SetCost(1);
            }
        }
    }
    return queue;
}
std::queue<Coord2D> SearchRoute::InitializeQueueAndPlane2DM(QuantumPlane& plane) {
    auto queue = std::queue<Coord2D>();
    const auto& topology = plane.GetTopology();
    auto& state = plane.GetParent().GetParent();
    const auto& magic_factory_list = state.GetMagicFactoryList(plane.GetTopology().GetZ());
    for (const auto& magic_factory : magic_factory_list) {
        if (!state.IsAvailable(magic_factory)) {
            continue;
        }
        const auto place_src = state.GetPlace(magic_factory).XY();
        for (const auto& place :
             {place_src + Coord2D::Left(),
              place_src + Coord2D::Right(),
              place_src + Coord2D::Up(),
              place_src + Coord2D::Down()}) {
            if (!topology.OutOfPlane(place) && plane.GetNode(place).IsFreeAncilla()
                && plane.GetNode(place).Cost() == Unreachable2D) {
                queue.emplace(place);
                plane.GetNode(place).SetCost(1);
            }
        }
    }
    return queue;
}
std::set<Coord2D> SearchRoute::InitializeGoal2D(
        QuantumPlane& plane,
        const Coord2D& place_dst,
        std::uint32_t dir_dst,
        std::uint32_t boundaries_dst
) {
    // dir=0  dir=1
    // ============
    //   X      Z
    //  ZQZ    XQX
    //   X      Z
    auto goals = std::set<Coord2D>();
    const auto& topology = plane.GetTopology();
    if ((boundaries_dst == LS_X_BOUNDARY && dir_dst == 1)
        || (boundaries_dst == LS_Z_BOUNDARY && dir_dst == 0) || boundaries_dst == LS_ANY_BOUNDARY) {
        for (const auto& place : GetHorizontalNeighbors(place_dst)) {
            if (!topology.OutOfPlane(place) && plane.GetNode(place).IsFreeAncilla()) {
                goals.emplace(place);
            }
        }
    }
    if ((boundaries_dst == LS_X_BOUNDARY && dir_dst == 0)
        || (boundaries_dst == LS_Z_BOUNDARY && dir_dst == 1) || boundaries_dst == LS_ANY_BOUNDARY) {
        for (const auto& place : GetVerticalNeighbors(place_dst)) {
            if (!topology.OutOfPlane(place) && plane.GetNode(place).IsFreeAncilla()) {
                goals.emplace(place);
            }
        }
    }
    return goals;
}
std::optional<Coord2D> SearchRoute::RunBFS2D(
        QuantumPlane& plane,
        std::queue<Coord2D>& queue,
        const std::set<Coord2D>& goals
) {
    const auto& topology = plane.GetTopology();

    auto reach_goal = false;
    auto goal = Coord2D{};
    while (!queue.empty()) {
        const auto tmp_place = queue.front();
        queue.pop();

        if (goals.contains(tmp_place)) {
            reach_goal = true;
            goal = tmp_place;
            break;
        }

        for (const auto& place : GetNeighbors(tmp_place)) {
            if (!topology.OutOfPlane(place) && plane.GetNode(place).IsFreeAncilla()
                && plane.GetNode(place).Cost() == Unreachable2D) {
                queue.emplace(place);
                auto& node = plane.GetNode(place);
                const auto& tmp_node = plane.GetNode(tmp_place);
                node.SetCost(tmp_node.Cost() + 1);
            }
        }
    }
    if (reach_goal) {
        return goal;
    }
    return std::nullopt;
}
std::optional<SearchRoute::Route2D> SearchRoute::FindRoute2D(
        QuantumPlane& plane,
        QSymbol q_src,
        QSymbol q_dst,
        std::uint32_t boundaries_src,
        std::uint32_t boundaries_dst
) {
    const auto& topology = plane.GetTopology();
    const auto& place_src = plane.GetParent().GetParent().GetPlace(q_src).XY();
    const auto& place_dst = plane.GetParent().GetParent().GetPlace(q_dst).XY();
    const auto dir_src = plane.GetParent().GetParent().GetDir(q_src);
    const auto dir_dst = plane.GetParent().GetParent().GetDir(q_dst);

    // Initialize plane, queue, and goal.
    auto queue = InitializeQueueAndPlane2D(plane, place_src, dir_src, boundaries_src);
    auto goals = InitializeGoal2D(plane, place_dst, dir_dst, boundaries_dst);

    // Unreachable if queue is empty.
    if (queue.empty()) {
        return std::nullopt;
    }

    // Run BFS.
    const auto goal = RunBFS2D(plane, queue, goals);
    if (!goal) {
        return std::nullopt;
    }

    // Traceback
    auto route = Route2D{
            .q_src = q_src,
            .q_dst = q_dst,
            .src = plane.GetParent().GetParent().GetPlace(q_src),
            .logical_path = {},
            .dst = plane.GetParent().GetParent().GetPlace(q_dst),
    };
    // Fill logical_path.
    const auto z = topology.GetZ();
    auto current_place = *goal;
    auto current_cost = plane.GetNode(current_place).Cost();
    route.logical_path.emplace_front(route.dst);
    route.logical_path.emplace_front(current_place.x, current_place.y, z);
    while (current_cost != 1) {
        auto found_prev = false;
        for (const auto& prev_place : GetNeighbors(current_place)) {
            if (topology.OutOfPlane(prev_place)) {
                continue;
            }

            const auto& prev_node = plane.GetNode(prev_place);
            if (!prev_node.IsFreeAncilla()) {
                continue;
            }

            if (prev_node.Cost() + 1 == current_cost) {
                current_place = prev_place;
                current_cost = prev_node.Cost();
                found_prev = true;
                break;
            }
        }
        if (!found_prev) {
            throw std::logic_error("Failed to find previous place.");
        }

        route.logical_path.emplace_front(current_place.x, current_place.y, z);
    }
    route.logical_path.emplace_front(route.src);
    route.Audit();

    return route;
}
std::optional<SearchRoute::Route2D> SearchRoute::FindCnotRoute2D(
        QuantumPlane& plane_0,
        QuantumPlane& plane_1,
        QSymbol q_src,
        QSymbol q_dst,
        std::uint32_t boundaries_src,
        std::uint32_t boundaries_dst
) {
    auto& state = plane_0.GetParent().GetParent();
    const auto& src = state.GetPlace(q_src);
    const auto& dst = state.GetPlace(q_dst);
    const auto dir_src = state.GetDir(q_src);

    if (src.z != dst.z || plane_0.GetTopology().GetZ() != src.z) {
        return std::nullopt;
    }

    const auto search = BuildCnotSearchData(plane_0, plane_1, q_dst, boundaries_dst);
    const auto best =
            FindBestCnotStartState(plane_0, plane_1, src.XY(), dir_src, boundaries_src, search);
    if (!best.has_value()) {
        return std::nullopt;
    }

    auto route = Route2D{
            .q_src = q_src,
            .q_dst = q_dst,
            .src = src,
            .logical_path = TraceBackPathOfCnot(search, best->second),
            .dst = dst,
    };
    route.logical_path.emplace_front(route.src);
    route.Audit();

    return route;
}
std::optional<SearchRoute::Route2DM>
SearchRoute::FindRoute2DM(QuantumPlane& plane, QSymbol q_dst, std::uint32_t boundaries_dst) {
    const auto& topology = plane.GetTopology();
    const auto& place_dst = plane.GetParent().GetParent().GetPlace(q_dst).XY();
    const auto dir_dst = plane.GetParent().GetParent().GetDir(q_dst);

    // Initialize plane, queue, and goal.
    // NOTE: queue seeds are deduplicated in InitializeQueueAndPlane2DM.
    auto queue = InitializeQueueAndPlane2DM(plane);
    auto goals = InitializeGoal2D(plane, place_dst, dir_dst, boundaries_dst);

    // Unreachable if queue is empty.
    if (queue.empty()) {
        return std::nullopt;
    }

    // Run BFS.
    const auto goal = RunBFS2D(plane, queue, goals);
    if (!goal) {
        return std::nullopt;
    }

    // Traceback
    auto route = Route2DM{
            .magic_factory = MSymbol(),
            .q_dst = q_dst,
            .src = Coord3D(),
            .logical_path = {},
            .dst = plane.GetParent().GetParent().GetPlace(q_dst),
    };
    // Fill src and logical_path.
    const auto z = topology.GetZ();
    auto current_place = *goal;
    auto current_cost = plane.GetNode(current_place).Cost();
    route.logical_path.emplace_front(route.dst);
    route.logical_path.emplace_front(current_place.x, current_place.y, z);
    while (current_cost != 1) {
        auto found_prev = false;
        for (const auto& prev_place :
             {current_place + Coord2D::Left(),
              current_place + Coord2D::Right(),
              current_place + Coord2D::Up(),
              current_place + Coord2D::Down()}) {
            if (topology.OutOfPlane(prev_place)) {
                continue;
            }

            const auto& prev_node = plane.GetNode(prev_place);
            if (!prev_node.IsFreeAncilla()) {
                continue;
            }

            if (prev_node.Cost() + 1 == current_cost) {
                current_place = prev_place;
                current_cost = prev_node.Cost();
                found_prev = true;
                break;
            }
        }
        if (!found_prev) {
            throw std::logic_error("Failed to find previous place");
        }

        route.logical_path.emplace_front(current_place.x, current_place.y, z);
    }

    auto& state = plane.GetParent().GetParent();
    const auto best = FindBestAdjacentMagicFactory(plane, state, current_place);
    if (!best.has_value()) {
        throw std::logic_error("No available magic factory at initial place.");
    }
    route.magic_factory = best->first;
    route.src = {best->second.x, best->second.y, z};
    route.logical_path.emplace_front(route.src);

    route.Audit();

    return route;
}
std::optional<SearchRoute::Route2DE> SearchRoute::FindRoute2DE(
        QuantumPlane& plane,
        ESymbol e_factory,
        QSymbol q_dst,
        std::uint32_t boundaries_dst
) {
    const auto& topology = plane.GetTopology();
    const auto& place_src = plane.GetParent().GetParent().GetPlace(e_factory).XY();
    const auto& place_dst = plane.GetParent().GetParent().GetPlace(q_dst).XY();
    const auto dir_dst = plane.GetParent().GetParent().GetDir(q_dst);

    // Initialize plane, queue, and goal.
    auto queue = InitializeQueueAndPlane2D(plane, place_src, 0, LS_ANY_BOUNDARY);
    auto goals = InitializeGoal2D(plane, place_dst, dir_dst, boundaries_dst);

    // Unreachable if queue is empty.
    if (queue.empty()) {
        return std::nullopt;
    }

    // Run BFS.
    const auto goal = RunBFS2D(plane, queue, goals);
    if (!goal) {
        return std::nullopt;
    }

    // Traceback
    auto route = Route2DE{
            .e_factory = e_factory,
            .q_dst = q_dst,
            .src = plane.GetParent().GetParent().GetPlace(e_factory),
            .logical_path = {},
            .dst = plane.GetParent().GetParent().GetPlace(q_dst),
    };
    // Fill logical_path.
    const auto z = topology.GetZ();
    auto current_place = *goal;
    auto current_cost = plane.GetNode(current_place).Cost();
    route.logical_path.emplace_front(route.dst);
    route.logical_path.emplace_front(current_place.x, current_place.y, z);
    while (current_cost != 1) {
        auto found_prev = false;
        for (const auto& prev_place : GetNeighbors(current_place)) {
            if (topology.OutOfPlane(prev_place)) {
                continue;
            }

            const auto& prev_node = plane.GetNode(prev_place);
            if (!prev_node.IsFreeAncilla()) {
                continue;
            }

            if (prev_node.Cost() + 1 == current_cost) {
                current_place = prev_place;
                current_cost = prev_node.Cost();
                found_prev = true;
                break;
            }
        }
        if (!found_prev) {
            throw std::logic_error("Failed to find previous place.");
        }

        route.logical_path.emplace_front(current_place.x, current_place.y, z);
    }
    route.logical_path.emplace_front(route.src);
    route.Audit();

    return route;
}
std::optional<SearchRoute::Ancilla2D> SearchRoute::FindAncilla(
        QuantumPlane& plane,
        const std::list<QSymbol>& qs,
        const std::list<Pauli>& boundaries,
        const std::list<ESymbol>& es,
        bool use_magic_state
) {
    // Fast paths for common small cases.
    if (qs.size() == 2 && es.empty() && !use_magic_state && boundaries.size() == 2) {
        const auto q_src = *qs.begin();
        const auto q_dst = *std::next(qs.begin());
        const auto boundary_src = *boundaries.begin();
        const auto boundary_dst = *std::next(boundaries.begin());
        SearchHelper::SetCostsOfFreeAncillae(plane, Unreachable2D);
        const auto route = FindRoute2D(
                plane,
                q_src,
                q_dst,
                GetBoundaryCode(boundary_src),
                GetBoundaryCode(boundary_dst)
        );
        if (!route) {
            return std::nullopt;
        }
        auto ancilla = route->logical_path;
        ancilla.pop_front();
        ancilla.pop_back();
        return Ancilla2D{.qs = qs, .es = es, .m = std::nullopt, .ancilla = std::move(ancilla)};
    }
    if (qs.size() == 1 && es.empty() && use_magic_state && boundaries.size() == 1) {
        const auto q_dst = *qs.begin();
        const auto boundary_dst = *boundaries.begin();
        SearchHelper::SetCostsOfFreeAncillae(plane, Unreachable2D);
        const auto route = FindRoute2DM(plane, q_dst, GetBoundaryCode(boundary_dst));
        if (!route) {
            return std::nullopt;
        }
        auto ancilla = route->logical_path;
        ancilla.pop_front();
        ancilla.pop_back();
        return Ancilla2D{
                .qs = qs,
                .es = es,
                .m = route->magic_factory,
                .ancilla = std::move(ancilla),
        };
    }
    if (qs.size() == 1 && es.size() == 1 && !use_magic_state && boundaries.size() == 1) {
        const auto q_dst = *qs.begin();
        const auto e_factory = *es.begin();
        const auto boundary_dst = *boundaries.begin();
        SearchHelper::SetCostsOfFreeAncillae(plane, Unreachable2D);
        const auto route = FindRoute2DE(plane, e_factory, q_dst, GetBoundaryCode(boundary_dst));
        if (!route) {
            return std::nullopt;
        }
        auto ancilla = route->logical_path;
        ancilla.pop_front();
        ancilla.pop_back();
        return Ancilla2D{.qs = qs, .es = es, .m = std::nullopt, .ancilla = std::move(ancilla)};
    }

    const auto& topology = plane.GetTopology();
    auto& state = plane.GetParent().GetParent();

    static constexpr auto MId = -1;
    const auto ind_width = topology.GetMaxY();
    if (ind_width <= 0) {
        throw std::logic_error("max_y must be positive");
    }
    // Use a fixed row-major encoding to keep graph IDs stable across runs.
    const auto to_ind = [ind_width](std::int32_t x, std::int32_t y) -> std::int32_t {
        return (x * ind_width) + y;
    };
    const auto from_ind = [ind_width](std::int32_t ind) -> Coord2D {
        return {ind / ind_width, ind % ind_width};
    };

    // Create graph.
    auto graph = Graph{};
    auto terminals = std::vector<std::int32_t>{};
    auto queue = std::queue<Coord2D>{};
    const auto add_or_get_node = [&graph](std::int32_t id) -> const Graph::Node& {
        return graph.HasNode(id) ? graph.GetNode(id) : graph.AddNode(id);
    };

    const auto add_to_node_if_free_ancilla = [&plane, &topology, to_ind, &graph, &queue](
                                                     const Coord2D& place,
                                                     const Graph::Node& parent
                                             ) {
        if (!topology.OutOfPlane(place) && plane.GetNode(place).IsFreeAncilla()) {
            const auto id = to_ind(place.x, place.y);
            const auto already_added = graph.HasNode(id);
            const auto& neighbor_node = already_added ? graph.GetNode(id) : graph.AddNode(id);
            graph.AddEdge(parent.id, neighbor_node.id, 1);
            if (!already_added) {
                queue.emplace(place);
            }
        }
    };

    if (use_magic_state) {
        const auto& imag_m_node = graph.AddNode(MId);
        terminals.emplace_back(MId);

        auto found_magic_state = false;
        const auto& magic_factory_list = state.GetMagicFactoryList(plane.GetTopology().GetZ());
        for (const auto& magic_factory : magic_factory_list) {
            if (!state.IsAvailable(magic_factory)) {
                continue;
            }

            found_magic_state = true;
            const auto m_place = state.GetPlace(magic_factory).XY();
            const auto& m_node = add_or_get_node(to_ind(m_place.x, m_place.y));
            // Keep virtual-magic edges expensive so Steiner still optimizes physical ancilla
            // segments.
            graph.AddEdge(imag_m_node.id, m_node.id, 1000);

            for (const auto& place : GetNeighbors(m_place)) {
                add_to_node_if_free_ancilla(place, m_node);
            }
        }
        if (!found_magic_state) {
            return std::nullopt;
        }
    }
    {
        auto q_itr = qs.begin();
        auto b_itr = boundaries.begin();
        while (q_itr != qs.end() && b_itr != boundaries.end()) {
            const auto q = *q_itr;
            const auto b = *b_itr;
            const auto q_place = state.GetPlace(q).XY();
            const auto dir = state.GetDir(q);

            const auto& q_node = add_or_get_node(to_ind(q_place.x, q_place.y));
            terminals.emplace_back(q_node.id);

            if (b.IsAny() || (b.IsX() && dir == 1) || (b.IsZ() && dir == 0)) {
                for (const auto& place : GetHorizontalNeighbors(q_place)) {
                    add_to_node_if_free_ancilla(place, q_node);
                }
            }
            if (b.IsAny() || (b.IsX() && dir == 0) || (b.IsZ() && dir == 1)) {
                for (const auto& place : GetVerticalNeighbors(q_place)) {
                    add_to_node_if_free_ancilla(place, q_node);
                }
            }
            q_itr++;
            b_itr++;
        }
    }
    for (const auto e_factory : es) {
        if (!state.GetState(e_factory).IsAvailable()) {
            return std::nullopt;
        }

        const auto e_place = state.GetPlace(e_factory).XY();

        const auto& e_node = add_or_get_node(to_ind(e_place.x, e_place.y));
        terminals.emplace_back(e_node.id);

        for (const auto place : GetNeighbors(e_place)) {
            add_to_node_if_free_ancilla(place, e_node);
        }
    }

    // BFS to construct graph.
    while (!queue.empty()) {
        const auto src_place = queue.front();
        queue.pop();
        const auto& node = graph.GetNode(to_ind(src_place.x, src_place.y));
        for (const auto& place : GetNeighbors(src_place)) {
            add_to_node_if_free_ancilla(place, node);
        }
    }

    std::sort(terminals.begin(), terminals.end());
    terminals.erase(std::unique(terminals.begin(), terminals.end()), terminals.end());
    if (terminals.empty()) {
        return std::nullopt;
    }

    // Search minimum steiner tree.
    const auto result = MinimumSteinerTreeSolver(graph).SolveByWu(terminals);
    if (!result) {
        return std::nullopt;
    }
    const auto& [cost, tree] = result.value();

    // Construct ancilla.
    auto ret = Ancilla2D{.qs = qs, .es = es, .m = std::nullopt, .ancilla = {}};
    if (use_magic_state) {
        const auto& m_imag_node = tree.GetNode(MId);
        if (m_imag_node.adj.size() != 1) {
            throw std::logic_error(
                    "The number of neighbors of imaginary magic factory node must 1."
            );
        }
        const auto neighbor_place = from_ind(*m_imag_node.adj.begin());
        ret.m = MSymbol{plane.GetNode(neighbor_place).symbol};
    }
    for (const auto& node : tree) {
        if (node.id == MId) {
            continue;
        }
        const auto place = from_ind(node.id);
        if (plane.GetNode(place).IsFreeAncilla()) {
            ret.ancilla.emplace_back(place.x, place.y, plane.GetTopology().GetZ());
        }
    }

    return ret;
}
bool SearchRoute::LS(QuantumPlane& plane, QSymbol q, std::uint32_t boundaries) {
    static constexpr auto Unreachable = LSCostAccessor::Inf;

    const auto& topology = plane.GetTopology();
    const auto& initial_place = plane.GetParent().GetParent().GetPlace(q).XY();
    const auto initial_dir = plane.GetParent().GetParent().GetDir(q);

    assert(boundaries == LS_X_BOUNDARY || boundaries == LS_Z_BOUNDARY
           || boundaries == LS_ANY_BOUNDARY);
    assert(topology.GetZ() == plane.GetParent().GetParent().GetPlace(q).z);

    auto queue = std::queue<Coord2D>();

    // Initialize queue and plane.
    // dir=0  dir=1
    // ============
    //   X      Z
    //  ZQZ    XQX
    //   X      Z
    if ((boundaries == LS_X_BOUNDARY && initial_dir == 1)
        || (boundaries == LS_Z_BOUNDARY && initial_dir == 0) || boundaries == LS_ANY_BOUNDARY) {
        for (const auto& place : GetHorizontalNeighbors(initial_place)) {
            if (!topology.OutOfPlane(place) && plane.GetNode(place).IsFreeAncilla()) {
                queue.emplace(place);
                auto cost = plane.GetNode(place).Cost();
                LSCostAccessor::SetCostX(cost, 1);
                plane.GetNode(place).SetCost(cost);
            }
        }
    }
    if ((boundaries == LS_X_BOUNDARY && initial_dir == 0)
        || (boundaries == LS_Z_BOUNDARY && initial_dir == 1) || boundaries == LS_ANY_BOUNDARY) {
        for (const auto& place : GetVerticalNeighbors(initial_place)) {
            if (!topology.OutOfPlane(place) && plane.GetNode(place).IsFreeAncilla()) {
                queue.emplace(place);
                auto cost = plane.GetNode(place).Cost();
                LSCostAccessor::SetCostY(cost, 1);
                plane.GetNode(place).SetCost(cost);
            }
        }
    }

    if (queue.empty()) {
        return false;
    }

    // Run BFS.
    while (!queue.empty()) {
        const auto tmp_place = queue.front();
        queue.pop();
        const auto& tmp_node = plane.GetNode(tmp_place);
        const auto tmp_cost = std::min(
                LSCostAccessor::CostX(tmp_node.Cost()),
                LSCostAccessor::CostY(tmp_node.Cost())
        );

        for (const auto& place : GetHorizontalNeighbors(tmp_place)) {
            if (!topology.OutOfPlane(place) && plane.GetNode(place).IsFreeAncilla()
                && LSCostAccessor::CostX(plane.GetNode(place).Cost()) == Unreachable) {
                queue.emplace(place);
                auto& node = plane.GetNode(place);
                auto cost = node.Cost();
                LSCostAccessor::SetCostX(cost, tmp_cost + 1);
                node.SetCost(cost);
            }
        }
        for (const auto& place : GetVerticalNeighbors(tmp_place)) {
            if (!topology.OutOfPlane(place) && plane.GetNode(place).IsFreeAncilla()
                && LSCostAccessor::CostY(plane.GetNode(place).Cost()) == Unreachable) {
                queue.emplace(place);
                auto& node = plane.GetNode(place);
                auto cost = node.Cost();
                LSCostAccessor::SetCostY(cost, tmp_cost + 1);
                node.SetCost(cost);
            }
        }
    }

    return true;
}
std::list<Coord3D> SearchRoute::TraceBackPathOfLS(
        QuantumPlane& plane,
        QSymbol q,
        char edge_from,
        const Coord2D& place
) {
    const auto& topology = plane.GetTopology();
    const auto z = topology.GetZ();
    const auto& initial_place = plane.GetParent().GetParent().GetPlace(q);

    assert(plane.GetNode(place).IsFreeAncilla());

    auto ret = std::list<Coord3D>();
    auto current_place = place;
    auto current_edge_from = edge_from;
    auto current_cost = LSCostAccessor::Cost(plane.GetNode(current_place).Cost(), edge_from);
    ret.emplace_back(current_place.x, current_place.y, z);
    while (current_cost != 1) {
        auto found_prev = false;
        for (const auto& prev_place : current_edge_from == 'X'
                     ? GetHorizontalNeighbors(current_place)
                     : GetVerticalNeighbors(current_place)) {
            if (topology.OutOfPlane(prev_place)) {
                continue;
            }

            const auto& prev_node = plane.GetNode(prev_place);
            if (!prev_node.IsFreeAncilla()) {
                continue;
            }

            for (const auto prev_edge_from : {'X', 'Y'}) {
                const auto prev_cost = LSCostAccessor::Cost(prev_node.Cost(), prev_edge_from);
                if (prev_cost + 1 == current_cost) {
                    current_place = prev_place;
                    current_edge_from = prev_edge_from;
                    current_cost = prev_cost;
                    found_prev = true;
                    break;
                }
            }
            if (found_prev) {
                break;
            }
        }
        if (!found_prev) {
            throw std::logic_error("Failed to find previous place");
        }

        ret.emplace_back(current_place.x, current_place.y, z);
    }
    ret.emplace_back(initial_place.x, initial_place.y, z);

    return ret;
}
bool SearchRoute::Move(QuantumPlane& plane, QSymbol q) {
    static constexpr auto Unreachable = MoveCostAccessor::Inf;

    const auto& topology = plane.GetTopology();
    const auto& initial_place = plane.GetParent().GetParent().GetPlace(q).XY();
    const auto initial_dir = plane.GetParent().GetParent().GetDir(q);

    // Start searching from the XY of q.
    // Z coordinate may be differrent.
    // assert(topology.GetZ() == plane.GetParent().GetParent().GetPlace(q).z);
    if (auto& initial_node = plane.GetNode(initial_place);
        plane.GetNode(initial_place).IsFreeAncilla()) {
        auto cost = initial_node.Cost();
        if (initial_dir == 0) {
            MoveCostAccessor::SetCostZ0(cost, 0);
            MoveCostAccessor::SetCostX0(cost, 0);
        } else {
            MoveCostAccessor::SetCostX1(cost, 0);
            MoveCostAccessor::SetCostZ1(cost, 0);
        }
        initial_node.SetCost(cost);
    }

    auto queue = std::queue<Coord2D>();

    // Initialize queue and plane.
    // dir=0  dir=1
    // ============
    //   X      Z
    //  ZQZ    XQX
    //   X      Z
    for (const auto& place : GetHorizontalNeighbors(initial_place)) {
        if (!topology.OutOfPlane(place) && plane.GetNode(place).IsFreeAncilla()) {
            queue.emplace(place);
            auto& node = plane.GetNode(place);
            auto cost = node.Cost();
            if (initial_dir == 0) {
                MoveCostAccessor::SetCostZ0(cost, 1);
            } else {
                MoveCostAccessor::SetCostX1(cost, 1);
            }
            node.SetCost(cost);
        }
    }
    for (const auto& place : GetVerticalNeighbors(initial_place)) {
        if (!topology.OutOfPlane(place) && plane.GetNode(place).IsFreeAncilla()) {
            queue.emplace(place);
            auto& node = plane.GetNode(place);
            auto cost = node.Cost();
            if (initial_dir == 0) {
                MoveCostAccessor::SetCostX0(cost, 1);
            } else {
                MoveCostAccessor::SetCostZ1(cost, 1);
            }
            node.SetCost(cost);
        }
    }

    if (queue.empty()) {
        return false;
    }

    // Run BFS.
    while (!queue.empty()) {
        const auto tmp_place = queue.front();
        const auto& tmp_node = plane.GetNode(tmp_place);
        const auto tmp_raw_cost = tmp_node.Cost();
        queue.pop();

        for (const auto pauli : {Pauli::X(), Pauli::Z()}) {
            const auto tmp_cost_dir0 = MoveCostAccessor::Cost(tmp_raw_cost, pauli, 0);
            const auto tmp_cost_dir1 = MoveCostAccessor::Cost(tmp_raw_cost, pauli, 1);
            const auto tmp_cost = std::min(tmp_cost_dir0, tmp_cost_dir1);
            if (tmp_cost == Unreachable) {
                continue;
            }

            // Search from state (pauli, tmp_dir).
            // next cost is tmp_cost + 1.

            // Search if destination's dir is 0:
            for (const auto place : pauli == Pauli::X() ? GetVerticalNeighbors(tmp_place)
                                                        : GetHorizontalNeighbors(tmp_place)) {
                if (!topology.OutOfPlane(place) && plane.GetNode(place).IsFreeAncilla()
                    && MoveCostAccessor::Cost(plane.GetNode(place).Cost(), pauli, 0)
                            == Unreachable) {
                    queue.emplace(place);
                    auto& node = plane.GetNode(place);
                    auto cost = node.Cost();
                    MoveCostAccessor::SetCost(cost, tmp_cost + 1, pauli, 0);
                    node.SetCost(cost);
                }
            }

            // Search if destination's dir is 1:
            for (const auto place : pauli == Pauli::X() ? GetHorizontalNeighbors(tmp_place)
                                                        : GetVerticalNeighbors(tmp_place)) {
                if (!topology.OutOfPlane(place) && plane.GetNode(place).IsFreeAncilla()
                    && MoveCostAccessor::Cost(plane.GetNode(place).Cost(), pauli, 1)
                            == Unreachable) {
                    queue.emplace(place);
                    auto& node = plane.GetNode(place);
                    auto cost = node.Cost();
                    MoveCostAccessor::SetCost(cost, tmp_cost + 1, pauli, 1);
                    node.SetCost(cost);
                }
            }
        }
    }

    return true;
}
std::list<Coord3D> SearchRoute::TraceBackPathOfMove(
        QuantumPlane& plane,
        QSymbol q,
        Pauli pauli,
        std::uint32_t moved_state_dir,
        const Coord2D& place
) {
    const auto& topology = plane.GetTopology();
    const auto z = topology.GetZ();
    const auto& initial_place = plane.GetParent().GetParent().GetPlace(q);
    // const auto initial_dir = plane.GetParent().GetParent().GetDir(q);

    assert(plane.GetNode(place).IsFreeAncilla());

    auto ret = std::list<Coord3D>();
    auto current_place = place;
    auto current_dir = moved_state_dir;
    auto current_cost =
            MoveCostAccessor::Cost(plane.GetNode(current_place).Cost(), pauli, current_dir);
    if (current_place == initial_place.XY() && current_cost == 0) {
        ret.emplace_back(initial_place.x, initial_place.y, z);
        return ret;
    }
    ret.emplace_back(current_place.x, current_place.y, z);
    while (current_cost != 1) {
        auto found_prev = false;
        // Mirror forward expansion: predecessor candidates depend on (pauli, current_dir).
        for (const auto& prev_place : ((pauli == Pauli::X() && current_dir == 1)
                                       || (pauli == Pauli::Z() && current_dir == 0))
                     ? GetHorizontalNeighbors(current_place)
                     : GetVerticalNeighbors(current_place)) {
            if (topology.OutOfPlane(prev_place)) {
                continue;
            }

            const auto& prev_node = plane.GetNode(prev_place);
            if (!prev_node.IsFreeAncilla()) {
                continue;
            }

            for (const auto prev_dir : {std::uint32_t{0}, std::uint32_t{1}}) {
                const auto prev_cost = MoveCostAccessor::Cost(prev_node.Cost(), pauli, prev_dir);
                if (prev_cost + 1 == current_cost) {
                    current_place = prev_place;
                    current_dir = prev_dir;
                    current_cost = prev_cost;
                    found_prev = true;
                    break;
                }
            }
            if (found_prev) {
                break;
            }
        }
        if (!found_prev) {
            throw std::logic_error("Failed to find previous place");
        }

        ret.emplace_back(current_place.x, current_place.y, z);
    }
    ret.emplace_back(initial_place.x, initial_place.y, z);

    return ret;
}
bool SearchRoute::MoveMagic(QuantumPlane& plane) {
    static constexpr auto Unreachable = std::uint64_t{0xFFFF'FFFFULL};

    const auto& topology = plane.GetTopology();
    auto& state = plane.GetParent().GetParent();
    const auto& magic_factory_list = state.GetMagicFactoryList(plane.GetTopology().GetZ());
    if (magic_factory_list.empty()) {
        return false;
    }

    auto queue = std::queue<Coord2D>();

    // Initialize queue and plane.
    for (const auto& magic_factory : magic_factory_list) {
        if (!state.IsAvailable(magic_factory)) {
            continue;
        }
        const auto initial_place = state.GetPlace(magic_factory).XY();
        for (const auto& place : GetNeighbors(initial_place)) {
            if (!topology.OutOfPlane(place) && plane.GetNode(place).IsFreeAncilla()) {
                auto& node = plane.GetNode(place);
                auto cost = node.Cost();
                if (!IsMoveCostUnreachable(cost)) {
                    continue;
                }
                queue.emplace(place);
                MoveCostAccessor::SetCostZ0(cost, 1);
                MoveCostAccessor::SetCostZ1(cost, 1);
                MoveCostAccessor::SetCostX0(cost, 1);
                MoveCostAccessor::SetCostX1(cost, 1);
                node.SetCost(cost);
            }
        }
    }

    if (queue.empty()) {
        return false;
    }

    // Run BFS.
    while (!queue.empty()) {
        const auto tmp_place = queue.front();
        const auto& tmp_node = plane.GetNode(tmp_place);
        const auto tmp_raw_cost = tmp_node.Cost();
        queue.pop();

        for (const auto pauli : {Pauli::X(), Pauli::Z()}) {
            const auto tmp_cost_dir0 = MoveCostAccessor::Cost(tmp_raw_cost, pauli, 0);
            const auto tmp_cost_dir1 = MoveCostAccessor::Cost(tmp_raw_cost, pauli, 1);
            const auto tmp_cost = std::min(tmp_cost_dir0, tmp_cost_dir1);
            if (tmp_cost == Unreachable) {
                continue;
            }

            // Search from state (pauli, tmp_dir).
            // next cost is tmp_cost + 1.

            // Search if destination's dir is 0:
            for (const auto place : pauli == Pauli::X() ? GetVerticalNeighbors(tmp_place)
                                                        : GetHorizontalNeighbors(tmp_place)) {
                if (!topology.OutOfPlane(place) && plane.GetNode(place).IsFreeAncilla()
                    && MoveCostAccessor::Cost(plane.GetNode(place).Cost(), pauli, 0)
                            == Unreachable) {
                    queue.emplace(place);
                    auto& node = plane.GetNode(place);
                    auto cost = node.Cost();
                    MoveCostAccessor::SetCost(cost, tmp_cost + 1, pauli, 0);
                    node.SetCost(cost);
                }
            }

            // Search if destination's dir is 1:
            for (const auto place : pauli == Pauli::X() ? GetHorizontalNeighbors(tmp_place)
                                                        : GetVerticalNeighbors(tmp_place)) {
                if (!topology.OutOfPlane(place) && plane.GetNode(place).IsFreeAncilla()
                    && MoveCostAccessor::Cost(plane.GetNode(place).Cost(), pauli, 1)
                            == Unreachable) {
                    queue.emplace(place);
                    auto& node = plane.GetNode(place);
                    auto cost = node.Cost();
                    MoveCostAccessor::SetCost(cost, tmp_cost + 1, pauli, 1);
                    node.SetCost(cost);
                }
            }
        }
    }

    return true;
}
std::pair<MSymbol, std::list<Coord3D>> SearchRoute::TraceBackPathOfMoveMagic(
        QuantumPlane& plane,
        Pauli pauli,
        std::uint32_t moved_state_dir,
        const Coord2D& place
) {
    const auto& topology = plane.GetTopology();
    const auto z = topology.GetZ();

    assert(plane.GetNode(place).IsFreeAncilla());

    auto ret = std::list<Coord3D>();
    auto current_place = place;
    auto current_dir = moved_state_dir;
    auto current_cost =
            MoveCostAccessor::Cost(plane.GetNode(current_place).Cost(), pauli, current_dir);
    ret.emplace_back(current_place.x, current_place.y, z);
    while (current_cost != 1) {
        auto found_prev = false;
        for (const auto& prev_place : ((pauli == Pauli::X() && current_dir == 1)
                                       || (pauli == Pauli::Z() && current_dir == 0))
                     ? GetHorizontalNeighbors(current_place)
                     : GetVerticalNeighbors(current_place)) {
            if (topology.OutOfPlane(prev_place)) {
                continue;
            }

            const auto& prev_node = plane.GetNode(prev_place);
            if (!prev_node.IsFreeAncilla()) {
                continue;
            }

            for (const auto prev_dir : {std::uint32_t{0}, std::uint32_t{1}}) {
                const auto prev_cost = MoveCostAccessor::Cost(prev_node.Cost(), pauli, prev_dir);
                if (prev_cost + 1 == current_cost) {
                    current_place = prev_place;
                    current_dir = prev_dir;
                    current_cost = prev_cost;
                    found_prev = true;
                    break;
                }
            }
            if (found_prev) {
                break;
            }
        }

        if (!found_prev) {
            throw std::logic_error("Failed to find previous place");
        }
        ret.emplace_back(current_place.x, current_place.y, z);
    }

    auto& state = plane.GetParent().GetParent();
    const auto best = FindBestAdjacentMagicFactory(plane, state, current_place);
    if (!best.has_value()) {
        throw std::logic_error("No available magic factory at initial place.");
    }
    ret.emplace_back(best->second.x, best->second.y, z);

    return {best->first, ret};
}
std::optional<SearchRoute::Route3D> SearchRoute::FindRoute3D(
        QuantumGrid& grid_0,
        QuantumGrid& grid_1,
        QuantumPlane& plane_2,
        QSymbol q_src,
        QSymbol q_dst,
        std::uint32_t boundaries_src
) {
    const auto& grid_topology = grid_0.GetTopology();

    assert(grid_0.GetTopology().GetIndex() == grid_1.GetTopology().GetIndex());
    assert(grid_0.GetTopology().GetIndex() == plane_2.GetParent().GetTopology().GetIndex());
    assert(grid_0.GetParent().GetBeat() + 1 == grid_1.GetParent().GetBeat());
    assert(grid_0.GetParent().GetBeat() + 2 == plane_2.GetParent().GetParent().GetBeat());

    const auto& initial_place = grid_0.GetParent().GetPlace(q_src);
    assert(grid_0.GetTopology().GetMinZ() <= initial_place.z
           && initial_place.z < grid_0.GetTopology().GetMaxZ());

    // Search beat 0.
    auto found_place_to_move = false;
    for (auto z = grid_topology.GetMinZ(); z < grid_topology.GetMaxZ(); ++z) {
        auto& target_plane = grid_0.GetPlane(z);

        // If z is reachable, search 'target_plane'.
        if (!TransReachable(grid_0, initial_place, z)) {
            continue;
        }
        found_place_to_move = true;

        // Search 'target_plane'.
        SearchRoute::Move(target_plane, q_src);
    }
    if (!found_place_to_move) {
        return std::nullopt;
    }

    // Search trans move place.
    const auto tmp_search_result =
            FindTransMovePlace(grid_0, grid_1, plane_2, initial_place.z, boundaries_src);
    if (!tmp_search_result) {
        return std::nullopt;
    }
    const auto& search_result = *tmp_search_result;

    // Construct route.
    auto route = Route3D();
    route.q_src = q_src;
    route.q_dst = q_dst;
    route.src = grid_0.GetParent().GetPlace(q_src);
    LOG_DEBUG(
            "[SearchRoute::FindRoute3D] trans move result: z={} place=({},{}), pauli={}, "
            "moved_state_dir={}, edge_from={}",
            search_result.z,
            search_result.place.x,
            search_result.place.y,
            search_result.pauli.ToChar(),
            search_result.moved_state_dir,
            search_result.edge_from
    );
    try {
        route.move_path = SearchRoute::TraceBackPathOfMove(
                grid_0.GetPlane(search_result.z),
                q_src,
                search_result.pauli,
                search_result.moved_state_dir,
                search_result.place
        );
    } catch (const std::logic_error& err) {
        LOG_ERROR(
                "[SearchRoute::FindRoute3D] TraceBackPathOfMove failed: {} -> {}, err={}",
                q_src,
                q_dst,
                err.what()
        );
        throw;
    }
    std::reverse(route.move_path.begin(), route.move_path.end());
    try {
        route.logical_path = SearchRoute::TraceBackPathOfLS(
                plane_2,
                q_dst,
                search_result.edge_from,
                search_result.place
        );
    } catch (const std::logic_error& err) {
        LOG_ERROR(
                "[SearchRoute::FindRoute3D] TraceBackPathOfLS failed: {} edge_from={} at ({},{})",
                q_dst,
                search_result.edge_from,
                search_result.place.x,
                search_result.place.y
        );
        throw;
    }
    route.dst = plane_2.GetParent().GetParent().GetPlace(q_dst);
    route.Audit();

    return std::move(route);
}
std::optional<SearchRoute::Route3D> SearchRoute::FindCnotRoute3D(
        QuantumGrid& grid_0,
        QuantumGrid& grid_1,
        QuantumPlane& plane_2,
        QuantumPlane& plane_3,
        QSymbol q_src,
        QSymbol q_dst,
        std::uint32_t boundaries_src,
        std::uint32_t boundaries_dst
) {
    struct SearchResult {
        std::uint64_t min_cost = std::numeric_limits<std::uint64_t>::max();
        Pauli pauli;
        std::uint32_t moved_state_dir = 0;
        std::int32_t z = 0;
        Coord2D place;
        std::size_t cnot_state_index = 0;
    };

    const auto& grid_topology = grid_0.GetTopology();
    const auto dst_z = plane_2.GetTopology().GetZ();

    assert(grid_0.GetTopology().GetIndex() == grid_1.GetTopology().GetIndex());
    assert(grid_0.GetTopology().GetIndex() == plane_2.GetParent().GetTopology().GetIndex());
    assert(grid_0.GetTopology().GetIndex() == plane_3.GetParent().GetTopology().GetIndex());
    assert(grid_0.GetParent().GetBeat() + 1 == grid_1.GetParent().GetBeat());
    assert(grid_0.GetParent().GetBeat() + 2 == plane_2.GetParent().GetParent().GetBeat());
    assert(grid_0.GetParent().GetBeat() + 3 == plane_3.GetParent().GetParent().GetBeat());

    const auto& initial_place = grid_0.GetParent().GetPlace(q_src);
    assert(grid_0.GetTopology().GetMinZ() <= initial_place.z
           && initial_place.z < grid_0.GetTopology().GetMaxZ());

    auto found_place_to_move = false;
    for (auto z = grid_topology.GetMinZ(); z < grid_topology.GetMaxZ(); ++z) {
        if (!TransReachable(grid_0, initial_place, z)) {
            continue;
        }
        found_place_to_move = true;
        SearchRoute::Move(grid_0.GetPlane(z), q_src);
    }
    if (!found_place_to_move) {
        return std::nullopt;
    }

    const auto cnot_search = BuildCnotSearchData(plane_2, plane_3, q_dst, boundaries_dst);
    auto search_result = SearchResult{};
    for (auto z = grid_topology.GetMinZ(); z < grid_topology.GetMaxZ(); ++z) {
        const auto trans_move_cost = std::abs(z - initial_place.z) + std::abs(dst_z - z);
        for (auto x = 0; x < grid_topology.GetMaxX(); ++x) {
            for (auto y = 0; y < grid_topology.GetMaxY(); ++y) {
                const auto& node_0 = grid_0.GetNode({x, y, z});
                if (!node_0.IsFreeAncilla() || IsMoveCostUnreachable(node_0.Cost())) {
                    continue;
                }
                if (!TransReachable(grid_1, {x, y, z}, dst_z)) {
                    continue;
                }

                const auto place = Coord2D{x, y};
                for (const auto moved_state_dir : {std::uint32_t{0}, std::uint32_t{1}}) {
                    const auto cnot_candidate = FindBestCnotStartState(
                            plane_2,
                            plane_3,
                            place,
                            moved_state_dir,
                            boundaries_src,
                            cnot_search
                    );
                    if (!cnot_candidate.has_value()) {
                        continue;
                    }

                    for (const auto pauli : {Pauli::X(), Pauli::Z()}) {
                        const auto move_cost =
                                MoveCostAccessor::Cost(node_0.Cost(), pauli, moved_state_dir);
                        if (move_cost == MoveCostAccessor::Inf) {
                            continue;
                        }

                        const auto total_cost = CalculateRoute3DCost(
                                move_cost,
                                cnot_candidate->first,
                                trans_move_cost
                        );
                        if (total_cost < search_result.min_cost) {
                            search_result = {
                                    .min_cost = total_cost,
                                    .pauli = pauli,
                                    .moved_state_dir = moved_state_dir,
                                    .z = z,
                                    .place = place,
                                    .cnot_state_index = cnot_candidate->second,
                            };
                        }
                    }
                }
            }
        }
    }

    if (search_result.min_cost == std::numeric_limits<std::uint64_t>::max()) {
        return std::nullopt;
    }

    auto route = Route3D{
            .q_src = q_src,
            .q_dst = q_dst,
            .src = grid_0.GetParent().GetPlace(q_src),
            .move_path = {},
            .logical_path = {},
            .dst = {},
    };
    route.move_path = SearchRoute::TraceBackPathOfMove(
            grid_0.GetPlane(search_result.z),
            q_src,
            search_result.pauli,
            search_result.moved_state_dir,
            search_result.place
    );
    std::reverse(route.move_path.begin(), route.move_path.end());
    route.logical_path = TraceBackPathOfCnot(cnot_search, search_result.cnot_state_index);
    route.logical_path.emplace_front(search_result.place.x, search_result.place.y, dst_z);
    route.dst = plane_2.GetParent().GetParent().GetPlace(q_dst);
    route.Audit();

    return route;
}
std::optional<SearchRoute::Route3DM> SearchRoute::FindRoute3DM(
        QuantumGrid& grid_0,
        QuantumGrid& grid_1,
        QuantumPlane& plane_2,
        QSymbol q_dst
) {
    const auto& grid_topology = grid_0.GetTopology();

    assert(grid_0.GetTopology().GetIndex() == grid_1.GetTopology().GetIndex());
    assert(grid_0.GetTopology().GetIndex() == plane_2.GetParent().GetTopology().GetIndex());
    assert(grid_0.GetParent().GetBeat() + 1 == grid_1.GetParent().GetBeat());
    assert(grid_0.GetParent().GetBeat() + 2 == plane_2.GetParent().GetParent().GetBeat());

    // Search beat 0.
    auto found_place_to_move_magic = false;
    for (auto z = grid_topology.GetMinZ(); z < grid_topology.GetMaxZ(); ++z) {
        auto& target_plane = grid_0.GetPlane(z);

        // Search 'target_plane'.
        if (SearchRoute::MoveMagic(target_plane)) {
            found_place_to_move_magic = true;
        }
    }
    if (!found_place_to_move_magic) {
        return std::nullopt;
    }

    // Search trans move place.
    const auto tmp_search_result =
            FindTransMovePlace(grid_0, grid_1, plane_2, AnySrcZ, LS_ANY_BOUNDARY);
    if (!tmp_search_result) {
        return std::nullopt;
    }
    const auto& search_result = *tmp_search_result;

    // Construct route.
    auto route = Route3DM();
    std::tie(route.magic_factory, route.move_path) = SearchRoute::TraceBackPathOfMoveMagic(
            grid_0.GetPlane(search_result.z),
            search_result.pauli,
            search_result.moved_state_dir,
            search_result.place
    );
    route.q_dst = q_dst;
    route.moved_state_dir = search_result.moved_state_dir;
    route.src = grid_0.GetParent().GetPlace(route.magic_factory);
    std::reverse(route.move_path.begin(), route.move_path.end());
    route.logical_path = SearchRoute::TraceBackPathOfLS(
            plane_2,
            q_dst,
            search_result.edge_from,
            search_result.place
    );
    route.dst = plane_2.GetParent().GetParent().GetPlace(q_dst);
    route.Audit();

    return std::move(route);
}
bool SearchRoute::TransReachable(QuantumGrid& grid, const Coord3D& src, std::int32_t dst_z) {
    const auto [s, d] = std::minmax(src.z, dst_z);
    for (auto z = s; z <= d; ++z) {
        if (z == src.z) {
            continue;
        }
        const auto& node = grid.GetNode({src.x, src.y, z});
        if (!node.IsFreeAncilla()) {
            return false;
        }
    }
    return true;
}
std::optional<SearchRoute::SearchResult> SearchRoute::FindTransMovePlace(
        QuantumGrid& grid_0,
        QuantumGrid& grid_1,
        QuantumPlane& plane_2,
        const std::int32_t src_z,
        std::uint32_t boundaries_src
) {
    auto search_result = SearchResult{};

    const auto update_search_result = [boundaries_src, &search_result](
                                              const std::uint64_t move_cost,
                                              const std::uint64_t ls_cost,
                                              const std::int32_t trans_move_cost,
                                              const std::int32_t z,
                                              const Coord2D& place
                                      ) {
        if (boundaries_src == LS_ANY_BOUNDARY) {
            for (const auto pauli : {Pauli::X(), Pauli::Z()}) {
                for (const auto moved_state_dir : {std::uint32_t{0}, std::uint32_t{1}}) {
                    const auto move = MoveCostAccessor::Cost(move_cost, pauli, moved_state_dir);
                    if (const auto min_cost = IsMinCostUpdated(
                                search_result.min_cost,
                                move,
                                LSCostAccessor::CostX(ls_cost),
                                trans_move_cost
                        );
                        min_cost) {
                        search_result = {*min_cost, pauli, moved_state_dir, 'X', z, place};
                    }
                    if (const auto min_cost = IsMinCostUpdated(
                                search_result.min_cost,
                                move,
                                LSCostAccessor::CostY(ls_cost),
                                trans_move_cost
                        );
                        min_cost) {
                        search_result = {*min_cost, pauli, moved_state_dir, 'Y', z, place};
                    }
                }
            }
            return;
        }

        // dir=0  dir=1
        // ============
        //   X      Z
        //  ZQZ    XQX
        //   X      Z

        // Logical operation with X boundary of moved qubit.
        if (boundaries_src == LS_X_BOUNDARY) {
            for (const auto pauli : {Pauli::X(), Pauli::Z()}) {
                // edge from X-axis
                const auto mo_x = MoveCostAccessor::Cost(move_cost, pauli, 1);
                const auto ls_x = LSCostAccessor::CostX(ls_cost);
                if (const auto min_cost =
                            IsMinCostUpdated(search_result.min_cost, mo_x, ls_x, trans_move_cost);
                    min_cost) {
                    search_result = {*min_cost, pauli, 1, 'X', z, place};
                }
                // edge from Y-axis
                const auto mo_y = MoveCostAccessor::Cost(move_cost, pauli, 0);
                const auto ls_y = LSCostAccessor::CostY(ls_cost);
                if (const auto min_cost =
                            IsMinCostUpdated(search_result.min_cost, mo_y, ls_y, trans_move_cost);
                    min_cost) {
                    search_result = {*min_cost, pauli, 0, 'Y', z, place};
                }
            }
            return;
        }

        // assert(boundaries_src == LS_Z_BOUNDARY);
        for (const auto pauli : {Pauli::X(), Pauli::Z()}) {
            // edge from X-axis
            const auto mo_x = MoveCostAccessor::Cost(move_cost, pauli, 0);
            const auto ls_x = LSCostAccessor::CostX(ls_cost);
            if (const auto min_cost =
                        IsMinCostUpdated(search_result.min_cost, mo_x, ls_x, trans_move_cost);
                min_cost) {
                search_result = {*min_cost, pauli, 0, 'X', z, place};
            }
            // edge from Y-axis
            const auto mo_y = MoveCostAccessor::Cost(move_cost, pauli, 1);
            const auto ls_y = LSCostAccessor::CostY(ls_cost);
            if (const auto min_cost =
                        IsMinCostUpdated(search_result.min_cost, mo_y, ls_y, trans_move_cost);
                min_cost) {
                search_result = {*min_cost, pauli, 1, 'Y', z, place};
            }
        }
    };

    // Search beats 0,1,2.
    const auto& grid_topology = grid_0.GetTopology();
    const auto dst_z = plane_2.GetTopology().GetZ();
    for (auto z = grid_topology.GetMinZ(); z < grid_topology.GetMaxZ(); ++z) {
        const auto trans_move_cost = src_z == AnySrcZ
                ? std::abs(dst_z - z)  // In case of move magic
                : std::abs(z - src_z) + std::abs(dst_z - z);  // Otherwise
        for (auto x = 0; x < grid_topology.GetMaxX(); ++x) {
            for (auto y = 0; y < grid_topology.GetMaxY(); ++y) {
                const auto& node_0 = grid_0.GetNode({x, y, z});
                const auto& node_2 = plane_2.GetNode({x, y});

                // Is {x,y,z} Reachable at beat 0?
                if (!node_0.IsFreeAncilla()
                    || node_0.Cost() == std::numeric_limits<std::uint64_t>::max()) {
                    continue;
                }

                // Is {x,y} Reachable at beat 2?
                if (!node_2.IsFreeAncilla()
                    || node_2.Cost() == std::numeric_limits<std::uint64_t>::max()) {
                    continue;
                }

                // Can trans move at beat 1?
                // (x, y, z) -> (x, y, dst_z)
                if (!TransReachable(grid_1, {x, y, z}, dst_z)) {
                    continue;
                }

                // Update search result if minimum cost path is found.
                update_search_result(node_0.Cost(), node_2.Cost(), trans_move_cost, z, {x, y});
            }
        }
    }

    // Do not find route.
    if (search_result.min_cost == std::numeric_limits<std::uint64_t>::max()) {
        return std::nullopt;
    }

    return search_result;
}
}  // namespace qret::sc_ls_fixed_v0
