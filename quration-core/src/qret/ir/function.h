/**
 * @file qret/ir/function.h
 * @brief Function.
 */

#ifndef QRET_IR_FUNCTION_H
#define QRET_IR_FUNCTION_H

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "qret/base/iterator.h"
#include "qret/ir/basic_block.h"
#include "qret/ir/instruction.h"
#include "qret/math/boolean_formula.h"
#include "qret/qret_export.h"

namespace qret::ir {
// Forward declarations
class Module;

/**
 * @brief Function of qret IR.
 * @details The owner of functions is a module.
 */
class QRET_EXPORT Function {
public:
    struct QRET_EXPORT Argument {
        std::size_t num_qubits = 0;
        std::vector<std::pair<std::string, std::size_t>> qubits;
        std::size_t num_registers = 0;
        std::vector<std::pair<std::string, std::size_t>> registers;
    };

    using BBListType = std::list<std::unique_ptr<BasicBlock>>;

    Function(const Function&) = delete;
    Function& operator=(const Function&) = delete;

    /**
     * @brief Create a new Function with no parent.
     *
     * @param name func name
     * @return std::unique_ptr<Function> a new func
     */
    static std::unique_ptr<Function> Create(std::string_view name);
    /**
     * @brief Create a new Function.
     *
     * @param name func name
     * @param parent module owning a new func
     * @return Function* a new func
     */
    static Function* Create(std::string_view name, Module* parent);

    bool HasParent() const {
        return parent_ != nullptr;
    }

    /**
     * @brief Get the module owning this func, or nullptr if the func does not have a module.
     */
    const Module* GetParent() const {
        return parent_;
    }
    Module* GetParent() {
        return parent_;
    }

    std::string_view GetName() const {
        return name_;
    }

    void Clear() {
        bb_list_.clear();
        entry_point_ = bb_list_.begin();
        ptr2itr_.clear();
        argument_ = Argument();
        num_tmp_registers_ = 0;
    }

    /**
     * @brief Get the entry basic block, or nullptr if it is not set.
     */
    BasicBlock* GetEntryBB() const {
        return entry_point_->get();
    }
    BasicBlock* GetEntryBB() {
        return entry_point_->get();
    }
    void SetEntryBB(const BasicBlock* bb);

    const BasicBlock* SearchBB(std::string_view name) const {
        for (const auto& bb : bb_list_) {
            if (bb->GetName() == name) {
                return &(*bb);
            }
        }
        return nullptr;
    }
    BasicBlock* SearchBB(std::string_view name) {
        for (auto& bb : bb_list_) {
            if (bb->GetName() == name) {
                return &(*bb);
            }
        }
        return nullptr;
    }

    /**
     * @brief Set predecessors and successors of BBs.
     *
     * @return true successfully set dependencies.
     * @return false otherwise.
     */
    bool SetBBDeps();
    /**
     * @brief Remove BBs that are not reachable from the entry BB.
     * @note Dependencies of BBs must be set correctly before calling this method.
     *
     * @return true successfully remove BBs.
     * @return false otherwise.
     */
    bool RemoveUnreachableBBs();
    /**
     * @brief Remove bbs that contains only the unconditional branch instruction.
     * @note Dependencies of BBs must be set correctly before calling this method.
     *
     * @return true successfully remove BBs.
     * @return false otherwise.
     */
    bool RemoveEmptyBBs();

    /**
     * @brief Get the number of basic blocks in this func.
     */
    std::size_t GetNumBBs() const {
        return bb_list_.size();
    }

    /**
     * @brief Get the total number of instructions in this func.
     * @details The sum of instructions contained in the basic blocks in this func.
     */
    std::size_t GetInstructionCount() const;

    using iterator = ListIterator<BasicBlock, false>;
    using const_iterator = ListIterator<BasicBlock, true>;
    iterator begin() {
        return iterator{bb_list_.begin()};
    }  // NOLINT
    const_iterator begin() const {
        return const_iterator{bb_list_.cbegin()};
    }  // NOLINT
    iterator end() {
        return iterator{bb_list_.end()};
    }  // NOLINT
    const_iterator end() const {
        return const_iterator{bb_list_.cend()};
    }  // NOLINT

    // Argument
    void AddQubit(std::string_view name, std::size_t size) {
        argument_.qubits.emplace_back(std::string(name), size);
        argument_.num_qubits += size;
    }
    void AddRegister(std::string_view name, std::size_t size) {
        argument_.registers.emplace_back(std::string(name), size);
        argument_.num_registers += size;
    }
    std::size_t GetNumQubits() const {
        return argument_.num_qubits;
    }
    std::size_t GetNumRegisters() const {
        return argument_.num_registers;
    }
    const Argument& GetArgument() const {
        return argument_;
    }

    // Temporal register
    void SetNumTmpRegisters(std::size_t num_tmp_registers) {
        num_tmp_registers_ = num_tmp_registers;
    }
    void IncNumTmpRegisters() {
        num_tmp_registers_++;
    }
    void AddNumTmpRegisters(std::size_t add) {
        num_tmp_registers_ += add;
    }
    std::size_t GetNumTmpRegisters() const {
        return num_tmp_registers_;
    }

    /**
     * @brief Return true if this func is unitary.
     *
     * @return true unitary
     * @return false not unitary
     */
    bool IsUnitary() const {
        return !DoesMeasurement();
    }
    /**
     * @brief Return true if this func contains measurement instructions.
     *
     * @return true contain measurement instructions
     * @return false not
     */
    bool DoesMeasurement() const;

    //--------------------------------------------------//
    // Algorithm
    //--------------------------------------------------//

    /**
     * @brief Check if the dependency graph of BBs is DAG or not.
     * @exception std::runtime_error If entry BB is not set, raises error.
     *
     * @return true if the dependency is DAG.
     * @return false otherwise.
     */
    bool IsDAG() const;

    /**
     * @brief Get the list of all BBs sorted in depth order.
     * @exception std::runtime_error If IsDAG() is false or entry BB is not set, raises error.
     *
     * @return std::vector<std::pair<BasicBlock*, std::uint32_t>> the list of (BB, depth).
     */
    std::vector<std::pair<BasicBlock*, std::uint32_t>> GetDepthOrderBB() const;

    /**
     * @brief Get the conditions of BB.
     * @exception std::runtime_error If IsDAG() is false or entry BB is not set, raises error.
     * @note branch depths greater than 1 are not supported.
     * @details To correctly handle branch depths greater than 1, you need to implement the
     * simplification of disjunctive normal form (DNF) formula.
     *
     * @verbatim
     *       A
     *      /  \
     *     B    C
     *    / \   |  \
     *   D   E  F  G
     *   | \ /  | /
     *   H  I   J
     *   |  /  /
     *   K
     * @endverbatim
     *
     * Assume there is a dependency as shown in the diagram above. The arrow pointing to the left
     * represents a branch when true, and the arrow pointing to the right represents a branch when
     * false.
     *
     * * Conditions of BB 'J':
     *     * There are two paths to 'J': A->C->F->J and A->C->G->J
     *     * When there are multiple paths, the conditions may potentially be canceled out.
     *     * The conditions of the path A->C->J->J: 'A-false, C-true'
     *     * The conditions of the path A->C->G->J: 'A-false, C-false'
     *     * When merging these conditions, the condition 'C' can be ignored, resulting in 'J's
     *       condition being 'A'
     * * Conditions of BB 'I':
     *     * There are two paths to 'I': A->B->D->I and A->B->E->I
     *     * The conditions of the path A->B->D->I: 'A-true, B-true, D-false'
     *     * The conditions of the path A->B->E->I: 'A-true, B-false'
     *     * Since these conditions cannot be simplified even after merging, 'I's condition is
     *       'A', 'B' and 'D'
     *
     * @return std::unordered_map<const BasicBlock*, Formula> conditions of BB.
     */
    std::unordered_map<const BasicBlock*, math::Formula> ConditionOfBB() const;

private:
    // BasicBlock
    friend QRET_EXPORT std::unique_ptr<BasicBlock> RemoveFromParent(BasicBlock* bb);
    friend QRET_EXPORT void
    InsertInto(Function* parent, std::unique_ptr<BasicBlock>&& bb, BasicBlock* insert_before);
    friend QRET_EXPORT void MoveBefore(BasicBlock* bb, BasicBlock* move_pos);
    friend QRET_EXPORT void MoveAfter(BasicBlock* bb, BasicBlock* move_pos);
    // Function
    friend QRET_EXPORT std::unique_ptr<Function> RemoveFromParent(Function* func);

    void SetParent(Module* parent) {
        parent_ = parent;
    }

    Function(std::string_view name, Module* parent);

    std::string name_;
    Module* parent_;
    BBListType bb_list_;  //!< list of basic blocks
    BBListType::iterator entry_point_;
    // FIXME: implement list without std and remove the next member
    std::unordered_map<const BasicBlock*, BBListType::iterator> ptr2itr_;

    Argument argument_;
    std::size_t num_tmp_registers_ = 0;
};

/**
 * @brief Unlink this func from its current module, but don't delete it.
 */
QRET_EXPORT std::unique_ptr<Function> RemoveFromParent(Function* func);

/**
 * @brief Unlink this func from its current module and delete it.
 */
void QRET_EXPORT DeleteFunction(Function* func);
}  // namespace qret::ir

#endif  // QRET_IR_FUNCTION_H
