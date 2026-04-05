/**
 * @file qret/target/sc_ls_fixed_v0/simulator_runnability.h
 * @brief Helper functions to validate simulator instruction runnability.
 */

#ifndef QRET_TARGET_SC_LS_FIXED_V0_SIMULATOR_RUNNABILITY_H
#define QRET_TARGET_SC_LS_FIXED_V0_SIMULATOR_RUNNABILITY_H

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <list>
#include <queue>
#include <stdexcept>
#include <tuple>
#include <unordered_set>
#include <vector>

#include "qret/target/sc_ls_fixed_v0/simulator.h"

namespace qret::sc_ls_fixed_v0::detail {
inline bool IsAdjacent(const Coord3D& lhs, const Coord3D& rhs) {
    if (lhs.z != rhs.z) {
        return false;
    }
    const auto dx = std::abs(lhs.x - rhs.x);
    const auto dy = std::abs(lhs.y - rhs.y);
    return (dx + dy) == 1;
}

inline bool
IsConnectedPathBetween(const Coord3D& src, const std::list<Coord3D>& path, const Coord3D& dst) {
    auto prev = src;
    if (path.empty()) {
        return IsAdjacent(src, dst);
    }
    for (const auto& p : path) {
        if (!IsAdjacent(prev, p)) {
            return false;
        }
        prev = p;
    }
    return IsAdjacent(prev, dst);
}

inline bool HasOnlyBoundaryPauli(const std::list<Pauli>& basis_list) {
    return std::all_of(basis_list.begin(), basis_list.end(), [](Pauli pauli) {
        return pauli.IsX() || pauli.IsZ() || pauli.IsAny();
    });
}

inline std::uint32_t BoundaryCodeFromPauli(Pauli pauli) {
    if (pauli.IsX()) {
        return SearchRoute::LS_X_BOUNDARY;
    }
    if (pauli.IsZ()) {
        return SearchRoute::LS_Z_BOUNDARY;
    }
    if (pauli.IsAny()) {
        return SearchRoute::LS_ANY_BOUNDARY;
    }
    throw std::logic_error("Unsupported boundary pauli.");
}

inline std::vector<Coord3D>
GetBoundaryCells(const Coord3D& place, std::uint32_t dir, std::uint32_t boundaries) {
    auto ret = std::vector<Coord3D>{};
    const auto append = [&ret, &place](bool horizontal) {
        const auto neighbors =
                horizontal ? GetHorizontalNeighbors(place.XY()) : GetVerticalNeighbors(place.XY());
        for (const auto& neighbor : neighbors) {
            ret.emplace_back(neighbor.x, neighbor.y, place.z);
        }
    };

    if ((boundaries == SearchRoute::LS_X_BOUNDARY && dir == 1)
        || (boundaries == SearchRoute::LS_Z_BOUNDARY && dir == 0)
        || boundaries == SearchRoute::LS_ANY_BOUNDARY) {
        append(true);
    }
    if ((boundaries == SearchRoute::LS_X_BOUNDARY && dir == 0)
        || (boundaries == SearchRoute::LS_Z_BOUNDARY && dir == 1)
        || boundaries == SearchRoute::LS_ANY_BOUNDARY) {
        append(false);
    }

    return ret;
}

inline bool IsConnectedPathSet(const std::list<Coord3D>& path) {
    if (path.empty()) {
        return false;
    }

    auto remaining = std::unordered_set<Coord3D>(path.begin(), path.end());
    auto queue = std::queue<Coord3D>{};
    const auto start = *remaining.begin();
    remaining.erase(start);
    queue.emplace(start);

    while (!queue.empty()) {
        const auto current = queue.front();
        queue.pop();
        for (const auto& neighbor :
             {current + Coord2D::Left(),
              current + Coord2D::Right(),
              current + Coord2D::Up(),
              current + Coord2D::Down()}) {
            const auto itr = remaining.find(neighbor);
            if (itr == remaining.end()) {
                continue;
            }
            queue.emplace(*itr);
            remaining.erase(itr);
        }
    }

    return remaining.empty();
}

inline bool
PathTouchesEndpoint(const std::unordered_set<Coord3D>& path_set, const Coord3D& endpoint) {
    for (const auto& neighbor :
         {endpoint + Coord2D::Left(),
          endpoint + Coord2D::Right(),
          endpoint + Coord2D::Up(),
          endpoint + Coord2D::Down()}) {
        if (path_set.contains(neighbor)) {
            return true;
        }
    }
    return false;
}

inline bool PathTouchesBoundary(
        const std::unordered_set<Coord3D>& path_set,
        const Coord3D& place,
        std::uint32_t dir,
        std::uint32_t boundaries
) {
    for (const auto& boundary_cell : GetBoundaryCells(place, dir, boundaries)) {
        if (path_set.contains(boundary_cell)) {
            return true;
        }
    }
    return false;
}

inline bool IsValidLsPath(
        const std::list<Coord3D>& path,
        const std::vector<std::tuple<Coord3D, std::uint32_t, std::uint32_t>>& qubit_boundaries,
        const std::vector<Coord3D>& endpoints
) {
    if (path.empty()) {
        if (!endpoints.empty()) {
            return false;
        }

        auto anchors = std::list<Coord3D>{};
        for (const auto& [place, _, __] : qubit_boundaries) {
            anchors.emplace_back(place);
        }
        if (!IsConnectedPathSet(anchors)) {
            return false;
        }
        for (const auto& [place, dir, boundaries] : qubit_boundaries) {
            auto touches_boundary = false;
            for (const auto& boundary_cell : GetBoundaryCells(place, dir, boundaries)) {
                if (std::find(anchors.begin(), anchors.end(), boundary_cell) != anchors.end()) {
                    touches_boundary = true;
                    break;
                }
            }
            if (!touches_boundary) {
                return false;
            }
        }
        return true;
    }

    if (!IsConnectedPathSet(path)) {
        return false;
    }

    const auto path_set = std::unordered_set<Coord3D>(path.begin(), path.end());
    for (const auto& [place, dir, boundaries] : qubit_boundaries) {
        if (!PathTouchesBoundary(path_set, place, dir, boundaries)) {
            return false;
        }
    }
    for (const auto& endpoint : endpoints) {
        if (!PathTouchesEndpoint(path_set, endpoint)) {
            return false;
        }
    }
    return true;
}

inline bool QubitIsAvailable(const MinimumAvailableBeat& avail, Beat beat, QSymbol qubit) {
    const auto itr = avail.q.find(qubit);
    assert(itr != avail.q.end() && "qubit is not allocated");
    return itr != avail.q.end() && itr->second <= beat;
}

inline bool
MagicFactoryIsAvailable(const MinimumAvailableBeat& avail, Beat beat, MSymbol magic_factory) {
    const auto itr = avail.m.find(magic_factory);
    assert(itr != avail.m.end() && "magic_factory is not allocated");
    return itr != avail.m.end() && itr->second <= beat;
}

inline bool EntanglementFactoryIsAvailable(
        const MinimumAvailableBeat& avail,
        Beat beat,
        ESymbol entanglement_factory
) {
    const auto itr = avail.e.find(entanglement_factory);
    assert(itr != avail.e.end() && "entanglement_factory is not allocated");
    return itr != avail.e.end() && itr->second <= beat;
}

inline bool
QubitsAreAvailable(const MinimumAvailableBeat& avail, Beat beat, const std::list<QSymbol>& qubits) {
    return std::all_of(qubits.begin(), qubits.end(), [&avail, beat](QSymbol qubit) {
        return QubitIsAvailable(avail, beat, qubit);
    });
}

inline bool IsLatticeSurgeryRunnable(
        QuantumStateBuffer& buffer,
        const MinimumAvailableBeat& avail,
        Beat beat,
        const LatticeSurgery& inst
) {
    const auto& qubits = inst.QubitList();
    if (qubits.size() <= 1) {
        return true;
    }

    auto& state = buffer.GetQuantumState(beat);
    const auto z = state.GetPlace(qubits.front()).z;
    const auto& topology = state.GetGrid(z).GetPlane(z).GetTopology();

    if (!QubitsAreAvailable(avail, beat, qubits)) {
        return false;
    }
    for (auto b = beat; b < beat + inst.Latency(); ++b) {
        auto& tmp_state = buffer.GetQuantumState(b);
        auto& plane = tmp_state.GetGrid(z).GetPlane(z);

        for (const auto& q : qubits) {
            const auto& place = tmp_state.GetPlace(q);
            if (z != place.z) {
                return false;
            }
            if (!plane.GetNode(place.XY()).is_available) {
                return false;
            }
        }
        for (const auto& coord : inst.Path()) {
            if (z != coord.z) {
                return false;
            }
            if (topology.OutOfPlane(coord.XY())) {
                return false;
            }
            if (!plane.GetNode(coord.XY()).IsFreeAncilla()) {
                return false;
            }
        }
    }
    if (!HasOnlyBoundaryPauli(inst.BasisList())) {
        return false;
    }

    auto qubit_boundaries = std::vector<std::tuple<Coord3D, std::uint32_t, std::uint32_t>>{};
    auto q_itr = qubits.begin();
    auto basis_itr = inst.BasisList().begin();
    while (q_itr != qubits.end() && basis_itr != inst.BasisList().end()) {
        qubit_boundaries.emplace_back(
                state.GetPlace(*q_itr),
                state.GetDir(*q_itr),
                BoundaryCodeFromPauli(*basis_itr)
        );
        q_itr++;
        basis_itr++;
    }
    return IsValidLsPath(inst.Path(), qubit_boundaries, {});
}

inline bool IsLatticeSurgeryMagicRunnable(
        QuantumStateBuffer& buffer,
        const MinimumAvailableBeat& avail,
        Beat beat,
        const LatticeSurgeryMagic& inst
) {
    const auto& qubits = inst.QubitList();
    const auto magic_factory = inst.MagicFactory();
    if (!QubitsAreAvailable(avail, beat, qubits)) {
        return false;
    }
    if (!MagicFactoryIsAvailable(avail, beat, magic_factory)) {
        return false;
    }

    auto& state = buffer.GetQuantumState(beat);
    const auto& place = state.GetPlace(qubits.front());
    const auto& magic_factory_place = state.GetPlace(magic_factory);
    auto& plane = state.GetGrid(place.z).GetPlane(place.z);
    const auto& topology = plane.GetTopology();

    if (place.z != magic_factory_place.z) {
        return false;
    }
    if (plane.GetNode(magic_factory_place.XY()).is_available) {
        return false;
    }
    if (!state.IsAvailable(magic_factory)) {
        return false;
    }
    for (auto b = beat; b < beat + inst.Latency(); ++b) {
        auto& tmp_state = buffer.GetQuantumState(b);
        auto& tmp_plane = tmp_state.GetGrid(place.z).GetPlane(place.z);
        for (const auto& [x, y, z] : inst.Path()) {
            if (place.z != z) {
                return false;
            }
            if (topology.OutOfPlane({x, y})) {
                return false;
            }
            if (!tmp_plane.GetNode({x, y}).IsFreeAncilla()) {
                return false;
            }
        }
        for (const auto& q : qubits) {
            const auto& tmp_place = tmp_state.GetPlace(q);
            if (!tmp_plane.GetNode(tmp_place.XY()).is_available) {
                return false;
            }
        }
    }
    if (!HasOnlyBoundaryPauli(inst.BasisList())) {
        return false;
    }

    auto qubit_boundaries = std::vector<std::tuple<Coord3D, std::uint32_t, std::uint32_t>>{};
    auto q_itr = qubits.begin();
    auto basis_itr = inst.BasisList().begin();
    while (q_itr != qubits.end() && basis_itr != inst.BasisList().end()) {
        qubit_boundaries.emplace_back(
                state.GetPlace(*q_itr),
                state.GetDir(*q_itr),
                BoundaryCodeFromPauli(*basis_itr)
        );
        q_itr++;
        basis_itr++;
    }
    return IsValidLsPath(inst.Path(), qubit_boundaries, {magic_factory_place});
}

inline bool IsLatticeSurgeryMultinodeRunnable(
        QuantumStateBuffer& buffer,
        const MinimumAvailableBeat& avail,
        Beat beat,
        const LatticeSurgeryMultinode& inst
) {
    const auto& qubits = inst.QubitList();
    auto& state = buffer.GetQuantumState(beat);
    const auto can_use_entanglement = [&state](ESymbol e, EHandle h) {
        const auto pair = state.GetTopology().GetPair(e);
        const auto& e_state = state.GetState(e);
        if (e_state.IsReserved(h)) {
            return true;
        }
        return e_state.IsAvailable() && state.GetState(pair).IsAvailable();
    };

    if (!QubitsAreAvailable(avail, beat, qubits)) {
        return false;
    }
    auto e_itr = inst.EFactoryList().begin();
    auto h_itr = inst.EHandleList().begin();
    while (e_itr != inst.EFactoryList().end() && h_itr != inst.EHandleList().end()) {
        if (!EntanglementFactoryIsAvailable(avail, beat, *e_itr)
            || !can_use_entanglement(*e_itr, *h_itr)) {
            return false;
        }
        e_itr++;
        h_itr++;
    }
    if (inst.UseMagicFactory() && !MagicFactoryIsAvailable(avail, beat, inst.MagicFactory())) {
        return false;
    }

    const auto& place = qubits.empty() ? state.GetPlace(inst.EFactoryList().front())
                                       : state.GetPlace(qubits.front());
    auto& plane = state.GetGrid(place.z).GetPlane(place.z);
    const auto& topology = plane.GetTopology();

    if (inst.UseMagicFactory()) {
        const auto magic_factory = inst.MagicFactory();
        const auto& magic_factory_place = state.GetPlace(magic_factory);
        if (place.z != magic_factory_place.z) {
            return false;
        }
        if (plane.GetNode(magic_factory_place.XY()).is_available) {
            return false;
        }
        if (!state.IsAvailable(magic_factory)) {
            return false;
        }
    }
    for (const auto e : inst.EFactoryList()) {
        const auto& e_place = state.GetPlace(e);
        if (place.z != e_place.z) {
            return false;
        }
        if (plane.GetNode(e_place.XY()).is_available) {
            return false;
        }
        if (!state.IsAvailable(e)) {
            return false;
        }
    }
    for (auto b = beat; b < beat + inst.Latency(); ++b) {
        auto& tmp_state = buffer.GetQuantumState(b);
        auto& tmp_plane = tmp_state.GetGrid(place.z).GetPlane(place.z);
        for (const auto& [x, y, z] : inst.Path()) {
            if (place.z != z) {
                return false;
            }
            if (topology.OutOfPlane({x, y})) {
                return false;
            }
            if (!tmp_plane.GetNode({x, y}).IsFreeAncilla()) {
                return false;
            }
        }
        for (const auto& q : qubits) {
            const auto& tmp_place = tmp_state.GetPlace(q);
            if (!tmp_plane.GetNode(tmp_place.XY()).is_available) {
                return false;
            }
        }
    }
    if (!HasOnlyBoundaryPauli(inst.BasisList())) {
        return false;
    }

    auto qubit_boundaries = std::vector<std::tuple<Coord3D, std::uint32_t, std::uint32_t>>{};
    auto q_itr = qubits.begin();
    auto basis_itr = inst.BasisList().begin();
    while (q_itr != qubits.end() && basis_itr != inst.BasisList().end()) {
        qubit_boundaries.emplace_back(
                state.GetPlace(*q_itr),
                state.GetDir(*q_itr),
                BoundaryCodeFromPauli(*basis_itr)
        );
        q_itr++;
        basis_itr++;
    }

    auto endpoints = std::vector<Coord3D>{};
    for (const auto e : inst.EFactoryList()) {
        endpoints.emplace_back(state.GetPlace(e));
    }
    if (inst.UseMagicFactory()) {
        endpoints.emplace_back(state.GetPlace(inst.MagicFactory()));
    }
    return IsValidLsPath(inst.Path(), qubit_boundaries, endpoints);
}

inline bool IsMoveEntanglementRunnable(
        QuantumStateBuffer& buffer,
        const MinimumAvailableBeat& avail,
        Beat beat,
        const MoveEntanglement& inst
) {
    const auto& qubit = inst.QDest();
    const auto e_factory = inst.EFactory();
    const auto e_handle = inst.GetEHandle();
    if (!QubitIsAvailable(avail, beat, qubit)) {
        return false;
    }
    if (!EntanglementFactoryIsAvailable(avail, beat, e_factory)) {
        return false;
    }

    auto& state = buffer.GetQuantumState(beat);
    const auto& place = state.GetPlace(qubit);
    const auto& e_factory_place = state.GetPlace(e_factory);
    auto& plane = state.GetGrid(place.z).GetPlane(place.z);
    const auto& topology = plane.GetTopology();

    if (place.z != e_factory_place.z) {
        return false;
    }
    if (plane.GetNode(e_factory_place.XY()).is_available) {
        return false;
    }
    const auto pair_factory = state.GetTopology().GetPair(e_factory);
    const auto& e_state = state.GetState(e_factory);
    const auto can_use_entanglement = e_state.IsReserved(e_handle)
            || (e_state.IsAvailable() && state.GetState(pair_factory).IsAvailable());
    if (!can_use_entanglement) {
        return false;
    }
    for (auto b = beat; b < beat + inst.Latency(); ++b) {
        auto& tmp_state = buffer.GetQuantumState(b);
        auto& tmp_plane = tmp_state.GetGrid(place.z).GetPlane(place.z);
        for (const auto& [x, y, z] : inst.Path()) {
            if (place.z != z) {
                return false;
            }
            if (topology.OutOfPlane({x, y})) {
                return false;
            }
            if (!tmp_plane.GetNode({x, y}).IsFreeAncilla()) {
                return false;
            }
        }
        const auto& tmp_place = tmp_state.GetPlace(qubit);
        if (!tmp_plane.GetNode(tmp_place.XY()).is_available) {
            return false;
        }
    }

    return IsConnectedPathBetween(
            state.GetPlace(inst.EFactory()),
            inst.Path(),
            state.GetPlace(qubit)
    );
}
}  // namespace qret::sc_ls_fixed_v0::detail

#endif  // QRET_TARGET_SC_LS_FIXED_V0_SIMULATOR_RUNNABILITY_H
