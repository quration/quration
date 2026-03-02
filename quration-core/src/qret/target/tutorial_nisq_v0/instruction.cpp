/**
 * @file qret/target/tutorial_nisq_v0/instruction.cpp
 * @brief Instruction set definition of TUTORIAL_NISQ_V0.
 */

#include "qret/target/tutorial_nisq_v0/instruction.h"

#include <fmt/format.h>

#include <stdexcept>

namespace qret::tutorial_nisq_v0 {
namespace {
[[nodiscard]] bool IsUnaryOpcode(const TutorialNisqV0Opcode opcode) {
    switch (opcode) {
        case TutorialNisqV0Opcode::X:
        case TutorialNisqV0Opcode::Y:
        case TutorialNisqV0Opcode::Z:
        case TutorialNisqV0Opcode::H:
        case TutorialNisqV0Opcode::S:
        case TutorialNisqV0Opcode::SDag:
        case TutorialNisqV0Opcode::T:
        case TutorialNisqV0Opcode::TDag:
            return true;
        default:
            return false;
    }
}

[[nodiscard]] bool IsRotationOpcode(const TutorialNisqV0Opcode opcode) {
    return opcode == TutorialNisqV0Opcode::RX || opcode == TutorialNisqV0Opcode::RY
            || opcode == TutorialNisqV0Opcode::RZ;
}

[[nodiscard]] bool IsBinaryOpcode(const TutorialNisqV0Opcode opcode) {
    return opcode == TutorialNisqV0Opcode::CX || opcode == TutorialNisqV0Opcode::CY
            || opcode == TutorialNisqV0Opcode::CZ;
}

[[nodiscard]] bool IsMeasurementOpcode(const TutorialNisqV0Opcode opcode) {
    return opcode == TutorialNisqV0Opcode::MX || opcode == TutorialNisqV0Opcode::MY
            || opcode == TutorialNisqV0Opcode::MZ;
}
}  // namespace

std::string ToString(const TutorialNisqV0Opcode opcode) {
    switch (opcode) {
        case TutorialNisqV0Opcode::X:
            return "X";
        case TutorialNisqV0Opcode::Y:
            return "Y";
        case TutorialNisqV0Opcode::Z:
            return "Z";
        case TutorialNisqV0Opcode::H:
            return "H";
        case TutorialNisqV0Opcode::S:
            return "S";
        case TutorialNisqV0Opcode::SDag:
            return "SDAG";
        case TutorialNisqV0Opcode::T:
            return "T";
        case TutorialNisqV0Opcode::TDag:
            return "TDAG";
        case TutorialNisqV0Opcode::RX:
            return "RX";
        case TutorialNisqV0Opcode::RY:
            return "RY";
        case TutorialNisqV0Opcode::RZ:
            return "RZ";
        case TutorialNisqV0Opcode::CX:
            return "CX";
        case TutorialNisqV0Opcode::CY:
            return "CY";
        case TutorialNisqV0Opcode::CZ:
            return "CZ";
        case TutorialNisqV0Opcode::MX:
            return "MX";
        case TutorialNisqV0Opcode::MY:
            return "MY";
        case TutorialNisqV0Opcode::MZ:
            return "MZ";
        default:
            throw std::runtime_error("unknown tutorial_nisq_v0 opcode");
    }
}

TutorialUnaryInstruction::TutorialUnaryInstruction(
        const TutorialNisqV0Opcode opcode,
        const qret::ir::Qubit qubit
)
    : opcode_{opcode}
    , qubit_{qubit} {
    if (!IsUnaryOpcode(opcode)) {
        throw std::runtime_error("invalid unary opcode for TutorialUnaryInstruction");
    }
}

std::string TutorialUnaryInstruction::ToString() const {
    return fmt::format("{} q{}", tutorial_nisq_v0::ToString(opcode_), qubit_.id);
}

TutorialRotationInstruction::TutorialRotationInstruction(
        const TutorialNisqV0Opcode opcode,
        const qret::ir::Qubit qubit,
        const double theta,
        const double precision
)
    : opcode_{opcode}
    , qubit_{qubit}
    , theta_{theta}
    , precision_{precision} {
    if (!IsRotationOpcode(opcode)) {
        throw std::runtime_error("invalid rotation opcode for TutorialRotationInstruction");
    }
}

std::string TutorialRotationInstruction::ToString() const {
    return fmt::format(
            "{} q{} theta={} precision={}",
            tutorial_nisq_v0::ToString(opcode_),
            qubit_.id,
            theta_,
            precision_
    );
}

TutorialBinaryInstruction::TutorialBinaryInstruction(
        const TutorialNisqV0Opcode opcode,
        const qret::ir::Qubit qubit0,
        const qret::ir::Qubit qubit1
)
    : opcode_{opcode}
    , qubit0_{qubit0}
    , qubit1_{qubit1} {
    if (!IsBinaryOpcode(opcode)) {
        throw std::runtime_error("invalid binary opcode for TutorialBinaryInstruction");
    }
}

std::string TutorialBinaryInstruction::ToString() const {
    return fmt::format("{} q{} q{}", tutorial_nisq_v0::ToString(opcode_), qubit0_.id, qubit1_.id);
}

TutorialMeasurementInstruction::TutorialMeasurementInstruction(
        const TutorialNisqV0Opcode opcode,
        const qret::ir::Qubit qubit,
        const qret::ir::Register reg
)
    : opcode_{opcode}
    , qubit_{qubit}
    , reg_{reg} {
    if (!IsMeasurementOpcode(opcode)) {
        throw std::runtime_error("invalid measurement opcode for TutorialMeasurementInstruction");
    }
}

std::string TutorialMeasurementInstruction::ToString() const {
    return fmt::format("{} q{} -> r{}", tutorial_nisq_v0::ToString(opcode_), qubit_.id, reg_.id);
}
}  // namespace qret::tutorial_nisq_v0
