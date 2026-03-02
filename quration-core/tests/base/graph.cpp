#include "qret/base/graph.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <numeric>
#include <stdexcept>

using namespace qret;

DiGraph CreateDAG() {
    // Graph from https://en.wikipedia.org/wiki/Topological_sorting
    auto dig = DiGraph();
    dig.AddNode(2, 1);
    dig.AddNode(3, 1);
    dig.AddNode(5, 1);
    dig.AddNode(7, 1);
    dig.AddNode(8, 1);
    dig.AddNode(9, 1);
    dig.AddNode(10, 1);
    dig.AddNode(11, 1);
    dig.AddEdge(3, 8);
    dig.AddEdge(3, 10);
    dig.AddEdge(5, 11);
    dig.AddEdge(7, 8);
    dig.AddEdge(7, 11);
    dig.AddEdge(8, 9);
    dig.AddEdge(11, 2);
    dig.AddEdge(11, 9);
    dig.AddEdge(11, 10);
    return dig;
}
DiGraph CreateCyclicGraph() {
    auto dig = DiGraph();
    dig.AddNode(1);
    dig.AddNode(2);
    dig.AddNode(3);
    dig.AddEdge(1, 2);
    dig.AddEdge(2, 3);
    dig.AddEdge(3, 1);
    return dig;
}
Graph CreateGraph() {
    // Graph from https://en.wikipedia.org/wiki/Minimum_spanning_tree
    auto g = Graph();
    g.AddNode(1);
    g.AddNode(2);
    g.AddNode(3);
    g.AddNode(4);
    g.AddNode(5);
    g.AddNode(6);
    g.AddEdge(1, 2, 1);
    g.AddEdge(1, 4, 4);
    g.AddEdge(1, 5, 3);
    g.AddEdge(2, 4, 4);
    g.AddEdge(2, 5, 2);
    g.AddEdge(3, 5, 4);
    g.AddEdge(3, 6, 5);
    g.AddEdge(4, 5, 4);
    g.AddEdge(5, 6, 7);
    return g;
}
Graph CreateDisconnectedGraph() {
    // {1,2,3,4}, {5,6,7,8} are connected
    auto g = Graph();
    g.AddNode(1);
    g.AddNode(2);
    g.AddNode(3);
    g.AddNode(4);
    g.AddNode(5);
    g.AddNode(6);
    g.AddNode(7);
    g.AddNode(8);
    g.AddEdge(1, 2, 1);
    g.AddEdge(2, 3, 1);
    g.AddEdge(1, 3, 1);
    g.AddEdge(3, 4, 1);
    g.AddEdge(5, 6, 1);
    g.AddEdge(6, 7, 1);
    g.AddEdge(7, 8, 1);
    g.AddEdge(8, 5, 1);
    return g;
}
Graph CreatePerfectGraph(std::uint32_t n) {
    auto g = Graph();
    for (auto id = 0u; id < n; ++id) {
        g.AddNode(id);
    }
    for (auto id1 = 0u; id1 < n; ++id1) {
        for (auto id2 = id1 + 1; id2 < n; ++id2) {
            g.AddEdge(id1, id2, 1);
        }
    }
    return g;
}

void CheckSteinerTree(
        const Graph& tree,
        const Graph& g,
        const std::vector<Graph::IdType>& terminals
) {
    std::size_t max_node_idx = 1;
    for (const auto& node : g.GetNodes()) {
        max_node_idx = std::max(max_node_idx, static_cast<std::size_t>(node.id) + 1);
    }
    UnionFind d(max_node_idx);
    std::unordered_set<Graph::IdType> nodes_in_tree;
    for (const auto& node : tree.GetNodes()) {
        EXPECT_TRUE(g.HasNode(node.id));
        nodes_in_tree.insert(node.id);
        for (const auto& neighbor : tree.GetNeighbors(node.id)) {
            EXPECT_TRUE(g.HasEdge(node.id, neighbor));
            d.Merge(node.id, neighbor);
        }
    }
    for (const auto& node : tree.GetNodes()) {
        EXPECT_TRUE(d.Same(node.id, terminals[0]));
    }
    for (const auto& terminal : terminals) {
        EXPECT_TRUE(nodes_in_tree.contains(terminal));
    }
    EXPECT_TRUE(tree.NumNodes() == tree.NumEdges() + 1 || tree.NumNodes() == 0);
}
TEST(DiGraph, Basic) {
    const auto dig = CreateDAG();
    EXPECT_EQ(8, dig.NumNodes());
    EXPECT_EQ(9, dig.NumEdges());
    EXPECT_TRUE(dig.HasNode(3));
    EXPECT_FALSE(dig.HasNode(1));
    EXPECT_TRUE(dig.HasEdge(3, 10));
    EXPECT_FALSE(dig.HasEdge(10, 3));
    EXPECT_FALSE(dig.HasEdge(0, 1));
    EXPECT_EQ(3, dig.GetNode(3).id);
    EXPECT_EQ(0, dig.GetEdge(8, 9).length);

    std::cout << ExportDiGraphToDOT(
            "test",
            dig,
            std::unordered_map<DiGraph::IdType, std::string>{
                    {5, std::string{"start"}},
                    {10, std::string{"end"}}
            }
    ) << std::endl;
}
TEST(DiGraph, Topsort) {
    auto dig = CreateDAG();
    EXPECT_THROW(dig.GetDepth(5), std::runtime_error);

    EXPECT_TRUE(dig.Topsort());
    EXPECT_TRUE(IsDAG(dig));
    for (const auto& node : dig) {
        std::cout << node.id << ", " << node.weight << std::endl;
    }

    EXPECT_EQ(0, dig.GetDepth(5));
    EXPECT_EQ(0, dig.GetDepth(7));
    EXPECT_EQ(0, dig.GetDepth(3));
    EXPECT_EQ(1, dig.GetDepth(11));
    EXPECT_EQ(1, dig.GetDepth(8));
    EXPECT_EQ(2, dig.GetDepth(2));
    EXPECT_EQ(2, dig.GetDepth(9));
    EXPECT_EQ(2, dig.GetDepth(10));
}
TEST(DiGraph, TopSortFailure) {
    auto dig = CreateCyclicGraph();
    EXPECT_FALSE(dig.Topsort());
    EXPECT_FALSE(IsDAG(dig));
}
TEST(DiGraph, FindHeaviestPath) {
    auto dig = CreateDAG();
    EXPECT_TRUE(dig.Topsort());
    EXPECT_TRUE(IsDAG(dig));
    const auto [heaviest_weight, path, mp] = FindHeaviestPath(dig);
    std::cout << heaviest_weight << std::endl;
    for (const auto& node : dig) {
        std::cout << node.id << " : " << mp.at(node.id) << std::endl;
    }
    EXPECT_EQ(3, heaviest_weight);
}
TEST(Graph, MST) {
    auto g = CreateGraph();
    const auto mst = g.MST();
    auto sum = Graph::Length{0};
    for (const auto& [child, parent] : mst) {
        if (child != parent) {
            sum += g.GetEdge(child, parent).length;
        }
        std::cout << child << " ---> " << parent << std::endl;
    }
    EXPECT_EQ(16, sum);
}
TEST(UnionFind, Normal) {
    std::size_t n = 5;
    auto d = UnionFind(n);
    for (std::size_t i = 0; i < n; i++) {
        for (std::size_t j = 0; j < n; j++) {
            if (i == j) {
                EXPECT_TRUE(d.Same(i, j));
            } else {
                EXPECT_FALSE(d.Same(i, j));
            }
        }
    }
    d.Merge(0, 1);
    d.Merge(2, 1);
    d.Merge(3, 4);
    EXPECT_TRUE(d.Same(0, 2));
    EXPECT_TRUE(d.Same(3, 4));
    EXPECT_FALSE(d.Same(0, 4));
}
TEST(MinimumSteinerTree, ZeroNode) {
    auto g = Graph();
    auto solver = MinimumSteinerTreeSolver(g);
    const auto terminals = std::vector<Graph::IdType>{};
    const auto result = solver.SolveByWu(terminals);
    ASSERT_TRUE(result.has_value());
    const auto& [cost, tree] = *result;
    EXPECT_EQ(cost, 0);
    CheckSteinerTree(tree, g, terminals);
    EXPECT_EQ(tree.GetNodes().size(), 0);
}
TEST(MinimumSteinerTree, OneNode) {
    auto g = Graph::CompleteGraph(1);
    auto solver = MinimumSteinerTreeSolver(g);
    const auto terminals = std::vector<Graph::IdType>{0};
    const auto result = solver.SolveByWu(terminals);
    ASSERT_TRUE(result.has_value());
    const auto& [cost, tree] = *result;
    EXPECT_EQ(cost, 0);
    CheckSteinerTree(tree, g, terminals);
    EXPECT_EQ(tree.GetNodes().size(), 1);
    EXPECT_TRUE(tree.HasNode(0));
}
TEST(MinimumSteinerTree, TwoNodes) {
    auto g = Graph::CompleteGraph(2);
    auto solver = MinimumSteinerTreeSolver(g);
    const auto terminals = std::vector<Graph::IdType>{0, 1};
    const auto result = solver.SolveByWu(terminals);
    ASSERT_TRUE(result.has_value());
    const auto& [cost, tree] = *result;
    EXPECT_EQ(cost, 1);
    CheckSteinerTree(tree, g, terminals);
    EXPECT_EQ(tree.GetNodes().size(), 2);
}
TEST(MinimumSteinerTree, TwoNodesDisconnected) {
    auto g = Graph();
    g.AddNode(0);
    g.AddNode(1);
    auto solver = MinimumSteinerTreeSolver(g);
    const auto terminals = std::vector<Graph::IdType>{0, 1};
    const auto result = solver.SolveByWu(terminals);
    ASSERT_FALSE(result.has_value());
}
TEST(MinimumSteinerTree, NoTerminals) {
    const auto g = CreateGraph();
    auto solver = MinimumSteinerTreeSolver(g);
    const auto terminals = std::vector<Graph::IdType>{};
    const auto result = solver.SolveByWu(terminals);
    ASSERT_TRUE(result.has_value());
    const auto& [cost, tree] = *result;
    EXPECT_EQ(cost, 0);
    CheckSteinerTree(tree, g, terminals);
    EXPECT_EQ(tree.GetNodes().size(), 0);
}
TEST(MinimumSteinerTree, OneTerminal) {
    const auto g = CreateGraph();
    auto solver = MinimumSteinerTreeSolver(g);
    const auto terminals = std::vector<Graph::IdType>{3};
    const auto result = solver.SolveByWu(terminals);
    ASSERT_TRUE(result.has_value());
    const auto& [cost, tree] = *result;
    EXPECT_EQ(cost, 0);
    CheckSteinerTree(tree, g, terminals);
    EXPECT_EQ(tree.GetNodes().size(), 1);
}
TEST(MinimumSteinerTree, AllTerminal) {
    const auto g = CreateGraph();
    auto solver = MinimumSteinerTreeSolver(g);
    auto terminals = std::vector<Graph::IdType>(g.NumNodes());
    std::iota(terminals.begin(), terminals.end(), 1);
    const auto result = solver.SolveByWu(terminals);
    ASSERT_TRUE(result.has_value());
    const auto& [cost, tree] = *result;
    EXPECT_GT(cost, 0);  // cost > 0
    CheckSteinerTree(tree, g, terminals);
    EXPECT_EQ(tree.GetNodes().size(), g.NumNodes());
}
TEST(MinimumSteinerTree, ZeroWeight) {
    auto g = Graph();
    g.AddNode(1);
    g.AddNode(2);
    g.AddNode(3);
    g.AddNode(4);
    g.AddNode(5);
    g.AddNode(6);
    g.AddEdge(1, 2);
    g.AddEdge(1, 4);
    g.AddEdge(1, 5);
    g.AddEdge(2, 4);
    g.AddEdge(2, 5);
    g.AddEdge(3, 5);
    g.AddEdge(3, 6);
    g.AddEdge(4, 5);
    g.AddEdge(5, 6);
    auto solver = MinimumSteinerTreeSolver(g);
    const auto terminals = std::vector<Graph::IdType>{3, 5};
    const auto result = solver.SolveByWu(terminals);
    ASSERT_TRUE(result.has_value());
    const auto& [cost, tree] = *result;
    EXPECT_EQ(cost, 0);
    CheckSteinerTree(tree, g, terminals);
}
TEST(MinimumSteinerTree, Complicated) {
    auto g = Graph();
    for (std::size_t i = 0; i < 7; i++) {
        g.AddNode(i);
    }
    g.AddEdge(0, 1, 6);
    g.AddEdge(0, 2, 5);
    g.AddEdge(0, 3, 5);
    g.AddEdge(1, 4, 2);
    g.AddEdge(1, 5, 3);
    g.AddEdge(2, 5, 2);
    g.AddEdge(3, 5, 3);
    g.AddEdge(3, 6, 4);
    g.AddEdge(4, 5, 4);
    g.AddEdge(5, 6, 2);
    g.AddEdge(4, 6, 13);
    auto solver = MinimumSteinerTreeSolver(g);
    const auto terminals = std::vector<Graph::IdType>{1, 3, 4, 6};
    const auto result = solver.SolveByWu(terminals);
    ASSERT_TRUE(result.has_value());
    const auto& [cost, tree] = *result;
    EXPECT_GT(cost, 0);  // cost > 0
    CheckSteinerTree(tree, g, terminals);
}
TEST(MinimumSteinerTree, PerfectGraph) {
    auto g = CreatePerfectGraph(4);
    auto solver = MinimumSteinerTreeSolver(g);
    for (const auto& terminals :
         {std::vector<Graph::IdType>{0, 1},
          std::vector<Graph::IdType>{0, 2},
          std::vector<Graph::IdType>{0, 3},
          std::vector<Graph::IdType>{1, 2},
          std::vector<Graph::IdType>{1, 3},
          std::vector<Graph::IdType>{2, 3}}) {
        const auto result = solver.SolveByWu(terminals);
        ASSERT_TRUE(result.has_value());
        const auto& [cost, tree] = *result;
        EXPECT_EQ(cost, 1);
        CheckSteinerTree(tree, g, terminals);
    }
}
TEST(MinimumSteinerTree, PerfectGraphGeneral) {
    auto g = CreatePerfectGraph(10);
    auto solver = MinimumSteinerTreeSolver(g);
    for (const auto& terminals :
         {std::vector<Graph::IdType>{0, 1, 2},
          std::vector<Graph::IdType>{0, 2, 4},
          std::vector<Graph::IdType>{0, 3, 6, 9},
          std::vector<Graph::IdType>{1, 2, 3, 4, 5}}) {
        const auto num_terminals = terminals.size();
        const auto result = solver.SolveByWu(terminals);
        ASSERT_TRUE(result.has_value());
        const auto& [cost, tree] = *result;
        EXPECT_EQ(cost, num_terminals - 1);
        CheckSteinerTree(tree, g, terminals);
    }
}
TEST(MinimumSteinerTree, DisconnectedGraph) {
    auto g = CreateDisconnectedGraph();
    auto solver = MinimumSteinerTreeSolver(g);
    const auto terminals = std::vector<Graph::IdType>{1, 3};
    const auto result = solver.SolveByWu(terminals);
    ASSERT_TRUE(result.has_value());
    const auto& [cost, tree] = *result;
    EXPECT_GT(cost, 0);  // cost > 0
    CheckSteinerTree(tree, g, terminals);
}
TEST(MinimumSteinerTree, DisconnectedGraphImpossible) {
    auto g = CreateDisconnectedGraph();
    auto solver = MinimumSteinerTreeSolver(g);
    const auto terminals = std::vector<Graph::IdType>{1, 3, 5};
    const auto result = solver.SolveByWu(terminals);
    ASSERT_FALSE(result.has_value());
}
TEST(MinimumSteinerTree, NonexistentTerminal) {
    auto g = CreateGraph();
    auto solver = MinimumSteinerTreeSolver(g);
    const auto terminals = std::vector<Graph::IdType>{0, 1, 2};
    EXPECT_THROW(solver.SolveByWu(terminals), std::out_of_range);
}
