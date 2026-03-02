/**
 * @file qret/target/sc_ls_fixed_v0/search_chip_comm.h
 * @brief Search route between chips.
 */

#ifndef QRET_TARGET_SC_LS_FIXED_V0_SEARCH_CHIP_COMM_H
#define QRET_TARGET_SC_LS_FIXED_V0_SEARCH_CHIP_COMM_H

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <vector>

#include "qret/base/graph.h"
#include "qret/codegen/machine_function.h"
#include "qret/qret_export.h"
#include "qret/target/sc_ls_fixed_v0/inst_queue.h"
#include "qret/target/sc_ls_fixed_v0/instruction.h"
#include "qret/target/sc_ls_fixed_v0/state.h"
#include "qret/target/sc_ls_fixed_v0/symbol.h"
#include "qret/target/sc_ls_fixed_v0/topology.h"

namespace qret::sc_ls_fixed_v0 {
/**
 * @brief Search route to connect all chips.
 */
class QRET_EXPORT SearchChipRoute {
public:
    explicit SearchChipRoute(const Topology& topology);

    /**
     * @brief Search steiner tree to connect chips.
     *
     * @param zs z-coordinates of chips to connect.
     * @return std::optional<Graph> steiner tree if found, otherwise std::nullopt.
     */
    std::optional<Graph> Search(const std::vector<std::int32_t>& zs);

    /**
     * @brief Set the length of chip graph based on the current states of entanglement factories.
     *
     * @param state current quantum state.
     */
    void SetGraphLength(const QuantumState& state);

private:
    static Graph CreateChipGraphImpl(const Topology& topology);
    static Graph::IdType ToId(std::int32_t z);

    Graph graph_;
    MinimumSteinerTreeSolver solver_;

    std::size_t max_efs_ = 0;
    std::map<std::pair<Graph::IdType, Graph::IdType>, std::vector<ESymbol>> ef_ = {};
};

class QRET_EXPORT SplitMultinodeInst {
public:
    explicit SplitMultinodeInst(
            const Topology& topology,
            std::shared_ptr<SymbolGenerator> symbol_generator
    )
        : topology_(topology)
        , symbol_generator_(std::move(symbol_generator))
        , router_(topology) {}

    bool Split(
            const QuantumState& state,
            MachineFunction& mf,
            InstQueue& queue,
            ScLsInstructionBase* inst
    );

private:
    void SplitLSMultinode(
            const QuantumState& state,
            MachineFunction& mf,
            InstQueue& queue,
            LatticeSurgeryMultinode* inst,
            const Graph& tree,
            const std::vector<std::int32_t>& zs
    );
    void SplitMove(
            const QuantumState& state,
            MachineFunction& mf,
            InstQueue& queue,
            Move* inst,
            const Graph& tree
    );
    void SplitCnot(
            const QuantumState& state,
            MachineFunction& mf,
            InstQueue& queue,
            Cnot* inst,
            const Graph& tree
    );

    static std::pair<ESymbol, ESymbol>
    GetPair(const QuantumState& state, std::int32_t z1, std::int32_t z2);
    static std::int32_t
    FindChipWithMostMagicStates(const QuantumState& state, const std::vector<std::int32_t>& zs);
    static MachineBasicBlock* FindMBB(MachineFunction& mf, MachineInstruction* inst);

    const Topology& topology_;
    std::shared_ptr<SymbolGenerator> symbol_generator_;
    SearchChipRoute router_;
};
}  // namespace qret::sc_ls_fixed_v0

#endif  // QRET_TARGET_SC_LS_FIXED_V0_SEARCH_CHIP_COMM_H
