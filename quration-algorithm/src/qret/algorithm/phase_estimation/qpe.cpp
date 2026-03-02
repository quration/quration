/**
 * @file qret/algorithm/phase_estimation/qpe.cpp
 * @brief QPE.
 */

#include "qret/algorithm/phase_estimation/qpe.h"

#include "qret/algorithm/arithmetic/integer.h"
#include "qret/algorithm/data/qrom.h"
#include "qret/algorithm/preparation/uniform.h"
#include "qret/algorithm/transform/qft.h"
#include "qret/algorithm/util/util.h"
#include "qret/frontend/argument.h"
#include "qret/frontend/attribute.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/functor.h"
#include "qret/frontend/intrinsic.h"
#include "qret/ir/instructions.h"

namespace qret::frontend::gate {
//--------------------------------------------------//
// Functions
//--------------------------------------------------//

ir::CallInst* PREPARE(
        std::span<const std::pair<std::uint32_t, BigInt>> alias_sampling,
        const Qubits& index,
        const Qubits& alternate_index,
        const Qubits& random,
        const Qubits& switch_weight,
        const Qubit& cmp,
        const CleanAncillae& aux
) {
    auto gen = PREPAREGen(index.GetBuilder(), alias_sampling, index.Size(), random.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(index, alternate_index, random, switch_weight, cmp, aux);
}

ir::CallInst* SELECT(
        const PauliArray& pauli_strings,
        const Qubits& address,
        const Qubits& tgt,
        const CleanAncillae& aux
) {
    auto gen = SELECTGen(address.GetBuilder(), pauli_strings);
    auto* circuit = gen.Generate();
    return (*circuit)(address, tgt, aux);
}

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

Circuit* PREPAREGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto index = GetQubits(0);  // size = n
    auto alternate_index = GetQubits(1);  // size = n
    auto random = GetQubits(2);  // size = m
    auto switch_weight = GetQubits(3);  // size = m
    auto cmp = GetQubit(4);  // size = 1
    auto aux = GetQubits(5);  // size = n+3

    const auto num_paulis = alias_sampling.size();

    PrepareUniformSuperposition(num_paulis, index, aux);

    for (const auto& q : random) {
        H(q);
    }

    {
        auto dict_impl = std::vector<BigInt>(num_paulis);
        const BigInt mask = (BigInt(1) << n) - 1;
        for (auto i = std::size_t{0}; i < num_paulis; ++i) {
            const auto [alternate_index, switch_weight_val] = alias_sampling[i];
            dict_impl[i] = (alternate_index & mask) + (switch_weight_val << n);
        }
        QROMImm(index, alternate_index + switch_weight, aux.Range(0, n - 1), dict_impl);
    }

    {
        // Flip cmp if random + switch_weight overflows
        AddCuccaro(random + cmp, switch_weight + aux[0], aux[1]);
        // uncompute
        SubCuccaro(random, switch_weight, aux[1]);
    }

    for (auto i = std::size_t{0}; i < n; ++i) {
        ControlledSwap(cmp, index[i], alternate_index[i], aux[0]);
    }

    return EndCircuitDefinition();
}
Circuit* SELECTGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto address = GetQubits(0);  // size = address_size
    auto system = GetQubits(1);  // size = system_size
    auto aux = GetQubits(2);  // size = address_size - 1

    const auto apply_pauli = [](const auto& c, const auto& q, const math::Pauli pauli) {
        switch (pauli) {
            case math::Pauli::X:
                CX(q, c);
                break;
            case math::Pauli::Y:
                CY(q, c);
                break;
            case math::Pauli::Z:
                CZ(q, c);
                break;
            default:
                break;
        }
    };
    const auto apply_paulis = [apply_pauli](
                                      const auto& c,
                                      const auto& qs,
                                      const std::span<const math::Pauli> paulis
                              ) {
        assert(qs.Size() == paulis.size());
        for (auto i = std::size_t{0}; i < qs.Size(); ++i) {
            apply_pauli(c, qs[i], paulis[i]);
        }
    };

    UnaryIterBegin(0, address, aux);
    for (auto k = std::size_t{0}; k < pauli_strings.size() - 1; ++k) {
        apply_paulis(aux[0], system, pauli_strings[k]);
        UnaryIterStep(k, address, aux);
    }
    apply_paulis(aux[0], system, pauli_strings[pauli_strings.size() - 1]);
    UnaryIterEnd(pauli_strings.size() - 1, address, aux);

    return EndCircuitDefinition();
}

frontend::Circuit* PhaseEstimationGen::Generate() const {
    BeginCircuitDefinition();
    auto system = GetQubits(0);  // size = system_size
    auto hadamard = GetQubits(1);  // size = hadamard_size
    auto index = GetQubits(2);  // size = n
    auto alternate_index = GetQubits(3);  // size = n
    auto random = GetQubits(4);  // size = m
    auto switch_weight = GetQubits(5);  // size = m
    auto cmp = GetQubit(6);  // size = 1
    auto aux = GetQubits(7);  // size = n+3
    auto measure_system = GetRegisters(8);  // size = system_size
    auto measure_hadamard = GetRegisters(9);  // size = hadamard_size
    auto measure_index = GetRegisters(10);  // size = n

    const auto num_paulis = paulis.size();

    const auto apply_pauli = [](const auto& c, const auto& q, std::span<const math::Pauli> pauli) {
        for (const auto& p : pauli) {
            switch (p) {
                case math::Pauli::X:
                    CX(q, c);
                    break;
                case math::Pauli::Y:
                    CY(q, c);
                    break;
                case math::Pauli::Z:
                    CZ(q, c);
                    break;
                default:
                    break;
            }
        }
    };
    const auto apply_paulis = [apply_pauli](
                                      const auto& c,
                                      const auto& qs,
                                      std::span<const std::vector<math::Pauli>> paulis
                              ) {
        assert(qs.Size() == paulis.size());
        for (auto i = std::size_t{0}; i < qs.Size(); ++i) {
            apply_pauli(c, qs[i], paulis[i]);
        }
    };

    for (const auto& q : hadamard) {
        H(q);
    }

    PREPARE(alias_sampling, index, alternate_index, random, switch_weight, cmp, aux);

    for (auto i = std::size_t{0}; i < hadamard_size; ++i) {
        // Apply SELECT-based unitary for 2^i times
        const auto& ctrl = hadamard[i];

        for (auto _ = std::size_t{0}; _ < std::size_t{1} << i; ++_) {
            // loop unitary for 2^i times
            // U^(2^i)
            auto aux_sel = aux.Range(0, n);
            MultiControlledUnaryIterBegin(0, ctrl, index, aux_sel);
            for (auto k = std::size_t{0}; k < num_paulis - 1; ++k) {
                apply_paulis(aux_sel[0], system, paulis[k]);
                MultiControlledUnaryIterStep(k, ctrl, index, aux_sel);
            }
            apply_paulis(aux_sel[0], system, paulis[num_paulis - 1]);
            MultiControlledUnaryIterEnd(num_paulis - 1, ctrl, index, aux_sel);

            for (const auto& q : aux) {
                MARK_AS_CLEAN(q);
            }

            Adjoint(PREPARE)(
                    alias_sampling,
                    index,
                    alternate_index,
                    random,
                    switch_weight,
                    cmp,
                    aux
            );
            // multi controlled Z
            {
                EqualToImm(0, index, aux[0], aux.Range(1, n - 1));
                CZ(ctrl, aux[0]);
                EqualToImm(0, index, aux[0], aux.Range(1, n - 1));  // uncomputation
            }
            PREPARE(alias_sampling, index, alternate_index, random, switch_weight, cmp, aux);
        }
    }

    Adjoint(FourierTransform)(hadamard);
    Adjoint(PREPARE)(alias_sampling, index, alternate_index, random, switch_weight, cmp, aux);

    for (auto i = std::size_t{0}; i < hadamard_size; ++i) {
        Measure(hadamard[i], measure_hadamard[i]);
    }
    for (auto i = std::size_t{0}; i < system_size; ++i) {
        Measure(system[i], measure_system[i]);
    }
    for (auto i = std::size_t{0}; i < n; ++i) {
        Measure(index[i], measure_index[i]);
    }

    return EndCircuitDefinition();
}
}  // namespace qret::frontend::gate
