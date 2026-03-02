/**
 * @file qret/frontend/argument.h
 * @brief Arguments of quantum circuit.
 * @details This file implements arguments of a quantum circuit.
 */

#ifndef QRET_FRONTEND_ARGUMENT_H
#define QRET_FRONTEND_ARGUMENT_H

#include <fmt/core.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "qret/qret_export.h"

namespace qret::frontend {
namespace tag {
struct Quantum {};
struct Qubit {};
struct Register {};
template <typename T>
using IsQTarget =
        std::conditional_t<std::is_base_of_v<Quantum, T>, std::true_type, std::false_type>;
template <typename T>
static constexpr bool IsQTargetV = IsQTarget<T>::value;
template <typename T>
using IsQubit =
        std::conditional_t<std::is_same_v<Qubit, typename T::Tag>, std::true_type, std::false_type>;
template <typename T>
static constexpr bool IsQubitV = IsQubit<T>::value;
template <typename T>
using IsRegister = std::
        conditional_t<std::is_same_v<Register, typename T::Tag>, std::true_type, std::false_type>;
template <typename T>
static constexpr bool IsRegisterV = IsRegister<T>::value;
}  // namespace tag
enum class SizeType : std::uint8_t { Single, Array /*, Dynamic */ };
enum class ObjectType : std::uint8_t { Qubit, Register };
template <ObjectType, SizeType size>
class QuantumBase : public tag::Quantum {};
using Qubit = QuantumBase<ObjectType::Qubit, SizeType::Single>;
using Qubits = QuantumBase<ObjectType::Qubit, SizeType::Array>;
using CleanAncilla = Qubit;  // FIXME: implement clean ancilla class
using CleanAncillae = Qubits;  // FIXME: implement clean ancillae class
using DirtyAncilla = Qubit;  // FIXME: implement dirty ancilla class
using DirtyAncillae = Qubits;  // FIXME: implement dirty ancillae class
using Register = QuantumBase<ObjectType::Register, SizeType::Single>;
using Registers = QuantumBase<ObjectType::Register, SizeType::Array>;
using Input = Register;  // FIXME: implement input register class
using Inputs = Registers;  // FIXME: implement input registers class
using Output = Register;  // FIXME: implement output register class
using Outputs = Registers;  // FIXME: implement output registers class

// Forward declarations
class CircuitBuilder;
class CircuitGenerator;

/**
 * @brief A template class that creates classes such as Qubit, Qubits, Register, and Registers
 * depending on the template parameters.
 * @details The argument of qret::frontend::Circuit must be instances of this template class.
 */
template <ObjectType type>
class QuantumBase<type, SizeType::Single> : public tag::Quantum {
public:
    using Tag = std::conditional_t<type == ObjectType::Qubit, tag::Qubit, tag::Register>;

    QuantumBase() = delete;
    virtual ~QuantumBase() = default;
    QuantumBase(const QuantumBase&) = default;
    QuantumBase(QuantumBase&&) noexcept = default;
    QuantumBase& operator=(const QuantumBase&) = delete;
    QuantumBase& operator=(QuantumBase&&) = delete;

    CircuitBuilder* GetBuilder() const {
        return builder_;
    }

    constexpr bool IsQubit() const {
        return type == ObjectType::Qubit;
    }
    constexpr bool IsRegister() const {
        return type == ObjectType::Register;
    }
    std::uint64_t Size() const {
        return 1;
    }
    std::uint64_t GetId() const {
        return id_;
    }

    // [Do not use] very dangerous method:
    void SetId(std::uint64_t id) {
        id_ = id;
    }

private:
    friend class CircuitBuilder;
    friend class CircuitGenerator;
    friend class QuantumBase<type, SizeType::Array>;
    friend class QuantumIterator;
    friend QRET_EXPORT std::ostream& operator<<(std::ostream& out, const Qubit& q);
    friend QRET_EXPORT std::ostream& operator<<(std::ostream& out, const Register& r);

    QuantumBase(CircuitBuilder* builder, std::uint64_t id)
        : builder_{builder}
        , id_{id} {}

    CircuitBuilder* const builder_;
    std::uint64_t id_;
};
QRET_EXPORT std::ostream& operator<<(std::ostream& out, const Qubit& q);
QRET_EXPORT std::ostream& operator<<(std::ostream& out, const Register& r);
template <ObjectType type>
class QuantumBase<type, SizeType::Array> : public tag::Quantum {
public:
    using Tag = std::conditional_t<type == ObjectType::Qubit, tag::Qubit, tag::Register>;

    class QuantumIterator {
    public:
        using difference_type = std::int64_t;
        using value_type = QuantumBase<type, SizeType::Single>;
        using iterator_concept = std::input_iterator_tag;

        QuantumIterator() = delete;
        QuantumIterator(const QuantumIterator&) = default;
        QuantumIterator(QuantumIterator&&) noexcept = default;
        QuantumIterator& operator=(const QuantumIterator&) = delete;
        QuantumIterator& operator=(QuantumIterator&&) noexcept = delete;

        QuantumIterator& operator++() {  // Prefix increment
            idx_++;
            if (idx_ == ptr_->sizes_[segment_idx_]) {
                segment_idx_++;
                idx_ = 0;
            }
            return *this;
        }
        QuantumIterator operator++(int) {  // Postfix increment
            const auto ret = *this;
            ++(*this);
            return ret;
        }
        QuantumIterator& operator--() {  // Prefix decrement
            if (idx_ == 0) {
                segment_idx_--;
                idx_ = ptr_->sizes_[segment_idx_] - 1;
            } else {
                idx_--;
            }
            return *this;
        }
        QuantumIterator operator--(int) {  // Postfix decrement
            const auto ret = *this;
            --(*this);
            return ret;
        }
        value_type operator*() const noexcept {
            return {ptr_->builder_, ptr_->offsets_[segment_idx_] + idx_};
        }

        auto operator<=>(const QuantumIterator&) const = default;

    private:
        using Array = QuantumBase<type, SizeType::Array>;
        friend class QuantumBase<type, SizeType::Array>;
        QuantumIterator(const Array* ptr, std::uint64_t segment_idx, std::uint64_t idx)
            : ptr_{ptr}
            , segment_idx_{segment_idx}
            , idx_{idx} {}
        const Array* const ptr_;
        std::uint64_t segment_idx_;
        std::uint64_t idx_;
    };

    QuantumBase() = delete;
    virtual ~QuantumBase() = default;
    QuantumBase(QuantumBase<type, SizeType::Single> s)  // NOLINT
        : builder_{s.builder_}
        , offsets_{s.id_}
        , sizes_{1} {}
    QuantumBase(const QuantumBase&) = default;
    QuantumBase(QuantumBase&&) noexcept = default;
    QuantumBase& operator=(const QuantumBase&) = delete;
    QuantumBase& operator=(QuantumBase&&) = delete;

    CircuitBuilder* GetBuilder() const {
        return builder_;
    }

    constexpr bool IsQubit() const {
        return type == ObjectType::Qubit;
    }
    constexpr bool IsRegister() const {
        return type == ObjectType::Register;
    }
    std::uint64_t Size() const {
        return std::accumulate(sizes_.begin(), sizes_.end(), std::uint64_t{0});
    }
    QuantumBase<type, SizeType::Single> operator[](std::uint64_t idx) const {
        const auto tmp = idx;
        for (auto i = std::size_t{0}; i < sizes_.size(); ++i) {
            if (idx < sizes_[i]) {
                return {builder_, offsets_[i] + idx};
            }
            idx -= sizes_[i];
        }
        throw std::out_of_range(
                fmt::format("out of range error: accessed {} but container size is {}", tmp, Size())
        );
        return {nullptr, 0};
    }
    QuantumBase<type, SizeType::Array> Range(std::uint64_t start, std::uint64_t size) const {
        if (size == 0) {
            return {builder_, {}, {}};
        }

        // [start..start+size)
        auto sizes = std::vector<std::uint64_t>();
        auto offsets = std::vector<std::uint64_t>();

        // Move to `start`
        auto i = std::uint64_t{0};
        for (; i < offsets_.size(); ++i) {
            if (start < sizes_[i]) {
                const auto offset = offsets_[i] + start;
                const auto tmp_size = std::min(sizes_[i] - start, size);
                offsets.emplace_back(offset);
                sizes.emplace_back(tmp_size);
                start = 0;
                size -= tmp_size;
                break;
            } else {
                start -= sizes_[i];
            }
        }
        ++i;
        // Push till `size` is 0
        for (; i < offsets_.size(); ++i) {
            if (size > 0) {
                const auto tmp_size = std::min(sizes_[i], size);
                offsets.emplace_back(offsets_[i]);
                sizes.emplace_back(tmp_size);
                size -= tmp_size;
            } else {
                break;
            }
        }

        if (size != 0) {
            throw std::runtime_error("`start` + `size` is out of range");
        }

        return {builder_, offsets, sizes};
    }

    template <SizeType RSize>
    QuantumBase& operator+=(const QuantumBase<type, RSize>& rhs) {
        if constexpr (RSize == SizeType::Single) {
            offsets_.emplace_back(rhs.id_);
            sizes_.emplace_back(1);
        } else {
            const auto sz = rhs.offsets_.size();
            for (auto i = std::size_t{0}; i < sz; ++i) {
                offsets_.emplace_back(rhs.offsets_[i]);
                sizes_.emplace_back(rhs.sizes_[i]);
            }
        }
        return *this;
    }

    QuantumIterator begin() const {
        return QuantumIterator{this, 0, 0};
    }  // NOLINT
    QuantumIterator end() const {
        return QuantumIterator{this, offsets_.size(), 0};
    }  // NOLINT

private:
    friend class CircuitBuilder;
    friend class CircuitGenerator;
    friend class QuantumIterator;
    friend QRET_EXPORT std::ostream& operator<<(std::ostream& out, const Qubits& qs);
    friend QRET_EXPORT std::ostream& operator<<(std::ostream& out, const Registers& rs);

    QuantumBase(
            CircuitBuilder* builder,
            const std::vector<std::uint64_t>& offsets,
            const std::vector<std::uint64_t>& sizes
    )
        : builder_{builder}
        , offsets_{offsets}
        , sizes_{sizes} {
        auto i = std::size_t{0};
        assert(offsets_.size() == sizes_.size());
        while (i < sizes_.size()) {
            if (sizes_[i] == 0) {
                offsets_.erase(offsets_.begin() + static_cast<std::int64_t>(i));
                sizes_.erase(sizes_.begin() + static_cast<std::int64_t>(i));
            } else {
                i++;
            }
        }
    }

    CircuitBuilder* const builder_;
    // num segments == offsets_.size() == sizes_.size()
    std::vector<std::uint64_t> offsets_;
    std::vector<std::uint64_t> sizes_;
};
QRET_EXPORT std::ostream& operator<<(std::ostream& out, const Qubits& qs);
QRET_EXPORT std::ostream& operator<<(std::ostream& out, const Registers& rs);
template <ObjectType type, SizeType LSize, SizeType RSize>
QuantumBase<type, SizeType::Array>
operator+(const QuantumBase<type, LSize>& lhs, const QuantumBase<type, RSize>& rhs) {
    auto ret = QuantumBase<type, SizeType::Array>(lhs);
    ret += rhs;
    return ret;
}
}  // namespace qret::frontend

#endif  // QRET_FRONTEND_ARGUMENT_H
