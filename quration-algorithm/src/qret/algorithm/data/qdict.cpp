/**
 * @file qret/algorithm/data/qdict.cpp
 * @brief Quantum dictionary.
 */

#include "qret/algorithm/data/qdict.h"

#include <stdexcept>

#include "qret/algorithm/arithmetic/integer.h"
#include "qret/algorithm/util/array.h"
#include "qret/algorithm/util/util.h"
#include "qret/frontend/argument.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/functor.h"
#include "qret/frontend/intrinsic.h"
#include "qret/ir/instructions.h"

namespace qret::frontend::gate {
//--------------------------------------------------//
// Functions
//--------------------------------------------------//

ir::CallInst* InitializeQDict(size_t l, const Qubits& address, const Qubits& value) {
    if (l == 0) {
        throw std::runtime_error("capacity size must not be zero");
    }
    if (address.Size() % l != 0) {
        throw std::runtime_error("size of 'address' must be multiple of capacity size");
    }
    if (value.Size() % l != 0) {
        throw std::runtime_error("size of 'value' must be multiple of capacity size");
    }
    if (l > (BigInt{1} << (address.Size() / l))) {
        throw std::runtime_error("capacity must be less than or equal to max address");
    }
    auto gen = InitializeQDictGen(address.GetBuilder(), address.Size() / l, value.Size() / l, l);
    auto* circuit = gen.Generate();
    return (*circuit)(address, value);
}
ir::CallInst* InjectIntoQDict(
        size_t l,
        const Qubits& address,
        const Qubits& value,
        const Qubits& key,
        const Qubits& input,
        const CleanAncillae& aux
) {
    return Adjoint(ExtractFromQDict)(l, address, value, key, input, aux);
}
ir::CallInst* ExtractFromQDict(
        size_t l,
        const Qubits& address,
        const Qubits& value,
        const Qubits& key,
        const Qubits& out,
        const CleanAncillae& aux
) {
    if (l == 0) {
        throw std::runtime_error("capacity size must not be zero");
    }
    if (address.Size() % l != 0) {
        throw std::runtime_error("size of 'address' must be multiple of capacity size");
    }
    if (value.Size() % l != 0) {
        throw std::runtime_error("size of 'value' must be multiple of capacity size");
    }
    if (l > (BigInt{1} << (address.Size() / l))) {
        throw std::runtime_error("capacity must be less than or equal to max address");
    }
    if (key.Size() != address.Size() / l) {
        throw std::runtime_error("size of 'key' must be equal to size of single address");
    }
    if (out.Size() != value.Size() / l) {
        throw std::runtime_error("size of 'out' must be equal to size of single value");
    }
    auto gen = ExtractFromQDictGen(address.GetBuilder(), key.Size(), out.Size(), l);
    auto* circuit = gen.Generate();
    return (*circuit)(address, value, key, out, aux);
}

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

Circuit* InitializeQDictGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();

    auto address = GetQubits(0);
    auto value = GetQubits(1);

    const auto max_address = (BigInt{1} << n) - 1;

    for (auto i = std::size_t{0}; i < l; ++i) {
        ApplyXToEachIfImm(max_address, address.Range(n * i, n));
    }

    return EndCircuitDefinition();
}
Circuit* ExtractFromQDictGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();

    auto address = GetQubits(0);
    auto value = GetQubits(1);
    auto key = GetQubits(2);
    auto out = GetQubits(3);
    auto aux = GetQubits(4);

    const auto max_address = (BigInt{1} << n) - 1;
    const auto aux_eq = aux[0];
    const auto aux_address = aux.Range(1, n);
    const auto aux_other = aux.Range(n + 1, std::max({n, m, std::size_t{3}}) - 1);

    // Preprocess
    ApplyXToEachIfImm(max_address, aux_address);
    const auto address_out = address + aux_address;
    const auto value_out = value + out;

    // Extract key value
    for (auto i = std::size_t{0}; i < l; ++i) {
        EqualTo(key, address.Range(n * i, n), aux_eq, aux_other.Range(0, n - 1));
        for (auto j = std::size_t{0}; j < n; ++j) {
            ControlledSwap(
                    aux_eq,
                    address_out.Range(n * i, n)[j],
                    address_out.Range(n * (i + 1), n)[j],
                    aux_other[0]
            );
        }
        for (auto j = std::size_t{0}; j < m; ++j) {
            ControlledSwap(
                    aux_eq,
                    value_out.Range(m * i, m)[j],
                    value_out.Range(m * (i + 1), m)[j],
                    aux_other[0]
            );
        }
        LessThan(
                address_out.Range(n * (i + 1), n),
                address_out.Range(n * i, n),
                aux_eq,
                aux_other.Range(0, 2)
        );
    }

    // Uncompute
    EqualToImm(0, out, aux_eq, aux_other.Range(0, m - 1));
    ApplyCXToEachIfImm(max_address, aux_eq, aux_address);
    X(aux_eq);
    for (auto i = std::size_t{0}; i < n; ++i) {
        CCX(aux_address[i], aux_eq, key[i]);
    }
    X(aux_eq);
    EqualToImm(0, out, aux_eq, aux_other.Range(0, m - 1));

    return EndCircuitDefinition();
}
}  // namespace qret::frontend::gate
