/**
 * @file qret/frontend/builder.cpp
 * @brief Builder of quantum circuits.
 */

#include "qret/frontend/builder.h"

#include <fmt/format.h>

#include <memory>
#include <stdexcept>
#include <string_view>

#include "qret/base/log.h"
#include "qret/frontend/circuit.h"
#include "qret/ir/basic_block.h"
#include "qret/ir/context.h"
#include "qret/ir/function.h"
#include "qret/ir/instruction.h"
#include "qret/ir/module.h"

namespace qret::frontend {
CircuitBuilder::CircuitBuilder(ir::Module* module)
    : module_{module} {
    if (module == nullptr) {
        throw std::runtime_error("module must not be nullptr");
    }
}
void CircuitBuilder::LoadModule(ir::Module* module) {
    for (auto& ir_circuit : *module) {
        // Check name.
        if (Contains(ir_circuit.GetName())) {
            throw std::runtime_error(
                    fmt::format("Circuit name '{}' already exists", ir_circuit.GetName())
            );
        }

        // Create a frontend circuit corresponding to ir circuit.
        const auto [itr, success_to_emplace] = cached_circuits_.emplace(
                ir_circuit.GetName(),
                std::unique_ptr<Circuit>(new Circuit(this, &ir_circuit))
        );
        if (!success_to_emplace) {
            throw std::runtime_error(
                    fmt::format("Failed to emplace circuit: {}", ir_circuit.GetName())
            );
        }
        inv_map_[&ir_circuit] = itr->second.get();

        // Set as loaded from ir.
        itr->second->SetLoadedFromIR(true);
    }
}
bool CircuitBuilder::Contains(std::string_view key) const {
    return cached_circuits_.contains(std::string(key));
}
Circuit* CircuitBuilder::GetCircuit(std::string_view key) const {
    auto itr = cached_circuits_.find(std::string(key));
    return itr == cached_circuits_.end() ? nullptr : itr->second.get();
}
std::vector<std::string> CircuitBuilder::GetCircuitList() const {
    auto ret = std::vector<std::string>();
    for (const auto& [name, _] : cached_circuits_) {
        ret.emplace_back(name);
    }
    return ret;
}
Circuit* CircuitBuilder::CreateCircuit(std::string_view unique_name) {
    if (unique_name.empty()) {
        throw std::runtime_error("empty circuit name");
    }
    if (Contains(unique_name)) {
        throw std::runtime_error("duplicate circuit name");
    }
    auto* ir = ir::Function::Create(unique_name, module_);
    const auto [itr, _] =
            cached_circuits_.emplace(unique_name, std::unique_ptr<Circuit>(new Circuit(this, ir)));
    name_hist_[ir->GetName()]++;
    inv_map_[ir] = itr->second.get();
    return itr->second.get();
}
Circuit* CircuitBuilder::CreateCircuitWithoutCaching(std::string_view name) {
    if (name.empty()) {
        throw std::runtime_error("empty circuit name");
    }
    auto itr = name_hist_.find(name);
    auto unique_name = std::string();
    if (itr == name_hist_.end()) {
        // unique
        unique_name = fmt::format("{}__{:09}__", name, 0);
    } else {
        unique_name = fmt::format("{}__{:09}__", name, itr->second);
    }
    auto* ir = ir::Function::Create(unique_name, module_);
    unnamed_circuits_.emplace_back(std::unique_ptr<Circuit>(new Circuit(this, ir)));
    auto sv = ir->GetName();
    sv.remove_suffix(13);
    name_hist_[sv]++;
    inv_map_[ir] = unnamed_circuits_.back().get();
    return unnamed_circuits_.back().get();
}
void CircuitBuilder::BeginCircuitDefinition(Circuit* circuit) {
    PushContext(circuit, ir::BasicBlock::Create("entry", circuit->GetIR()));
}
void CircuitBuilder::EndCircuitDefinition(Circuit* circuit) {
    if (GetCurrentBuildContext()->circuit != circuit) {
        throw std::runtime_error(
                "definition of some circuits is not ended\nCall "
                "CircuitGenerator::EndCircuitDefinition()"
        );
    }
    if (GetCurrentBuildContext()->IsBranching()) {
        throw std::runtime_error("some branch is open\nCall EndIf or EndSwitch to close branching");
    }

    // Insert return at the end of circuit.
    ir::ReturnInst::Create(GetCurrentBuildContext()->insertion_point);

    // Set entry BB and arguments in IR.
    circuit->GetIR()->SetEntryBB(circuit->GetIR()->begin().GetPointer());
    const auto& argument = circuit->GetArgument();
    for (const auto& [name, type, size, attr] : argument) {
        using Type = Circuit::Type;
        if (type == Type::Qubit) {
            circuit->GetIR()->AddQubit(name, size);
        } else {
            circuit->GetIR()->AddRegister(name, size);
        }
    }

    // Set BB's dependency
    circuit->GetIR()->SetBBDeps();

    // Minimize BB
    // TODO: Move these operations to some IR's optimization pass
    circuit->GetIR()->RemoveUnreachableBBs();
    circuit->GetIR()->RemoveEmptyBBs();

    // Check BB's dependency
    if (!circuit->GetIR()->IsDAG()) {
        LOG_WARN("the dependency graph of BB is not DAG (circuit: {})", circuit->GetName());
    }

    PopContext();
}
void CircuitBuilder::PushContext(Circuit* circuit, ir::BasicBlock* insertion_point) {
    context_.emplace_back(std::make_unique<BuildContext>());
    context_.back()->circuit = circuit;
    context_.back()->insertion_point = insertion_point;
}
void CircuitBuilder::PopContext() {
    context_.pop_back();
}
}  // namespace qret::frontend
