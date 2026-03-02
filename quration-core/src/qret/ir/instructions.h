/**
 * @file qret/ir/instructions.h
 * @brief Define each instruction class.
 */

#ifndef QRET_IR_INSTRUCTIONS_H
#define QRET_IR_INSTRUCTIONS_H

#include <vector>

#include "qret/base/portable_function.h"
#include "qret/base/type.h"
#include "qret/ir/basic_block.h"
#include "qret/ir/instruction.h"
#include "qret/ir/value.h"
#include "qret/qret_export.h"

namespace qret::ir {
class QRET_EXPORT MeasurementInst : public Instruction {
public:
    static MeasurementInst* Create(const Qubit& q, const Register& r, Instruction* insert_before);
    static MeasurementInst* Create(const Qubit& q, const Register& r, BasicBlock* insert_at_end);

    const Qubit& GetQubit() const {
        return q_;
    }
    const Register& GetRegister() const {
        return r_;
    }
    Opcode GetOpcode() const {
        return Instruction::GetOpcode();
    }
    void Print(std::ostream& out) const override;

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->GetOpcode().IsMeasurement();
    }

private:
    MeasurementInst(const Qubit& q, const Register& r)
        : Instruction(Opcode::Table::Measurement)
        , q_{q}
        , r_{r} {}

    Qubit q_;
    Register r_;
};
class QRET_EXPORT UnaryInst : public Instruction {
public:
    static UnaryInst* Create(Opcode opcode, const Qubit& q, Instruction* insert_before);
    static UnaryInst* Create(Opcode opcode, const Qubit& q, BasicBlock* insert_at_end);
    static UnaryInst* CreateIInst(const Qubit& q, Instruction* insert_before) {
        return Create(Opcode::Table::I, q, insert_before);
    }
    static UnaryInst* CreateIInst(const Qubit& q, BasicBlock* insert_at_end) {
        return Create(Opcode::Table::I, q, insert_at_end);
    }
    static UnaryInst* CreateXInst(const Qubit& q, Instruction* insert_before) {
        return Create(Opcode::Table::X, q, insert_before);
    }
    static UnaryInst* CreateXInst(const Qubit& q, BasicBlock* insert_at_end) {
        return Create(Opcode::Table::X, q, insert_at_end);
    }
    static UnaryInst* CreateYInst(const Qubit& q, Instruction* insert_before) {
        return Create(Opcode::Table::Y, q, insert_before);
    }
    static UnaryInst* CreateYInst(const Qubit& q, BasicBlock* insert_at_end) {
        return Create(Opcode::Table::Y, q, insert_at_end);
    }
    static UnaryInst* CreateZInst(const Qubit& q, Instruction* insert_before) {
        return Create(Opcode::Table::Z, q, insert_before);
    }
    static UnaryInst* CreateZInst(const Qubit& q, BasicBlock* insert_at_end) {
        return Create(Opcode::Table::Z, q, insert_at_end);
    }
    static UnaryInst* CreateHInst(const Qubit& q, Instruction* insert_before) {
        return Create(Opcode::Table::H, q, insert_before);
    }
    static UnaryInst* CreateHInst(const Qubit& q, BasicBlock* insert_at_end) {
        return Create(Opcode::Table::H, q, insert_at_end);
    }
    static UnaryInst* CreateSInst(const Qubit& q, Instruction* insert_before) {
        return Create(Opcode::Table::S, q, insert_before);
    }
    static UnaryInst* CreateSInst(const Qubit& q, BasicBlock* insert_at_end) {
        return Create(Opcode::Table::S, q, insert_at_end);
    }
    static UnaryInst* CreateSDagInst(const Qubit& q, Instruction* insert_before) {
        return Create(Opcode::Table::SDag, q, insert_before);
    }
    static UnaryInst* CreateSDagInst(const Qubit& q, BasicBlock* insert_at_end) {
        return Create(Opcode::Table::SDag, q, insert_at_end);
    }
    static UnaryInst* CreateTInst(const Qubit& q, Instruction* insert_before) {
        return Create(Opcode::Table::T, q, insert_before);
    }
    static UnaryInst* CreateTInst(const Qubit& q, BasicBlock* insert_at_end) {
        return Create(Opcode::Table::T, q, insert_at_end);
    }
    static UnaryInst* CreateTDagInst(const Qubit& q, Instruction* insert_before) {
        return Create(Opcode::Table::TDag, q, insert_before);
    }
    static UnaryInst* CreateTDagInst(const Qubit& q, BasicBlock* insert_at_end) {
        return Create(Opcode::Table::TDag, q, insert_at_end);
    }

    const Qubit& GetQubit() const {
        return q_;
    }
    Opcode GetOpcode() const {
        return Instruction::GetOpcode();
    }
    void Print(std::ostream& out) const override;

    bool IsParametrizedRotation() const {
        return GetOpcode().IsParametrizedRotation();
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->GetOpcode().IsUnary();
    }

protected:
    UnaryInst(Opcode opcode, const Qubit& q)
        : Instruction(opcode)
        , q_{q} {}

private:
    Qubit q_;
};
class QRET_EXPORT ParametrizedRotationInst : public UnaryInst {
public:
    static ParametrizedRotationInst* Create(
            Opcode opcode,
            const Qubit& q,
            const FloatWithPrecision& theta,
            Instruction* insert_before
    );
    static ParametrizedRotationInst* Create(
            Opcode opcode,
            const Qubit& q,
            const FloatWithPrecision& theta,
            BasicBlock* insert_at_end
    );
    static ParametrizedRotationInst*
    CreateRXInst(const Qubit& q, const FloatWithPrecision& theta, Instruction* insert_before) {
        return Create(Opcode::Table::RX, q, theta, insert_before);
    }
    static ParametrizedRotationInst*
    CreateRXInst(const Qubit& q, const FloatWithPrecision& theta, BasicBlock* insert_at_end) {
        return Create(Opcode::Table::RX, q, theta, insert_at_end);
    }
    static ParametrizedRotationInst*
    CreateRYInst(const Qubit& q, const FloatWithPrecision& theta, Instruction* insert_before) {
        return Create(Opcode::Table::RY, q, theta, insert_before);
    }
    static ParametrizedRotationInst*
    CreateRYInst(const Qubit& q, const FloatWithPrecision& theta, BasicBlock* insert_at_end) {
        return Create(Opcode::Table::RY, q, theta, insert_at_end);
    }
    static ParametrizedRotationInst*
    CreateRZInst(const Qubit& q, const FloatWithPrecision& theta, Instruction* insert_before) {
        return Create(Opcode::Table::RZ, q, theta, insert_before);
    }
    static ParametrizedRotationInst*
    CreateRZInst(const Qubit& q, const FloatWithPrecision& theta, BasicBlock* insert_at_end) {
        return Create(Opcode::Table::RZ, q, theta, insert_at_end);
    }

    const Qubit& GetQubit() const {
        return UnaryInst::GetQubit();
    }
    const FloatWithPrecision& GetTheta() const {
        return theta_;
    }
    Opcode GetOpcode() const {
        return Instruction::GetOpcode();
    }
    void Print(std::ostream& out) const override;

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->GetOpcode().IsParametrizedRotation();
    }

private:
    ParametrizedRotationInst(Opcode opcode, const Qubit& q, const FloatWithPrecision& theta)
        : UnaryInst(opcode, q)
        , theta_{theta} {}

    FloatWithPrecision theta_;
};
class QRET_EXPORT BinaryInst : public Instruction {
public:
    static BinaryInst*
    Create(Opcode opcode, const Qubit& q0, const Qubit& q1, Instruction* insert_before);
    static BinaryInst*
    Create(Opcode opcode, const Qubit& q0, const Qubit& q1, BasicBlock* insert_at_end);
    static BinaryInst* CreateCXInst(const Qubit& q0, const Qubit& q1, Instruction* insert_before) {
        return Create(Opcode::Table::CX, q0, q1, insert_before);
    }
    static BinaryInst* CreateCXInst(const Qubit& q0, const Qubit& q1, BasicBlock* insert_at_end) {
        return Create(Opcode::Table::CX, q0, q1, insert_at_end);
    }
    static BinaryInst* CreateCYInst(const Qubit& q0, const Qubit& q1, Instruction* insert_before) {
        return Create(Opcode::Table::CY, q0, q1, insert_before);
    }
    static BinaryInst* CreateCYInst(const Qubit& q0, const Qubit& q1, BasicBlock* insert_at_end) {
        return Create(Opcode::Table::CY, q0, q1, insert_at_end);
    }
    static BinaryInst* CreateCZInst(const Qubit& q0, const Qubit& q1, Instruction* insert_before) {
        return Create(Opcode::Table::CZ, q0, q1, insert_before);
    }
    static BinaryInst* CreateCZInst(const Qubit& q0, const Qubit& q1, BasicBlock* insert_at_end) {
        return Create(Opcode::Table::CZ, q0, q1, insert_at_end);
    }

    const Qubit& GetQubit0() const {
        return q0_;
    }
    const Qubit& GetQubit1() const {
        return q1_;
    }
    Opcode GetOpcode() const {
        return Instruction::GetOpcode();
    }
    void Print(std::ostream& out) const override;

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->GetOpcode().IsBinary();
    }

private:
    BinaryInst(Opcode opcode, const Qubit& q0, const Qubit& q1)
        : Instruction(opcode)
        , q0_{q0}
        , q1_{q1} {}

    Qubit q0_, q1_;
};
class QRET_EXPORT TernaryInst : public Instruction {
public:
    static TernaryInst* Create(
            Opcode opcode,
            const Qubit& q0,
            const Qubit& q1,
            const Qubit& q2,
            Instruction* insert_before
    );
    static TernaryInst* Create(
            Opcode opcode,
            const Qubit& q0,
            const Qubit& q1,
            const Qubit& q2,
            BasicBlock* insert_at_end
    );
    static TernaryInst*
    CreateCCXInst(const Qubit& q0, const Qubit& q1, const Qubit& q2, Instruction* insert_before) {
        return Create(Opcode::Table::CCX, q0, q1, q2, insert_before);
    }
    static TernaryInst*
    CreateCCXInst(const Qubit& q0, const Qubit& q1, const Qubit& q2, BasicBlock* insert_at_end) {
        return Create(Opcode::Table::CCX, q0, q1, q2, insert_at_end);
    }
    static TernaryInst*
    CreateCCYInst(const Qubit& q0, const Qubit& q1, const Qubit& q2, Instruction* insert_before) {
        return Create(Opcode::Table::CCY, q0, q1, q2, insert_before);
    }
    static TernaryInst*
    CreateCCYInst(const Qubit& q0, const Qubit& q1, const Qubit& q2, BasicBlock* insert_at_end) {
        return Create(Opcode::Table::CCY, q0, q1, q2, insert_at_end);
    }
    static TernaryInst*
    CreateCCZInst(const Qubit& q0, const Qubit& q1, const Qubit& q2, Instruction* insert_before) {
        return Create(Opcode::Table::CCZ, q0, q1, q2, insert_before);
    }
    static TernaryInst*
    CreateCCZInst(const Qubit& q0, const Qubit& q1, const Qubit& q2, BasicBlock* insert_at_end) {
        return Create(Opcode::Table::CCZ, q0, q1, q2, insert_at_end);
    }

    const Qubit& GetQubit0() const {
        return q0_;
    }
    const Qubit& GetQubit1() const {
        return q1_;
    }
    const Qubit& GetQubit2() const {
        return q2_;
    }
    Opcode GetOpcode() const {
        return Instruction::GetOpcode();
    }
    void Print(std::ostream& out) const override;

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->GetOpcode().IsTernary();
    }

private:
    TernaryInst(Opcode opcode, const Qubit& q0, const Qubit& q1, const Qubit& q2)
        : Instruction(opcode)
        , q0_{q0}
        , q1_{q1}
        , q2_{q2} {}

    Qubit q0_, q1_, q2_;
};
class QRET_EXPORT MultiControlInst : public Instruction {
public:
    static MultiControlInst*
    Create(Opcode opcode, const Qubit& t, const std::vector<Qubit>& c, Instruction* insert_before);
    static MultiControlInst*
    Create(Opcode opcode, const Qubit& q0, const std::vector<Qubit>& c, BasicBlock* insert_at_end);
    static MultiControlInst*
    CreateMCXInst(const Qubit& t, const std::vector<Qubit>& c, Instruction* insert_before) {
        return Create(Opcode::Table::MCX, t, c, insert_before);
    }
    static MultiControlInst*
    CreateMCXInst(const Qubit& t, const std::vector<Qubit>& c, BasicBlock* insert_at_end) {
        return Create(Opcode::Table::MCX, t, c, insert_at_end);
    }
    const Qubit& GetTarget() const {
        return t_;
    }
    const std::vector<Qubit>& GetControl() const {
        return c_;
    }
    auto control_begin() const {
        return c_.begin();
    }  // NOLINT
    auto control_end() const {
        return c_.end();
    }  // NOLINT
    Opcode GetOpcode() const {
        return Instruction::GetOpcode();
    }
    void Print(std::ostream& out) const override;

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->GetOpcode().IsMultiControl();
    }

private:
    MultiControlInst(Opcode opcode, const Qubit& t, const std::vector<Qubit>& c)
        : Instruction(opcode)
        , t_{t}
        , c_{c} {}

    Qubit t_;
    std::vector<Qubit> c_;
};
class QRET_EXPORT GlobalPhaseInst : public Instruction {
public:
    static GlobalPhaseInst* Create(const FloatWithPrecision& theta, Instruction* insert_before);
    static GlobalPhaseInst* Create(const FloatWithPrecision& theta, BasicBlock* insert_at_end);

    FloatWithPrecision GetTheta() const {
        return theta_;
    }
    Opcode GetOpcode() const {
        return Instruction::GetOpcode();
    }
    void Print(std::ostream& out) const override;

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->GetOpcode() == Opcode::Table::GlobalPhase;
    }

private:
    explicit GlobalPhaseInst(const FloatWithPrecision& theta)
        : Instruction(Opcode::Table::GlobalPhase)
        , theta_{theta} {}

    FloatWithPrecision theta_;
};
class QRET_EXPORT CallInst : public Instruction {
public:
    static CallInst* Create(
            Function* callee,
            const std::vector<Qubit>& operate,
            const std::vector<Register>& input,
            const std::vector<Register>& output,
            Instruction* insert_before
    );
    static CallInst* Create(
            Function* callee,
            const std::vector<Qubit>& operate,
            const std::vector<Register>& input,
            const std::vector<Register>& output,
            BasicBlock* insert_at_end
    );

    Function* GetCallee() const {
        return callee_;
    }
    void SetCallee(Function* callee) {
        callee_ = callee;
    }
    const std::vector<Qubit>& GetOperate() const {
        return operate_;
    }
    std::vector<Qubit>& GetMutOperate() {
        return operate_;
    }
    const std::vector<Register>& GetInput() const {
        return input_;
    }
    const std::vector<Register>& GetOutput() const {
        return output_;
    }
    Opcode GetOpcode() const {
        return Instruction::GetOpcode();
    }
    void Print(std::ostream& out) const override;

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->GetOpcode() == Opcode::Table::Call;
    }

private:
    CallInst(
            Function* callee,
            const std::vector<Qubit>& operate,
            const std::vector<Register>& input,
            const std::vector<Register>& output
    )
        : Instruction(Opcode::Table::Call)
        , callee_{callee}
        , operate_{operate}
        , input_{input}
        , output_{output} {}

    Function* callee_;
    std::vector<Qubit> operate_;
    std::vector<Register> input_;
    std::vector<Register> output_;
};
class QRET_EXPORT CallCFInst : public Instruction {
public:
    static CallCFInst* Create(
            const PortableAnnotatedFunction& function,
            const std::vector<Register>& input,
            const std::vector<Register>& output,
            Instruction* insert_before
    );
    static CallCFInst* Create(
            const PortableAnnotatedFunction& function,
            const std::vector<Register>& input,
            const std::vector<Register>& output,
            BasicBlock* insert_at_end
    );

    Opcode GetOpcode() const {
        return Instruction::GetOpcode();
    }
    void Print(std::ostream& out) const override;
    const PortableAnnotatedFunction& GetFunction() const {
        return function_;
    }
    const std::vector<Register>& GetInput() const {
        return input_;
    }
    const std::vector<Register>& GetOutput() const {
        return output_;
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->GetOpcode() == Opcode::Table::CallCF;
    }

private:
    CallCFInst(
            const PortableAnnotatedFunction& function,
            const std::vector<Register>& input,
            const std::vector<Register>& output
    )
        : Instruction(Opcode::Table::CallCF)
        , function_{function}
        , input_{input}
        , output_{output} {}

    PortableAnnotatedFunction function_;
    std::vector<Register> input_;
    std::vector<Register> output_;
};
/**
 * @brief DiscreteDistInst represents a classical random number generator instruction.
 *
 * This instruction produces an integer value according to an arbitrary discrete distribution.
 * The distribution is specified by a sequence of non-negative weights.
 *
 * Specification:
 *  - All elements of @p weights_ must be non-negative.
 *  - Let S = sum(weights). Then S must be strictly positive.
 *  - With probability weights[i] / S, the integer value i is chosen and stored
 *    into @p registers_.
 *  - The number of registers in @p registers_ must be at least
 *    ceil(log2(weights.size())) to hold the generated index.
 */
class QRET_EXPORT DiscreteDistInst : public Instruction {
public:
    static DiscreteDistInst* Create(
            const std::vector<double>& weights,
            const std::vector<Register>& registers,
            Instruction* insert_before
    );
    static DiscreteDistInst* Create(
            const std::vector<double>& weights,
            const std::vector<Register>& registers,
            BasicBlock* insert_at_end
    );

    Opcode GetOpcode() const {
        return Instruction::GetOpcode();
    }
    void Print(std::ostream& out) const override;
    const std::vector<double>& GetWeights() const {
        return weights_;
    }
    const std::vector<Register>& GetRegisters() const {
        return registers_;
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->GetOpcode() == Opcode::Table::DiscreteDistribution;
    }

private:
    DiscreteDistInst(const std::vector<double>& weights, const std::vector<Register>& registers)
        : Instruction(Opcode::Table::DiscreteDistribution)
        , weights_{weights}
        , registers_{registers} {}

    std::vector<double> weights_;
    std::vector<Register> registers_;
};
class QRET_EXPORT BranchInst : public Instruction {
public:
    static BranchInst* Create(BasicBlock* if_true, BasicBlock* insert_at_end);
    static BranchInst* Create(
            BasicBlock* if_true,
            BasicBlock* if_false,
            const Register& cond,
            BasicBlock* insert_at_end
    );

    BasicBlock* GetTrueBB() const {
        return if_true_;
    }
    BasicBlock* GetFalseBB() const {
        return if_false_;
    }
    bool IsUnconditional() const {
        return if_false_ == nullptr;
    }
    bool IsConditional() const {
        return if_false_ != nullptr;
    }
    std::size_t GetNumSuccessors() const {
        return IsUnconditional() ? 1 : 2;
    }
    const Register& GetCondition() const {
        return cond_;
    }
    Opcode GetOpcode() const {
        return Instruction::GetOpcode();
    }
    void Print(std::ostream& out) const override;

    void ReplaceBB(BasicBlock* from, BasicBlock* to) {
        if (from == nullptr || to == nullptr) {
            return;
        }
        if (from == if_true_) {
            if_true_ = to;
        }
        if (from == if_false_) {
            if_false_ = to;
        }
        if (if_true_ == if_false_) {
            // To unconditional branch
            if_false_ = nullptr;
        }
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->GetOpcode().GetCode() == Opcode::Table::Branch;
    }

private:
    BranchInst(BasicBlock* if_true, BasicBlock* if_false, const Register& cond)
        : Instruction(Opcode::Table::Branch)
        , if_true_{if_true}
        , if_false_{if_false}
        , cond_{cond} {}

    BasicBlock* if_true_;
    BasicBlock* if_false_;
    Register cond_;
};
class QRET_EXPORT SwitchInst : public Instruction {
public:
    static SwitchInst* Create(
            const std::vector<Register>& value,
            BasicBlock* default_bb,
            const std::map<std::uint64_t, BasicBlock*>& case_bb,
            BasicBlock* insert_at_end
    );

    const std::vector<Register>& GetValue() const {
        return value_;
    }
    BasicBlock* GetDefaultBB() const {
        return default_bb_;
    }
    const std::map<std::uint64_t, BasicBlock*>& GetCaseBB() const {
        return case_bb_;
    }
    std::size_t NumCases() const {
        return case_bb_.size();
    }

    BasicBlock* GetNext(std::uint64_t value) const {
        const auto itr = case_bb_.find(value);
        if (itr == case_bb_.end()) {
            return GetDefaultBB();
        }
        return itr->second;
    }
    BasicBlock* GetNext(const std::vector<bool>& value) const {
        return GetNext(BoolArrayAsInt(value));
    }

    auto case_begin() {
        return case_bb_.begin();
    }
    auto case_begin() const {
        return case_bb_.begin();
    }
    auto case_end() {
        return case_bb_.end();
    }
    auto case_end() const {
        return case_bb_.end();
    }

    std::size_t GetNumSuccessors() const {
        return NumCases() + 1;
    }

    Opcode GetOpcode() const {
        return Instruction::GetOpcode();
    }
    void Print(std::ostream& out) const override;

    void ReplaceBB(BasicBlock* from, BasicBlock* to) {
        if (from == nullptr || to == nullptr) {
            return;
        }
        if (from == default_bb_) {
            default_bb_ = to;
        }
        for (auto&& [key, case_bb] : case_bb_) {
            if (from == case_bb) {
                case_bb = to;
            }
        }
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->GetOpcode().GetCode() == Opcode::Table::Switch;
    }

private:
    SwitchInst(
            const std::vector<Register>& value,
            BasicBlock* default_bb,
            const std::map<std::uint64_t, BasicBlock*>& case_bb
    )
        : Instruction(Opcode::Table::Switch)
        , value_{value}
        , default_bb_{default_bb}
        , case_bb_{case_bb} {}

    std::vector<Register> value_;
    BasicBlock* default_bb_;
    std::map<std::uint64_t, BasicBlock*> case_bb_;
};
class QRET_EXPORT ReturnInst : public Instruction {
public:
    static ReturnInst* Create(BasicBlock* insert_at_end);

    std::size_t GetNumSuccessors() const {
        return 0;
    }
    Opcode GetOpcode() const {
        return Instruction::GetOpcode();
    }
    void Print(std::ostream& out) const override;

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->GetOpcode().GetCode() == Opcode::Table::Return;
    }

private:
    ReturnInst()
        : Instruction(Opcode::Table::Return) {}
};
class QRET_EXPORT CleanInst : public Instruction {
public:
    static CleanInst* Create(const Qubit& q, Instruction* insert_before);
    static CleanInst* Create(const Qubit& q, BasicBlock* insert_at_end);

    const Qubit& GetQubit() const {
        return q_;
    }
    Opcode GetOpcode() const {
        return Instruction::GetOpcode();
    }
    void Print(std::ostream& out) const override;

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->GetOpcode().GetCode() == Opcode::Table::Clean;
    }

private:
    explicit CleanInst(const Qubit& q)
        : Instruction(Opcode::Table::Clean)
        , q_{q} {}

    Qubit q_;
};
class QRET_EXPORT CleanProbInst : public Instruction {
public:
    static CleanProbInst*
    Create(const Qubit& q, const FloatWithPrecision& clean_prob, Instruction* insert_before);
    static CleanProbInst*
    Create(const Qubit& q, const FloatWithPrecision& clean_prob, BasicBlock* insert_at_end);

    const Qubit& GetQubit() const {
        return q_;
    }
    const FloatWithPrecision& GetCleanProb() const {
        return clean_prob_;
    }
    Opcode GetOpcode() const {
        return Instruction::GetOpcode();
    }
    void Print(std::ostream& out) const override;

    bool IsStateApproximatelyClean(double epsilon, bool value = false) const {
        if (value) {
            // is state |1> ?
            return clean_prob_.value + clean_prob_.precision < epsilon;
        } else {
            // is state |0> ?
            return 1 - epsilon < clean_prob_.value - clean_prob_.precision;
        }
    }
    bool IsStateApproximatelyBasis(double epsilon) const {
        return IsStateApproximatelyClean(epsilon, true)
                || IsStateApproximatelyClean(epsilon, false);
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->GetOpcode().GetCode() == Opcode::Table::CleanProb;
    }

private:
    explicit CleanProbInst(const Qubit& q, const FloatWithPrecision& clean_prob)
        : Instruction(Opcode::Table::CleanProb)
        , q_{q}
        , clean_prob_{clean_prob} {}

    Qubit q_;
    FloatWithPrecision
            clean_prob_;  //!< the probability of obtaining the state |0> after Z-basis measurement
};
class QRET_EXPORT DirtyInst : public Instruction {
public:
    static DirtyInst* Create(Opcode opcode, const Qubit& q, Instruction* insert_before);
    static DirtyInst* Create(Opcode opcode, const Qubit& q, BasicBlock* insert_at_end);
    static DirtyInst* CreateBegin(const Qubit& q, Instruction* insert_before) {
        return Create(Opcode::Table::DirtyBegin, q, insert_before);
    }
    static DirtyInst* CreateBegin(const Qubit& q, BasicBlock* insert_at_end) {
        return Create(Opcode::Table::DirtyBegin, q, insert_at_end);
    }
    static DirtyInst* CreateEnd(const Qubit& q, Instruction* insert_before) {
        return Create(Opcode::Table::DirtyEnd, q, insert_before);
    }
    static DirtyInst* CreateEnd(const Qubit& q, BasicBlock* insert_at_end) {
        return Create(Opcode::Table::DirtyEnd, q, insert_at_end);
    }

    const Qubit& GetQubit() const {
        return q_;
    }
    Opcode GetOpcode() const {
        return Instruction::GetOpcode();
    }
    void Print(std::ostream& out) const override;

    bool IsBegin() const {
        return GetOpcode().GetCode() == Opcode::Table::DirtyBegin;
    }
    bool IsEnd() const {
        return GetOpcode().GetCode() == Opcode::Table::DirtyEnd;
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        const auto opcode = from->GetOpcode().GetCode();
        return opcode == Opcode::Table::DirtyBegin || opcode == Opcode::Table::DirtyEnd;
    }

private:
    explicit DirtyInst(Opcode opcode, const Qubit& q)
        : Instruction(opcode)
        , q_{q} {}

    Qubit q_;
};
}  // namespace qret::ir

#endif  // QRET_IR_INSTRUCTIONS_H
