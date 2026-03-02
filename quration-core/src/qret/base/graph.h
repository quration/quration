/**
 * @file qret/base/graph.h
 * @brief Basic graph classes and functions.
 */

#ifndef QRET_BASE_GRAPH_H
#define QRET_BASE_GRAPH_H

#include <cstddef>
#include <cstdint>
#include <list>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "qret/qret_export.h"

namespace qret {
/**
 * @brief Directed graph.
 * @details Edges point from parent to child.
 */
class QRET_EXPORT DiGraph {
public:
    using IdType = std::uint32_t;
    using Weight = std::uint64_t;
    using Length = std::uint64_t;
    struct Node {
        IdType id = 0;
        Weight weight = 0;
        std::unordered_set<IdType> parent = {};
        std::unordered_set<IdType> child = {};

        bool HasParent() const {
            return !parent.empty();
        }
        bool HasChild() const {
            return !child.empty();
        }
    };
    struct Edge {
        Length length = 0;
    };
    using Container = std::list<Node>;
    using Iterator = typename Container::iterator;
    using ConstIterator = typename Container::const_iterator;
    using ConstReverseIterator = typename Container::const_reverse_iterator;

    DiGraph() = default;
    DiGraph(DiGraph&&) noexcept = default;
    DiGraph(const DiGraph&) = delete;
    virtual ~DiGraph() = default;
    DiGraph& operator=(const DiGraph&) = delete;
    DiGraph& operator=(DiGraph&&) = default;

    [[nodiscard]] std::size_t NumNodes() const;
    [[nodiscard]] std::size_t NumEdges() const;
    [[nodiscard]] bool HasNode(IdType id) const;
    [[nodiscard]] const Node& GetNode(IdType id) const;
    [[nodiscard]] bool HasEdge(IdType from, IdType to) const;
    [[nodiscard]] const Edge& GetEdge(IdType from, IdType to) const;

    const Node& AddNode(IdType id, Weight weight = 0);
    void SetNodeWeight(IdType id, Weight weight) {
        auto node = id2node_.at(id);
        node->weight = weight;
    }
    const Edge& AddEdge(IdType from, IdType to, Length length = 0);
    void SetEdgeLength(IdType from, IdType to, Length length) {
        const auto key = GetEdgeKey(from, to);
        edge_list_.at(key).length = length;
    }
    void DelNode(IdType id);
    void DelEdge(IdType from, IdType to);

    /**
     * @brief If the digraph is  DAG, sort the nodes in topological order.
     *
     * @return true the digraph is DAG
     * @return false the digraph is not DAG
     */
    bool Topsort();
    /**
     * @brief Returns 1 if the graph is DAG.
     * @details This method return 0 or 1 only after calling `Topsort()` otherwise returns -1.
     *
     * @return std::int32_t 1 -> DAG, 0 -> not DAG, -1 -> unknown
     */
    std::int32_t IsDAG() const {
        if (!is_dag_.has_value()) {
            return -1;
        }
        return *is_dag_ ? 1 : 0;
    }
    /**
     * @brief Get the depth of node.
     * @details This method is valid only if the graph is DAG and after calling `Topsort()`.
     *
     * @param id node id
     * @return std::uint32_t depth (0 if node is root)
     */
    std::uint32_t GetDepth(IdType id) const {
        if (!is_dag_.has_value() || !(*is_dag_)) {
            throw std::runtime_error("call Topsort() before calling GetDepth().");
        }
        return depth_.at(id);
    }

    [[nodiscard]] ConstIterator begin() const {
        return node_list_.begin();
    }  // NOLINT
    [[nodiscard]] ConstIterator end() const {
        return node_list_.end();
    }  // NOLINT
    [[nodiscard]] ConstReverseIterator rbegin() const {
        return node_list_.rbegin();
    }  // NOLINT
    [[nodiscard]] ConstReverseIterator rend() const {
        return node_list_.rend();
    }  // NOLINT

private:
    [[nodiscard]] std::uint64_t GetEdgeKey(IdType from, IdType to) const {
        return (static_cast<std::uint64_t>(from) << 32) + static_cast<std::uint64_t>(to);
    }
    void UpdateGraph() {
        is_dag_ = std::nullopt;
        depth_.clear();
    }

    std::list<Node> node_list_ = {};
    std::unordered_map<IdType, Iterator> id2node_ = {};
    std::unordered_map<std::uint64_t, Edge> edge_list_ =
            {};  //!< map of edges (key is tuple of (from, to))

    // The following two fields are valid only after calling `Topsort()`.
    // When graph is updated, these are cleared by `UpdateGraph()`.
    std::optional<bool> is_dag_;  //!< true if graph is dag
    std::unordered_map<IdType, std::uint32_t> depth_;  //!< depth of node
};

/**
 * @brief Check whether directed graph is DAG or not.
 * @note Complexity: O(V+E).
 *
 * @param dig directed graph
 * @return true directed graph is DAG
 * @return false directed graph is not DAG
 */
QRET_EXPORT bool IsDAG(const DiGraph& dig);

/**
 * @brief Find the heaviest path of DAG.
 * @details Use BFS to find the heaviest path.
 * The weight of a path is defined as the sum of the weights of the nodes.
 *
 * @param dag DAG
 * @return std::tuple<DiGraph::Weight, std::vector<DiGraph::IdType>,
 * std::unordered_map<DiGraph::IdType, DiGraph::Weight>> Tuple of (the heaviest weight, the heaviest
 * path, weight to each node from root).
 */
QRET_EXPORT std::tuple<
        DiGraph::Weight,
        std::vector<DiGraph::IdType>,
        std::unordered_map<DiGraph::IdType, DiGraph::Weight>>
FindHeaviestPath(const DiGraph& dag);

/**
 * @brief Find the longest path of DAG.
 * @details Use BFS to find the longest path.
 * The length of a path is defined as the sum of the lengths of the edges.
 *
 * @param dag DAG
 * @return std::tuple<DiGraph::Length, std::vector<DiGraph::IdType>,
 * std::unordered_map<DiGraph::IdType, DiGraph::Length>> Tuple of (the longest length, the longest
 * path, length to each node from root).
 */
QRET_EXPORT std::tuple<
        DiGraph::Length,
        std::vector<DiGraph::IdType>,
        std::unordered_map<DiGraph::IdType, DiGraph::Length>>
FindLongestPath(const DiGraph& dag);

/**
 * @brief Export directed graph in DOT format.
 *
 * @param name name of graph
 * @param dig directed graph
 * @param hook change node name if id is in the hook map
 * @return std::string DOT format of directed graph
 */
QRET_EXPORT std::string ExportDiGraphToDOT(
        const std::string& name,
        const DiGraph& dig,
        const std::unordered_map<DiGraph::IdType, std::string>& hook = {}
);

/**
 * @brief Undirected graph.
 */
class QRET_EXPORT Graph {
public:
    using IdType = std::int32_t;
    using Weight = std::uint64_t;
    using Length = std::uint64_t;
    struct Node {
        IdType id = 0;
        Weight weight = 0;
        std::unordered_set<IdType> adj = {};
    };
    struct Edge {
        Length length = 0;
    };
    using Container = std::list<Node>;
    using Iterator = typename Container::iterator;
    using ConstIterator = typename Container::const_iterator;
    using ConstReverseIterator = typename Container::const_reverse_iterator;

    Graph() = default;
    Graph(Graph&&) = default;
    Graph(const Graph&) = delete;
    virtual ~Graph() = default;
    Graph& operator=(const Graph&) = delete;
    Graph& operator=(Graph&&) = default;

    static Graph CompleteGraph(IdType num_nodes, Length length = 1);

    [[nodiscard]] std::size_t NumNodes() const;
    [[nodiscard]] std::size_t NumEdges() const;
    [[nodiscard]] bool HasNode(IdType id) const;
    [[nodiscard]] const Node& GetNode(IdType id) const;
    [[nodiscard]] const std::list<Node>& GetNodes() const;
    [[nodiscard]] bool HasEdge(IdType x, IdType y) const;
    [[nodiscard]] const Edge& GetEdge(IdType x, IdType y) const;
    [[nodiscard]] const std::unordered_set<IdType>& GetNeighbors(IdType x) const;
    [[nodiscard]] Length GetEdgeLength(IdType x, IdType y) const;

    const Node& AddNode(IdType id, Weight weight = 0);
    void SetNodeWeight(IdType id, Weight weight);
    const Edge& AddEdge(IdType x, IdType y, Length length = 0);
    void SetEdgeLength(IdType x, IdType y, Length length);
    void DelNode(IdType id);
    void DelEdge(IdType x, IdType y);

    /**
     * @brief Calculate minimum spanning tree.
     * @details Implement Prim's algorithm.
     * The tree is represented by its own parent's ID. The parent of a root is itself.
     *
     * @return std::unordered_map<IdType, IdType> key: child's id, value: parent's id
     */
    std::unordered_map<IdType, IdType> MST() const;

    [[nodiscard]] ConstIterator begin() const {
        return node_list_.begin();
    }  // NOLINT
    [[nodiscard]] ConstIterator end() const {
        return node_list_.end();
    }  // NOLINT
    [[nodiscard]] ConstReverseIterator rbegin() const {
        return node_list_.rbegin();
    }  // NOLINT
    [[nodiscard]] ConstReverseIterator rend() const {
        return node_list_.rend();
    }  // NOLINT

private:
    [[nodiscard]] std::uint64_t GetEdgeKey(IdType x, IdType y) const {
        if (x < y) {
            std::swap(x, y);
        }
        // assert(x >= y);
        auto key = std::uint64_t{0};
        std::copy(&x, &x + 1, reinterpret_cast<IdType*>(&key));
        std::copy(
                &y,
                &y + 1,
                reinterpret_cast<IdType*>(reinterpret_cast<char*>(&key) + sizeof(IdType))
        );
        return key;
    }
    [[nodiscard]] std::pair<IdType, IdType> SplitEdgeKey(std::uint64_t key) const {
        auto x = IdType{0};
        auto y = IdType{0};
        std::copy(reinterpret_cast<IdType*>(&key), reinterpret_cast<IdType*>(&key) + 1, &x);
        std::copy(
                reinterpret_cast<IdType*>(reinterpret_cast<char*>(&key) + sizeof(IdType)),
                reinterpret_cast<IdType*>(
                        reinterpret_cast<char*>(&key) + sizeof(IdType) + sizeof(IdType)
                ),
                &y
        );
        if (x < y) {
            std::swap(x, y);
        }
        return {x, y};
    }

    std::list<Node> node_list_ = {};
    std::unordered_map<IdType, Iterator> id2node_ = {};
    std::unordered_map<std::uint64_t, Edge> edge_list_ =
            {};  // //!< map of edges (key is tuple of (from, to))
};

class QRET_EXPORT UnionFind {
public:
    explicit UnionFind(std::size_t n);
    std::size_t Leader(std::size_t a);
    std::size_t Merge(std::size_t a, std::size_t b);
    bool Same(std::size_t a, std::size_t b);
    std::size_t NumGroups() const {
        return num_groups_;
    }

private:
    std::vector<std::size_t> parent_, size_;
    std::size_t num_groups_;
};

/**
 * @brief Search minimum steiner tree.
 */
class QRET_EXPORT MinimumSteinerTreeSolver {
public:
    enum class Algorithm : std::uint8_t {
        Wu,
        Kou,  // Not implemented
    };

    explicit MinimumSteinerTreeSolver(const Graph& graph);

    std::optional<std::tuple<std::uint64_t, Graph>>
    Solve(const std::vector<Graph::IdType>& terminals, Algorithm algorithm);

    /**
     * @brief Solve the minimum steiner tree problem using Wu et al.'s approach.
     *
     * @details For more information, see: Wu, Widmayer and Wong. "A faster approximation algorithm
     * for the Steiner problem in graphs" Acta informatica 23 (1986): 223-229.
     *
     * @param terminals terminals of steiner tree.
     * @return std::optional<std::tuple<std::uint64_t, Graph>> tuple of (1st: cost of the steiner
     * tree, 2nd: the steiner tree) if steiner tree exists, otherwise std::nullopt.
     */
    std::optional<std::tuple<std::uint64_t, Graph>> SolveByWu(
            const std::vector<Graph::IdType>& terminals
    );

private:
    const Graph& graph_;
};
}  // namespace qret

#endif  // QRET_BASE_GRAPH_H
