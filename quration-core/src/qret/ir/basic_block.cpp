/**
 * @file qret/ir/basic_block.cpp
 * @brief BasicBlock.
 */

#include "qret/ir/basic_block.h"

#include <cassert>
#include <memory>
#include <string_view>

#include "qret/base/cast.h"
#include "qret/ir/function.h"
#include "qret/ir/instruction.h"
#include "qret/ir/instructions.h"

namespace qret::ir {
std::unique_ptr<BasicBlock> BasicBlock::Create(std::string_view name) {
    return std::unique_ptr<BasicBlock>(new BasicBlock(name, nullptr));
}
BasicBlock* BasicBlock::Create(std::string_view name, Function* parent) {
    assert(parent != nullptr);
    auto bb = std::unique_ptr<BasicBlock>(new BasicBlock(name, parent));
    auto* bb_ptr = bb.get();
    InsertInto(parent, std::move(bb));
    return bb_ptr;  // NOLINT
}
const Module* BasicBlock::GetModule() const {
    return GetParent() == nullptr ? nullptr : GetParent()->GetParent();
}
const BasicBlock* BasicBlock::GetUniquePredecessor() const {
    return predecessors_.size() == 1 ? predecessors_[0] : nullptr;
}
const BasicBlock* BasicBlock::GetUniqueSuccessor() const {
    return successors_.size() == 1 ? successors_[0] : nullptr;
}
bool BasicBlock::IsEntryBlock() const {
    return HasParent() ? GetParent()->GetEntryBB() == this : false;
}
bool BasicBlock::IsExitBlock() const {
    if (inst_list_.empty()) {
        return false;
    }
    return DynCast<ReturnInst>(inst_list_.back().get()) != nullptr;
}
void BasicBlock::Splice(Instruction* to_it, std::unique_ptr<BasicBlock>&& from) {
    const auto map_itr = ptr2itr_.find(to_it);
    if (map_itr == ptr2itr_.end()) {
        throw std::runtime_error("insertion point is not in this BB");
    }
    inst_list_.splice(map_itr->second, from->inst_list_);
    ResetP2I();
}
BasicBlock* BasicBlock::Split(Instruction* i, std::string_view name) {
    const auto map_itr = ptr2itr_.find(i);
    if (map_itr == ptr2itr_.end()) {
        throw std::runtime_error("split point is not in this BB");
    }

    BasicBlock* next_bb = BasicBlock::Create(name, GetParent());
    next_bb->inst_list_
            .splice(next_bb->inst_list_.begin(), inst_list_, map_itr->second, inst_list_.cend());
    BranchInst::Create(next_bb, this);
    next_bb->successors_ = successors_;
    successors_.clear();
    AddEdge(next_bb);
    ResetP2I();
    next_bb->ResetP2I();
    return next_bb;
}
bool BasicBlock::DoesMeasurement() const {
    for (const auto& inst : inst_list_) {
        if (inst->IsMeasurement()) {
            return true;
        }
    }
    return false;
}
BasicBlock::BasicBlock(std::string_view name, Function* parent)
    : name_(name)
    , parent_(parent) {}
std::unique_ptr<BasicBlock> RemoveFromParent(BasicBlock* bb) {
    if (bb == nullptr || !bb->HasParent()) {
        return nullptr;
    }

    auto* func = bb->GetParent();
    const auto map_itr = func->ptr2itr_.find(bb);
    if (map_itr == func->ptr2itr_.end()) {
        return nullptr;
    }

    const auto itr = map_itr->second;
    auto ret = std::move(*itr);
    ret->SetParent(nullptr);
    ret->predecessors_.clear();
    ret->successors_.clear();
    func->bb_list_.erase(itr);
    func->ptr2itr_.erase(map_itr);
    return ret;
}
void DeleteBasicBlock(BasicBlock* bb) {
    (void)RemoveFromParent(bb);
}
void InsertInto(Function* func, std::unique_ptr<BasicBlock>&& bb, BasicBlock* insert_before) {
    if (func == nullptr || bb == nullptr
        || (insert_before != nullptr && insert_before->GetParent() != func)) {
        return;
    }

    bb->SetParent(func);
    auto* bb_ptr = bb.get();
    if (insert_before == nullptr) {
        func->bb_list_.push_back(std::move(bb));
        func->ptr2itr_[bb_ptr] = std::prev(func->bb_list_.end());
    } else {
        const auto map_itr = func->ptr2itr_.find(insert_before);
        if (map_itr == func->ptr2itr_.end()) {
            return;
        }
        const auto itr = func->bb_list_.insert(map_itr->second, std::move(bb));
        func->ptr2itr_[bb_ptr] = itr;
    }
}
void MoveBefore(BasicBlock* bb, BasicBlock* move_pos) {
    if (bb == nullptr || !bb->HasParent() || move_pos == nullptr || !move_pos->HasParent()) {
        return;
    }

    auto* func = move_pos->GetParent();
    const auto map_itr = func->ptr2itr_.find(move_pos);
    assert(map_itr != func->ptr2itr_.end() && "bb must be registered in parent");

    bb->SetParent(func);
    const auto itr = func->bb_list_.insert(map_itr->second, RemoveFromParent(bb));
    func->ptr2itr_[bb] = itr;
}
void MoveAfter(BasicBlock* bb, BasicBlock* move_pos) {
    if (bb == nullptr || !bb->HasParent() || move_pos == nullptr || !move_pos->HasParent()) {
        return;
    }

    auto* func = move_pos->GetParent();
    const auto map_itr = func->ptr2itr_.find(move_pos);
    assert(map_itr != func->ptr2itr_.end() && "bb must be registered in parent");

    bb->SetParent(func);
    const auto itr = func->bb_list_.insert(std::next(map_itr->second), RemoveFromParent(bb));
    func->ptr2itr_[bb] = itr;
}
bool MergeBasicBlockIntoOnlyPred(BasicBlock* bb) {
    if (bb == nullptr) {
        return false;
    }
    if (bb->GetParent() == nullptr) {
        return false;
    }
    if (bb->NumPredecessors() != 1) {
        return false;
    }

    auto* pred = bb->GetUniquePredecessor();
    auto* terminator = pred->GetTerminator();

    // Predecessor must be unconditional branch.
    if (const auto* inst = DynCast<BranchInst>(terminator)) {
        if (inst->IsConditional()) {
            return false;
        }
        if (inst->GetTrueBB() != bb) {
            return false;
        }
    } else {
        return false;
    }

    // Set dependency of pred.
    pred->ClearSuccessors();
    for (auto itr = bb->succ_begin(); itr != bb->succ_end(); ++itr) {
        pred->AddEdge(*itr);
    }
    bb->ClearSuccessors();

    // Move instructions in bb to pred.
    pred->Splice(terminator, RemoveFromParent(bb));

    // Delete pred terminator, which is unconditional branch.
    RemoveFromParent(terminator);

    return true;
}
}  // namespace qret::ir
