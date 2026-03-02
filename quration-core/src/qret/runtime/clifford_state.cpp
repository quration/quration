/**
 * @file qret/runtime/clifford_state.cpp
 * @brief Quantum state: tableau based stabilizer group.
 */

#include "qret/runtime/clifford_state.h"

#include <stdexcept>

#include "qret/math/clifford.h"
#include "qret/math/pauli.h"
#include "qret/runtime/quantum_state.h"

namespace qret::runtime {
using math::PauliString;
using math::StabilizerTableau;

CliffordState::CliffordState(std::uint64_t seed, std::uint64_t num_qubits)
    : QuantumState(seed)
    , num_qubits_(num_qubits)
    , sg_(num_qubits) {}

void CliffordState::Measure(std::uint64_t q, std::uint64_t r) {
    const auto measurement_base = PauliString::Z(num_qubits_, q);

    bool found_anti_commute = false;
    std::size_t anti_commute_idx = 0;

    for (std::size_t i = 0; i < num_qubits_; i++) {
        if (!sg_[i].IsCommuteWith(measurement_base)) {
            if (!found_anti_commute) {
                found_anti_commute = true;
                anti_commute_idx = i;
            } else {
                // Multiply sg_[anti_commute_idx] so that sg_[i] is commute with measurement_base.
                sg_[i] *= sg_[anti_commute_idx];
            }
        }
    }

    auto result = true;
    if (found_anti_commute) {
        sg_[anti_commute_idx] = measurement_base;

        // Set sign randomly.
        if (Flip(0.5)) {
            sg_[anti_commute_idx].SetSign(0);  // 1 -> 0
            result = false;
        } else {
            sg_[anti_commute_idx].SetSign(2);  // -1 -> 1
            result = true;
        }
    } else {
        const auto [contain, sign] = sg_.CalculateSign(measurement_base);
        if (!contain) {
            throw std::logic_error("measurement base is not commute with some generator");
        }
        if (sign == 0b00) {
            result = false;
        } else if (sign == 0b10) {
            result = true;
        } else {
            throw std::logic_error("Found i or -i in the sign of measurement base.");
        }
    }

    SaveMeasuredResult(r, result);
}
void CliffordState::X(std::uint64_t q) {
    StabilizerTableau::X(num_qubits_, q).Apply(sg_);
}
void CliffordState::Y(std::uint64_t q) {
    StabilizerTableau::Y(num_qubits_, q).Apply(sg_);
}
void CliffordState::Z(std::uint64_t q) {
    StabilizerTableau::Z(num_qubits_, q).Apply(sg_);
}
void CliffordState::H(std::uint64_t q) {
    StabilizerTableau::H(num_qubits_, q).Apply(sg_);
}
void CliffordState::S(std::uint64_t q) {
    StabilizerTableau::S(num_qubits_, q).Apply(sg_);
}
void CliffordState::SDag(std::uint64_t q) {
    StabilizerTableau::SDag(num_qubits_, q).Apply(sg_);
}
void CliffordState::CX(std::uint64_t t, std::uint64_t c) {
    StabilizerTableau::CX(num_qubits_, t, c).Apply(sg_);
}
void CliffordState::CY(std::uint64_t t, std::uint64_t c) {
    StabilizerTableau::CY(num_qubits_, t, c).Apply(sg_);
}
void CliffordState::CZ(std::uint64_t t, std::uint64_t c) {
    StabilizerTableau::CZ(num_qubits_, t, c).Apply(sg_);
}
bool CliffordState::IsClean(std::uint64_t q) const {
    return Calc1Prob(q) < Eps;
}
std::uint64_t CliffordState::GetNumSuperpositions() const {
    std::uint64_t num_superpositions = 1;

    for (std::size_t i = 0; i < num_qubits_; i++) {
        const auto prob = Calc1Prob(i);
        if (0.1 < prob && prob < 0.9) {
            num_superpositions *= 2;
        }
    }

    return num_superpositions;
}
double CliffordState::Calc1Prob(std::uint64_t q) const {
    const auto measurement_base = PauliString::Z(num_qubits_, q);
    const auto [contain, sign] = sg_.CalculateSign(measurement_base);

    if (!contain) {
        return 0.5;
    }

    if (sign == 0b00) {
        // 1 -> 0
        return 0;
    } else if (sign == 0b10) {
        // -1 -> 1
        return 1;
    } else {
        throw std::logic_error("Found i or -i in the sign of measurement base.");
    }
}
}  // namespace qret::runtime
