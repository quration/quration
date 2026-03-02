/**
 * @file qret/target/sc_ls_fixed_v0/pauli.h
 * @brief Pauli.
 */

#ifndef QRET_TARGET_SC_LS_FIXED_V0_PAULI_H
#define QRET_TARGET_SC_LS_FIXED_V0_PAULI_H

#include <ostream>
#include <stdexcept>

#include "qret/base/json.h"
#include "qret/qret_export.h"

namespace qret::sc_ls_fixed_v0 {
/**
 * @brief Pauli operator class without coefficients.
 */
class QRET_EXPORT Pauli {
public:
    static constexpr Pauli I() {
        return Pauli{0};
    };
    static constexpr Pauli X() {
        return Pauli{1};
    };
    static constexpr Pauli Y() {
        return Pauli{2};
    };
    static constexpr Pauli Z() {
        return Pauli{3};
    };
    static constexpr Pauli Any() {
        return Pauli{4};
    };
    static constexpr Pauli FromChar(char c) {
        switch (c) {
            case 'i':
            case 'I':
                return I();
            case 'x':
            case 'X':
                return X();
            case 'y':
            case 'Y':
                return Y();
            case 'z':
            case 'Z':
                return Z();
            case 'a':
            case 'A':
                return Any();
            default:
                break;
        }
        throw std::runtime_error("unknown character");
    }

    constexpr Pauli()
        : v_{0} {}

    [[nodiscard]] constexpr bool IsI() const {
        return v_ == 0;
    }
    [[nodiscard]] constexpr bool IsX() const {
        return v_ == 1;
    }
    [[nodiscard]] constexpr bool IsY() const {
        return v_ == 2;
    }
    [[nodiscard]] constexpr bool IsZ() const {
        return v_ == 3;
    }
    [[nodiscard]] constexpr bool IsAny() const {
        return v_ == 4;
    }

    [[nodiscard]] constexpr char ToChar() const {
        switch (v_) {
            case 0:
                return 'I';
            case 1:
                return 'X';
            case 2:
                return 'Y';
            case 3:
                return 'Z';
            case 4:
                return 'A';
            default:
                break;
        }
        return ' ';
    }

    constexpr auto operator<=>(const Pauli&) const = default;

    constexpr Pauli& operator*=(const Pauli rhs) {
        v_ ^= rhs.v_;
        return *this;
    }

private:
    explicit constexpr Pauli(std::uint8_t v)
        : v_{v} {}
    std::uint8_t v_;
};
constexpr Pauli operator*(Pauli lhs, const Pauli rhs) {
    lhs *= rhs;
    return lhs;
}
inline std::ostream& operator<<(std::ostream& out, const Pauli p) {
    return out << p.ToChar();
}
inline void to_json(Json& j, const Pauli p) {
    j = p.ToChar();
}
inline void from_json(const Json& j, Pauli& p) {
    char tmp;
    j.get_to(tmp);
    p = Pauli::FromChar(tmp);
}
}  // namespace qret::sc_ls_fixed_v0

#endif  // QRET_TARGET_SC_LS_FIXED_V0_PAULI_H
