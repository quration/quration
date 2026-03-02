/**
 * @file qret/frontend/circuit.cpp
 * @brief Quantum circuits of frontend.
 */

#include "qret/frontend/circuit.h"

namespace qret::frontend {
std::size_t Circuit::Argument::Count(Type type) const {
    auto ret = std::size_t{0};
    for (auto i = std::size_t{0}; i < GetNumArgs(); ++i) {
        if (GetType(i) == type) {
            ret += GetSize(i);
        }
    }
    return ret;
}
std::size_t Circuit::Argument::Count(Type type, Attribute attr) const {
    auto ret = std::size_t{0};
    for (auto i = std::size_t{0}; i < GetNumArgs(); ++i) {
        if (GetType(i) == type && GetAttribute(i) == attr) {
            ret += GetSize(i);
        }
    }
    return ret;
}
std::size_t Circuit::Argument::GetNumQubits() const {
    return Count(Type::Qubit);
}
std::size_t Circuit::Argument::GetNumOperateQubits() const {
    return Count(Type::Qubit, Attribute::Operate);
}
std::size_t Circuit::Argument::GetNumCleanAncillae() const {
    return Count(Type::Qubit, Attribute::CleanAncilla);
}
std::size_t Circuit::Argument::GetNumDirtyAncillae() const {
    return Count(Type::Qubit, Attribute::DirtyAncilla);
}
std::size_t Circuit::Argument::GetNumCatalysts() const {
    return Count(Type::Qubit, Attribute::Catalyst);
}
std::size_t Circuit::Argument::GetNumRegisters() const {
    return Count(Type::Register);
}
std::size_t Circuit::Argument::GetNumInputRegisters() const {
    return Count(Type::Register, Attribute::Input);
}
std::size_t Circuit::Argument::GetNumOutputRegisters() const {
    return Count(Type::Register, Attribute::Output);
}
}  // namespace qret::frontend
