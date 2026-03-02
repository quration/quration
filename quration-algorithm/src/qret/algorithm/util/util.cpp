/**
 * @file qret/algorithm/util/util.cpp
 */

#include "qret/algorithm/util/util.h"

#include "qret/algorithm/arithmetic/boolean.h"
#include "qret/frontend/argument.h"
#include "qret/frontend/control_flow.h"
#include "qret/frontend/intrinsic.h"

namespace qret::frontend::gate {
//--------------------------------------------------//
// Functions
//--------------------------------------------------//

ir::CallInst*
ControlledH(const Qubit& c, const Qubit& q, const Qubit& catalysis, const CleanAncilla& aux) {
    auto gen = ControlledHGen(c.GetBuilder());
    auto* circuit = gen.Generate();
    return (*circuit)(c, q, catalysis, aux);
}
ir::CallInst* Swap(const Qubit& lhs, const Qubit& rhs) {
    auto gen = SwapGen(lhs.GetBuilder());
    auto* circuit = gen.Generate();
    return (*circuit)(lhs, rhs);
}
ir::CallInst*
ControlledSwap(const Qubit& c, const Qubit& lhs, const Qubit& rhs, const CleanAncilla& aux) {
    auto gen = ControlledSwapGen(lhs.GetBuilder());
    auto* circuit = gen.Generate();
    return (*circuit)(c, lhs, rhs, aux);
}

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

Circuit* ControlledHGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto c = GetQubit(0);
    auto q = GetQubit(1);
    auto catalysis = GetQubit(2);
    auto aux = GetQubit(3);
    auto out = GetTemporalRegister();

    H(q);
    S(q);
    H(q);
    X(q);
    CX(catalysis, q);
    X(q);
    X(c);
    TemporalAnd(aux, catalysis, c);
    CX(aux, q);
    S(aux);
    CX(aux, q);
    H(aux);
    Measure(aux, out);
    control_flow::If(out);
    {
        CZ(catalysis, c);
        X(aux);
    }
    control_flow::EndIf(out);
    X(c);
    CX(q, c);
    CX(catalysis, q);
    H(q);
    SDag(q);
    H(q);

    return EndCircuitDefinition();
}
Circuit* SwapGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto lhs = GetQubit(0);
    auto rhs = GetQubit(1);

    CX(lhs, rhs);
    CX(rhs, lhs);
    CX(lhs, rhs);

    return EndCircuitDefinition();
}
Circuit* ControlledSwapGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto c = GetQubit(0);
    auto lhs = GetQubit(1);
    auto rhs = GetQubit(2);
    auto aux = GetQubit(3);

    CX(lhs, rhs);
    TemporalAnd(aux, c, lhs);
    CX(rhs, aux);
    UncomputeTemporalAnd(aux, c, lhs);
    CX(lhs, rhs);

    return EndCircuitDefinition();
}
}  // namespace qret::frontend::gate
