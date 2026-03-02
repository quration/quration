/**
 * @file qret/runtime/quantum_state.h
 * @brief Base quantum state.
 */

#ifndef QRET_RUNTIME_QUANTUM_STATE_H
#define QRET_RUNTIME_QUANTUM_STATE_H

#include <fmt/format.h>

#include <cstdint>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "qret/base/type.h"

namespace qret::runtime {
/**
 * @brief Quantum state class.
 * @details If you want to implement some specific quantum state, inherit this class (e.g.,
 * ToffoliState, FullQuantumState).
 * The Simulator class simulates quantum circuit by manipulating this QuantumState class.
 */
class QuantumState {
public:
    static constexpr auto Eps = 1e-9;

    // FIXME: the way to set seed (it must depends on the specific implementation of quantum state)
    explicit QuantumState(std::uint64_t seed)
        : dist_{std::uniform_real_distribution<>(0, 1)}
        , engine_{seed} {}
    virtual ~QuantumState() = default;

    // Set name&size of qubit and register
    void AddQubit(std::string_view name, std::size_t size) {
        qubits_.emplace_back(std::string(name), size);
    }
    void AddRegister(std::string_view name, std::size_t size) {
        registers_.emplace_back(std::string(name), size);
    }

    // Gate
    virtual void Measure(std::uint64_t q, std::uint64_t r) = 0;
    virtual void X(std::uint64_t q) = 0;
    virtual void Y(std::uint64_t q) = 0;
    virtual void Z(std::uint64_t q) = 0;
    virtual void H(std::uint64_t q) = 0;
    virtual void S(std::uint64_t q) = 0;
    virtual void SDag(std::uint64_t q) = 0;
    virtual void T(std::uint64_t q) = 0;
    virtual void TDag(std::uint64_t q) = 0;
    virtual void RX(std::uint64_t q, double theta) = 0;
    virtual void RY(std::uint64_t q, double theta) = 0;
    virtual void RZ(std::uint64_t q, double theta) = 0;
    virtual void CX(std::uint64_t t, std::uint64_t c) = 0;
    virtual void CY(std::uint64_t t, std::uint64_t c) = 0;
    virtual void CZ(std::uint64_t t, std::uint64_t c) = 0;
    virtual void CCX(std::uint64_t t, std::uint64_t c0, std::uint64_t c1) = 0;
    virtual void CCY(std::uint64_t t, std::uint64_t c0, std::uint64_t c1) = 0;
    virtual void CCZ(std::uint64_t t, std::uint64_t c0, std::uint64_t c1) = 0;
    virtual void MCX(std::uint64_t id, const std::vector<std::uint64_t>& c) = 0;
    virtual void RotateGlobalPhase(double theta) = 0;

    // Check optimization hint is valid
    virtual bool IsClean(std::uint64_t q) const = 0;

    // Register
    void WriteRegister(std::uint64_t r, bool value) {
        register_map_[r] = value;
    }
    void WriteRegisters(std::uint64_t start, std::uint64_t size, const BigInt& value) {
        for (auto r = start; r < start + size; ++r) {
            register_map_[r] = ((value >> (r - start)) & 1) == 1;
        }
    }
    bool ReadRegister(std::uint64_t r) const {
        const auto itr = register_map_.find(r);
        return itr == register_map_.end() ? false : itr->second;
    }
    std::vector<bool> ReadRegisters(std::uint64_t start, std::uint64_t size) const {
        auto ret = std::vector<bool>();
        ret.reserve(size);
        for (auto i = std::uint64_t{0}; i < size; ++i) {
            ret.emplace_back(ReadRegister(start + i));
        }
        return ret;
    }
    std::vector<bool> ReadRegistersByName(std::string_view name) const {
        bool find_name = false;
        auto start = std::size_t{0};
        auto size = std::size_t{0};
        for (const auto& [reg_name, reg_size] : registers_) {
            if (name == reg_name) {
                find_name = true;
                size = reg_size;
                break;
            } else {
                start += reg_size;
            }
        }
        if (!find_name) {
            throw std::runtime_error(fmt::format("Unknown register name: {}", name));
        }
        return ReadRegisters(start, size);
    }

    // Read quantum state
    virtual std::uint64_t GetNumSuperpositions() const = 0;
    double Calc0Prob(std::uint64_t q) const {
        return 1 - Calc1Prob(q);
    }
    virtual double Calc1Prob(std::uint64_t q) const = 0;
    bool IsSuperposed(std::uint64_t q) const {
        const auto prob = Calc1Prob(q);
        return Eps < prob && prob < 1.0 - Eps;
    }

    std::mt19937_64& GetMutEngine() {
        return engine_;
    }

protected:
    /**
     * @brief Flip coin and get 1 with probability `prob`.
     * @details Because of using `engine_` internally, this method is not const.
     */
    bool Flip(double prob) {
        return dist_(engine_) <= prob;
    }
    void SaveMeasuredResult(std::uint64_t r, bool result) {
        register_map_[r] = result;
    }

private:
    std::uniform_real_distribution<> dist_;
    std::mt19937_64 engine_;
    std::unordered_map<std::uint64_t, bool> register_map_;

    // name of qubit and register
    std::vector<std::pair<std::string, std::size_t>> qubits_;
    std::vector<std::pair<std::string, std::size_t>> registers_;
};
}  // namespace qret::runtime

#endif  // QRET_RUNTIME_QUANTUM_STATE_H
