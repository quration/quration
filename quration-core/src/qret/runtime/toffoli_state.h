/**
 * @file qret/runtime/toffoli_state.h
 * @brief Quantum state: toffoli.
 */

#ifndef QRET_RUNTIME_TOFFOLI_STATE_H
#define QRET_RUNTIME_TOFFOLI_STATE_H

#include <complex>
#include <stdexcept>

#include "qret/base/type.h"
#include "qret/qret_export.h"
#include "qret/runtime/quantum_state.h"

namespace qret::runtime {
/**
 * @brief A quantum state class that efficiently simulates X, CX, CCX gates.
 * @details ToffoliState is a quantum state class that efficiently simulates gates that can be
 * calculated over the computational basis.
 * However, gates that create superposition states like arbitrary rotations except Hadamard gate
 * cannot be simulated.
 * If you use Hadamard gate, you need to set the SimulatorConfig's max_superpositions value to 2 or
 * more.
 *
 * The gates that can be simulated by the ToffoliState class are as follows:
 * Measurement, X, Y, Z, H, S, SDag, T, TDag, CX, CY, CZ, CCX, CCY, CCZ, MCX
 */
class QRET_EXPORT ToffoliState : public QuantumState {
public:
    using Coefficient = std::complex<double>;
    using State = std::vector<std::int_fast8_t>;

    ToffoliState(std::uint64_t seed, std::uint64_t num_qubits, double eps = 1e-8)
        : QuantumState(seed)
        , eps_{eps}
        , states_(1, {1.0, State(num_qubits, 0)}) {}

    void Measure(std::uint64_t q, std::uint64_t r) override;
    void X(std::uint64_t q) override;
    void Y(std::uint64_t q) override;
    void Z(std::uint64_t q) override;
    void H(std::uint64_t q) override;
    void S(std::uint64_t q) override;
    void SDag(std::uint64_t q) override;
    void T(std::uint64_t q) override;
    void TDag(std::uint64_t q) override;
    void RX(std::uint64_t, double) override {
        throw std::runtime_error("cannot run RX");
    }
    void RY(std::uint64_t, double) override {
        throw std::runtime_error("cannot run RY");
    }
    void RZ(std::uint64_t, double) override {
        throw std::runtime_error("cannot run RZ");
    }
    void CX(std::uint64_t t, std::uint64_t c) override;
    void CY(std::uint64_t t, std::uint64_t c) override;
    void CZ(std::uint64_t t, std::uint64_t c) override;
    void CCX(std::uint64_t t, std::uint64_t c0, std::uint64_t c1) override;
    void CCY(std::uint64_t t, std::uint64_t c0, std::uint64_t c1) override;
    void CCZ(std::uint64_t t, std::uint64_t c0, std::uint64_t c1) override;
    void MCX(std::uint64_t id, const std::vector<std::uint64_t>& c) override;
    void RotateGlobalPhase(double) override {
        throw std::runtime_error("cannot run RotateGlobalPhase");
    }

    bool IsClean(std::uint64_t q) const override;

    bool ReadRegister(std::uint64_t r) const {
        return QuantumState::ReadRegister(r);
    }
    std::vector<bool> ReadRegisters(std::uint64_t start, std::uint64_t size) const {
        return QuantumState::ReadRegisters(start, size);
    }

    std::uint64_t GetNumSuperpositions() const override {
        return states_.size();
    }
    bool IsComputationalBasis() const {
        return GetNumSuperpositions() == 1;
    }
    double Calc0Prob(std::uint64_t q) const {
        return QuantumState::Calc0Prob(q);
    }
    double Calc1Prob(std::uint64_t q) const override;
    bool IsSuperposed(std::uint64_t q) const {
        return QuantumState::IsSuperposed(q);
    }
    Coefficient GetPhase() const {
        if (GetNumSuperpositions() != 1) {
            throw std::runtime_error("cannot get phase of superposed state");
        }
        return states_.front().first;
    }

    // Get the value of qubits
    BigInt GetValue(std::uint64_t start, std::uint64_t size) const;

    const std::vector<std::pair<Coefficient, State>>& GetRawVector() const {
        return states_;
    }

private:
    void Reduce();

    double eps_;
    std::vector<std::pair<Coefficient, State>> states_;
};
}  // namespace qret::runtime

#endif  // QRET_RUNTIME_TOFFOLI_STATE_H
