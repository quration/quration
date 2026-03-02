/**
 * @file qret/runtime/toffoli_state.cpp
 * @brief Quantum state: toffoli.
 */

#include "qret/runtime/toffoli_state.h"

#include <algorithm>
#include <cassert>
#include <numbers>
#include <stdexcept>

#include "qret/base/log.h"
#include "qret/base/type.h"
#include "qret/runtime/quantum_state.h"

namespace qret::runtime {
namespace constant {
static inline const auto Sqrt = std::complex<double>(std::numbers::sqrt2);
static inline const auto Imag = std::complex<double>(0.0, 1.0);
static inline const auto Omega = std::complex(1.0 / std::numbers::sqrt2, 1.0 / std::numbers::sqrt2);
}  // namespace constant
namespace {
void XImpl(std::uint64_t q, ToffoliState::Coefficient&, ToffoliState::State& state) {
    state[q] = 1 - state[q];
}
void YImpl(std::uint64_t q, ToffoliState::Coefficient& coef, ToffoliState::State& state) {
    if (state[q] == 0) {
        coef *= constant::Imag;
        state[q] = 1;
    } else {
        coef *= -constant::Imag;
        state[q] = 0;
    }
}
void ZImpl(std::uint64_t q, ToffoliState::Coefficient& coef, ToffoliState::State& state) {
    if (state[q] == 1) {
        coef *= -1.0;
    }
}
}  // namespace
void ToffoliState::Measure(std::uint64_t q, std::uint64_t r) {
    auto zero_sum = 0.0;
    auto one_sum = 0.0;
    for (const auto& [coef, state] : states_) {
        if (state[q] == 0) {
            zero_sum += std::norm(coef);
        } else {
            one_sum += std::norm(coef);
        }
    }

    if (std::abs(zero_sum - 1.0) < eps_) {
        QuantumState::SaveMeasuredResult(r, false);
        return;
    } else if (std::abs(one_sum - 1.0) < eps_) {
        QuantumState::SaveMeasuredResult(r, true);
        return;
    }

    if (std::abs(zero_sum) >= eps_) {
        LOG_DEBUG("zero_sum must be a real number.");
    }
    if (std::abs(one_sum) >= eps_) {
        LOG_DEBUG("one_sum must be a real number.");
    }
    if (std::abs(1.0 - (zero_sum + one_sum)) >= eps_) {
        LOG_DEBUG("invalid quantum state: norm sum of |0> and |1> coefficients is not 1.");
    }

    // Use random to get measurement result
    const auto measured_one = QuantumState::Flip(one_sum);

    if (measured_one) {
        // Measured |1>
        QuantumState::SaveMeasuredResult(r, true);
    } else {
        // Measured |0>
        QuantumState::SaveMeasuredResult(r, false);
    }

    // If result is true, project to |1>; otherwise, project to |0>
    auto num = static_cast<std::int64_t>(GetNumSuperpositions());
    for (auto i = std::int64_t{0}; i < num; ++i) {
        auto& [coef, state] = states_[static_cast<std::size_t>(i)];
        if (measured_one && state[q] == 1) {
            coef /= std::sqrt(one_sum);
        } else if (!measured_one && state[q] == 0) {
            coef /= std::sqrt(zero_sum);
        } else {
            states_.erase(states_.begin() + i);
            i--;
            num--;
        }
    }
}
void ToffoliState::X(std::uint64_t q) {
    for (auto&& [coef, state] : states_) {
        XImpl(q, coef, state);
    }
}
void ToffoliState::Y(std::uint64_t q) {
    for (auto&& [coef, state] : states_) {
        YImpl(q, coef, state);
    }
}
void ToffoliState::Z(std::uint64_t q) {
    for (auto&& [coef, state] : states_) {
        ZImpl(q, coef, state);
    }
}
void ToffoliState::H(std::uint64_t q) {
    const auto num = states_.size();
    states_.reserve(2 * num);
    for (auto i = std::size_t{0}; i < num; ++i) {
        states_.emplace_back(states_[i]);
    }
    for (auto i = std::size_t{0}; i < num; ++i) {
        auto& [coef, state] = states_[i];
        auto& [cp_coef, cp_state] = states_[i + num];
        if (state[q] == 0) {
            cp_state[q] = 1;
            coef /= constant::Sqrt;  // 0 --> 0
            cp_coef /= constant::Sqrt;  // 0 --> 1
        } else {
            cp_state[q] = 0;
            coef /= -constant::Sqrt;  // 1 --> 1
            cp_coef /= constant::Sqrt;  // 1 --> 0
        }
    }

    Reduce();
}
void ToffoliState::S(std::uint64_t q) {
    for (auto&& [coef, state] : states_) {
        if (state[q] == 1) {
            coef *= constant::Imag;
        }
    }
}
void ToffoliState::SDag(std::uint64_t q) {
    for (auto&& [coef, state] : states_) {
        if (state[q] == 1) {
            coef *= std::conj(constant::Imag);
        }
    }
}
void ToffoliState::T(std::uint64_t q) {
    for (auto&& [coef, state] : states_) {
        if (state[q] == 1) {
            coef *= constant::Omega;
        }
    }
}
void ToffoliState::TDag(std::uint64_t q) {
    for (auto&& [coef, state] : states_) {
        if (state[q] == 1) {
            coef *= -std::conj(constant::Omega);
        }
    }
}
void ToffoliState::CX(std::uint64_t t, std::uint64_t c) {
    for (auto&& [coef, state] : states_) {
        if (state[c] == 1) {
            XImpl(t, coef, state);
        }
    }
}
void ToffoliState::CY(std::uint64_t t, std::uint64_t c) {
    for (auto&& [coef, state] : states_) {
        if (state[c] == 1) {
            YImpl(t, coef, state);
        }
    }
}
void ToffoliState::CZ(std::uint64_t t, std::uint64_t c) {
    for (auto&& [coef, state] : states_) {
        if (state[c] == 1) {
            ZImpl(t, coef, state);
        }
    }
}
void ToffoliState::CCX(std::uint64_t t, std::uint64_t c0, std::uint64_t c1) {
    for (auto&& [coef, state] : states_) {
        if (state[c0] == 1 && state[c1] == 1) {
            XImpl(t, coef, state);
        }
    }
}
void ToffoliState::CCY(std::uint64_t t, std::uint64_t c0, std::uint64_t c1) {
    for (auto&& [coef, state] : states_) {
        if (state[c0] == 1 && state[c1] == 1) {
            YImpl(t, coef, state);
        }
    }
}
void ToffoliState::CCZ(std::uint64_t t, std::uint64_t c0, std::uint64_t c1) {
    for (auto&& [coef, state] : states_) {
        if (state[c0] == 1 && state[c1] == 1) {
            ZImpl(t, coef, state);
        }
    }
}
void ToffoliState::MCX(std::uint64_t t, const std::vector<std::uint64_t>& c) {
    for (auto&& [coef, state] : states_) {
        if (std::all_of(c.begin(), c.end(), [&state](const auto q) { return state[q] == 1; })) {
            XImpl(t, coef, state);
        }
    }
}
bool ToffoliState::IsClean(std::uint64_t q) const {
    for (const auto& [_, state] : states_) {
        if (state[q] != 0) {
            return false;
        }
    }
    return true;
}
double ToffoliState::Calc1Prob(std::uint64_t q) const {
    auto sum = 0.0;
    for (const auto& [coef, state] : states_) {
        if (state[q] == 1) {
            sum += std::norm(coef);
        }
    }
    return sum;
}
BigInt ToffoliState::GetValue(std::uint64_t start, std::uint64_t size) const {
    if (!IsComputationalBasis()) {
        throw std::runtime_error("not computational basis");
    }
    auto ret = BigInt(0);
    for (auto i = std::uint64_t{0}; i < size; ++i) {
        ret += states_.front().second[start + i] * (BigInt{1} << i);
    }
    return ret;
}
void ToffoliState::Reduce() {
    const auto eq_state = [](const State& lhs, const State& rhs) -> bool {
        if (lhs.size() != rhs.size()) {
            return false;
        }
        for (auto i = std::size_t{0}; i < lhs.size(); ++i) {
            if (lhs[i] != rhs[i]) {
                return false;
            }
        }
        return true;
    };

    auto num = static_cast<std::int64_t>(GetNumSuperpositions());
    if (num == 1) {
        return;
    }

    // Reduces states in the same computational basis
    for (auto i = std::int64_t{0}; i < num; ++i) {
        for (auto j = std::int64_t{0}; j < i; ++j) {
            auto& [i_coef, i_state] = states_[static_cast<std::size_t>(i)];
            auto& [j_coef, j_state] = states_[static_cast<std::size_t>(j)];
            if (eq_state(i_state, j_state)) {
                // Delete i
                j_coef += i_coef;
                states_.erase(states_.begin() + i);
                num--;
                i--;
                break;
            }
        }
    }

    // Delete coef 0 states
    for (auto i = std::int64_t{0}; i < num; ++i) {
        const auto& [coef, _] = states_[static_cast<std::size_t>(i)];
        if (std::abs(coef) < eps_) {
            states_.erase(states_.begin() + i);
            i--;
            num--;
        }
    }
}
}  // namespace qret::runtime
