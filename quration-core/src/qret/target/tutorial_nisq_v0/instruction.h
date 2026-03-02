/**
 * @file qret/target/tutorial_nisq_v0/instruction.h
 * @brief Instruction set definition of TUTORIAL_NISQ_V0.
 *
 * @details
 * TUTORIAL_NISQ_V0 is a minimal NISQ-oriented machine instruction set for tutorials.
 * It is intentionally small and excludes control-flow and call instructions.
 *
 * Supported instructions (assembly mnemonics):
 * - Single-qubit Clifford/non-Clifford: X, Y, Z, H, S, SDAG, T, TDAG
 *   - Syntax: `<OP> q<id>`
 * - Single-qubit rotations: RX, RY, RZ
 *   - Syntax: `<OP> q<id> theta=<value> precision=<value>`
 * - Two-qubit gates: CX, CY, CZ
 *   - Syntax: `<OP> q<control> q<target>`
 * - Measurements: MX, MY, MZ
 *   - Syntax: `<OP> q<id> -> r<id>`
 *
 * Notes:
 * - This ISA does not include branch, switch, call, or multi-control gates.
 * - Current IR lowering emits MZ from `Measurement` because qret IR measurement is
 *   computational-basis measurement.
 */

#ifndef QRET_TARGET_TUTORIAL_NISQ_V0_INSTRUCTION_H
#define QRET_TARGET_TUTORIAL_NISQ_V0_INSTRUCTION_H

#include <string>

#include "qret/codegen/machine_function.h"
#include "qret/ir/value.h"
#include "qret/qret_export.h"

namespace qret::tutorial_nisq_v0 {
enum class TutorialNisqV0Opcode {
    X,
    Y,
    Z,
    H,
    S,
    SDag,
    T,
    TDag,
    RX,
    RY,
    RZ,
    CX,
    CY,
    CZ,
    MX,
    MY,
    MZ,
};

QRET_EXPORT std::string ToString(TutorialNisqV0Opcode opcode);

class QRET_EXPORT TutorialNisqV0Instruction : public qret::MachineInstruction {
public:
    [[nodiscard]] virtual TutorialNisqV0Opcode GetOpcode() const = 0;
};

class QRET_EXPORT TutorialUnaryInstruction final : public TutorialNisqV0Instruction {
public:
    TutorialUnaryInstruction(TutorialNisqV0Opcode opcode, qret::ir::Qubit qubit);

    [[nodiscard]] TutorialNisqV0Opcode GetOpcode() const override {
        return opcode_;
    }
    [[nodiscard]] const qret::ir::Qubit& GetQubit() const {
        return qubit_;
    }
    [[nodiscard]] std::string ToString() const override;

private:
    TutorialNisqV0Opcode opcode_;
    qret::ir::Qubit qubit_;
};

class QRET_EXPORT TutorialRotationInstruction final : public TutorialNisqV0Instruction {
public:
    TutorialRotationInstruction(
            TutorialNisqV0Opcode opcode,
            qret::ir::Qubit qubit,
            double theta,
            double precision
    );

    [[nodiscard]] TutorialNisqV0Opcode GetOpcode() const override {
        return opcode_;
    }
    [[nodiscard]] const qret::ir::Qubit& GetQubit() const {
        return qubit_;
    }
    [[nodiscard]] double GetTheta() const {
        return theta_;
    }
    [[nodiscard]] double GetPrecision() const {
        return precision_;
    }
    [[nodiscard]] std::string ToString() const override;

private:
    TutorialNisqV0Opcode opcode_;
    qret::ir::Qubit qubit_;
    double theta_;
    double precision_;
};

class QRET_EXPORT TutorialBinaryInstruction final : public TutorialNisqV0Instruction {
public:
    TutorialBinaryInstruction(
            TutorialNisqV0Opcode opcode,
            qret::ir::Qubit qubit0,
            qret::ir::Qubit qubit1
    );

    [[nodiscard]] TutorialNisqV0Opcode GetOpcode() const override {
        return opcode_;
    }
    [[nodiscard]] const qret::ir::Qubit& GetQubit0() const {
        return qubit0_;
    }
    [[nodiscard]] const qret::ir::Qubit& GetQubit1() const {
        return qubit1_;
    }
    [[nodiscard]] std::string ToString() const override;

private:
    TutorialNisqV0Opcode opcode_;
    qret::ir::Qubit qubit0_;
    qret::ir::Qubit qubit1_;
};

class QRET_EXPORT TutorialMeasurementInstruction final : public TutorialNisqV0Instruction {
public:
    TutorialMeasurementInstruction(
            TutorialNisqV0Opcode opcode,
            qret::ir::Qubit qubit,
            qret::ir::Register reg
    );

    [[nodiscard]] TutorialNisqV0Opcode GetOpcode() const override {
        return opcode_;
    }
    [[nodiscard]] const qret::ir::Qubit& GetQubit() const {
        return qubit_;
    }
    [[nodiscard]] const qret::ir::Register& GetRegister() const {
        return reg_;
    }
    [[nodiscard]] std::string ToString() const override;

private:
    TutorialNisqV0Opcode opcode_;
    qret::ir::Qubit qubit_;
    qret::ir::Register reg_;
};
}  // namespace qret::tutorial_nisq_v0

#endif  // QRET_TARGET_TUTORIAL_NISQ_V0_INSTRUCTION_H
