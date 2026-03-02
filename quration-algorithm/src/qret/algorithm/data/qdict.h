/**
 * @file qret/algorithm/data/qdict.h
 * @brief Quantum dictionary.
 */

#ifndef QRET_GATE_DATA_QDICT_H
#define QRET_GATE_DATA_QDICT_H

#include <algorithm>

#include "qret/frontend/argument.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/ir/instructions.h"
#include "qret/qret_export.h"

namespace qret::frontend::gate {
//--------------------------------------------------//
// Functions
//--------------------------------------------------//

/**
 * @brief Initializes quantum dictionaries (q-dict) without QRAM.
 * @details Implements https://arxiv.org/abs/2204.13835.
 * This q-dict holds fixed-length list of sorted address-value pairs.
 * All addresses are initialized to the maximum address and the value is initialized to 0.
 * Note that if you want to use `InjectIntoQDict` or `ExtractFromQDict`, run this function in
 * advance.
 *
 * @param l       capacity of q-dict
 * @param address size = n * l
 * @param value   size = m * l
 */
QRET_EXPORT ir::CallInst* InitializeQDict(size_t l, const Qubits& address, const Qubits& value);
/**
 * @brief Injects addressed value into quantum dictionaries (q-dict) without QRAM.
 * @details Implements https://arxiv.org/abs/2204.13835.
 * Injects `input` value to address corresponding to `key`.
 * Note that if you want to inject into an address that already has a value,
 * extract that address first and then inject into it.
 *
 * @param l       capacity of q-dict
 * @param address size = n * l
 * @param value   size = m * l
 * @param key     size = n
 * @param input   size = m
 * @param aux     size = n + max(n, m, 3)
 */
QRET_EXPORT ir::CallInst* InjectIntoQDict(
        size_t l,
        const Qubits& address,
        const Qubits& value,
        const Qubits& key,
        const Qubits& input,
        const CleanAncillae& aux
);
/**
 * @brief Extracts addressed value from quantum dictionaries (q-dict) without QRAM.
 * @details Implements https://arxiv.org/abs/2204.13835.
 * If `key` exists in `address`, save its value in `out` and remove it from q-dict.
 * Does nothing if `key` does not exist.
 *
 * @param l       capacity of q-dict
 * @param address size = n * l
 * @param value   size = m * l
 * @param key     size = n
 * @param out     size = m
 * @param aux     size = n + max(n, m, 3)
 */
QRET_EXPORT ir::CallInst* ExtractFromQDict(
        size_t l,
        const Qubits& address,
        const Qubits& value,
        const Qubits& key,
        const Qubits& out,
        const CleanAncillae& aux
);

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

struct QRET_EXPORT InitializeQDictGen : CircuitGenerator {
    std::size_t n;
    std::size_t m;
    std::size_t l;

    static inline const char* Name = "InitializeQDict";
    explicit InitializeQDictGen(
            CircuitBuilder* builder,
            std::size_t n,
            std::size_t m,
            std::size_t l
    )
        : CircuitGenerator(builder)
        , n{n}
        , m{m}
        , l{l} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{},{})", GetName(), n, m, l);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("address", Type::Qubit, n * l, Attribute::Operate);
        arg.Add("value", Type::Qubit, m * l, Attribute::Operate);
    }
    Circuit* Generate() const override;
};
struct QRET_EXPORT ExtractFromQDictGen : CircuitGenerator {
    std::size_t n;
    std::size_t m;
    std::size_t l;

    static inline const char* Name = "ExtractFromQDict";
    explicit ExtractFromQDictGen(
            CircuitBuilder* builder,
            std::size_t n,
            std::size_t m,
            std::size_t l
    )
        : CircuitGenerator(builder)
        , n{n}
        , m{m}
        , l{l} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{},{})", GetName(), n, m, l);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("address", Type::Qubit, n * l, Attribute::Operate);
        arg.Add("value", Type::Qubit, m * l, Attribute::Operate);
        arg.Add("key", Type::Qubit, n, Attribute::Operate);
        arg.Add("out", Type::Qubit, m, Attribute::Operate);
        arg.Add("aux", Type::Qubit, n + std::max({n, m, std::size_t{3}}), Attribute::CleanAncilla);
    }
    Circuit* Generate() const override;
};
}  // namespace qret::frontend::gate

#endif  // QRET_GATE_DATA_QDICT_H
