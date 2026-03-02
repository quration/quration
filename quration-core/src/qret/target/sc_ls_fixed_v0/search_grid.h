/**
 * @file qret/target/sc_ls_fixed_v0/search_grid.h
 * @brief Search grid in chip.
 */

#ifndef QRET_TARGET_SC_LS_FIXED_V0_SEARCH_GRID_H
#define QRET_TARGET_SC_LS_FIXED_V0_SEARCH_GRID_H

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <limits>
#include <optional>
#include <ostream>
#include <queue>
#include <set>

#include "qret/target/sc_ls_fixed_v0/pauli.h"
#include "qret/target/sc_ls_fixed_v0/state.h"
#include "qret/target/sc_ls_fixed_v0/symbol.h"
#include "qret/target/sc_ls_fixed_v0/topology.h"

namespace qret::sc_ls_fixed_v0 {
struct SearchHelper {
    static void SetCostsOfFreeAncillae(QuantumState& state, std::uint64_t cost);
    static void SetCostsOfFreeAncillae(QuantumGrid& grid, std::uint64_t cost);
    static void SetCostsOfFreeAncillae(QuantumPlane& grid, std::uint64_t cost);
};

/**
 * @brief Accessor to 'cost' during MOVE instruction search.
 *
 * @details
 * 64-bit unsigned integer 'cost' is split into 4 regions of 16 bits each:
 * @verbatim
 * +-----------------------+-----------------------+-----------------------+-----------------------+
 * |        costX0         |        costX1         |        costZ0         |        costZ1         |
 * | 16 bits (bits 63-48)  | 16 bits (bits 47-32)  | 16 bits (bits 31-16)  | 16 bits (bits 15-0)   |
 * +-----------------------+-----------------------+-----------------------+-----------------------+
 * @endverbatim
 *
 * Explanation:
 * - costX0: Cost of path starting from the X boundary with the current qubit state 'dir = 0'.
 * - costX1: Cost of path starting from the X boundary with the current qubit state 'dir = 1'.
 * - costZ0: Cost of path starting from the Z boundary with the current qubit state 'dir = 0'.
 * - costZ1: Cost of path starting from the Z boundary with the current qubit state 'dir = 1'.
 *
 * Example: If cost = 0x0123456789ABCDEF, then:
 *   - costX0 = 0x0123
 *   - costX1 = 0x4567
 *   - costZ0 = 0x89AB
 *   - costZ1 = 0xCDEF
 */
struct MoveCostAccessor {
    static constexpr std::uint64_t MaskX0 = 0xFFFF'0000'0000'0000ULL;
    static constexpr std::uint64_t MaskX1 = 0x0000'FFFF'0000'0000ULL;
    static constexpr std::uint64_t MaskZ0 = 0x0000'0000'FFFF'0000ULL;
    static constexpr std::uint64_t MaskZ1 = 0x0000'0000'0000'FFFFULL;
    static constexpr std::uint64_t Inf = 0x0000'0000'0000'FFFFULL;

    static constexpr std::uint64_t Cost(std::uint64_t cost, Pauli pauli, std::uint32_t dir) {
        if (pauli == Pauli::X() && dir == 0) {
            return CostX0(cost);
        } else if (pauli == Pauli::X() && dir == 1) {
            return CostX1(cost);
        } else if (pauli == Pauli::Z() && dir == 0) {
            return CostZ0(cost);
        } else if (pauli == Pauli::Z() && dir == 1) {
            return CostZ1(cost);
        }
        assert(0 && "unreachable");
        return 0;
    }
    static constexpr std::uint64_t CostX0(std::uint64_t cost) {
        return (cost & MaskX0) >> 48;
    }
    static constexpr std::uint64_t CostX1(std::uint64_t cost) {
        return (cost & MaskX1) >> 32;
    }
    static constexpr std::uint64_t CostZ0(std::uint64_t cost) {
        return (cost & MaskZ0) >> 16;
    }
    static constexpr std::uint64_t CostZ1(std::uint64_t cost) {
        return (cost & MaskZ1) >> 0;
    }

    static constexpr void
    SetCost(std::uint64_t& cost, std::uint64_t new_cost, Pauli pauli, std::uint32_t dir) {
        if (pauli == Pauli::X() && dir == 0) {
            SetCostX0(cost, new_cost);
            return;
        } else if (pauli == Pauli::X() && dir == 1) {
            SetCostX1(cost, new_cost);
            return;
        } else if (pauli == Pauli::Z() && dir == 0) {
            SetCostZ0(cost, new_cost);
            return;
        } else if (pauli == Pauli::Z() && dir == 1) {
            SetCostZ1(cost, new_cost);
            return;
        }
        assert(0 && "unreachable");
    }
    static constexpr void SetCostX0(std::uint64_t& cost, std::uint64_t new_cost) {
        cost &= (~MaskX0);
        cost |= (new_cost << 48) & MaskX0;
    }
    static constexpr void SetCostX1(std::uint64_t& cost, std::uint64_t new_cost) {
        cost &= (~MaskX1);
        cost |= (new_cost << 32) & MaskX1;
    }
    static constexpr void SetCostZ0(std::uint64_t& cost, std::uint64_t new_cost) {
        cost &= (~MaskZ0);
        cost |= (new_cost << 16) & MaskZ0;
    }
    static constexpr void SetCostZ1(std::uint64_t& cost, std::uint64_t new_cost) {
        cost &= (~MaskZ1);
        cost |= (new_cost << 0) & MaskZ1;
    }
};
/**
 * @brief Accessor to 'cost' during LATTICE_SURGERY instructions search.
 *
 * @details
 * 64-bit unsigned integer 'cost' is split into 2 regions of 32 bits each:
 * @verbatim
 * +-----------------------+-----------------------+
 * |        costX          |        costY          |
 * | 32 bits (bits 63-32)  | 132 bits (bits 31-0)  |
 * +-----------------------+-----------------------+
 * @endverbatim
 *
 * Explanation:
 * - costX: Cost of the path from a cell in X-axis direction to the current cell.
 * - costY: Cost of the path from a cell in Y-axis direction to the current cell.
 *
 * Example: If cost = 0x0123456789ABCDEF, then:
 *   - costX = 0x01234567
 *   - costY = 0x89ABCDEF
 */
struct LSCostAccessor {
    static constexpr std::uint64_t MaskX = 0xFFFF'FFFF'0000'0000ULL;
    static constexpr std::uint64_t MaskY = 0x0000'0000'FFFF'FFFFULL;
    static constexpr std::uint64_t Inf = 0x0000'0000'FFFF'FFFFULL;

    static constexpr std::uint64_t Cost(std::uint64_t cost, char edge_from) {
        if (edge_from == 'X') {
            return CostX(cost);
        } else if (edge_from == 'Y') {
            return CostY(cost);
        }
        assert(0 && "unreachable");
        return 0;
    }
    static constexpr std::uint64_t CostX(std::uint64_t cost) {
        return (cost & MaskX) >> 32;
    }
    static constexpr std::uint64_t CostY(std::uint64_t cost) {
        return (cost & MaskY) >> 0;
    }

    static constexpr void SetCost(std::uint64_t& cost, std::uint64_t new_cost, char edge_from) {
        if (edge_from == 'X') {
            SetCostX(cost, new_cost);
            return;
        } else if (edge_from == 'Y') {
            SetCostY(cost, new_cost);
            return;
        }
        assert(0 && "unreachable");
    }
    static constexpr void SetCostX(std::uint64_t& cost, std::uint64_t new_cost) {
        cost &= (~MaskX);
        cost |= (new_cost << 32) & MaskX;
    }
    static constexpr void SetCostY(std::uint64_t& cost, std::uint64_t new_cost) {
        cost &= (~MaskY);
        cost |= (new_cost << 0) & MaskY;
    }
};

struct SearchRoute {
    struct Route2D {
        QSymbol q_src;
        QSymbol q_dst;

        Coord3D src;  //!< place of 'q_src'.
        std::list<Coord3D> logical_path;  //!< path of logical operation.
        Coord3D dst;  //!< place of 'q_dst'.

        void Audit() const {
            assert(src == logical_path.front());
            assert(logical_path.back() == dst);
            {
                auto curr_itr = logical_path.begin();
                auto next_itr = std::next(curr_itr);
                while (curr_itr != logical_path.end() && next_itr != logical_path.end()) {
                    const auto& curr = *curr_itr;
                    const auto& next = *next_itr;
                    assert(curr.z == next.z);
                    [[maybe_unused]] const auto diff = curr.XY() - next.XY();
                    assert(diff.x == 0 || diff.y == 0);
                    assert(std::abs(diff.x) + std::abs(diff.y) == 1);
                    curr_itr++;
                    next_itr++;
                }
            }
        }
    };
    struct Route2DM {
        MSymbol magic_factory;
        QSymbol q_dst;

        Coord3D src;  //!< place of 'magic_factory'.
        std::list<Coord3D> logical_path;  //!< path of logical operation.
        Coord3D dst;  //!< place of 'q_dst'.

        void Audit() const {
            assert(src == logical_path.front());
            assert(logical_path.back() == dst);
            {
                auto curr_itr = logical_path.begin();
                auto next_itr = std::next(curr_itr);
                while (curr_itr != logical_path.end() && next_itr != logical_path.end()) {
                    const auto& curr = *curr_itr;
                    const auto& next = *next_itr;
                    assert(curr.z == next.z);
                    [[maybe_unused]] const auto diff = curr.XY() - next.XY();
                    assert(diff.x == 0 || diff.y == 0);
                    assert(std::abs(diff.x) + std::abs(diff.y) == 1);
                    curr_itr++;
                    next_itr++;
                }
            }
        }
    };
    struct Route2DE {
        ESymbol e_factory;
        QSymbol q_dst;

        Coord3D src;  //!< place of 'e_factory'.
        std::list<Coord3D> logical_path;  //!< path of logical operation.
        Coord3D dst;  //!< place of 'q_dst'.

        void Audit() const {
            assert(src == logical_path.front());
            assert(logical_path.back() == dst);
            {
                auto curr_itr = logical_path.begin();
                auto next_itr = std::next(curr_itr);
                while (curr_itr != logical_path.end() && next_itr != logical_path.end()) {
                    const auto& curr = *curr_itr;
                    const auto& next = *next_itr;
                    assert(curr.z == next.z);
                    [[maybe_unused]] const auto diff = curr.XY() - next.XY();
                    assert(diff.x == 0 || diff.y == 0);
                    assert(std::abs(diff.x) + std::abs(diff.y) == 1);
                    curr_itr++;
                    next_itr++;
                }
            }
        }
    };
    struct Ancilla2D {
        std::list<QSymbol> qs;
        std::list<ESymbol> es;
        std::optional<MSymbol> m;

        std::list<Coord3D> ancilla;

        void Audit() const {
            // TODO: Check if ancilla is connected.
            if (ancilla.empty()) {
                return;
            }
            [[maybe_unused]] const auto z = ancilla.front().z;
            for ([[maybe_unused]] const auto& p : ancilla) {
                assert(p.z == z);
            }
        }
    };

    struct Route3D {
        QSymbol q_src;
        QSymbol q_dst;

        Coord3D src;  //!< place of 'q_src'.
        std::list<Coord3D> move_path;  //!< path of plain move (after trans move).
        std::list<Coord3D> logical_path;  //!< path of logical operation (after trans move + plain
                                          //!< move + trans move).
        Coord3D dst;  //!< place of 'q_dst'.

        void Audit() const {
            assert(src.XY() == move_path.front().XY());
            assert(move_path.back().XY() == logical_path.front().XY());
            assert(logical_path.back() == dst);
        }
    };
    struct Route3DM {
        MSymbol magic_factory;
        QSymbol q_dst;
        std::uint32_t moved_state_dir = 0;

        Coord3D src;
        std::list<Coord3D> move_path;
        std::list<Coord3D> logical_path;
        Coord3D dst;

        void Audit() const {
            assert(src == move_path.front());
            assert(move_path.back().XY() == logical_path.front().XY());
            assert(logical_path.back() == dst);
        }
    };

    static constexpr std::uint32_t LS_X_BOUNDARY = 1;
    static constexpr std::uint32_t LS_Z_BOUNDARY = 3;
    static constexpr std::uint32_t LS_ANY_BOUNDARY = 4;
    static constexpr std::uint32_t GetBoundaryCode(Pauli pauli) {
        if (pauli.IsX()) {
            return LS_X_BOUNDARY;
        } else if (pauli.IsZ()) {
            return LS_Z_BOUNDARY;
        }
        return LS_ANY_BOUNDARY;
    }

    // For 2D

private:
    static constexpr auto Unreachable2D = std::numeric_limits<std::uint64_t>::max();

    static std::queue<Coord2D> InitializeQueueAndPlane2D(
            QuantumPlane& plane,
            const Coord2D& place_src,
            std::uint32_t dir_src,
            std::uint32_t boundaries_src
    );
    static std::queue<Coord2D> InitializeQueueAndPlane2DM(QuantumPlane& plane);
    static std::set<Coord2D> InitializeGoal2D(
            QuantumPlane& plane,
            const Coord2D& place_dst,
            std::uint32_t dir_dst,
            std::uint32_t boundaries_dst
    );
    static std::optional<Coord2D>
    RunBFS2D(QuantumPlane& plane, std::queue<Coord2D>& queue, const std::set<Coord2D>& set);

public:
    /**
     * @brief Run BFS for LATTICE_SURGERY.
     *
     * @details
     *
     * value of 'boundaries' :
     * * 1: Start BFS from X boundary.
     * * 3: Start BFS from Z boundary.
     * * 4: Start BFS from both Z and X boundary.
     *
     * @note
     * * Costs of free ancillae must be initialized to std::numeric_limits<std::uint64_t>::max()
     * before using this method.
     * * Assume latency of LATTICE_SURGERY is 1.
     *
     * @param plane
     * @param q_src
     * @param q_dst
     * @param boundaries_src
     * @param boundaries_dst
     * @return std::optional<Route2D> Route from 'q_src' to 'q_dst' including both boundary if
     * found, std::nullopt otherwise.
     */
    static std::optional<Route2D> FindRoute2D(
            QuantumPlane& plane,
            QSymbol q_src,
            QSymbol q_dst,
            std::uint32_t boundaries_src,
            std::uint32_t boundaries_dst
    );
    /**
     * @brief Run BFS for LATTICE_SURGERY_MAGIC.
     *
     * @details
     *
     * value of 'boundaries' :
     * * 1: Start BFS from X boundary.
     * * 3: Start BFS from Z boundary.
     * * 4: Start BFS from both Z and X boundary.
     *
     * @note
     * * Costs of free ancillae must be initialized to std::numeric_limits<std::uint64_t>::max()
     * before using this method.
     * * Assume latency of LATTICE_SURGERY_MAGIC is 1.
     *
     * @param plane
     * @param q_dst
     * @param boundaries_dst
     * @return std::optional<Route2DM> Route from magic factory to 'q_dst' including both boundary
     * if found, std::nullopt otherwise.
     */
    static std::optional<Route2DM>
    FindRoute2DM(QuantumPlane& plane, QSymbol q_dst, std::uint32_t boundaries_dst);
    static std::optional<Route2DE> FindRoute2DE(
            QuantumPlane& plane,
            ESymbol e_factory,
            QSymbol q_dst,
            std::uint32_t boundaries_dst
    );
    static std::optional<Ancilla2D> FindAncilla(
            QuantumPlane& plane,
            const std::list<QSymbol>& qs,
            const std::list<Pauli>& boundaries,
            const std::list<ESymbol>& es,
            bool use_magic_state
    );

    // For 3D
    /**
     * @brief Run BFS for LATTICE_SURGERY.
     *
     * @details
     *
     * value of 'boundaries' :
     * * 1: Start BFS from X boundary.
     * * 3: Start BFS from Z boundary.
     * * 4: Start BFS from both Z and X boundary.
     *
     * @note Assume latency of LATTICE_SURGERY is 1.
     *
     * @param plane
     * @param q
     * @param boundaries
     * @return true
     * @return false
     */
    static bool LS(QuantumPlane& plane, QSymbol q, std::uint32_t boundaries);
    /**
     * @brief
     *
     * @param plane
     * @param q
     * @param edge_from
     * @param place
     * @return std::list<Coord3D> path from 'place' to 'q'. (contains both boundary place, i.e.
     * 'place' and place of 'q')
     */
    static std::list<Coord3D>
    TraceBackPathOfLS(QuantumPlane& plane, QSymbol q, char edge_from, const Coord2D& place);

    /**
     * @brief Q
     * @details
     * @todo Replaced QSymbol with Coordinate to catch up with SearchRoute::Move after trans move.
     *
     * @param plane
     * @param q
     * @return true
     * @return false
     */
    static bool Move(QuantumPlane& plane, QSymbol q);
    /**
     * @brief
     *
     * @param plane
     * @param q
     * @param pauli
     * @param moved_state_dir
     * @param place
     * @return std::list<Coord3D> path from 'place' to 'q'. (contains both boundary place, i.e.
     * 'place' and place of 'q')
     */
    static std::list<Coord3D> TraceBackPathOfMove(
            QuantumPlane& plane,
            QSymbol q,
            Pauli pauli,
            std::uint32_t moved_state_dir,
            const Coord2D& place
    );

    /**
     * @brief Run BFS for MOVE_MAGIC.
     *
     * @param plane
     * @return true
     * @return false
     */
    static bool MoveMagic(QuantumPlane& plane);
    static std::pair<MSymbol, std::list<Coord3D>> TraceBackPathOfMoveMagic(
            QuantumPlane& plane,
            Pauli pauli,
            std::uint32_t moved_state_dir,
            const Coord2D& place
    );

    /**
     * @brief Search 3D route.
     *
     * @details
     *
     * value of 'boundaries' :
     * * 1: Connect to X boundary of moved 'q' at 'plane_2'.
     * * 3: Connect to Z boundary of moved 'q' at 'plane_2'.
     * * 4: Connect to either Z or X boundary of moved 'q' at 'plane_2'.
     *
     * @param grid_0
     * @param grid_1
     * @param plane_2
     * @param q_src
     * @param q_dst
     * @param boundaries_src
     * @return true
     * @return false
     */
    static std::optional<Route3D> FindRoute3D(
            QuantumGrid& grid_0,
            QuantumGrid& grid_1,
            QuantumPlane& plane_2,
            QSymbol q_src,
            QSymbol q_dst,
            std::uint32_t boundaries_src
    );

    /**
     * @brief
     *
     * @param grid_0
     * @param grid_1
     * @param plane_2
     * @param q_dst
     * @return std::optional<Route3DM>
     */
    static std::optional<Route3DM>
    FindRoute3DM(QuantumGrid& grid_0, QuantumGrid& grid_1, QuantumPlane& plane_2, QSymbol q_dst);

    /**
     * @brief
     *
     * @param grid
     * @param src
     * @param dst_z
     * @return true
     * @return false
     */
    static bool TransReachable(QuantumGrid& grid, const Coord3D& src, std::int32_t dst_z);

private:
    struct SearchResult {
        std::uint64_t min_cost = std::numeric_limits<std::uint64_t>::max();
        Pauli pauli;
        std::uint32_t moved_state_dir;
        char edge_from;
        std::int32_t z;  //!< z of plane where move occurred.
        Coord2D place;  //<! place where trans move to plane_2 occurred (after plane move).

        friend std::ostream& operator<<(std::ostream& out, const SearchResult& result) {
            const auto& [min_cost, pauli, moved_state_dir, edge_from, z, place] = result;
            return out << fmt::format(
                           "min_cost: {}, pauli: {}, moved_state_dir: {}, edge_from: {}, z: {}, "
                           "place: "
                           "{}",
                           min_cost,
                           pauli.ToChar(),
                           moved_state_dir,
                           edge_from,
                           z,
                           place
                   );
        }
    };
    static constexpr auto MoveWeight = std::uint64_t{2};
    static constexpr auto LogicalWeight = std::uint64_t{3};
    static constexpr auto TransMoveWeight = std::uint64_t{1};
    static constexpr std::uint64_t
    CalculateRoute3DCost(std::uint64_t mo, std::uint64_t ls, std::int32_t trans_move_cost) {
        return (MoveWeight * mo) + (LogicalWeight * ls)
                + (TransMoveWeight * static_cast<std::uint64_t>(trans_move_cost));
    }
    static constexpr std::optional<std::uint64_t> IsMinCostUpdated(
            std::uint64_t current_min_cost,
            std::uint64_t mo,
            std::uint64_t ls,
            std::int32_t trans_move_cost
    ) {
        const auto cost = CalculateRoute3DCost(mo, ls, trans_move_cost);
        if (mo != MoveCostAccessor::Inf && ls != LSCostAccessor::Inf && cost < current_min_cost) {
            return cost;
        }
        return std::nullopt;
    };
    static constexpr auto AnySrcZ = std::numeric_limits<std::int32_t>::max();
    static std::optional<SearchResult> FindTransMovePlace(
            QuantumGrid& grid_0,
            QuantumGrid& grid_1,
            QuantumPlane& plane_2,
            std::int32_t src_z,
            std::uint32_t boundaries
    );
};
}  // namespace qret::sc_ls_fixed_v0

#endif  // QRET_TARGET_SC_LS_FIXED_V0_SEARCH_GRID_H
