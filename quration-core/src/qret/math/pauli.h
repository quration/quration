/**
 * @file qret/math/pauli.h
 * @brief Pauli calculation and circuit transformation.
 * @details This file defines data structures for manipulating Pauli operators and
 * circuits composed of Pauli rotations and Pauli-basis measurements.
 *
 * Main components:
 * - PauliString: Represents tensor products of Pauli operators (I, X, Y, Z)
 *   and a global phase.
 *
 * - PauliRotation: Encodes \f$\exp(-i \theta P)\f$ gates acting on Pauli
 *     strings P.
 * - PauliMeasurement: Represents measurement in a general Pauli basis.
 *
 * - PauliCircuit: A container for sequences of Pauli operations,
 *   with built-in transformations like normalization, commuting, and merging.
 */

#ifndef QRET_MATH_PAULI_H
#define QRET_MATH_PAULI_H

#include <boost/dynamic_bitset.hpp>
#include <boost/dynamic_bitset/dynamic_bitset.hpp>

#include <cstddef>
#include <cstdint>
#include <list>
#include <string>
#include <variant>

#include "qret/qret_export.h"

namespace qret::math {
/**
 * @brief Enum class representing Pauli operators.
 *
 */
enum class QRET_EXPORT Pauli : std::uint8_t { I, X, Y, Z };
/**
 * @brief Converts a Pauli operator to a string.
 *
 * @param p Pauli operator enum value.
 * @return std::string String of the Pauli operator.
 */
std::string QRET_EXPORT ToString(Pauli p);
/**
 * @brief Converts a string to a Pauli operator.
 *
 * @param str String representation of the Pauli operator.
 * @return Pauli Pauli operator enum value.
 */
Pauli QRET_EXPORT PauliFromString(const std::string& str);

/**
 * @brief Class representing a Pauli string.
 *
 * This class stores and manipulates tensor products of single-qubit Pauli operators.
 * The sign is stored as exp(i * sign_ * pi / 2):
 *   - 0: 1
 *   - 1: i
 *   - 2: -1
 *   - 3: -i
 *
 * Each qubit is represented as (xs_[i], zs_[i]):
 *   - (0, 0): I
 *   - (1, 0): X
 *   - (0, 1): Z
 *   - (1, 1): Y
 */
class QRET_EXPORT PauliString {
public:
    using BitSet = boost::dynamic_bitset<>;

    explicit PauliString(std::size_t num_qubits, std::uint32_t sign = 0);
    explicit PauliString(std::string_view pauli_string, std::uint32_t sign = 0);

    static PauliString I(std::size_t num_qubits, std::uint32_t sign = 0);
    static PauliString X(std::size_t num_qubits, std::size_t index, std::uint32_t sign = 0);
    static PauliString Z(std::size_t num_qubits, std::size_t index, std::uint32_t sign = 0);
    static PauliString Y(std::size_t num_qubits, std::size_t index, std::uint32_t sign = 0);

    /**
     * @brief Get the number of qubits.
     */
    std::size_t GetNumQubits() const {
        return xs_.size();
    }

    /**
     * @brief Get const reference to sign.
     */
    std::uint32_t GetSign() const {
        return sign_;
    }

    void SetSign(std::uint32_t sign) {
        sign_ = sign % 4;
    }
    void AddSign(std::uint32_t offset) {
        sign_ = (sign_ + offset) % 4;
    }

    /**
     * @brief Convert Pauli string to a human-readable string.
     *
     * @param show_sign If true, show the sign of pauli string.
     * @return String representation, e.g., "-i XIZY".
     */
    std::string ToString(bool show_sign = true) const;

    /**
     * @brief Multiply this Pauli string by another.
     *
     * @param other Pauli string to multiply with.
     * @return Reference to this object after multiplication.
     *
     * @note The number of qubits must match.
     */
    PauliString& operator*=(const PauliString& other);

    /**
     * @brief Multiply another Pauli string from the left (i.e., other * this).
     *
     * @param other The Pauli string to multiply from the left.
     * @return Reference to this object after multiplication.
     */
    PauliString& MulFromLeft(const PauliString& other);

    /**
     * @brief Compute the inverse of this Pauli string.
     */
    void Inverse() {
        sign_ = (4 - sign_) % 4;
    }

    /**
     * @brief Check if this Pauli string commutes with another.
     *
     * @param other Pauli string to check commutation with.
     * @return true if they commute, false otherwise.
     */
    bool IsCommuteWith(const PauliString& other) const;

    [[nodiscard]] bool IsI() const {
        return xs_.count() == 0 && zs_.count() == 0;
    }

    const BitSet& XS() const {
        return xs_;
    }
    const BitSet& ZS() const {
        return zs_;
    }

    [[nodiscard]] char operator[](const std::size_t index) const {
        if (xs_[index] && zs_[index]) {
            return 'Y';
        }
        if (xs_[index]) {
            return 'X';
        }
        if (zs_[index]) {
            return 'Z';
        }
        return 'I';
    }
    void Set(const std::size_t index, char p) {
        switch (p) {
            case 'x':
            case 'X':
                xs_[index] = true;
                zs_[index] = false;
                break;
            case 'z':
            case 'Z':
                xs_[index] = false;
                zs_[index] = true;
                break;
            case 'y':
            case 'Y':
                xs_[index] = true;
                zs_[index] = true;
                break;
            default:
                xs_[index] = false;
                zs_[index] = false;
                break;
        }
    }

private:
    using SignType = std::uint_fast8_t;

    SignType sign_ = 0;  ///< Global phase.
    BitSet xs_{0};  ///< X components.
    BitSet zs_{0};  ///< Z components.
};

/**
 * @brief Binary multiplication of Pauli strings.
 *
 * @param lhs Left-hand Pauli string.
 * @param rhs Right-hand Pauli string.
 * @return Resulting Pauli string.
 */
QRET_EXPORT PauliString operator*(const PauliString& lhs, const PauliString& rhs);

QRET_EXPORT inline bool operator==(const PauliString& lhs, const PauliString& rhs) {
    return lhs.GetSign() == rhs.GetSign() && lhs.XS() == rhs.XS() && lhs.ZS() == rhs.ZS();
}

QRET_EXPORT inline bool operator!=(const PauliString& lhs, const PauliString& rhs) {
    return !(lhs == rhs);
}

QRET_EXPORT inline bool operator<(const PauliString& lhs, const PauliString& rhs) {
    if (lhs.ZS() < rhs.ZS()) {
        return true;
    } else if (rhs.ZS() < lhs.ZS()) {
        return false;
    }
    // lhs.ZS() == rhs.ZS()
    if (lhs.XS() < rhs.XS()) {
        return true;
    } else if (rhs.XS() < lhs.XS()) {
        return false;
    }
    return lhs.GetSign() < rhs.GetSign();
}
QRET_EXPORT inline bool operator<=(const PauliString& lhs, const PauliString& rhs) {
    if (lhs == rhs) {
        return true;
    }
    return lhs < rhs;
}
QRET_EXPORT inline bool operator>(const PauliString& lhs, const PauliString& rhs) {
    return !(lhs <= rhs);
}
QRET_EXPORT inline bool operator>=(const PauliString& lhs, const PauliString& rhs) {
    return !(lhs < rhs);
}

/**
 * @brief A class representing a Pauli rotation of the form \f$exp(-i * \theta * P)\f$,
 *        where P is a tensor product of Pauli operators (a Pauli string).
 *
 * Valid only when PauliString's global phase is real (\f$\pm\f$ 1).
 */
class QRET_EXPORT PauliRotation {
public:
    enum class Theta : std::uint8_t {
        pi_over_2,
        pi_over_4,
        pi_over_8,
    };

    PauliRotation(Theta theta, const PauliString& pauli)
        : theta_{theta}
        , pauli_{pauli} {}
    PauliRotation(Theta theta, PauliString&& pauli)
        : theta_{theta}
        , pauli_{std::move(pauli)} {}
    PauliRotation(Theta theta, std::string_view pauli, std::uint32_t sign = 0)
        : theta_{theta}
        , pauli_{pauli, sign} {}

    /**
     * @brief Check if the Pauli rotation is physically valid.
     *
     * Only real global phases (0 or 2 corresponding to \f$\pm 1\f$) are allowed.
     */
    [[nodiscard]] bool IsValid() const {
        return pauli_.GetSign() % 2 == 0;
    }

    [[nodiscard]] Theta GetTheta() const {
        return theta_;
    }
    [[nodiscard]] bool IsClifford() const {
        return theta_ != Theta::pi_over_8;
    }
    [[nodiscard]] const PauliString& GetPauli() const {
        return pauli_;
    }
    [[nodiscard]] PauliString& GetMutPauli() {
        return pauli_;
    }

    [[nodiscard]] std::string ToString() const {
        auto ret = pauli_.ToString();
        switch (theta_) {
            case Theta::pi_over_2:
                return "(pi/2, " + ret + ')';
            case Theta::pi_over_4:
                return "(pi/4, " + ret + ')';
            case Theta::pi_over_8:
                return "(pi/8, " + ret + ')';
            default:
                break;
        }
        return ret;
    }

private:
    Theta theta_;
    PauliString pauli_;  ///!< sign must be 0 or 2 (not imaginary).
};

QRET_EXPORT inline bool operator==(const PauliRotation& lhs, const PauliRotation& rhs) {
    return lhs.GetTheta() == rhs.GetTheta() && lhs.GetPauli() == rhs.GetPauli();
}

QRET_EXPORT inline bool operator!=(const PauliRotation& lhs, const PauliRotation& rhs) {
    return !(lhs == rhs);
}

QRET_EXPORT bool ExchangeRotation(PauliRotation& lhs, PauliRotation& rhs);

/**
 * @brief Represents a measurement in the Pauli basis (e.g., X, Y, Z, or tensor products).
 *
 * Assumes the measurement operator has a real global phase (\f$\pm 1\f$).
 */
class QRET_EXPORT PauliMeasurement {
public:
    explicit PauliMeasurement(const PauliString& pauli)
        : pauli_{pauli} {}
    explicit PauliMeasurement(PauliString&& pauli)
        : pauli_{std::move(pauli)} {}
    explicit PauliMeasurement(std::string_view pauli, std::uint32_t sign = 0)
        : pauli_{pauli, sign} {}

    [[nodiscard]] bool IsValid() const {
        return pauli_.GetSign() % 2 == 0;
    }

    [[nodiscard]] const PauliString& GetPauli() const {
        return pauli_;
    }
    [[nodiscard]] PauliString& GetMutPauli() {
        return pauli_;
    }

    [[nodiscard]] std::string ToString() const {
        return pauli_.ToString();
    }

private:
    PauliString pauli_;  ///!< sign must be 0 or 2 (not imaginary).
};

QRET_EXPORT inline bool operator==(const PauliMeasurement& lhs, const PauliMeasurement& rhs) {
    return lhs.GetPauli() == rhs.GetPauli();
}

QRET_EXPORT inline bool operator!=(const PauliMeasurement& lhs, const PauliMeasurement& rhs) {
    return !(lhs == rhs);
}

QRET_EXPORT bool MergeClifford(PauliRotation& clifford, PauliMeasurement& measurement);

/**
 * @brief A circuit consisting of Pauli rotations and Pauli measurements.
 *
 * Provides transformation passes such as:
 *   - Moving non-Clifford gates forward
 *   - Merging Clifford gates into measurement
 *   - Normalizing to a standard form
 */
class QRET_EXPORT PauliCircuit {
public:
    /**
     * @brief A single item in a PauliCircuit, which can be either a rotation or a measurement.
     */
    struct Item {
        Item(const PauliRotation& x)  // NOLINT
            : value{x} {}
        Item(PauliRotation&& x)  // NOLINT
            : value{std::move(x)} {}
        Item(const PauliMeasurement& x)  // NOLINT
            : value{x} {}
        Item(PauliMeasurement&& x)  // NOLINT
            : value{std::move(x)} {}

        Item(const Item&) = default;
        Item(Item&&) noexcept = default;
        Item& operator=(const Item&) = default;
        Item& operator=(Item&&) noexcept = default;
        ~Item() = default;

        [[nodiscard]] bool IsValid() const {
            return std::visit([](const auto& x) { return x.IsValid(); }, value);
        }
        [[nodiscard]] std::string ToString() const {
            return std::visit([](const auto& x) { return x.ToString(); }, value);
        }

        [[nodiscard]] const PauliRotation& GetRotation() const {
            return *std::get_if<PauliRotation>(&value);
        }
        [[nodiscard]] const PauliMeasurement& GetMeasurement() const {
            return *std::get_if<PauliMeasurement>(&value);
        }

        [[nodiscard]] bool IsRotation() const {
            const auto* pauli_rotation = std::get_if<PauliRotation>(&value);
            return pauli_rotation != nullptr;
        }
        [[nodiscard]] bool IsClifford() const {
            const auto* pauli_rotation = std::get_if<PauliRotation>(&value);
            return pauli_rotation != nullptr && pauli_rotation->IsClifford();
        }
        [[nodiscard]] bool IsNonClifford() const {
            const auto* pauli_rotation = std::get_if<PauliRotation>(&value);
            return pauli_rotation != nullptr && !pauli_rotation->IsClifford();
        }
        [[nodiscard]] bool IsMeasurement() const {
            const auto* pauli_measurement = std::get_if<PauliMeasurement>(&value);
            return pauli_measurement != nullptr;
        }

        std::variant<PauliRotation, PauliMeasurement> value;
    };
    using List = std::list<Item>;

    PauliCircuit() = default;
    explicit PauliCircuit(const List& ls)
        : ls_{ls} {}
    explicit PauliCircuit(List&& ls)
        : ls_{std::move(ls)} {}

    void EmplaceBack(const Item& item) {
        ls_.emplace_back(item);
    }
    void EmplaceBack(Item&& item) {
        ls_.emplace_back(std::move(item));
    }

    /**
     * @brief Move all non-Clifford Pauli rotations toward the front of the circuit,
     *        commuting them through Clifford gates.
     */
    void MoveNonCliffordForward();
    /**
     * @brief Merge Clifford Pauli rotations into adjacent measurements.
     */
    void MergeCliffordToMeasurement();
    /**
     * @brief Normalize the circuit into a "standard form" by applying
     *        a sequence of transformations:
     *          1. MoveNonCliffordForward
     *          2. MergeCliffordToMeasurement
     *          3. Pop operations after measurements
     */
    void ToStandardFormat();

    [[nodiscard]] bool IsValid() const;
    [[nodiscard]] bool IsStandardFormat() const;

    [[nodiscard]] auto begin() {
        return ls_.begin();
    }
    [[nodiscard]] auto begin() const {
        return ls_.begin();
    }
    [[nodiscard]] auto end() {
        return ls_.end();
    }
    [[nodiscard]] auto end() const {
        return ls_.end();
    }

private:
    using Iterator = typename List::iterator;

    bool ExchangeRotationImpl(Iterator lhs, Iterator rhs);
    bool MergeCliffordImpl(Iterator clifford, Iterator measurement);
    void MoveNonCliffordForwardImpl(Iterator non_clifford);
    void MergeCliffordToMeasurementImpl(Iterator clifford);
    void PopOperationAfterMeasurement();

    List ls_;
};
}  // namespace qret::math

#endif  // QRET_BASE_PAULI_H
