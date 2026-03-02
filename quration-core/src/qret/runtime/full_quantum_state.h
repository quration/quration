/**
 * @file qret/runtime/full_quantum_state.h
 * @brief Quantum state: 2^N x 2^N.
 */

#ifndef QRET_RUNTIME_FULL_QUANTUM_STATE_H
#define QRET_RUNTIME_FULL_QUANTUM_STATE_H

#include <complex>
#include <memory>
#include <stdexcept>

#include "qret/qret_export.h"
#include "qret/runtime/quantum_state.h"

namespace qret::runtime {
/**
 * @brief Determine if Qulacs can be used as the implementation of 'FullQuantumState'.
 * @details When the USE_QULACS macro is defined, you can use Qulacs.
 *
 * @return true if Qulacs can be used.
 * @return false otherwise.
 */
QRET_EXPORT bool CanUseQulacs();

/**
 * @brief A quantum state class that simulates all instructions.
 * @details This class consumes approximately 2^N memory if the number of qubits is N.
 */
class QRET_EXPORT FullQuantumState : public QuantumState {
public:
    FullQuantumState(
            std::uint64_t seed,
            std::uint64_t num_qubits,
            bool use_qulacs,
            bool save_operation_matrix
    );
    ~FullQuantumState() override;

    std::uint64_t NumQubits() const {
        return num_qubits_;
    }

    void Measure(std::uint64_t q, std::uint64_t r) override;
    void X(std::uint64_t q) override;
    void Y(std::uint64_t q) override;
    void Z(std::uint64_t q) override;
    void H(std::uint64_t q) override;
    void S(std::uint64_t q) override;
    void SDag(std::uint64_t q) override;
    void T(std::uint64_t q) override;
    void TDag(std::uint64_t q) override;
    void RX(std::uint64_t, double) override;
    void RY(std::uint64_t, double) override;
    void RZ(std::uint64_t, double) override;
    void CX(std::uint64_t t, std::uint64_t c) override;
    void CY(std::uint64_t t, std::uint64_t c) override;
    void CZ(std::uint64_t t, std::uint64_t c) override;
    void CCX(std::uint64_t t, std::uint64_t c0, std::uint64_t c1) override;
    void CCY(std::uint64_t t, std::uint64_t c0, std::uint64_t c1) override;
    void CCZ(std::uint64_t t, std::uint64_t c0, std::uint64_t c1) override;
    void MCX(std::uint64_t, const std::vector<std::uint64_t>&) override {
        throw std::runtime_error("currently MCX is not supported");
    }
    void RotateGlobalPhase(double) override;

    bool IsClean(std::uint64_t q) const override;

    bool ReadRegister(std::uint64_t r) const {
        return QuantumState::ReadRegister(r);
    }
    std::vector<bool> ReadRegisters(std::uint64_t start, std::uint64_t size) const {
        return QuantumState::ReadRegisters(start, size);
    }

    std::uint64_t GetNumSuperpositions() const override;
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

    double GetEntropy() const;
    std::vector<std::uint64_t> Sampling(std::size_t num_samples) const;

    /**
     * @brief Get the internal pointer to the state vector.
     *
     * @note
     * - The returned pointer is nullptr or pointer to the first element of the vector.
     * - The returned pointer is **borrowed**: DO NOT delete or free it.
     * - The pointer remains valid as long as this object exists and is not modified.
     * - The total size of the array is 2^N (where N is the number of target qubits).
     *
     * @return const std::complex<double>* Pointer to the first element of the state vector or
     * nullptr.
     */
    const std::complex<double>* GetStateVector() const;

    /**
     * @brief Get the internal pointer to the operation matrix.
     * The matrix represents the linear operator acting on the qubits.
     * The data is stored in **row-major order** (C-style).
     *
     * @note
     * - The returned pointer is nullptr or pointer to the first element of the matrix.
     * - The returned pointer is **borrowed**: DO NOT delete or free it.
     * - The pointer remains valid as long as this object exists and is not modified.
     * - The total size of the array is 2^N * 2^N (where N is the number of target qubits).
     *
     * @return const std::complex<double>* Pointer to the first element of the matrix or nullptr.
     */
    const std::complex<double>* GetOperationMatrix() const;

private:
    struct Impl;
    struct QRETFullQuantumState;
#ifdef USE_QULACS
    struct QulacsFullQuantumState;
#endif

    const std::uint64_t num_qubits_;  //!< the number of qubits
    std::unique_ptr<Impl> impl_;
};
}  // namespace qret::runtime

#endif  // QRET_RUNTIME_FULL_QUANTUM_STATE_H
