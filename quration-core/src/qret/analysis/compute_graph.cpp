/**
 * @file qret/analysis/compute_graph.cpp
 * @brief Compute-graph IR and DOT exporter with region-aware bundling.
 */

#include "compute_graph.h"

#include <fmt/format.h>

#include <cassert>
#include <optional>
#include <ranges>
#include <sstream>
#include <vector>

#include "qret/analysis/dot.h"

namespace qret {
/**
 * @brief Partition adjacent positions into contiguous blocks.
 * @details Given the per-argument adjacency along a single region (e.g., the vector
 * indexed by `arg` in `[0, size)` holding the nearest west/east neighbor
 * positions, or `nullopt` where none), this helper computes block boundaries
 * such that indices belong to the same block iff:
 * - Both current and previous entries are absent (`nullopt`), or
 * - Both are present and refer to the *same* `(node, region)` with consecutive
 *   `arg` values (`prev.arg + 1 == cur.arg`).
 *
 * The resulting `bound` vector stores block starts followed by a sentinel end:
 * `bound = [b0=0, b1, b2, ..., bk, end=size]`. The number of blocks is
 * `k = bound.size() - 1`.
 *
 * @note Two adjacent `nullopt` entries are considered *continuous* and will be
 *       grouped into the same block (useful when rendering unlabeled spans).
 */
struct RegionBlocks {
    using ArgId = ComputeGraph::ArgId;
    using Position = ComputeGraph::Position;

    std::vector<ArgId> bound;

    /**
     * @brief Construct from adjacent positions of a region
     *
     * @param adjacent adjacent positions of a region
     */
    explicit RegionBlocks(const std::vector<std::optional<Position>>& adjacent) {
        const auto num_args = ArgId{adjacent.size()};
        bound.push_back(0);
        for (const auto arg_id : std::views::iota(ArgId{1}, num_args)) {
            const auto continuous_from_prev = [&] {
                const auto cur_pos = adjacent[arg_id];
                const auto prev_pos = adjacent[arg_id - 1];
                if (!cur_pos.has_value() && !prev_pos.has_value()) {
                    return true;
                }
                if (cur_pos.has_value() ^ prev_pos.has_value()) {
                    return false;
                }
                return cur_pos->node == prev_pos->node && cur_pos->region == prev_pos->region
                        && cur_pos->arg == prev_pos->arg + 1;
            };
            if (!continuous_from_prev()) {
                bound.push_back(arg_id);
            }
        }
        bound.push_back(num_args);
    }

    [[nodiscard]] std::size_t NumBlocks() const {
        return bound.size() - 1;
    }

    [[nodiscard]] std::size_t GetBlockId(ArgId arg_id) const {
        auto it = std::ranges::upper_bound(bound, arg_id);
        if (it == bound.begin() || it == bound.end()) {
            throw std::runtime_error("out of bound");
        }
        return static_cast<std::size_t>(std::distance(bound.begin(), it)) - 1;
    }

    [[nodiscard]] std::pair<std::size_t, std::size_t> GetBlockRange(std::size_t block_id) const {
        if (block_id + 1 >= bound.size()) {
            throw std::runtime_error("out of bound");
        }
        return {bound[block_id], bound[block_id + 1]};
    }
};

DotGraph ComputeGraph::BuildAsDot() const {
    const auto num_nodes = NodeId{nodes_.size()};

    // get adjacent node
    auto adjacent_west = std::vector<std::vector<std::vector<std::optional<Position>>>>(num_nodes);
    auto adjacent_east = std::vector<std::vector<std::vector<std::optional<Position>>>>(num_nodes);
    for (const auto node_id : std::views::iota(NodeId{0}, num_nodes)) {
        const auto& regions = nodes_[node_id].regions;
        const auto num_regions = RegionId{regions.size()};
        adjacent_west[node_id].resize(num_regions);
        adjacent_east[node_id].resize(num_regions);
        for (const auto region_id : std::views::iota(RegionId{0}, num_regions)) {
            adjacent_west[node_id][region_id].resize(regions[region_id].size, std::nullopt);
            adjacent_east[node_id][region_id].resize(regions[region_id].size, std::nullopt);
        }
    }
    for (const auto& [src, dst] : edges_) {
        adjacent_west[dst.node][dst.region][dst.arg] = src;
        adjacent_east[src.node][src.region][src.arg] = dst;
    }

    // get continuous blocks
    const auto adjacent_to_blocks =
            [&](const std::vector<std::vector<std::vector<std::optional<Position>>>>&
                        node_adjacent) {
                auto blocks = std::vector<std::vector<RegionBlocks>>(num_nodes);
                std::ranges::transform(node_adjacent, blocks.begin(), [](const auto& regions) {
                    auto blocks_per_region = std::vector<RegionBlocks>{};
                    blocks_per_region.reserve(regions.size());
                    std::ranges::transform(
                            regions,
                            std::back_inserter(blocks_per_region),
                            [](const auto& adjacent) { return RegionBlocks(adjacent); }
                    );
                    return blocks_per_region;
                });
                return blocks;
            };
    const auto blocks_west = adjacent_to_blocks(adjacent_west);
    const auto blocks_east = adjacent_to_blocks(adjacent_east);

    // draw graph
    auto dot = DotGraph("circuit");
    for (const auto node_id : std::views::iota(NodeId{0}, num_nodes)) {
        const auto& node = nodes_[node_id];
        const auto& regions = node.regions;

        // add standard_node
        const auto name = fmt::format("n{}", node_id);
        if (node.is_init) {
            const auto label = fmt::format("<r0> {}", node.label);
            dot.AddNode(name, {.label = label, .shape = "record"});
        } else {
            auto label_ss = std::stringstream{};
            label_ss << "{" << node.label;
            for (const auto region_id : std::views::iota(RegionId{0}, RegionId{regions.size()})) {
                label_ss << "|<r" << region_id << "> " << regions[region_id].label;
            }
            label_ss << "}";
            dot.AddNode(name, {.label = label_ss.str(), .shape = "record"});
        }

        // add join node
        for (const auto region_id : std::views::iota(RegionId{0}, RegionId{regions.size()})) {
            const auto& blocks = blocks_west[node_id][region_id];
            auto num_blocks = blocks.NumBlocks();
            if (num_blocks == 1) {
                continue;
            }
            const auto name = fmt::format("j{}_{}", node_id, region_id);
            auto label_ss = std::stringstream{};
            label_ss << "{ |{{";
            for (const auto block_id : std::views::iota(std::size_t{0}, num_blocks)) {
                if (block_id != 0) {
                    label_ss << "|";
                }
                label_ss << "<b" << block_id << "> ";
            }
            label_ss << "}|<dst> }}";
            dot.AddNode(name, {.label = label_ss.str(), .shape = "record"});
            dot.AddDirectedEdge(
                    fmt::format("j{}_{}:dst:e", node_id, region_id),
                    fmt::format("n{}:r{}:w", node_id, region_id),
                    {.label = std::to_string(nodes_[node_id].regions[region_id].size)}
            );
        }

        // add split node
        for (const auto region_id : std::views::iota(RegionId{0}, RegionId{regions.size()})) {
            const auto& blocks = blocks_east[node_id][region_id];
            auto num_blocks = blocks.NumBlocks();
            if (num_blocks == 1) {
                continue;
            }
            const auto name = fmt::format("s{}_{}", node_id, region_id);
            auto label_ss = std::stringstream{};
            label_ss << "{ |{<src> |{";
            for (const auto block_id : std::views::iota(std::size_t{0}, num_blocks)) {
                if (block_id != 0) {
                    label_ss << "|";
                }
                label_ss << "<b" << block_id << "> ";
            }
            label_ss << "}}}";
            dot.AddNode(name, {.label = label_ss.str(), .shape = "record"});
            dot.AddDirectedEdge(
                    fmt::format("n{}:r{}:e", node_id, region_id),
                    fmt::format("s{}_{}:src:w", node_id, region_id),
                    {.label = std::to_string(nodes_[node_id].regions[region_id].size)}
            );
        }
    }

    for (const auto& [src, dst] : edges_) {
        const auto& [src_n, src_r, src_a] = src;
        const auto& [dst_n, dst_r, dst_a] = dst;
        const auto& src_blocks = blocks_east[src_n][src_r];
        const auto& dst_blocks = blocks_west[dst_n][dst_r];
        const auto src_block_id = src_blocks.GetBlockId(src_a);
        const auto dst_block_id = dst_blocks.GetBlockId(dst_a);
        const auto [src_block_begin, src_block_end] = src_blocks.GetBlockRange(src_block_id);
        const auto [dst_block_begin, dst_block_end] = dst_blocks.GetBlockRange(dst_block_id);
        const auto src_is_bound = src_block_begin == src_a;
        const auto dst_is_bound = dst_block_begin == dst_a;
        assert(src_is_bound == dst_is_bound);
        if (src_is_bound || dst_is_bound) {
            const auto width = src_block_end - src_block_begin;
            const auto src_is_split = src_blocks.NumBlocks() > 1;
            const auto dst_is_join = dst_blocks.NumBlocks() > 1;
            const auto src_label = src_is_split
                    ? fmt::format("s{}_{}:b{}:e", src_n, src_r, src_block_id)
                    : fmt::format("n{}:r{}:e", src_n, src_r);
            const auto dst_label = dst_is_join
                    ? fmt::format("j{}_{}:b{}:w", dst_n, dst_r, dst_block_id)
                    : fmt::format("n{}:r{}:w", dst_n, dst_r);
            dot.AddDirectedEdge(src_label, dst_label, {.label = std::to_string(width)});
        }
    }
    return dot;
}
}  // namespace qret
