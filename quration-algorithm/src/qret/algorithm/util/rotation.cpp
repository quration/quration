/**
 * @file qret/algorithm/util/rotation.cpp
 */

#include "qret/algorithm/util/rotation.h"

#include <numbers>
#include <random>

#include "qret/algorithm/data/qrom.h"
#include "qret/algorithm/util/write_register.h"
#include "qret/base/type.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/intrinsic.h"
#include "qret/ir/instructions.h"
#include "qret/math/float.h"

namespace qret::frontend::gate {
//--------------------------------------------------//
// Functions
//--------------------------------------------------//

ir::CallInst* R1(const Qubit& q, double theta, double precision) {
    auto gen = R1Gen(q.GetBuilder(), theta, precision);
    auto* circuit = gen.Generate();
    return (*circuit)(q);
}
ir::CallInst* ControlledR1(const Qubit& c, const Qubit& q, double theta, double precision) {
    auto gen = ControlledRGen(q.GetBuilder(), ControlledRGen::RType::R1, theta, precision);
    auto* circuit = gen.Generate();
    return (*circuit)(c, q);
}
ir::CallInst* ControlledRX(const Qubit& c, const Qubit& q, double theta, double precision) {
    auto gen = ControlledRGen(q.GetBuilder(), ControlledRGen::RType::RX, theta, precision);
    auto* circuit = gen.Generate();
    return (*circuit)(c, q);
}
ir::CallInst* ControlledRY(const Qubit& c, const Qubit& q, double theta, double precision) {
    auto gen = ControlledRGen(q.GetBuilder(), ControlledRGen::RType::RY, theta, precision);
    auto* circuit = gen.Generate();
    return (*circuit)(c, q);
}
ir::CallInst* ControlledRZ(const Qubit& c, const Qubit& q, double theta, double precision) {
    auto gen = ControlledRGen(q.GetBuilder(), ControlledRGen::RType::RZ, theta, precision);
    auto* circuit = gen.Generate();
    return (*circuit)(c, q);
}
ir::CallInst* RandomizedMultiplexedRotation(
        std::uint64_t lookup_par,
        std::uint64_t del_lookup_par,
        std::uint64_t sub_bit_precision,
        std::uint64_t seed,
        std::span<const ir::FloatWithPrecision> theta,
        const Qubit& q,
        const Qubits& address,
        const CleanAncillae& aux
) {
    auto gen = RandomizedMultiplexedRotationGen(
            q.GetBuilder(),
            theta,
            lookup_par,
            del_lookup_par,
            sub_bit_precision,
            seed,
            address.Size()
    );
    auto* circuit = gen.Generate();
    return (*circuit)(q, address, aux);
}

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

Circuit* R1Gen::Generate() const {
    BeginCircuitDefinition();

    auto q = GetQubit(0);

    RZ(q, theta, precision);
    GlobalPhase(GetBuilder(), theta / 2, precision);

    return EndCircuitDefinition();
}
Circuit* ControlledRGen::Generate() const {
    BeginCircuitDefinition();

    auto c = GetQubit(0);
    auto q = GetQubit(1);

    switch (type) {
        case RType::R1: {
            CX(q, c);
            RZ(q, -theta / 2, precision);
            CX(q, c);
            RZ(q, theta / 2, precision);
            GlobalPhase(GetBuilder(), theta / 2, precision);
            X(c);
            R1(c, -theta / 2, precision);
            X(c);
            break;
        }
        case RType::RX: {
            CX(q, c);
            RX(q, -theta / 2, precision);
            CX(q, c);
            RX(q, theta / 2, precision);
            break;
        }
        case RType::RY: {
            CX(q, c);
            RY(q, -theta / 2, precision);
            CX(q, c);
            RY(q, theta / 2, precision);
            break;
        }
        case RType::RZ: {
            CX(q, c);
            RZ(q, -theta / 2, precision);
            CX(q, c);
            RZ(q, theta / 2, precision);
            break;
        }
        default: {
            // unreachable
            break;
        }
    }

    return EndCircuitDefinition();
}
Circuit* RandomizedMultiplexedRotationGen::Generate() const {
    BeginCircuitDefinition();
    auto q = GetQubit(0);
    auto address = GetQubits(1);
    auto aux = GetQubits(2);
    const auto dict_size = theta.size();

    // Split aux
    auto to_rotate = aux[0];
    auto value = aux.Range(1, sub_bit_precision);
    auto others = aux.Range(1 + sub_bit_precision, aux_size - 1 - sub_bit_precision);

    // Create randomized dictionary using random
    auto theta_impl = GetTemporalRegisters(sub_bit_precision * dict_size);
    {
        auto engine = std::mt19937_64{seed};
        auto dist = std::uniform_real_distribution<>(0.0, 1.0);

        for (auto i = std::size_t{0}; i < dict_size; ++i) {
            const auto tmp = theta[i].value / (2.0 * std::numbers::pi);
            auto [error, bits] = math::DiscretizeDoubleWithError(sub_bit_precision - 1, tmp);
            bits.emplace_back(error >= dist(engine));
            WriteRegisters(
                    BoolArrayAsBigInt(bits),
                    theta_impl.Range(i * sub_bit_precision, sub_bit_precision)
            );
        }
    }

    // QROAM
    {
        const auto num_duplicates = std::size_t{1} << lookup_par;
        auto duplicate = others.Range(0, (num_duplicates - 1) * sub_bit_precision);
        auto tmp_aux = others.Range((num_duplicates - 1) * sub_bit_precision, n - 1 - lookup_par);
        QROAMDirty(lookup_par, address, value, duplicate, tmp_aux, theta_impl);
    }
    // QROM(address, value, others.Range(0, n - 1), theta_impl);
    // Controlled Pauli-Z rotation
    for (auto i = std::size_t{0}; i < sub_bit_precision; ++i) {
        CX(q, value[i]);
        RZ(q, std::pow(0.5, i + 1) * std::numbers::pi);
        CX(q, value[i]);
        RZ(q, -std::pow(0.5, i + 1) * std::numbers::pi);
    }
    // UncomputeQROAM
    // FIXME
    // {
    //     const auto num_duplicates = std::size_t{1} << del_lookup_par;
    //     auto duplicate = others.Range(0, num_duplicates - 1);
    //     auto src = others.Range(num_duplicates - 1, 1)[0];
    //     auto tmp_aux = others.Range(num_duplicates, n - 1 - del_lookup_par);
    //     UncomputeQROAMDirty(del_lookup_par, address, value, duplicate, src, tmp_aux, theta_impl);
    // }
    QROM(address, value, others.Range(0, n - 1), theta_impl);

    return EndCircuitDefinition();
}
}  // namespace qret::frontend::gate
