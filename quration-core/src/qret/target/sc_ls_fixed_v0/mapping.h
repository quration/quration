/**
 * @file qret/target/sc_ls_fixed_v0/mapping.h
 * @brief Mapping.
 */

#ifndef QRET_TARGET_SC_LS_FIXED_V0_MAPPING_H
#define QRET_TARGET_SC_LS_FIXED_V0_MAPPING_H

#include <unordered_map>
#include <vector>

#include "qret/base/graph.h"
#include "qret/codegen/machine_function.h"
#include "qret/codegen/machine_function_pass.h"
#include "qret/qret_export.h"
#include "qret/target/sc_ls_fixed_v0/symbol.h"
#include "qret/target/sc_ls_fixed_v0/topology.h"

namespace qret::sc_ls_fixed_v0 {
struct QRET_EXPORT Mapping : public MachineFunctionPass {
    static inline char ID = 0;
    Mapping()
        : MachineFunctionPass(&ID) {}

    bool RunOnMachineFunction(MachineFunction& mf) override;

private:
    bool MapBasedOnTopologyFile(MachineFunction& mf);
};

class QRET_EXPORT QubitGraph {
public:
    enum class PartitionAlgorithm : std::uint8_t {
        Greedy = 0,
        Random = 1,
        METIS = 2,
    };

    explicit QubitGraph(const MachineFunction& mf);

    const Graph& GetGraph() const {
        return graph_;
    }
    std::size_t NumQubits() const {
        return num_qubits_;
    }

    /**
     * @brief Split qubit graph into subgraphs.
     *
     * @param max_weights maximum weight of each subgraph.
     * @param algorithm
     * @return true if graph is successfully partitioned.
     * @return false otherwise.
     */
    [[nodiscard]] bool
    Partition(const std::vector<std::size_t>& max_weights, PartitionAlgorithm algorithm);
    [[nodiscard]] bool PartitionByGreedy(const std::vector<std::size_t>& max_weights);
    [[nodiscard]] bool
    PartitionByRandom(std::vector<std::size_t> max_weights, std::uint64_t seed = 314);
    [[nodiscard]] bool PartitionByMETIS(const std::vector<std::size_t>& max_weights);

    auto partition_begin() const {
        return partition_.begin();
    }
    auto partition_end() const {
        return partition_.end();
    }

    auto pair_begin() const {
        return pairs_.begin();
    }
    auto pair_end() const {
        return pairs_.end();
    }

private:
    Graph graph_;
    std::unordered_map<QSymbol, QSymbol> pairs_ = {};

    std::size_t num_qubits_ = 0;
    std::vector<std::vector<QSymbol>> partition_ = {};
};

class QubitMapper {
public:
    using PartitionAlgorithm = QubitGraph::PartitionAlgorithm;
    enum class FindPlaceAlgorithm : std::uint8_t {
        EnoughSpaceSoft = 0,  //!< allow boundary
        EnoughSpaceHard = 1,  //!< place around qubit is free ancilla
        // Square
    };

    explicit QubitMapper(
            const std::shared_ptr<const Topology>& topology,
            FindPlaceAlgorithm algorithm = FindPlaceAlgorithm::EnoughSpaceSoft
    );

    const Coord3D& GetPlace(QSymbol q) const {
        return map_.at(q);
    }

    /**
     * @brief Map qubits to place
     *
     * @param graph qubit graph
     * @param algorithm
     * @return true if mapping is succeeded.
     * @return false otherwise.
     */
    bool MapQubits(QubitGraph& graph, PartitionAlgorithm algorithm = PartitionAlgorithm::Greedy);

    auto place_begin(std::int32_t z) const {
        return place_.at(z).begin();
    }
    auto place_end(std::int32_t z) const {
        return place_.at(z).end();
    }

    auto map_begin() const {
        return map_.begin();
    }
    auto map_end() const {
        return map_.end();
    }

private:
    void FindPlace(const ScLsPlane& plane, FindPlaceAlgorithm algorithm);

    std::shared_ptr<const Topology> topology_;

    std::unordered_map<std::int32_t, std::unordered_set<Coord3D>> place_;  //!< z to graphs

    std::unordered_map<QSymbol, Coord3D> map_;
};
}  // namespace qret::sc_ls_fixed_v0

#endif  // QRET_TARGET_SC_LS_FIXED_V0_MAPPING_H
