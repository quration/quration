/**
 * @file qret/analysis/compute_graph.h
 * @brief Compute-graph IR and DOT exporter with region-aware bundling.
 * @details This module defines a light-weight compute graph for visualizing dataflow
 * between instruction regions over (logical) arguments such as qubits or registers
 *
 * A node may contain multiple *regions* (ports), each region spans
 * a contiguous range of argument indices `[0, size)`. Edges connect a single
 * argument position `(node, region, arg)` to another.
 *
 * The graph can be exported to Graphviz DOT via `BuildAsDot()`. The exporter:
 *
 * - Emits each instruction as a `record`-shaped node with per-region ports.
 * - Groups consecutive argument indices into *blocks* to avoid drawing one
 *   edge per element; instead, edges are drawn per block and labeled with
 *   their width (number of grouped arguments).
 * - Inserts auxiliary *join* (west side) and *split* (east side) nodes when a
 *   region's adjacent positions are partitioned into multiple blocks.
 *
 * ### Coordinate system
 *
 * - NodeId: index into `nodes`.
 * - RegionId: index into `Node::regions`.
 * - ArgId: index of an argument within a region `[0, Region::size)`.
 *
 * ### Invariants / assumptions
 *
 * - `ir::Qubit::id` and `ir::Register::id` share the same type (enforced by `static_assert`).
 * - Positions used by edges must be within declared region sizes.
 * - `BuildAsDot()` assumes that block boundaries at source and destination sides are consistent; a
 *     mismatch triggers `assert`.
 *
 * @see DotGraph for the underlying DOT builder.
 */

#ifndef QRET_ANALYSIS_COMPUTE_GRAPH_H
#define QRET_ANALYSIS_COMPUTE_GRAPH_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "qret/analysis/dot.h"
#include "qret/ir/value.h"
#include "qret/qret_export.h"

namespace qret {
/**
 * @brief IR-level compute graph for regioned instructions.
 * @details A compute graph consists of:
 *
 * - **Nodes**: either initialization nodes (`is_init == true`) with a single
 *   region, or instruction nodes with one or more regions.
 * - **Regions**: typed ports with a declared `size` (number of arguments).
 * - **Edges**: fine-grained connections from a single `(node, region, arg)`
 *   to another.
 *
 * Use `AddInit()` and `AddInstruction()` to build nodes and `AddEdge()` to
 * connect positions. Call `BuildAsDot()` to get a compact, region-aware DOT.
 */
class QRET_EXPORT ComputeGraph {
public:
    static_assert(std::is_same_v<decltype(ir::Qubit::id), decltype(ir::Register::id)>);

    using NodeId = std::size_t;
    using RegionId = std::size_t;
    using ArgId = decltype(ir::Qubit::id);

    enum class ArgKind : std::uint8_t { Qubit, Register };
    struct Region {
        std::string label;
        ArgKind kind;
        ArgId size;
    };
    struct Node {
        std::string label;
        std::vector<Region> regions;
        bool is_init;
    };
    struct Position {
        NodeId node;
        RegionId region;
        ArgId arg;
    };
    struct Edge {
        Position src;
        Position dst;
    };

    ComputeGraph() = default;

    NodeId AddInit(std::string_view label, ArgKind kind, ArgId size) {
        NodeId id = nodes_.size();
        nodes_.emplace_back(
                std::string(label),
                std::vector<Region>{Region{.label = "", .kind = kind, .size = size}},
                true
        );
        return id;
    }

    NodeId AddInstruction(std::string_view label, const std::vector<Region>& regions) {
        const auto id = NodeId{nodes_.size()};
        nodes_.emplace_back(std::string(label), regions, false);
        return id;
    }

    void AddEdge(Position src, Position dst) {
        edges_.push_back(Edge(src, dst));
    }

    DotGraph BuildAsDot() const;

private:
    std::vector<Node> nodes_;
    std::vector<Edge> edges_;
};
}  // namespace qret

#endif  // QRET_ANALYSIS_COMPUTE_GRAPH_H
