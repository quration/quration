/**
 * @file qret/frontend/intrinsic.cpp
 * @brief Intrinsic gates.
 */

#include "qret/frontend/intrinsic.h"

#include <vector>

#include "qret/frontend/argument.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"  // DO NOT DELETE.
#include "qret/ir/instructions.h"
#include "qret/ir/value.h"

namespace qret::frontend::gate {
namespace {
ir::Qubit GetQ(const Qubit& q) {
    return {q.GetId()};
}
std::vector<ir::Qubit> GetQVec(const Qubits& qs) {
    auto ret = std::vector<ir::Qubit>();
    ret.reserve(qs.Size());
    for (const auto& q : qs) {
        ret.emplace_back(q.GetId());
    }
    return ret;
}
ir::Register GetR(const Register& r) {
    return {r.GetId()};
}
std::vector<ir::Register> GetRVec(const Registers& rs) {
    auto ret = std::vector<ir::Register>();
    ret.reserve(rs.Size());
    for (const auto& r : rs) {
        ret.emplace_back(r.GetId());
    }
    return ret;
}
}  // namespace

using ir::MeasurementInst, ir::UnaryInst, ir::ParametrizedRotationInst, ir::BinaryInst,
        ir::TernaryInst, ir::MultiControlInst, ir::GlobalPhaseInst, ir::CallCFInst,
        ir::DiscreteDistInst;

//--------------------------------------------------//
// Measurement
//--------------------------------------------------//

MeasurementInst* Measure(const Qubit& q, const Register& r) {
    auto* bb = q.GetBuilder()->GetInsertionPoint();
    return MeasurementInst::Create(GetQ(q), GetR(r), bb);
}

//--------------------------------------------------//
// Unary qubit gate
//--------------------------------------------------//

UnaryInst* I(const Qubit& q) {
    auto* bb = q.GetBuilder()->GetInsertionPoint();
    return UnaryInst::CreateIInst(GetQ(q), bb);
}
UnaryInst* X(const Qubit& q) {
    auto* bb = q.GetBuilder()->GetInsertionPoint();
    return UnaryInst::CreateXInst(GetQ(q), bb);
}
UnaryInst* Y(const Qubit& q) {
    auto* bb = q.GetBuilder()->GetInsertionPoint();
    return UnaryInst::CreateYInst(GetQ(q), bb);
}
UnaryInst* Z(const Qubit& q) {
    auto* bb = q.GetBuilder()->GetInsertionPoint();
    return UnaryInst::CreateZInst(GetQ(q), bb);
}
UnaryInst* H(const Qubit& q) {
    auto* bb = q.GetBuilder()->GetInsertionPoint();
    return UnaryInst::CreateHInst(GetQ(q), bb);
}
UnaryInst* S(const Qubit& q) {
    auto* bb = q.GetBuilder()->GetInsertionPoint();
    return UnaryInst::CreateSInst(GetQ(q), bb);
}
UnaryInst* SDag(const Qubit& q) {
    auto* bb = q.GetBuilder()->GetInsertionPoint();
    return UnaryInst::CreateSDagInst(GetQ(q), bb);
}
UnaryInst* T(const Qubit& q) {
    auto* bb = q.GetBuilder()->GetInsertionPoint();
    return UnaryInst::CreateTInst(GetQ(q), bb);
}
UnaryInst* TDag(const Qubit& q) {
    auto* bb = q.GetBuilder()->GetInsertionPoint();
    return UnaryInst::CreateTDagInst(GetQ(q), bb);
}
ParametrizedRotationInst* RX(const Qubit& q, double theta, double precision) {
    auto* bb = q.GetBuilder()->GetInsertionPoint();
    return ParametrizedRotationInst::CreateRXInst(
            GetQ(q),
            ir::FloatWithPrecision{theta, precision},
            bb
    );
}
ParametrizedRotationInst* RY(const Qubit& q, double theta, double precision) {
    auto* bb = q.GetBuilder()->GetInsertionPoint();
    return ParametrizedRotationInst::CreateRYInst(
            GetQ(q),
            ir::FloatWithPrecision{theta, precision},
            bb
    );
}
ParametrizedRotationInst* RZ(const Qubit& q, double theta, double precision) {
    auto* bb = q.GetBuilder()->GetInsertionPoint();
    return ParametrizedRotationInst::CreateRZInst(
            GetQ(q),
            ir::FloatWithPrecision{theta, precision},
            bb
    );
}

//--------------------------------------------------//
// Binary qubit gate
//--------------------------------------------------//

BinaryInst* CX(const Qubit& t, const Qubit& c) {
    auto* bb = t.GetBuilder()->GetInsertionPoint();
    return BinaryInst::CreateCXInst(GetQ(t), GetQ(c), bb);
}
BinaryInst* CY(const Qubit& t, const Qubit& c) {
    auto* bb = t.GetBuilder()->GetInsertionPoint();
    return BinaryInst::CreateCYInst(GetQ(t), GetQ(c), bb);
}
BinaryInst* CZ(const Qubit& t, const Qubit& c) {
    auto* bb = t.GetBuilder()->GetInsertionPoint();
    return BinaryInst::CreateCZInst(GetQ(t), GetQ(c), bb);
}

//--------------------------------------------------//
// Ternary qubit gate
//--------------------------------------------------//

TernaryInst* CCX(const Qubit& t, const Qubit& c0, const Qubit& c1) {
    auto* bb = t.GetBuilder()->GetInsertionPoint();
    return TernaryInst::CreateCCXInst(GetQ(t), GetQ(c0), GetQ(c1), bb);
}
TernaryInst* CCY(const Qubit& t, const Qubit& c0, const Qubit& c1) {
    auto* bb = t.GetBuilder()->GetInsertionPoint();
    return TernaryInst::CreateCCYInst(GetQ(t), GetQ(c0), GetQ(c1), bb);
}
TernaryInst* CCZ(const Qubit& t, const Qubit& c0, const Qubit& c1) {
    auto* bb = t.GetBuilder()->GetInsertionPoint();
    return TernaryInst::CreateCCZInst(GetQ(t), GetQ(c0), GetQ(c1), bb);
}

//--------------------------------------------------//
// Multi control qubit gate
//--------------------------------------------------//

MultiControlInst* MCX(const Qubit& t, const Qubits& c) {
    auto* bb = t.GetBuilder()->GetInsertionPoint();
    return MultiControlInst::CreateMCXInst(GetQ(t), GetQVec(c), bb);
}

//--------------------------------------------------//
// Others
//--------------------------------------------------//

GlobalPhaseInst* GlobalPhase(CircuitBuilder* builder, double theta, double precision) {
    auto* bb = builder->GetInsertionPoint();
    return GlobalPhaseInst::Create({theta, precision}, bb);
}

//--------------------------------------------------//
// Classical
//--------------------------------------------------//

CallCFInst*
CallCF(const PortableAnnotatedFunction& function, const Registers& in, const Registers& out) {
    CircuitBuilder* builder = in.GetBuilder();
    auto* bb = builder->GetInsertionPoint();
    return CallCFInst::Create(function, GetRVec(in), GetRVec(out), bb);
}
DiscreteDistInst*
DiscreteDistribution(const std::vector<double>& weights, const Registers& registers) {
    CircuitBuilder* builder = registers.GetBuilder();
    auto* bb = builder->GetInsertionPoint();
    return ir::DiscreteDistInst::Create(weights, GetRVec(registers), bb);
}
}  // namespace qret::frontend::gate
