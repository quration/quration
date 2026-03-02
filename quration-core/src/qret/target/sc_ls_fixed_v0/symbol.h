/**
 * @file qret/target/sc_ls_fixed_v0/symbol.h
 * @brief Symbol.
 * @details This file defines symbol classes used in SC_LS_FIXED_V0 instructions.
 */

#ifndef QRET_TARGET_SC_LS_FIXED_V0_SYMBOL_H
#define QRET_TARGET_SC_LS_FIXED_V0_SYMBOL_H

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <cstdint>
#include <limits>
#include <memory>
#include <ostream>
#include <string>

#include "qret/base/json.h"
#include "qret/qret_export.h"

namespace qret::sc_ls_fixed_v0 {
/**
 * @brief Type of symbol.
 */
enum class QRET_EXPORT SymbolType : std::uint8_t {
    Quantum,
    Classical,
    MagicFactory,
    EntanglementFactory,
    EntanglementHandle,
};

/**
 * @brief Symbol class.
 */
template <SymbolType T>
class QRET_EXPORT Symbol {
public:
    using IdType = std::uint64_t;
    explicit constexpr Symbol(IdType id = std::numeric_limits<IdType>::max())
        : id_{id} {}

    [[nodiscard]] constexpr SymbolType Type() const {
        return T;
    }
    [[nodiscard]] constexpr IdType Id() const {
        return id_;
    }
    [[nodiscard]] std::string ToString() const {
        return GetPrefix() + std::to_string(id_);
    }

    constexpr auto operator<=>(const Symbol&) const = default;

private:
    static constexpr char GetPrefix() {
        if constexpr (T == SymbolType::Quantum) {
            return 'q';
        } else if constexpr (T == SymbolType::Classical) {
            return 'c';
        } else if constexpr (T == SymbolType::MagicFactory) {
            return 'm';
        } else if constexpr (T == SymbolType::EntanglementFactory) {
            return 'e';
        } else if constexpr (T == SymbolType::EntanglementHandle) {
            return 'h';
        }
        return ' ';
    }
    IdType id_;
};

template <SymbolType T>
inline std::ostream& operator<<(std::ostream& out, const Symbol<T>& s) {
    return out << s.ToString();
}
template <SymbolType T>
inline void to_json(Json& j, const Symbol<T>& s) {
    j = s.Id();
}
template <SymbolType T>
inline void from_json(const Json& j, Symbol<T>& s) {
    auto tmp = typename Symbol<T>::IdType{0};
    j.get_to(tmp);
    s = Symbol<T>(tmp);
}

/**
 * @brief Symbol class representing quantum object.
 */
using QSymbol = Symbol<SymbolType::Quantum>;
/**
 * @brief Symbol class representing classical object.
 */
using CSymbol = Symbol<SymbolType::Classical>;
/**
 * @brief Symbol class representing magic factory object.
 */
using MSymbol = Symbol<SymbolType::MagicFactory>;
/**
 * @brief Symbol class representing entanglement factory object.
 */
using ESymbol = Symbol<SymbolType::EntanglementFactory>;
/**
 * @brief Symbol class representing entanglement handle object.
 */
using EHandle = Symbol<SymbolType::EntanglementHandle>;

/**
 * @brief Generate new symbols.
 */
class SymbolGenerator {
public:
    static std::shared_ptr<SymbolGenerator> New() {
        return std::shared_ptr<SymbolGenerator>(new SymbolGenerator{});
    }

    void SetQ(QSymbol offset) {
        q_ = offset.Id();
    }
    void SetC(CSymbol offset) {
        c_ = offset.Id();
    }
    void SetM(MSymbol offset) {
        m_ = offset.Id();
    }
    void SetE(ESymbol offset) {
        e_ = offset.Id();
    }
    void SetEH(EHandle offset) {
        eh_ = offset.Id();
    }

    QSymbol LatestQ() const {
        return QSymbol(q_);
    }
    CSymbol LatestC() const {
        return CSymbol(c_);
    }
    MSymbol LatestM() const {
        return MSymbol(m_);
    }
    ESymbol LatestE() const {
        return ESymbol(e_);
    }
    EHandle LatestEH() const {
        return EHandle(eh_);
    }

    QSymbol GenerateQ() {
        return QSymbol(++q_);
    }
    CSymbol GenerateC() {
        return CSymbol(++c_);
    }
    MSymbol GenerateM() {
        return MSymbol(++m_);
    }
    ESymbol GenerateE() {
        return ESymbol(++e_);
    }
    EHandle GenerateEH() {
        return EHandle(++eh_);
    }

private:
    SymbolGenerator() = default;

    QSymbol::IdType q_;
    CSymbol::IdType c_;
    MSymbol::IdType m_;
    ESymbol::IdType e_;
    EHandle::IdType eh_;
};
}  // namespace qret::sc_ls_fixed_v0

template <::qret::sc_ls_fixed_v0::SymbolType T>
struct std::hash<::qret::sc_ls_fixed_v0::Symbol<T>> {  // NOLINT
    std::size_t operator()(const ::qret::sc_ls_fixed_v0::Symbol<T>& s) const {
        return std::hash<std::uint64_t>()(s.Id());
    }
};
template <::qret::sc_ls_fixed_v0::SymbolType T>
struct fmt::formatter<::qret::sc_ls_fixed_v0::Symbol<T>> : ostream_formatter {};

#endif  //  QRET_TARGET_SC_LS_FIXED_V0_SYMBOL_H
