/**
 * @file qret/cmd/common.h
 * @brief Common in cmd module.
 */

#ifndef QRET_CMD_COMMON_H
#define QRET_CMD_COMMON_H

#include <boost/program_options.hpp>
#include <boost/program_options/variables_map.hpp>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/yaml.h>

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>

#include "qret/base/string.h"

namespace qret::cmd {
/**
 * @brief Return status.
 */
enum class ReturnStatus : std::uint8_t { Success = 0, Failure = 1 };

/**
 * @brief Base class of sub-commands.
 */
class SubCommand {
public:
    virtual ~SubCommand() = default;

    /**
     * @brief Main function of sub command.
     *
     * @param argc argument count
     * @param argv argument vector
     * @return ReturnStatus return status
     */
    virtual ReturnStatus Main(int argc, const char** argv) = 0;

    /**
     * @brief Get the name of sub-command.
     *
     * @return std::string name
     */
    virtual std::string Name() const = 0;
};

/**
 * @brief Variables map.
 */
struct VariablesMap {
    boost::program_options::variables_map vm;
    std::optional<YAML::Node> yaml = std::nullopt;

    bool Contains(const std::string& key) const {
        if (UseYAML()) {
            if (key == "function") {
                return (*yaml)["function"].IsDefined() || (*yaml)["circuit"].IsDefined();
            }
            return (*yaml)[key].IsDefined();
        }
        return vm.count(key) > 0;
    }
    template <typename T>
    T Get(const std::string& key) const {
        if (!Contains(key)) {
            throw std::runtime_error(fmt::format("key '{}' is not set", key));
        }

        return GetImpl<T>(key);
    }
    template <typename T>
    T Get(const std::string& key, const T& value) const {
        // 'value' : default value
        if (!Contains(key)) {
            return value;
        }

        return GetImpl<T>(key);
    }

private:
    bool UseYAML() const {
        return yaml.has_value();
    }
    template <typename T>
    T GetImpl(const std::string& key) const {
        if (UseYAML()) {
            if constexpr (std::is_same_v<T, std::string>) {
                if (key == "function" && !(*yaml)["function"].IsDefined()
                    && (*yaml)["circuit"].IsDefined()) {
                    return (*yaml)["circuit"].Scalar();
                }
                return (*yaml)[key].Scalar();
            } else {
                return (*yaml)[key].as<T>();
            }
        }
        return vm[key].as<T>();
    }
};

/**
 * @brief Config of pass.
 */
struct PassConfig {
    std::string arg;  // pass argument
    std::string cmd;
    std::string input;
    std::string output;
    std::string runner;

    bool IsExternalPass() const {
        return !cmd.empty();
    }
};

inline void ParsePassFromYAML(std::vector<PassConfig>& out, const YAML::Node& yaml) {
    if (!yaml.IsSequence()) {
        throw std::runtime_error("pass field must be sequence");
    }

    for (const auto& pass_config : yaml) {
        if (pass_config.IsScalar()) {
            out.emplace_back(pass_config.Scalar());
        } else {
            const auto get = [&pass_config](const std::string& key) -> std::string {
                return pass_config[key] ? pass_config[key].Scalar() : "";
            };
            out.emplace_back(get("name"), get("cmd"), get("input"), get("output"), get("runner"));
        }
    }
}

inline std::vector<PassConfig> ParsePass(const VariablesMap& vm, const std::string& key) {
    if (!vm.Contains(key)) {
        return {};
    }

    auto ret = std::vector<PassConfig>{};
    if (vm.yaml.has_value()) {
        const auto& yaml = vm.yaml.value()[key];
        ParsePassFromYAML(ret, yaml);
    } else {
        const auto pass_arg = SplitString(vm.Get<std::string>(key), ',');
        for (const auto& arg : pass_arg) {
            ret.emplace_back(arg);
            assert(!ret.back().IsExternalPass());
        }
    }
    return ret;
}
}  // namespace qret::cmd

#endif  // QRET_CMD_COMMON_H
