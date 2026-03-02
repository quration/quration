/**
 * @file qret/target/target_enum.h
 * @brief Target enum.
 */

#ifndef QRET_TARGET_TARGET_ENUM_H
#define QRET_TARGET_TARGET_ENUM_H

#include <cstdint>
#include <stdexcept>
#include <string_view>

#include "qret/base/string.h"

namespace qret {
class TargetEnum {
public:
    enum class Table : std::uint8_t {
        Unknown,  //!< invalid target
        SC_LS_FIXED_V0,
        OpenQASM2,  //!< Unimplemented
        OpenQASM3,  //!< Unimplemented
    };

    explicit TargetEnum(const Table& x)
        : x_{x} {}

    static TargetEnum SCLSFixedV0() {
        return TargetEnum{Table::SC_LS_FIXED_V0};
    }
    static TargetEnum OpenQasm2() {
        return TargetEnum{Table::OpenQASM2};
    }
    static TargetEnum OpenQasm3() {
        return TargetEnum{Table::OpenQASM3};
    }
    static TargetEnum FromString(std::string_view target) noexcept {
        const auto lt = GetLowerString(target);
        if (lt == "sc_ls_fixed_v0") {
            return TargetEnum{Table::SC_LS_FIXED_V0};
        } else if (lt == "openqasm2") {
            return TargetEnum{Table::OpenQASM2};
        } else if (lt == "openqasm3") {
            return TargetEnum{Table::OpenQASM3};
        }
        return TargetEnum{Table::Unknown};
    }

    [[nodiscard]] std::string_view ToString() const {
        switch (x_) {
            case Table::SC_LS_FIXED_V0:
                return "sc_ls_fixed_v0";
            case Table::OpenQASM2:
                return "openqasm2";
            case Table::OpenQASM3:
                return "openqasm3";
            case Table::Unknown:
                return "unknown";
            default:
                throw std::logic_error("unreachable");
        }
    }

    [[nodiscard]] Table GetEnum() const {
        return x_;
    }
    [[nodiscard]] bool IsValid() const {
        return x_ != Table::Unknown;
    }
    [[nodiscard]] bool IsSCLSFixedV0() const {
        return x_ == Table::SC_LS_FIXED_V0;
    }
    [[nodiscard]] bool IsOpenQASM2() const {
        return x_ == Table::OpenQASM2;
    }
    [[nodiscard]] bool IsOpenQASM3() const {
        return x_ == Table::OpenQASM3;
    }

private:
    Table x_;
};

inline bool operator==(const TargetEnum& lhs, const TargetEnum& rhs) {
    return lhs.GetEnum() == rhs.GetEnum();
}
inline bool operator==(const TargetEnum& lhs, const TargetEnum::Table& rhs) {
    return lhs.GetEnum() == rhs;
}
inline bool operator==(const TargetEnum::Table& lhs, const TargetEnum& rhs) {
    return lhs == rhs.GetEnum();
}
inline bool operator!=(const TargetEnum& lhs, const TargetEnum& rhs) {
    return !(lhs == rhs);
}
inline bool operator!=(const TargetEnum& lhs, const TargetEnum::Table& rhs) {
    return !(lhs == rhs);
}
inline bool operator!=(const TargetEnum::Table& lhs, const TargetEnum& rhs) {
    return !(lhs == rhs);
}
}  // namespace qret

#endif  // QRET_TARGET_TARGET_ENUM_H
