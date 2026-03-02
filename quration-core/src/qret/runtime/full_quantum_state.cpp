/**
 * @file qret/runtime/full_quantum_state.cpp
 * @brief Quantum state: 2^N x 2^N.
 */

#include "qret/runtime/full_quantum_state.h"

#include <Eigen/Dense>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <complex>
#include <memory>
#include <numbers>
#include <random>
#include <vector>

#include "qret/base/log.h"
#include "qret/runtime/quantum_state.h"
#ifdef USE_QULACS
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wfloat-conversion"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#pragma clang diagnostic ignored "-Wunqualified-std-cast-call"
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#include <cppsim/gate.hpp>
#include <cppsim/gate_factory.hpp>
#include <cppsim/state.hpp>
#pragma clang diagnostic pop
#endif

namespace qret::runtime {
bool CanUseQulacs() {
#ifdef USE_QULACS
    return true;
#else
    return false;
#endif
}

struct FullQuantumState::Impl {
    using Data = std::complex<double>;

    Impl() = default;
    virtual ~Impl() = default;

    // Getter
    virtual double GetEntropy() const = 0;
    virtual double GetZeroProbability(std::uint32_t id) const = 0;
    virtual std::vector<std::uint64_t> Sampling(std::size_t num_samples) const = 0;
    virtual const Data* GetStateVector() const {
        return nullptr;
    };
    virtual const std::complex<double>* GetOperationMatrix() const {
        return nullptr;
    }

    // Gate
    virtual bool Measure(std::uint32_t id) = 0;
    virtual void X(std::uint32_t id) = 0;
    virtual void Y(std::uint32_t id) = 0;
    virtual void Z(std::uint32_t id) = 0;
    virtual void H(std::uint32_t id) = 0;
    virtual void S(std::uint32_t id) = 0;
    virtual void SDag(std::uint32_t id) = 0;
    virtual void T(std::uint32_t id) = 0;
    virtual void TDag(std::uint32_t id) = 0;
    virtual void RX(std::uint32_t id, double theta) = 0;
    virtual void RY(std::uint32_t id, double theta) = 0;
    virtual void RZ(std::uint32_t id, double theta) = 0;
    virtual void CX(std::uint32_t target, std::uint32_t control) = 0;
    virtual void CY(std::uint32_t target, std::uint32_t control) = 0;
    virtual void CZ(std::uint32_t target, std::uint32_t control) = 0;
    virtual void CCX(std::uint32_t target, std::uint32_t c1, std::uint32_t c2) = 0;
    virtual void CCY(std::uint32_t target, std::uint32_t c1, std::uint32_t c2) = 0;
    virtual void CCZ(std::uint32_t target, std::uint32_t c1, std::uint32_t c2) = 0;
    virtual void RotateGlobalPhase(double theta) = 0;
};

#pragma region QRETFullQuantumState
// Eigen::Ref allows treating std::vector, Eigen::Vector, and Matrix columns uniformly.
using StateRef = Eigen::Ref<Eigen::VectorXcd>;
using ConstStateRef = Eigen::Ref<const Eigen::VectorXcd>;

struct FullQuantumState::QRETFullQuantumState : public FullQuantumState::Impl {
    QRETFullQuantumState(std::uint64_t seed, std::uint32_t num_qubits, bool save_operation_matrix)
        : save_operation_matrix_{save_operation_matrix}
        , engine_{seed}
        // Explicit cast to Eigen::Index to match Eigen API and avoid sign-conversion warnings.
        , data_(Eigen::VectorXcd::Zero(static_cast<Eigen::Index>(std::uint64_t{1} << num_qubits)))
        , compute_(
                  Eigen::VectorXcd::Zero(static_cast<Eigen::Index>(std::uint64_t{1} << num_qubits))
          ) {
        // Initial state |0...0> -> Index 0 has probability amplitude 1.0
        data_[0] = Data(1.0, 0.0);

        if (save_operation_matrix_) {
            // Initialize with Identity matrix
            operation_matrix_ = Eigen::MatrixXcd::Identity(data_.size(), data_.size());
        }
    }

    // --- Getters ---

    [[nodiscard]] double GetEntropy() const override;

    [[nodiscard]] double GetZeroProbability(std::uint32_t id) const override;

    [[nodiscard]] std::vector<std::uint64_t> Sampling(std::size_t num_samples) const override;

    [[nodiscard]] const Data* GetStateVector() const override {
        return data_.data();
    }

    // Debug/Validation helper: Returns raw pointer to the operation matrix data
    [[nodiscard]] const std::complex<double>* GetOperationMatrix() const override {
        return save_operation_matrix_ ? operation_matrix_.data() : nullptr;
    }

    // --- Gate Operations ---

    bool Measure(std::uint32_t id) override;
    void X(std::uint32_t id) override;
    void Y(std::uint32_t id) override;
    void Z(std::uint32_t id) override;
    void H(std::uint32_t id) override;
    void S(std::uint32_t id) override;
    void SDag(std::uint32_t id) override;
    void T(std::uint32_t id) override;
    void TDag(std::uint32_t id) override;
    void RX(std::uint32_t id, double theta) override;
    void RY(std::uint32_t id, double theta) override;
    void RZ(std::uint32_t id, double theta) override;
    void CX(std::uint32_t target, std::uint32_t control) override;
    void CY(std::uint32_t target, std::uint32_t control) override;
    void CZ(std::uint32_t target, std::uint32_t control) override;
    void CCX(std::uint32_t target, std::uint32_t c1, std::uint32_t c2) override;
    void CCY(std::uint32_t target, std::uint32_t c1, std::uint32_t c2) override;
    void CCZ(std::uint32_t target, std::uint32_t c1, std::uint32_t c2) override;
    void RotateGlobalPhase(double theta) override;

private:
    // Helper: Checks if the `id`-th bit of `idx` is 1.
    [[nodiscard]] bool Is1(Eigen::Index idx, std::uint32_t id) const {
        return ((static_cast<std::uint64_t>(idx) >> id) & 1) == 1;
    }

    // Helper: Flips the `id`-th bit of `idx`.
    [[nodiscard]] Eigen::Index Flip(Eigen::Index idx, std::uint32_t id) const {
        return static_cast<Eigen::Index>(
                static_cast<std::uint64_t>(idx) ^ (std::uint64_t{1} << id)
        );
    }

    // C++20 Constants
    static constexpr double Sqrt2 = std::numbers::sqrt2;
    static constexpr double InvSqrt2 = 1.0 / std::numbers::sqrt2;

    // Pre-calculated Phase constants
    static inline const Data PhaseT = Data(InvSqrt2, InvSqrt2);
    static inline const Data PhaseTdag = Data(InvSqrt2, -InvSqrt2);
    static inline const Data Imag = Data(0.0, 1.0);

    const bool save_operation_matrix_;
    mutable std::mt19937_64 engine_;

    Eigen::VectorXcd data_;
    Eigen::VectorXcd compute_;  // Compute buffer to avoid allocation during gate application
    Eigen::MatrixXcd operation_matrix_;

    // Helper: Applies a gate using a buffer (for gates that mix basis states like X, Y, H, Rx).
    // The lambda `func` receives (input_vector, output_vector).
    template <typename Func>
    void ApplyGateWithBuffer(Func func) {
        // 1. Apply to the main state vector
        func(data_, compute_);
        data_.swap(compute_);

        // 2. Apply to operation matrix if enabled.
        // To compute U_new = G * U_old, we apply G to every COLUMN of U_old.
        // Each column represents a basis state vector being transformed.
        if (save_operation_matrix_) {
            for (Eigen::Index j = 0; j < operation_matrix_.cols(); ++j) {
                // operation_matrix_.col(j) behaves like a vector
                func(operation_matrix_.col(j), compute_);
                operation_matrix_.col(j) = compute_;  // Copy result back
            }
        }
    }

    // Helper: Applies a gate in-place (for diagonal gates like Z, S, T, Rz, CZ).
    // The lambda `func` receives (target_vector_ref).
    template <typename Func>
    void ApplyGateInPlace(Func func) {
        // 1. Apply to state vector
        func(data_);

        // 2. Apply to operation matrix (column-wise)
        if (save_operation_matrix_) {
            for (Eigen::Index j = 0; j < operation_matrix_.cols(); ++j) {
                func(operation_matrix_.col(j));
            }
        }
    }
};

double FullQuantumState::QRETFullQuantumState::GetEntropy() const {
    double entropy = 0.0;
    for (Eigen::Index i = 0; i < data_.size(); ++i) {
        const double prob = std::norm(data_[i]);
        if (prob > 1e-12) {  // Use a small epsilon for stability
            entropy -= prob * std::log(prob);
        }
    }
    return entropy;
}

double FullQuantumState::QRETFullQuantumState::GetZeroProbability(std::uint32_t id) const {
    double sum0 = 0.0;
    for (Eigen::Index idx = 0; idx < data_.size(); ++idx) {
        if (!Is1(idx, id)) {
            sum0 += std::norm(data_[idx]);
        }
    }
    return sum0;
}

std::vector<std::uint64_t> FullQuantumState::QRETFullQuantumState::Sampling(
        std::size_t num_samples
) const {
    std::uniform_real_distribution<> dist(0.0, 1.0);

    // Create Cumulative Distribution Function (CDF)
    std::vector<double> sum(static_cast<std::size_t>(data_.size()));
    double current_sum = 0.0;
    for (Eigen::Index idx = 0; idx < data_.size(); ++idx) {
        current_sum += std::norm(data_[idx]);
        sum[static_cast<std::size_t>(idx)] = current_sum;
    }

    // Binary search for sampling
    const auto sample = [&sum](double x) -> std::uint64_t {
        auto it = std::lower_bound(sum.begin(), sum.end(), x);
        if (it != sum.end()) {
            return static_cast<std::uint64_t>(std::distance(sum.begin(), it));
        }
        return static_cast<std::uint64_t>(sum.size() - 1);
    };

    std::vector<std::uint64_t> ret(num_samples);
    for (std::size_t i = 0; i < num_samples; ++i) {
        ret[i] = sample(dist(engine_));
    }
    return ret;
}

bool FullQuantumState::QRETFullQuantumState::Measure(std::uint32_t id) {
    std::uniform_real_distribution<> dist(0.0, 1.0);
    double sum0 = 0.0;
    double sum1 = 0.0;

    // Calculate probabilities for 0 and 1
    for (Eigen::Index idx = 0; idx < data_.size(); ++idx) {
        if (Is1(idx, id)) {
            sum1 += std::norm(data_[idx]);
        } else {
            sum0 += std::norm(data_[idx]);
        }
    }

    // Determine measurement result
    const bool result_one = [&](double sum0, double sum1) {
        if (sum0 < 1e-10) {
            return true;
        } else if (sum1 < 1e-10) {
            return false;
        }

        return (dist(engine_) > sum0);
    }(sum0, sum1);
    const double normalize = 1.0 / std::sqrt(result_one ? sum1 : sum0);

    // Update State Vector (Project and Normalize)
    for (Eigen::Index idx = 0; idx < data_.size(); ++idx) {
        if (Is1(idx, id) == result_one) {
            data_[idx] *= normalize;
        } else {
            data_[idx] = 0.0;
        }
    }

    // Update Operation Matrix (Project and Normalize)
    // Measurement is a projection operator P. M = P * U.
    // We apply P to each column of U. P zeroes out rows that don't match the result.
    if (save_operation_matrix_) {
        for (Eigen::Index j = 0; j < operation_matrix_.cols(); ++j) {
            for (Eigen::Index i = 0; i < operation_matrix_.rows(); ++i) {
                if (Is1(i, id) == result_one) {
                    operation_matrix_(i, j) *= normalize;
                } else {
                    operation_matrix_(i, j) = 0.0;
                }
            }
        }
    }
    return result_one;
}

void FullQuantumState::QRETFullQuantumState::X(std::uint32_t id) {
    ApplyGateWithBuffer([&](ConstStateRef in, StateRef out) {
        for (Eigen::Index idx = 0; idx < in.size(); ++idx) {
            out[Flip(idx, id)] = in[idx];
        }
    });
}

void FullQuantumState::QRETFullQuantumState::Y(std::uint32_t id) {
    ApplyGateWithBuffer([&](ConstStateRef in, StateRef out) {
        for (Eigen::Index idx = 0; idx < in.size(); ++idx) {
            if (Is1(idx, id)) {
                out[Flip(idx, id)] = -Imag * in[idx];
            } else {
                out[Flip(idx, id)] = Imag * in[idx];
            }
        }
    });
}

void FullQuantumState::QRETFullQuantumState::Z(std::uint32_t id) {
    ApplyGateInPlace([&](StateRef state) {
        for (Eigen::Index idx = 0; idx < state.size(); ++idx) {
            if (Is1(idx, id)) {
                state[idx] *= -1.0;
            }
        }
    });
}

void FullQuantumState::QRETFullQuantumState::H(std::uint32_t id) {
    ApplyGateWithBuffer([&](ConstStateRef in, StateRef out) {
        out.setZero();  // Must clear buffer as we accumulate values
        for (Eigen::Index idx = 0; idx < in.size(); ++idx) {
            Data val = in[idx] * InvSqrt2;
            if (Is1(idx, id)) {
                // H|1> = 1/sqrt2 (|0> - |1>)
                out[idx] -= val;  // Component for |1> (flipped from 0) gets -
                out[Flip(idx, id)] += val;  // Component for |0> (flipped from 1) gets +
            } else {
                // H|0> = 1/sqrt2 (|0> + |1>)
                out[idx] += val;
                out[Flip(idx, id)] += val;
            }
        }
    });
}

void FullQuantumState::QRETFullQuantumState::S(std::uint32_t id) {
    ApplyGateInPlace([&](StateRef state) {
        for (Eigen::Index idx = 0; idx < state.size(); ++idx) {
            if (Is1(idx, id)) {
                state[idx] *= Imag;
            }
        }
    });
}

void FullQuantumState::QRETFullQuantumState::SDag(std::uint32_t id) {
    ApplyGateInPlace([&](StateRef state) {
        for (Eigen::Index idx = 0; idx < state.size(); ++idx) {
            if (Is1(idx, id)) {
                state[idx] *= -Imag;
            }
        }
    });
}

void FullQuantumState::QRETFullQuantumState::T(std::uint32_t id) {
    ApplyGateInPlace([&](StateRef state) {
        for (Eigen::Index idx = 0; idx < state.size(); ++idx) {
            if (Is1(idx, id)) {
                state[idx] *= PhaseT;
            }
        }
    });
}

void FullQuantumState::QRETFullQuantumState::TDag(std::uint32_t id) {
    ApplyGateInPlace([&](StateRef state) {
        for (Eigen::Index idx = 0; idx < state.size(); ++idx) {
            if (Is1(idx, id)) {
                state[idx] *= PhaseTdag;
            }
        }
    });
}

void FullQuantumState::QRETFullQuantumState::RX(std::uint32_t id, double theta) {
    const double s = std::sin(theta / 2.0);
    const double c = std::cos(theta / 2.0);
    const Data term1(c, 0);
    const Data term2(0, s);  // 0 + i*sin

    ApplyGateWithBuffer([&](ConstStateRef in, StateRef out) {
        out.setZero();
        for (Eigen::Index idx = 0; idx < in.size(); ++idx) {
            // Rx = [cos  -isin]
            //      [-isin  cos]
            out[idx] += term1 * in[idx];
            out[Flip(idx, id)] -= term2 * in[idx];
        }
    });
}

void FullQuantumState::QRETFullQuantumState::RY(std::uint32_t id, double theta) {
    const double s = std::sin(theta / 2.0);
    const double c = std::cos(theta / 2.0);

    ApplyGateWithBuffer([&](ConstStateRef in, StateRef out) {
        out.setZero();
        for (Eigen::Index idx = 0; idx < in.size(); ++idx) {
            // Ry = [cos  -sin]
            //      [sin   cos]
            if (Is1(idx, id)) {
                // If current bit is 1: contribute to 1 (cos) and 0 (-sin)
                out[idx] += c * in[idx];
                out[Flip(idx, id)] -= s * in[idx];
            } else {
                // If current bit is 0: contribute to 0 (cos) and 1 (sin)
                out[idx] += c * in[idx];
                out[Flip(idx, id)] += s * in[idx];
            }
        }
    });
}

void FullQuantumState::QRETFullQuantumState::RZ(std::uint32_t id, double theta) {
    const double s = std::sin(theta / 2.0);
    const double c = std::cos(theta / 2.0);
    const Data phase0(c, -s);  // e^{-i theta/2}
    const Data phase1(c, s);  // e^{i theta/2}

    ApplyGateInPlace([&](StateRef state) {
        for (Eigen::Index idx = 0; idx < state.size(); ++idx) {
            if (Is1(idx, id)) {
                state[idx] *= phase1;
            } else {
                state[idx] *= phase0;
            }
        }
    });
}

void FullQuantumState::QRETFullQuantumState::CX(std::uint32_t target, std::uint32_t control) {
    ApplyGateWithBuffer([&](ConstStateRef in, StateRef out) {
        for (Eigen::Index idx = 0; idx < in.size(); ++idx) {
            if (Is1(idx, control)) {
                // If control is 1, swap target bit
                out[Flip(idx, target)] = in[idx];
            } else {
                out[idx] = in[idx];
            }
        }
    });
}

void FullQuantumState::QRETFullQuantumState::CY(std::uint32_t target, std::uint32_t control) {
    // CY = S_dag * CX * S
    SDag(target);
    CX(target, control);
    S(target);
}

void FullQuantumState::QRETFullQuantumState::CZ(std::uint32_t target, std::uint32_t control) {
    ApplyGateInPlace([&](StateRef state) {
        for (Eigen::Index idx = 0; idx < state.size(); ++idx) {
            if (Is1(idx, control) && Is1(idx, target)) {
                state[idx] *= -1.0;
            }
        }
    });
}

void FullQuantumState::QRETFullQuantumState::CCX(
        std::uint32_t target,
        std::uint32_t c1,
        std::uint32_t c2
) {
    // Composite Toffoli decomposition
    H(target);
    CX(target, c2);
    TDag(target);
    CX(target, c1);
    T(target);
    CX(target, c2);
    TDag(target);
    CX(target, c1);
    T(target);
    T(c2);
    CX(c2, c1);
    H(target);
    T(c1);
    TDag(c2);
    CX(c2, c1);
}

void FullQuantumState::QRETFullQuantumState::CCY(
        std::uint32_t target,
        std::uint32_t c1,
        std::uint32_t c2
) {
    // Y = S X SDag
    SDag(target);
    CCX(target, c1, c2);
    S(target);
}

void FullQuantumState::QRETFullQuantumState::CCZ(
        std::uint32_t target,
        std::uint32_t c1,
        std::uint32_t c2
) {
    // Z = H X H
    H(target);
    CCX(target, c1, c2);
    H(target);
}

void FullQuantumState::QRETFullQuantumState::RotateGlobalPhase(double theta) {
    const Data e = std::polar(1.0, theta);
    ApplyGateInPlace([&](StateRef state) { state *= e; });
}
#pragma endregion

#ifdef USE_QULACS
#pragma region QulacsFullQuantumState
struct FullQuantumState::QulacsFullQuantumState : public FullQuantumState::Impl {
    QulacsFullQuantumState(std::uint64_t seed, std::uint32_t num_qubits)
        : engine{static_cast<std::uint_fast32_t>(seed)}
        ,
        // Allocate num_qubits + `1` to run RotateGlobalPhase
        state(std::unique_ptr<QuantumStateCpu>(new QuantumStateCpu(num_qubits + 1, true))) {
        state->set_zero_state();
    }

    // Getter
    double GetEntropy() const override {
        return state->get_entropy();
    }
    double GetZeroProbability(std::uint32_t id) const override {
        return state->get_zero_probability(id);
    }
    std::vector<std::uint64_t> Sampling(std::size_t num_samples) const override {
        auto raw = state->sampling(static_cast<std::uint32_t>(num_samples));
        auto ret = std::vector<std::uint64_t>();
        ret.reserve(raw.size());
        for (const auto& x : raw) {
            ret.emplace_back(x);
        }
        return ret;
    }
    const Data* GetStateVector() const override {
        return state->data_cpp();
    }

    // Gate
    bool Measure(std::uint32_t id) override {
        auto gate = std::unique_ptr<QuantumGateBase>(gate::Measurement(id, 0));
        gate->set_seed(static_cast<std::int32_t>(engine()));
        gate->update_quantum_state(state.get());
        const auto result = state->get_classical_value(0);
        return result == UINT{1};
    }
    void X(std::uint32_t id) override {
        std::unique_ptr<QuantumGateBase>(gate::X(id))->update_quantum_state(state.get());
    }
    void Y(std::uint32_t id) override {
        std::unique_ptr<QuantumGateBase>(gate::Y(id))->update_quantum_state(state.get());
    }
    void Z(std::uint32_t id) override {
        std::unique_ptr<QuantumGateBase>(gate::Z(id))->update_quantum_state(state.get());
    }
    void H(std::uint32_t id) override {
        std::unique_ptr<QuantumGateBase>(gate::H(id))->update_quantum_state(state.get());
    }
    void S(std::uint32_t id) override {
        std::unique_ptr<QuantumGateBase>(gate::S(id))->update_quantum_state(state.get());
    }
    void SDag(std::uint32_t id) override {
        std::unique_ptr<QuantumGateBase>(gate::Sdag(id))->update_quantum_state(state.get());
    }
    void T(std::uint32_t id) override {
        std::unique_ptr<QuantumGateBase>(gate::T(id))->update_quantum_state(state.get());
    }
    void TDag(std::uint32_t id) override {
        std::unique_ptr<QuantumGateBase>(gate::Tdag(id))->update_quantum_state(state.get());
    }
    void RX(std::uint32_t id, double theta) override {
        std::unique_ptr<QuantumGateBase>(gate::RX(id, -theta))->update_quantum_state(state.get());
    }
    void RY(std::uint32_t id, double theta) override {
        std::unique_ptr<QuantumGateBase>(gate::RY(id, -theta))->update_quantum_state(state.get());
    }
    void RZ(std::uint32_t id, double theta) override {
        std::unique_ptr<QuantumGateBase>(gate::RZ(id, -theta))->update_quantum_state(state.get());
    }
    void CX(std::uint32_t target, std::uint32_t control) override {
        std::unique_ptr<QuantumGateBase>(gate::CNOT(control, target))
                ->update_quantum_state(state.get());
    }
    void CY(std::uint32_t target, std::uint32_t control) override {
        std::unique_ptr<QuantumGateBase>(gate::Sdag(target))->update_quantum_state(state.get());
        std::unique_ptr<QuantumGateBase>(gate::CNOT(control, target))
                ->update_quantum_state(state.get());
        std::unique_ptr<QuantumGateBase>(gate::S(target))->update_quantum_state(state.get());
    }
    void CZ(std::uint32_t target, std::uint32_t control) override {
        std::unique_ptr<QuantumGateBase>(gate::CZ(control, target))
                ->update_quantum_state(state.get());
    }
    void CCX(std::uint32_t target, std::uint32_t c1, std::uint32_t c2) override {
        std::unique_ptr<QuantumGateBase>(gate::H(target))->update_quantum_state(state.get());
        std::unique_ptr<QuantumGateBase>(gate::CNOT(c2, target))->update_quantum_state(state.get());
        std::unique_ptr<QuantumGateBase>(gate::Tdag(target))->update_quantum_state(state.get());
        std::unique_ptr<QuantumGateBase>(gate::CNOT(c1, target))->update_quantum_state(state.get());
        std::unique_ptr<QuantumGateBase>(gate::T(target))->update_quantum_state(state.get());
        std::unique_ptr<QuantumGateBase>(gate::CNOT(c2, target))->update_quantum_state(state.get());
        std::unique_ptr<QuantumGateBase>(gate::Tdag(target))->update_quantum_state(state.get());
        std::unique_ptr<QuantumGateBase>(gate::CNOT(c1, target))->update_quantum_state(state.get());
        std::unique_ptr<QuantumGateBase>(gate::T(target))->update_quantum_state(state.get());
        std::unique_ptr<QuantumGateBase>(gate::T(c2))->update_quantum_state(state.get());
        std::unique_ptr<QuantumGateBase>(gate::CNOT(c1, c2))->update_quantum_state(state.get());
        std::unique_ptr<QuantumGateBase>(gate::H(target))->update_quantum_state(state.get());
        std::unique_ptr<QuantumGateBase>(gate::T(c1))->update_quantum_state(state.get());
        std::unique_ptr<QuantumGateBase>(gate::Tdag(c2))->update_quantum_state(state.get());
        std::unique_ptr<QuantumGateBase>(gate::CNOT(c1, c2))->update_quantum_state(state.get());
    }
    void CCY(std::uint32_t target, std::uint32_t c1, std::uint32_t c2) override {
        // Y = S X SDag
        SDag(target);
        CCX(target, c1, c2);
        S(target);
    }
    void CCZ(std::uint32_t target, std::uint32_t c1, std::uint32_t c2) override {
        // Z = H X H
        H(target);
        CCX(target, c1, c2);
        H(target);
    }
    void RotateGlobalPhase(double theta) override {
        const auto num_qubits = state->qubit_count - 1;
        std::unique_ptr<QuantumGateBase>(gate::X(num_qubits))->update_quantum_state(state.get());
        std::unique_ptr<QuantumGateBase>(gate::U1(num_qubits, theta))
                ->update_quantum_state(state.get());
        std::unique_ptr<QuantumGateBase>(gate::X(num_qubits))->update_quantum_state(state.get());
    }

    std::mt19937 engine;
    std::unique_ptr<QuantumStateCpu> state;
};
#pragma endregion
#endif
}  // namespace qret::runtime

namespace qret::runtime {
FullQuantumState::FullQuantumState(
        std::uint64_t seed,
        std::uint64_t num_qubits,
        bool use_qulacs,
        bool save_operation_matrix
)
    : QuantumState(seed)
    , num_qubits_{num_qubits} {
    if (use_qulacs) {
        if (save_operation_matrix) {
            LOG_WARN("Currently QulacsFullQuantumState does not support 'save_operation_matrix'.");
        }

#ifdef USE_QULACS
        LOG_INFO("Use qulacs as the implementation of FullQuantumState");
        impl_ = std::unique_ptr<QulacsFullQuantumState>(
                new QulacsFullQuantumState(seed, static_cast<std::uint32_t>(num_qubits))
        );
        return;
#else
        LOG_WARN(
                "qulacs is not available. qret uses the internal full quantum state class as an "
                "alternative, which is slow. To use qulacs, you need to install it first and then "
                "rebuild qret with the QRET_USE_QULACS option set to true in cmake."
        );
#endif
    }

    LOG_INFO("Use qret naive FullQuantumState, which is slow");
    impl_ = std::unique_ptr<QRETFullQuantumState>(new QRETFullQuantumState(
            seed,
            static_cast<std::uint32_t>(num_qubits),
            save_operation_matrix
    ));
}
FullQuantumState::~FullQuantumState() = default;
void FullQuantumState::Measure(std::uint64_t q, std::uint64_t r) {
    const auto q32 = static_cast<std::uint32_t>(q);
    const auto result = impl_->Measure(q32);
    QuantumState::SaveMeasuredResult(r, result);
}
void FullQuantumState::X(std::uint64_t q) {
    const auto q32 = static_cast<std::uint32_t>(q);
    impl_->X(q32);
}
void FullQuantumState::Y(std::uint64_t q) {
    const auto q32 = static_cast<std::uint32_t>(q);
    impl_->Y(q32);
}
void FullQuantumState::Z(std::uint64_t q) {
    const auto q32 = static_cast<std::uint32_t>(q);
    impl_->Z(q32);
}
void FullQuantumState::H(std::uint64_t q) {
    const auto q32 = static_cast<std::uint32_t>(q);
    impl_->H(q32);
}
void FullQuantumState::S(std::uint64_t q) {
    const auto q32 = static_cast<std::uint32_t>(q);
    impl_->S(q32);
}
void FullQuantumState::SDag(std::uint64_t q) {
    const auto q32 = static_cast<std::uint32_t>(q);
    impl_->SDag(q32);
}
void FullQuantumState::T(std::uint64_t q) {
    const auto q32 = static_cast<std::uint32_t>(q);
    impl_->T(q32);
}
void FullQuantumState::TDag(std::uint64_t q) {
    const auto q32 = static_cast<std::uint32_t>(q);
    impl_->TDag(q32);
}
void FullQuantumState::RX(std::uint64_t q, double theta) {
    const auto q32 = static_cast<std::uint32_t>(q);
    impl_->RX(q32, theta);
}
void FullQuantumState::RY(std::uint64_t q, double theta) {
    const auto q32 = static_cast<std::uint32_t>(q);
    impl_->RY(q32, theta);
}
void FullQuantumState::RZ(std::uint64_t q, double theta) {
    const auto q32 = static_cast<std::uint32_t>(q);
    impl_->RZ(q32, theta);
}
void FullQuantumState::CX(std::uint64_t t, std::uint64_t c) {
    const auto t32 = static_cast<std::uint32_t>(t);
    const auto c32 = static_cast<std::uint32_t>(c);
    impl_->CX(t32, c32);
}
void FullQuantumState::CY(std::uint64_t t, std::uint64_t c) {
    const auto t32 = static_cast<std::uint32_t>(t);
    const auto c32 = static_cast<std::uint32_t>(c);
    impl_->CY(t32, c32);
}
void FullQuantumState::CZ(std::uint64_t t, std::uint64_t c) {
    const auto t32 = static_cast<std::uint32_t>(t);
    const auto c32 = static_cast<std::uint32_t>(c);
    impl_->CZ(t32, c32);
}
void FullQuantumState::CCX(std::uint64_t t, std::uint64_t c0, std::uint64_t c1) {
    const auto t32 = static_cast<std::uint32_t>(t);
    const auto c032 = static_cast<std::uint32_t>(c0);
    const auto c132 = static_cast<std::uint32_t>(c1);
    impl_->CCX(t32, c032, c132);
}
void FullQuantumState::CCY(std::uint64_t t, std::uint64_t c0, std::uint64_t c1) {
    const auto t32 = static_cast<std::uint32_t>(t);
    const auto c032 = static_cast<std::uint32_t>(c0);
    const auto c132 = static_cast<std::uint32_t>(c1);
    impl_->CCY(t32, c032, c132);
}
void FullQuantumState::CCZ(std::uint64_t t, std::uint64_t c0, std::uint64_t c1) {
    const auto t32 = static_cast<std::uint32_t>(t);
    const auto c032 = static_cast<std::uint32_t>(c0);
    const auto c132 = static_cast<std::uint32_t>(c1);
    impl_->CCZ(t32, c032, c132);
}
void FullQuantumState::RotateGlobalPhase(double theta) {
    impl_->RotateGlobalPhase(theta);
}
bool FullQuantumState::IsClean(std::uint64_t q) const {
    return Calc1Prob(q) < QuantumState::Eps;
}
std::uint64_t FullQuantumState::GetNumSuperpositions() const {
    auto num_superpositions = std::uint64_t{0};
    const auto* begin = impl_->GetStateVector();
    const auto* end = begin + (std::int64_t{1} << num_qubits_);
    for (const auto* ptr = begin; ptr != end; ++ptr) {
        if (std::norm(*ptr) > QuantumState::Eps) {
            num_superpositions++;
        }
    }
    return num_superpositions;
}
double FullQuantumState::Calc1Prob(std::uint64_t q) const {
    const auto q32 = static_cast<std::uint32_t>(q);
    return 1 - impl_->GetZeroProbability(q32);
}
double FullQuantumState::GetEntropy() const {
    return impl_->GetEntropy();
}
std::vector<std::uint64_t> FullQuantumState::Sampling(std::size_t num_samples) const {
    const auto& sampling = impl_->Sampling(num_samples);
    return {sampling.begin(), sampling.end()};
}
const std::complex<double>* FullQuantumState::GetStateVector() const {
    return impl_->GetStateVector();
}
const std::complex<double>* FullQuantumState::GetOperationMatrix() const {
    return impl_->GetOperationMatrix();
}
}  // namespace qret::runtime
