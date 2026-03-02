/**
 * @file qret/analysis/dot.cpp
 * @brief Minimal DOT (Graphviz) graph builder with streaming support.
 */

#include "qret/analysis/dot.h"

namespace qret {
std::ostream& operator<<(std::ostream& out, const DotNodeOption& option) {
    out << "[";
    if (!option.label.empty()) {
        out << fmt::format("label = \"{}\"; ", option.label);
    }
    if (!option.shape.empty()) {
        out << fmt::format("shape = \"{}\"; ", option.shape);
    }
    out << "]";
    return out;
}

std::ostream& operator<<(std::ostream& out, const DotEdgeOption& option) {
    out << "[";
    if (!option.label.empty()) {
        out << fmt::format("label = \"{}\"; ", option.label);
    }
    out << "]";
    return out;
}

std::ostream& operator<<(std::ostream& out, const DotGraph& graph) {
    out << "digraph \"" << graph.name_ << "\" {\n";
    for (const auto& [name, option] : graph.nodes_) {
        out << "    " << name << " " << option << ";\n";
    }
    for (const auto& [src_dst, option] : graph.direct_edges_) {
        const auto& [src, dst] = src_dst;
        out << "    " << src << " -> " << dst << " " << option << ";\n";
    }
    out << "}\n";
    return out;
}
}  // namespace qret
