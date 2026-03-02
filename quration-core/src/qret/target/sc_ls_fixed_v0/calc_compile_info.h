/**
 * @file qret/target/sc_ls_fixed_v0/calc_compile_info.h
 * @brief Calculate compile information.
 */

#ifndef QRET_TARGET_SC_LS_FIXED_V0_CALC_COMPILE_INFO_H
#define QRET_TARGET_SC_LS_FIXED_V0_CALC_COMPILE_INFO_H

#include <fmt/format.h>
#include <fmt/ostream.h>

#include "qret/base/graph.h"
#include "qret/codegen/machine_function.h"
#include "qret/codegen/machine_function_pass.h"
#include "qret/qret_export.h"
#include "qret/target/sc_ls_fixed_v0/instruction.h"

namespace qret::sc_ls_fixed_v0 {
class QRET_EXPORT DepGraph {
public:
    explicit DepGraph(const MachineFunction& mf);

    void SetInstWeight(const ScLsInstructionBase& inst, DiGraph::Weight weight) {
        const auto id = ptr2id_.at(&inst);
        graph_.SetNodeWeight(id, weight);
    }
    void SetAllLength(DiGraph::Length length) {
        for (const auto& node : graph_) {
            for (const auto& from : node.parent) {
                graph_.SetEdgeLength(from, node.id, length);
            }
        }
    }
    void SetLength(
            const ScLsInstructionBase& from,
            const ScLsInstructionBase& to,
            DiGraph::Length length
    ) {
        const auto from_id = ptr2id_.at(&from);
        const auto to_id = ptr2id_.at(&to);
        graph_.SetEdgeLength(from_id, to_id, length);
    }

    DiGraph::Weight CalcHeaviest() const {
        const auto& [weight, _path, _depth_of_each_node] = FindHeaviestPath(graph_);
        return weight;
    }
    DiGraph::Length CalcLongest() const {
        const auto& [length, _path, _depth_of_each_node] = FindLongestPath(graph_);
        return length;
    }

private:
    std::map<const ScLsInstructionBase*, DiGraph::IdType> ptr2id_;
    std::map<DiGraph::IdType, const ScLsInstructionBase*> id2ptr_;
    DiGraph graph_;
};
class QRET_EXPORT TimeSeries {
public:
    struct ChipInfo {
        std::uint32_t space = 0;
        std::uint32_t m_symb = 0;
        std::uint32_t e_symb = 0;
        std::uint32_t q_symb = 0;
        std::uint32_t used_ancilla_count = 0;

        std::uint32_t ChipCellCount() const {
            return space - m_symb - e_symb;
        }
        std::uint32_t ChipCellAlgorithmicQubit() const {
            return q_symb;
        }
        double ChipCellAlgorithmicQubitRatio() const {
            return static_cast<double>(ChipCellAlgorithmicQubit())
                    / static_cast<double>(ChipCellCount());
        }
        std::uint32_t ChipCellActiveQubitArea() const {
            return used_ancilla_count + q_symb;
        }
        double ChipCellActiveQubitAreaRatio() const {
            return static_cast<double>(ChipCellActiveQubitArea())
                    / static_cast<double>(ChipCellCount());
        }
    };

    explicit TimeSeries(const MachineFunction& mf);

    std::uint64_t GetRuntime() const {
        assert(beat2chip_.size() == beat2chip_.size());
        return beat2chip_.size();
    }
    const std::vector<const ScLsInstructionBase*>& GetInstructions(std::uint64_t beat) const {
        return beat2inst_[beat];
    }
    const ChipInfo& GetChipInfo(std::uint64_t beat) const {
        return beat2chip_[beat];
    }

    auto begin() const {
        return beat2chip_.begin();
    }
    auto cbegin() const {
        return beat2chip_.cbegin();
    }
    auto end() const {
        return beat2chip_.end();
    }
    auto cend() const {
        return beat2chip_.cend();
    }

private:
    std::vector<std::vector<const ScLsInstructionBase*>> beat2inst_;
    std::vector<ChipInfo> beat2chip_;
};
/**
 * @brief Calculate compile information without topology.
 * @details Calculate following statistics:
 *
 * * about runtime
 *     * runtime_without_topology
 * * about gate
 *     * gate_count
 *     * gate_count_dict
 *     * gate_depth
 * * about measurement
 *     * measurement_feedback_count
 *     * measurement_feedback_depth
 *     * runtime_estimation_measurement_feedback_count
 *     * runtime_estimation_measurement_feedback_depth
 * * about magic state
 *     * magic_state_consumption_count
 *     * magic_state_consumption_depth
 *     * runtime_estimation_magic_state_consumption_count
 *     * runtime_estimation_magic_state_consumption_depth
 *     * magic_factory_count
 * * about entanglement
 *     * entanglement_consumption_count
 *     * entanglement_consumption_depth
 *     * runtime_estimation_entanglement_consumption_count
 *     * runtime_estimation_entanglement_consumption_depth
 *     * entanglement_factory_count
 */
struct QRET_EXPORT CompileInfoWithoutTopology : public MachineFunctionPass {
    static inline char ID = 0;
    CompileInfoWithoutTopology()
        : MachineFunctionPass(&ID) {}

    bool RunOnMachineFunction(MachineFunction& mf) override;
};
/**
 * @brief Calculate compile information with topology.
 * @details Calculate following statistics:
 *
 * * about runtime
 *     * runtime
 * * about gate
 *     * gate_throughput
 * * about measurement
 *     * measurement_feedback_rate
 * * about magic state
 *     * magic_state_consumption_rate
 * * about entanglement
 *     * entanglement_consumption_rate
 * * about cell consumption
 *     * chip_cell_count
 *     * chip_cell_algorithmic_qubit
 *     * chip_cell_algorithmic_qubit_ratio
 *     * chip_cell_activate_qubit_area
 *     * chip_cell_activate_qubit_area_ratio
 *     * qubit_volume
 */
struct QRET_EXPORT CompileInfoWithTopology : public MachineFunctionPass {
    static inline char ID = 0;
    CompileInfoWithTopology()
        : MachineFunctionPass(&ID) {}

    bool RunOnMachineFunction(MachineFunction& mf) override;
};
/**
 * @brief Estimate QEC-related compile information.
 * @details Run CompileInfoWithTopology and CompileInfoWithoutTopology passes before running this
 * pass. This pass adds the following statistics:
 *
 * * code_distance
 * * execution_time_sec
 * * num_physical_qubits
 */
struct QRET_EXPORT CompileInfoWithQecResourceEstimation : public MachineFunctionPass {
    static inline char ID = 0;
    CompileInfoWithQecResourceEstimation()
        : MachineFunctionPass(&ID) {}

    /**
     * @brief Estimate the minimum odd code distance that satisfies the target failure probability.
     * @details Uses pL = p * lambda^{(d-1)/2} and requires pL * active_volume <= eps.
     *
     * @param p Physical error rate.
     * @param lambda Drop rate in (0, 1).
     * @param eps Allowed failure probability for the program.
     * @param active_volume Active volume (e.g., sum of active qubits over code beats).
     */
    static std::uint64_t
    EstimateMinimumCodeDistance(double p, double lambda, double eps, std::uint64_t active_volume);
    /**
     * @brief Estimate execution time in seconds.
     * @details execution_time_sec = runtime * d * t_cycle.
     *
     * @param d Code distance.
     * @param runtime Number of code beats.
     * @param t_cycle Code cycle time in seconds.
     */
    static double EstimateExecutionTimeSec(std::uint64_t d, std::uint64_t runtime, double t_cycle);
    /**
     * @brief Estimate number of physical qubits.
     * @details num_physical_qubits = chip_cell_count * d^2 * 2.
     *
     * @param d Code distance.
     * @param chip_cell_count Number of cells in the chip.
     */
    static std::uint64_t EstimatePhysicalQubitCount(std::uint64_t d, std::uint64_t chip_cell_count);

    bool RunOnMachineFunction(MachineFunction& mf) override;
};
/**
 * @brief Initialize compile information.
 */
struct QRET_EXPORT InitCompileInfo : public MachineFunctionPass {
    static inline char ID = 0;
    InitCompileInfo()
        : MachineFunctionPass(&ID) {}

    bool RunOnMachineFunction(MachineFunction& mf) override;
};
/**
 * @brief Dump compile information.
 */
struct QRET_EXPORT DumpCompileInfo : public MachineFunctionPass {
    static inline char ID = 0;
    DumpCompileInfo()
        : MachineFunctionPass(&ID) {}

    bool RunOnMachineFunction(MachineFunction& mf) override;
};
}  // namespace qret::sc_ls_fixed_v0

#endif  // QRET_TARGET_SC_LS_FIXED_V0_CALC_COMPILE_INFO_H
