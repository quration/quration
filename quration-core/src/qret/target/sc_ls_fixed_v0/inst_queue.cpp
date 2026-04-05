/**
 * @file qret/target/sc_ls_fixed_v0/inst_queue.cpp
 * @brief Instruction queue.
 */

#include "qret/target/sc_ls_fixed_v0/inst_queue.h"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <iterator>
#include <stdexcept>
#include <unordered_map>

#include "qret/target/sc_ls_fixed_v0/constants.h"
#include "qret/target/sc_ls_fixed_v0/instruction.h"
#include "qret/target/sc_ls_fixed_v0/sc_ls_fixed_v0_target_machine.h"
#include "qret/target/sc_ls_fixed_v0/symbol.h"

namespace qret::sc_ls_fixed_v0 {
namespace {
std::int64_t EstimateInsertedWeight(
        InstQueue::WeightAlgorithm algorithm,
        ScLsInstructionBase* replacement,
        std::int64_t base_weight,
        std::size_t base_index,
        std::size_t rank
) {
    (void)algorithm;
    (void)replacement;
    (void)base_index;
    // Keep inserted instructions near the original priority and use rank for deterministic
    // ordering.
    return base_weight + static_cast<std::int64_t>(rank);
}
}  // namespace

ScLsInstIterator::ScLsInstIterator(MachineFunction& tmp)
    : mf_(tmp)
    , mbb_itr_(mf_.begin()) {
    if (!IsEnd()) {
        minst_itr_ = mbb_itr_->begin();
        ForwardTillValidInstruction();
    }
}
ScLsInstructionBase* ScLsInstIterator::Next() {
    if (IsEnd()) {
        return nullptr;
    }

    auto* ret = static_cast<ScLsInstructionBase*>(minst_itr_->get());

    minst_itr_++;
    ForwardTillValidInstruction();

    return ret;
}
ScLsInstructionBase* ScLsInstIterator::Peek() const {
    if (IsEnd()) {
        return nullptr;
    }
    auto* ret = static_cast<ScLsInstructionBase*>(minst_itr_->get());
    return ret;
}
bool ScLsInstIterator::IsEnd() const {
    return mbb_itr_ == mf_.end();
}
void ScLsInstIterator::ForwardTillValidInstruction() {
    while (minst_itr_ == mbb_itr_->end()) {
        ++mbb_itr_;
        if (IsEnd()) {
            break;
        }
        minst_itr_ = mbb_itr_->begin();
    }
}

InstQueue::InstQueue(
        const ScLsFixedV0MachineOption& option,
        MachineFunction& mf,
        WeightAlgorithm algorithm
)
    : option_(option)
    , iter_(mf)
    , algorithm_(algorithm)
    , runnable_(CompareWeight{&nodes_}) {}
void InstQueue::CalculateWeight(
        WeightAlgorithm algorithm,
        std::unordered_map<ScLsInstructionBase*, Node>& nodes
) {
    switch (algorithm) {
        case WeightAlgorithm::Index:
            CalculateWeightByIndex(nodes);
            break;
        case WeightAlgorithm::Type:
            CalculateWeightByType(nodes);
            break;
        case WeightAlgorithm::InvDepth:
            CalculateWeightByInvDepth(nodes);
            break;
        default:
            break;
    }
}
void InstQueue::CalculateWeightByIndex(std::unordered_map<ScLsInstructionBase*, Node>& nodes) {
    for (auto&& [_, node] : nodes) {
        node.weight = static_cast<std::int64_t>(node.index);
    }
}
void InstQueue::CalculateWeightByType(std::unordered_map<ScLsInstructionBase*, Node>& nodes) {
    for (auto&& [inst, node] : nodes) {
        if (const auto type = inst->Type(); type == ScLsInstructionType::ALLOCATE) {
            node.weight = 999;
        } else {
            node.weight = static_cast<std::int64_t>(inst->Latency());
        }
    }
}
namespace {
bool Visit(
        ScLsInstructionBase* inst,
        std::unordered_map<ScLsInstructionBase*, InstQueue::Node>& nodes,
        std::list<ScLsInstructionBase*>& sorted
) {
    auto& node = nodes.at(inst);
    if (node.weight == 1) {
        return false;
    } else if (node.weight == 2) {
        return true;
    }
    node.weight = 1;
    for (auto&& [child, _] : node.children) {
        const auto success = Visit(child, nodes, sorted);
        if (!success) {
            return false;
        }
    }
    node.weight = 2;
    sorted.emplace_front(inst);
    return true;
}
}  // namespace
void InstQueue::CalculateWeightByInvDepth(std::unordered_map<ScLsInstructionBase*, Node>& nodes) {
    constexpr auto MinWeight = std::numeric_limits<std::int64_t>::min() / 2;
    // constexpr auto MaxWeight = std::numeric_limits<std::int64_t>::max() / 2;
    constexpr auto MaxWeight = std::int64_t{0};

    // Topsort.
    // NOTE: although the sequence of instructions can be sorted by index to achieve a topological
    // sort, this approach cannot handle cases where new instructions with no index are inserted
    // into the sequence. Therefore, a straightforward DFS-based implementation of topological sort
    // is used.
    // Status: 0->unvisited, 1->temporary, 2->visited.
    for (auto&& [_, node] : nodes) {
        node.weight = 0;
    }
    auto sorted = std::list<ScLsInstructionBase*>();
    for (auto&& [inst, node] : nodes) {
        if (node.weight == 2) {
            continue;
        }
        const auto success = Visit(inst, nodes, sorted);
        if (!success) {
            throw std::logic_error("Dependency graph of instruction is not DAG");
        }
    }

    // Initialize weight.
    for (auto&& [inst, node] : nodes) {
        if (const auto type = inst->Type(); type == ScLsInstructionType::ALLOCATE_MAGIC_FACTORY
            || type == ScLsInstructionType::ALLOCATE_ENTANGLEMENT_FACTORY) {
            node.weight = MinWeight;
        } else {
            node.weight = MaxWeight;
        }
    }

    // Set weight.
    // Traverse the topologically sorted instruction sequence in reverse order to calculate the
    // longest distance from the last instruction.
    for (auto itr = sorted.rbegin(); itr != sorted.rend(); ++itr) {
        auto* inst = *itr;
        const auto& node = nodes.at(inst);

        for (const auto& [parent, beat] : node.parents) {
            auto parent_itr = nodes.find(parent);

            // If parent is not in InstQueue, do nothing.
            if (parent_itr == nodes.end()) {
                continue;
            }

            parent_itr->second.weight = std::min(
                    parent_itr->second.weight,
                    node.weight - static_cast<std::int64_t>(beat)
            );
        }
    }

    // Finalize weight.
    for (auto&& [inst, node] : nodes) {
        if (inst->Type() == ScLsInstructionType::DEALLOCATE) {
            node.weight = MinWeight;
        }
    }
}
bool InstQueue::Peek(const std::size_t size) {
    const auto update_qmap = [&](QSymbol q, ScLsInstructionBase* inst, Node& node) {
        auto itr = qmap_.find(q);
        if (itr != qmap_.end()) {
            auto* prev = itr->second;

            // If the previous instruction is still in the InstQueue, set a dependency.
            auto prev_node = nodes_.find(prev);
            if (prev_node != nodes_.end()) {
                const auto latency = prev->Latency();
                const auto length = prev_node->second.children.contains(inst)
                        ? std::max(prev_node->second.children.at(inst), latency)
                        : latency;
                prev_node->second.children[inst] = length;
                node.parents[prev] = length;
                node.num_remaining_parents = static_cast<std::uint32_t>(node.parents.size());
            }
        }
        qmap_[q] = inst;
    };
    const auto set_c_dep = [&](CSymbol condition, ScLsInstructionBase* inst, Node& node) {
        auto itr = cmap_.find(condition);
        if (itr != cmap_.end()) {
            auto* prev = itr->second;

            // If the previous instruction is still in the InstQueue, set a dependency.
            auto prev_node = nodes_.find(prev);
            if (prev_node != nodes_.end()) {
                const auto latency = prev->StartCorrecting() + option_.reaction_time;
                const auto length = prev_node->second.children.contains(inst)
                        ? std::max(prev_node->second.children.at(inst), latency)
                        : latency;
                prev_node->second.children[inst] = length;
                node.parents[prev] = length;
                node.num_remaining_parents = static_cast<std::uint32_t>(node.parents.size());
            }
        }
    };

    nodes_.reserve(nodes_.size() + size);

    for (auto num_peeked = std::size_t{0}; num_peeked < size; ++num_peeked) {
        if (iter_.IsEnd()) {
            break;
        }

        // Peek a instruction.
        auto* inst = iter_.Next();
        index_++;

        auto& node = nodes_[inst] = Node(index_, {}, {});

        for (const auto& q : inst->QTarget()) {
            update_qmap(q, inst, node);
        }
        for (const auto& c : inst->Condition()) {
            set_c_dep(c, inst, node);
        }
        for (const auto& c : inst->CDepend()) {
            if (c.Id() >= NumReservedCSymbols) {
                set_c_dep(c, inst, node);
            }
        }
        for (const auto& c : inst->CCreate()) {
            cmap_[c] = inst;
        }

        if (node.num_remaining_parents == 0) {
            runnable_.insert(inst);
        }
    }

    {
        // NOTE: The 'runnable_' set is sorted based on the weights of the nodes.
        // Therefore, when updating a node's weight, the 'runnable_' set needs to be reconstructed.
        auto runnable_stash = std::vector<ScLsInstructionBase*>();
        runnable_stash.reserve(runnable_.size());
        std::copy(runnable_.begin(), runnable_.end(), std::back_inserter(runnable_stash));
        runnable_.clear();

        CalculateWeight(algorithm_, nodes_);

        for (auto* inst : runnable_stash) {
            runnable_.emplace(inst);
        }
    }

    return iter_.IsEnd();
}
bool InstQueue::Run(ScLsInstructionBase* inst) {
    if (!IsRunnable(inst)) {
        return false;
    }

    const auto& node = nodes_.at(inst);
    for (auto&& [child_inst, _] : node.children) {
        // NOTE: DO NOT UPDATE 'parents' of child_node.
        auto& child_node = nodes_.at(child_inst);
        child_node.num_remaining_parents--;
        if (child_node.num_remaining_parents == 0) {
            runnable_.insert(child_inst);
        }
    }

    runnable_.erase(inst);
    nodes_.erase(inst);

    return true;
}
bool InstQueue::RunFuture(Beat beat, ScLsInstructionBase* inst) {
    if (!IsRunnable(inst)) {
        return false;
    }

    reserved_.insert({beat, inst});
    runnable_.erase(inst);

    return true;
}
std::size_t InstQueue::SetBeat(Beat beat) {
    auto num_run = std::size_t{0};
    auto itr = reserved_.begin();
    while (itr != reserved_.end() && itr->first <= beat) {
        // Run 'node'.
        num_run++;
        auto* inst = itr->second;
        auto& node = nodes_.at(inst);

        for (auto&& [child_inst, _] : node.children) {
            auto& child_node = nodes_.at(child_inst);
            child_node.num_remaining_parents--;
            if (child_node.num_remaining_parents == 0) {
                runnable_.insert(child_inst);
            }
        }

        nodes_.erase(inst);
        reserved_.erase(itr);

        itr = reserved_.begin();
    }
    return num_run;
}
void InstQueue::InsertBefore(ScLsInstructionBase* inst, ScLsInstructionBase* new_inst, Beat len) {
    if (!Contains(inst)) {
        throw std::runtime_error("'inst' is not in queue");
    }
    if (IsReserved(inst)) {
        throw std::runtime_error("Cannot insert new instruction before reserved instruction.");
    }
    const auto was_runnable = IsRunnable(inst);

    // Insert new node as a COPY of 'node'.
    auto& node = nodes_.at(inst);
    const auto insert_weight =
            EstimateInsertedWeight(algorithm_, new_inst, node.weight, node.index, 0);
    const auto [new_node_itr, success_to_emplace_new_node] = nodes_.emplace(new_inst, node);
    if (!success_to_emplace_new_node) {
        throw std::runtime_error("Failed to emplace new instruction");
    }
    auto& new_node = new_node_itr->second;
    new_node.weight = insert_weight;
    new_node.index = ++index_;

    // Parent of 'node' is 'new_inst'.
    node.parents.clear();
    node.parents[new_inst] = len;
    node.num_remaining_parents = 1;

    // Child of 'new_node' is 'inst'.
    new_node.children.clear();
    new_node.children[inst] = len;

    // Replace 'inst' with 'new_inst' in children of parents.
    for (auto&& [parent, tmp_len] : new_node.parents) {
        if (!Contains(parent)) {
            continue;
        }

        auto& parent_node = nodes_.at(parent);
        parent_node.children.erase(inst);
        parent_node.children[new_inst] = tmp_len;
    }

    // Update runnable queue if 'inst' is runnable.
    if (was_runnable) {
        runnable_.erase(inst);
        runnable_.insert(new_inst);
    }
}
void InstQueue::InsertAfter(ScLsInstructionBase* inst, ScLsInstructionBase* new_inst, Beat len) {
    if (!Contains(inst)) {
        throw std::runtime_error("'inst' is not in queue");
    }

    // Insert new node as a COPY of 'node'.
    auto& node = nodes_.at(inst);
    const auto insert_weight =
            EstimateInsertedWeight(algorithm_, new_inst, node.weight, node.index, 0);
    const auto [new_node_itr, success_to_emplace_new_node] = nodes_.emplace(new_inst, node);
    if (!success_to_emplace_new_node) {
        throw std::runtime_error("Failed to emplace new instruction");
    }
    auto& new_node = new_node_itr->second;
    new_node.weight = insert_weight;
    new_node.index = ++index_;

    // Parent of 'new_node' is 'inst'.
    new_node.parents.clear();
    new_node.parents[inst] = len;
    new_node.num_remaining_parents = 1;

    // Child of 'node' is 'new_inst'.
    node.children.clear();
    node.children[new_inst] = len;

    // Replace 'inst' with 'new_inst' in parents of children.
    for (auto&& [child, tmp_len] : new_node.children) {
        assert(Contains(child));

        auto& child_node = nodes_.at(child);
        child_node.parents.erase(inst);
        child_node.parents[new_inst] = tmp_len;
    }

    // Replace 'inst' with 'new_inst' in 'qmap_' and 'cmap_'.
    for (auto&& [_, tmp_inst] : qmap_) {
        if (tmp_inst == inst) {
            tmp_inst = new_inst;
        }
    }
    for (auto&& [_, tmp_inst] : cmap_) {
        if (tmp_inst == inst) {
            tmp_inst = new_inst;
        }
    }
}
void InstQueue::Replace(
        ScLsInstructionBase* const inst,
        const std::vector<ScLsInstructionBase*>& new_insts,
        Beat len
) {
    if (!Contains(inst)) {
        throw std::runtime_error("'inst' is not in queue.");
    }
    if (IsReserved(inst)) {
        throw std::runtime_error("Cannot replace reserved instruction with new instructions.");
    }

    // Insert new node as a COPY of 'node'.
    auto& node = nodes_.at(inst);
    for (std::size_t i = 0; i < new_insts.size(); ++i) {
        auto* new_inst = new_insts[i];
        const auto [new_node_itr, success_to_emplace_new_node] = nodes_.emplace(new_inst, node);
        if (!success_to_emplace_new_node) {
            const auto msg =
                    fmt::format("Failed to emplace new instruction: {}", new_inst->ToString());
            throw std::runtime_error(msg);
        }
        new_node_itr->second.weight =
                EstimateInsertedWeight(algorithm_, new_inst, node.weight, node.index, i);
        new_node_itr->second.index = ++index_;
    }

    // Replace 'inst' with 'new_insts' in parents of children.
    for (auto&& [child, _tmp_len] : node.children) {
        auto& child_node = nodes_.at(child);
        for (auto* new_inst : new_insts) {
            child_node.parents[new_inst] = len;
            child_node.num_remaining_parents++;
        }
        child_node.parents.erase(inst);
        child_node.num_remaining_parents--;
    }
    // Replace 'inst' with 'new_insts' in children of parents.
    for (auto&& [parent, tmp_len] : node.parents) {
        if (!Contains(parent)) {
            continue;
        }

        auto& parent_node = nodes_.at(parent);
        for (auto* new_inst : new_insts) {
            parent_node.children[new_inst] = tmp_len;
        }
        parent_node.children.erase(inst);
    }
    // Replace 'inst' with 'new_inst' in 'qmap_'.
    for (auto q_itr = qmap_.begin(); q_itr != qmap_.end();) {
        const auto q = q_itr->first;
        if (q_itr->second != inst) {
            ++q_itr;
            continue;
        }
        auto* replacement = static_cast<ScLsInstructionBase*>(nullptr);
        for (auto* new_inst : new_insts) {
            for (const auto tmp_q : new_inst->QTarget()) {
                if (q == tmp_q) {
                    replacement = new_inst;
                }
            }
        }
        if (replacement == nullptr) {
            q_itr = qmap_.erase(q_itr);
        } else {
            q_itr->second = replacement;
            ++q_itr;
        }
    }
    // Replace 'inst' with 'new_inst' in 'cmap_'.
    for (auto c_itr = cmap_.begin(); c_itr != cmap_.end();) {
        const auto c = c_itr->first;
        if (c_itr->second != inst) {
            ++c_itr;
            continue;
        }
        auto* replacement = static_cast<ScLsInstructionBase*>(nullptr);
        for (auto* new_inst : new_insts) {
            for (const auto tmp_c : new_inst->CCreate()) {
                if (c == tmp_c) {
                    replacement = new_inst;
                }
            }
        }
        if (replacement == nullptr) {
            c_itr = cmap_.erase(c_itr);
        } else {
            c_itr->second = replacement;
            ++c_itr;
        }
    }
    // Replace 'inst' with 'new_insts' in runnable if 'inst' is in runnable.
    if (runnable_.contains(inst)) {
        for (auto* new_inst : new_insts) {
            runnable_.emplace(new_inst);
        }
        runnable_.erase(inst);
    }

    // Delete 'inst' in node.
    nodes_.erase(inst);
}
}  // namespace qret::sc_ls_fixed_v0
