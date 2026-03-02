/**
 * @file qret/runtime/clifford_state.h
 * @brief Quantum state: tableau based stabilizer group.
 */

#ifndef QRET_RUNTIME_CLIFFORD_STATE_H
#define QRET_RUNTIME_CLIFFORD_STATE_H

#include <stdexcept>

#include "qret/math/clifford.h"
#include "qret/runtime/quantum_state.h"

namespace qret::runtime {
class QRET_EXPORT CliffordState : public QuantumState {
public:
    CliffordState(std::uint64_t seed, std::uint64_t num_qubits);

    // Gate
    void Measure(std::uint64_t q, std::uint64_t r) override;
    void X(std::uint64_t q) override;
    void Y(std::uint64_t q) override;
    void Z(std::uint64_t q) override;
    void H(std::uint64_t q) override;
    void S(std::uint64_t q) override;
    void SDag(std::uint64_t q) override;
    void T(std::uint64_t /*q*/) override {
        throw std::runtime_error(
                "T gate is not supported in TableauBasesCliffordState because it is not a Clifford "
                "gate."
        );
    }
    void TDag(std::uint64_t /*q*/) override {
        throw std::runtime_error(
                "TDag gate is not supported in TableauBasesCliffordState because it is not a "
                "Clifford "
                "gate."
        );
    }
    void RX(std::uint64_t /*q*/, double /*theta*/) override {
        throw std::runtime_error(
                "RX is not supported in TableauBasesCliffordState because it is not a Clifford "
                "gate."
        );
    }
    void RY(std::uint64_t /*q*/, double /*theta*/) override {
        throw std::runtime_error(
                "RY is not supported in TableauBasesCliffordState because it is not a Clifford "
                "gate."
        );
    }
    void RZ(std::uint64_t /*q*/, double /*theta*/) override {
        throw std::runtime_error(
                "RZ is not supported in TableauBasesCliffordState because it is not a Clifford "
                "gate."
        );
    }

    void CX(std::uint64_t t, std::uint64_t c) override;
    void CY(std::uint64_t t, std::uint64_t c) override;
    void CZ(std::uint64_t t, std::uint64_t c) override;
    void CCX(std::uint64_t /*t*/, std::uint64_t /*c0*/, std::uint64_t /*c1*/) override {
        throw std::runtime_error(
                "CCX is not supported in TableauBasesCliffordState because it is not a Clifford "
                "gate."
        );
    }
    void CCY(std::uint64_t /*t*/, std::uint64_t /*c0*/, std::uint64_t /*c1*/) override {
        throw std::runtime_error(
                "CCY is not supported in TableauBasesCliffordState because it is not a Clifford "
                "gate."
        );
    }
    void CCZ(std::uint64_t /*t*/, std::uint64_t /*c0*/, std::uint64_t /*c1*/) override {
        throw std::runtime_error(
                "CCZ is not supported in TableauBasesCliffordState because it is not a Clifford "
                "gate."
        );
    }
    void MCX(std::uint64_t /*id*/, const std::vector<std::uint64_t>& /*c*/) override {
        throw std::runtime_error(
                "MCX is not supported in TableauBasesCliffordState because it is not a Clifford "
                "gate."
        );
    }
    void RotateGlobalPhase(double /*theta*/) override {
        throw std::runtime_error(
                "RotateGlobalPhase is not supported in TableauBasesCliffordState because it is not "
                "a "
                "Clifford gate."
        );
    }

    bool IsClean(std::uint64_t q) const override;
    std::uint64_t GetNumSuperpositions() const override;
    double Calc1Prob(std::uint64_t q) const override;

    const math::StabilizerGroup& GetStabilizerGroup() const {
        return sg_;
    }

private:
    std::size_t num_qubits_ = 0;
    math::StabilizerGroup sg_;
};
}  // namespace qret::runtime

#endif  // QRET_RUNTIME_CLIFFORD_STATE_H
