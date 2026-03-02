/**
 * @file qret/base/portable_function.cpp
 * @brief (De)serializable Function.
 */

#include "qret/base/portable_function.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iterator>
#include <list>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace qret {
namespace {
// Static variables used only to avoid compiler Warns
static inline char NullChar = '0';
static inline std::int64_t NullInt = 0;
}  // namespace

std::list<std::string> PortableFunction::OutputValues::Keys() const {
    std::list<std::string> keys;
    for (const auto& [key, _] : bool_variables) {
        keys.push_back(key);
    }
    for (const auto& [key, _] : int_variables) {
        keys.push_back(key);
    }
    for (const auto& [key, _] : boolarray_variables) {
        keys.push_back(key);
    }
    for (const auto& [key, _] : intarray_variables) {
        keys.push_back(key);
    }
    return keys;
}

bool PortableFunction::OutputValues::Contains(std::string_view key) const {
    auto key_s = std::string(key);
    if (bool_variables.contains(key_s)) {
        return true;
    }
    if (int_variables.contains(key_s)) {
        return true;
    }
    if (boolarray_variables.contains(key_s)) {
        return true;
    }
    if (intarray_variables.contains(key_s)) {
        return true;
    }
    return false;
}

PortableFunction::RegisterType PortableFunction::OutputValues::ValueType(
        std::string_view key
) const {
    auto key_s = std::string(key);
    if (auto it = bool_variables.find(key_s); it != bool_variables.end()) {
        return RegisterType::Bool();
    }
    if (auto it = int_variables.find(key_s); it != int_variables.end()) {
        return RegisterType::Int();
    }
    if (auto it = boolarray_variables.find(key_s); it != boolarray_variables.end()) {
        return RegisterType::BoolArray(it->second.size());
    }
    if (auto it = intarray_variables.find(key_s); it != intarray_variables.end()) {
        return RegisterType::IntArray(it->second.size());
    }
    throw std::runtime_error("Value named \"" + key_s + "\" is not found");
}

PortableFunction::Bool PortableFunction::OutputValues::GetBool(std::string_view key) const {
    auto key_s = std::string(key);
    if (auto it = bool_variables.find(key_s); it != bool_variables.end()) {
        return it->second;
    } else {
        throw std::runtime_error("Bool value named \"" + key_s + "\" is not found");
    }
}

PortableFunction::Int PortableFunction::OutputValues::GetInt(std::string_view key) const {
    auto key_s = std::string(key);
    if (auto it = int_variables.find(key_s); it != int_variables.end()) {
        return it->second;
    } else {
        throw std::runtime_error("Int value named " + key_s + " is not found");
    }
}

const std::vector<PortableFunction::Bool>& PortableFunction::OutputValues::GetBoolArray(
        std::string_view key
) const {
    auto key_s = std::string(key);
    if (auto it = boolarray_variables.find(key_s); it != boolarray_variables.end()) {
        return it->second;
    } else {
        throw std::runtime_error("Bool array value named " + key_s + " is not found");
    }
}

const std::vector<PortableFunction::Int>& PortableFunction::OutputValues::GetIntArray(
        std::string_view key
) const {
    auto key_s = std::string(key);
    if (auto it = intarray_variables.find(key_s); it != intarray_variables.end()) {
        return it->second;
    } else {
        throw std::runtime_error("Int array value named " + key_s + " is not found");
    }
}

PortableFunction::PortableFunction(
        std::string_view name,
        const std::map<std::string, RegisterType>& input,
        const std::map<std::string, RegisterType>& output,
        const std::map<std::string, RegisterType>& tmp_variable
)
    : name_(name)
    , input_(input)
    , output_(output)
    , tmp_variable_(tmp_variable)
    , instructions_() {
    auto keys = std::vector<std::string>();
    keys.reserve(input.size() + output.size() + tmp_variable.size());
    std::ranges::transform(input, std::back_inserter(keys), [](auto& p) { return p.first; });
    std::ranges::transform(output, std::back_inserter(keys), [](auto& p) { return p.first; });
    std::ranges::transform(tmp_variable, std::back_inserter(keys), [](auto& p) { return p.first; });
    std::ranges::sort(keys);
    if (std::ranges::adjacent_find(keys) != keys.end()) {
        throw std::runtime_error("keys in input, output and tmp_variable must be distinct");
    }
}

void PortableFunction::CopyBool(const RegisterAccessor& dst, const RegisterAccessor& src) {
    AddAssignInstruction(dst, src, Instruction::CopyBool);
}
void PortableFunction::CopyInt(const RegisterAccessor& dst, const RegisterAccessor& src) {
    AddAssignInstruction(dst, src, Instruction::CopyInt);
}
void PortableFunction::BoolToInt(const RegisterAccessor& dst, const RegisterAccessor& src) {
    AddAssignInstruction(dst, src, Instruction::BoolToInt);
}
void PortableFunction::IntToBool(const RegisterAccessor& dst, const RegisterAccessor& src) {
    AddAssignInstruction(dst, src, Instruction::IntToBool);
}
void PortableFunction::LNot(const RegisterAccessor& dst, const RegisterAccessor& src) {
    AddUnaryBoolInstruction(dst, src, Instruction::LNot);
}
void PortableFunction::LAnd(
        const RegisterAccessor& dst,
        const RegisterAccessor& src1,
        const RegisterAccessor& src2
) {
    AddBinaryBoolInstruction(dst, src1, src2, Instruction::LAnd);
}
void PortableFunction::LOr(
        const RegisterAccessor& dst,
        const RegisterAccessor& src1,
        const RegisterAccessor& src2
) {
    AddBinaryBoolInstruction(dst, src1, src2, Instruction::LOr);
}
void PortableFunction::LXor(
        const RegisterAccessor& dst,
        const RegisterAccessor& src1,
        const RegisterAccessor& src2
) {
    AddBinaryBoolInstruction(dst, src1, src2, Instruction::LXor);
}
void PortableFunction::BNot(const RegisterAccessor& dst, const RegisterAccessor& src) {
    AddUnaryIntInstruction(dst, src, Instruction::BNot);
}
void PortableFunction::BAnd(
        const RegisterAccessor& dst,
        const RegisterAccessor& src1,
        const RegisterAccessor& src2
) {
    AddBinaryIntInstruction(dst, src1, src2, Instruction::BAnd);
}
void PortableFunction::BOr(
        const RegisterAccessor& dst,
        const RegisterAccessor& src1,
        const RegisterAccessor& src2
) {
    AddBinaryIntInstruction(dst, src1, src2, Instruction::BOr);
}
void PortableFunction::BXor(
        const RegisterAccessor& dst,
        const RegisterAccessor& src1,
        const RegisterAccessor& src2
) {
    AddBinaryIntInstruction(dst, src1, src2, Instruction::BXor);
}
void PortableFunction::LShift(
        const RegisterAccessor& dst,
        const RegisterAccessor& src1,
        const RegisterAccessor& src2
) {
    AddBinaryIntInstruction(dst, src1, src2, Instruction::LShift);
}
void PortableFunction::RShift(
        const RegisterAccessor& dst,
        const RegisterAccessor& src1,
        const RegisterAccessor& src2
) {
    AddBinaryIntInstruction(dst, src1, src2, Instruction::RShift);
}
void PortableFunction::Neg(const RegisterAccessor& dst, const RegisterAccessor& src) {
    AddUnaryIntInstruction(dst, src, Instruction::Neg);
}
void PortableFunction::Add(
        const RegisterAccessor& dst,
        const RegisterAccessor& src1,
        const RegisterAccessor& src2
) {
    AddBinaryIntInstruction(dst, src1, src2, Instruction::Add);
}
void PortableFunction::Sub(
        const RegisterAccessor& dst,
        const RegisterAccessor& src1,
        const RegisterAccessor& src2
) {
    AddBinaryIntInstruction(dst, src1, src2, Instruction::Sub);
}
void PortableFunction::Mul(
        const RegisterAccessor& dst,
        const RegisterAccessor& src1,
        const RegisterAccessor& src2
) {
    AddBinaryIntInstruction(dst, src1, src2, Instruction::Mul);
}
void PortableFunction::Div(
        const RegisterAccessor& dst,
        const RegisterAccessor& src1,
        const RegisterAccessor& src2
) {
    AddBinaryIntInstruction(dst, src1, src2, Instruction::Div);
}
void PortableFunction::Mod(
        const RegisterAccessor& dst,
        const RegisterAccessor& src1,
        const RegisterAccessor& src2
) {
    AddBinaryIntInstruction(dst, src1, src2, Instruction::Mod);
}
void PortableFunction::Eq(
        const RegisterAccessor& dst,
        const RegisterAccessor& src1,
        const RegisterAccessor& src2
) {
    AddCompareInstruction(dst, src1, src2, Instruction::Eq);
}
void PortableFunction::Ne(
        const RegisterAccessor& dst,
        const RegisterAccessor& src1,
        const RegisterAccessor& src2
) {
    AddCompareInstruction(dst, src1, src2, Instruction::Ne);
}
void PortableFunction::Lt(
        const RegisterAccessor& dst,
        const RegisterAccessor& src1,
        const RegisterAccessor& src2
) {
    AddCompareInstruction(dst, src1, src2, Instruction::Lt);
}
void PortableFunction::Le(
        const RegisterAccessor& dst,
        const RegisterAccessor& src1,
        const RegisterAccessor& src2
) {
    AddCompareInstruction(dst, src1, src2, Instruction::Le);
}
void PortableFunction::Gt(
        const RegisterAccessor& dst,
        const RegisterAccessor& src1,
        const RegisterAccessor& src2
) {
    AddCompareInstruction(dst, src1, src2, Instruction::Gt);
}
void PortableFunction::Ge(
        const RegisterAccessor& dst,
        const RegisterAccessor& src1,
        const RegisterAccessor& src2
) {
    AddCompareInstruction(dst, src1, src2, Instruction::Ge);
}

PortableFunction::OutputValues PortableFunction::Run(
        const std::map<std::string, std::variant<Int, std::vector<Bool>, std::vector<Int>>>&
                input_values
) const {
    auto bool_variables = std::map<std::string, char>{};
    auto int_variables = std::map<std::string, Int>{};
    // Reference type to the element of std::vector<bool> is proxy type, so it is prone to bugs.
    // Use std::vector<char> instead.
    auto boolarray_variables = std::map<std::string, std::vector<char>>{};
    auto intarray_variables = std::map<std::string, std::vector<Int>>{};
    for (const auto& [k, v] : input_values) {
        if (!input_.contains(k)) {
            throw std::runtime_error("input_value \"" + k + "\" is unknown");
        }
        auto type = GetRegisterType(k);
        if (!type.IsArray()) {
            if (v.index() != 0) {
                throw std::runtime_error("Type of input_value \"" + k + "\" is invalid");
            }
            if (type.IsBool()) {
                bool_variables[k] = static_cast<Bool>(std::get<0>(v));
            }
            if (type.IsInt()) {
                int_variables[k] = std::get<0>(v);
            }
        } else if (type.IsBoolArray()) {
            if (v.index() != 1) {
                throw std::runtime_error("Type of input_value \"" + k + "\" is invalid");
            }
            auto value = std::get<1>(v);
            if (type.ArraySize() != value.size()) {
                throw std::runtime_error("size of input array is invalid");
            }
            std::vector<char> char_v(value.size());
            std::ranges::copy(value, char_v.begin());
            boolarray_variables[k] = std::move(char_v);
        } else if (type.IsIntArray()) {
            if (v.index() != 2) {
                throw std::runtime_error("Type of input_value \"" + k + "\" is invalid");
            }
            if (type.ArraySize() != std::get<2>(v).size()) {
                throw std::runtime_error("size of input array is invalid");
            }
            intarray_variables[k] = std::get<2>(v);
        } else {
            assert(false);
        }
    }
    for (const auto& [k, t] : input_) {
        if (!input_values.contains(k)) {
            throw std::runtime_error("input value named \"" + k + "\" is not given");
        }
    }
    for (const auto& variable_containers : {output_, tmp_variable_}) {
        for (const auto& [k, t] : variable_containers) {
            if (t.IsBool()) {
                bool_variables[k] = false;
            } else if (t.IsInt()) {
                int_variables[k] = 0;
            } else if (t.IsBoolArray()) {
                boolarray_variables[k] = std::vector<char>(t.ArraySize());
            } else if (t.IsIntArray()) {
                intarray_variables[k] = std::vector<Int>(t.ArraySize());
            } else {
                assert(false);
            }
        }
    }
    for (const auto& [instruction, operand] : instructions_) {
        auto resolve_bool_lvalue = [&](const RegisterAccessor& acc) -> char& {
            if (acc.IsImmediate()) {
                assert(false);
            } else if (acc.IsDirectAccess()) {
                auto name = acc.GetDirectAccess();
                auto it = bool_variables.find(name);
                assert(it != bool_variables.end());
                return it->second;
            } else if (acc.IsIndexedAccessByImmediate()) {
                auto [name, idx] = acc.GetIndexedAccessByImmediate();
                auto it = boolarray_variables.find(name);
                assert(it != boolarray_variables.end());
                return (it->second)[static_cast<std::size_t>(idx)];
            } else if (acc.IsIndexedAccessByRegister()) {
                auto [name1, name2] = acc.GetIndexedAccessByRegister();
                auto it1 = boolarray_variables.find(name1);
                assert(it1 != boolarray_variables.end());
                auto it2 = int_variables.find(name2);
                assert(it2 != int_variables.end());
                auto idx = static_cast<std::size_t>(it2->second);
                if (idx >= it1->second.size()) {
                    throw std::runtime_error("PortableFunction::Run: index out of bound");
                }
                return (it1->second)[idx];
            }
            assert(0 && "unreachable");
            return NullChar;
        };
        auto resolve_int_lvalue = [&](const RegisterAccessor& acc) -> Int& {
            if (acc.IsImmediate()) {
                assert(false);
            } else if (acc.IsDirectAccess()) {
                auto name = acc.GetDirectAccess();
                auto it = int_variables.find(name);
                assert(it != int_variables.end());
                return it->second;
            } else if (acc.IsIndexedAccessByImmediate()) {
                auto [name, idx] = acc.GetIndexedAccessByImmediate();
                auto it = intarray_variables.find(name);
                assert(it != intarray_variables.end());
                return (it->second)[static_cast<std::size_t>(idx)];
            } else if (acc.IsIndexedAccessByRegister()) {
                auto [name1, name2] = acc.GetIndexedAccessByRegister();
                auto it1 = intarray_variables.find(name1);
                assert(it1 != intarray_variables.end());
                auto it2 = int_variables.find(name2);
                assert(it2 != int_variables.end());
                auto idx = static_cast<std::size_t>(it2->second);
                if (idx >= it1->second.size()) {
                    throw std::runtime_error("PortableFunction::Run: index out of bound");
                }
                return (it1->second)[idx];
            }
            assert(0 && "unreachable");
            return NullInt;
        };
        auto resolve_bool_rvalue = [&](const RegisterAccessor& acc) -> Bool {
            if (acc.IsImmediate()) {
                return acc.GetImmediate();
            } else if (acc.IsDirectAccess()) {
                auto name = acc.GetDirectAccess();
                auto it = bool_variables.find(name);
                assert(it != bool_variables.end());
                return it->second;
            } else if (acc.IsIndexedAccessByImmediate()) {
                auto [name, idx] = acc.GetIndexedAccessByImmediate();
                auto it = boolarray_variables.find(name);
                assert(it != boolarray_variables.end());
                return (it->second)[static_cast<std::size_t>(idx)];
            } else if (acc.IsIndexedAccessByRegister()) {
                auto [name1, name2] = acc.GetIndexedAccessByRegister();
                auto it1 = boolarray_variables.find(name1);
                assert(it1 != boolarray_variables.end());
                auto it2 = int_variables.find(name2);
                assert(it2 != int_variables.end());
                auto idx = static_cast<std::size_t>(it2->second);
                if (idx >= it1->second.size()) {
                    throw std::runtime_error("PortableFunction::Run: index out of bound");
                }
                return (it1->second)[idx];
            }
            assert(0 && "unreachable");
            return false;
        };
        auto resolve_int_rvalue = [&](const RegisterAccessor& acc) -> Int {
            if (acc.IsImmediate()) {
                return acc.GetImmediate();
            } else if (acc.IsDirectAccess()) {
                auto name = acc.GetDirectAccess();
                auto it = int_variables.find(name);
                assert(it != int_variables.end());
                return it->second;
            } else if (acc.IsIndexedAccessByImmediate()) {
                auto [name, idx] = acc.GetIndexedAccessByImmediate();
                auto it = intarray_variables.find(name);
                assert(it != intarray_variables.end());
                return (it->second)[static_cast<std::size_t>(idx)];
            } else if (acc.IsIndexedAccessByRegister()) {
                auto [name1, name2] = acc.GetIndexedAccessByRegister();
                auto it1 = intarray_variables.find(name1);
                assert(it1 != intarray_variables.end());
                auto it2 = int_variables.find(name2);
                assert(it2 != int_variables.end());
                auto idx = static_cast<std::size_t>(it2->second);
                if (idx >= it1->second.size()) {
                    throw std::runtime_error("PortableFunction::Run: index out of bound");
                }
                return (it1->second)[idx];
            } else {
                assert(false);
            }
            assert(0 && "unreachable");
            return 0;
        };
        if (instruction == Instruction::CopyBool) {
            auto lacc = operand[0];
            auto racc = operand[1];
            resolve_bool_lvalue(lacc) = resolve_bool_rvalue(racc);
        } else if (instruction == Instruction::CopyInt) {
            auto lacc = operand[0];
            auto racc = operand[1];
            resolve_int_lvalue(lacc) = resolve_int_rvalue(racc);
        } else if (instruction == Instruction::BoolToInt) {
            auto lacc = operand[0];
            auto racc = operand[1];
            resolve_int_lvalue(lacc) = resolve_bool_rvalue(racc);
        } else if (instruction == Instruction::IntToBool) {
            auto lacc = operand[0];
            auto racc = operand[1];
            resolve_bool_lvalue(lacc) = static_cast<Bool>(resolve_int_rvalue(racc));
        } else if (instruction == Instruction::LNot) {
            auto lacc = operand[0];
            auto racc = operand[1];
            resolve_bool_lvalue(lacc) = !resolve_bool_rvalue(racc);
        } else if (instruction == Instruction::LAnd) {
            auto lacc = operand[0];
            auto racc1 = operand[1];
            auto racc2 = operand[2];
            resolve_bool_lvalue(lacc) = resolve_bool_rvalue(racc1) && resolve_bool_rvalue(racc2);
        } else if (instruction == Instruction::LOr) {
            auto lacc = operand[0];
            auto racc1 = operand[1];
            auto racc2 = operand[2];
            resolve_bool_lvalue(lacc) = resolve_bool_rvalue(racc1) || resolve_bool_rvalue(racc2);
        } else if (instruction == Instruction::LXor) {
            auto lacc = operand[0];
            auto racc1 = operand[1];
            auto racc2 = operand[2];
            resolve_bool_lvalue(lacc) = resolve_bool_rvalue(racc1) ^ resolve_bool_rvalue(racc2);
        } else if (instruction == Instruction::BNot) {
            auto lacc = operand[0];
            auto racc = operand[1];
            resolve_int_lvalue(lacc) = ~resolve_int_rvalue(racc);
        } else if (instruction == Instruction::BAnd) {
            auto lacc = operand[0];
            auto racc1 = operand[1];
            auto racc2 = operand[2];
            resolve_int_lvalue(lacc) = resolve_int_rvalue(racc1) & resolve_int_rvalue(racc2);
        } else if (instruction == Instruction::BOr) {
            auto lacc = operand[0];
            auto racc1 = operand[1];
            auto racc2 = operand[2];
            resolve_int_lvalue(lacc) = resolve_int_rvalue(racc1) | resolve_int_rvalue(racc2);
        } else if (instruction == Instruction::BXor) {
            auto lacc = operand[0];
            auto racc1 = operand[1];
            auto racc2 = operand[2];
            resolve_int_lvalue(lacc) = resolve_int_rvalue(racc1) ^ resolve_int_rvalue(racc2);
        } else if (instruction == Instruction::LShift) {
            auto lacc = operand[0];
            auto racc1 = operand[1];
            auto racc2 = operand[2];
            resolve_int_lvalue(lacc) = resolve_int_rvalue(racc1) << resolve_int_rvalue(racc2);
        } else if (instruction == Instruction::RShift) {
            auto lacc = operand[0];
            auto racc1 = operand[1];
            auto racc2 = operand[2];
            resolve_int_lvalue(lacc) = resolve_int_rvalue(racc1) >> resolve_int_rvalue(racc2);
        } else if (instruction == Instruction::Neg) {
            auto lacc = operand[0];
            auto racc = operand[1];
            resolve_int_lvalue(lacc) = -resolve_int_rvalue(racc);
        } else if (instruction == Instruction::Add) {
            auto lacc = operand[0];
            auto racc1 = operand[1];
            auto racc2 = operand[2];
            resolve_int_lvalue(lacc) = resolve_int_rvalue(racc1) + resolve_int_rvalue(racc2);
        } else if (instruction == Instruction::Sub) {
            auto lacc = operand[0];
            auto racc1 = operand[1];
            auto racc2 = operand[2];
            resolve_int_lvalue(lacc) = resolve_int_rvalue(racc1) - resolve_int_rvalue(racc2);
        } else if (instruction == Instruction::Mul) {
            auto lacc = operand[0];
            auto racc1 = operand[1];
            auto racc2 = operand[2];
            resolve_int_lvalue(lacc) = resolve_int_rvalue(racc1) * resolve_int_rvalue(racc2);
        } else if (instruction == Instruction::Div) {
            auto lacc = operand[0];
            auto racc1 = operand[1];
            auto racc2 = operand[2];
            auto divisor = resolve_int_rvalue(racc2);
            if (divisor == 0) {
                throw std::runtime_error("PortableFunction::Run: division by zero");
            }
            resolve_int_lvalue(lacc) = resolve_int_rvalue(racc1) / divisor;
        } else if (instruction == Instruction::Mod) {
            auto lacc = operand[0];
            auto racc1 = operand[1];
            auto racc2 = operand[2];
            auto divisor = resolve_int_rvalue(racc2);
            if (divisor == 0) {
                throw std::runtime_error("PortableFunction::Run: division by zero");
            }
            resolve_int_lvalue(lacc) = resolve_int_rvalue(racc1) % divisor;
        } else if (instruction == Instruction::Eq) {
            auto lacc = operand[0];
            auto racc1 = operand[1];
            auto racc2 = operand[2];
            resolve_bool_lvalue(lacc) = resolve_int_rvalue(racc1) == resolve_int_rvalue(racc2);
        } else if (instruction == Instruction::Ne) {
            auto lacc = operand[0];
            auto racc1 = operand[1];
            auto racc2 = operand[2];
            resolve_bool_lvalue(lacc) = resolve_int_rvalue(racc1) != resolve_int_rvalue(racc2);
        } else if (instruction == Instruction::Lt) {
            auto lacc = operand[0];
            auto racc1 = operand[1];
            auto racc2 = operand[2];
            resolve_bool_lvalue(lacc) = resolve_int_rvalue(racc1) < resolve_int_rvalue(racc2);
        } else if (instruction == Instruction::Le) {
            auto lacc = operand[0];
            auto racc1 = operand[1];
            auto racc2 = operand[2];
            resolve_bool_lvalue(lacc) = resolve_int_rvalue(racc1) <= resolve_int_rvalue(racc2);
        } else if (instruction == Instruction::Gt) {
            auto lacc = operand[0];
            auto racc1 = operand[1];
            auto racc2 = operand[2];
            resolve_bool_lvalue(lacc) = resolve_int_rvalue(racc1) > resolve_int_rvalue(racc2);
        } else if (instruction == Instruction::Ge) {
            auto lacc = operand[0];
            auto racc1 = operand[1];
            auto racc2 = operand[2];
            resolve_bool_lvalue(lacc) = resolve_int_rvalue(racc1) >= resolve_int_rvalue(racc2);
        }
    }
    auto output_bool = std::map<std::string, Bool>{};
    auto output_int = std::map<std::string, Int>{};
    auto output_boolarray = std::map<std::string, std::vector<Bool>>{};
    auto output_intarray = std::map<std::string, std::vector<Int>>{};
    for (const auto& [k, v] : bool_variables) {
        if (output_.contains(k)) {
            output_bool[k] = v;
        }
    }
    for (const auto& [k, v] : int_variables) {
        if (output_.contains(k)) {
            output_int[k] = v;
        }
    }
    for (const auto& [k, v] : boolarray_variables) {
        auto to_bool_array = [&](const std::vector<char>& vc) {
            std::vector<Bool> vb(vc.size());
            std::copy(vc.begin(), vc.end(), vb.begin());
            return vb;
        };
        if (output_.contains(k)) {
            output_boolarray[k] = to_bool_array(v);
        }
    }
    for (const auto& [k, v] : intarray_variables) {
        if (output_.contains(k)) {
            output_intarray[k] = v;
        }
    }
    return PortableFunction::OutputValues{
            std::move(output_bool),
            std::move(output_int),
            std::move(output_boolarray),
            std::move(output_intarray)
    };
}

[[nodiscard]] std::vector<std::tuple<std::string, std::vector<PortableFunction::RegisterAccessor>>>
PortableFunction::DumpInstructions() const {
    auto dumped_instructions =
            std::vector<std::tuple<std::string, std::vector<RegisterAccessor>>>();
    dumped_instructions.reserve(instructions_.size());
    for (const auto& [inst, operands] : instructions_) {
        const auto* inst_str = [](Instruction inst) {
            if (inst == Instruction::CopyBool) {
                return "CopyBool";
            } else if (inst == Instruction::CopyInt) {
                return "CopyInt";
            } else if (inst == Instruction::BoolToInt) {
                return "BoolToInt";
            } else if (inst == Instruction::IntToBool) {
                return "IntToBool";
            } else if (inst == Instruction::LNot) {
                return "LNot";
            } else if (inst == Instruction::LAnd) {
                return "LAnd";
            } else if (inst == Instruction::LOr) {
                return "LOr";
            } else if (inst == Instruction::LXor) {
                return "LXor";
            } else if (inst == Instruction::BNot) {
                return "BNot";
            } else if (inst == Instruction::BAnd) {
                return "BAnd";
            } else if (inst == Instruction::BOr) {
                return "BOr";
            } else if (inst == Instruction::BXor) {
                return "BXor";
            } else if (inst == Instruction::LShift) {
                return "LShift";
            } else if (inst == Instruction::RShift) {
                return "RShift";
            } else if (inst == Instruction::Neg) {
                return "Neg";
            } else if (inst == Instruction::Add) {
                return "Add";
            } else if (inst == Instruction::Sub) {
                return "Sub";
            } else if (inst == Instruction::Mul) {
                return "Mul";
            } else if (inst == Instruction::Div) {
                return "Div";
            } else if (inst == Instruction::Mod) {
                return "Mod";
            } else if (inst == Instruction::Eq) {
                return "Eq";
            } else if (inst == Instruction::Ne) {
                return "Ne";
            } else if (inst == Instruction::Lt) {
                return "Lt";
            } else if (inst == Instruction::Le) {
                return "Le";
            } else if (inst == Instruction::Gt) {
                return "Gt";
            } else if (inst == Instruction::Ge) {
                return "Ge";
            }
            assert(0);
            return "Unknown";
        }(inst);
        dumped_instructions.emplace_back(inst_str, operands);
    }
    return dumped_instructions;
}
void PortableFunction::LoadInstructions(
        const std::vector<std::tuple<std::string, std::vector<RegisterAccessor>>>&
                dumped_instructions
) {
    instructions_.reserve(instructions_.size() + dumped_instructions.size());
    for (const auto& [inst_str, operands] : dumped_instructions) {
        auto inst = [](const std::string& inst_str) {
            if (inst_str == "CopyBool") {
                return Instruction::CopyBool;
            } else if (inst_str == "CopyInt") {
                return Instruction::CopyInt;
            } else if (inst_str == "BoolToInt") {
                return Instruction::BoolToInt;
            } else if (inst_str == "IntToBool") {
                return Instruction::IntToBool;
            } else if (inst_str == "LNot") {
                return Instruction::LNot;
            } else if (inst_str == "LAnd") {
                return Instruction::LAnd;
            } else if (inst_str == "LOr") {
                return Instruction::LOr;
            } else if (inst_str == "LXor") {
                return Instruction::LXor;
            } else if (inst_str == "BNot") {
                return Instruction::BNot;
            } else if (inst_str == "BAnd") {
                return Instruction::BAnd;
            } else if (inst_str == "BOr") {
                return Instruction::BOr;
            } else if (inst_str == "BXor") {
                return Instruction::BXor;
            } else if (inst_str == "LShift") {
                return Instruction::LShift;
            } else if (inst_str == "RShift") {
                return Instruction::RShift;
            } else if (inst_str == "Neg") {
                return Instruction::Neg;
            } else if (inst_str == "Add") {
                return Instruction::Add;
            } else if (inst_str == "Sub") {
                return Instruction::Sub;
            } else if (inst_str == "Mul") {
                return Instruction::Mul;
            } else if (inst_str == "Div") {
                return Instruction::Div;
            } else if (inst_str == "Mod") {
                return Instruction::Mod;
            } else if (inst_str == "Eq") {
                return Instruction::Eq;
            } else if (inst_str == "Ne") {
                return Instruction::Ne;
            } else if (inst_str == "Lt") {
                return Instruction::Lt;
            } else if (inst_str == "Le") {
                return Instruction::Le;
            } else if (inst_str == "Gt") {
                return Instruction::Gt;
            } else if (inst_str == "Ge") {
                return Instruction::Ge;
            }
            throw std::runtime_error("unknown instruction: " + inst_str);
        }(inst_str);
        instructions_.emplace_back(static_cast<Instruction>(inst), operands);
    }
}

PortableFunction::RegisterType PortableFunction::GetRegisterType(
        const std::string& key_name
) const {
    if (auto it = input_.find(key_name); it != input_.end()) {
        return it->second;
    }
    if (auto it = output_.find(key_name); it != output_.end()) {
        return it->second;
    }
    if (auto it = tmp_variable_.find(key_name); it != tmp_variable_.end()) {
        return it->second;
    }
    throw std::runtime_error("no register named " + key_name);
}
PortableFunction::Type PortableFunction::GetType(const RegisterAccessor& acc) const {
    if (acc.IsImmediate()) {
        return Type::Immediate();
    } else if (acc.IsDirectAccess()) {
        return GetRegisterType(acc.GetDirectAccess());
    } else if (acc.IsIndexedAccessByImmediate()) {
        auto arr = GetRegisterType(acc.GetIndexedAccessByImmediate().first);
        if (arr.IsBoolArray()) {
            return RegisterType::Bool();
        } else if (arr.IsIntArray()) {
            return RegisterType::Int();
        }

        throw std::runtime_error("indexed access must be applied to array type");
    }
    if (acc.IsIndexedAccessByRegister()) {
        auto [arr_reg, idx_reg] = acc.GetIndexedAccessByRegister();
        if (!GetRegisterType(idx_reg).IsInt()) {
            throw std::runtime_error("index must be int type");
        }
        auto arr = GetRegisterType(arr_reg);
        if (arr.IsBoolArray()) {
            return RegisterType::Bool();
        } else if (arr.IsIntArray()) {
            return RegisterType::Int();
        }

        throw std::runtime_error("indexed access must be applied to array type");
    }
    assert(0);
    return Type::Immediate();
}

void PortableFunction::AddAssignInstruction(
        const RegisterAccessor& dst,
        const RegisterAccessor& src,
        Instruction instruction
) {
    auto dst_type_ = GetType(dst);
    auto src_type_ = GetType(src);
    if (dst_type_.is_immediate) {
        throw std::runtime_error("dst must not be immediate value");
    }
    auto dst_type = dst_type_.type;
    if (instruction == Instruction::CopyBool || instruction == Instruction::IntToBool) {
        if (!dst_type.IsBool()) {
            throw std::runtime_error("dst must be Bool");
        }
    } else {
        if (!dst_type.IsInt()) {
            throw std::runtime_error("dst must be Int");
        }
    }
    if (instruction == Instruction::CopyBool || instruction == Instruction::BoolToInt) {
        if (!src_type_.is_immediate && !src_type_.type.IsBool()) {
            throw std::runtime_error("src must be Bool or immediate");
        }
    } else {
        if (!src_type_.is_immediate && !src_type_.type.IsInt()) {
            throw std::runtime_error("src must be Int or immediate");
        }
    }
    instructions_.emplace_back(instruction, std::vector{dst, src});
}

void PortableFunction::AddUnaryBoolInstruction(
        const RegisterAccessor& dst,
        const RegisterAccessor& src,
        Instruction instruction
) {
    auto dst_type_ = GetType(dst);
    auto src_type_ = GetType(src);
    if (dst_type_.is_immediate) {
        throw std::runtime_error("dst must not be immediate value");
    }
    auto dst_type = dst_type_.type;
    auto src_type = src_type_.is_immediate ? dst_type : src_type_.type;
    if (!dst_type.IsBool()) {
        throw std::runtime_error("dst must be Bool");
    }
    if (!src_type.IsBool()) {
        throw std::runtime_error("src must be Bool or immediate");
    }
    instructions_.emplace_back(instruction, std::vector{dst, src});
}
void PortableFunction::AddBinaryBoolInstruction(
        const RegisterAccessor& dst,
        const RegisterAccessor& src1,
        const RegisterAccessor& src2,
        Instruction instruction
) {
    auto dst_type_ = GetType(dst);
    auto src1_type_ = GetType(src1);
    auto src2_type_ = GetType(src2);
    if (dst_type_.is_immediate) {
        throw std::runtime_error("dst must not be immediate value");
    }
    auto dst_type = dst_type_.type;
    auto src1_type = src1_type_.is_immediate ? dst_type : src1_type_.type;
    auto src2_type = src2_type_.is_immediate ? dst_type : src2_type_.type;
    if (!dst_type.IsBool()) {
        throw std::runtime_error("dst must be Bool.");
    }
    if (!src1_type.IsBool()) {
        throw std::runtime_error("src1 must be Bool or immediate");
    }
    if (!src2_type.IsBool()) {
        throw std::runtime_error("src2 must be Bool or immediate");
    }
    instructions_.emplace_back(instruction, std::vector{dst, src1, src2});
}
void PortableFunction::AddUnaryIntInstruction(
        const RegisterAccessor& dst,
        const RegisterAccessor& src,
        Instruction instruction
) {
    auto dst_type_ = GetType(dst);
    auto src_type_ = GetType(src);
    if (dst_type_.is_immediate) {
        throw std::runtime_error("dst must not be immediate value");
    }
    auto dst_type = dst_type_.type;
    auto src_type = src_type_.is_immediate ? dst_type : src_type_.type;
    if (!dst_type.IsInt()) {
        throw std::runtime_error("dst must be Int");
    }
    if (!src_type.IsInt()) {
        throw std::runtime_error("src must be Int or immediate");
    }
    instructions_.emplace_back(instruction, std::vector{dst, src});
}
void PortableFunction::AddBinaryIntInstruction(
        const RegisterAccessor& dst,
        const RegisterAccessor& src1,
        const RegisterAccessor& src2,
        Instruction instruction
) {
    auto dst_type_ = GetType(dst);
    auto src1_type_ = GetType(src1);
    auto src2_type_ = GetType(src2);
    if (dst_type_.is_immediate) {
        throw std::runtime_error("dst must not be immediate value");
    }
    auto dst_type = dst_type_.type;
    auto src1_type = src1_type_.is_immediate ? dst_type : src1_type_.type;
    auto src2_type = src2_type_.is_immediate ? dst_type : src2_type_.type;
    if (!dst_type.IsInt()) {
        throw std::runtime_error("dst must be Int.");
    }
    if (!src1_type.IsInt()) {
        throw std::runtime_error("src1 must be Int or immediate");
    }
    if (!src2_type.IsInt()) {
        throw std::runtime_error("src2 must be Int or immediate");
    }
    instructions_.emplace_back(instruction, std::vector{dst, src1, src2});
}
void PortableFunction::AddCompareInstruction(
        const RegisterAccessor& dst,
        const RegisterAccessor& src1,
        const RegisterAccessor& src2,
        Instruction instruction
) {
    auto dst_type_ = GetType(dst);
    auto src1_type_ = GetType(src1);
    auto src2_type_ = GetType(src2);
    if (dst_type_.is_immediate) {
        throw std::runtime_error("dst must not be immediate value");
    }
    auto dst_type = dst_type_.type;
    auto src1_type = src1_type_.is_immediate ? RegisterType::Int() : src1_type_.type;
    auto src2_type = src2_type_.is_immediate ? RegisterType::Int() : src2_type_.type;
    if (!dst_type.IsBool()) {
        throw std::runtime_error("dst must be Bool");
    }
    if (!src1_type.IsInt()) {
        throw std::runtime_error("src1 must be Int or immediate");
    }
    if (!src2_type.IsInt()) {
        throw std::runtime_error("src2 must be Int or immediate");
    }
    instructions_.emplace_back(instruction, std::vector{dst, src1, src2});
}

PortableFunctionBuilder::PrimitiveVariable<PortableFunction::Bool>
PortableFunctionBuilder::AddBoolInputVariable(const std::string& name) {
    if (name.length() >= 1 && name[0] == '_') {
        throw std::runtime_error("variable name start with underscore is invalid");
    }
    if (input_.contains(name) || output_.contains(name) || tmp_variable_.contains(name)) {
        throw std::runtime_error("variable named " + name + " is already used");
    }
    input_[name] = PortableFunction::RegisterType::Bool();
    return PrimitiveVariable<Bool>(*this, {name});
}

PortableFunctionBuilder::PrimitiveVariable<PortableFunction::Int>
PortableFunctionBuilder::AddIntInputVariable(const std::string& name) {
    if (name.length() >= 1 && name[0] == '_') {
        throw std::runtime_error("variable name start with underscore is invalid");
    }
    if (input_.contains(name) || output_.contains(name) || tmp_variable_.contains(name)) {
        throw std::runtime_error("variable named " + name + " is already used");
    }
    input_[name] = PortableFunction::RegisterType::Int();
    return PrimitiveVariable<Int>(*this, {name});
}

PortableFunctionBuilder::ArrayVariable<PortableFunction::Bool>
PortableFunctionBuilder::AddBoolArrayInputVariable(const std::string& name, std::size_t size) {
    if (name.length() >= 1 && name[0] == '_') {
        throw std::runtime_error("variable name start with underscore is invalid");
    }
    if (input_.contains(name) || output_.contains(name) || tmp_variable_.contains(name)) {
        throw std::runtime_error("variable named " + name + " is already used");
    }
    input_[name] = PortableFunction::RegisterType::BoolArray(size);
    return ArrayVariable<Bool>(*this, {name});
}

PortableFunctionBuilder::ArrayVariable<PortableFunction::Int>
PortableFunctionBuilder::AddIntArrayInputVariable(const std::string& name, std::size_t size) {
    if (name.length() >= 1 && name[0] == '_') {
        throw std::runtime_error("variable name start with underscore is invalid");
    }
    if (input_.contains(name) || output_.contains(name) || tmp_variable_.contains(name)) {
        throw std::runtime_error("variable named " + name + " is already used");
    }
    input_[name] = PortableFunction::RegisterType::IntArray(size);
    return ArrayVariable<Int>(*this, {name});
}

PortableFunctionBuilder::PrimitiveVariable<PortableFunction::Bool>
PortableFunctionBuilder::AddBoolOutputVariable(const std::string& name) {
    if (name.length() >= 1 && name[0] == '_') {
        throw std::runtime_error("variable name start with underscore is invalid");
    }
    if (input_.contains(name) || output_.contains(name) || tmp_variable_.contains(name)) {
        throw std::runtime_error("variable named " + name + " is already used");
    }
    output_[name] = PortableFunction::RegisterType::Bool();
    return PrimitiveVariable<Bool>(*this, {name});
}

PortableFunctionBuilder::PrimitiveVariable<PortableFunction::Int>
PortableFunctionBuilder::AddIntOutputVariable(const std::string& name) {
    if (name.length() >= 1 && name[0] == '_') {
        throw std::runtime_error("variable name start with underscore is invalid");
    }
    if (input_.contains(name) || output_.contains(name) || tmp_variable_.contains(name)) {
        throw std::runtime_error("variable named " + name + " is already used");
    }
    output_[name] = PortableFunction::RegisterType::Int();
    return PrimitiveVariable<Int>(*this, {name});
}

PortableFunctionBuilder::ArrayVariable<PortableFunction::Bool>
PortableFunctionBuilder::AddBoolArrayOutputVariable(const std::string& name, std::size_t size) {
    if (name.length() >= 1 && name[0] == '_') {
        throw std::runtime_error("variable name start with underscore is invalid");
    }
    if (input_.contains(name) || output_.contains(name) || tmp_variable_.contains(name)) {
        throw std::runtime_error("variable named " + name + " is already used");
    }
    output_[name] = PortableFunction::RegisterType::BoolArray(size);
    return ArrayVariable<Bool>(*this, {name});
}

PortableFunctionBuilder::ArrayVariable<PortableFunction::Int>
PortableFunctionBuilder::AddIntArrayOutputVariable(const std::string& name, std::size_t size) {
    if (name.length() >= 1 && name[0] == '_') {
        throw std::runtime_error("variable name start with underscore is invalid");
    }
    if (input_.contains(name) || output_.contains(name) || tmp_variable_.contains(name)) {
        throw std::runtime_error("variable named " + name + " is already used");
    }
    output_[name] = PortableFunction::RegisterType::IntArray(size);
    return ArrayVariable<Int>(*this, {name});
}

PortableFunctionBuilder::PrimitiveVariable<PortableFunction::Bool>
PortableFunctionBuilder::AddTmpBoolVariable() {
    auto ss = std::stringstream{};
    ss << "_bool_" << std::setw(9) << std::setfill('0') << num_tmp_bool_++;
    tmp_variable_[ss.str()] = PortableFunction::RegisterType::Bool();
    return PrimitiveVariable<Bool>(*this, {ss.str()});
}
PortableFunctionBuilder::PrimitiveVariable<PortableFunction::Int>
PortableFunctionBuilder::AddTmpIntVariable() {
    auto ss = std::stringstream{};
    ss << "_int_" << std::setw(9) << std::setfill('0') << num_tmp_int_++;
    tmp_variable_[ss.str()] = PortableFunction::RegisterType::Int();
    return PrimitiveVariable<Int>(*this, {ss.str()});
}
PortableFunctionBuilder::ArrayVariable<PortableFunction::Bool>
PortableFunctionBuilder::AddTmpBoolArrayVariable(std::size_t size) {
    bool_array_sizes_.push_back(size);
    auto ss = std::stringstream{};
    ss << "_boolarray_" << std::setw(9) << std::setfill('0') << num_tmp_boolarray_++;
    tmp_variable_[ss.str()] = PortableFunction::RegisterType::BoolArray(size);
    return ArrayVariable<Bool>(*this, {ss.str()});
}
PortableFunctionBuilder::ArrayVariable<PortableFunction::Int>
PortableFunctionBuilder::AddTmpIntArrayVariable(std::size_t size) {
    int_array_sizes_.push_back(size);
    auto ss = std::stringstream{};
    ss << "_intarray_" << std::setw(9) << std::setfill('0') << num_tmp_intarray_++;
    tmp_variable_[ss.str()] = PortableFunction::RegisterType::IntArray(size);
    return ArrayVariable<Int>(*this, {ss.str()});
}

PortableFunctionBuilder::PrimitiveVariable<PortableFunction::Bool>
PortableFunctionBuilder::AssignInternalBoolVariable() {
    bool newly_assign = internal_bool_pool_.empty();
    if (newly_assign) {
        internal_bool_pool_.push_back(num_internal_bool_++);
        internal_bool_refcount_.push_back(0);
    }
    std::size_t id = internal_bool_pool_.front();
    internal_bool_pool_.pop_front();
    auto ss = std::stringstream{};
    ss << "_internal_bool_" << std::setw(9) << std::setfill('0') << id;
    if (newly_assign) {
        tmp_variable_[ss.str()] = PortableFunction::RegisterType::Bool();
    }
    return PrimitiveVariable<Bool>(*this, {ss.str()});
}
PortableFunctionBuilder::PrimitiveVariable<PortableFunction::Int>
PortableFunctionBuilder::AssignInternalIntVariable() {
    bool newly_assign = internal_int_pool_.empty();
    if (newly_assign) {
        internal_int_pool_.push_back(num_internal_int_++);
        internal_int_refcount_.push_back(0);
    }
    std::size_t id = internal_int_pool_.front();
    internal_int_pool_.pop_front();
    auto ss = std::stringstream{};
    ss << "_internal_int_" << std::setw(9) << std::setfill('0') << id;
    if (newly_assign) {
        tmp_variable_[ss.str()] = PortableFunction::RegisterType::Int();
    }
    return PrimitiveVariable<Int>(*this, {ss.str()});
}
PortableFunctionBuilder::ArrayVariable<PortableFunction::Bool>
PortableFunctionBuilder::AssignInternalBool2Variable() {
    bool newly_assign = internal_bool2_pool_.empty();
    if (newly_assign) {
        internal_bool2_pool_.push_back(num_internal_bool2_++);
        internal_bool2_refcount_.push_back(0);
    }
    std::size_t id = internal_bool2_pool_.front();
    internal_bool2_pool_.pop_front();
    auto ss = std::stringstream{};
    ss << "_internal_bool2_" << std::setw(9) << std::setfill('0') << id;
    if (newly_assign) {
        tmp_variable_[ss.str()] = PortableFunction::RegisterType::BoolArray(2);
    }
    return ArrayVariable<Bool>(*this, {ss.str()});
}
PortableFunctionBuilder::ArrayVariable<PortableFunction::Int>
PortableFunctionBuilder::AssignInternalInt2Variable() {
    bool newly_assign = internal_int2_pool_.empty();
    if (newly_assign) {
        internal_int2_pool_.push_back(num_internal_int2_++);
        internal_int2_refcount_.push_back(0);
    }
    std::size_t id = internal_int2_pool_.front();
    internal_int2_pool_.pop_front();
    auto ss = std::stringstream{};
    ss << "_internal_int2_" << std::setw(9) << std::setfill('0') << id;
    if (newly_assign) {
        tmp_variable_[ss.str()] = PortableFunction::RegisterType::IntArray(2);
    }
    return ArrayVariable<Int>(*this, {ss.str()});
}

void PortableFunctionBuilder::ReleaseInternalBoolVariable(std::size_t id) {
    internal_bool_pool_.push_back(id);
}
void PortableFunctionBuilder::ReleaseInternalIntVariable(std::size_t id) {
    internal_int_pool_.push_back(id);
}
void PortableFunctionBuilder::ReleaseInternalBool2Variable(std::size_t id) {
    internal_bool2_pool_.push_back(id);
}
void PortableFunctionBuilder::ReleaseInternalInt2Variable(std::size_t id) {
    internal_int2_pool_.push_back(id);
}

void PortableFunctionBuilder::IncrementInternalBoolRefCount(std::size_t id) {
    internal_bool_refcount_[id]++;
}
void PortableFunctionBuilder::IncrementInternalIntRefCount(std::size_t id) {
    internal_int_refcount_[id]++;
}
void PortableFunctionBuilder::IncrementInternalBool2RefCount(std::size_t id) {
    internal_bool2_refcount_[id]++;
}
void PortableFunctionBuilder::IncrementInternalInt2RefCount(std::size_t id) {
    internal_int2_refcount_[id]++;
}
void PortableFunctionBuilder::DecrementInternalBoolRefCount(std::size_t id) {
    if (--internal_bool_refcount_[id] == 0) {
        ReleaseInternalBoolVariable(id);
    }
}
void PortableFunctionBuilder::DecrementInternalIntRefCount(std::size_t id) {
    if (--internal_int_refcount_[id] == 0) {
        ReleaseInternalIntVariable(id);
    }
}
void PortableFunctionBuilder::DecrementInternalBool2RefCount(std::size_t id) {
    if (--internal_bool2_refcount_[id] == 0) {
        ReleaseInternalBool2Variable(id);
    }
}
void PortableFunctionBuilder::DecrementInternalInt2RefCount(std::size_t id) {
    if (--internal_int2_refcount_[id] == 0) {
        ReleaseInternalInt2Variable(id);
    }
}

PortableFunction PortableFunctionBuilder::Compile(std::string_view name) const {
    auto input = std::map<std::string, PortableFunction::RegisterType>{};
    for (const auto& [key, val] : input_) {
        input[key] = val;
    }
    auto output = std::map<std::string, PortableFunction::RegisterType>{};
    for (const auto& [key, val] : output_) {
        output[key] = val;
    }
    auto tmp = std::map<std::string, PortableFunction::RegisterType>{};
    for (const auto& [key, val] : tmp_variable_) {
        tmp[key] = val;
    }
    auto function = PortableFunction{name, input, output, tmp};
    function.LoadInstructions(instructions_);
    return function;
}

PortableFunction::RegisterType PortableFunctionBuilder::GetRegisterType(
        const std::string& key_name
) const {
    if (auto it = input_.find(key_name); it != input_.end()) {
        return it->second;
    }
    if (auto it = output_.find(key_name); it != output_.end()) {
        return it->second;
    }
    if (auto it = tmp_variable_.find(key_name); it != tmp_variable_.end()) {
        return it->second;
    }

    throw std::logic_error("unreachable");
}
}  // namespace qret
