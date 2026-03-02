/**
 * @file qret/target/tutorial_nisq_v0/lowering.cpp
 * @brief Lower qret IR into TUTORIAL_NISQ_V0 machine instructions.
 */

#include "qret/target/tutorial_nisq_v0/lowering.h"

#include <iostream>
#include <memory>
#include <stdexcept>

#include "qret/base/cast.h"
#include "qret/ir/function.h"
#include "qret/ir/instruction.h"
#include "qret/ir/instructions.h"
#include "qret/target/tutorial_nisq_v0/instruction.h"

namespace qret::tutorial_nisq_v0 {
namespace {
TutorialNisqV0Opcode FromIRUnaryOpcode(const qret::ir::Opcode::Table code) {
    using QOpcode = qret::ir::Opcode::Table;
    switch (code) {
        case QOpcode::X:
            return TutorialNisqV0Opcode::X;
        case QOpcode::Y:
            return TutorialNisqV0Opcode::Y;
        case QOpcode::Z:
            return TutorialNisqV0Opcode::Z;
        case QOpcode::H:
            return TutorialNisqV0Opcode::H;
        case QOpcode::S:
            return TutorialNisqV0Opcode::S;
        case QOpcode::SDag:
            return TutorialNisqV0Opcode::SDag;
        case QOpcode::T:
            return TutorialNisqV0Opcode::T;
        case QOpcode::TDag:
            return TutorialNisqV0Opcode::TDag;
        default:
            throw std::runtime_error("unsupported unary opcode for tutorial_nisq_v0");
    }
}

TutorialNisqV0Opcode FromIRRotationOpcode(const qret::ir::Opcode::Table code) {
    using QOpcode = qret::ir::Opcode::Table;
    switch (code) {
        case QOpcode::RX:
            return TutorialNisqV0Opcode::RX;
        case QOpcode::RY:
            return TutorialNisqV0Opcode::RY;
        case QOpcode::RZ:
            return TutorialNisqV0Opcode::RZ;
        default:
            throw std::runtime_error("unsupported rotation opcode for tutorial_nisq_v0");
    }
}

TutorialNisqV0Opcode FromIRBinaryOpcode(const qret::ir::Opcode::Table code) {
    using QOpcode = qret::ir::Opcode::Table;
    switch (code) {
        case QOpcode::CX:
            return TutorialNisqV0Opcode::CX;
        case QOpcode::CY:
            return TutorialNisqV0Opcode::CY;
        case QOpcode::CZ:
            return TutorialNisqV0Opcode::CZ;
        default:
            throw std::runtime_error("unsupported binary opcode for tutorial_nisq_v0");
    }
}

[[nodiscard]] bool LowerInstruction(
        const qret::ir::Instruction& inst,
        qret::MachineBasicBlock& mbb,
        std::string_view bb_name,
        bool* saw_return
) {
    const auto code = inst.GetOpcode().GetCode();

    if (*saw_return) {
        std::cerr << "instruction appears after Return in basic block '" << bb_name << "'"
                  << std::endl;
        return false;
    }
    if (inst.IsOptHint()) {
        // tutorial_nisq_v0 lowering treats optimization hints as no-op.
        return true;
    }

    using QOpcode = qret::ir::Opcode::Table;
    switch (code) {
        case QOpcode::X:
        case QOpcode::Y:
        case QOpcode::Z:
        case QOpcode::H:
        case QOpcode::S:
        case QOpcode::SDag:
        case QOpcode::T:
        case QOpcode::TDag: {
            const auto* i = qret::Cast<qret::ir::UnaryInst>(&inst);
            mbb.EmplaceBack(
                    std::unique_ptr<qret::MachineInstruction>(
                            new TutorialUnaryInstruction(FromIRUnaryOpcode(code), i->GetQubit())
                    )
            );
            return true;
        }
        case QOpcode::RX:
        case QOpcode::RY:
        case QOpcode::RZ: {
            const auto* i = qret::Cast<qret::ir::ParametrizedRotationInst>(&inst);
            mbb.EmplaceBack(
                    std::unique_ptr<qret::MachineInstruction>(new TutorialRotationInstruction(
                            FromIRRotationOpcode(code),
                            i->GetQubit(),
                            i->GetTheta().value,
                            i->GetTheta().precision
                    ))
            );
            return true;
        }
        case QOpcode::CX:
        case QOpcode::CY:
        case QOpcode::CZ: {
            const auto* i = qret::Cast<qret::ir::BinaryInst>(&inst);
            mbb.EmplaceBack(
                    std::unique_ptr<qret::MachineInstruction>(new TutorialBinaryInstruction(
                            FromIRBinaryOpcode(code),
                            i->GetQubit0(),
                            i->GetQubit1()
                    ))
            );
            return true;
        }
        case QOpcode::Measurement: {
            const auto* i = qret::Cast<qret::ir::MeasurementInst>(&inst);
            mbb.EmplaceBack(
                    std::unique_ptr<qret::MachineInstruction>(new TutorialMeasurementInstruction(
                            TutorialNisqV0Opcode::MZ,
                            i->GetQubit(),
                            i->GetRegister()
                    ))
            );
            return true;
        }
        case QOpcode::Return:
            *saw_return = true;
            return true;
        default:
            break;
    }

    std::cerr << "unsupported opcode for tutorial_nisq_v0 lowering: " << inst.GetOpcode().ToString()
              << " (basic block='" << bb_name << "')" << std::endl;
    return false;
}
}  // namespace

bool TutorialNisqV0Lowering::RunOnMachineFunction(qret::MachineFunction& mf) const {
    if (!mf.HasIR() || mf.GetIR() == nullptr) {
        std::cerr << "tutorial_nisq_v0 lowering requires an IR function" << std::endl;
        return false;
    }

    const auto& func = *mf.GetIR();
    if (func.GetNumBBs() != 1) {
        std::cerr << "tutorial_nisq_v0 requires exactly one basic block, but found "
                  << func.GetNumBBs() << std::endl;
        return false;
    }

    mf.Clear();
    auto& mbb = mf.AddBlock();

    for (const auto& bb : func) {
        bool saw_return = false;
        for (const auto& inst : bb) {
            if (!LowerInstruction(inst, mbb, bb.GetName(), &saw_return)) {
                return false;
            }
        }
    }

    return true;
}
}  // namespace qret::tutorial_nisq_v0
