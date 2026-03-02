/**
 * @file qret/frontend/attribute.cpp
 * @brief Attributes of qubits.
 */

#include "qret/frontend/attribute.h"

#include "qret/frontend/argument.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/builder.h"  // DO NOT DELETE
#include "qret/frontend/circuit.h"  // DO NOT DELETE
#include "qret/frontend/circuit_generator.h"  // DO NOT DELETE
#include "qret/ir/instructions.h"
#include "qret/ir/value.h"

namespace qret::frontend::attribute {
namespace {
ir::Qubit GetQ(const Qubit& q) {
    return {q.GetId()};
}
}  // namespace
void MarkAsClean(const Qubit& q) {
    auto* bb = q.GetBuilder()->GetInsertionPoint();
    ir::CleanInst::Create(GetQ(q), bb);
}
void MarkAsClean(const Qubit& q, std::string_view file, std::uint32_t line) {
    auto* bb = q.GetBuilder()->GetInsertionPoint();
    ir::CleanInst::Create(GetQ(q), bb)->AddLocation(file, line);
}
void SetCleanProb(const Qubit& q, double clean_prob, double precision) {
    auto* bb = q.GetBuilder()->GetInsertionPoint();
    ir::CleanProbInst::Create(GetQ(q), ir::FloatWithPrecision{clean_prob, precision}, bb);
}
void SetCleanProb(
        const Qubit& q,
        double clean_prob,
        double precision,
        std::string_view file,
        std::uint32_t line
) {
    auto* bb = q.GetBuilder()->GetInsertionPoint();
    ir::CleanProbInst::Create(GetQ(q), ir::FloatWithPrecision{clean_prob, precision}, bb)
            ->AddLocation(file, line);
}
void MarkAsDirtyBegin(const Qubit& q) {
    auto* bb = q.GetBuilder()->GetInsertionPoint();
    ir::DirtyInst::CreateBegin(GetQ(q), bb);
}
void MarkAsDirtyBegin(const Qubit& q, std::string_view file, std::uint32_t line) {
    auto* bb = q.GetBuilder()->GetInsertionPoint();
    ir::DirtyInst::CreateBegin(GetQ(q), bb)->AddLocation(file, line);
}
void MarkAsDirtyEnd(const Qubit& q) {
    auto* bb = q.GetBuilder()->GetInsertionPoint();
    ir::DirtyInst::CreateEnd(GetQ(q), bb);
}
void MarkAsDirtyEnd(const Qubit& q, std::string_view file, std::uint32_t line) {
    auto* bb = q.GetBuilder()->GetInsertionPoint();
    ir::DirtyInst::CreateEnd(GetQ(q), bb)->AddLocation(file, line);
}
}  // namespace qret::frontend::attribute
