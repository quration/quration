/**
 * @file qret/ir/basic_block.h
 * @brief BasicBlock.
 */

#ifndef QRET_IR_BASIC_BLOCK_H
#define QRET_IR_BASIC_BLOCK_H

#include <algorithm>
#include <list>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "qret/base/iterator.h"
#include "qret/base/log.h"
#include "qret/ir/instruction.h"
#include "qret/qret_export.h"

namespace qret::ir {
// Forward declarations
class Function;

/**
 * @brief Basic block of qret IR.
 * @details This class represents a single basic block in qret IR.
 * A single basic block is simply a container of instructions that execute sequentially.
 *
 * A well formed basic block is formed of a list of non-terminating instructions followed by a
 * single terminator instruction. Terminator instructions may not occur in the middle of basic
 * blocks, and must terminate the blocks. The BasicBlock class allows malformed basic blocks to
 * occur because it may be useful in the intermediate stage of constructing or modifying a program.
 *
 * \todo Implement a verifier to ensure that basic blocks are "well formed".
 *
 * The owner of basic blocks is a func.
 */
class QRET_EXPORT BasicBlock {
public:
    using InstListType = std::list<std::unique_ptr<Instruction>>;

    BasicBlock(const BasicBlock&) = delete;
    BasicBlock& operator=(const BasicBlock&) = delete;

    /**
     * @brief Create a new BasicBlock with no parent.
     * @details If you want to set a parent, use the function "InsertInto".
     *
     * @param name basic block name
     * @return std::unique_ptr<BasicBlock> a new basic block
     */
    static std::unique_ptr<BasicBlock> Create(std::string_view name);
    /**
     * @brief Create a new BasicBlock.
     *
     * @param name basic block name
     * @param parent func owning a new basic block
     * @return BasicBlock* a new basic block
     */
    static BasicBlock* Create(std::string_view name, Function* parent);

    bool HasParent() const {
        return parent_ != nullptr;
    }

    /**
     * @brief Get the func owning this basic block, or nullptr if the basic block does not have a
     * func.
     */
    const Function* GetParent() const {
        return parent_;
    }
    Function* GetParent() {
        return parent_;
    }

    /**
     * @brief Get the module owning the func this basic block belongs to, or nullptr if no such
     * module exists.
     */
    const Module* GetModule() const;
    Module* GetModule() {
        return const_cast<Module*>(static_cast<const BasicBlock*>(this)->GetModule());
    }

    std::string_view GetName() const {
        return name_;
    }

    /**
     * @brief Get the terminator instruction if the block is well formed. Otherwise return a null
     * pointer.
     */
    const Instruction* GetTerminator() const {
        if (inst_list_.empty() || !inst_list_.back()->IsTerminator()) {
            return nullptr;
        }
        return inst_list_.back().get();
    }
    Instruction* GetTerminator() {
        return const_cast<Instruction*>(static_cast<const BasicBlock*>(this)->GetTerminator());
    }

    /**
     * @brief Get the predecessor of this block if it has a unique predecessor block. Otherwise
     * return a null pointer.
     */
    const BasicBlock* GetUniquePredecessor() const;
    BasicBlock* GetUniquePredecessor() {
        return const_cast<BasicBlock*>(
                static_cast<const BasicBlock*>(this)->GetUniquePredecessor()
        );
    }
    /**
     * @brief Get the number of predecessor blocks.
     */
    std::size_t NumPredecessors() const {
        return predecessors_.size();
    }

    /**
     * @brief Get the successor of this block if it has a unique successor block. Otherwise
     * return a null pointer.
     */
    const BasicBlock* GetUniqueSuccessor() const;
    BasicBlock* GetUniqueSuccessor() {
        return const_cast<BasicBlock*>(static_cast<const BasicBlock*>(this)->GetUniqueSuccessor());
    }
    /**
     * @brief Get the number of successor blocks.
     */
    std::size_t NumSuccessors() const {
        return successors_.size();
    }

    /**
     * @brief Get the number of instructions.
     *
     * @return std::size_t the number of instructions
     */
    std::size_t Size() const {
        return inst_list_.size();
    }
    bool Empty() const {
        return inst_list_.empty();
    }
    const Instruction& Front() const {
        return *inst_list_.front();
    }
    Instruction& Front() {
        return *inst_list_.front();
    }
    const Instruction& Back() const {
        return *inst_list_.back();
    }

    using iterator = ListIterator<Instruction, false>;
    using const_iterator = ListIterator<Instruction, true>;
    using reverse_iterator = ListIterator<Instruction, false, true>;
    using const_reverse_iterator = ListIterator<Instruction, true, true>;
    iterator begin() {
        return iterator{inst_list_.begin()};
    }  // NOLINT
    const_iterator begin() const {
        return const_iterator{inst_list_.cbegin()};
    }  // NOLINT
    const_iterator cbegin() const {
        return const_iterator{inst_list_.cbegin()};
    }  // NOLINT
    iterator end() {
        return iterator{inst_list_.end()};
    }  // NOLINT
    const_iterator end() const {
        return const_iterator{inst_list_.cend()};
    }  // NOLINT
    const_iterator cend() const {
        return const_iterator{inst_list_.cend()};
    }  // NOLINT
    reverse_iterator rbegin() {
        return reverse_iterator{inst_list_.rbegin()};
    }  // NOLINT
    const_reverse_iterator rbegin() const {  // NOLINT
        return const_reverse_iterator{inst_list_.crbegin()};
    }
    const_reverse_iterator crbegin() const {  // NOLINT
        return const_reverse_iterator{inst_list_.crbegin()};
    }
    reverse_iterator rend() {
        return reverse_iterator{inst_list_.rend()};
    }  // NOLINT
    const_reverse_iterator rend() const {  // NOLINT
        return const_reverse_iterator{inst_list_.crend()};
    }
    const_reverse_iterator crend() const {  // NOLINT
        return const_reverse_iterator{inst_list_.crend()};
    }

    using pred_iterator = std::vector<BasicBlock*>::iterator;
    using pred_const_iterator = std::vector<BasicBlock*>::const_iterator;
    pred_iterator pred_begin() {
        return predecessors_.begin();
    }  // NOLINT
    pred_const_iterator pred_begin() const {
        return predecessors_.cbegin();
    }  // NOLINT
    pred_const_iterator pred_cbegin() {
        return predecessors_.cbegin();
    }  // NOLINT
    pred_iterator pred_end() {
        return predecessors_.end();
    }  // NOLINT
    pred_const_iterator pred_end() const {
        return predecessors_.cend();
    }  // NOLINT
    pred_const_iterator pred_cend() {
        return predecessors_.cend();
    }  // NOLINT

    using succ_iterator = std::vector<BasicBlock*>::iterator;
    using succ_const_iterator = std::vector<BasicBlock*>::const_iterator;
    succ_iterator succ_begin() {
        return successors_.begin();
    }  // NOLINT
    succ_const_iterator succ_begin() const {
        return successors_.cbegin();
    }  // NOLINT
    succ_const_iterator succ_cbegin() {
        return successors_.cbegin();
    }  // NOLINT
    succ_iterator succ_end() {
        return successors_.end();
    }  // NOLINT
    succ_const_iterator succ_end() const {
        return successors_.cend();
    }  // NOLINT
    succ_const_iterator succ_cend() {
        return successors_.cend();
    }  // NOLINT

    /**
     * @brief Return true if this is the entry basic block of the containing func.
     *
     * @return true entry block
     * @return false not entry block
     */
    bool IsEntryBlock() const;

    /**
     * @brief Return true if this is the exit basic block of the containing func.
     *
     * @return true exit block
     * @return false not exit block
     */
    bool IsExitBlock() const;

    /**
     * @brief Transfer all instructions from \p from to this basic block at \p to_it .
     *
     * @param to_it instruction before which the content will be inserted
     * @param from another basic block to transfer the content from
     */
    void Splice(Instruction* to_it, std::unique_ptr<BasicBlock>&& from);

    /**
     * @brief Split the basic block into two basic blocks at the specified instruction.
     * @details All instructions BEFORE the specified iterator stay as part of the original basic
     * block, an unconditional branch is added to the original BB, and the rest of the instructions
     * in the BB are moved to the new BB, including the old terminator.
     *
     * @param i the specified instruction.
     * @param name name of new BB.
     * @return BasicBlock* newly formed BB.
     */
    BasicBlock* Split(Instruction* i, std::string_view name);

    /**
     * @brief Return true if this basic block is unitary.
     *
     * @return true unitary
     * @return false not unitary
     */
    bool IsUnitary() const {
        return !DoesMeasurement();
    }
    /**
     * @brief Return true if this basic block contains measurement instructions.
     *
     * @return true contain measurement instructions
     * @return false not
     */
    bool DoesMeasurement() const;

    iterator GetIteratorFromPointer(const Instruction* inst) const {
        return iterator{ptr2itr_.at(inst)};
    }

    /**
     * @brief Set the successor, predecessor relation between this basic block and \p successor .
     *
     * @param successor a successor of this basic block
     */
    void AddEdge(BasicBlock* successor) {
        EmplaceIfUnique(successors_, successor);
        EmplaceIfUnique(successor->predecessors_, this);
    }
    bool RemovePredecessor(BasicBlock* predecessor) {
        EraseIfExist(predecessors_, predecessor);
        EraseIfExist(predecessor->successors_, this);
        return false;
    }
    void ClearPredecessors() {
        for (auto* predecessor : predecessors_) {
            EraseIfExist(predecessor->successors_, this);
        }
        predecessors_.clear();
    }
    bool RemoveSuccessor(BasicBlock* successor) {
        EraseIfExist(successors_, successor);
        EraseIfExist(successor->predecessors_, this);
        return false;
    }
    void ClearSuccessors() {
        for (auto* successor : successors_) {
            EraseIfExist(successor->predecessors_, this);
        }
        successors_.clear();
    }

private:
    // Instruction
    friend Instruction* Instruction::GetNextInstruction() const;
    friend QRET_EXPORT std::unique_ptr<Instruction> RemoveFromParent(Instruction* inst);
    friend QRET_EXPORT void EraseFromParent(Instruction* inst);
    friend QRET_EXPORT void
    InsertBefore(std::unique_ptr<Instruction>&& inst, Instruction* insert_pos);
    friend QRET_EXPORT void
    InsertAfter(std::unique_ptr<Instruction>&& inst, Instruction* insert_pos);
    friend QRET_EXPORT void InsertAtEnd(std::unique_ptr<Instruction>&& inst, BasicBlock* bb);
    // BasicBlock
    friend QRET_EXPORT std::unique_ptr<BasicBlock> RemoveFromParent(BasicBlock* bb);
    friend QRET_EXPORT void
    InsertInto(Function* parent, std::unique_ptr<BasicBlock>&& bb, BasicBlock* insert_before);
    friend QRET_EXPORT void MoveBefore(BasicBlock* bb, BasicBlock* move_pos);
    friend QRET_EXPORT void MoveAfter(BasicBlock* bb, BasicBlock* move_pos);

    void SetParent(Function* parent) {
        parent_ = parent;
        predecessors_.clear();
        successors_.clear();
    }
    void ResetP2I() {
        ptr2itr_.clear();
        ptr2itr_.reserve(inst_list_.size());
        for (auto itr = inst_list_.begin(); itr != inst_list_.end(); ++itr) {
            ptr2itr_[itr->get()] = itr;
            itr->get()->SetParent(this);
        }
    }
    bool EmplaceIfUnique(auto&& container, auto&& to_insert) {
        const auto itr = std::ranges::find(container, to_insert);
        if (itr == container.end()) {
            container.emplace_back(to_insert);
            return true;
        }
        return false;
    };
    bool EraseIfExist(auto&& container, auto&& to_erase) {
        if (container.empty()) {
            LOG_ERROR("[EraseIfExist] 'container' is empty.");
            return false;
        }

        const auto itr = std::ranges::find(container, to_erase);
        if (itr != container.end()) {
            container.erase(itr);
            return true;
        }
        return false;
    };

    BasicBlock(std::string_view name, Function* parent);

    std::string name_;
    Function* parent_;  //!< the owner of this basic block
    InstListType inst_list_;  //!< list of instructions
    // FIXME: implement list without std and remove the next member
    std::unordered_map<const Instruction*, InstListType::iterator> ptr2itr_;

    // CFG
    std::vector<BasicBlock*> predecessors_;  //!< predecessors of this block
    std::vector<BasicBlock*> successors_;  //!< successors of this block
};
/**
 * @brief Unlink basic block from its current func, but don't delete it.
 */
QRET_EXPORT std::unique_ptr<BasicBlock> RemoveFromParent(BasicBlock* bb);
/**
 * @brief Unlink basic block from its current func and delete it.
 */
QRET_EXPORT void DeleteBasicBlock(BasicBlock* bb);
/**
 * @brief Insert an unlinked basic block into a func.
 * @details Insert an unlinked basic block into \p parent. If insert_before is provided, insert
 * before that basic block, otherwise insert at the end.
 */
QRET_EXPORT void
InsertInto(Function* parent, std::unique_ptr<BasicBlock>&& bb, BasicBlock* insert_before = nullptr);
/**
 * @brief Unlink this basic block from its current func and insert it into a func right
 * before the specified iterator.
 * @details RemoveFromParent + InsertBefore.
 */
QRET_EXPORT void MoveBefore(BasicBlock* bb, BasicBlock* move_pos);
/**
 * @brief Unlink this basic block from its current func and insert it into a func right
 * after the specified iterator.
 * @details RemoveFromParent + InsertAfter.
 */
QRET_EXPORT void MoveAfter(BasicBlock* bb, BasicBlock* move_pos);
/**
 * @brief Merges a basic block into its only predecessor if it is trivially mergeable.
 *
 * This function checks whether the given basic block (`bb`) has exactly one predecessor,
 * and whether that predecessor (`pred`) ends with a terminator instruction (e.g., `br`).
 * If so, the instructions in `bb` are spliced into `pred`, and the terminator of `pred`
 * is removed. The block `bb` is then removed from its parent and deleted.
 *
 * @param bb Pointer to the basic block to be merged.
 * @return true if the block was successfully merged; false otherwise.
 */
QRET_EXPORT bool MergeBasicBlockIntoOnlyPred(BasicBlock* bb);
}  // namespace qret::ir

#endif  // QRET_IR_BASIC_BLOCK_H
