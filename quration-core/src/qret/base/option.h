/**
 * @file qret/base/option.h
 * @brief Easy way to add command line option.
 * @details This file provides an easy way to set the option values via the command line argument.
 */

#ifndef QRET_BASE_OPTION_H
#define QRET_BASE_OPTION_H

#include <fmt/format.h>

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>

#include "qret/qret_export.h"

namespace qret {
// Forward declarations
template <typename T>
class Option;

namespace impl {
template <typename... Args>
struct VariantGeneratorType {
    using StorableType = std::variant<Args...>;
    using PointerToValueType = std::variant<Option<Args>*...>;
};
using OptTypes = VariantGeneratorType<
        bool,
        char,
        std::int32_t,
        std::int64_t,
        std::uint32_t,
        std::uint64_t,
        std::string>;
using StorableType = OptTypes::StorableType;
}  // namespace impl

/**
 * @brief Concept to determine if it can be added to the OptionStorage class.
 */
template <typename T>
concept Storable = requires(const T& x) { impl::StorableType(x); };
/**
 * @brief Control whether -help shows this option.
 */
enum class OptionHidden : std::uint8_t {
    /**
     * @brief Option included in -help & -help-hidden
     */
    NotHidden,
    /**
     * @brief -help doesn't, but -help-hidden does
     */
    Hidden,
    /**
     * @brief Neither -help nor -help-hidden show this option
     */
    ReallyHidden,
};
/**
 * @brief Add the value of this class as an option to the command line argument.
 *
 * @tparam T Expected to fulfill the concept `Storable`.
 */
template <typename T>
class Option {
public:
    Option(const Option&) = delete;
    Option& operator=(const Option&) = delete;

    Option(std::string_view command,
           const T& default_value,
           std::string_view description,
           OptionHidden hidden)
        : command_(command)
        , value_(default_value)
        , default_value_(default_value)
        , description_(description)
        , hidden_(hidden) {};

    /**
     * @brief Get the command.
     */
    [[nodiscard]] std::string_view GetCommand() const {
        return command_;
    }
    /**
     * @brief Get the value set in the command line. If no value is set, get the default value.
     */
    [[nodiscard]] const T& GetValue() const {
        return value_;
    }
    /**
     * @brief Get the default value.
     */
    [[nodiscard]] const T& GetDefaultValue() const {
        return default_value_;
    }
    /**
     * @brief Get the description.
     */
    [[nodiscard]] std::string_view GetDescription() const {
        return description_;
    }
    /**
     * @brief Get the enum of OptionHidden.
     */
    [[nodiscard]] OptionHidden GetHiddenFlag() const {
        return hidden_;
    }
    /**
     * @brief Set the value of the option.
     */
    void SetValue(const T& v) {
        value_ = v;
    }

private:
    std::string_view command_;
    T value_;
    T default_value_;
    std::string_view description_;
    OptionHidden hidden_;
};
/**
 * @brief Manage options to be added to the command line argument.
 */
class QRET_EXPORT OptionStorage {
    using OptionType = impl::OptTypes::PointerToValueType;

    using Container = std::map<std::string_view, OptionType>;
    using Iterator = Container::iterator;
    using ConstIterator = Container::const_iterator;

public:
    OptionStorage() = default;
    ~OptionStorage() = default;
    OptionStorage(const OptionStorage&) = delete;
    OptionStorage& operator=(const OptionStorage&) = delete;
    OptionStorage(OptionStorage&&) = delete;
    OptionStorage& operator=(OptionStorage&&) = delete;

    /**
     * @brief Get a pointer to the singleton OptionStorage.
     */
    static OptionStorage* GetOptionStorage();

    /**
     * @brief Add option to to the storage.
     *
     * @tparam T Meets the concept of Storable
     * @param option pointer to the option
     */
    template <Storable T>
    void AddOption(Option<T>* option) {
        if (options_.contains(option->GetCommand())) {
            throw std::logic_error(fmt::format("Command {} already exists.", option->GetCommand()));
        } else {
            options_[option->GetCommand()] = option;
        }
    }

    [[nodiscard]] std::size_t Size() const {
        return options_.size();
    }
    [[nodiscard]] bool Contains(std::string_view key) const {
        return options_.contains(key);
    }
    [[nodiscard]] const OptionType& At(std::string_view key) const {
        auto itr = options_.find(key);
        if (itr == options_.end()) {
            throw std::runtime_error(fmt::format("Unknown option key: {}", key));
        }
        return itr->second;
    }

    Iterator begin() {
        return options_.begin();
    }  // NOLINT
    Iterator end() {
        return options_.end();
    }  // NOLINT
    [[nodiscard]] ConstIterator begin() const {
        return options_.begin();
    }  // NOLINT
    [[nodiscard]] ConstIterator end() const {
        return options_.end();
    }  // NOLINT
    [[nodiscard]] ConstIterator cbegin() const {
        return options_.cbegin();
    }  // NOLINT
    [[nodiscard]] ConstIterator cend() const {
        return options_.cend();
    }  // NOLINT

private:
    Container options_;
};
/**
 * @brief Scalar command line option.
 * @details As a simple example, usage would be as follows:
 * @code{.cpp}
 * static Opt<std::uint64_t> NumThreads("num_threads", 1, "run `num_threads` jobs in parallel",
 *     Option::NotHidden);
 * @endcode
 *
 * The detailed usage instructions can be found in qret/target/sc_ls_fixed_v0/mapping.cpp.
 */
template <Storable T>
class Opt {
public:
    Opt(const Opt&) = delete;
    Opt& operator=(const Opt&) = delete;
    Opt(Opt&&) = delete;
    Opt& operator=(Opt&&) = delete;

    Opt(std::string_view command,
        const T& default_value,
        std::string_view description,
        OptionHidden hidden)
        : option_(command, default_value, description, hidden) {
        auto* storage = OptionStorage::GetOptionStorage();
        storage->AddOption(&option_);
    }

    const T& Get() const {
        return option_.GetValue();
    }
    operator const T&() const {  // NOLINT
        return option_.GetValue();
    }

    const Option<T>& GetOption() const {
        return option_;
    }

private:
    Option<T> option_;
};
/**
 * @brief Command line option of std::string.
 * @details For two reasons, Option<std::string> is explicitly specialized.
 *
 * 1. Implicit type conversion to const std::string& often does not work well.
 * 2. boost::program_options cannot accept pointer types such as const char*.
 */
template <>
class Opt<std::string> {
public:
    Opt(const Opt&) = delete;
    Opt& operator=(const Opt&) = delete;
    Opt(Opt&&) = delete;
    Opt& operator=(Opt&&) = delete;

    Opt(std::string_view command,
        const std::string& default_value,
        std::string_view description,
        OptionHidden hidden)
        : option_(command, default_value, description, hidden) {
        auto* storage = OptionStorage::GetOptionStorage();
        storage->AddOption(&option_);
    }

    const std::string& Get() const {
        return option_.GetValue();
    }
    // operator const std::string& () const { return option_.GetValue(); }  // NOLINT

    const Option<std::string>& GetOption() const {
        return option_;
    }

private:
    Option<std::string> option_;
};
}  // namespace qret

#endif  // QRET_BASE_OPTION_H
