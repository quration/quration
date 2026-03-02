/**
 * @file qret/target/sc_ls_fixed_v0/compile_info.h
 * @brief Compile information.
 */

#ifndef QRET_TARGET_SC_LS_FIXED_V0_COMPILE_INFO_H
#define QRET_TARGET_SC_LS_FIXED_V0_COMPILE_INFO_H

#include <fmt/format.h>
#include <fmt/ostream.h>

#include "qret/qret_export.h"
#include "qret/target/sc_ls_fixed_v0/instruction.h"

namespace qret::sc_ls_fixed_v0 {
struct QRET_EXPORT ScLsFixedV0CompileInfo : CompileInfo {
    // constant
    bool use_magic_state_cultivation = false;
    std::uint64_t magic_factory_seed_offset = 0;
    std::uint64_t magic_generation_period = 0;
    double prob_magic_state_creation = 1.0;
    std::uint64_t maximum_magic_state_stock = 0;
    std::uint64_t entanglement_generation_period = 0;
    std::uint64_t maximum_entangled_state_stock = 0;
    std::uint64_t reaction_time = 0;
    std::shared_ptr<const Topology> topology = nullptr;

    // info about runtime
    std::uint64_t runtime = 0;
    std::uint64_t runtime_without_topology = 0;

    // info about gate
    std::uint64_t gate_count = 0;
    std::map<ScLsInstructionType, std::uint64_t> gate_count_dict = {};
    std::uint64_t gate_depth = 0;
    std::vector<std::uint64_t> gate_throughput = {};

    // info about measurement depth
    std::uint64_t measurement_feedback_count = 0;
    std::uint64_t measurement_feedback_depth = 0;
    std::vector<std::uint64_t> measurement_feedback_rate = {};
    std::uint64_t runtime_estimation_measurement_feedback_count = 0;
    std::uint64_t runtime_estimation_measurement_feedback_depth = 0;

    // info about magic state consumption
    std::uint64_t magic_state_consumption_count = 0;
    std::uint64_t magic_state_consumption_depth = 0;
    std::vector<std::uint64_t> magic_state_consumption_rate = {};
    std::uint64_t runtime_estimation_magic_state_consumption_count = 0;
    std::uint64_t runtime_estimation_magic_state_consumption_depth = 0;
    std::uint64_t magic_factory_count = 0;

    // info about entanglement consumption
    std::uint64_t entanglement_consumption_count = 0;
    std::uint64_t entanglement_consumption_depth = 0;
    std::vector<std::uint64_t> entanglement_consumption_rate = {};
    std::uint64_t runtime_estimation_entanglement_consumption_count = 0;
    std::uint64_t runtime_estimation_entanglement_consumption_depth = 0;
    std::uint64_t entanglement_factory_count = 0;

    // info about cell consumption
    std::uint64_t chip_cell_count = 0;
    std::vector<std::uint64_t> chip_cell_algorithmic_qubit = {};
    std::vector<double> chip_cell_algorithmic_qubit_ratio = {};
    std::vector<std::uint64_t> chip_cell_active_qubit_area = {};
    std::vector<double> chip_cell_active_qubit_area_ratio = {};
    std::uint64_t qubit_volume = 0;

    // info about QEC resource estimation
    std::uint64_t code_distance = 0;
    double execution_time_sec = 0.0;
    std::uint64_t num_physical_qubits = 0;

    template <typename T>
    static std::tuple<double, T> CalcAveAndPeak(const std::vector<T>& vec) {
        if (vec.empty()) {
            return {0.0, T{0}};
        }

        auto sum = T{0};
        auto peak = T{0};
        for (const auto& x : vec) {
            sum += x;
            peak = std::max(peak, x);
        }
        return {static_cast<double>(sum) / static_cast<double>(vec.size()), peak};
    }

    double GateThroughputAve() const;
    std::uint64_t GateThroughputPeak() const;
    double MeasurementFeedbackRateAve() const;
    std::uint64_t MeasurementFeedbackRatePeak() const;
    double MagicStateConsumptionRateAve() const;
    std::uint64_t MagicStateConsumptionRatePeak() const;
    double EntanglementConsumptionRateAve() const;
    std::uint64_t EntanglementConsumptionRatePeak() const;
    double ChipCellAlgorithmicQubitAve() const;
    std::uint64_t ChipCellAlgorithmicQubitPeak() const;
    double ChipCellAlgorithmicQubitRatioAve() const;
    double ChipCellAlgorithmicQubitRatioPeak() const;
    double ChipCellActiveQubitAreaAve() const;
    std::uint64_t ChipCellActiveQubitAreaPeak() const;
    double ChipCellActiveQubitAreaRatioAve() const;
    double ChipCellActiveQubitAreaRatioPeak() const;

    ::qret::Json Json() const override;
    std::string Markdown() const override;
};

void QRET_EXPORT to_json(Json& j, const ScLsFixedV0CompileInfo& info);
void QRET_EXPORT from_json(const Json& j, ScLsFixedV0CompileInfo& info);
QRET_EXPORT std::ostream& operator<<(std::ostream& out, const ScLsFixedV0CompileInfo& info);
}  // namespace qret::sc_ls_fixed_v0

#endif  // QRET_TARGET_SC_LS_FIXED_V0_COMPILE_INFO_H
