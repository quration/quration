/**
 * @file qret/math/clifford.cpp
 * @brief Stabilizer group and stabilizer tableau.
 */

#include "qret/math/clifford.h"

#include <stdexcept>
#include <unordered_set>

#include "qret/math/pauli.h"

namespace qret::math {
StabilizerGroup::StabilizerGroup(const std::size_t num_qubits)
    : num_qubits_(num_qubits) {
    generators_.reserve(num_qubits_);
    for (std::size_t i = 0; i < num_qubits_; i++) {
        generators_.emplace_back(PauliString::Z(num_qubits, i));
    }
}

StabilizerGroup::StabilizerGroup(
        const std::size_t num_qubits,
        const std::vector<PauliString>& generators
)
    : num_qubits_{num_qubits}
    , generators_{generators} {
    Validate();
}

void StabilizerGroup::Validate() const {
    if (GetNumQubits() < GetNumStabilizerGenerators()) {
        throw std::runtime_error(
                "The number of stabilizer generators is larger than the number of qubits."
        );
    }

    for (const auto& stabilizer : generators_) {
        // Check: All stabilizer generators have the same qubit size.
        if (stabilizer.GetNumQubits() != GetNumQubits()) {
            throw std::runtime_error("Qubit size of some stabilizer generators is different.");
        }
        // Check: Stabilizer generator's phase is -1 or 1.
        if (stabilizer.GetSign() % 2 != 0) {
            throw std::runtime_error("Stabilizer generator's phase isn't -1 nor 1.");
        }
    }

    // Check: All stabilizer generators are commutative.
    for (auto i = std::size_t{0}; i < GetNumStabilizerGenerators(); ++i) {
        for (auto j = i + 1; j < GetNumStabilizerGenerators(); ++j) {
            if (!generators_[i].IsCommuteWith(generators_[j])) {
                throw std::runtime_error("Stabilizer generators isn't commutative.");
            }
        }
    }

    // Check: Stabilizer generators do not generate -I.
    // Check: Stabilizer generators are independent.

    auto normalized_generators = std::vector<PauliString>{};
    normalized_generators.reserve(GetNumStabilizerGenerators());
    for (auto generator : generators_) {
        for (auto& normalized : normalized_generators) {
            generator = std::min(generator, generator * normalized);
        }

        if (generator.IsI()) {
            if (generator.GetSign() == 1) {
                throw std::runtime_error("Stabilizer generators are not independent.");
            } else {
                throw std::runtime_error("Stabilizer generators produce -I.");
            }
        }
        normalized_generators.emplace_back(std::move(generator));
    }
}

void StabilizerGroup::NormalizeInPlace() {
    auto normalized_generators = NormalizedGenerators();
    std::swap(generators_, normalized_generators);
}

StabilizerGroup StabilizerGroup::Normalized() const {
    return StabilizerGroup(GetNumQubits(), NormalizedGenerators());
}

std::tuple<bool, std::uint8_t> StabilizerGroup::CalculateSign(PauliString pauli) const {
    auto normalized_generators = NormalizedGenerators();
    for (const auto& generator : normalized_generators) {
        pauli = std::min(pauli, pauli * generator);
    }
    return {pauli.IsI(), pauli.GetSign()};
}

std::vector<PauliString> StabilizerGroup::NormalizedGenerators() const {
    auto normalized_generators = std::vector<PauliString>{};
    normalized_generators.reserve(GetNumStabilizerGenerators());
    for (auto generator : generators_) {
        for (auto& normalized : normalized_generators) {
            generator = std::min(generator, generator * normalized);
        }
        normalized_generators.emplace_back(std::move(generator));
    }
    return normalized_generators;
}

StabilizerTableau::StabilizerTableau(
        const std::size_t num_qubits,
        const std::vector<std::size_t>& target_idx,
        const std::vector<PauliString>& xs,
        const std::vector<PauliString>& zs
)
    : num_qubits_{num_qubits}
    , target_idx_{target_idx}
    , xs_{xs}
    , zs_{zs} {
    const auto nt = target_idx.size();

    // Check size of xs.
    if (xs.size() != nt) {
        throw std::logic_error("Size of 'xs' is not equal to the number of targets");
    }
    for (const auto& x : xs) {
        if (x.GetNumQubits() != nt) {
            throw std::logic_error(
                    "Size of pauli string in 'xs' is not equal to the number of targets."
            );
        }
    }

    // Check size of zs.
    if (zs.size() != nt) {
        throw std::logic_error("Size of 'zs' is not equal to the number of targets");
    }
    for (const auto& z : zs) {
        if (z.GetNumQubits() != nt) {
            throw std::logic_error(
                    "Size of pauli string in 'zs' is not equal to the number of targets."
            );
        }
    }

    // Check target size.
    for (const auto t : target_idx) {
        if (t >= num_qubits) {
            throw std::out_of_range("Target index is larger than the number of qubits.");
        }
    }

    // Check target is not duplicated.
    const auto tmp = std::unordered_set<std::size_t>(target_idx.begin(), target_idx.end());
    if (tmp.size() < target_idx.size()) {
        throw std::invalid_argument("Target index is duplicated.");
    }
}
void StabilizerTableau::Apply(PauliString& pauli) const {
    if (pauli.GetNumQubits() != GetNumQubits()) {
        throw std::runtime_error("The number of qubits in pauli is not matched");
    }

    // Apply clifford operation to target pauli.
    auto target = PauliString(GetNumTargets());
    auto num_y = std::uint32_t{0};
    for (auto i = std::size_t{0}; i < GetNumTargets(); ++i) {
        switch (pauli[target_idx_[i]]) {
            case 'X':
                target *= xs_[i];
                break;
            case 'Z':
                target *= zs_[i];
                break;
            case 'Y':  // Y = i X Z
                target *= xs_[i];
                target *= zs_[i];
                num_y++;
                break;
            default:
                break;
        }
    }

    // Check.
    if ((target.GetSign() + num_y) % 2 != 0) {
        throw std::logic_error("Sign becomes i or -i after clifford operation");
    }

    // Update pauli.
    for (auto i = std::size_t{0}; i < GetNumTargets(); ++i) {
        pauli.Set(target_idx_[i], target[i]);
    }
    pauli.SetSign(pauli.GetSign() + target.GetSign() + num_y);
}
StabilizerTableau StabilizerTableau::FromPauliString(const PauliString& pauli) {
    const auto nq = pauli.GetNumQubits();

    auto target_idx = std::vector<std::size_t>{};
    for (auto t = std::size_t{0}; t < nq; ++t) {
        if (pauli[t] != 'I') {
            target_idx.emplace_back(t);
        }
    }

    const auto nt = target_idx.size();
    auto xs = std::vector<PauliString>{};
    auto zs = std::vector<PauliString>{};
    xs.reserve(target_idx.size());
    zs.reserve(target_idx.size());
    for (auto i = std::size_t{0}; i < target_idx.size(); ++i) {
        const auto t = target_idx[i];
        switch (pauli[t]) {
            case 'X':
                xs.emplace_back(PauliString::X(nt, i));
                zs.emplace_back(PauliString::Z(nt, i, 2));
                break;
            case 'Y':
                xs.emplace_back(PauliString::X(nt, i, 2));
                zs.emplace_back(PauliString::Z(nt, i, 2));
                break;
            case 'Z':
                xs.emplace_back(PauliString::X(nt, i, 2));
                zs.emplace_back(PauliString::Z(nt, i));
                break;
            default:
                break;
        }
    }

    return {nq, target_idx, xs, zs};
}
StabilizerTableau StabilizerTableau::X(std::size_t num_qubits, std::uint64_t q) {
    // X X XDag = X
    // X Z XDag = -Z
    return StabilizerTableau(num_qubits, {q}, {PauliString("x")}, {PauliString("z", 2)});
}
StabilizerTableau StabilizerTableau::Y(std::size_t num_qubits, std::uint64_t q) {
    // Y X YDag = -X
    // Y Z YDag = -Z
    return StabilizerTableau(num_qubits, {q}, {PauliString("x", 2)}, {PauliString("z", 2)});
}
StabilizerTableau StabilizerTableau::Z(std::size_t num_qubits, std::uint64_t q) {
    // Z X ZDag = -X
    // Z Z ZDag = Z
    return StabilizerTableau(num_qubits, {q}, {PauliString("x", 2)}, {PauliString("z")});
}
StabilizerTableau StabilizerTableau::H(std::size_t num_qubits, std::uint64_t q) {
    // H X HDag = Z
    // H Z HDag = X
    return StabilizerTableau(num_qubits, {q}, {PauliString("z")}, {PauliString("x")});
}
StabilizerTableau StabilizerTableau::S(std::size_t num_qubits, std::uint64_t q) {
    // S X SDag = Y
    // S Z SDag = Z
    return StabilizerTableau(num_qubits, {q}, {PauliString("y")}, {PauliString("z")});
}
StabilizerTableau StabilizerTableau::SDag(std::size_t num_qubits, std::uint64_t q) {
    // SDag X SDagDag = -Y
    // SDag Z SDagDag = Z
    return StabilizerTableau(num_qubits, {q}, {PauliString("y", 2)}, {PauliString("z")});
}
StabilizerTableau StabilizerTableau::CZ(std::size_t num_qubits, std::uint64_t t, std::uint64_t c) {
    return StabilizerTableau(
            num_qubits,
            {c, t},
            {PauliString("xz"), PauliString("zx")},
            {PauliString("zi"), PauliString("iz")}
    );
}
StabilizerTableau StabilizerTableau::CX(std::size_t num_qubits, std::uint64_t t, std::uint64_t c) {
    return StabilizerTableau(
            num_qubits,
            {c, t},
            {PauliString("xx"), PauliString("ix")},
            {PauliString("zi"), PauliString("zz")}
    );
}
StabilizerTableau StabilizerTableau::CY(std::size_t num_qubits, std::uint64_t t, std::uint64_t c) {
    return StabilizerTableau(
            num_qubits,
            {c, t},
            {PauliString("xy", 2), PauliString("zx")},
            {PauliString("zi"), PauliString("zz")}
    );
}
}  // namespace qret::math
