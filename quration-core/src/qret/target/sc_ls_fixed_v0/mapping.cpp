/**
 * @file qret/target/sc_ls_fixed_v0/mapping.cpp
 * @brief Mapping.
 */

#include "qret/target/sc_ls_fixed_v0/mapping.h"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <cstdint>
#include <list>
#include <memory>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "qret/base/cast.h"
#include "qret/base/graph.h"
#include "qret/base/log.h"
#include "qret/base/option.h"
#include "qret/codegen/machine_function.h"
#include "qret/target/sc_ls_fixed_v0/instruction.h"
#include "qret/target/sc_ls_fixed_v0/sc_ls_fixed_v0_target_machine.h"
#include "qret/target/sc_ls_fixed_v0/symbol.h"
#include "qret/target/sc_ls_fixed_v0/topology.h"

namespace qret::sc_ls_fixed_v0 {
namespace {
// Register pass.
static auto X = RegisterPass<Mapping>("Mapping", "sc_ls_fixed_v0::mapping");

// Options.
static const auto ScLsMappingAlgorithm = Opt<std::uint32_t>(
        "sc_ls_fixed_v0-mapping-algorithm",
        1,
        "Mapping algorithm (0: Map based on topology file, 1: Auto)",
        OptionHidden::Hidden
);
static auto ScLsPartitionAlgorithm = Opt<std::uint32_t>(
        "sc_ls_fixed_v0-partition-algorithm",
        0,
        "Partition algorithm of mapping (0: Greedy, 1: Random, 2: METIS)",
        OptionHidden::Hidden
);
static auto ScLsFindPlaceAlgorithm = Opt<std::uint32_t>(
        "sc_ls_fixed_v0-find-place-algorithm",
        0,
        "Find place algorithm of mapping (0: EnoughSpaceSoft, 1: EnoughSpaceHard)",
        OptionHidden::Hidden
);
}  // namespace

void UpdateEdges(
        Graph& g,
        const std::list<QSymbol>& qs,
        const std::unordered_map<QSymbol, QSymbol>& pairs,
        Graph::Length len
) {
    for (auto lhs = qs.begin(); lhs != qs.end(); ++lhs) {
        for (auto rhs = std::next(lhs); rhs != qs.end(); ++rhs) {
            const auto q1 = pairs.contains(*lhs) ? pairs.at(*lhs) : *lhs;
            const auto q2 = pairs.contains(*rhs) ? pairs.at(*rhs) : *rhs;
            if (q1 == q2) {
                continue;
            }

            const auto x = static_cast<Graph::IdType>(q1.Id());
            const auto y = static_cast<Graph::IdType>(q2.Id());
            if (g.HasEdge(x, y)) {
                g.SetEdgeLength(x, y, g.GetEdgeLength(x, y) + len);
            } else {
                g.AddEdge(x, y, len);
            }
        }
    }
}
QubitGraph::QubitGraph(const MachineFunction& mf) {
    for (const auto& mbb : mf) {
        for (const auto& minst : mbb) {
            const auto* inst = static_cast<const ScLsInstructionBase*>(minst.get());

            switch (inst->Type()) {
                case ScLsInstructionType::ALLOCATE: {
                    const auto* allocate = Cast<Allocate>(inst);
                    graph_.AddNode(static_cast<Graph::IdType>(allocate->Qubit().Id()), 1);
                    num_qubits_++;
                    break;
                }
                default: {
                    const auto& qs = inst->QTarget();
                    const auto len = inst->Latency();
                    if (qs.size() >= 2) {
                        UpdateEdges(graph_, qs, pairs_, len);
                    }
                    break;
                }
            }
        }
    }
}
bool QubitGraph::Partition(
        const std::vector<std::size_t>& max_weights,
        PartitionAlgorithm algorithm
) {
    switch (algorithm) {
        case PartitionAlgorithm::Greedy:
            return PartitionByGreedy(max_weights);
        case PartitionAlgorithm::Random:
            return PartitionByRandom(max_weights);
        case PartitionAlgorithm::METIS:
            return PartitionByMETIS(max_weights);
        default:
            return false;
    }
}
bool QubitGraph::PartitionByGreedy(const std::vector<std::size_t>& max_weights) {
    if (max_weights.empty()) {
        return graph_.NumNodes() == 0;
    }

    partition_.resize(max_weights.size());

    auto itr = graph_.begin();
    for (auto i = std::size_t{0}; i < max_weights.size(); ++i) {
        auto weight = max_weights[i];
        while (itr != graph_.end() && itr->weight <= weight) {
            const auto q = QSymbol(static_cast<QSymbol::IdType>(itr->id));
            partition_[i].emplace_back(q);
            if (pairs_.contains(q)) {
                partition_[i].emplace_back(pairs_.at(q));
            }
            weight -= itr->weight;
            ++itr;
        }
    }
    return itr == graph_.end();
}
bool QubitGraph::PartitionByRandom(std::vector<std::size_t> max_weights, std::uint64_t seed) {
    if (max_weights.empty()) {
        return graph_.NumNodes() == 0;
    }

    partition_.resize(max_weights.size());

    auto engine = std::mt19937_64(seed);
    auto dist = std::uniform_int_distribution<std::size_t>(0, max_weights.size() - 1);

    for (const auto& node : graph_) {
        auto success = false;

        // Search place to insert node starting from 'tmp'.
        auto tmp = dist(engine);

        for (auto j = std::size_t{0}; j < max_weights.size(); ++j) {
            // Try to insert node at 'index'.
            const auto index = (tmp + j) % max_weights.size();
            const auto weight = max_weights[index];
            if (node.weight <= weight) {
                const auto q = QSymbol(static_cast<QSymbol::IdType>(node.id));
                partition_[index].emplace_back(q);
                if (pairs_.contains(q)) {
                    partition_[index].emplace_back(pairs_.at(q));
                }
                success = true;
                max_weights[index] -= node.weight;
                break;
            }
        }
        if (!success) {
            return false;
        }
    }

    return true;
}
bool QubitGraph::PartitionByMETIS(const std::vector<std::size_t>& max_weights) {
    if (max_weights.empty()) {
        return graph_.NumNodes() == 0;
    }

    throw std::logic_error("Partition by METIS is not implemented.");

    auto success = false;
    return success;
}
QubitMapper::QubitMapper(
        const std::shared_ptr<const Topology>& topology,
        FindPlaceAlgorithm algorithm
)
    : topology_(topology) {
    for (const auto& grid : *topology) {
        for (const auto& plane : grid) {
            FindPlace(plane, algorithm);
        }
    }
}
void QubitMapper::FindPlace(const ScLsPlane& plane, FindPlaceAlgorithm algorithm) {
    const auto z = plane.GetZ();
    auto place = std::unordered_set<Coord3D>();

    const auto is_ok = [&plane, &place, z, algorithm](std::int32_t x, std::int32_t y) {
        for (auto dx = -1; dx <= 1; ++dx) {
            for (auto dy = -1; dy <= 1; ++dy) {
                const auto tmp_x = x + dx;
                const auto tmp_y = y + dy;

                if (algorithm == FindPlaceAlgorithm::EnoughSpaceSoft
                    && plane.OutOfPlane({tmp_x, tmp_y})) {
                    continue;
                }
                if (!plane.IsFree({tmp_x, tmp_y}) || place.contains({tmp_x, tmp_y, z})) {
                    return false;
                }
            }
        }
        return true;
    };

    for (auto x = 0; x < plane.GetMaxX(); ++x) {
        for (auto y = 0; y < plane.GetMaxY(); ++y) {
            if (is_ok(x, y)) {
                place.emplace(x, y, z);
            }
        }
    }
    place_[z] = std::move(place);
}
bool QubitMapper::MapQubits(QubitGraph& graph, PartitionAlgorithm algorithm) {
    auto max_weights = std::vector<std::size_t>();
    for (const auto& [_, places] : place_) {
        max_weights.emplace_back(places.size());
    }
    // std::cout << fmt::format("max weights: {}", max_weights) << std::endl;

    if (!graph.Partition(max_weights, algorithm)) {
        LOG_ERROR("Failed to find partition");
        return false;
    }

    auto partition_itr = graph.partition_begin();
    auto place_itr = place_.begin();
    while (partition_itr != graph.partition_end() && place_itr != place_.end()) {
        const auto& qsymbols = *partition_itr;
        const auto& places = place_itr->second;

        if (qsymbols.size() > places.size()) {
            LOG_ERROR("The number of qsymbols is larger than places.");
            return false;
        }

        auto a = qsymbols.begin();
        auto b = places.begin();
        while (a != qsymbols.end() && b != places.end()) {
            map_[*a] = *b;
            ++a;
            ++b;
        }

        ++partition_itr;
        ++place_itr;
    }

    if (map_.size() != graph.NumQubits()) {
        LOG_ERROR("The number of mapped qubits is not equal to the number of qubits in graph");
        return false;
    }

    return true;
}
bool Mapping::RunOnMachineFunction(MachineFunction& mf) {
    if (ScLsMappingAlgorithm.Get() == 0) {
        return MapBasedOnTopologyFile(mf);
    }
    if (ScLsMappingAlgorithm.Get() != 1) {
        throw std::runtime_error("sc_ls_fixed_v0-mapping-algorithm must be 0 or 1");
    }
    if (ScLsPartitionAlgorithm > 2) {
        throw std::runtime_error(
                "ScLsPartitionAlgorithm must be 0, 1, or 2 (0: Greedy, 1: Random, 2: METIS)."
        );
    }
    if (ScLsFindPlaceAlgorithm > 1) {
        throw std::runtime_error(
                "ScLsFindPlaceAlgorithm must be 0 or 1 (0: EnoughSpaceSoft, 1: EnoughSpaceHard)."
        );
    }

    const auto& target = *static_cast<const ScLsFixedV0TargetMachine*>(mf.GetTarget());
    const auto& topology = target.topology;

    auto graph = QubitGraph(mf);
    auto mapper =
            QubitMapper(topology, QubitMapper::FindPlaceAlgorithm(ScLsFindPlaceAlgorithm.Get()));
    if (!mapper.MapQubits(graph, QubitMapper::PartitionAlgorithm(ScLsPartitionAlgorithm.Get()))) {
        throw std::runtime_error("Failed to find place to map qubits");
    }

    for (auto&& mbb : mf) {
        for (auto&& minst : mbb) {
            auto* inst = static_cast<ScLsInstructionBase*>(minst.get());

            if (inst->Type() == ScLsInstructionType::ALLOCATE) {
                auto* allocate = Cast<Allocate>(inst);
                const auto q = allocate->Qubit();
                allocate->SetDest(mapper.GetPlace(q));
            }
        }
    }

    auto changed = true;
    return changed;
}
bool Mapping::MapBasedOnTopologyFile(MachineFunction& mf) {
    const auto& target = *static_cast<const ScLsFixedV0TargetMachine*>(mf.GetTarget());
    const auto& topology = target.topology;

    for (auto&& mbb : mf) {
        for (auto&& minst : mbb) {
            auto* inst = static_cast<ScLsInstructionBase*>(minst.get());

            if (inst->Type() == ScLsInstructionType::ALLOCATE) {
                auto* allocate = Cast<Allocate>(inst);
                const auto q = allocate->Qubit();
                if (!topology->Contains(q)) {
                    throw std::runtime_error(
                            fmt::format("Place of {} is not defined in topology file", q)
                    );
                }
                allocate->SetDest(topology->GetPlace(q));
            }
        }
    }

    auto changed = true;
    return changed;
}
}  // namespace qret::sc_ls_fixed_v0
