/**
 * @file qret/algorithm/util/array.cpp
 */

#include "qret/algorithm/util/array.h"

#include "qret/base/type.h"
#include "qret/frontend/control_flow.h"
#include "qret/frontend/intrinsic.h"
#include "qret/ir/instructions.h"

namespace qret::frontend::gate {
//--------------------------------------------------//
// Functions
//--------------------------------------------------//

qret::ir::CallInst* ApplyXToEach(const Qubits& tgt) {
    auto gen = ApplyXToEachGen(tgt.GetBuilder(), tgt.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(tgt);
}
ir::CallInst* ApplyXToEachIf(const Qubits& tgt, const Registers& bools) {
    auto gen = ApplyXToEachIfGen(tgt.GetBuilder(), tgt.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(tgt, bools);
}
ir::CallInst* ApplyCXToEachIf(const Qubit& c, const Qubits& tgt, const Registers& bools) {
    auto gen = ApplyCXToEachIfGen(tgt.GetBuilder(), tgt.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(c, tgt, bools);
}
ir::CallInst* ApplyXToEachIfImm(const BigInt& imm, const Qubits& t) {
    auto gen = ApplyXToEachIfImmGen(t.GetBuilder(), imm, t.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(t);
}
ir::CallInst* ApplyCXToEachIfImm(const BigInt& imm, const Qubit& c, const Qubits& t) {
    auto gen = ApplyCXToEachIfImmGen(t.GetBuilder(), imm, t.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(c, t);
}

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

qret::frontend::Circuit* ApplyXToEachGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto tgt = GetQubits(0);

    for (const auto& q : tgt) {
        X(q);
    }

    return EndCircuitDefinition();
}
qret::frontend::Circuit* ApplyXToEachIfGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto tgt = GetQubits(0);  // size = size
    auto bools = GetRegisters(1);  // size = size

    for (auto i = std::size_t{0}; i < size; ++i) {
        control_flow::If(bools[i]);
        { X(tgt[i]); }
        control_flow::EndIf(bools[i]);
    }

    return EndCircuitDefinition();
}
qret::frontend::Circuit* ApplyCXToEachIfGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto c = GetQubit(0);  // size = 1
    auto tgt = GetQubits(1);  // size = size
    auto bools = GetRegisters(2);  // size = size

    for (auto i = std::size_t{0}; i < size; ++i) {
        control_flow::If(bools[i]);
        { CX(tgt[i], c); }
        control_flow::EndIf(bools[i]);
    }

    return EndCircuitDefinition();
}
Circuit* ApplyXToEachIfImmGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto tgt = GetQubits(0);

    for (auto i = std::uint64_t{0}; i < size; ++i) {
        if (((imm >> i) & 1) == 1) {
            X(tgt[i]);
        }
    }

    return EndCircuitDefinition();
}
Circuit* ApplyCXToEachIfImmGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto c = GetQubit(0);
    auto tgt = GetQubits(1);

    for (auto i = std::uint64_t{0}; i < size; ++i) {
        if (((imm >> i) & 1) == 1) {
            CX(tgt[i], c);
        }
    }

    return EndCircuitDefinition();
}
}  // namespace qret::frontend::gate
