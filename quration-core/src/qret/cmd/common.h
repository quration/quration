/**
 * @file qret/cmd/common.h
 * @brief Common in cmd module.
 */

#ifndef QRET_CMD_COMMON_H
#define QRET_CMD_COMMON_H

#include <unordered_set>
#include <filesystem>
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


/*
 * Keeps track of which key is a potential relative path.
 */
class PathKeyRegistry {
public:
	static PathKeyRegistry &Instance() {
		static PathKeyRegistry instance;
		return instance;
	}
	void Register(std::string key) { keys_.insert(std::move(key)); }
	bool IsPathKey(const std::string &key) const { return keys_.contains(key); }

private:
	std::unordered_set<std::string> keys_;
};

/* 
 * Absolute paths are returned as-is (normalized only); relative paths are
 * joined against the YAML file's directory. We branch explicitly rather
 * than relying on operator/'s absolute-path behavior, to keep intent clear.
 */
static std::string ResolveRelativePaths( const YAML::Node &node, const std::filesystem::path &base_dir) {
	auto node_path = std::filesystem::path(node.Scalar());
	if (node_path.is_relative())
		return (base_dir / node_path).lexically_normal().string();
	else
		return node_path.lexically_normal().string();
}

/**
 * @brief Variables map.
 */
struct VariablesMap {
    boost::program_options::variables_map vm;

	void LoadYamlPipeline(const std::string &path) {
		yaml = YAML::LoadFile(path);
		yaml_base_dir_ = std::filesystem::path(path).parent_path();
	}

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

	inline std::vector<PassConfig> ParsePass(const std::string& key) const {
		if (!Contains(key)) {
			return {};
		}

		auto ret = std::vector<PassConfig>{};
		if (yaml.has_value()) {
			const auto& yaml_ = yaml.value()[key];
			ParsePassFromYAML(ret, yaml_);
		} else {
			const auto pass_arg = SplitString(Get<std::string>(key), ',');
			for (const auto& arg : pass_arg) {
				ret.emplace_back(arg);
				assert(!ret.back().IsExternalPass());
			}
		}
		return ret;
	}

private:
    std::optional<YAML::Node> yaml = std::nullopt;
	std::optional<std::filesystem::path> yaml_base_dir_ = std::nullopt;

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

				const auto &node = (*yaml)[key];
				if (yaml_base_dir_.has_value() && PathKeyRegistry::Instance().IsPathKey(key))
					return ResolveRelativePaths(node, yaml_base_dir_.value());
                return node.Scalar();
            } else {
                return (*yaml)[key].as<T>();
            }
        }
        return vm[key].as<T>();
    }
};
}  // namespace qret::cmd

#endif  // QRET_CMD_COMMON_H
