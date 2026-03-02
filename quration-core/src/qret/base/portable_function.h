/**
 * @file qret/base/portable_function.h
 * @brief (De)serializable Function.
 * @details This file defines a function class which can serialize into / deserialize
 * from JSON.
 * If you use this class, quantum circuits that use classical functions can be
 * fully serialized into JSON without loss of information.
 */

#ifndef QRET_BASE_PORTABLE_FUNCTION_H
#define QRET_BASE_PORTABLE_FUNCTION_H

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <list>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include "qret/qret_export.h"

namespace qret {
namespace _portable_function_impl {
using Bool = bool;
using Int = std::int64_t;
}  // namespace _portable_function_impl
template <typename T>
concept PortableFunctionPrimitiveRegisterContentType =
        std::is_same_v<T, _portable_function_impl::Bool>
        || std::is_same_v<T, _portable_function_impl::Int>;

class QRET_EXPORT PortableFunction {
public:
    using Bool = _portable_function_impl::Bool;
    using Int = _portable_function_impl::Int;

    class QRET_EXPORT RegisterType {
    public:
        RegisterType() = default;
        [[nodiscard]] static RegisterType Bool() {
            return RegisterType{RegisterTypeBasic::Bool, 0ULL};
        }
        [[nodiscard]] static RegisterType BoolArray(std::size_t size) {
            return RegisterType{RegisterTypeBasic::BoolArray, size};
        }
        [[nodiscard]] static RegisterType Int() {
            return RegisterType{RegisterTypeBasic::Int, 0ULL};
        }
        [[nodiscard]] static RegisterType IntArray(std::size_t size) {
            return RegisterType{RegisterTypeBasic::IntArray, size};
        }
        template <PortableFunctionPrimitiveRegisterContentType T>
        [[nodiscard]] static RegisterType FromTemplateType() {
            if constexpr (std::is_same_v<T, PortableFunction::Bool>) {
                return Bool();
            }
            if constexpr (std::is_same_v<T, PortableFunction::Int>) {
                return Int();
            }
            assert(0);
        }
        [[nodiscard]] bool IsBool() const {
            return type_ == RegisterTypeBasic::Bool;
        }
        [[nodiscard]] bool IsBoolArray() const {
            return type_ == RegisterTypeBasic::BoolArray;
        }
        [[nodiscard]] bool IsInt() const {
            return type_ == RegisterTypeBasic::Int;
        }
        [[nodiscard]] bool IsIntArray() const {
            return type_ == RegisterTypeBasic::IntArray;
        }
        [[nodiscard]] bool IsArray() const {
            return IsBoolArray() || IsIntArray();
        }

        [[nodiscard]] RegisterType ValueType() const {
            if (IsBoolArray()) {
                return Bool();
            } else if (IsIntArray()) {
                return Int();
            }

            throw std::runtime_error("type must be array");
        }

        [[nodiscard]] std::size_t BitSize() const {
            if (IsBool()) {
                return 1;
            } else if (IsBoolArray()) {
                return size_;
            } else if (IsInt()) {
                return 64;
            } else if (IsIntArray()) {
                return 64 * size_;
            }

            assert(0 && "unreachable");
            return 0;
        }
        [[nodiscard]] std::size_t ArraySize() const {
            if (!IsArray()) {
                throw std::runtime_error("type must be array");
            }
            return size_;
        }

        enum class RegisterTypeBasic : std::int32_t { Bool, Int, BoolArray, IntArray };

        [[nodiscard]] bool operator==(const RegisterType& rhs) const noexcept = default;

    private:
        RegisterTypeBasic type_;
        std::size_t size_;

        RegisterType(RegisterTypeBasic type, std::size_t size)
            : type_(type)
            , size_(size) {}
    };

    class QRET_EXPORT RegisterAccessor {
    public:
        using KeyType = std::variant<
                Int,
                std::string,
                std::pair<std::string, Int>,
                std::pair<std::string, std::string>>;
        RegisterAccessor() = default;
        RegisterAccessor(Int immediate)  // NOLINT
            : key_(immediate) {}
        RegisterAccessor(std::string_view register_name)  // NOLINT
            : key_(std::string(register_name)) {}  // NOLINT
        RegisterAccessor(const std::string& register_name, Int index_immediate)
            : key_(std::make_pair(std::string(register_name), index_immediate)) {}
        RegisterAccessor(std::string_view register_name, std::string_view index_register)
            : key_(std::make_pair(std::string(register_name), std::string(index_register))) {}

        [[nodiscard]] bool IsImmediate() const {
            return key_.index() == 0;
        }
        [[nodiscard]] bool IsDirectAccess() const {
            return key_.index() == 1;
        }
        [[nodiscard]] bool IsIndexedAccessByImmediate() const {
            return key_.index() == 2;
        }
        [[nodiscard]] bool IsIndexedAccessByRegister() const {
            return key_.index() == 3;
        }
        [[nodiscard]] bool IsIndexedAccess() const {
            return IsIndexedAccessByImmediate() || IsIndexedAccessByRegister();
        }

        [[nodiscard]] Int GetImmediate() const {
            return std::get<0>(key_);
        }
        [[nodiscard]] std::string GetDirectAccess() const {
            return std::get<1>(key_);
        }
        [[nodiscard]] std::pair<std::string, Int> GetIndexedAccessByImmediate() const {
            return std::get<2>(key_);
        }
        [[nodiscard]] std::pair<std::string, std::string> GetIndexedAccessByRegister() const {
            return std::get<3>(key_);
        }

        [[nodiscard]] bool operator==(const RegisterAccessor& rhs) const noexcept = default;

    private:
        KeyType key_;
    };

    class QRET_EXPORT OutputValues {
        friend PortableFunction;

    public:
        std::list<std::string> Keys() const;
        bool Contains(std::string_view key) const;
        RegisterType ValueType(std::string_view key) const;

        Bool GetBool(std::string_view key) const;
        Int GetInt(std::string_view key) const;
        const std::vector<Bool>& GetBoolArray(std::string_view key) const;
        const std::vector<Int>& GetIntArray(std::string_view key) const;

    private:
        std::map<std::string, Bool> bool_variables;
        std::map<std::string, Int> int_variables;
        std::map<std::string, std::vector<Bool>> boolarray_variables;
        std::map<std::string, std::vector<Int>> intarray_variables;

        OutputValues(
                std::map<std::string, Bool>&& bool_variables,
                std::map<std::string, Int>&& int_variables,
                std::map<std::string, std::vector<Bool>>&& boolarray_variables,
                std::map<std::string, std::vector<Int>>&& intarray_variables
        )
            : bool_variables(std::move(bool_variables))
            , int_variables(std::move(int_variables))
            , boolarray_variables(std::move(boolarray_variables))
            , intarray_variables(std::move(intarray_variables)) {}
    };

    PortableFunction() = default;
    PortableFunction(
            std::string_view name,
            const std::map<std::string, RegisterType>& input,
            const std::map<std::string, RegisterType>& output,
            const std::map<std::string, RegisterType>& tmp_variable
    );

    void CopyBool(const RegisterAccessor& dst, const RegisterAccessor& src);
    void CopyInt(const RegisterAccessor& dst, const RegisterAccessor& src);
    void BoolToInt(const RegisterAccessor& dst, const RegisterAccessor& src);
    void IntToBool(const RegisterAccessor& dst, const RegisterAccessor& src);
    void LNot(const RegisterAccessor& dst, const RegisterAccessor& src);
    void
    LAnd(const RegisterAccessor& dst, const RegisterAccessor& src1, const RegisterAccessor& src2);
    void
    LOr(const RegisterAccessor& dst, const RegisterAccessor& src1, const RegisterAccessor& src2);
    void
    LXor(const RegisterAccessor& dst, const RegisterAccessor& src1, const RegisterAccessor& src2);
    void BNot(const RegisterAccessor& dst, const RegisterAccessor& src);
    void
    BAnd(const RegisterAccessor& dst, const RegisterAccessor& src1, const RegisterAccessor& src2);
    void
    BOr(const RegisterAccessor& dst, const RegisterAccessor& src1, const RegisterAccessor& src2);
    void
    BXor(const RegisterAccessor& dst, const RegisterAccessor& src1, const RegisterAccessor& src2);
    void
    LShift(const RegisterAccessor& dst, const RegisterAccessor& src1, const RegisterAccessor& src2);
    void
    RShift(const RegisterAccessor& dst, const RegisterAccessor& src1, const RegisterAccessor& src2);
    void Neg(const RegisterAccessor& dst, const RegisterAccessor& src);
    void
    Add(const RegisterAccessor& dst, const RegisterAccessor& src1, const RegisterAccessor& src2);
    void
    Sub(const RegisterAccessor& dst, const RegisterAccessor& src1, const RegisterAccessor& src2);
    void
    Mul(const RegisterAccessor& dst, const RegisterAccessor& src1, const RegisterAccessor& src2);
    void
    Div(const RegisterAccessor& dst, const RegisterAccessor& src1, const RegisterAccessor& src2);
    void
    Mod(const RegisterAccessor& dst, const RegisterAccessor& src1, const RegisterAccessor& src2);
    void Eq(const RegisterAccessor& dst,
            const RegisterAccessor& src1,
            const RegisterAccessor& src2);
    void Ne(const RegisterAccessor& dst,
            const RegisterAccessor& src1,
            const RegisterAccessor& src2);
    void Lt(const RegisterAccessor& dst,
            const RegisterAccessor& src1,
            const RegisterAccessor& src2);
    void Le(const RegisterAccessor& dst,
            const RegisterAccessor& src1,
            const RegisterAccessor& src2);
    void Gt(const RegisterAccessor& dst,
            const RegisterAccessor& src1,
            const RegisterAccessor& src2);
    void Ge(const RegisterAccessor& dst,
            const RegisterAccessor& src1,
            const RegisterAccessor& src2);

    [[nodiscard]] const std::string& Name() const {
        return name_;
    }
    [[nodiscard]] const std::map<std::string, RegisterType>& Input() const {
        return input_;
    }
    [[nodiscard]] const std::map<std::string, RegisterType>& Output() const {
        return output_;
    }
    [[nodiscard]] const std::map<std::string, RegisterType>& TmpVariable() const {
        return tmp_variable_;
    }

    OutputValues Run(
            const std::map<std::string, std::variant<Int, std::vector<Bool>, std::vector<Int>>>&
                    intput_values
    ) const;

    [[nodiscard]] std::vector<std::tuple<std::string, std::vector<RegisterAccessor>>>
    DumpInstructions() const;
    void LoadInstructions(
            const std::vector<std::tuple<std::string, std::vector<RegisterAccessor>>>&
                    dumped_instructions
    );

protected:
    enum class Instruction : std::uint32_t {
        CopyBool = 0,
        CopyInt,
        BoolToInt,
        IntToBool,
        LNot,
        LAnd,
        LOr,
        LXor,
        BNot,
        BAnd,
        BOr,
        BXor,
        LShift,
        RShift,
        Neg,
        Add,
        Sub,
        Mul,
        Div,
        Mod,
        Eq,
        Ne,
        Lt,
        Le,
        Gt,
        Ge,
    };

    struct Type {
        bool is_immediate;
        RegisterType type;

        static Type Immediate() {
            return {true, RegisterType::Int()};
        }
        Type(RegisterType type)
            : is_immediate(false)
            , type(type) {}  // NOLINT

    private:
        Type(bool is_immediate, RegisterType type)
            : is_immediate(is_immediate)
            , type(type) {}
    };

    std::string name_;
    std::map<std::string, RegisterType> input_, output_, tmp_variable_;
    std::vector<std::tuple<Instruction, std::vector<RegisterAccessor>>> instructions_;

    [[nodiscard]] RegisterType GetRegisterType(const std::string& key_name) const;
    [[nodiscard]] Type GetType(const RegisterAccessor& acc) const;

    void AddAssignInstruction(
            const RegisterAccessor& dst,
            const RegisterAccessor& src,
            Instruction instruction
    );
    void AddUnaryBoolInstruction(
            const RegisterAccessor& dst,
            const RegisterAccessor& src,
            Instruction instruction
    );
    void AddBinaryBoolInstruction(
            const RegisterAccessor& dst,
            const RegisterAccessor& src1,
            const RegisterAccessor& src2,
            Instruction instruction
    );
    void AddUnaryIntInstruction(
            const RegisterAccessor& dst,
            const RegisterAccessor& src,
            Instruction instruction
    );
    void AddBinaryIntInstruction(
            const RegisterAccessor& dst,
            const RegisterAccessor& src1,
            const RegisterAccessor& src2,
            Instruction instruction
    );
    void AddCompareInstruction(
            const RegisterAccessor& dst,
            const RegisterAccessor& src1,
            const RegisterAccessor& src2,
            Instruction instruction
    );
};

class QRET_EXPORT PortableFunctionBuilder {
    friend class Variable;

public:
    using VariableType = PortableFunction::RegisterType;
    using RegisterAccessor = PortableFunction::RegisterAccessor;
    using Bool = PortableFunction::Bool;
    using Int = PortableFunction::Int;

    template <PortableFunctionPrimitiveRegisterContentType T>
    class PrimitiveVariable {
        friend class PortableFunctionBuilder;

    public:
        PrimitiveVariable(const PrimitiveVariable& other)
            : PrimitiveVariable(*other.builder_, other.accessor_) {}

        ~PrimitiveVariable() {
            if (accessor_.IsDirectAccess()) {
                if (auto name = accessor_.GetDirectAccess();
                    name.substr(0ULL, 15) == "_internal_bool_") {
                    builder_->DecrementInternalBoolRefCount(std::stoull(name.substr(15)));
                }
                if (auto name = accessor_.GetDirectAccess();
                    name.substr(0ULL, 14) == "_internal_int_") {
                    builder_->DecrementInternalIntRefCount(std::stoull(name.substr(14)));
                }
            }
        }

        [[nodiscard]] PortableFunctionBuilder* Builder() const {
            return builder_;
        }
        [[nodiscard]] const RegisterAccessor& Accessor() const {
            return accessor_;
        }

        PrimitiveVariable& operator=(const PrimitiveVariable& rhs) {
            if (builder_ != rhs.builder_) {
                throw std::runtime_error(
                        "cannot operate two variables from different PortableFunctionBuilder"
                );
            }
            if constexpr (std::is_same_v<T, Bool>) {
                builder_->AddInstruction("CopyBool", {accessor_, rhs.accessor_});
            } else {
                builder_->AddInstruction("CopyInt", {accessor_, rhs.accessor_});
            }
            return *this;
        }
        template <PortableFunctionPrimitiveRegisterContentType RT>
        PrimitiveVariable& operator=(const PrimitiveVariable<RT>& rhs) {
            if (builder_ != rhs.builder_) {
                throw std::runtime_error(
                        "cannot operate two variables from different PortableFunctionBuilder"
                );
            }
            if constexpr (std::is_same_v<T, Bool>) {
                if constexpr (std::is_same_v<RT, Bool>) {
                    builder_->AddInstruction("CopyBool", {accessor_, rhs.accessor_});
                } else {
                    builder_->AddInstruction("IntToBool", {accessor_, rhs.accessor_});
                }
            } else {
                if constexpr (std::is_same_v<RT, Bool>) {
                    builder_->AddInstruction("BoolToInt", {accessor_, rhs.accessor_});
                } else {
                    builder_->AddInstruction("CopyInt", {accessor_, rhs.accessor_});
                }
            }
            return *this;
        }

        [[nodiscard]] PrimitiveVariable<Bool> operator!() const {
            if constexpr (std::is_same_v<T, Int>) {
                return !(builder_->AssignInternalBoolVariable() = *this);
            }
            auto dst = builder_->AssignInternalBoolVariable();
            builder_->AddInstruction("LNot", {dst.accessor_, accessor_});
            return dst;
        }
        [[nodiscard]] PrimitiveVariable<Int> operator~() const {
            if constexpr (std::is_same_v<T, Bool>) {
                return ~(builder_->AssignInternalIntVariable() = *this);
            }
            auto dst = builder_->AssignInternalIntVariable();
            builder_->AddInstruction("BNot", {dst.accessor_, accessor_});
            return dst;
        }
        template <PortableFunctionPrimitiveRegisterContentType RT>
        [[nodiscard]] PrimitiveVariable<
                std::conditional_t<std::is_same_v<T, Int> || std::is_same_v<RT, Int>, Int, Bool>>
        operator&(const PrimitiveVariable<RT>& rhs) const {
            if (builder_ != rhs.builder_) {
                throw std::runtime_error(
                        "cannot operate two variables from different PortableFunctionBuilder"
                );
            }
            constexpr bool ResultIsInt = std::is_same_v<T, PortableFunction::Int>
                    || std::is_same_v<RT, PortableFunction::Int>;
            if constexpr (ResultIsInt) {
                if constexpr (std::is_same_v<T, Bool>) {
                    return (builder_->AssignInternalIntVariable() = *this) & rhs;
                }
                if constexpr (std::is_same_v<RT, Bool>) {
                    return *this & (builder_->AssignInternalIntVariable() = rhs);
                }
                auto dst = builder_->AssignInternalIntVariable();
                builder_->AddInstruction("BAnd", {dst.accessor_, accessor_, rhs.accessor_});
                return dst;
            } else {
                auto dst = builder_->AssignInternalBoolVariable();
                builder_->AddInstruction("LAnd", {dst.accessor_, accessor_, rhs.accessor_});
                return dst;
            }
        }
        template <PortableFunctionPrimitiveRegisterContentType RT>
        [[nodiscard]] PrimitiveVariable<
                std::conditional_t<std::is_same_v<T, Int> || std::is_same_v<RT, Int>, Int, Bool>>
        operator|(const PrimitiveVariable<RT>& rhs) const {
            if (builder_ != rhs.builder_) {
                throw std::runtime_error(
                        "cannot operate two variables from different PortableFunctionBuilder"
                );
            }
            constexpr bool ResultIsInt = std::is_same_v<T, PortableFunction::Int>
                    || std::is_same_v<RT, PortableFunction::Int>;
            if constexpr (ResultIsInt) {
                if constexpr (std::is_same_v<T, Bool>) {
                    return (builder_->AssignInternalIntVariable() = *this) | rhs;
                }
                if constexpr (std::is_same_v<RT, Bool>) {
                    return *this | (builder_->AssignInternalIntVariable() = rhs);
                }
                auto dst = builder_->AssignInternalIntVariable();
                builder_->AddInstruction("BOr", {dst.accessor_, accessor_, rhs.accessor_});
                return dst;
            } else {
                auto dst = builder_->AssignInternalBoolVariable();
                builder_->AddInstruction("LOr", {dst.accessor_, accessor_, rhs.accessor_});
                return dst;
            }
        }
        template <PortableFunctionPrimitiveRegisterContentType RT>
        [[nodiscard]] PrimitiveVariable<
                std::conditional_t<std::is_same_v<T, Int> || std::is_same_v<RT, Int>, Int, Bool>>
        operator^(const PrimitiveVariable<RT>& rhs) const {
            if (builder_ != rhs.builder_) {
                throw std::runtime_error(
                        "cannot operate two variables from different PortableFunctionBuilder"
                );
            }
            constexpr bool ResultIsInt = std::is_same_v<T, PortableFunction::Int>
                    || std::is_same_v<RT, PortableFunction::Int>;
            if constexpr (ResultIsInt) {
                if constexpr (std::is_same_v<T, Bool>) {
                    return (builder_->AssignInternalIntVariable() = *this) ^ rhs;
                }
                if constexpr (std::is_same_v<RT, Bool>) {
                    return *this ^ (builder_->AssignInternalIntVariable() = rhs);
                }
                auto dst = builder_->AssignInternalIntVariable();
                builder_->AddInstruction("BXor", {dst.accessor_, accessor_, rhs.accessor_});
                return dst;
            } else {
                auto dst = builder_->AssignInternalBoolVariable();
                builder_->AddInstruction("LXor", {dst.accessor_, accessor_, rhs.accessor_});
                return dst;
            }
        }
        template <PortableFunctionPrimitiveRegisterContentType RT>
        [[nodiscard]] PrimitiveVariable<Int> operator<<(const PrimitiveVariable<RT>& rhs) const {
            if (builder_ != rhs.builder_) {
                throw std::runtime_error(
                        "cannot operate two variables from different PortableFunctionBuilder"
                );
            }
            if constexpr (std::is_same_v<T, Bool>) {
                return (builder_->AssignInternalIntVariable() = *this) << rhs;
            }
            if constexpr (std::is_same_v<RT, Bool>) {
                return *this << (builder_->AssignInternalIntVariable() = rhs);
            }
            auto dst = builder_->AssignInternalIntVariable();
            builder_->AddInstruction("LShift", {dst.accessor_, accessor_, rhs.accessor_});
            return dst;
        }
        template <PortableFunctionPrimitiveRegisterContentType RT>
        [[nodiscard]] PrimitiveVariable<Int> operator>>(const PrimitiveVariable<RT>& rhs) const {
            if (builder_ != rhs.builder_) {
                throw std::runtime_error(
                        "cannot operate two variables from different PortableFunctionBuilder"
                );
            }
            if constexpr (std::is_same_v<T, Bool>) {
                return (builder_->AssignInternalIntVariable() = *this) >> rhs;
            }
            if constexpr (std::is_same_v<RT, Bool>) {
                return *this >> (builder_->AssignInternalIntVariable() = rhs);
            }
            auto dst = builder_->AssignInternalIntVariable();
            builder_->AddInstruction("RShift", {dst.accessor_, accessor_, rhs.accessor_});
            return dst;
        }
        [[nodiscard]] PrimitiveVariable<Int> operator+() const {
            if constexpr (std::is_same_v<T, Bool>) {
                return +(builder_->AssignInternalIntVariable() = *this);
            }
            return *this;
        }
        [[nodiscard]] PrimitiveVariable<Int> operator-() const {
            if constexpr (std::is_same_v<T, Bool>) {
                return -(builder_->AssignInternalIntVariable() = *this);
            }
            auto dst = builder_->AssignInternalIntVariable();
            builder_->AddInstruction("Neg", {dst.accessor_, accessor_});
            return dst;
        }
        template <PortableFunctionPrimitiveRegisterContentType RT>
        [[nodiscard]] PrimitiveVariable<Int> operator+(const PrimitiveVariable<RT>& rhs) const {
            if (builder_ != rhs.builder_) {
                throw std::runtime_error(
                        "cannot operate two variables from different PortableFunctionBuilder"
                );
            }
            if constexpr (std::is_same_v<T, Bool>) {
                return (builder_->AssignInternalIntVariable() = *this) + rhs;
            }
            if constexpr (std::is_same_v<RT, Bool>) {
                return *this + (builder_->AssignInternalIntVariable() = rhs);
            }
            auto dst = builder_->AssignInternalIntVariable();
            builder_->AddInstruction("Add", {dst.accessor_, accessor_, rhs.accessor_});
            return dst;
        }
        template <PortableFunctionPrimitiveRegisterContentType RT>
        [[nodiscard]] PrimitiveVariable<Int> operator-(const PrimitiveVariable<RT>& rhs) const {
            if (builder_ != rhs.builder_) {
                throw std::runtime_error(
                        "cannot operate two variables from different PortableFunctionBuilder"
                );
            }
            if constexpr (std::is_same_v<T, Bool>) {
                return (builder_->AssignInternalIntVariable() = *this) - rhs;
            }
            if constexpr (std::is_same_v<RT, Bool>) {
                return *this - (builder_->AssignInternalIntVariable() = rhs);
            }
            auto dst = builder_->AssignInternalIntVariable();
            builder_->AddInstruction("Sub", {dst.accessor_, accessor_, rhs.accessor_});
            return dst;
        }
        template <PortableFunctionPrimitiveRegisterContentType RT>
        [[nodiscard]] PrimitiveVariable<Int> operator*(const PrimitiveVariable<RT>& rhs) const {
            if (builder_ != rhs.builder_) {
                throw std::runtime_error(
                        "cannot operate two variables from different PortableFunctionBuilder"
                );
            }
            if constexpr (std::is_same_v<T, Bool>) {
                return (builder_->AssignInternalIntVariable() = *this) * rhs;
            }
            if constexpr (std::is_same_v<RT, Bool>) {
                return *this * (builder_->AssignInternalIntVariable() = rhs);
            }
            auto dst = builder_->AssignInternalIntVariable();
            builder_->AddInstruction("Mul", {dst.accessor_, accessor_, rhs.accessor_});
            return dst;
        }
        template <PortableFunctionPrimitiveRegisterContentType RT>
        [[nodiscard]] PrimitiveVariable<Int> operator/(const PrimitiveVariable<RT>& rhs) const {
            if (builder_ != rhs.builder_) {
                throw std::runtime_error(
                        "cannot operate two variables from different PortableFunctionBuilder"
                );
            }
            if constexpr (std::is_same_v<T, Bool>) {
                return (builder_->AssignInternalIntVariable() = *this) / rhs;
            }
            if constexpr (std::is_same_v<RT, Bool>) {
                return *this / (builder_->AssignInternalIntVariable() = rhs);
            }
            auto dst = builder_->AssignInternalIntVariable();
            builder_->AddInstruction("Div", {dst.accessor_, accessor_, rhs.accessor_});
            return dst;
        }
        template <PortableFunctionPrimitiveRegisterContentType RT>
        [[nodiscard]] PrimitiveVariable<Int> operator%(const PrimitiveVariable<RT>& rhs) const {
            if (builder_ != rhs.builder_) {
                throw std::runtime_error(
                        "cannot operate two variables from different PortableFunctionBuilder"
                );
            }
            if constexpr (std::is_same_v<T, Bool>) {
                return (builder_->AssignInternalIntVariable() = *this) % rhs;
            }
            if constexpr (std::is_same_v<RT, Bool>) {
                return *this % (builder_->AssignInternalIntVariable() = rhs);
            }
            auto dst = builder_->AssignInternalIntVariable();
            builder_->AddInstruction("Mod", {dst.accessor_, accessor_, rhs.accessor_});
            return dst;
        }
        template <PortableFunctionPrimitiveRegisterContentType RT>
        [[nodiscard]] PrimitiveVariable<Bool> operator==(const PrimitiveVariable<RT>& rhs) const {
            if (builder_ != rhs.builder_) {
                throw std::runtime_error(
                        "cannot operate two variables from different PortableFunctionBuilder"
                );
            }
            if constexpr (std::is_same_v<T, Bool>) {
                return (builder_->AssignInternalIntVariable() = *this) == rhs;
            }
            if constexpr (std::is_same_v<RT, Bool>) {
                return *this == (builder_->AssignInternalIntVariable() = rhs);
            }
            auto dst = builder_->AssignInternalBoolVariable();
            builder_->AddInstruction("Eq", {dst.accessor_, accessor_, rhs.accessor_});
            return dst;
        }
        template <PortableFunctionPrimitiveRegisterContentType RT>
        [[nodiscard]] PrimitiveVariable<Bool> operator!=(const PrimitiveVariable<RT>& rhs) const {
            if (builder_ != rhs.builder_) {
                throw std::runtime_error(
                        "cannot operate two variables from different PortableFunctionBuilder"
                );
            }
            if constexpr (std::is_same_v<T, Bool>) {
                return (builder_->AssignInternalIntVariable() = *this) != rhs;
            }
            if constexpr (std::is_same_v<RT, Bool>) {
                return *this != (builder_->AssignInternalIntVariable() = rhs);
            }
            auto dst = builder_->AssignInternalBoolVariable();
            builder_->AddInstruction("Ne", {dst.accessor_, accessor_, rhs.accessor_});
            return dst;
        }
        template <PortableFunctionPrimitiveRegisterContentType RT>
        [[nodiscard]] PrimitiveVariable<Bool> operator<(const PrimitiveVariable<RT>& rhs) const {
            if (builder_ != rhs.builder_) {
                throw std::runtime_error(
                        "cannot operate two variables from different PortableFunctionBuilder"
                );
            }
            if constexpr (std::is_same_v<T, Bool>) {
                return (builder_->AssignInternalIntVariable() = *this) < rhs;
            }
            if constexpr (std::is_same_v<RT, Bool>) {
                return *this < (builder_->AssignInternalIntVariable() = rhs);
            }
            auto dst = builder_->AssignInternalBoolVariable();
            builder_->AddInstruction("Lt", {dst.accessor_, accessor_, rhs.accessor_});
            return dst;
        }
        template <PortableFunctionPrimitiveRegisterContentType RT>
        [[nodiscard]] PrimitiveVariable<Bool> operator<=(const PrimitiveVariable<RT>& rhs) const {
            if (builder_ != rhs.builder_) {
                throw std::runtime_error(
                        "cannot operate two variables from different PortableFunctionBuilder"
                );
            }
            if constexpr (std::is_same_v<T, Bool>) {
                return (builder_->AssignInternalIntVariable() = *this) <= rhs;
            }
            if constexpr (std::is_same_v<RT, Bool>) {
                return *this <= (builder_->AssignInternalIntVariable() = rhs);
            }
            auto dst = builder_->AssignInternalBoolVariable();
            builder_->AddInstruction("Le", {dst.accessor_, accessor_, rhs.accessor_});
            return dst;
        }
        template <PortableFunctionPrimitiveRegisterContentType RT>
        [[nodiscard]] PrimitiveVariable<Bool> operator>(const PrimitiveVariable<RT>& rhs) const {
            if (builder_ != rhs.builder_) {
                throw std::runtime_error(
                        "cannot operate two variables from different PortableFunctionBuilder"
                );
            }
            if constexpr (std::is_same_v<T, Bool>) {
                return (builder_->AssignInternalIntVariable() = *this) > rhs;
            }
            if constexpr (std::is_same_v<RT, Bool>) {
                return *this > (builder_->AssignInternalIntVariable() = rhs);
            }
            auto dst = builder_->AssignInternalBoolVariable();
            builder_->AddInstruction("Gt", {dst.accessor_, accessor_, rhs.accessor_});
            return dst;
        }
        template <PortableFunctionPrimitiveRegisterContentType RT>
        [[nodiscard]] PrimitiveVariable<Bool> operator>=(const PrimitiveVariable<RT>& rhs) const {
            if (builder_ != rhs.builder_) {
                throw std::runtime_error(
                        "cannot operate two variables from different PortableFunctionBuilder"
                );
            }
            if constexpr (std::is_same_v<T, Bool>) {
                return (builder_->AssignInternalIntVariable() = *this) >= rhs;
            }
            if constexpr (std::is_same_v<RT, Bool>) {
                return *this >= (builder_->AssignInternalIntVariable() = rhs);
            }
            auto dst = builder_->AssignInternalBoolVariable();
            builder_->AddInstruction("Ge", {dst.accessor_, accessor_, rhs.accessor_});
            return dst;
        }

        PrimitiveVariable& operator=(Int rhs) {
            *this = PrimitiveVariable<Int>{*builder_, rhs};
            return *this;
        }
        [[nodiscard]] friend PrimitiveVariable<Int>
        operator&(const PrimitiveVariable& lhs, Int rhs) {
            return lhs & PrimitiveVariable<Int>{*lhs.builder_, rhs};
        }
        [[nodiscard]] friend PrimitiveVariable<Int>
        operator|(const PrimitiveVariable& lhs, Int rhs) {
            return lhs | PrimitiveVariable<Int>{*lhs.builder_, rhs};
        }
        [[nodiscard]] friend PrimitiveVariable<Int>
        operator^(const PrimitiveVariable& lhs, Int rhs) {
            return lhs ^ PrimitiveVariable<Int>{*lhs.builder_, rhs};
        }
        [[nodiscard]] friend PrimitiveVariable<Int>
        operator<<(const PrimitiveVariable& lhs, Int rhs) {
            return lhs << PrimitiveVariable<Int>{*lhs.builder_, rhs};
        }
        [[nodiscard]] friend PrimitiveVariable<Int>
        operator>>(const PrimitiveVariable& lhs, Int rhs) {
            return lhs >> PrimitiveVariable<Int>{*lhs.builder_, rhs};
        }
        [[nodiscard]] friend PrimitiveVariable<Int>
        operator+(const PrimitiveVariable& lhs, Int rhs) {
            return lhs + PrimitiveVariable<Int>{*lhs.builder_, rhs};
        }
        [[nodiscard]] friend PrimitiveVariable<Int>
        operator-(const PrimitiveVariable& lhs, Int rhs) {
            return lhs - PrimitiveVariable<Int>{*lhs.builder_, rhs};
        }
        [[nodiscard]] friend PrimitiveVariable<Int>
        operator*(const PrimitiveVariable& lhs, Int rhs) {
            return lhs * PrimitiveVariable<Int>{*lhs.builder_, rhs};
        }
        [[nodiscard]] friend PrimitiveVariable<Int>
        operator/(const PrimitiveVariable& lhs, Int rhs) {
            if (rhs == 0) {
                throw std::runtime_error(
                        "PortableFunctionBuilder::PrimitiveVariable::operator/: division by zero"
                );
            }
            return lhs / PrimitiveVariable<Int>{*lhs.builder_, rhs};
        }
        [[nodiscard]] friend PrimitiveVariable<Int>
        operator%(const PrimitiveVariable& lhs, Int rhs) {
            if (rhs == 0) {
                throw std::runtime_error(
                        "PortableFunctionBuilder::PrimitiveVariable::operator/: division by zero"
                );
            }
            return lhs % PrimitiveVariable<Int>{*lhs.builder_, rhs};
        }
        [[nodiscard]] friend PrimitiveVariable<Bool>
        operator==(const PrimitiveVariable& lhs, Int rhs) {
            return lhs == PrimitiveVariable<Int>{*lhs.builder_, rhs};
        }
        [[nodiscard]] friend PrimitiveVariable<Bool>
        operator!=(const PrimitiveVariable& lhs, Int rhs) {
            return lhs != PrimitiveVariable<Int>{*lhs.builder_, rhs};
        }
        [[nodiscard]] friend PrimitiveVariable<Bool>
        operator<(const PrimitiveVariable& lhs, Int rhs) {
            return lhs < PrimitiveVariable<Int>{*lhs.builder_, rhs};
        }
        [[nodiscard]] friend PrimitiveVariable<Bool>
        operator<=(const PrimitiveVariable& lhs, Int rhs) {
            return lhs <= PrimitiveVariable<Int>{*lhs.builder_, rhs};
        }
        [[nodiscard]] friend PrimitiveVariable<Bool>
        operator>(const PrimitiveVariable& lhs, Int rhs) {
            return lhs > PrimitiveVariable<Int>{*lhs.builder_, rhs};
        }
        [[nodiscard]] friend PrimitiveVariable<Bool>
        operator>=(const PrimitiveVariable& lhs, Int rhs) {
            return lhs >= PrimitiveVariable<Int>{*lhs.builder_, rhs};
        }

        [[nodiscard]] friend PrimitiveVariable<Int>
        operator&(Int lhs, const PrimitiveVariable& rhs) {
            return PrimitiveVariable<Int>{*rhs.builder_, lhs} & rhs;
        }
        [[nodiscard]] friend PrimitiveVariable<Int>
        operator|(Int lhs, const PrimitiveVariable& rhs) {
            return PrimitiveVariable<Int>{*rhs.builder_, lhs} | rhs;
        }
        [[nodiscard]] friend PrimitiveVariable<Int>
        operator^(Int lhs, const PrimitiveVariable& rhs) {
            return PrimitiveVariable<Int>{*rhs.builder_, lhs} ^ rhs;
        }
        [[nodiscard]] friend PrimitiveVariable<Int>
        operator<<(Int lhs, const PrimitiveVariable& rhs) {
            return PrimitiveVariable<Int>{*rhs.builder_, lhs} << rhs;
        }
        [[nodiscard]] friend PrimitiveVariable<Int>
        operator>>(Int lhs, const PrimitiveVariable& rhs) {
            return PrimitiveVariable<Int>{*rhs.builder_, lhs} >> rhs;
        }
        [[nodiscard]] friend PrimitiveVariable<Int>
        operator+(Int lhs, const PrimitiveVariable& rhs) {
            return PrimitiveVariable<Int>{*rhs.builder_, lhs} + rhs;
        }
        [[nodiscard]] friend PrimitiveVariable<Int>
        operator-(Int lhs, const PrimitiveVariable& rhs) {
            return PrimitiveVariable<Int>{*rhs.builder_, lhs} - rhs;
        }
        [[nodiscard]] friend PrimitiveVariable<Int>
        operator*(Int lhs, const PrimitiveVariable& rhs) {
            return PrimitiveVariable<Int>{*rhs.builder_, lhs} * rhs;
        }
        [[nodiscard]] friend PrimitiveVariable<Int>
        operator/(Int lhs, const PrimitiveVariable& rhs) {
            return PrimitiveVariable<Int>{*rhs.builder_, lhs} / rhs;
        }
        [[nodiscard]] friend PrimitiveVariable<Int>
        operator%(Int lhs, const PrimitiveVariable& rhs) {
            return PrimitiveVariable<Int>{*rhs.builder_, lhs} % rhs;
        }
        [[nodiscard]] friend PrimitiveVariable<Bool>
        operator==(Int lhs, const PrimitiveVariable& rhs) {
            return PrimitiveVariable<Int>{*rhs.builder_, lhs} == rhs;
        }
        [[nodiscard]] friend PrimitiveVariable<Bool>
        operator!=(Int lhs, const PrimitiveVariable& rhs) {
            return PrimitiveVariable<Int>{*rhs.builder_, lhs} != rhs;
        }
        [[nodiscard]] friend PrimitiveVariable<Bool>
        operator<(Int lhs, const PrimitiveVariable& rhs) {
            return PrimitiveVariable<Int>{*rhs.builder_, lhs} < rhs;
        }
        [[nodiscard]] friend PrimitiveVariable<Bool>
        operator<=(Int lhs, const PrimitiveVariable& rhs) {
            return PrimitiveVariable<Int>{*rhs.builder_, lhs} <= rhs;
        }
        [[nodiscard]] friend PrimitiveVariable<Bool>
        operator>(Int lhs, const PrimitiveVariable& rhs) {
            return PrimitiveVariable<Int>{*rhs.builder_, lhs} > rhs;
        }
        [[nodiscard]] friend PrimitiveVariable<Bool>
        operator>=(Int lhs, const PrimitiveVariable& rhs) {
            return PrimitiveVariable<Int>{*rhs.builder_, lhs} >= rhs;
        }

        template <PortableFunctionPrimitiveRegisterContentType RT>
        PrimitiveVariable& operator&=(const PrimitiveVariable<RT>& rhs) {
            return *this = *this & rhs;
        }
        PrimitiveVariable& operator&=(Int rhs) {
            return *this = *this & rhs;
        }
        template <PortableFunctionPrimitiveRegisterContentType RT>
        PrimitiveVariable& operator|=(const PrimitiveVariable<RT>& rhs) {
            return *this = *this | rhs;
        }
        PrimitiveVariable& operator|=(Int rhs) {
            return *this = *this | rhs;
        }
        template <PortableFunctionPrimitiveRegisterContentType RT>
        PrimitiveVariable& operator^=(const PrimitiveVariable<RT>& rhs) {
            return *this = *this ^ rhs;
        }
        PrimitiveVariable& operator^=(Int rhs) {
            return *this = *this ^ rhs;
        }
        template <PortableFunctionPrimitiveRegisterContentType RT>
        PrimitiveVariable& operator<<=(const PrimitiveVariable<RT>& rhs) {
            return *this = *this << rhs;
        }
        PrimitiveVariable& operator<<=(Int rhs) {
            return *this = *this << rhs;
        }
        template <PortableFunctionPrimitiveRegisterContentType RT>
        PrimitiveVariable& operator>>=(const PrimitiveVariable<RT>& rhs) {
            return *this = *this >> rhs;
        }
        PrimitiveVariable& operator>>=(Int rhs) {
            return *this = *this >> rhs;
        }
        template <PortableFunctionPrimitiveRegisterContentType RT>
        PrimitiveVariable& operator+=(const PrimitiveVariable<RT>& rhs) {
            return *this = *this + rhs;
        }
        PrimitiveVariable& operator+=(Int rhs) {
            return *this = *this + rhs;
        }
        template <PortableFunctionPrimitiveRegisterContentType RT>
        PrimitiveVariable& operator-=(const PrimitiveVariable<RT>& rhs) {
            return *this = *this - rhs;
        }
        PrimitiveVariable& operator-=(Int rhs) {
            return *this = *this - rhs;
        }
        template <PortableFunctionPrimitiveRegisterContentType RT>
        PrimitiveVariable& operator*=(const PrimitiveVariable<RT>& rhs) {
            return *this = *this * rhs;
        }
        PrimitiveVariable& operator*=(Int rhs) {
            return *this = *this * rhs;
        }
        template <PortableFunctionPrimitiveRegisterContentType RT>
        PrimitiveVariable& operator/=(const PrimitiveVariable<RT>& rhs) {
            return *this = *this / rhs;
        }
        PrimitiveVariable& operator/=(Int rhs) {
            return *this = *this / rhs;
        }
        template <PortableFunctionPrimitiveRegisterContentType RT>
        PrimitiveVariable& operator%=(const PrimitiveVariable<RT>& rhs) {
            return *this = *this % rhs;
        }
        PrimitiveVariable& operator%=(Int rhs) {
            return *this = *this % rhs;
        }

        PrimitiveVariable(PortableFunctionBuilder& builder, const RegisterAccessor& accessor)
            : builder_(&builder)
            , accessor_(accessor) {
            if (accessor.IsImmediate()) {
                if constexpr (!PortableFunctionPrimitiveRegisterContentType<T>) {
                    std::runtime_error("Variable type mismatch for accessor");
                }
            } else if (accessor.IsDirectAccess()) {
                auto type = builder.GetRegisterType(accessor.GetDirectAccess());
                if (type != PortableFunction::RegisterType::FromTemplateType<T>()) {
                    throw std::runtime_error("Variable type mismatch for accessor");
                }
                if (auto name = accessor.GetDirectAccess();
                    name.substr(0ULL, 15) == "_internal_bool_") {
                    builder.IncrementInternalBoolRefCount(std::stoull(name.substr(15)));
                }
                if (auto name = accessor.GetDirectAccess();
                    name.substr(0ULL, 14) == "_internal_int_") {
                    builder.IncrementInternalIntRefCount(std::stoull(name.substr(14)));
                }
            } else if (accessor.IsIndexedAccessByImmediate()) {
                auto [name, idx] = accessor.GetIndexedAccessByImmediate();
                auto arr_type = builder.GetRegisterType(name);
                auto type = arr_type.ValueType();
                if (type != PortableFunction::RegisterType::FromTemplateType<T>()) {
                    throw std::runtime_error("Variable type mismatch for accessor");
                }
                if (static_cast<std::size_t>(idx) >= arr_type.ArraySize()) {
                    throw std::runtime_error(
                            "PortableFunctionBuilder::PrimitiveVariable::PrimitiveVariable: index "
                            "out "
                            "of bound"
                    );
                }
            } else {
                auto type = builder.GetRegisterType(accessor.GetIndexedAccessByRegister().first)
                                    .ValueType();
                if (type != PortableFunction::RegisterType::FromTemplateType<T>()) {
                    throw std::runtime_error("Variable type mismatch for accessor");
                }
            }
        }

    private:
        PortableFunctionBuilder* builder_;
        RegisterAccessor accessor_;
    };

    template <PortableFunctionPrimitiveRegisterContentType T>
    class ArrayVariable {
        friend PortableFunctionBuilder;

    public:
        ArrayVariable(const ArrayVariable& other)
            : ArrayVariable(*other.builder_, other.accessor_) {}

        ~ArrayVariable() {
            if (accessor_.IsDirectAccess()) {
                if (auto name = accessor_.GetDirectAccess();
                    name.substr(0ULL, 16) == "_internal_bool2_") {
                    builder_->DecrementInternalBool2RefCount(std::stoull(name.substr(16)));
                }
                if (auto name = accessor_.GetDirectAccess();
                    name.substr(0ULL, 15) == "_internal_int2_") {
                    builder_->DecrementInternalInt2RefCount(std::stoull(name.substr(15)));
                }
            }
        }

        [[nodiscard]] PortableFunctionBuilder* Builder() const {
            return builder_;
        }
        [[nodiscard]] const RegisterAccessor& Accessor() const {
            return accessor_;
        }

        template <PortableFunctionPrimitiveRegisterContentType RT>
        ArrayVariable& operator=(const ArrayVariable<RT>&) = delete;
        template <PortableFunctionPrimitiveRegisterContentType RT>
        [[nodiscard]] PrimitiveVariable<T> operator[](const PrimitiveVariable<RT>& rhs) const {
            if (builder_ != rhs.builder_) {
                throw std::runtime_error(
                        "cannot operate two variables from different PortableFunctionBuilder"
                );
            }
            assert(accessor_.IsDirectAccess());
            if (rhs.accessor_.IsIndexedAccess()) {
                return (*this)[builder_->AssignInternalIntVariable() = (rhs)];
            }
            if constexpr (std::is_same_v<RT, Bool>) {
                return (*this)[builder_->AssignInternalIntVariable() = (rhs)];
            }
            auto name = accessor_.GetDirectAccess();
            if (rhs.accessor_.IsImmediate()) {
                return {*builder_, {name, rhs.accessor_.GetImmediate()}};
            }
            return {*builder_, {name, rhs.accessor_.GetDirectAccess()}};
        }
        [[nodiscard]] PrimitiveVariable<T> operator[](std::size_t rhs) const {
            return (*this)[PrimitiveVariable<Int>{*builder_, static_cast<Int>(rhs)}];
        }

    private:
        PortableFunctionBuilder* builder_;
        RegisterAccessor accessor_;

        ArrayVariable(PortableFunctionBuilder& builder, const RegisterAccessor& accessor)
            : builder_(&builder)
            , accessor_(accessor) {
            if (accessor.IsDirectAccess()) {
                auto type = builder.GetRegisterType(accessor.GetDirectAccess());
                if (!type.IsArray()
                    || type.ValueType() != PortableFunction::RegisterType::FromTemplateType<T>()) {
                    throw std::runtime_error("Variable type mismatch for accessor");
                }
                if (auto name = accessor.GetDirectAccess();
                    name.substr(0ULL, 16) == "_internal_bool2_") {
                    builder.IncrementInternalBool2RefCount(std::stoull(name.substr(16)));
                }
                if (auto name = accessor.GetDirectAccess();
                    name.substr(0ULL, 15) == "_internal_int2_") {
                    builder.IncrementInternalInt2RefCount(std::stoull(name.substr(15)));
                }
            } else {
                throw std::runtime_error("Variable type mismatch for accessor");
            }
        }
    };

    [[nodiscard]] PrimitiveVariable<Bool> AddBoolInputVariable(const std::string& name);
    [[nodiscard]] PrimitiveVariable<Int> AddIntInputVariable(const std::string& name);
    [[nodiscard]] ArrayVariable<Bool>
    AddBoolArrayInputVariable(const std::string& name, std::size_t size);
    [[nodiscard]] ArrayVariable<Int>
    AddIntArrayInputVariable(const std::string& name, std::size_t size);
    [[nodiscard]] PrimitiveVariable<Bool> AddBoolOutputVariable(const std::string& name);
    [[nodiscard]] PrimitiveVariable<Int> AddIntOutputVariable(const std::string& name);
    [[nodiscard]] ArrayVariable<Bool>
    AddBoolArrayOutputVariable(const std::string& name, std::size_t size);
    [[nodiscard]] ArrayVariable<Int>
    AddIntArrayOutputVariable(const std::string& name, std::size_t size);

    [[nodiscard]] PrimitiveVariable<Bool> AddTmpBoolVariable();
    [[nodiscard]] PrimitiveVariable<Int> AddTmpIntVariable();
    [[nodiscard]] ArrayVariable<Bool> AddTmpBoolArrayVariable(std::size_t size);
    [[nodiscard]] ArrayVariable<Int> AddTmpIntArrayVariable(std::size_t size);

    [[nodiscard]] PortableFunction Compile(std::string_view name) const;

    template <
            PortableFunctionPrimitiveRegisterContentType Cond,
            PortableFunctionPrimitiveRegisterContentType TrueSrc,
            PortableFunctionPrimitiveRegisterContentType FalseSrc>
    [[nodiscard]] PrimitiveVariable<std::conditional_t<
            std::is_same_v<TrueSrc, Int> || std::is_same_v<FalseSrc, Int>,
            Int,
            Bool>>
    Mux(const PrimitiveVariable<Cond>& condition,
        const PrimitiveVariable<TrueSrc>& true_src,
        const PrimitiveVariable<FalseSrc>& false_src) {
        if (this != condition.builder_ || this != true_src.builder_ || this != false_src.builder_) {
            throw std::runtime_error(
                    "cannot operate two variables from different PortableFunctionBuilder"
            );
        }
        if constexpr (std::is_same_v<Cond, Int>) {
            return Mux(AssignInternalBoolVariable() = condition, true_src, false_src);
        }
        constexpr bool ResultIsInt = std::is_same_v<TrueSrc, Int> || std::is_same_v<FalseSrc, Int>;
        if constexpr (ResultIsInt) {
            auto tmp_array = AssignInternalInt2Variable();
            tmp_array[0] = false_src;
            tmp_array[1] = true_src;
            return tmp_array[condition];
        } else {
            auto tmp_array = AssignInternalBool2Variable();
            tmp_array[0] = false_src;
            tmp_array[1] = true_src;
            return tmp_array[condition];
        }
    }
    template <
            PortableFunctionPrimitiveRegisterContentType Cond,
            PortableFunctionPrimitiveRegisterContentType TrueSrc>
    [[nodiscard]] PrimitiveVariable<TrueSrc> Mux(
            const PrimitiveVariable<Cond>& condition,
            const PrimitiveVariable<TrueSrc>& true_src,
            Int false_src
    ) {
        return Mux(condition, true_src, PrimitiveVariable<TrueSrc>(*condition.builder_, false_src));
    }
    template <
            PortableFunctionPrimitiveRegisterContentType Cond,
            PortableFunctionPrimitiveRegisterContentType FalseSrc>
    [[nodiscard]] PrimitiveVariable<FalseSrc> Mux(
            const PrimitiveVariable<Cond>& condition,
            Int true_src,
            const PrimitiveVariable<FalseSrc>& false_src
    ) {
        return Mux(
                condition,
                PrimitiveVariable<FalseSrc>(*condition.builder_, true_src),
                false_src
        );
    }
    template <PortableFunctionPrimitiveRegisterContentType Cond>
    [[nodiscard]] PrimitiveVariable<Int>
    Mux(const PrimitiveVariable<Cond>& condition, Int true_src, Int false_src) {
        return Mux(
                condition,
                PrimitiveVariable(*condition.builder_, true_src),
                PrimitiveVariable(*condition.builder_, false_src)
        );
    }

private:
    std::map<std::string, VariableType> input_, output_, tmp_variable_;
    std::size_t num_tmp_bool_ = 0, num_tmp_int_ = 0, num_tmp_boolarray_ = 0, num_tmp_intarray_ = 0,
                num_internal_bool_ = 0, num_internal_int_ = 0, num_internal_bool2_ = 0,
                num_internal_int2_ = 0;
    std::list<std::size_t> internal_bool_pool_ = {}, internal_int_pool_ = {},
                           internal_bool2_pool_ = {}, internal_int2_pool_ = {};
    std::vector<std::size_t> internal_bool_refcount_ = {}, internal_int_refcount_ = {},
                             internal_bool2_refcount_ = {}, internal_int2_refcount_ = {};
    std::vector<std::size_t> bool_array_sizes_, int_array_sizes_;
    std::vector<std::tuple<std::string, std::vector<RegisterAccessor>>> instructions_;

    [[nodiscard]] VariableType GetRegisterType(const std::string& key_name) const;

    void AddInstruction(std::string_view op, const std::vector<RegisterAccessor>& operands) {
        instructions_.emplace_back(op, operands);
    }

    [[nodiscard]] PrimitiveVariable<Bool> AssignInternalBoolVariable();
    [[nodiscard]] PrimitiveVariable<Int> AssignInternalIntVariable();
    [[nodiscard]] ArrayVariable<Bool> AssignInternalBool2Variable();
    [[nodiscard]] ArrayVariable<Int> AssignInternalInt2Variable();
    void ReleaseInternalBoolVariable(std::size_t id);
    void ReleaseInternalIntVariable(std::size_t id);
    void ReleaseInternalBool2Variable(std::size_t id);
    void ReleaseInternalInt2Variable(std::size_t id);

    void IncrementInternalBoolRefCount(std::size_t id);
    void IncrementInternalIntRefCount(std::size_t id);
    void IncrementInternalBool2RefCount(std::size_t id);
    void IncrementInternalInt2RefCount(std::size_t id);
    void DecrementInternalBoolRefCount(std::size_t id);
    void DecrementInternalIntRefCount(std::size_t id);
    void DecrementInternalBool2RefCount(std::size_t id);
    void DecrementInternalInt2RefCount(std::size_t id);
};

class QRET_EXPORT PortableAnnotatedFunction : public PortableFunction {
public:
    PortableAnnotatedFunction() = default;
    explicit PortableAnnotatedFunction(const PortableFunction& function)
        : PortableFunction(function) {
        if (!IsValidAsRegisterFunction()) {
            throw std::runtime_error(
                    "PortableAnnotatedFunction must have only one BoolArray input named \"input\" "
                    "and "
                    "only one BoolArray output named \"output\""
            );
        }
    }
    PortableAnnotatedFunction& operator=(const PortableFunction& function) {
        return *this = PortableAnnotatedFunction(function);
    }

    void operator()(const std::vector<Bool>& input, std::vector<Bool>& output) const {
        output = Run({{"input", input}}).GetBoolArray("output");
    }

    [[nodiscard]] std::size_t InputWidth() const {
        if (auto it = input_.find("input"); it != input_.end()) {
            auto type = it->second;
            assert(type.IsBoolArray());
            return type.ArraySize();
        }
        assert(0 && "unreachable");
        return 0;
    }
    [[nodiscard]] std::size_t OutputWidth() const {
        if (auto it = output_.find("output"); it != output_.end()) {
            auto type = it->second;
            assert(type.IsBoolArray());
            return type.ArraySize();
        }
        assert(0 && "unreachable");
        return 0;
    }

private:
    [[nodiscard]] bool IsValidAsRegisterFunction() const {
        {
            if (input_.size() != 1) {
                return false;
            }
            auto it = input_.begin();
            if (it->first != "input") {
                return false;
            }
            if (!it->second.IsBoolArray()) {
                return false;
            }
            return true;
        }
        {
            if (output_.size() != 1) {
                return false;
            }
            auto it = output_.begin();
            if (it->first != "output") {
                return false;
            }
            if (!it->second.IsBoolArray()) {
                return false;
            }
            return true;
        }
    }
};
}  // namespace qret

#endif  // QRET_BASE_PORTABLE_FUNCTION_H
