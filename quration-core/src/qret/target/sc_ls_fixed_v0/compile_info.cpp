/**
 * @file qret/target/sc_ls_fixed_v0/compile_info.cpp
 * @brief Compile information.
 */

#include "qret/target/sc_ls_fixed_v0/compile_info.h"

#include <fmt/core.h>

#include <cassert>

#include "qret/target/sc_ls_fixed_v0/instruction.h"
#include "qret/target/sc_ls_fixed_v0/topology.h"

namespace qret::sc_ls_fixed_v0 {
double ScLsFixedV0CompileInfo::GateThroughputAve() const {
    return std::get<0>(CalcAveAndPeak(gate_throughput));
}
std::uint64_t ScLsFixedV0CompileInfo::GateThroughputPeak() const {
    return std::get<1>(CalcAveAndPeak(gate_throughput));
}
double ScLsFixedV0CompileInfo::MeasurementFeedbackRateAve() const {
    return std::get<0>(CalcAveAndPeak(measurement_feedback_rate));
}
std::uint64_t ScLsFixedV0CompileInfo::MeasurementFeedbackRatePeak() const {
    return std::get<1>(CalcAveAndPeak(measurement_feedback_rate));
}
double ScLsFixedV0CompileInfo::MagicStateConsumptionRateAve() const {
    return std::get<0>(CalcAveAndPeak(magic_state_consumption_rate));
}
std::uint64_t ScLsFixedV0CompileInfo::MagicStateConsumptionRatePeak() const {
    return std::get<1>(CalcAveAndPeak(magic_state_consumption_rate));
}
double ScLsFixedV0CompileInfo::EntanglementConsumptionRateAve() const {
    return std::get<0>(CalcAveAndPeak(entanglement_consumption_rate));
}
std::uint64_t ScLsFixedV0CompileInfo::EntanglementConsumptionRatePeak() const {
    return std::get<1>(CalcAveAndPeak(entanglement_consumption_rate));
}
double ScLsFixedV0CompileInfo::ChipCellAlgorithmicQubitAve() const {
    return std::get<0>(CalcAveAndPeak(chip_cell_algorithmic_qubit));
}
std::uint64_t ScLsFixedV0CompileInfo::ChipCellAlgorithmicQubitPeak() const {
    return std::get<1>(CalcAveAndPeak(chip_cell_algorithmic_qubit));
}
double ScLsFixedV0CompileInfo::ChipCellAlgorithmicQubitRatioAve() const {
    return std::get<0>(CalcAveAndPeak(chip_cell_algorithmic_qubit_ratio));
}
double ScLsFixedV0CompileInfo::ChipCellAlgorithmicQubitRatioPeak() const {
    return std::get<1>(CalcAveAndPeak(chip_cell_algorithmic_qubit_ratio));
}
double ScLsFixedV0CompileInfo::ChipCellActiveQubitAreaAve() const {
    return std::get<0>(CalcAveAndPeak(chip_cell_active_qubit_area));
}
std::uint64_t ScLsFixedV0CompileInfo::ChipCellActiveQubitAreaPeak() const {
    return std::get<1>(CalcAveAndPeak(chip_cell_active_qubit_area));
}
double ScLsFixedV0CompileInfo::ChipCellActiveQubitAreaRatioAve() const {
    return std::get<0>(CalcAveAndPeak(chip_cell_active_qubit_area_ratio));
}
double ScLsFixedV0CompileInfo::ChipCellActiveQubitAreaRatioPeak() const {
    return std::get<1>(CalcAveAndPeak(chip_cell_active_qubit_area_ratio));
}
::qret::Json ScLsFixedV0CompileInfo::Json() const {
    auto j = ::qret::Json();

    const auto to_json_time_series = [&j](const auto& vec, const std::string& key) {
        const auto [ave, peak] = ScLsFixedV0CompileInfo::CalcAveAndPeak(vec);
        j[key] = vec;
        j[key + "_ave"] = ave;
        j[key + "_peak"] = peak;
    };

    // about constants
    j["use_magic_state_cultivation"] = use_magic_state_cultivation;
    j["magic_factory_seed_offset"] = magic_factory_seed_offset;
    j["magic_generation_period"] = magic_generation_period;
    j["prob_magic_state_creation"] = prob_magic_state_creation;
    j["maximum_magic_state_stock"] = maximum_magic_state_stock;
    j["entanglement_generation_period"] = entanglement_generation_period;
    j["maximum_entangled_state_stock"] = maximum_entangled_state_stock;
    j["reaction_time"] = reaction_time;
    if (topology) {
        j["topology"] = *topology;
    }

    // about runtime
    j["runtime"] = runtime;
    j["runtime_without_topology"] = runtime_without_topology;

    // about gate
    j["gate_count"] = gate_count;
    j["gate_count_detail"] = ::qret::Json();
    for (const auto& [type, count] : gate_count_dict) {
        j["gate_count_detail"][ToString(type)] = count;
    }
    j["gate_depth"] = gate_depth;
    to_json_time_series(gate_throughput, "gate_throughput");

    // about measurement depth
    j["measurement_feedback_count"] = measurement_feedback_count;
    j["measurement_feedback_depth"] = measurement_feedback_depth;
    to_json_time_series(measurement_feedback_rate, "measurement_feedback_rate");
    j["runtime_estimation_measurement_feedback_count"] =
            runtime_estimation_measurement_feedback_count;
    j["runtime_estimation_measurement_feedback_depth"] =
            runtime_estimation_measurement_feedback_depth;

    // about magic state consumption
    j["magic_state_consumption_count"] = magic_state_consumption_count;
    j["magic_state_consumption_depth"] = magic_state_consumption_depth;
    to_json_time_series(magic_state_consumption_rate, "magic_state_consumption_rate");
    j["runtime_estimation_magic_state_consumption_count"] =
            runtime_estimation_magic_state_consumption_count;
    j["runtime_estimation_magic_state_consumption_depth"] =
            runtime_estimation_magic_state_consumption_depth;
    j["magic_factory_count"] = magic_factory_count;

    // about entanglement consumption
    j["entanglement_consumption_count"] = entanglement_consumption_count;
    j["entanglement_consumption_depth"] = entanglement_consumption_depth;
    to_json_time_series(entanglement_consumption_rate, "entanglement_consumption_rate");
    j["runtime_estimation_entanglement_consumption_count"] =
            runtime_estimation_entanglement_consumption_count;
    j["runtime_estimation_entanglement_consumption_depth"] =
            runtime_estimation_entanglement_consumption_depth;
    j["entanglement_factory_count"] = entanglement_factory_count;

    // about cell consumption
    j["chip_cell_count"] = chip_cell_count;
    to_json_time_series(chip_cell_algorithmic_qubit, "chip_cell_algorithmic_qubit");
    to_json_time_series(chip_cell_algorithmic_qubit_ratio, "chip_cell_algorithmic_qubit_ratio");
    to_json_time_series(chip_cell_active_qubit_area, "chip_cell_active_qubit_area");
    to_json_time_series(chip_cell_active_qubit_area_ratio, "chip_cell_active_qubit_area_ratio");
    j["qubit_volume"] = qubit_volume;

    // about QEC resource estimation
    j["code_distance"] = code_distance;
    j["execution_time_sec"] = execution_time_sec;
    j["num_physical_qubits"] = num_physical_qubits;
    return j;
}
std::string ScLsFixedV0CompileInfo::Markdown() const {
    const auto* prefix = "- ";
    const auto* indent = "  ";
    auto ss = std::stringstream();
    ss << "# Compile information of SC_LS_FIXED_V0\n\n";
    ss << "## Constant\n\n";
    ss << prefix << "magic_generation_period: " << magic_generation_period << '\n';
    ss << prefix << "maximum_magic_state_stock: " << maximum_magic_state_stock << '\n';
    ss << prefix << "entanglement_generation_period: " << entanglement_generation_period << '\n';
    ss << prefix << "maximum_entangled_state_stock: " << maximum_entangled_state_stock << '\n';
    ss << prefix << "reaction_time: " << reaction_time << '\n';
    for (const auto& grid : *topology) {
        if (grid.IsPlane()) {
            ss << prefix << fmt::format("plane (z: {}, index: {})", grid.GetMinZ(), grid.GetIndex())
               << '\n';
            ss << indent << prefix << "chip_x: " << grid.GetMaxX() << '\n';
            ss << indent << prefix << "chip_y: " << grid.GetMaxY() << '\n';
        } else {
            ss << prefix
               << fmt::format(
                          "grid (z: [{}, {}], index: {})",
                          grid.GetMinZ(),
                          grid.GetMaxZ(),
                          grid.GetIndex()
                  )
               << '\n';
            ss << indent << prefix << "chip_x: " << grid.GetMaxX() << '\n';
            ss << indent << prefix << "chip_y: " << grid.GetMaxY() << '\n';
        }
    }
    ss << '\n';

    ss << "## Runtime\n\n";
    ss << prefix << "runtime: " << runtime << '\n';
    ss << prefix << "runtime without topology: " << runtime_without_topology << '\n';
    ss << '\n';

    ss << "## Gate\n\n";
    ss << prefix << "gate count: " << gate_count << '\n';
    ss << prefix << "gate count per instruction type\n";
    for (const auto [type, count] : gate_count_dict) {
        ss << indent << prefix << "gate count of " << ToString(type) << ": " << count << '\n';
    }
    ss << prefix << "gate depth: " << gate_depth << '\n';
    ss << prefix << "gate_throughput [ave]: " << GateThroughputAve() << '\n';
    ss << prefix << "gate_throughput [peak]: " << GateThroughputPeak() << '\n';
    ss << '\n';

    ss << "## Measurement depth\n\n";
    ss << prefix << "measurement feedback count: " << measurement_feedback_count << '\n';
    ss << prefix << "measurement feedback depth: " << measurement_feedback_depth << '\n';
    ss << prefix << "measurement feedback rate [ave]: " << MeasurementFeedbackRateAve() << '\n';
    ss << prefix << "measurement feedback rate [peak]: " << MeasurementFeedbackRatePeak() << '\n';
    ss << prefix << "runtime estimation measurement feedback count: "
       << runtime_estimation_measurement_feedback_count << '\n';
    ss << prefix << "runtime estimation measurement feedback depth: "
       << runtime_estimation_measurement_feedback_depth << '\n';
    ss << '\n';

    ss << "## Magic state consumption\n\n";
    ss << prefix << "magic state consumption count: " << magic_state_consumption_count << '\n';
    ss << prefix << "magic state consumption depth: " << magic_state_consumption_depth << '\n';
    ss << prefix << "magic state consumption rate [ave]: " << MagicStateConsumptionRateAve()
       << '\n';
    ss << prefix << "magic state consumption rate [peak]: " << MagicStateConsumptionRatePeak()
       << '\n';
    ss << prefix << "runtime estimation magic state consumption count: "
       << runtime_estimation_magic_state_consumption_count << '\n';
    ss << prefix << "runtime estimation magic state consumption depth: "
       << runtime_estimation_magic_state_consumption_depth << '\n';
    ss << prefix << "magic factory count: " << magic_factory_count << '\n';
    ss << '\n';

    ss << "## Entanglement consumption\n\n";
    ss << prefix << "entanglement consumption count: " << entanglement_consumption_count << '\n';
    ss << prefix << "entanglement consumption depth: " << entanglement_consumption_depth << '\n';
    ss << prefix << "entanglement consumption rate [ave]: " << EntanglementConsumptionRateAve()
       << '\n';
    ss << prefix << "entanglement consumption rate [peak]: " << EntanglementConsumptionRatePeak()
       << '\n';
    ss << prefix << "runtime estimation entanglement consumption count: "
       << runtime_estimation_entanglement_consumption_count << '\n';
    ss << prefix << "runtime estimation entanglement consumption depth: "
       << runtime_estimation_entanglement_consumption_depth << '\n';
    ss << prefix << "entanglement factory count: " << entanglement_factory_count << '\n';
    ss << '\n';

    ss << "## Cell consumption\n\n";
    ss << prefix << "chip cell count: " << chip_cell_count << '\n';
    ss << prefix << "chip cell algorithmic qubit [ave]: " << ChipCellAlgorithmicQubitAve() << '\n';
    ss << prefix << "chip cell algorithmic qubit [peak]: " << ChipCellAlgorithmicQubitPeak()
       << '\n';
    ss << prefix
       << "chip cell algorithmic qubit ratio [ave]: " << ChipCellAlgorithmicQubitRatioAve() << '\n';
    ss << prefix
       << "chip cell algorithmic qubit ratio [peak]: " << ChipCellAlgorithmicQubitRatioPeak()
       << '\n';
    ss << prefix << "chip cell active qubit area [ave]: " << ChipCellActiveQubitAreaAve() << '\n';
    ss << prefix << "chip cell active qubit area [peak]: " << ChipCellActiveQubitAreaPeak() << '\n';
    ss << prefix << "chip cell active qubit area ratio [ave]: " << ChipCellActiveQubitAreaRatioAve()
       << '\n';
    ss << prefix
       << "chip cell active qubit area ratio [peak]: " << ChipCellActiveQubitAreaRatioPeak()
       << '\n';
    ss << prefix << "qubit volume: " << qubit_volume << '\n';

    ss << "## QEC Resource Estimation\n\n";
    ss << prefix << "code distance: " << code_distance << '\n';
    ss << prefix << "execution time (sec): " << execution_time_sec << '\n';
    ss << prefix << "num physical qubits: " << num_physical_qubits;
    return ss.str();
}
void to_json(Json& j, const ScLsFixedV0CompileInfo& info) {
    j = info.Json();
}
void from_json(const Json& j, ScLsFixedV0CompileInfo& info) {
    // about constants
    j["magic_generation_period"].get_to(info.magic_generation_period);
    j["maximum_magic_state_stock"].get_to(info.maximum_magic_state_stock);
    j["entanglement_generation_period"].get_to(info.entanglement_generation_period);
    j["maximum_entangled_state_stock"].get_to(info.maximum_entangled_state_stock);
    j["reaction_time"].get_to(info.reaction_time);
    if (j.contains("topology")) {
        info.topology = Topology::FromJSON(j["topology"]);
    }

    // about runtime
    j["runtime"].get_to(info.runtime);
    j["runtime_without_topology"].get_to(info.runtime_without_topology);

    // about gate
    j["gate_count"].get_to(info.gate_count);
    j["gate_depth"].get_to(info.gate_depth);
    for (const auto& [type, count] : j["gate_count_detail"].items()) {
        info.gate_count_dict[ScLsInstructionTypeFromString(type)] =
                count.template get<std::uint64_t>();
    }
    j["gate_throughput"].get_to(info.gate_throughput);

    // about measurement depth
    j["measurement_feedback_count"].get_to(info.measurement_feedback_count);
    j["measurement_feedback_depth"].get_to(info.measurement_feedback_depth);
    j["measurement_feedback_rate"].get_to(info.measurement_feedback_rate);
    j["runtime_estimation_measurement_feedback_count"].get_to(
            info.runtime_estimation_measurement_feedback_count
    );
    j["runtime_estimation_measurement_feedback_depth"].get_to(
            info.runtime_estimation_measurement_feedback_depth
    );

    // about magic state consumption
    j["magic_state_consumption_count"].get_to(info.magic_state_consumption_count);
    j["magic_state_consumption_depth"].get_to(info.magic_state_consumption_depth);
    j["magic_state_consumption_rate"].get_to(info.magic_state_consumption_rate);
    j["runtime_estimation_magic_state_consumption_count"].get_to(
            info.runtime_estimation_magic_state_consumption_count
    );
    j["runtime_estimation_magic_state_consumption_depth"].get_to(
            info.runtime_estimation_magic_state_consumption_depth
    );
    j["magic_factory_count"].get_to(info.magic_factory_count);

    // about entanglement consumption
    j["entanglement_consumption_count"].get_to(info.entanglement_consumption_count);
    j["entanglement_consumption_depth"].get_to(info.entanglement_consumption_depth);
    j["entanglement_consumption_rate"].get_to(info.entanglement_consumption_rate);
    j["runtime_estimation_entanglement_consumption_count"].get_to(
            info.runtime_estimation_entanglement_consumption_count
    );
    j["runtime_estimation_entanglement_consumption_depth"].get_to(
            info.runtime_estimation_entanglement_consumption_depth
    );
    j["entanglement_factory_count"].get_to(info.entanglement_factory_count);

    // about cell consumption
    j["chip_cell_count"].get_to(info.chip_cell_count);
    j["chip_cell_algorithmic_qubit"].get_to(info.chip_cell_algorithmic_qubit);
    j["chip_cell_algorithmic_qubit_ratio"].get_to(info.chip_cell_algorithmic_qubit_ratio);
    j["chip_cell_active_qubit_area"].get_to(info.chip_cell_active_qubit_area);
    j["chip_cell_active_qubit_area_ratio"].get_to(info.chip_cell_active_qubit_area_ratio);
    j["qubit_volume"].get_to(info.qubit_volume);

    // about QEC resource estimation
    if (j.contains("code_distance")) {
        j["code_distance"].get_to(info.code_distance);
    }
    if (j.contains("execution_time_sec")) {
        j["execution_time_sec"].get_to(info.execution_time_sec);
    }
    if (j.contains("num_physical_qubits")) {
        j["num_physical_qubits"].get_to(info.num_physical_qubits);
    }
}
std::ostream& operator<<(std::ostream& out, const ScLsFixedV0CompileInfo& info) {
    const auto* indent = "  ";
    out << "about constant\n"
        << indent << "use_magic_state_cultivation: " << info.use_magic_state_cultivation << '\n'
        << indent << "magic_factory_seed_offset: " << info.magic_factory_seed_offset << '\n'
        << indent << "magic_generation_period: " << info.magic_generation_period << '\n'
        << indent << "prob_magic_state_creation: " << info.prob_magic_state_creation << '\n'
        << indent << "maximum_magic_state_stock: " << info.maximum_magic_state_stock << '\n'
        << indent << "entanglement_generation_period: " << info.entanglement_generation_period
        << '\n'
        << indent << "maximum_entangled_state_stock: " << info.maximum_entangled_state_stock << '\n'
        << indent << "reaction_time: " << info.reaction_time << '\n';
    for (const auto& grid : *info.topology) {
        if (grid.IsPlane()) {
            out << indent
                << fmt::format("plane (z: {}, index: {})", grid.GetMinZ(), grid.GetIndex()) << '\n';
            out << indent << "  chip_x: " << grid.GetMaxX() << '\n';
            out << indent << "  chip_y: " << grid.GetMaxY() << '\n';
        } else {
            out << indent
                << fmt::format(
                           "grid (z: [{}, {}], index: {})",
                           grid.GetMinZ(),
                           grid.GetMaxZ(),
                           grid.GetIndex()
                   )
                << '\n';
            out << indent << "  chip_x: " << grid.GetMaxX() << '\n';
            out << indent << "  chip_y: " << grid.GetMaxY() << '\n';
        }
    }
    out << "about runtime\n"
        << indent << "runtime: " << info.runtime << '\n'
        << indent << "runtime without topology: " << info.runtime_without_topology << '\n'
        << "about gate\n"
        << indent << "gate count: " << info.gate_count << '\n';
    for (const auto& [type, count] : info.gate_count_dict) {
        out << indent << "gate count " << ToString(type) << ": " << count << '\n';
    }
    out << indent << "gate depth: " << info.gate_depth << '\n'
        << indent << "gate throughput [ave]: " << info.GateThroughputAve() << '\n'
        << indent << "gate throughput [peak]: " << info.GateThroughputPeak() << '\n'
        << "about measurement depth\n"
        << indent << "measurement feedback count: " << info.measurement_feedback_count << '\n'
        << indent << "measurement feedback depth: " << info.measurement_feedback_depth << '\n'
        << indent << "measurement feedback rate [ave]: " << info.MeasurementFeedbackRateAve()
        << '\n'
        << indent << "measurement feedback rate [peak]: " << info.MeasurementFeedbackRatePeak()
        << '\n'
        << indent << "runtime estimation measurement feedback count: "
        << info.runtime_estimation_measurement_feedback_count << '\n'
        << indent << "runtime estimation measurement feedback depth: "
        << info.runtime_estimation_measurement_feedback_depth << '\n'
        << "about magic state consumption\n"
        << indent << "magic state consumption count: " << info.magic_state_consumption_count << '\n'
        << indent << "magic state consumption depth: " << info.magic_state_consumption_depth << '\n'
        << indent << "magic state consumption rate [ave]: " << info.MagicStateConsumptionRateAve()
        << '\n'
        << indent << "magic state consumption rate [peak]: " << info.MagicStateConsumptionRatePeak()
        << '\n'
        << indent << "runtime estimation magic state consumption count: "
        << info.runtime_estimation_magic_state_consumption_count << '\n'
        << indent << "runtime estimation magic state consumption depth: "
        << info.runtime_estimation_magic_state_consumption_depth << '\n'
        << indent << "magic factory count: " << info.magic_factory_count << '\n'
        << "about entanglement consumption\n"
        << indent << "entanglement consumption count: " << info.entanglement_consumption_count
        << '\n'
        << indent << "entanglement consumption depth: " << info.entanglement_consumption_depth
        << '\n'
        << indent
        << "entanglement consumption rate [ave]: " << info.EntanglementConsumptionRateAve() << '\n'
        << indent
        << "entanglement consumption rate [peak]: " << info.EntanglementConsumptionRatePeak()
        << '\n'
        << indent << "runtime estimation entanglement consumption count: "
        << info.runtime_estimation_entanglement_consumption_count << '\n'
        << indent << "runtime estimation entanglement consumption depth: "
        << info.runtime_estimation_entanglement_consumption_depth << '\n'
        << indent << "entanglement factory count: " << info.entanglement_factory_count << '\n'
        << "about cell consumption\n"
        << indent << "chip cell count: " << info.chip_cell_count << '\n'
        << indent << "chip cell algorithmic qubit [ave]: " << info.ChipCellAlgorithmicQubitAve()
        << '\n'
        << indent << "chip cell algorithmic qubit [peak]: " << info.ChipCellAlgorithmicQubitPeak()
        << '\n'
        << indent
        << "chip cell algorithmic qubit ratio [ave]: " << info.ChipCellAlgorithmicQubitRatioAve()
        << '\n'
        << indent
        << "chip cell algorithmic qubit ratio [peak]: " << info.ChipCellAlgorithmicQubitRatioPeak()
        << '\n'
        << indent << "chip cell active qubit area [ave]: " << info.ChipCellActiveQubitAreaAve()
        << '\n'
        << indent << "chip cell active qubit area [peak]: " << info.ChipCellActiveQubitAreaPeak()
        << '\n'
        << indent
        << "chip cell active qubit area ratio [ave]: " << info.ChipCellActiveQubitAreaRatioAve()
        << '\n'
        << indent
        << "chip cell active qubit area ratio [peak]: " << info.ChipCellActiveQubitAreaRatioPeak()
        << '\n'
        << indent << "qubit volume: " << info.qubit_volume << '\n'
        << "about QEC resource estimation\n"
        << indent << "code distance: " << info.code_distance << '\n'
        << indent << "execution time (sec): " << info.execution_time_sec << '\n'
        << indent << "num physical qubits: " << info.num_physical_qubits;
    return out;
}
}  // namespace qret::sc_ls_fixed_v0
