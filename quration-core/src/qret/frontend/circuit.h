/**
 * @file qret/frontend/circuit.h
 * @brief Quantum circuits of frontend.
 * @details This file implements frontend quantum circuits.
 */

#ifndef QRET_FRONTEND_CIRCUIT_H
#define QRET_FRONTEND_CIRCUIT_H

#include <fmt/core.h>

#include <stdexcept>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

#include "qret/frontend/argument.h"
#include "qret/frontend/builder.h"
#include "qret/ir/function.h"
#include "qret/ir/instruction.h"
#include "qret/ir/instructions.h"
#include "qret/ir/value.h"
#include "qret/qret_export.h"

namespace qret::frontend {
/**
 * @brief Frontend circuit class.
 */
class QRET_EXPORT Circuit {
public:
    Circuit(CircuitBuilder* builder, ir::Function* ir)
        : builder_{builder}
        , ir_{ir} {}

    enum class Type : std::uint8_t {
        Qubit,
        Register,
    };
    enum class Attribute : std::uint8_t {
        Operate,  // type must be qubit
        CleanAncilla,  // type must be qubit
        DirtyAncilla,  // type must be qubit
        Catalyst,  // type must be qubit
        Input,  // type must be register
        Output,  // type must be register
    };

    /**
     * @brief Argument of frontend circuit.
     */
    class QRET_EXPORT Argument {
    public:
        /**
         * @brief Information of argument.
         * @details Tuple of name, type, size, attribute.
         */
        using ArgInfo = std::tuple<std::string, Type, std::size_t, Attribute>;

        std::size_t GetNumArgs() const {
            return args_.size();
        }
        const ArgInfo& GetArgInfo(std::size_t arg_idx) const {
            return args_[arg_idx];
        }
        const std::string& GetArgName(std::size_t arg_idx) const {
            return std::get<0>(GetArgInfo(arg_idx));
        }
        Type GetType(std::size_t arg_idx) const {
            return std::get<1>(GetArgInfo(arg_idx));
        }
        std::size_t GetSize(std::size_t arg_idx) const {
            return std::get<2>(GetArgInfo(arg_idx));
        }
        Attribute GetAttribute(std::size_t arg_idx) const {
            return std::get<3>(GetArgInfo(arg_idx));
        }
        bool IsQubit(std::size_t arg_idx) const {
            return GetType(arg_idx) == Type::Qubit && GetSize(arg_idx) == 1;
        }
        bool IsQubits(std::size_t arg_idx) const {
            return GetType(arg_idx) == Type::Qubit;
        }
        bool IsRegister(std::size_t arg_idx) const {
            return GetType(arg_idx) == Type::Register && GetSize(arg_idx) == 1;
        }
        bool IsRegisters(std::size_t arg_idx) const {
            return GetType(arg_idx) == Type::Register;
        }
        bool Contains(std::string_view arg_name) const {
            for (const auto& info : args_) {
                if (std::get<0>(info) == arg_name) {
                    return true;
                }
            }
            return false;
        }
        std::size_t GetArgIdx(std::string_view arg_name) const {
            for (auto i = std::size_t{0}; i < GetNumArgs(); ++i) {
                if (std::get<0>(args_[i]) == arg_name) {
                    return i;
                }
            }
            throw std::runtime_error("invalid arg name");
        }

        std::size_t GetNumQubits() const;
        std::size_t GetNumOperateQubits() const;
        std::size_t GetNumCleanAncillae() const;
        std::size_t GetNumDirtyAncillae() const;
        std::size_t GetNumCatalysts() const;
        std::size_t GetNumRegisters() const;
        std::size_t GetNumInputRegisters() const;
        std::size_t GetNumOutputRegisters() const;

        void Add(std::string_view arg_name, Type type, std::size_t size, Attribute attr) {
            if (type == Type::Qubit) {
                const auto ok = Attribute::Operate <= attr && attr <= Attribute::Catalyst;
                if (!ok) {
                    throw std::runtime_error(
                            "attribute of Type::Qubit must be either Operate, CleanAncilla, "
                            "DirtyAncilla or Catalyst"
                    );
                }
            } else if (type == Type::Register) {
                const auto ok = Attribute::Input <= attr && attr <= Attribute::Output;
                if (!ok) {
                    throw std::runtime_error(
                            "attribute of Type::Register must be either Input or Output"
                    );
                }
            }
            args_.emplace_back(arg_name, type, size, attr);
        }

        using iterator = std::vector<ArgInfo>::iterator;
        using const_iterator = std::vector<ArgInfo>::const_iterator;
        iterator begin() {
            return args_.begin();
        }  // NOLINT
        const_iterator begin() const {
            return args_.cbegin();
        }  // NOLINT
        const_iterator cbegin() const {
            return args_.cbegin();
        }  // NOLINT
        iterator end() {
            return args_.end();
        }  // NOLINT
        const_iterator end() const {
            return args_.cend();
        }  // NOLINT
        const_iterator cend() const {
            return args_.cend();
        }  // NOLINT

    private:
        std::size_t CountQubit() const {
            return Count(Type::Qubit);
        }
        std::size_t CountRegister() const {
            return Count(Type::Register);
        }
        std::size_t Count(Type type) const;
        std::size_t Count(Type type, Attribute attr) const;

        std::vector<ArgInfo> args_;
    };

    CircuitBuilder* GetBuilder() const {
        return builder_;
    }
    const Argument& GetArgument() const {
        return argument_;
    }
    Argument& GetMutArgument() {
        return argument_;
    }

    /**
     * @brief Insert a call instruction into the circuit currently being defined in CircuitBuilder.
     * @details The arguments must match those obtained from GetArgument().
     *
     * @param args arguments
     * @return ir::CallInst* the inserted instruction
     */
    template <
            typename... Args,
            std::enable_if_t<std::conjunction_v<tag::IsQTarget<std::remove_cv_t<Args>>...>, bool> =
                    false>
    ir::CallInst* operator()(const Args&... args) const;

    /**
     * @brief Insert a call instruction into the circuit currently being defined in CircuitBuilder.
     *
     * @param q qubits
     * @param in_r input registers
     * @param out_r output registers
     * @return ir::CallInst* the inserted instruction
     */
    ir::CallInst* CallImpl(
            const std::vector<ir::Qubit>& q,
            const std::vector<ir::Register>& in_r,
            const std::vector<ir::Register>& out_r
    ) const {
        if (!loaded_from_ir_) {
            CheckIRArg(q, in_r, out_r);
        }
        auto* bb = GetBuilder()->GetInsertionPoint();
        return ir::CallInst::Create(ir_, q, in_r, out_r, bb);
    }

    std::string_view GetName() const {
        return ir_->GetName();
    }
    void SetLoadedFromIR(bool status) {
        loaded_from_ir_ = status;
    }
    [[nodiscard]] bool LoadedFromIR() const {
        return loaded_from_ir_;
    }
    ir::Function* GetIR() const {
        return ir_;
    }

    void SetAdjoint(Circuit* adjoint) {
        adjoint_ = adjoint;
    }
    bool HasAdjoint() const {
        return adjoint_ != nullptr;
    }
    Circuit* GetAdjoint() const {
        return adjoint_;
    }
    void SetControlled(Circuit* control) {
        controlled_ = control;
    }
    bool HasControlled() const {
        return controlled_ != nullptr;
    }
    Circuit* GetControlled() const {
        return controlled_;
    }

private:
    template <typename... Args>
    void ParseArguments(
            std::vector<ir::Qubit>& q,
            std::vector<ir::Register>& in_r,
            std::vector<ir::Register>& out_r,
            const Args&... args
    ) const {
        ParseArgument<0>(q, in_r, out_r, args...);
    }
    template <std::size_t arg_idx, typename Head, typename... Tail>
    void ParseArgument(
            std::vector<ir::Qubit>& q,
            std::vector<ir::Register>& in_r,
            std::vector<ir::Register>& out_r,
            const Head& arg,
            const Tail&... tail
    ) const {
        CheckTypeAndSize(arg_idx, arg);
        if constexpr (tag::IsQubitV<Head>) {
            Parse(q, arg);
        }
        if constexpr (tag::IsRegisterV<Head>) {
            if (GetArgument().GetAttribute(arg_idx) == Attribute::Input) {
                Parse(in_r, arg);
            } else {
                Parse(out_r, arg);
            }
        }
        ParseArgument<arg_idx + 1>(q, in_r, out_r, tail...);
    }
    template <std::size_t arg_idx>
    void ParseArgument(
            std::vector<ir::Qubit>&,
            std::vector<ir::Register>&,
            std::vector<ir::Register>&
    ) const {
        if (arg_idx != GetArgument().GetNumArgs()) {
            throw std::runtime_error("passed fewer args than expected");
        }
    }
    template <typename... Args>
    void ParseArguments(std::vector<ir::Qubit>& q, const Args&... args) const {
        ParseArgument<0>(q, args...);
    }
    template <std::size_t arg_idx, typename Head, typename... Tail>
    void ParseArgument(std::vector<ir::Qubit>& q, const Head& arg, const Tail&... tail) const {
        if constexpr (tag::IsQubitV<Head>) {
            Parse(q, arg);
        }
        if constexpr (tag::IsRegisterV<Head>) {
            throw std::runtime_error("Loaded circuits with register argument is not supported.");
        }
        ParseArgument<arg_idx + 1>(q, tail...);
    }
    template <std::size_t arg_idx>
    void ParseArgument(std::vector<ir::Qubit>&) const {}
    template <typename T>
    void CheckTypeAndSize(std::size_t arg_idx, const T& arg) const {
        if (arg_idx >= GetArgument().GetNumArgs()) {
            throw std::runtime_error("passed more args than expected");
        }

        const auto& arg_name = GetArgument().GetArgName(arg_idx);
        const auto ex_type = GetArgument().GetType(arg_idx);
        const auto ex_size = GetArgument().GetSize(arg_idx);

        if (ex_type == Type::Qubit && !tag::IsQubitV<T>) {
            throw std::runtime_error("type error: expected Qubit but got Register");
        }
        if (ex_type == Type::Register && !tag::IsRegisterV<T>) {
            throw std::runtime_error("type error: expected Register but got Qubit");
        }
        if (ex_size != arg.Size()) {
            throw std::runtime_error(
                    fmt::format(
                            R"(size error of argument "{}" in circuit "{}": expected = {}, actual = {})",
                            arg_name,
                            ir_->GetName(),
                            ex_size,
                            arg.Size()
                    )
            );
        }
    }
    void CheckIRArg(
            const std::vector<ir::Qubit>& q,
            const std::vector<ir::Register>& in_r,
            const std::vector<ir::Register>& out_r
    ) const {
        if (q.size() != argument_.GetNumQubits()) {
            throw std::runtime_error(
                    fmt::format(
                            "invalid number of qubits: {} (expected: {})",
                            q.size(),
                            argument_.GetNumQubits()
                    )
            );
        }
        if (in_r.size() != argument_.GetNumInputRegisters()) {
            throw std::runtime_error(
                    fmt::format(
                            "invalid number of input registers: {} (expected: {})",
                            in_r.size(),
                            argument_.GetNumInputRegisters()
                    )
            );
        }
        if (out_r.size() != argument_.GetNumOutputRegisters()) {
            throw std::runtime_error(
                    fmt::format(
                            "invalid number of output registers: {} (expected: {})",
                            out_r.size(),
                            argument_.GetNumOutputRegisters()
                    )
            );
        }
    }
    void Parse(std::vector<ir::Qubit>& q, const Qubits& qs) const {
        for (const auto& i : qs) {
            q.emplace_back(i.GetId());
        }
    }
    void Parse(std::vector<ir::Register>& r, const Registers& rs) const {
        for (const auto& i : rs) {
            r.emplace_back(i.GetId());
        }
    }

    CircuitBuilder* const builder_;
    ir::Function* const ir_;
    bool loaded_from_ir_ = false;
    Argument argument_;

    Circuit* adjoint_ = nullptr;
    Circuit* controlled_ = nullptr;
};
template <
        typename... Args,
        std::enable_if_t<std::conjunction_v<tag::IsQTarget<std::remove_cv_t<Args>>...>, bool>>
ir::CallInst* Circuit::operator()(const Args&... args) const {
    // If loaded from ir, argument_ is not set.
    if (loaded_from_ir_) {
        auto q = std::vector<ir::Qubit>();
        ParseArguments(q, args...);

        // CheckIRArg(q, {}, {});
        auto* bb = GetBuilder()->GetInsertionPoint();

        return ir::CallInst::Create(ir_, q, {}, {}, bb);
    }

    if (sizeof...(args) != GetArgument().GetNumArgs()) {
        throw std::runtime_error(
                fmt::format(
                        "arg size error: expected {} but got {}",
                        GetArgument().GetNumArgs(),
                        sizeof...(args)
                )
        );
    }
    auto q = std::vector<ir::Qubit>();
    auto in_r = std::vector<ir::Register>();
    auto out_r = std::vector<ir::Register>();
    ParseArguments(q, in_r, out_r, args...);

    CheckIRArg(q, in_r, out_r);
    auto* bb = GetBuilder()->GetInsertionPoint();

    return ir::CallInst::Create(ir_, q, in_r, out_r, bb);
}
}  // namespace qret::frontend

#endif  // QRET_FRONTEND_CIRCUIT_H
