/**
 * @file qret/frontend/functor.cpp
 * @brief Functor: adjoint and control.
 */

#include "qret/frontend/functor.h"

#include <fmt/core.h>

#include <cassert>
#include <stdexcept>

#include "qret/base/cast.h"
#include "qret/frontend/argument.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/ir/instruction.h"
#include "qret/ir/instructions.h"
#include "qret/ir/value.h"

namespace qret::frontend::impl {
namespace {
ir::Qubit GetQ(const Qubit& q) {
    return {q.GetId()};
}
ir::Qubit IncrementIndex(ir::Qubit q) {
    return {q.id + 1};
}
std::vector<ir::Qubit> IncrementIndex(const std::vector<ir::Qubit>& qs) {
    auto ret = std::vector<ir::Qubit>();
    ret.reserve(qs.size());
    for (const auto q : qs) {
        ret.emplace_back(q.id + 1);
    }
    return ret;
}
}  // namespace
static inline const auto AdjointPrefix = std::string("__adjoint__");
static inline const auto ControlledPrefix = std::string("__controlled__");

//--------------------------------------------------//
// Functor: adjoint
//--------------------------------------------------//

void InsertAdjointOfParametrizedRotation(
        const ir::ParametrizedRotationInst* inst,
        ir::BasicBlock* bb
) {
    auto theta = inst->GetTheta();
    theta.value *= -1;
    ir::ParametrizedRotationInst::Create(inst->GetOpcode(), inst->GetQubit(), theta, bb);
}
void InsertAdjointOfUnary(const ir::UnaryInst* inst, ir::BasicBlock* bb) {
    const auto q = inst->GetQubit();
    switch (inst->GetOpcode().GetCode()) {
        case ir::Opcode::Table::I:
            ir::UnaryInst::CreateIInst(q, bb);
            return;
        case ir::Opcode::Table::X:
            ir::UnaryInst::CreateXInst(q, bb);
            return;
        case ir::Opcode::Table::Y:
            ir::UnaryInst::CreateYInst(q, bb);
            return;
        case ir::Opcode::Table::Z:
            ir::UnaryInst::CreateZInst(q, bb);
            return;
        case ir::Opcode::Table::H:
            ir::UnaryInst::CreateHInst(q, bb);
            return;
        case ir::Opcode::Table::S:
            ir::UnaryInst::CreateSDagInst(q, bb);
            return;
        case ir::Opcode::Table::SDag:
            ir::UnaryInst::CreateSInst(q, bb);
            return;
        case ir::Opcode::Table::T:
            ir::UnaryInst::CreateTDagInst(q, bb);
            return;
        case ir::Opcode::Table::TDag:
            ir::UnaryInst::CreateTInst(q, bb);
            return;
        default:
            assert(0 && "unreachable");
    }
}
void InsertAdjointOfBinary(const ir::BinaryInst* inst, ir::BasicBlock* bb) {
    const auto q0 = inst->GetQubit0();
    const auto q1 = inst->GetQubit1();
    switch (inst->GetOpcode().GetCode()) {
        case ir::Opcode::Table::CX:
            ir::BinaryInst::CreateCXInst(q0, q1, bb);
            return;
        case ir::Opcode::Table::CY:
            ir::BinaryInst::CreateCYInst(q0, q1, bb);
            return;
        case ir::Opcode::Table::CZ:
            ir::BinaryInst::CreateCZInst(q0, q1, bb);
            return;
        default:
            assert(0 && "unreachable");
    }
}
void InsertAdjointOfTernary(const ir::TernaryInst* inst, ir::BasicBlock* bb) {
    const auto q0 = inst->GetQubit0();
    const auto q1 = inst->GetQubit1();
    const auto q2 = inst->GetQubit2();
    switch (inst->GetOpcode().GetCode()) {
        case ir::Opcode::Table::CCX:
            ir::TernaryInst::CreateCCXInst(q0, q1, q2, bb);
            return;
        case ir::Opcode::Table::CCY:
            ir::TernaryInst::CreateCCYInst(q0, q1, q2, bb);
            return;
        case ir::Opcode::Table::CCZ:
            ir::TernaryInst::CreateCCZInst(q0, q1, q2, bb);
            return;
        default:
            assert(0 && "unreachable");
    }
}
void InsertAdjointOfMultiControl(const ir::MultiControlInst* inst, ir::BasicBlock* bb) {
    const auto t = inst->GetTarget();
    const auto& c = inst->GetControl();
    switch (inst->GetOpcode().GetCode()) {
        case ir::Opcode::Table::MCX:
            ir::MultiControlInst::CreateMCXInst(t, c, bb);
            return;
        default:
            assert(0 && "unreachable");
    }
}
Circuit* CreateAdjointCircuit(CircuitBuilder* builder, const Circuit* circuit) {
    const auto* ir = circuit->GetIR();
    if (ir->GetNumBBs() > 1) {
        throw std::runtime_error(
                "Frontend error: Currently adjoint of circuit with more than single BB is not "
                "supported."
        );
    }

    auto* adjoint = builder->CreateCircuit(AdjointPrefix + ir->GetName().data());
    adjoint->GetMutArgument() = circuit->GetArgument();
    builder->BeginCircuitDefinition(adjoint);
    for (auto itr = ir->GetEntryBB()->rbegin(); itr != ir->GetEntryBB()->rend(); ++itr) {
        if (itr->IsMeasurement()) {
            throw std::runtime_error(
                    "Frontend error: Cannot create adjoint measurement instruction."
            );
        } else if (itr->IsUnary()) {
            if (itr->IsParametrizedRotation()) {
                const auto* inst = Cast<ir::ParametrizedRotationInst>(itr.GetPointer());
                InsertAdjointOfParametrizedRotation(inst, builder->GetInsertionPoint());
            } else {
                const auto* inst = Cast<ir::UnaryInst>(itr.GetPointer());
                InsertAdjointOfUnary(inst, builder->GetInsertionPoint());
            }
        } else if (itr->IsBinary()) {
            const auto* inst = Cast<ir::BinaryInst>(itr.GetPointer());
            InsertAdjointOfBinary(inst, builder->GetInsertionPoint());
        } else if (itr->IsTernary()) {
            const auto* inst = Cast<ir::TernaryInst>(itr.GetPointer());
            InsertAdjointOfTernary(inst, builder->GetInsertionPoint());
        } else if (itr->IsMultiControl()) {
            const auto* inst = Cast<ir::MultiControlInst>(itr.GetPointer());
            InsertAdjointOfMultiControl(inst, builder->GetInsertionPoint());
        } else if (itr->IsStateInvariant()) {
            if (itr->GetOpcode() == ir::Opcode::Table::GlobalPhase) {
                const auto* inst = Cast<ir::GlobalPhaseInst>(itr.GetPointer());
                auto theta = inst->GetTheta();
                theta.value *= -1;
                ir::GlobalPhaseInst::Create(theta, builder->GetInsertionPoint());
            }
        } else if (itr->IsCall()) {
            const auto* inst = Cast<ir::CallInst>(itr.GetPointer());
            auto* to_adjoint = ir::CallInst::Create(
                    inst->GetCallee(),
                    inst->GetOperate(),
                    inst->GetInput(),
                    inst->GetOutput(),
                    builder->GetInsertionPoint()
            );
            ReplaceWithAdjoint(builder, to_adjoint);
        } else if (itr->IsClassical()) {
            throw std::runtime_error(
                    "Frontend Error: Cannot create controlled classical instruction."
            );
        } else if (itr->IsRandom()) {
            throw std::runtime_error(
                    "Frontend Error: Cannot create controlled random instruction."
            );
        } else if (itr->IsTerminator()) {
            if (itr->GetOpcode() != ir::Opcode::Table::Return) {
                throw std::runtime_error(
                        "Frontend error: Cannot create adjoint branch/switch instruction."
                );
            }
            // Do nothing
        } else if (itr->IsOptHint()) {
            switch (itr->GetOpcode().GetCode()) {
                case ir::Opcode::Table::Clean: {
                    const auto* inst = Cast<ir::CleanInst>(itr.GetPointer());
                    ir::CleanInst::Create(inst->GetQubit(), builder->GetInsertionPoint());
                    break;
                }
                case ir::Opcode::Table::CleanProb: {
                    const auto* inst = Cast<ir::CleanProbInst>(itr.GetPointer());
                    ir::CleanProbInst::Create(
                            inst->GetQubit(),
                            inst->GetCleanProb(),
                            builder->GetInsertionPoint()
                    );
                    break;
                }
                case ir::Opcode::Table::DirtyBegin: {
                    const auto* inst = Cast<ir::DirtyInst>(itr.GetPointer());
                    ir::DirtyInst::CreateEnd(inst->GetQubit(), builder->GetInsertionPoint());
                    break;
                }
                case ir::Opcode::Table::DirtyEnd: {
                    const auto* inst = Cast<ir::DirtyInst>(itr.GetPointer());
                    ir::DirtyInst::CreateBegin(inst->GetQubit(), builder->GetInsertionPoint());
                    break;
                }
                default:
                    assert(0 && "unreachable");
            }
        } else {
            assert(0 && "unreachable");
        }
    }
    builder->EndCircuitDefinition(adjoint);
    return adjoint;
}
void ReplaceWithAdjoint(CircuitBuilder* builder, ir::CallInst* inst) {
    auto* circuit = builder->GetCircuitFromIR(inst->GetCallee());
    if (circuit == nullptr) {
        throw std::runtime_error(
                "Frontend error: Currently adjoint of circuit not cached in the builder is not "
                "supported."
        );
    }

    if (circuit->HasAdjoint()) {
        // Use the adjoint circuit user defined
        inst->SetCallee(circuit->GetAdjoint()->GetIR());
    } else {
        // Automatically generate an adjoint circuit
        auto* adjoint = CreateAdjointCircuit(builder, circuit);
        circuit->SetAdjoint(adjoint);
        adjoint->SetAdjoint(circuit);
        inst->SetCallee(adjoint->GetIR());
    }
}

//--------------------------------------------------//
// Functor: controlled
//--------------------------------------------------//

void InsertControlledOfParametrizedRotation(
        const ir::Qubit& /* c */,
        const ir::ParametrizedRotationInst* inst,
        ir::BasicBlock* /* bb */
) {
    throw std::runtime_error(
            fmt::format(
                    "Frontend error: Currently control of {} is not supported.",
                    inst->GetOpcode().ToString()
            )
    );
}
void InsertControlledOfUnary(const ir::Qubit& c, const ir::UnaryInst* inst, ir::BasicBlock* bb) {
    const auto q = IncrementIndex(inst->GetQubit());
    switch (inst->GetOpcode().GetCode()) {
        case ir::Opcode::Table::I:
            ir::UnaryInst::CreateIInst(q, bb);
            return;
        case ir::Opcode::Table::X:
            ir::BinaryInst::CreateCXInst(q, c, bb);
            return;
        case ir::Opcode::Table::Y:
            ir::BinaryInst::CreateCYInst(q, c, bb);
            return;
        case ir::Opcode::Table::Z:
            ir::BinaryInst::CreateCZInst(q, c, bb);
            return;
        case ir::Opcode::Table::H:
        case ir::Opcode::Table::S:
        case ir::Opcode::Table::SDag:
        case ir::Opcode::Table::T:
        case ir::Opcode::Table::TDag:
            throw std::runtime_error(
                    fmt::format(
                            "Frontend error: Currently control of {} is not supported.",
                            inst->GetOpcode().ToString()
                    )
            );
        default:
            assert(0 && "unreachable");
    }
}
void InsertControlledOfBinary(const ir::Qubit& c, const ir::BinaryInst* inst, ir::BasicBlock* bb) {
    const auto q0 = IncrementIndex(inst->GetQubit0());
    const auto q1 = IncrementIndex(inst->GetQubit1());
    switch (inst->GetOpcode().GetCode()) {
        case ir::Opcode::Table::CX:
            ir::TernaryInst::CreateCCXInst(q0, q1, c, bb);
            return;
        case ir::Opcode::Table::CY:
            ir::TernaryInst::CreateCCYInst(q0, q1, c, bb);
            return;
        case ir::Opcode::Table::CZ:
            ir::TernaryInst::CreateCCZInst(q0, q1, c, bb);
            return;
        default:
            assert(0 && "unreachable");
    }
}
void InsertControlledOfTernary(
        const ir::Qubit& c,
        const ir::TernaryInst* inst,
        ir::BasicBlock* bb
) {
    const auto q0 = IncrementIndex(inst->GetQubit0());
    const auto q1 = IncrementIndex(inst->GetQubit1());
    const auto q2 = IncrementIndex(inst->GetQubit2());
    switch (inst->GetOpcode().GetCode()) {
        case ir::Opcode::Table::CCX:
            ir::MultiControlInst::CreateMCXInst(q0, {q1, q2, c}, bb);
            return;
        case ir::Opcode::Table::CCY:
        case ir::Opcode::Table::CCZ:
            throw std::runtime_error(
                    fmt::format(
                            "Frontend error: Currently control of {} is not supported",
                            inst->GetOpcode().ToString()
                    )
            );
        default:
            assert(0 && "unreachable");
    }
}
void InsertControlledOfMultiControl(
        const ir::Qubit& c,
        const ir::MultiControlInst* inst,
        ir::BasicBlock* bb
) {
    const auto t = IncrementIndex(inst->GetTarget());
    auto cs = IncrementIndex(inst->GetControl());
    cs.emplace_back(c);
    switch (inst->GetOpcode().GetCode()) {
        case ir::Opcode::Table::MCX:
            ir::MultiControlInst::CreateMCXInst(t, cs, bb);
            return;
        default:
            assert(0 && "unreachable");
    }
}
std::string GenerateControlQubitName(const Circuit* circuit) {
    const auto* default_name = "__control__";
    if (!circuit->GetArgument().Contains(default_name)) {
        return default_name;
    }

    for (auto i = 0; i < 1e5; ++i) {
        const auto custom_name = fmt::format("{}_{}", default_name, i);
        if (!circuit->GetArgument().Contains(custom_name)) {
            return custom_name;
        }
    }

    throw std::runtime_error("Failed to generate name of the control qubit.");
}
Circuit* CreateControlledCircuit(const Qubit& c, const Circuit* circuit) {
    auto* builder = c.GetBuilder();
    const auto* ir = circuit->GetIR();
    if (ir->GetNumBBs() > 1) {
        throw std::runtime_error(
                "Frontend error: Currently control of circuit with more than single BB is "
                "supported."
        );
    }
    auto* controlled = builder->CreateCircuit(ControlledPrefix + ir->GetName().data());
    // Set argument
    {
        auto& controlled_argument = controlled->GetMutArgument();
        controlled_argument.Add(
                GenerateControlQubitName(circuit),
                Circuit::Type::Qubit,
                1,
                Circuit::Attribute::Operate
        );
        const auto& argument = circuit->GetArgument();
        for (const auto& [name, type, size, attr] : argument) {
            controlled_argument.Add(name, type, size, attr);
        }
    }
    builder->BeginCircuitDefinition(controlled);
    for (auto itr = ir->GetEntryBB()->begin(); itr != ir->GetEntryBB()->end(); ++itr) {
        if (itr->IsMeasurement()) {
            throw std::runtime_error(
                    "Frontend error: Cannot create control measurement instruction."
            );
        } else if (itr->IsUnary()) {
            if (itr->IsParametrizedRotation()) {
                const auto* inst = Cast<ir::ParametrizedRotationInst>(itr.GetPointer());
                InsertControlledOfParametrizedRotation(GetQ(c), inst, builder->GetInsertionPoint());
            } else {
                const auto* inst = Cast<ir::UnaryInst>(itr.GetPointer());
                InsertControlledOfUnary(GetQ(c), inst, builder->GetInsertionPoint());
            }
        } else if (itr->IsBinary()) {
            const auto* inst = Cast<ir::BinaryInst>(itr.GetPointer());
            InsertControlledOfBinary(GetQ(c), inst, builder->GetInsertionPoint());
        } else if (itr->IsTernary()) {
            const auto* inst = Cast<ir::TernaryInst>(itr.GetPointer());
            InsertControlledOfTernary(GetQ(c), inst, builder->GetInsertionPoint());
        } else if (itr->IsMultiControl()) {
            const auto* inst = Cast<ir::MultiControlInst>(itr.GetPointer());
            InsertControlledOfMultiControl(GetQ(c), inst, builder->GetInsertionPoint());
        } else if (itr->IsStateInvariant()) {
            throw std::runtime_error(
                    "Frontend Error: Cannot create control GlobalPhase instruction."
            );
        } else if (itr->IsCall()) {
            const auto* inst = Cast<ir::CallInst>(itr.GetPointer());
            auto* to_control = ir::CallInst::Create(
                    inst->GetCallee(),
                    IncrementIndex(inst->GetOperate()),
                    inst->GetInput(),
                    inst->GetOutput(),
                    builder->GetInsertionPoint()
            );
            ReplaceWithControlled(c, to_control);
        } else if (itr->IsClassical()) {
            throw std::runtime_error(
                    "Frontend Error: Cannot create control classical instruction."
            );
        } else if (itr->IsRandom()) {
            throw std::runtime_error("Frontend Error: Cannot create control random instruction.");
        } else if (itr->IsTerminator()) {
            if (itr->GetOpcode() != ir::Opcode::Table::Return) {
                throw std::runtime_error(
                        "Frontend Error: Cannot create control branch/switch instruction."
                );
            }
            // Do nothing
        } else if (itr->IsOptHint()) {
            // Do nothing
            // std::cerr << "skip optimization hint" << std::endl;
        } else {
            assert(0 && "unreachable");
        }
    }
    builder->EndCircuitDefinition(controlled);
    return controlled;
}
void ReplaceWithControlled(const Qubit& c, ir::CallInst* inst) {
    auto* builder = c.GetBuilder();
    auto* circuit = builder->GetCircuitFromIR(inst->GetCallee());
    if (circuit == nullptr) {
        throw std::runtime_error(
                "Frontend error: Currently control of circuit not cached in the builder is not "
                "supported."
        );
    }

    // Insert control qubit at front
    auto& operate = inst->GetMutOperate();
    operate.insert(operate.begin(), ir::Qubit{c.GetId()});

    if (circuit->HasControlled()) {
        // Use the controlled circuit user defined
        inst->SetCallee(circuit->GetControlled()->GetIR());
    } else {
        // Automatically generate a controlled circuit
        auto copy = c;
        copy.SetId(0);
        auto* controlled = CreateControlledCircuit(copy, circuit);
        circuit->SetControlled(controlled);
        inst->SetCallee(controlled->GetIR());
    }
}
}  // namespace qret::frontend::impl
