/**
 * @file qret/target/compile_backend_context.h
 * @brief Compile backend context interfaces.
 */

#ifndef QRET_TARGET_COMPILE_BACKEND_CONTEXT_H
#define QRET_TARGET_COMPILE_BACKEND_CONTEXT_H

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "qret/qret_export.h"

namespace qret {
/**
 * @brief Common pass configuration consumed by target compile backends.
 */
struct PassConfig {
    std::string arg;  //!< Pass argument.
    std::string cmd;  //!< External pass command.
    std::string input;  //!< External pass input file.
    std::string output;  //!< External pass output file.
    std::string runner;  //!< External pass runner.

    PassConfig() = default;
    explicit PassConfig(std::string arg)
        : arg(std::move(arg)) {}
    PassConfig(
            std::string arg,
            std::string cmd,
            std::string input,
            std::string output,
            std::string runner
    )
        : arg(std::move(arg))
        , cmd(std::move(cmd))
        , input(std::move(input))
        , output(std::move(output))
        , runner(std::move(runner)) {}

    [[nodiscard]] bool IsExternalPass() const {
        return !cmd.empty();
    }
};

class QRET_EXPORT CompileOptionRegistrar {
public:
    virtual ~CompileOptionRegistrar() = default;

    // Target backends register option definitions through this interface
    // and remain independent from concrete frontends (CLI/GUI/YAML).
    virtual void AddFlagOption(std::string_view name, std::string_view description) = 0;
    virtual void AddStringOption(
            std::string_view name,
            std::string_view value_name,
            std::string_view description
    ) = 0;
    virtual void AddStringOptionWithDefault(
            std::string_view name,
            std::string_view default_value,
            std::string_view value_name,
            std::string_view description
    ) = 0;
    virtual void AddUInt64Option(
            std::string_view name,
            std::uint64_t default_value,
            std::string_view description
    ) = 0;
    virtual void
    AddDoubleOption(std::string_view name, double default_value, std::string_view description) = 0;
};

class QRET_EXPORT CompileOptionReader {
public:
    virtual ~CompileOptionReader() = default;

    // Target backends only read this interface.
    // The caller adapts concrete sources such as CLI/YAML/Python.
    [[nodiscard]] virtual bool Contains(std::string_view key) const = 0;
    [[nodiscard]] virtual std::string GetString(std::string_view key) const = 0;
    [[nodiscard]] virtual std::string
    GetString(std::string_view key, std::string_view default_value) const = 0;
    [[nodiscard]] virtual std::uint64_t GetUInt64(std::string_view key) const = 0;
    [[nodiscard]] virtual std::uint64_t
    GetUInt64(std::string_view key, std::uint64_t default_value) const = 0;
    [[nodiscard]] virtual double GetDouble(std::string_view key, double default_value) const = 0;
    [[nodiscard]] virtual std::vector<PassConfig> GetPassConfigList(std::string_view key) const = 0;
};
}  // namespace qret

#endif  // QRET_TARGET_COMPILE_BACKEND_CONTEXT_H
