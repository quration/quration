/**
 * @file qret/target/sc_ls_fixed_v0/sc_ls_fixed_v0_target_machine.h
 * @brief Target machines of sc_ls_fixed_v0.
 */

#ifndef QRET_TARGET_SC_LS_FIXED_V0_SC_LS_FIXED_V0_TARGET_MACHINE_H
#define QRET_TARGET_SC_LS_FIXED_V0_SC_LS_FIXED_V0_TARGET_MACHINE_H

#include <nlohmann/adl_serializer.hpp>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include "qret/base/json.h"
#include "qret/qret_export.h"
#include "qret/target/sc_ls_fixed_v0/topology.h"
#include "qret/target/target_machine.h"

namespace qret::sc_ls_fixed_v0 {
/**
 * @brief Machine type.
 */
enum class ScLsFixedV0MachineType : std::uint8_t {
    Dim2,
    Dim3,
    DistributedDim2,
    DistributedDim3,  // Currently unsupported.
};

static inline std::string ToString(ScLsFixedV0MachineType type) {
    switch (type) {
        case ScLsFixedV0MachineType::Dim2:
            return "Dim2";
        case ScLsFixedV0MachineType::Dim3:
            return "Dim3";
        case ScLsFixedV0MachineType::DistributedDim2:
            return "DistributedDim2";
        case ScLsFixedV0MachineType::DistributedDim3:
            return "DistributedDim3";
        default:
            break;
    }
    throw std::runtime_error("unknown ScLsFixedV0MachineType");
}

static inline ScLsFixedV0MachineType ScLsFixedV0MachineTypeFromString(const std::string& str) {
    if (str == "Dim2") {
        return ScLsFixedV0MachineType::Dim2;
    }
    if (str == "Dim3") {
        return ScLsFixedV0MachineType::Dim3;
    }
    if (str == "DistributedDim2") {
        return ScLsFixedV0MachineType::DistributedDim2;
    }
    if (str == "DistributedDim3") {
        return ScLsFixedV0MachineType::DistributedDim3;
    }
    throw std::runtime_error(fmt::format("{} is not ScLsFixedV0MachineType.", str));
}

static inline bool
IsCompatible(ScLsFixedV0MachineType candidate, ScLsFixedV0MachineType requirement) {
    using ScLsFixedV0MachineType::Dim2;
    using ScLsFixedV0MachineType::Dim3;
    using ScLsFixedV0MachineType::DistributedDim2;
    using ScLsFixedV0MachineType::DistributedDim3;
    switch (requirement) {
        case Dim2:
            return candidate == Dim2 || candidate == DistributedDim2 || candidate == Dim3
                    || candidate == DistributedDim3;
        case Dim3:
            return candidate == Dim3 || candidate == DistributedDim3;
        case DistributedDim2:
            return candidate == DistributedDim2 || candidate == DistributedDim3;
        case DistributedDim3:
            return candidate == DistributedDim3;
        default:
            return false;
    }
}

inline ScLsFixedV0MachineType GetMachineType(const Topology& topology) {
    if (topology.NumGrids() >= 2) {
        for (const auto& grid : topology) {
            if (!grid.IsPlane()) {
                return ScLsFixedV0MachineType::DistributedDim3;
            }
        }
        return ScLsFixedV0MachineType::DistributedDim2;
    }

    const auto& grid = topology.GetGridByIndex(0);
    return grid.IsPlane() ? ScLsFixedV0MachineType::Dim2 : ScLsFixedV0MachineType::Dim3;
}

/**
 * @brief Machine option.
 */
struct QRET_EXPORT ScLsFixedV0MachineOption {
    ScLsFixedV0MachineType type = ScLsFixedV0MachineType::Dim2;  //!< Type of backend machine.
    bool enable_pbc_mode = false;  //!< Enable Pauli Based Computing lowering mode.
    bool use_magic_state_cultivation =
            false;  // If true, magic state factories are simulated using the cultivation method.
    std::uint64_t magic_factory_seed_offset = 0;
    std::uint64_t magic_generation_period = 0;  //!< Beats to generate a magic state
    double prob_magic_state_creation = 1.0;  //!< Success probability of creating a magic state.
    std::uint64_t maximum_magic_state_stock =
            0;  //!< The number of magic states that can be stored in a magic state factory
    std::uint64_t entanglement_generation_period = 0;  //!< Beats to generate a logical entanglement
    std::uint64_t maximum_entangled_state_stock =
            0;  //!< The number of entanglement states that can be stored in an entanglement state
                //!< factory
    std::uint64_t reaction_time =
            0;  //!< Beats it takes for the measured value to be error-corrected
    double physical_error_rate = 0.0;  //!< Physical error rate (p)
    double drop_rate = 0.0;  //!< Drop rate (Lambda)
    double code_cycle_time_sec = 0.0;  //!< Code cycle time in seconds (t_cycle)
    double allowed_failure_prob = 0.0;  //!< Allowed failure probability (eps)
};

inline void to_json(Json& j, const ScLsFixedV0MachineOption& option) {
    j["type"] = ToString(option.type);
    j["enable_pbc_mode"] = option.enable_pbc_mode;
    j["use_magic_state_cultivation"] = option.use_magic_state_cultivation;
    j["magic_factory_seed_offset"] = option.magic_factory_seed_offset;
    j["magic_generation_period"] = option.magic_generation_period;
    j["prob_magic_state_creation"] = option.prob_magic_state_creation;
    j["maximum_magic_state_stock"] = option.maximum_magic_state_stock;
    j["entanglement_generation_period"] = option.entanglement_generation_period;
    j["maximum_entangled_state_stock"] = option.maximum_entangled_state_stock;
    j["reaction_time"] = option.reaction_time;
    j["physical_error_rate"] = option.physical_error_rate;
    j["drop_rate"] = option.drop_rate;
    j["code_cycle_time_sec"] = option.code_cycle_time_sec;
    j["allowed_failure_prob"] = option.allowed_failure_prob;
}

inline void from_json(const Json& j, ScLsFixedV0MachineOption& option) {
    option.type = ScLsFixedV0MachineTypeFromString(j.at("type").get<std::string>());
    if (j.contains("enable_pbc_mode")) {
        j.at("enable_pbc_mode").get_to(option.enable_pbc_mode);
    }
    j.at("use_magic_state_cultivation").get_to(option.use_magic_state_cultivation);
    j.at("magic_factory_seed_offset").get_to(option.magic_factory_seed_offset);
    j.at("magic_generation_period").get_to(option.magic_generation_period);
    j.at("prob_magic_state_creation").get_to(option.prob_magic_state_creation);
    j.at("maximum_magic_state_stock").get_to(option.maximum_magic_state_stock);
    j.at("entanglement_generation_period").get_to(option.entanglement_generation_period);
    j.at("maximum_entangled_state_stock").get_to(option.maximum_entangled_state_stock);
    j.at("reaction_time").get_to(option.reaction_time);
    if (j.contains("physical_error_rate")) {
        j.at("physical_error_rate").get_to(option.physical_error_rate);
    }
    if (j.contains("drop_rate")) {
        j.at("drop_rate").get_to(option.drop_rate);
    }
    if (j.contains("code_cycle_time_sec")) {
        j.at("code_cycle_time_sec").get_to(option.code_cycle_time_sec);
    }
    if (j.contains("allowed_failure_prob")) {
        j.at("allowed_failure_prob").get_to(option.allowed_failure_prob);
    }
}

struct ScLsFixedV0TargetMachine : public TargetMachine {
    std::shared_ptr<const Topology> topology;
    ScLsFixedV0MachineOption machine_option;

    static inline auto Name = std::string("SC_LS_FIXED_V0Machine");
    [[nodiscard]] std::string_view GetName() const override {
        return Name;
    }

    static std::unique_ptr<ScLsFixedV0TargetMachine> New(
            const std::shared_ptr<const Topology>& topology,
            const ScLsFixedV0MachineOption& machine_option
    ) {
        return std::unique_ptr<ScLsFixedV0TargetMachine>(
                new ScLsFixedV0TargetMachine(topology, machine_option)
        );
    }
    ScLsFixedV0TargetMachine() = default;
    ScLsFixedV0TargetMachine(
            const std::shared_ptr<const Topology>& topology,
            const ScLsFixedV0MachineOption& machine_option
    )
        : topology{topology}
        , machine_option{machine_option} {}
};

inline void to_json(Json& j, const ScLsFixedV0TargetMachine& machine) {
    j["topology"] = *machine.topology;
    j["machine_option"] = machine.machine_option;
}

inline void from_json(const Json& j, ScLsFixedV0TargetMachine& machine) {
    machine.topology = Topology::FromJSON(j.at("topology"));
    j.at("machine_option").get_to(machine.machine_option);
}
}  // namespace qret::sc_ls_fixed_v0

#endif  // QRET_TARGET_SC_LS_FIXED_V0_SC_LS_FIXED_V0_TARGET_MACHINE_H
