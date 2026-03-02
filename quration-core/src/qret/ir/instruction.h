/**
 * @file qret/ir/instruction.h
 * @brief Opcode and base instruction class.
 */

#ifndef QRET_IR_INSTRUCTION_H
#define QRET_IR_INSTRUCTION_H

#include <concepts>
#include <cstdint>
#include <list>
#include <memory>
#include <ostream>
#include <string_view>

#include "qret/base/iterator.h"
#include "qret/ir/metadata.h"
#include "qret/ir/value.h"
#include "qret/qret_export.h"

namespace qret::ir {
// Forward declarations
class Module;
class Function;
class BasicBlock;

/**
 * @brief Opcode class.
 */
class QRET_EXPORT Opcode {
public:
    enum class Table : std::uint8_t {
        // clang-format off
        // Measurement
        MeasurementOpsBegin = 0,
        Measurement,
        MeasurementOpsEnd,

        // Unary
        UnaryOpsBegin,
        I, X, Y, Z, H, S, SDag, T, TDag, RX, RY, RZ,
        UnaryOpsEnd,

        // Binary
        BinaryOpsBegin,
        CX, CY, CZ,
        BinaryOpsEnd,

        // Ternary
        TernaryOpsBegin,
        CCX, CCY, CCZ,
        TernaryOpsEnd,

        // Multi control
        MultiControlOpsBegin,
        MCX,
        MultiControlOpsEnd,

        // Call
        CallOpsBegin,
        Call,
        CallOpsEnd,

        // State invariant
        StateInvariantOpsBegin,
        GlobalPhase,
        StateInvariantOpsEnd,

        // Classical
        ClassicalOpsBegin,
        CallCF,
        ClassicalOpsEnd,

        // Generate random number in classical way
        RandomOpsBegin,
        DiscreteDistribution,
        RandomOpsEnd,

        // Terminator
        TerminatorOpsBegin,
        Branch, Switch, Return,
        TerminatorOpsEnd,

        // Optimization hint
        OptHintOpsBegin,
        Clean, CleanProb, DirtyBegin, DirtyEnd,
        OptHintOpsEnd,
        // clang-format on
    };

    static Opcode FromString(std::string_view opcode);

    static constexpr bool IsMeasurement(Opcode o) noexcept {
        return InRange(o.x_, Table::MeasurementOpsBegin, Table::MeasurementOpsEnd);
    }
    static constexpr bool IsUnary(Opcode o) noexcept {
        return InRange(o.x_, Table::UnaryOpsBegin, Table::UnaryOpsEnd);
    }
    static constexpr bool IsParametrizedRotation(Opcode o) noexcept {
        return o.x_ == Table::RX || o.x_ == Table::RY || o.x_ == Table::RZ;
    }
    static constexpr bool IsBinary(Opcode o) noexcept {
        return InRange(o.x_, Table::BinaryOpsBegin, Table::BinaryOpsEnd);
    }
    static constexpr bool IsTernary(Opcode o) noexcept {
        return InRange(o.x_, Table::TernaryOpsBegin, Table::TernaryOpsEnd);
    }
    static constexpr bool IsMultiControl(Opcode o) noexcept {
        return InRange(o.x_, Table::MultiControlOpsBegin, Table::MultiControlOpsEnd);
    }
    static constexpr bool IsStateInvariant(Opcode o) noexcept {
        return InRange(o.x_, Table::StateInvariantOpsBegin, Table::StateInvariantOpsEnd);
    }
    static constexpr bool IsCall(Opcode o) noexcept {
        return InRange(o.x_, Table::CallOpsBegin, Table::CallOpsEnd);
    }
    static constexpr bool IsClassical(Opcode o) noexcept {
        return InRange(o.x_, Table::ClassicalOpsBegin, Table::ClassicalOpsEnd);
    }
    static constexpr bool IsRandom(Opcode o) noexcept {
        return InRange(o.x_, Table::RandomOpsBegin, Table::RandomOpsEnd);
    }
    static constexpr bool IsTerminator(Opcode o) noexcept {
        return InRange(o.x_, Table::TerminatorOpsBegin, Table::TerminatorOpsEnd);
    }
    static constexpr bool IsOptHint(Opcode o) noexcept {
        return InRange(o.x_, Table::OptHintOpsBegin, Table::OptHintOpsEnd);
    }

    static constexpr bool IsGate(Opcode o) noexcept {
        return IsUnary(o) || IsBinary(o) || IsTernary(o) || IsMultiControl(o);
    }
    static constexpr bool IsClifford(Opcode o) noexcept {
        switch (o.x_) {
            case Table::I:
            case Table::X:
            case Table::Y:
            case Table::Z:
            case Table::H:
            case Table::S:
            case Table::SDag:
            case Table::CX:
            case Table::CY:
            case Table::CZ:
                return true;
            default:
                return false;
        }
    }

    constexpr Opcode(Table x)  // NOLINT
        : x_{x} {}

    [[nodiscard]] constexpr Table GetCode() const noexcept {
        return x_;
    }

    void Print(std::ostream& out) const;
    [[nodiscard]] std::string ToString() const;

    [[nodiscard]] constexpr bool IsMeasurement() const noexcept {
        return IsMeasurement(*this);
    }
    [[nodiscard]] constexpr bool IsUnary() const noexcept {
        return IsUnary(*this);
    }
    [[nodiscard]] constexpr bool IsParametrizedRotation() const noexcept {
        return IsParametrizedRotation(*this);
    }
    [[nodiscard]] constexpr bool IsBinary() const noexcept {
        return IsBinary(*this);
    }
    [[nodiscard]] constexpr bool IsTernary() const noexcept {
        return IsTernary(*this);
    }
    [[nodiscard]] constexpr bool IsMultiControl() const noexcept {
        return IsMultiControl(*this);
    }
    [[nodiscard]] constexpr bool IsStateInvariant() const noexcept {
        return IsStateInvariant(*this);
    }
    [[nodiscard]] constexpr bool IsCall() const noexcept {
        return IsCall(*this);
    }
    [[nodiscard]] constexpr bool IsClassical() const noexcept {
        return IsClassical(*this);
    }
    [[nodiscard]] constexpr bool IsRandom() const noexcept {
        return IsRandom(*this);
    }
    [[nodiscard]] constexpr bool IsTerminator() const noexcept {
        return IsTerminator(*this);
    }
    [[nodiscard]] constexpr bool IsOptHint() const noexcept {
        return IsOptHint(*this);
    }

    [[nodiscard]] constexpr bool IsGate() const noexcept {
        return IsGate(*this);
    }
    [[nodiscard]] constexpr bool IsClifford() const noexcept {
        return IsClifford(*this);
    }

private:
    template <class E>
    static constexpr bool InRange(E x, E begin, E end) noexcept {
        using U = std::underlying_type_t<E>;
        return static_cast<U>(begin) < static_cast<U>(x) && static_cast<U>(x) < static_cast<U>(end);
    }

    Table x_;
};

constexpr bool operator==(const Opcode& lhs, const Opcode& rhs) {
    return lhs.GetCode() == rhs.GetCode();
}
constexpr bool operator==(const Opcode& lhs, const Opcode::Table& rhs) {
    return lhs.GetCode() == rhs;
}
constexpr bool operator==(const Opcode::Table& lhs, const Opcode& rhs) {
    return lhs == rhs.GetCode();
}
constexpr bool operator!=(const Opcode& lhs, const Opcode& rhs) {
    return !(lhs == rhs);
}
constexpr bool operator!=(const Opcode& lhs, const Opcode::Table& rhs) {
    return !(lhs == rhs);
}
constexpr bool operator!=(const Opcode::Table& lhs, const Opcode& rhs) {
    return !(lhs == rhs);
}

template <class T>
concept DerivedFromInstruction = std::derived_from<T, class Instruction>;

/**
 * @brief Parent class of instructions in qret IR.
 * @details The owner of instructions is a basic block.
 */
class QRET_EXPORT Instruction {
public:
    Instruction(const Instruction&) = delete;
    Instruction& operator=(const Instruction&) = delete;
    virtual ~Instruction() = default;

    bool HasParent() const {
        return parent_ != nullptr;
    }

    /**
     * @brief Get the basic block owning this instruction, or nullptr if the instruction does not
     * have a basic block.
     */
    const BasicBlock* GetParent() const {
        return parent_;
    }
    BasicBlock* GetParent() {
        return parent_;
    }

    /**
     * @brief Get the func this instruction belongs to, or nullptr if no such func exists.
     */
    const Function* GetFunction() const;
    Function* GetFunction() {
        return const_cast<Function*>(static_cast<const Instruction*>(this)->GetFunction());
    }

    /**
     * @brief Get the module owning the func this instruction belongs to, or nullptr if no such
     * module exists.
     */
    const Module* GetModule() const;
    Module* GetModule() {
        return const_cast<Module*>(static_cast<const Instruction*>(this)->GetModule());
    }

    /**
     * @brief Get a pointer to the next instruction in the same basic block, or nullptr if no such
     * instruction exists.
     */
    [[nodiscard]] Instruction* GetNextInstruction() const;

    // Subclass specification
    [[nodiscard]] Opcode GetOpcode() const {
        return opcode_;
    }
    [[nodiscard]] bool IsMeasurement() const {
        return opcode_.IsMeasurement();
    }
    [[nodiscard]] bool IsUnary() const {
        return opcode_.IsUnary();
    }
    [[nodiscard]] bool IsParametrizedRotation() const {
        return opcode_.IsParametrizedRotation();
    }
    [[nodiscard]] bool IsBinary() const {
        return opcode_.IsBinary();
    }
    [[nodiscard]] bool IsTernary() const {
        return opcode_.IsTernary();
    }
    [[nodiscard]] bool IsMultiControl() const {
        return opcode_.IsMultiControl();
    }
    [[nodiscard]] bool IsStateInvariant() const {
        return opcode_.IsStateInvariant();
    }
    [[nodiscard]] bool IsCall() const {
        return opcode_.IsCall();
    }
    [[nodiscard]] bool IsClassical() const {
        return opcode_.IsClassical();
    }
    [[nodiscard]] bool IsRandom() const {
        return opcode_.IsRandom();
    }
    [[nodiscard]] bool IsTerminator() const {
        return opcode_.IsTerminator();
    }
    [[nodiscard]] bool IsOptHint() const {
        return opcode_.IsOptHint();
    }

    [[nodiscard]] bool IsGate() const {
        return opcode_.IsGate();
    }
    [[nodiscard]] bool IsClifford() const {
        return opcode_.IsClifford();
    }

    // Metadata manipulation
    using md_iterator = ListIterator<Metadata, false>;
    using md_const_iterator = ListIterator<Metadata, true>;

    bool HasMetadata() const {
        return !md_list_.empty();
    }
    bool HasMetadata(const MDKind& kind) const {
        return GetMetadata(kind) != nullptr;
    }
    const Metadata* GetMetadata(const MDKind& kind) const;

    Instruction* AddMDAnnotation(std::string_view annot);
    Instruction* AddMDQubit(const Qubit& q);
    Instruction* AddMDRegister(const Register& q);
    Instruction* MarkAsBreakPoint(std::string_view name);
    Instruction* AddLocation(std::string_view file, std::uint32_t line);
    void DeleteMetadata(md_iterator itr) {
        md_list_.erase(itr.GetRaw());
    }

    md_iterator md_begin() {
        return md_iterator{md_list_.begin()};
    }  // NOLINT
    md_const_iterator md_begin() const {
        return md_const_iterator{md_list_.cbegin()};
    }  // NOLINT
    md_const_iterator md_cbegin() {
        return md_const_iterator{md_list_.cbegin()};
    }  // NOLINT
    md_iterator md_end() {
        return md_iterator{md_list_.end()};
    }  // NOLINT
    md_const_iterator md_end() const {
        return md_const_iterator{md_list_.cend()};
    }  // NOLINT
    md_const_iterator md_cend() {
        return md_const_iterator{md_list_.cend()};
    }  // NOLINT

    // Helper methods

    // Print methods

    /**
     * @brief Print the instruction to a stream \p out.
     * @details this method is a overridable hook.
     */
    virtual void Print(std::ostream& out) const {
        GetOpcode().Print(out);
    }
    /**
     * @brief Print the metadata of this instruction to a stream \p out.
     */
    void PrintMetadata(std::ostream& out) const;

    /**
     * @brief Return if the given object is the instance of this class.
     * @details this is used from LLVM-style cast functions defined in base/cast.h
     */
#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From*) {
        return true;
    }

protected:
    explicit Instruction(Opcode opcode)
        : opcode_{opcode} {}

private:
    friend BasicBlock;
    friend QRET_EXPORT std::unique_ptr<Instruction> RemoveFromParent(Instruction* inst);
    friend QRET_EXPORT void
    InsertBefore(std::unique_ptr<Instruction>&& inst, Instruction* insert_pos);
    friend QRET_EXPORT void
    InsertAfter(std::unique_ptr<Instruction>&& inst, Instruction* insert_pos);
    friend QRET_EXPORT void InsertAtEnd(std::unique_ptr<Instruction>&& inst, BasicBlock* bb);

    void SetParent(BasicBlock* parent) {
        parent_ = parent;
    }
    Instruction* AddMetadata(std::unique_ptr<Metadata>&& md) {
        md_list_.emplace_back(std::move(md));
        return this;
    }

    const Opcode opcode_;
    BasicBlock* parent_ = nullptr;
    std::list<std::unique_ptr<Metadata>> md_list_;  //!< list of metadata
};
/**
 * @brief Unlink this instruction from its current basic block, but don't delete it.
 */
QRET_EXPORT std::unique_ptr<Instruction> RemoveFromParent(Instruction* inst);
/**
 * @brief Unlink this instruction from its current basic block and delete it.
 */
QRET_EXPORT void EraseFromParent(Instruction* inst);
/**
 * @brief Insert an unlinked this instruction into a basic block right before the
 * specified iterator.
 */
QRET_EXPORT void InsertBefore(std::unique_ptr<Instruction>&& inst, Instruction* insert_pos);
/**
 * @brief Insert an unlinked this instruction into a basic block right after the
 * specified iterator.
 */
QRET_EXPORT void InsertAfter(std::unique_ptr<Instruction>&& inst, Instruction* insert_pos);
/**
 * @brief Insert an unlinked this instruction into the end of a basic block.
 */
QRET_EXPORT void InsertAtEnd(std::unique_ptr<Instruction>&& inst, BasicBlock* bb);
/**
 * @brief Unlink this instruction from its current basic block and insert it into a basic block
 * right before the specified iterator.
 * @details RemoveFromParent + InsertBefore.
 */
QRET_EXPORT void MoveBefore(Instruction* inst, Instruction* move_pos);
/**
 * @brief Unlink this instruction from its current basic block and insert it into a basic block
 * right after the specified iterator.
 * @details RemoveFromParent + InsertAfter.
 */
QRET_EXPORT void MoveAfter(Instruction* inst, Instruction* move_pos);
}  // namespace qret::ir

#endif  // QRET_IR_INSTRUCTION_H
