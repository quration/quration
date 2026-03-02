/**
 * @file qret/algorithm/phase_estimation/trotter.cpp
 * @brief Trotter expansion.
 */

#include "qret/algorithm/phase_estimation/trotter.h"

#include <fstream>
#include <map>
#include <stdexcept>
#include <string>

#include "qret/frontend/argument.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/intrinsic.h"

namespace qret::frontend::gate {
// PauliTerm
void from_json(const nlohmann::json& j, PauliTerm& pauli_term) {
    j.at("coeff").get_to(pauli_term.coeff);

    pauli_term.pauli_string.clear();
    for (const auto& item : j.at("pauli_string").items()) {
        const auto qubit_index = static_cast<std::size_t>(std::stoull(item.key()));

        const auto s = item.value().get<std::string>();
        pauli_term.pauli_string[qubit_index] = math::PauliFromString(s);
    }
}
void to_json(nlohmann::json& j, const PauliTerm& pauli_term) {
    nlohmann::json pauli_string_json;
    for (const auto& [qubit_index, pauli_op] : pauli_term.pauli_string) {
        pauli_string_json[std::to_string(qubit_index)] = ToString(pauli_op);
    }

    j["coeff"] = pauli_term.coeff;
    j["pauli_string"] = pauli_string_json;
}
// Hamiltonian
void from_json(const nlohmann::json& j, Hamiltonian& h) {
    j.at("num_qubits").get_to(h.num_qubits);
    j.at("pauli_terms").get_to(h.pauli_terms);
}
void to_json(nlohmann::json& j, const Hamiltonian& h) {
    j["num_qubits"] = h.num_qubits;
    j["pauli_terms"] = h.pauli_terms;
}
Hamiltonian ReadHamiltonian(const std::string& path) {
    auto ifs = std::ifstream(path.data());
    if (!ifs.good()) {
        throw std::runtime_error("Could not open file: " + path);
    }
    nlohmann::json j;
    ifs >> j;
    return j.get<Hamiltonian>();
}

frontend::Circuit* PauliRotationGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto q = GetQubits(0);

    for (const auto& [idx, op] : pauli_string) {
        switch (op) {
            case math::Pauli::X:
                H(q[idx]);
                break;
            case math::Pauli::Y:
                SDag(q[idx]);
                H(q[idx]);
                break;
            case math::Pauli::Z:
            case math::Pauli::I:
                break;
            default:
                throw std::runtime_error("Unsupported pauli character.");
        }
    }

    auto target_qubits = std::vector<std::size_t>();
    for (const auto& [idx, _] : pauli_string) {
        target_qubits.emplace_back(idx);
    }
    for (auto i = std::size_t{0}; i < target_qubits.size() - 1; ++i) {
        CX(q[target_qubits[i]], q[target_qubits[i + 1]]);
    }
    RZ(q[target_qubits.back()], angle);

    for (const auto& [idx, op] : pauli_string) {
        switch (op) {
            case math::Pauli::X:
                frontend::gate::H(q[idx]);
                break;
            case math::Pauli::Y:
                frontend::gate::H(q[idx]);
                frontend::gate::S(q[idx]);
                break;
            case math::Pauli::Z:
            case math::Pauli::I:
                break;
            default:
                throw std::runtime_error("Unsupported pauli character.");
        }
    }

    return EndCircuitDefinition();
}
void AddPauliRotation(
        const frontend::Qubits& q,
        const std::map<std::size_t, math::Pauli>& pauli_string,
        const double angle
) {
    auto gen = PauliRotationGen(q.GetBuilder(), q.Size(), pauli_string, angle);
    auto* circuit = gen.Generate();
    (*circuit)(q);
}

frontend::Circuit* TrotterStepGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }

    BeginCircuitDefinition();
    auto q = GetQubits(0);
    for (const auto& term : hamiltonian.pauli_terms) {
        const auto step_angle = 2 * term.coeff * step_factor;
        AddPauliRotation(q, term.pauli_string, step_angle);
    }
    return EndCircuitDefinition();
}
void AddTrotterStep(
        const frontend::Qubits& q,
        const Hamiltonian& hamiltonian,
        const double step_factor
) {
    auto gen = TrotterStepGen(q.GetBuilder(), hamiltonian, step_factor);
    auto* circuit = gen.Generate();
    (*circuit)(q);
}

frontend::Circuit* TrotterGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto q = GetQubits(0);
    for (auto i = std::size_t{0}; i < num_trotter_steps; ++i) {
        AddTrotterStep(q, hamiltonian, time / static_cast<double>(num_trotter_steps));
    }
    return EndCircuitDefinition();
}

}  // namespace qret::frontend::gate
