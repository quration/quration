/**
 * @file qret/base/graph.cpp
 * @brief Basic graph classes and functions.
 */

#include "qret/base/graph.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <list>
#include <numeric>
#include <queue>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace qret {
std::size_t DiGraph::NumNodes() const {
    return node_list_.size();
}
std::size_t DiGraph::NumEdges() const {
    return edge_list_.size();
}
bool DiGraph::HasNode(IdType id) const {
    return id2node_.contains(id);
}
const DiGraph::Node& DiGraph::GetNode(IdType id) const {
    return *id2node_.at(id);
}
bool DiGraph::HasEdge(IdType from, IdType to) const {
    return edge_list_.contains(this->GetEdgeKey(from, to));
}
const DiGraph::Edge& DiGraph::GetEdge(IdType from, IdType to) const {
    return edge_list_.at(this->GetEdgeKey(from, to));
}
const DiGraph::Node& DiGraph::AddNode(IdType id, Weight weight) {
    UpdateGraph();

    node_list_.emplace_back(Node{id, weight});
    id2node_[id] = --(node_list_.end());
    return *id2node_[id];
}
const DiGraph::Edge& DiGraph::AddEdge(IdType from, IdType to, Length length) {
    UpdateGraph();

    auto f = id2node_[from];
    auto t = id2node_[to];
    f->child.insert(t->id);
    t->parent.insert(f->id);
    return edge_list_[this->GetEdgeKey(from, to)] = Edge{length};
}
void DiGraph::DelNode(IdType id) {
    UpdateGraph();

    auto itr = id2node_[id];
    for (const auto& x : itr->parent) {
        id2node_[x]->child.erase(id);
        edge_list_.erase(this->GetEdgeKey(x, id));
    }
    for (const auto& x : itr->child) {
        id2node_[x]->parent.erase(id);
        edge_list_.erase(this->GetEdgeKey(id, x));
    }
    id2node_.erase(id);
    node_list_.erase(itr);
}
void DiGraph::DelEdge(IdType from, IdType to) {
    UpdateGraph();

    auto f = id2node_[from];
    auto t = id2node_[to];
    f->child.erase(t->id);
    t->parent.erase(f->id);
    edge_list_.erase(this->GetEdgeKey(from, to));
}
namespace {
bool Visit(
        const DiGraph::IdType id,
        const std::unordered_map<DiGraph::IdType, DiGraph::Iterator>& id2node,
        std::unordered_map<DiGraph::IdType, std::uint32_t>& mp,
        std::list<DiGraph::Node>& sorted
) {
    if (mp[id] == 1u) {
        return false;
    } else if (mp[id] == 2u) {
        return true;
    }
    assert(mp[id] == 0u);
    mp[id] = 1u;
    for (const auto& node : id2node.at(id)->child) {
        const auto success = Visit(node, id2node, mp, sorted);
        if (!success) {
            return false;
        }
    }
    mp[id] = 2u;
    sorted.emplace_front(*id2node.at(id));
    return true;
}
}  // namespace
bool DiGraph::Topsort() {
    // Status: 0->unvisited, 1->temporary, 2->visited
    auto mp = std::unordered_map<IdType, std::uint32_t>();
    for (const auto& [id, _] : id2node_) {
        mp[id] = 0u;
    }
    auto sorted = std::list<Node>();

    // Run dfs
    for (const auto& [id, node] : id2node_) {
        if (mp[id] == 2u) {
            continue;
        }
        const auto success = Visit(id, id2node_, mp, sorted);
        if (!success) {
            // Cannot convert DiGraph to DAG
            is_dag_ = false;
            return false;
        }
    }

    // Update container
    node_list_ = sorted;
    for (auto itr = node_list_.begin(); itr != node_list_.end(); ++itr) {
        id2node_[itr->id] = itr;
    }
    is_dag_ = true;
    for (const auto& node : node_list_) {
        auto depth = std::uint32_t{0};
        for (const auto& parent : node.parent) {
            assert(depth_.contains(parent));
            depth = std::max(depth, depth_.at(parent) + 1);
        }
        depth_[node.id] = depth;
    }
    return true;
}
bool IsDAG(const DiGraph& dig) {
    if (dig.IsDAG() == 0) {
        return false;
    }
    if (dig.IsDAG() == 1) {
        return true;
    }

    assert(dig.IsDAG() == -1);

    auto appeared = std::unordered_set<DiGraph::IdType>();
    appeared.reserve(dig.NumNodes());
    for (const auto& node : dig) {
        for (const auto& child : node.child) {
            if (appeared.contains(child)) {
                return false;
            }
        }
        appeared.insert(node.id);
    }
    return true;
}
std::tuple<
        DiGraph::Weight,
        std::vector<DiGraph::IdType>,
        std::unordered_map<DiGraph::IdType, DiGraph::Weight>>
FindHeaviestPath(const DiGraph& dag) {
    if (
            // dag is not DAG
            dag.IsDAG() == 0 ||
            // DAG or not is unknown at first and dag is not DAG after explicitly calling IsDAG()
            // function
            (dag.IsDAG() == -1 && !IsDAG(dag))) {
        throw std::runtime_error(
                "graph must be DAG.\nTry DiGraph::TopSort() before calling FindHeaviestPath."
        );
    }

    auto max = DiGraph::Weight{0};
    auto mp = std::unordered_map<DiGraph::IdType, DiGraph::Weight>();
    for (const auto& node : dag) {
        auto tmp_max = DiGraph::Weight{0};
        for (const auto& parent : node.parent) {
            tmp_max = std::max(tmp_max, mp[parent]);
        }
        mp[node.id] = tmp_max + node.weight;
        max = std::max(max, mp[node.id]);
    }
    const auto heaviest_weight = max;

    auto path = std::vector<DiGraph::IdType>();
    for (auto itr = dag.rbegin(); itr != dag.rend(); ++itr) {
        const auto& node = *itr;
        const auto id = node.id;
        if (mp.at(id) == max) {
            path.emplace_back(id);
            max -= node.weight;
        }
    }

    return {heaviest_weight, path, mp};
}
std::tuple<
        DiGraph::Length,
        std::vector<DiGraph::IdType>,
        std::unordered_map<DiGraph::IdType, DiGraph::Length>>
FindLongestPath(const DiGraph& dag) {
    if (
            // dag is not DAG
            dag.IsDAG() == 0 ||
            // DAG or not is unknown at first and dag is not DAG after explicitly calling IsDAG()
            // function
            (dag.IsDAG() == -1 && !IsDAG(dag))) {
        throw std::runtime_error(
                "graph must be DAG.\nTry DiGraph::TopSort() before calling FindLongestPath."
        );
    }

    auto max = DiGraph::Length{0};
    auto mp = std::unordered_map<DiGraph::IdType, DiGraph::Length>();
    for (const auto& node : dag) {
        auto tmp_max = DiGraph::Length{0};
        for (const auto& parent : node.parent) {
            const auto edge = dag.GetEdge(parent, node.id);
            tmp_max = std::max(tmp_max, mp[parent] + edge.length);
        }
        mp[node.id] = tmp_max;
        max = std::max(max, mp[node.id]);
    }
    const auto longest_length = max;

    auto path = std::vector<DiGraph::IdType>();
    for (auto itr = dag.rbegin(); itr != dag.rend(); ++itr) {
        const auto& node = *itr;
        const auto id = node.id;
        if (mp.at(id) == max) {
            path.emplace_back(id);
            max -= node.weight;
        }
    }

    return {longest_length, path, mp};
}
std::string ExportDiGraphToDOT(
        const std::string& name,
        const DiGraph& dig,
        const std::unordered_map<DiGraph::IdType, std::string>& hook
) {
    const auto nid = [](const auto& id) -> std::string { return "n" + std::to_string(id); };

    auto ss = std::stringstream();
    ss << "digraph " << name << " {\n";
    // Print nodes
    for (const auto& node : dig) {
        const auto id = node.id;
        if (hook.contains(id)) {
            ss << nid(id) << " [label=\"" << hook.at(id) << "\"];\n";
        } else {
            ss << nid(id) << ";\n";
        }
    }
    // Print edges
    for (const auto& node : dig) {
        const auto id = node.id;
        for (const auto& child : node.child) {
            ss << nid(id) << " -> " << nid(child) << ";\n";
        }
    }
    ss << "}\n";

    auto ret = ss.str();
    ret.resize(ret.size() - 1);
    return ret;
}
Graph Graph::CompleteGraph(IdType num_nodes, Length length) {
    auto ret = Graph();
    for (auto i = IdType{0}; i < num_nodes; ++i) {
        ret.AddNode(i);
    }
    for (auto i = IdType{0}; i < num_nodes; ++i) {
        for (auto j = i + 1; j < num_nodes; ++j) {
            ret.AddEdge(i, j, length);
        }
    }
    return ret;
}
std::size_t Graph::NumNodes() const {
    return node_list_.size();
}
std::size_t Graph::NumEdges() const {
    return edge_list_.size();
}
bool Graph::HasNode(IdType id) const {
    return id2node_.contains(id);
}
const Graph::Node& Graph::GetNode(IdType id) const {
    return *id2node_.at(id);
}
const std::list<Graph::Node>& Graph::GetNodes() const {
    return node_list_;
}
bool Graph::HasEdge(IdType x, IdType y) const {
    return edge_list_.contains(GetEdgeKey(x, y));
}
const Graph::Edge& Graph::GetEdge(IdType x, IdType y) const {
    return edge_list_.at(GetEdgeKey(x, y));
}
const std::unordered_set<Graph::IdType>& Graph::GetNeighbors(IdType x) const {
    return GetNode(x).adj;
}
Graph::Length Graph::GetEdgeLength(IdType x, IdType y) const {
    return GetEdge(x, y).length;
}
const Graph::Node& Graph::AddNode(IdType id, Weight weight) {
    node_list_.emplace_back(Node{id, weight});
    id2node_[id] = --(node_list_.end());
    return *id2node_[id];
}
void Graph::SetNodeWeight(IdType id, Weight weight) {
    id2node_[id]->weight = weight;
}
const Graph::Edge& Graph::AddEdge(IdType x, IdType y, Length length) {
    auto nx = id2node_[x];
    auto ny = id2node_[y];
    nx->adj.insert(ny->id);
    ny->adj.insert(nx->id);
    return edge_list_[GetEdgeKey(x, y)] = Edge{length};
}
void Graph::SetEdgeLength(IdType x, IdType y, Length length) {
    edge_list_[GetEdgeKey(x, y)].length = length;
}
void Graph::DelNode(IdType id) {
    auto itr = id2node_[id];
    for (const auto& x : itr->adj) {
        id2node_[x]->adj.erase(id);
        edge_list_.erase(GetEdgeKey(id, x));
    }
    id2node_.erase(id);
    node_list_.erase(itr);
}
void Graph::DelEdge(IdType x, IdType y) {
    auto nx = id2node_[x];
    auto ny = id2node_[y];
    nx->adj.erase(ny->id);
    ny->adj.erase(nx->id);
    edge_list_.erase(GetEdgeKey(x, y));
}
std::unordered_map<Graph::IdType, Graph::IdType> Graph::MST() const {
    // key: child's id, value: parents' id
    auto ret = std::unordered_map<Graph::IdType, Graph::IdType>();
    ret.reserve(NumNodes());

    // Arrange the edges in ascending order of length.
    // Edges are adjacent to or inside the working tree of Prim's algorithm.
    using Value = std::pair<Length, std::pair<IdType, IdType>>;
    auto edges = std::priority_queue<Value, std::vector<Value>, std::greater<>>();

    const auto add_adj_edges = [this, &ret, &edges](IdType p) {
        for (const auto& c : GetNode(p).adj) {
            if (!ret.contains(c)) {
                edges.push({GetEdge(p, c).length, {p, c}});
            }
        }
    };

    // Set root
    {
        const auto& root = *node_list_.begin();
        ret[root.id] = root.id;
        add_adj_edges(root.id);
    }
    while (!edges.empty()) {
        const auto& [_, vp] = edges.top();
        const auto [p, c] = vp;
        edges.pop();
        if (!ret.contains(c)) {
            ret[c] = p;
            add_adj_edges(c);
        }
        if (ret.size() == NumNodes()) {
            break;
        }
    }

    return ret;
}
UnionFind::UnionFind(std::size_t n)
    : parent_(n)
    , size_(n, 1)
    , num_groups_(n) {
    std::iota(parent_.begin(), parent_.end(), 0);
}
std::size_t UnionFind::Leader(std::size_t a) {
    if (parent_[a] == a) {
        return a;
    }
    return parent_[a] = Leader(parent_[a]);
}
std::size_t UnionFind::Merge(std::size_t a, std::size_t b) {
    a = Leader(a), b = Leader(b);
    if (a == b) {
        return a;
    }

    num_groups_--;
    if (size_[a] < size_[b]) {
        std::swap(a, b);
    }
    size_[a] += size_[b];
    parent_[b] = a;
    return a;
}
bool UnionFind::Same(std::size_t a, std::size_t b) {
    return Leader(a) == Leader(b);
}
MinimumSteinerTreeSolver::MinimumSteinerTreeSolver(const Graph& graph)
    : graph_{graph} {}
std::optional<std::tuple<std::uint64_t, Graph>> MinimumSteinerTreeSolver::SolveByWu(
        const std::vector<Graph::IdType>& terminals
) {
    // Tuple of (distance from terminal, terminal idx, current idx, child idx).
    using QueueDataType = std::tuple<Graph::Length, std::size_t, Graph::IdType, Graph::IdType>;
    // Sort priority queue by distance from terminal.
    struct CompQueueData {
        bool operator()(const QueueDataType& lhs, const QueueDataType& rhs) const {
            if (terminals.contains(std::get<2>(lhs))) {  // if lhs is terminal
                return false;
            } else if (terminals.contains(std::get<2>(rhs))) {  // if rhs is terminal
                return true;
            } else {
                return std::get<0>(lhs) > std::get<0>(rhs);
            }
        }
        const std::unordered_set<Graph::IdType> terminals;
    };
    // Tuple of (nearest terminal, distance from nearest terminal, children of steiner tree).
    using NodeInfoType = std::tuple<std::size_t, std::size_t, std::vector<Graph::IdType>>;

    const auto num_terminals = terminals.size();

    auto que = std::priority_queue<QueueDataType, std::vector<QueueDataType>, CompQueueData>{
            CompQueueData{
                    .terminals =
                            std::unordered_set<Graph::IdType>(terminals.begin(), terminals.end())
            }
    };
    auto node_infos = std::unordered_map<Graph::IdType, NodeInfoType>{};
    auto uf = UnionFind(num_terminals);
    auto intersections = std::vector<Graph::IdType>{};

    // Initialize que and node_infos.
    for (auto terminal_idx = std::size_t{0}; terminal_idx < num_terminals; ++terminal_idx) {
        que.emplace(0, terminal_idx, terminals[terminal_idx], terminals[terminal_idx]);
        node_infos[terminals[terminal_idx]] = {terminal_idx, 0, {}};
    }

    // Starting from terminals, expand tree like Kruskal's algorithm when the search encounters a
    // node that beglongs to another tree, merge them the nodes which connect trees are added to
    // intersections.
    while (!que.empty() && uf.NumGroups() != 1) {
        const auto [dist, terminal_idx, current_idx, prev_idx] = que.top();
        que.pop();

        if (!uf.Same(terminal_idx, std::get<0>(node_infos[current_idx]))) {
            auto& children = std::get<2>(node_infos[current_idx]);
            if (children.size() == 1) {
                intersections.push_back(current_idx);
            }
            uf.Merge(terminal_idx, std::get<0>(node_infos[current_idx]));
            children.push_back(prev_idx);
        }

        // If path from terminal_idx to current_idx is the shortest path, search neighbors.
        const auto min_dist = std::get<1>(node_infos[current_idx]);
        if (min_dist < dist) {
            continue;
        }

        // Search neighbors of current_idx.
        for (const auto& next_idx : graph_.GetNeighbors(current_idx)) {
            const auto next_dist = dist + graph_.GetEdgeLength(current_idx, next_idx);
            const auto itr = node_infos.find(next_idx);
            if (itr == node_infos.end()) {
                node_infos[next_idx] = {terminal_idx, next_dist, {current_idx}};
                que.emplace(next_dist, terminal_idx, next_idx, current_idx);
            } else {
                const auto& [nearest_terminal, min_next_dist, children] = itr->second;
                if (next_dist < min_next_dist) {
                    node_infos[next_idx] = {terminal_idx, next_dist, {current_idx}};
                    que.emplace(next_dist, terminal_idx, next_idx, current_idx);
                } else if (!uf.Same(terminal_idx, std::get<0>(node_infos[next_idx]))) {
                    que.emplace(next_dist, terminal_idx, next_idx, current_idx);
                }
            }
        }
    }

    // Steiner tree does not exist as the terminals are not all connected.
    for (auto terminal_idx = std::size_t{1}; terminal_idx < num_terminals; ++terminal_idx) {
        if (!uf.Same(0, terminal_idx)) {
            return std::nullopt;
        }
    }

    // Construct steiner tree.
    // Starting from intersections or terminals, search nodes and edges in steiner tree using DFS
    // and add them to tree.
    auto cost = std::uint64_t{0};
    auto tree = Graph();
    const auto construct_tree = [this, &cost, &tree, &node_infos](const Graph::IdType st) -> void {
        if (tree.HasNode(st)) {
            return;
        }
        auto tree_stk = std::stack<Graph::IdType>{};
        tree_stk.push(st);
        tree.AddNode(st, graph_.GetNode(st).weight);
        while (!tree_stk.empty()) {
            const auto p = tree_stk.top();
            tree_stk.pop();
            for (auto next : std::get<2>(node_infos[p])) {
                if (!tree.HasNode(next)) {
                    tree_stk.push(next);
                    tree.AddNode(next, graph_.GetNode(next).weight);
                }
                tree.AddEdge(p, next, graph_.GetEdgeLength(p, next));
                cost += graph_.GetEdgeLength(p, next);
            }
        }
    };
    for (const auto& st : intersections) {
        construct_tree(st);
    }
    for (const auto& st : terminals) {
        construct_tree(st);
    }

    return std::optional<std::tuple<std::uint64_t, Graph>>({cost, std::move(tree)});
}
}  // namespace qret
