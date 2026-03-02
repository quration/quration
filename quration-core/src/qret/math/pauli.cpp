/**
 * @file qret/math/pauli.cpp
 * @brief Pauli calculation and circuit transformation.
 */

#include "qret/math/pauli.h"

#include <cctype>
#include <iterator>
#include <memory>
#include <stdexcept>

#include "qret/base/list.h"

namespace qret::math {
std::string ToString(Pauli p) {
    switch (p) {
        case Pauli::I:
            return "I";
        case Pauli::X:
            return "X";
        case Pauli::Y:
            return "Y";
        case Pauli::Z:
            return "Z";
        default:
            throw std::logic_error("unreachable");
    }
}
Pauli PauliFromString(const std::string& str) {
    if (str == "I") {
        return Pauli::I;
    } else if (str == "X") {
        return Pauli::X;
    } else if (str == "Y") {
        return Pauli::Y;
    } else if (str == "Z") {
        return Pauli::Z;
    } else {
        throw std::runtime_error("Unknown Pauli: " + str);
    }
}

PauliString::PauliString(std::size_t num_qubits, std::uint32_t sign)
    : sign_(sign % 4) {
    xs_.resize(num_qubits);
    zs_.resize(num_qubits);
}

PauliString::PauliString(std::string_view pauli_string, std::uint32_t sign)
    : sign_(sign % 4) {
    xs_.resize(pauli_string.size());
    zs_.resize(pauli_string.size());
    for (auto i = std::size_t{0}; i < pauli_string.size(); ++i) {
        switch (std::tolower(pauli_string[i])) {
            case 'x':
            case 'X':
                xs_.set(i);
                break;
            case 'z':
            case 'Z':
                zs_.set(i);
                break;
            case 'y':
            case 'Y':
                xs_.set(i);
                zs_.set(i);
                break;
            case 'i':
            case 'I':
                break;
            default:
                throw std::runtime_error("Unknown char");
                break;
        }
    }
}

PauliString PauliString::I(std::size_t num_qubits, std::uint32_t sign) {
    return PauliString(num_qubits, sign);
}

PauliString PauliString::X(std::size_t num_qubits, std::size_t index, std::uint32_t sign) {
    auto ret = PauliString(num_qubits, sign);
    ret.xs_.set(index);
    return ret;
}

PauliString PauliString::Z(std::size_t num_qubits, std::size_t index, std::uint32_t sign) {
    auto ret = PauliString(num_qubits, sign);
    ret.zs_.set(index);
    return ret;
}

PauliString PauliString::Y(std::size_t num_qubits, std::size_t index, std::uint32_t sign) {
    auto ret = PauliString(num_qubits, sign);
    ret.xs_.set(index);
    ret.zs_.set(index);
    return ret;
}

std::string PauliString::ToString(const bool show_sign) const {
    auto ret = std::string();
    ret.reserve(3 + GetNumQubits());

    if (show_sign) {
        switch (sign_) {
            case 0:
                ret += "+1 ";
                break;
            case 1:
                ret += "+i ";
                break;
            case 2:
                ret += "-1 ";
                break;
            case 3:
                ret += "-i ";
                break;
            default:
                throw std::logic_error("sign_ must be smaller than 4.");
        }
    }
    for (auto i = std::size_t{0}; i < GetNumQubits(); ++i) {
        if (xs_[i] && zs_[i]) {
            ret += 'Y';
        } else if (xs_[i]) {
            ret += 'X';
        } else if (zs_[i]) {
            ret += 'Z';
        } else {
            ret += 'I';
        }
    }
    return ret;
}

PauliString& PauliString::operator*=(const PauliString& other) {
    if (xs_.size() != other.xs_.size()) {
        throw std::invalid_argument("PauliString length mismatch.");
    }

    auto temp1 = xs_ & other.zs_;
    auto temp2 = zs_ & other.xs_;
    auto anti_commute = temp1 ^ temp2;
    auto anti_commute_count = anti_commute.count();
    auto negative_sign_count = (anti_commute & (xs_ ^ (other.zs_ & (zs_ | other.xs_)))).count();

    sign_ = (sign_ + other.sign_ + anti_commute_count + 2 * negative_sign_count) % 4;

    // Multiply Pauli operators bitwise
    xs_ ^= other.xs_;
    zs_ ^= other.zs_;

    return *this;
}

PauliString& PauliString::MulFromLeft(const PauliString& other) {
    if (xs_.size() != other.xs_.size()) {
        throw std::invalid_argument("PauliString length mismatch.");
    }

    auto temp1 = xs_ & other.zs_;
    auto temp2 = zs_ & other.xs_;
    auto anti_commute = temp1 ^ temp2;
    auto anti_commute_count = anti_commute.count();
    auto negative_sign_count = (anti_commute & (other.xs_ ^ (zs_ & (other.zs_ | xs_)))).count();

    sign_ = (sign_ + other.sign_ + anti_commute_count + 2 * negative_sign_count) % 4;

    // Multiply Pauli operators bitwise
    xs_ ^= other.xs_;
    zs_ ^= other.zs_;

    return *this;
}

bool PauliString::IsCommuteWith(const PauliString& other) const {
    // Calculate the number of positions where this has X and other has Z,
    // or this has Z and other has X.
    // The sum of these counts tells us how many times the two Pauli strings anti-commute.
    // If this number is even, the Pauli strings commute; if odd, they anti-commute.
    auto xz_overlap = (xs_ & other.zs_).count();  // Positions where X (this) meets Z (other)
    auto zx_overlap = (zs_ & other.xs_).count();  // Positions where Z (this) meets X (other)
    auto anti_commute_count = xz_overlap + zx_overlap;

    // Commute if the total anti-commuting positions is even
    return (anti_commute_count % 2) == 0;
}

PauliString operator*(const PauliString& lhs, const PauliString& rhs) {
    auto res = PauliString(lhs);
    res *= rhs;
    return res;
}

bool ExchangeRotation(PauliRotation& lhs, PauliRotation& rhs) {
    if (!rhs.IsClifford()) {
        return false;
    }

    if (!lhs.GetPauli().IsCommuteWith(rhs.GetPauli())) {
        // If anti-commute:
        // P'=lhs
        // P =rhs
        // P'^{\phi} * P^{\theta} ---> P^{\theta} * (i P P')^{\phi}
        // \theta = \pi/2 or \pi/4
        lhs.GetMutPauli().MulFromLeft(rhs.GetPauli());

        // Add 1 to calculate: "i" P P'
        lhs.GetMutPauli().AddSign(1);
    }
    return true;
}

bool MergeClifford(PauliRotation& clifford, PauliMeasurement& measurement) {
    if (!clifford.IsClifford()) {
        return false;
    }

    if (!clifford.GetPauli().IsCommuteWith(measurement.GetPauli())) {
        const auto theta = clifford.GetTheta();

        if (theta == PauliRotation::Theta::pi_over_4) {
            // If anti-commute:
            // P'=measurement
            // P =clifford
            // P'^{measurement} * P^{\pi/4} ---> (i P P')^{measurement}
            measurement.GetMutPauli().MulFromLeft(clifford.GetPauli());

            // Add 1 to calculate: "i" P P'
            measurement.GetMutPauli().AddSign(1);
        } else if (theta == PauliRotation::Theta::pi_over_2) {
            measurement.GetMutPauli().AddSign(2);
        }
    }
    return true;
}

void PauliCircuit::MoveNonCliffordForward() {
    auto itr = ls_.begin();

    while (itr != ls_.end()) {
        auto next = std::next(itr);
        if (itr->IsNonClifford()) {
            MoveNonCliffordForwardImpl(itr);
        }
        itr = next;
    }
}

void PauliCircuit::MergeCliffordToMeasurement() {
    auto itr = ls_.rbegin();
    while (itr != ls_.rend()) {
        auto next = std::next(itr);

        if (itr->IsClifford()) {
            // Convert reverse iterator to forward iterator
            // *(std::prev(itr.base())) == *itr
            MergeCliffordToMeasurementImpl(std::prev(itr.base()));
        }

        itr = next;
    }
}

void PauliCircuit::ToStandardFormat() {
    MoveNonCliffordForward();
    MergeCliffordToMeasurement();
    PopOperationAfterMeasurement();
}

bool PauliCircuit::IsValid() const {
    for (const auto& x : ls_) {
        if (!x.IsValid()) {
            return false;
        }
    }
    return true;
}

bool PauliCircuit::IsStandardFormat() const {
    auto itr = ls_.begin();

    // Non-clifford block
    while (itr != ls_.end()) {
        if (!itr->IsNonClifford()) {
            break;
        }
        itr++;
    }

    // Measurement blck
    while (itr != ls_.end()) {
        if (!itr->IsMeasurement()) {
            break;
        }
        itr++;
    }

    return itr == ls_.end();
}

bool PauliCircuit::ExchangeRotationImpl(Iterator lhs, Iterator rhs) {
    auto* lhs_rotation = std::get_if<PauliRotation>(std::addressof(lhs->value));
    auto* rhs_rotation = std::get_if<PauliRotation>(std::addressof(rhs->value));
    if (lhs_rotation == nullptr || rhs_rotation == nullptr) {
        return false;
    }

    auto exchanged = ExchangeRotation(*lhs_rotation, *rhs_rotation);
    if (!exchanged) {
        return false;
    }

    SwapListNodes(ls_, lhs, rhs);

    return true;
}

bool PauliCircuit::MergeCliffordImpl(Iterator clifford, Iterator measurement) {
    auto* lhs_clifford = std::get_if<PauliRotation>(std::addressof(clifford->value));
    auto* rhs_measurement = std::get_if<PauliMeasurement>(std::addressof(measurement->value));
    if (lhs_clifford == nullptr || rhs_measurement == nullptr) {
        return false;
    }

    auto merged = MergeClifford(*lhs_clifford, *rhs_measurement);
    if (!merged) {
        return false;
    }

    SwapListNodes(ls_, clifford, measurement);

    return true;
}

void PauliCircuit::MoveNonCliffordForwardImpl(Iterator non_clifford) {
    while (non_clifford != ls_.begin()) {
        auto prev = std::prev(non_clifford);
        if (prev->IsNonClifford()) {
            break;
        }

        // Order of iterator: prev -> non_clifford
        auto exchanged = ExchangeRotationImpl(non_clifford, prev);
        if (!exchanged) {
            throw std::logic_error("List broken");
        }
        // Order of iterator: non_clifford -> prev
    }
}

void PauliCircuit::MergeCliffordToMeasurementImpl(Iterator clifford) {
    while (clifford != ls_.end()) {
        auto next = std::next(clifford);
        if (next == ls_.end()) {
            break;
        }

        if (!next->IsMeasurement()) {
            break;
        }

        // Order of iterator: clifford -> next
        auto merged = MergeCliffordImpl(clifford, next);
        if (!merged) {
            throw std::logic_error("List broken");
        }
        // Order of iterator: next -> clifford
    }
}

void PauliCircuit::PopOperationAfterMeasurement() {
    while (!ls_.empty()) {
        if (!ls_.back().IsMeasurement()) {
            ls_.pop_back();
        } else {
            break;
        }
    }
}
}  // namespace qret::math
