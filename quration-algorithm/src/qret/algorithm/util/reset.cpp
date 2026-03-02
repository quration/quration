/**
 * @file qret/algorithm/util/reset.cpp
 */

#include "qret/algorithm/util/reset.h"

#include "qret/frontend/control_flow.h"
#include "qret/frontend/intrinsic.h"
#include "qret/ir/instructions.h"

namespace qret::frontend::gate {
//--------------------------------------------------//
// Functions
//--------------------------------------------------//

ir::CallInst* Reset(const Qubit& q) {
    auto gen = ResetGen(q.GetBuilder());
    auto* circuit = gen.Generate();
    return (*circuit)(q);
}

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

Circuit* ResetGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();

    auto q = GetQubit(0);
    auto tmp = GetTemporalRegister();

    Measure(q, tmp);
    control_flow::If(tmp);
    X(q);
    control_flow::EndIf(tmp);

    return EndCircuitDefinition();
}
}  // namespace qret::frontend::gate
