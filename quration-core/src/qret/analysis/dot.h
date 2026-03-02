/**
 * @file qret/analysis/dot.h
 * @brief Minimal DOT (Graphviz) graph builder with streaming support.
 * @details This module provides a lightweight utility for constructing a directed graph
 * in the DOT language (as used by Graphviz) and streaming it to an output
 * stream. It focuses on a small subset of node/edge attributes (label, shape)
 * but is designed to be easily extended.
 *
 * ### Design notes
 *
 * - Node identifiers are emitted **as-is** (unquoted). If your identifiers
 *   contain spaces or special characters, quote them on input (e.g., `"A B"`).
 * - Attribute values for `label` and `shape` are quoted and not escaped beyond
 *   that. If you use quotes or backslashes inside labels, you may need to
 *   sanitize them beforehand.
 * - Insertion order for nodes follows the ordering of `std::map` keys.
 *   Multiple edges between the same endpoints are supported via `std::multimap`.
 *
 * @see https://graphviz.org/doc/info/lang.html (DOT language)
 * @see https://graphviz.org/docs/nodes/ (Node attributes)
 * @see https://graphviz.org/docs/edges/ (Edge attributes)
 */

#ifndef QRET_ANALYSIS_DOT_H
#define QRET_ANALYSIS_DOT_H

#include <fmt/format.h>

#include <map>
#include <ostream>
#include <string>
#include <string_view>

#include "qret/qret_export.h"

namespace qret {
/**
 * @brief Node attribute bundle for DOT emission.
 * @details
 * Currently supports:
 * - `label`: text shown in/near the node (emitted as `label="..."`).
 * - `shape`: Graphviz node shape name (e.g., `circle`, `box`, `ellipse`).
 *
 * Attributes that are empty strings are omitted from the output.
 */
struct DotNodeOption {
    std::string label;
    std::string shape;
};

std::ostream& operator<<(std::ostream& out, const DotNodeOption& option);

/**
 * @brief Edge attribute bundle for DOT emission.
 * @details
 * Currently supports:
 * - `label`: text shown near the edge (emitted as `label="..."`).
 *
 * Attributes that are empty strings are omitted from the output.
 */
struct DotEdgeOption {
    std::string label;
};

std::ostream& operator<<(std::ostream& out, const DotEdgeOption& option);

class QRET_EXPORT DotGraph {
public:
    explicit DotGraph(std::string_view name)
        : name_(std::string(name)) {}

    [[nodiscard]] std::string GetName() const {
        return name_;
    }
    [[nodiscard]] std::size_t NumNodes() const {
        return nodes_.size();
    }
    [[nodiscard]] std::size_t NumEdges() const {
        return direct_edges_.size();
    }
    [[nodiscard]] bool HasNode(std::string_view name) const {
        return nodes_.contains(std::string(name));
    }
    [[nodiscard]] std::size_t NumEdges(std::string_view src, std::string_view dst) const {
        const auto key = std::make_pair(std::string(src), std::string(dst));
        return direct_edges_.count(key);
    }

    void AddNode(std::string_view name, const DotNodeOption& node_option = {}) {
        nodes_.emplace(std::string(name), node_option);
    }
    void AddNode(std::string_view name, std::string_view label, std::string_view shape = "") {
        AddNode(name, DotNodeOption{.label = std::string(label), .shape = std::string(shape)});
    }

    void
    AddDirectedEdge(std::string_view src, std::string_view dst, const DotEdgeOption& option = {}) {
        auto src_s = std::string(src);
        auto dst_s = std::string(dst);
        direct_edges_.insert(std::make_pair(std::make_pair(src_s, dst_s), option));
    }
    void AddDirectedEdge(std::string_view src, std::string_view dst, std::string_view label) {
        AddDirectedEdge(src, dst, DotEdgeOption{.label = std::string(label)});
    }

private:
    friend std::ostream& operator<<(std::ostream& out, const DotGraph& graph);

    std::string name_;
    std::map<std::string, DotNodeOption> nodes_;
    std::multimap<std::pair<std::string, std::string>, DotEdgeOption> direct_edges_;
};

std::ostream& operator<<(std::ostream& out, const DotGraph& graph);
}  // namespace qret

#endif  // QRET_ANALYSIS_DOT_H
