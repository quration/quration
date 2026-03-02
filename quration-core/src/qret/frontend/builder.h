/**
 * @file qret/frontend/builder.h
 * @brief Builder of quantum circuits.
 * @details This file implements builder of quantum circuits.
 */

#ifndef QRET_FRONTEND_BUILDER_H
#define QRET_FRONTEND_BUILDER_H

#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

#include "qret/frontend/argument.h"
#include "qret/ir/basic_block.h"
#include "qret/ir/context.h"
#include "qret/ir/function.h"
#include "qret/ir/instruction.h"
#include "qret/ir/module.h"
#include "qret/qret_export.h"

namespace qret::frontend {
// Forward declarations
class QRET_EXPORT Circuit;
class QRET_EXPORT CircuitGenerator;
namespace control_flow {
QRET_EXPORT void If(const Register&);
QRET_EXPORT void If(const Register&, std::string);
QRET_EXPORT void Else(const Register&);
QRET_EXPORT void EndIf(const Register&);
QRET_EXPORT void Switch(const Registers&);
QRET_EXPORT void Switch(const Registers&, std::string);
QRET_EXPORT void Case(const Registers&, std::uint64_t);
QRET_EXPORT void Default(const Registers&);
QRET_EXPORT void EndSwitch(const Registers&);
}  // namespace control_flow

struct BranchContext {
    std::variant<Register, Registers> cond;

    std::string branch_name;
    ir::BasicBlock* bb_to_insert_branch = nullptr;

    ir::BasicBlock* if_true = nullptr;
    ir::BasicBlock* if_false = nullptr;
    ir::BasicBlock* if_cont = nullptr;

    ir::BasicBlock* default_bb = nullptr;
    std::map<std::uint64_t, ir::BasicBlock*> case_bb = {};
    ir::BasicBlock* switch_cont = nullptr;

    bool IsIfBlock() const {
        return std::get_if<Register>(&cond) != nullptr;
    }
    bool IsIfTrueBranch() const {
        return std::get_if<Register>(&cond) != nullptr && if_false == nullptr;
    }
    bool IsIfFalseBranch() const {
        return std::get_if<Register>(&cond) != nullptr && if_false != nullptr;
    }
    bool IsSwitchBlock() const {
        return std::get_if<Registers>(&cond) != nullptr;
    }

    bool SameCond(const Register& r) const {
        const auto* ptr = std::get_if<Register>(&cond);
        if (ptr == nullptr) {
            return false;
        }
        return r.GetId() == ptr->GetId();
    }
    bool SameValue(const Registers& rs) const {
        const auto* ptr = std::get_if<Registers>(&cond);
        if (ptr == nullptr) {
            return false;
        }
        if (rs.Size() != ptr->Size()) {
            return false;
        }
        for (std::size_t i = 0; i < rs.Size(); ++i) {
            if (rs[i].GetId() != (*ptr)[i].GetId()) {
                return false;
            }
        }

        return true;
    }
};

struct BuildContext {
    Circuit* circuit = nullptr;  //!< the circuit currently being defined
    ir::BasicBlock* insertion_point = nullptr;  //!< the basic block to insert instructions into

    std::list<std::unique_ptr<BranchContext>> branch = {};

    std::size_t num_if = 0;
    std::size_t num_switch = 0;

    bool IsBranching() const {
        return !branch.empty();
    }
};

/**
 * @brief Builder of qret::frontend::Circuit.
 * @details If the circuit's name is unique, cache it with that name as the key.
 */
class QRET_EXPORT CircuitBuilder {
public:
    explicit CircuitBuilder(ir::Module* module);

    CircuitBuilder(const CircuitBuilder&) = delete;
    CircuitBuilder(CircuitBuilder&&) = default;
    CircuitBuilder& operator=(const CircuitBuilder&) = delete;
    CircuitBuilder& operator=(CircuitBuilder&&) = delete;

    const ir::Module* GetModule() const {
        return module_;
    }

    /**
     * @brief Load ir module.
     *
     * @param module
     */
    void LoadModule(ir::Module* module);

    /**
     * @brief Determine if the "key" is the name of a circuit cached within the builder.
     *
     * @param key circuit name
     * @return true cached
     * @return false not cached
     */
    bool Contains(std::string_view key) const;
    /**
     * @brief Get the Circuit object if cached
     *
     * @param key circuit name
     * @return Circuit* if cached, return a pointer to the circuit; otherwise, return nullptr
     */
    Circuit* GetCircuit(std::string_view key) const;
    /**
     * @brief Get the name of all circuits created in this builder.
     *
     * @return std::vector<std::string> the name of all circuits created
     */
    std::vector<std::string> GetCircuitList() const;

    /**
     * @brief Get the qret::frontend::Circuit from qret::ir::Function.
     * @details qret::ir::Function must be created by this builder.
     *
     * @param ir circuit of IR
     * @return Circuit* circuit of frontend
     */
    Circuit* GetCircuitFromIR(ir::Function* ir) const {
        const auto itr = inv_map_.find(ir);
        if (itr != inv_map_.end()) {
            return itr->second;
        }
        return nullptr;
    }

    /**
     * @brief Create a qret::frontend::Circuit and cache it with the key named "unique_name" within
     * the builder.
     *
     * @param unique_name unique circuit name (the cache key)
     * @return Circuit* an empty circuit of frontend, to be defined further
     */
    Circuit* CreateCircuit(std::string_view unique_name);
    /**
     * @brief Create a qret::frontend::Circuit without caching.
     *
     * @param name circuit name
     * @return Circuit* an empty circuit of frontend, to be defined further
     */
    Circuit* CreateCircuitWithoutCaching(std::string_view name);

    /**
     * @brief Begin defining the circuit.
     * @details The circuit begun with BeginCircuitDefinition() must always call
     * EndCircuitDefinition().
     * It is possible to start defining another circuit while defining one circuit.
     *
     * How to use (cpp code):
     * \code{.cpp}
     * builder.BeginCircuitDefinition(circuit1);
     * // definition of circuit1
     * builder.BeginCircuitDefinition(circuit2);
     * // definition of circuit2
     * builder.EndCircuitDefinition(circuit2);
     * // definition of circuit1
     * builder.EndCircuitDefinition(circuit1);
     * \endcode
     *
     * @param circuit the circuit to begin defining
     */
    void BeginCircuitDefinition(Circuit* circuit);
    /**
     * @brief End defining the circuit.
     * @details The circuit calling EndCircuitDefinition must be the one beginning definition with
     * BeginCircuitDefinition immediately prior.
     *
     * @param circuit the circuit to end defining
     */
    void EndCircuitDefinition(Circuit* circuit);

    /**
     * @brief Get the circuit currently being defined.
     * @details Returns the circuit that was last started defining with BeginCircuitDefinition().
     *
     * @return Circuit* the circuit currently being defined
     */
    Circuit* GetCurrentCircuit() const {
        return GetCurrentBuildContext()->circuit;
    }
    /**
     * @brief Get the basic block begin inserted in the circuit currently begin defined.
     *
     * @return ir::BasicBlock* the basic block to insert instructions into
     */
    ir::BasicBlock* GetInsertionPoint() const {
        return GetCurrentBuildContext()->insertion_point;
    }

    // Create arguments
    Qubit GetQubit(std::uint64_t id) {
        return {this, id};
    }
    Qubits GetQubits(std::uint64_t offset, std::uint64_t size) {
        return {this, {offset}, {size}};
    }
    Register GetRegister(std::uint64_t id) {
        return {this, id};
    }
    Registers GetRegisters(std::uint64_t offset, std::uint64_t size) {
        return {this, {offset}, {size}};
    }

private:
    friend class CircuitGenerator;
    friend QRET_EXPORT void control_flow::If(const Register& cond);
    friend QRET_EXPORT void control_flow::If(const Register& cond, std::string);
    friend QRET_EXPORT void control_flow::Else(const Register& cond);
    friend QRET_EXPORT void control_flow::EndIf(const Register& cond);
    friend QRET_EXPORT void control_flow::Switch(const Registers&);
    friend QRET_EXPORT void control_flow::Switch(const Registers&, std::string);
    friend QRET_EXPORT void control_flow::Case(const Registers&, std::uint64_t);
    friend QRET_EXPORT void control_flow::Default(const Registers&);
    friend QRET_EXPORT void control_flow::EndSwitch(const Registers&);

    void PushContext(Circuit* circuit, ir::BasicBlock* insertion_point);
    void PopContext();

    BuildContext* GetCurrentBuildContext() const {
        return context_.back().get();
    }

    ir::Module* const module_;

    std::unordered_map<std::string, std::unique_ptr<Circuit>>
            cached_circuits_;  //!< key: unique circuit name, value: circuit
    std::vector<std::unique_ptr<Circuit>> unnamed_circuits_;
    std::unordered_map<std::string_view, std::uint32_t>
            name_hist_;  //!< a histogram of how many times circuits with the same name have been
                         //!< created

    std::unordered_map<const ir::Function*, Circuit*>
            inv_map_;  //!< map from ir circuit to frontend circuit

    /**
     * @brief Contexts of building.
     *
     *@details The stack of circuits begun definition, but not yet ended.
     */
    std::list<std::unique_ptr<BuildContext>> context_;
};
}  // namespace qret::frontend

#endif  // QRET_FRONTEND_BUILDER_H
