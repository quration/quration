/**
 * @file qret/algorithm/phase_estimation/trotter.h
 * @brief Trotter expansion.
 */

#ifndef QRET_ALGORITHM_PHASE_ESTIMATION_TROTTER_H
#define QRET_ALGORITHM_PHASE_ESTIMATION_TROTTER_H

#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "qret/frontend/circuit_generator.h"
#include "qret/math/pauli.h"
#include "qret/qret_export.h"

namespace qret::frontend::gate {

/**
 * @brief Pauli term.
 * @details Pair of coefficient and Pauli string.
 */
struct QRET_EXPORT PauliTerm {
    double coeff;
    std::map<std::size_t, math::Pauli> pauli_string;

    PauliTerm() = default;
    explicit PauliTerm(double coeff, std::map<std::size_t, math::Pauli>& pauli_string)
        : coeff(coeff)
        , pauli_string(pauli_string) {

        };
};
void QRET_EXPORT from_json(const nlohmann::json& j, PauliTerm& pauli_term);
void QRET_EXPORT to_json(nlohmann::json& j, const PauliTerm& pauli_term);

/**
 * @brief Hamiltonian.
 * @details Hamiltonian constructed by Pauli terms.
 */
struct QRET_EXPORT Hamiltonian {
    std::size_t num_qubits;
    std::vector<PauliTerm> pauli_terms;

    Hamiltonian() = default;
    explicit Hamiltonian(std::size_t num_qubits, std::vector<PauliTerm>& pauli_terms)
        : num_qubits(num_qubits)
        , pauli_terms(pauli_terms) {};
};
void QRET_EXPORT from_json(const nlohmann::json& j, Hamiltonian& h);
void QRET_EXPORT to_json(nlohmann::json& j, const Hamiltonian& h);

/**
 * @brief Load Hamiltonian from JSON file.
 *
 * @param path JSON file path
 * @return Hamiltonian object.
 */
Hamiltonian QRET_EXPORT ReadHamiltonian(const std::string& path);

/**
 * @brief Pauli rotation.
 * @details Implementation of Pauli rotation quantum circuit by a Pauli string.
 */
struct QRET_EXPORT PauliRotationGen : CircuitGenerator {
    std::size_t num_qubits;
    std::map<std::size_t, math::Pauli> pauli_string;
    double angle;

    explicit PauliRotationGen(
            CircuitBuilder* builder,
            const std::size_t num_qubits,
            const std::map<std::size_t, math::Pauli>& pauli_string,
            const double angle
    )
        : CircuitGenerator(builder)
        , num_qubits(num_qubits)
        , pauli_string(pauli_string)
        , angle(angle) {}
    std::string GetName() const override {
        auto serialized_pauli_string = std::string();
        for (const auto& pair : pauli_string) {
            serialized_pauli_string += fmt::format("{}{}", pair.first, ToString(pair.second));
        }
        return fmt::format("PauliRotation({}, {:.4f})", serialized_pauli_string, angle);
    }
    std::string GetCacheKey() const override {
        return GetName();
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, num_qubits, Attribute::Operate);
    }
    frontend::Circuit* Generate() const override;
};

/**
 * @brief Add PauliRotation.
 *
 * @param q Target qubits.
 * @param pauli_string Pauli string.
 * @param angle Rotation angle.
 */
void QRET_EXPORT AddPauliRotation(
        const frontend::Qubits& q,
        const std::map<std::size_t, math::Pauli>& pauli_string,
        double angle
);

/**
 * @brief One Trotter step quantum circuit.
 * @details One step quantum circuit of Trotter expansion for the Hamiltonian.
 */
struct QRET_EXPORT TrotterStepGen : CircuitGenerator {
    Hamiltonian hamiltonian;
    double step_factor;
    static inline const char* Name = "TrotterStep";

    explicit TrotterStepGen(CircuitBuilder* builder, const Hamiltonian& h, const double step_factor)
        : CircuitGenerator(builder)
        , hamiltonian(h)
        , step_factor(step_factor) {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, hamiltonian.num_qubits, Attribute::Operate);
    }
    frontend::Circuit* Generate() const override;
};

/**
 * @brief Add one Trotter step.
 *
 * @param q Target qubits
 * @param hamiltonian Hamiltonian.
 * @param step_factor the size of the time slice applied in each layer of the Trotterized circuit.
 */
void QRET_EXPORT
AddTrotterStep(const frontend::Qubits& q, const Hamiltonian& hamiltonian, double step_factor);

/**
 * @brief Trotterized circuit for Hamiltonian time evolution.
 * @details Simulates the time evolution operator of the Hamiltonian using the Trotter expansion.
 */
struct QRET_EXPORT TrotterGen : CircuitGenerator {
    Hamiltonian hamiltonian;
    std::size_t num_trotter_steps;
    double time;

    TrotterGen(
            CircuitBuilder* builder,
            const Hamiltonian& h,
            const std::size_t num_trotter_steps,
            const double time
    )
        : CircuitGenerator(builder)
        , hamiltonian(h)
        , num_trotter_steps(num_trotter_steps)
        , time(time) {}
    std::string GetName() const override {
        return fmt::format("Trotter({})", num_trotter_steps);
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, hamiltonian.num_qubits, Attribute::Operate);
    }
    frontend::Circuit* Generate() const override;
};

}  // namespace qret::frontend::gate
#endif  // QRET_ALGORITHM_PHASE_ESTIMATION_TROTTER_H
