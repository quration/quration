/**
 * @file qret/frontend/circuit_generator.cpp
 * @brief Generator of quantum circuits.
 */

#include "qret/frontend/circuit_generator.h"

#include <fmt/core.h>

#include <stdexcept>

#include "qret/frontend/attribute.h"
#include "qret/frontend/builder.h"

namespace qret::frontend {
bool CircuitGenerator::IsCached() const {
    const auto cache_key = GetCacheKey();
    if (cache_key.empty()) {
        return false;
    }
    return GetBuilder()->Contains(cache_key);
}
Circuit* CircuitGenerator::GetCachedCircuit() const {
    return GetBuilder()->GetCircuit(GetCacheKey());
}
Qubit CircuitGenerator::GetQubit(std::size_t arg_idx) const {
    const auto& argument = GetCircuit()->GetArgument();
    if (!argument.IsQubit(arg_idx)) {
        throw std::runtime_error(fmt::format("{}-th argument is not Qubit", arg_idx));
    }
    auto id = std::size_t{0};
    for (auto i = std::size_t{0}; i < arg_idx; ++i) {
        if (argument.IsQubits(i)) {
            id += argument.GetSize(i);
        }
    }
    return {GetBuilder(), id};
}
Qubits CircuitGenerator::GetQubits(std::size_t arg_idx) const {
    const auto& argument = GetCircuit()->GetArgument();
    if (!argument.IsQubits(arg_idx)) {
        throw std::runtime_error(fmt::format("{}-th argument is not Qubits", arg_idx));
    }
    auto id = std::size_t{0};
    for (auto i = std::size_t{0}; i < arg_idx; ++i) {
        if (argument.IsQubits(i)) {
            id += argument.GetSize(i);
        }
    }
    return {GetBuilder(), {id}, {argument.GetSize(arg_idx)}};
}
Register CircuitGenerator::GetRegister(std::size_t arg_idx) const {
    const auto& argument = GetCircuit()->GetArgument();
    if (!argument.IsRegister(arg_idx)) {
        throw std::runtime_error(fmt::format("{}-th argument is not Register", arg_idx));
    }
    auto id = std::size_t{0};
    for (auto i = std::size_t{0}; i < arg_idx; ++i) {
        if (argument.IsRegisters(i)) {
            id += argument.GetSize(i);
        }
    }
    return {GetBuilder(), id};
}
Registers CircuitGenerator::GetRegisters(std::size_t arg_idx) const {
    const auto& argument = GetCircuit()->GetArgument();
    if (!argument.IsRegisters(arg_idx)) {
        throw std::runtime_error(fmt::format("{}-th argument is not Registers", arg_idx));
    }
    auto id = std::size_t{0};
    for (auto i = std::size_t{0}; i < arg_idx; ++i) {
        if (argument.IsRegisters(i)) {
            id += argument.GetSize(i);
        }
    }
    return {GetBuilder(), {id}, {argument.GetSize(arg_idx)}};
}
Register CircuitGenerator::GetTemporalRegister() const {
    GetCircuit()->GetIR()->IncNumTmpRegisters();
    return GetBuilder()->GetRegister(temporal_register_id_++);
}
Registers CircuitGenerator::GetTemporalRegisters(std::size_t size) const {
    for (auto i = std::size_t{0}; i < size; ++i) {
        GetCircuit()->GetIR()->IncNumTmpRegisters();
    }
    return GetBuilder()->GetRegisters(temporal_register_id_++, size);
}
void CircuitGenerator::BeginCircuitDefinition() const {
    const auto cache_key = GetCacheKey();
    if (cache_key.empty()) {
        circuit_ = GetBuilder()->CreateCircuitWithoutCaching(GetName());
    } else {
        circuit_ = GetBuilder()->CreateCircuit(GetCacheKey());
    }
    SetArgument(circuit_->GetMutArgument());
    GetBuilder()->BeginCircuitDefinition(circuit_);
    temporal_register_id_ = circuit_->GetArgument().GetNumRegisters();

    // Set attribute of arguments
    const auto& argument = circuit_->GetArgument();
    for (auto i = std::size_t{0}; i < argument.GetNumArgs(); ++i) {
        if (argument.GetAttribute(i) == Attribute::CleanAncilla) {
            if (argument.IsQubit(i)) {
                const auto q = GetQubit(i);
                attribute::MarkAsClean(q);
            } else {
                const auto qs = GetQubits(i);
                for (const auto& q : qs) {
                    attribute::MarkAsClean(q);
                }
            }
        } else if (argument.GetAttribute(i) == Attribute::DirtyAncilla) {
            if (argument.IsQubit(i)) {
                const auto q = GetQubit(i);
                attribute::MarkAsDirtyBegin(q);
            } else {
                const auto qs = GetQubits(i);
                for (const auto& q : qs) {
                    attribute::MarkAsDirtyBegin(q);
                }
            }
        }
    }
}
Circuit* CircuitGenerator::EndCircuitDefinition() const {
    // Set attribute of arguments
    const auto& argument = circuit_->GetArgument();
    for (auto i = std::size_t{0}; i < argument.GetNumArgs(); ++i) {
        if (argument.GetAttribute(i) == Attribute::CleanAncilla) {
            if (argument.IsQubit(i)) {
                const auto q = GetQubit(i);
                attribute::MarkAsClean(q);
            } else {
                const auto qs = GetQubits(i);
                for (const auto& q : qs) {
                    attribute::MarkAsClean(q);
                }
            }
        } else if (argument.GetAttribute(i) == Attribute::DirtyAncilla) {
            if (argument.IsQubit(i)) {
                const auto q = GetQubit(i);
                attribute::MarkAsDirtyEnd(q);
            } else {
                const auto qs = GetQubits(i);
                for (const auto& q : qs) {
                    attribute::MarkAsDirtyEnd(q);
                }
            }
        }
    }

    GetBuilder()->EndCircuitDefinition(circuit_);

    // Generate adjoint
    if (!circuit_->HasAdjoint()) {
        auto* adjoint = GenerateAdjoint();
        if (adjoint != nullptr) {
            circuit_->SetAdjoint(adjoint);
            adjoint->SetAdjoint(circuit_);
        }
    }
    return circuit_;
}
}  // namespace qret::frontend
