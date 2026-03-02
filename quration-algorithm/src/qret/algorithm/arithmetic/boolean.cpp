/**
 * @file qret/algorithm/arithmetic/boolean.cpp
 * @brief Boolean quantum circuits.
 */

#include "qret/algorithm/arithmetic/boolean.h"

#include "qret/frontend/attribute.h"
#include "qret/frontend/control_flow.h"
#include "qret/frontend/intrinsic.h"
#include "qret/ir/instructions.h"

namespace qret::frontend::gate {
//--------------------------------------------------//
// Functions
//--------------------------------------------------//

ir::CallInst* TemporalAnd(const Qubit& t, const Qubit& c0, const Qubit& c1) {
    auto gen = TemporalAndGen(t.GetBuilder());
    auto* circuit = gen.Generate();
    return (*circuit)(t, c0, c1);
}
ir::CallInst* UncomputeTemporalAnd(const Qubit& t, const Qubit& c0, const Qubit& c1) {
    auto gen = UncomputeTemporalAndGen(t.GetBuilder());
    auto* circuit = gen.Generate();
    return (*circuit)(t, c0, c1);
}
ir::CallInst* RT3(const Qubit& t, const Qubit& c0, const Qubit& c1) {
    auto gen = RT3Gen(t.GetBuilder());
    auto* circuit = gen.Generate();
    return (*circuit)(t, c0, c1);
}
ir::CallInst* IRT3(const Qubit& t, const Qubit& c0, const Qubit& c1) {
    auto gen = IRT3Gen(t.GetBuilder());
    auto* circuit = gen.Generate();
    return (*circuit)(t, c0, c1);
}
ir::CallInst* RT4(const Qubit& t, const Qubit& c0, const Qubit& c1) {
    auto gen = RT4Gen(t.GetBuilder());
    auto* circuit = gen.Generate();
    return (*circuit)(t, c0, c1);
}
ir::CallInst* IRT4(const Qubit& t, const Qubit& c0, const Qubit& c1) {
    auto gen = IRT4Gen(t.GetBuilder());
    auto* circuit = gen.Generate();
    return (*circuit)(t, c0, c1);
}
ir::CallInst* MAJ(const Qubit& x, const Qubit& y, const Qubit& z) {
    auto gen = MAJGen(x.GetBuilder());
    auto* circuit = gen.Generate();
    return (*circuit)(x, y, z);
}
ir::CallInst* UMA(const Qubit& x, const Qubit& y, const Qubit& z) {
    auto gen = UMAGen(x.GetBuilder());
    auto* circuit = gen.Generate();
    return (*circuit)(x, y, z);
}

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

Circuit* TemporalAndGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto t = GetQubit(0);
    auto c0 = GetQubit(1);
    auto c1 = GetQubit(2);

    MARK_AS_CLEAN(t);
    H(t);
    T(t);
    CX(t, c0);
    CX(t, c1);
    CX(c0, t);
    CX(c1, t);
    TDag(c0);
    TDag(c1);
    T(t);
    CX(c0, t);
    CX(c1, t);
    H(t);
    S(t);

    return EndCircuitDefinition();
}
Circuit* TemporalAndGen::GenerateAdjoint() const {
    return UncomputeTemporalAndGen(GetBuilder()).Generate();
}
Circuit* UncomputeTemporalAndGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto t = GetQubit(0);
    auto c0 = GetQubit(1);
    auto c1 = GetQubit(2);
    auto r = GetTemporalRegister();

    H(t);
    Measure(t, r);
    control_flow::If(r);
    {
        X(t);
        CZ(c0, c1);
    }
    control_flow::EndIf(r);

    MARK_AS_CLEAN(t);

    return EndCircuitDefinition();
}
Circuit* UncomputeTemporalAndGen::GenerateAdjoint() const {
    return TemporalAndGen(GetBuilder()).Generate();
}
Circuit* RT3Gen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();

    auto t = GetQubit(0);
    auto c0 = GetQubit(1);
    auto c1 = GetQubit(2);

    H(t);
    T(t);
    CX(t, c1);
    TDag(t);
    CX(t, c0);
    T(t);
    CX(t, c1);
    TDag(t);
    H(t);

    return EndCircuitDefinition();
}
Circuit* RT3Gen::GenerateAdjoint() const {
    return IRT3Gen(GetBuilder()).Generate();
}
Circuit* IRT3Gen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();

    auto t = GetQubit(0);
    auto c0 = GetQubit(1);
    auto c1 = GetQubit(2);

    H(t);
    T(t);
    CX(t, c1);
    TDag(t);
    CX(t, c0);
    T(t);
    CX(t, c1);
    TDag(t);
    H(t);

    return EndCircuitDefinition();
}
Circuit* IRT3Gen::GenerateAdjoint() const {
    return RT3Gen(GetBuilder()).Generate();
}
Circuit* RT4Gen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();

    auto t = GetQubit(0);
    auto c0 = GetQubit(1);
    auto c1 = GetQubit(2);

    H(t);
    CX(t, c1);
    TDag(t);
    CX(t, c0);
    T(t);
    CX(t, c1);
    TDag(t);
    CX(t, c0);
    T(t);
    H(t);

    return EndCircuitDefinition();
}
Circuit* RT4Gen::GenerateAdjoint() const {
    return IRT4Gen(GetBuilder()).Generate();
}
Circuit* IRT4Gen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();

    auto t = GetQubit(0);
    auto c0 = GetQubit(1);
    auto c1 = GetQubit(2);

    H(t);
    TDag(t);
    CX(t, c0);
    T(t);
    CX(t, c1);
    TDag(t);
    CX(t, c0);
    T(t);
    CX(t, c1);
    H(t);

    return EndCircuitDefinition();
}
Circuit* IRT4Gen::GenerateAdjoint() const {
    return RT4Gen(GetBuilder()).Generate();
}
Circuit* MAJGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();

    auto x = GetQubit(0);
    auto y = GetQubit(1);
    auto z = GetQubit(2);

    CX(y, x);
    CX(z, x);
    CCX(x, y, z);

    return EndCircuitDefinition();
}
Circuit* UMAGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();

    auto x = GetQubit(0);
    auto y = GetQubit(1);
    auto z = GetQubit(2);

    CCX(x, y, z);
    CX(y, x);
    CX(z, y);

    return EndCircuitDefinition();
}
}  // namespace qret::frontend::gate
