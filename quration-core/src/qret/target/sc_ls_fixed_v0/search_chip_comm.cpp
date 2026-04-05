/**
 * @file qret/target/sc_ls_fixed_v0/search_chip_comm.cpp
 * @brief Search route between chips.
 */

#include "qret/target/sc_ls_fixed_v0/search_chip_comm.h"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <map>
#include <optional>
#include <stdexcept>
#include <vector>

#include "qret/base/graph.h"
#include "qret/base/log.h"
#include "qret/codegen/machine_function.h"
#include "qret/target/sc_ls_fixed_v0/instruction.h"
#include "qret/target/sc_ls_fixed_v0/pauli.h"
#include "qret/target/sc_ls_fixed_v0/state.h"
#include "qret/target/sc_ls_fixed_v0/symbol.h"
#include "qret/target/sc_ls_fixed_v0/topology.h"

namespace qret::sc_ls_fixed_v0 {
SearchChipRoute::SearchChipRoute(const Topology& topology)
    : graph_(CreateChipGraphImpl(topology))
    , solver_(graph_) {
    // Initialize 'ef_'
    for (const auto z : topology.GetAllZs()) {
        for (const auto z_pair : topology.GetLinkedZs(z)) {
            if (z > z_pair || ef_.contains({z, z_pair})) {
                continue;
            }

            const auto pairs = topology.GetEntanglementPairsBetween(z, z_pair);
            auto& links = ef_[{z, z_pair}];
            links.reserve(pairs.size());
            for (const auto& [e, _] : pairs) {
                links.emplace_back(e);
            }
        }
    }

    for (const auto& entry : ef_) {
        max_efs_ = std::max(max_efs_, entry.second.size());
    }
}
std::optional<Graph> SearchChipRoute::Search(const std::vector<std::int32_t>& zs) {
    auto terminals = zs;
    std::sort(terminals.begin(), terminals.end());
    terminals.erase(std::unique(terminals.begin(), terminals.end()), terminals.end());
    if (terminals.empty()) {
        return std::nullopt;
    }

    auto result = solver_.SolveByWu(terminals);
    if (!result) {
        return std::nullopt;
    }

    auto& [cost, tree] = *result;
    return std::move(tree);
}
void SearchChipRoute::SetGraphLength(const QuantumState& state) {
    for (const auto& [z_z_pair, ef] : ef_) {
        const auto& [z, z_pair] = z_z_pair;

        auto num_available = Graph::Length{0};
        auto free_stock = Graph::Length{0};
        auto reserved_stock = Graph::Length{0};
        for (const auto e : ef) {
            const auto& factory = state.GetState(e);
            free_stock += factory.entangled_state_stock_count_free;
            reserved_stock += factory.entangled_state_stock_count_reserve();
            if (factory.IsAvailable()) {
                num_available++;
            }
        }

        if (free_stock > 0) {
            graph_.SetEdgeLength(z, z_pair, 1);
            continue;
        }

        const auto scarcity = std::max<Graph::Length>(1, max_efs_ - num_available + 1);
        const auto length = 1 + (scarcity * 10) + reserved_stock;
        graph_.SetEdgeLength(z, z_pair, length);
    }
}
Graph SearchChipRoute::CreateChipGraphImpl(const Topology& topology) {
    auto graph = Graph();
    const auto zs = topology.GetAllZs();
    // Add nodes.
    for (const auto z : zs) {
        graph.AddNode(ToId(z));
    }
    // Add edges.
    for (const auto z : zs) {
        for (const auto z_pair : topology.GetLinkedZs(z)) {
            if (z < z_pair) {
                graph.AddEdge(ToId(z), ToId(z_pair), 1);
            }
        }
    }
    return graph;
}
Graph::IdType SearchChipRoute::ToId(std::int32_t z) {
    return static_cast<Graph::IdType>(z);
}

bool SplitMultinodeInst::Split(
        const QuantumState& state,
        MachineFunction& mf,
        InstQueue& queue,
        ScLsInstructionBase* inst
) {
    const auto zs = [inst, &state]() -> std::vector<std::int32_t> {
        auto zs = std::vector<std::int32_t>{};
        zs.reserve(inst->QTarget().size() + inst->ETarget().size());
        for (const auto& q : inst->QTarget()) {
            zs.emplace_back(state.GetPlace(q).z);
        }
        for (const auto& e : inst->ETarget()) {
            zs.emplace_back(state.GetPlace(e).z);
        }
        std::sort(zs.begin(), zs.end());
        const auto itr = std::unique(zs.begin(), zs.end());
        zs.erase(itr, zs.end());
        return zs;
    }();

    if (zs.size() <= 1) {
        return false;
    }

    // Need split.

    // Initialize the edge lengths of chip graph.
    router_.SetGraphLength(state);

    // Search minimum steiner tree.
    const auto tmp_tree = router_.Search(zs);
    if (!tmp_tree) {
        LOG_DEBUG("Failed to find steiner tree to connect all chips");
        return false;
    }

    const auto& tree = tmp_tree.value();

    // Split into instructions.
    const auto inst_type = inst->Type();

    if (inst_type == ScLsInstructionType::LATTICE_SURGERY_MULTINODE) {
        SplitLSMultinode(state, mf, queue, static_cast<LatticeSurgeryMultinode*>(inst), tree, zs);
    } else if (inst_type == ScLsInstructionType::MOVE) {
        SplitMove(state, mf, queue, static_cast<Move*>(inst), tree);
    } else if (inst_type == ScLsInstructionType::CNOT) {
        SplitCnot(state, mf, queue, static_cast<Cnot*>(inst), tree);
    }

    return true;
}
void SplitMultinodeInst::SplitLSMultinode(
        const QuantumState& state,
        [[maybe_unused]] MachineFunction& mf,
        InstQueue& queue,
        LatticeSurgeryMultinode* inst,
        const Graph& tree,
        const std::vector<std::int32_t>& zs
) {
    // Set magic factory.
    const auto z_magic = inst->UseMagicFactory()
            ? std::make_optional(FindChipWithMostMagicStates(state, zs))
            : std::nullopt;
    const auto select_magic_factory = [&state](std::int32_t z) -> std::optional<MSymbol> {
        const auto& factories = state.GetMagicFactoryList(z);
        if (factories.empty()) {
            return std::nullopt;
        }

        std::optional<MSymbol> best = std::nullopt;
        std::uint64_t max_stock = 0;
        for (const auto m : factories) {
            const auto stock = state.GetState(m).magic_state_count;
            if (!best.has_value() || stock > max_stock) {
                best = m;
                max_stock = stock;
            }
        }
        return best;
    };

    auto z2qme = std::map<
            std::int32_t,
            std::tuple<
                    std::list<QSymbol>,
                    std::list<Pauli>,
                    std::list<MSymbol>,
                    std::list<ESymbol>,
                    std::list<EHandle>>>{};

    // Initialize and set MSymbol.
    for (const auto& n : tree) {
        auto& qme = z2qme[n.id] = {};
        if (z_magic.has_value() && z_magic.value() == n.id) {
            const auto magic_factory = select_magic_factory(n.id);
            if (!magic_factory.has_value()) {
                throw std::runtime_error(fmt::format("No magic factory is defined at z={}", n.id));
            }
            std::get<std::list<MSymbol>>(qme).emplace_back(*magic_factory);
        }
    }

    // Set QSymbol and Pauli.
    {
        auto q_itr = inst->QTarget().begin();
        const auto q_end = inst->QTarget().end();
        auto b_itr = inst->BasisList().begin();
        const auto b_end = inst->BasisList().end();
        while (q_itr != q_end && b_itr != b_end) {
            const auto q = *q_itr;
            const auto b = *b_itr;
            const auto z = state.GetPlace(q).z;
            std::get<std::list<QSymbol>>(z2qme.at(z)).emplace_back(q);
            std::get<std::list<Pauli>>(z2qme.at(z)).emplace_back(b);
            q_itr++;
            b_itr++;
        }
    }

    // Set ESymbol and EHandle.
    for (const auto& n : tree) {
        for (const auto& z2 : n.adj) {
            const auto z1 = n.id;
            if (z1 > z2) {
                continue;
            }

            const auto e_handle = symbol_generator_->GenerateEH();
            const auto [e1, e2] = GetPair(state, z1, z2);
            std::get<std::list<ESymbol>>(z2qme[z1]).emplace_back(e1);
            std::get<std::list<EHandle>>(z2qme[z1]).emplace_back(e_handle);
            std::get<std::list<ESymbol>>(z2qme[z2]).emplace_back(e2);
            std::get<std::list<EHandle>>(z2qme[z2]).emplace_back(e_handle);
        }
    }

    // Find machine basic block which contains 'inst'.
    auto* const mbb = FindMBB(mf, inst);
    if (mbb == nullptr) {
        throw std::runtime_error(
                fmt::format(
                        "Machine function does not contain LATTICE_SURGERY_MULTINODE: {}",
                        inst->ToString()
                )
        );
    }

    // Generate instructions.
    auto new_insts = std::vector<ScLsInstructionBase*>{};
    new_insts.reserve(z2qme.size());
    for (const auto& [z, qme] : z2qme) {
        const auto& qs = std::get<std::list<QSymbol>>(qme);
        const auto& bs = std::get<std::list<Pauli>>(qme);
        const auto& ms = std::get<std::list<MSymbol>>(qme);
        const auto& es = std::get<std::list<ESymbol>>(qme);
        const auto& ehs = std::get<std::list<EHandle>>(qme);

        auto new_inst = LatticeSurgeryMultinode::New(
                qs,
                bs,
                {},
                symbol_generator_->GenerateC(),
                ms,
                es,
                ehs,
                inst->Condition()
        );
        new_insts.emplace_back(new_inst.get());
        mbb->InsertAfter(inst, std::move(new_inst));
    }

    // Delete LATTICE_SURGERY_MULTINODE.
    mbb->Erase(inst);

    // Update instruction queue.
    queue.Replace(inst, new_insts, new_insts.front()->Latency());
}
void SplitMultinodeInst::SplitMove(
        const QuantumState& state,
        MachineFunction& mf,
        InstQueue& queue,
        Move* inst,
        const Graph& tree
) {
    const auto src = inst->Qubit();
    const auto dst = inst->QDest();
    const auto z_src = topology_.GetPlace(src).z;
    const auto z_dst = topology_.GetPlace(dst).z;
    if (z_src == z_dst) {
        return;
    }

    auto z2qe = std::map<
            std::int32_t,
            std::tuple<std::list<QSymbol>, std::list<ESymbol>, std::list<EHandle>>>{};
    for (const auto& n : tree) {
        z2qe[n.id] = {};
    }

    // Set QSymbol.
    std::get<0>(z2qe[z_src]).emplace_back(src);
    std::get<0>(z2qe[z_dst]).emplace_back(dst);

    // Set ESymbol and EHandle.
    for (const auto& n : tree) {
        for (const auto& z2 : n.adj) {
            const auto z1 = n.id;
            if (z1 > z2) {
                continue;
            }

            const auto e_handle = symbol_generator_->GenerateEH();
            const auto [e1, e2] = GetPair(state, z1, z2);
            std::get<1>(z2qe[z1]).emplace_back(e1);
            std::get<2>(z2qe[z1]).emplace_back(e_handle);
            std::get<1>(z2qe[z2]).emplace_back(e2);
            std::get<2>(z2qe[z2]).emplace_back(e_handle);
        }
    }

    // Find machine basic block which contains 'inst'.
    auto* mbb = FindMBB(mf, inst);
    if (mbb == nullptr) {
        throw std::runtime_error(
                fmt::format("Machine function does not contain MOVE: {}", inst->ToString())
        );
    }

    // Generate instructions.
    auto new_insts = std::vector<ScLsInstructionBase*>{};
    new_insts.reserve(z2qe.size());
    for (const auto& [z, qe] : z2qe) {
        const auto& qs = std::get<0>(qe);
        const auto& es = std::get<1>(qe);
        const auto& ehs = std::get<2>(qe);

        auto new_inst = [this, inst, src, dst, z, z_src, z_dst, &qs, &es, &ehs]()
                -> std::unique_ptr<ScLsInstructionBase> {
            if (qs.size() == 1 && es.size() == 1) {
                if (z == z_dst) {
                    return MoveEntanglement::New(
                            es.front(),
                            ehs.front(),
                            dst,
                            {},
                            inst->Condition()
                    );
                } else if (z == z_src) {
                    return LatticeSurgeryMultinode::New(
                            {src},
                            {Pauli::X()},
                            {},
                            symbol_generator_->GenerateC(),
                            {},
                            es,
                            ehs,
                            inst->Condition()
                    );
                } else {
                    throw std::logic_error("Invalid z value.");
                }
            } else if (qs.empty() && es.size() == 2) {
                return LatticeSurgeryMultinode::New(
                        {},
                        {},
                        {},
                        symbol_generator_->GenerateC(),
                        {},
                        es,
                        ehs,
                        inst->Condition()
                );
            } else {
                throw std::logic_error("Invalid qubit and entanglement symbol size.");
            }
        }();
        new_insts.emplace_back(new_inst.get());
        mbb->InsertAfter(inst, std::move(new_inst));
    }

    // Delete MOVE.
    mbb->Erase(inst);

    // Update instruction queue.
    queue.Replace(inst, new_insts, new_insts.front()->Latency());
}
void SplitMultinodeInst::SplitCnot(
        const QuantumState& state,
        MachineFunction& mf,
        InstQueue& queue,
        Cnot* const inst,
        const Graph& tree
) {
    const auto q1 = inst->Qubit1();
    const auto q2 = inst->Qubit2();
    const auto z_q1 = state.GetPlace(q1).z;
    const auto z_q2 = state.GetPlace(q2).z;
    if (z_q1 == z_q2) {
        return;
    }

    auto z2qe = std::map<
            std::int32_t,
            std::tuple<std::list<QSymbol>, std::list<ESymbol>, std::list<EHandle>>>{};
    for (const auto& n : tree) {
        z2qe[n.id] = {};
    }

    // Set QSymbol.
    std::get<0>(z2qe[z_q1]).emplace_back(q1);
    std::get<0>(z2qe[z_q2]).emplace_back(q2);

    // Set ESymbol and EHandle.
    for (const auto& n : tree) {
        for (const auto& z2 : n.adj) {
            const auto z1 = n.id;
            if (z1 > z2) {
                continue;
            }

            const auto e_handle = symbol_generator_->GenerateEH();
            const auto [e1, e2] = GetPair(state, z1, z2);
            std::get<1>(z2qe[z1]).emplace_back(e1);
            std::get<2>(z2qe[z1]).emplace_back(e_handle);
            std::get<1>(z2qe[z2]).emplace_back(e2);
            std::get<2>(z2qe[z2]).emplace_back(e_handle);
        }
    }

    // Find machine basic block which contains 'inst'.
    auto* mbb = FindMBB(mf, inst);
    if (mbb == nullptr) {
        throw std::runtime_error(
                fmt::format("Machine function does not contain CNOT: {}", inst->ToString())
        );
    }

    // Generate instructions and insert them into the machine basic block.
    auto new_insts = std::vector<ScLsInstructionBase*>{};
    new_insts.reserve(z2qe.size());
    for (const auto& [z, qe] : z2qe) {
        const auto& qs = std::get<0>(qe);
        const auto& es = std::get<1>(qe);
        const auto& ehs = std::get<2>(qe);

        auto new_inst = [this, inst, q1, q2, z, z_q1, z_q2, &qs, &es, &ehs]()
                -> std::unique_ptr<ScLsInstructionBase> {
            if (qs.size() == 1 && es.size() == 1) {
                if (z == z_q2) {
                    return LatticeSurgeryMultinode::New(
                            {q2},
                            {Pauli::X()},
                            {},
                            symbol_generator_->GenerateC(),
                            {},
                            es,
                            ehs,
                            inst->Condition()
                    );
                } else if (z == z_q1) {
                    return LatticeSurgeryMultinode::New(
                            {q1},
                            {Pauli::Z()},
                            {},
                            symbol_generator_->GenerateC(),
                            {},
                            es,
                            ehs,
                            inst->Condition()
                    );
                } else {
                    throw std::logic_error("Invalid z value.");
                }
            } else if (qs.empty() && es.size() == 2) {
                return LatticeSurgeryMultinode::New(
                        {},
                        {},
                        {},
                        symbol_generator_->GenerateC(),
                        {},
                        es,
                        ehs,
                        inst->Condition()
                );
            } else {
                throw std::logic_error("Invalid qubit and entanglement symbol size.");
            }
        }();
        new_insts.emplace_back(new_inst.get());
        mbb->InsertAfter(inst, std::move(new_inst));
    }

    // Update instruction queue.
    queue.Replace(inst, new_insts, new_insts.front()->Latency());

    // Delete CNOT.
    mbb->Erase(inst);
}
std::pair<ESymbol, ESymbol>
SplitMultinodeInst::GetPair(const QuantumState& state, std::int32_t z1, std::int32_t z2) {
    const auto& topology = state.GetTopology();
    auto pairs = std::vector<std::pair<ESymbol, ESymbol>>{};
    for (const auto e : state.GetEntanglementFactoryList(z1)) {
        const auto pair_opt = topology.FindPair(e);
        if (!pair_opt.has_value()) {
            continue;
        }
        const auto pair_place = topology.FindPlace(*pair_opt);
        if (pair_place != nullptr && pair_place->z == z2) {
            pairs.emplace_back(e, *pair_opt);
        }
    }

    if (pairs.empty()) {
        throw std::runtime_error(fmt::format("No link between z-{} and z-{}", z1, z2));
    }

    std::sort(
            pairs.begin(),
            pairs.end(),
            [](const std::pair<ESymbol, ESymbol>& x, const std::pair<ESymbol, ESymbol>& y) {
                if (x.first.Id() != y.first.Id()) {
                    return x.first.Id() < y.first.Id();
                }
                return x.second.Id() < y.second.Id();
            }
    );

    auto ret = pairs.front();
    auto max_stock = state.GetState(ret.first).entangled_state_stock_count_free;
    for (const auto& [e, e_pair] : pairs) {
        const auto stock = state.GetState(e).entangled_state_stock_count_free;
        if (stock > max_stock) {
            ret = {e, e_pair};
            max_stock = stock;
        }
    }

    return ret;
}
std::int32_t SplitMultinodeInst::FindChipWithMostMagicStates(
        const QuantumState& state,
        const std::vector<std::int32_t>& zs
) {
    if (zs.empty()) {
        throw std::invalid_argument("Set of z coordinate is empty.");
    }

    auto ret = zs.front();
    auto max_magic_states = std::uint32_t{0};

    for (const auto z : zs) {
        // Count the number of magic states in chip z.
        auto tmp_magic_states = std::uint32_t{0};
        for (const auto magic_factory : state.GetMagicFactoryList(z)) {
            tmp_magic_states += state.GetState(magic_factory).magic_state_count;
        }
        if (tmp_magic_states > max_magic_states) {
            max_magic_states = tmp_magic_states;
            ret = z;
        }
    }
    return ret;
}
MachineBasicBlock* SplitMultinodeInst::FindMBB(MachineFunction& mf, MachineInstruction* inst) {
    MachineBasicBlock* mbb = nullptr;
    for (auto& tmp_mbb : mf) {
        if (tmp_mbb.Contain(inst)) {
            mbb = &tmp_mbb;
            break;
        }
    }
    return mbb;
}
}  // namespace qret::sc_ls_fixed_v0
