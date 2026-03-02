/**
 * @file qret/ir/function.cpp
 * @brief Function.
 */

#include "qret/ir/function.h"

#include <fmt/format.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <numeric>
#include <queue>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "qret/base/cast.h"
#include "qret/base/graph.h"
#include "qret/base/log.h"
#include "qret/ir/basic_block.h"
#include "qret/ir/instruction.h"
#include "qret/ir/instructions.h"
#include "qret/ir/module.h"
#include "qret/ir/value.h"
#include "qret/math/boolean_formula.h"

namespace qret::ir {
std::unique_ptr<Function> Function::Create(std::string_view name) {
    return std::unique_ptr<Function>(new Function(name, nullptr));
}

Function* Function::Create(std::string_view name, Module* parent) {
    auto func = std::unique_ptr<Function>(new Function(name, parent));
    auto* func_ptr = func.get();
    parent->function_list_.emplace_back(std::move(func));
    parent->ptr2itr_[func_ptr] = std::prev(parent->function_list_.end());
    return func_ptr;
}

Function::Function(std::string_view name, Module* parent)
    : name_{name}
    , parent_{parent} {}
void Function::SetEntryBB(const BasicBlock* bb) {
    if (bb == nullptr) {
        throw std::runtime_error("argument is nullptr");
    }
    if (bb->GetParent() != this) {
        throw std::runtime_error("invalid parent");
    }
    entry_point_ = ptr2itr_.at(bb);
}

namespace {
bool SetDepsImpl(
        const std::unordered_set<const BasicBlock*>& bbs,
        ir::BasicBlock* bb,
        ir::BranchInst* branch
) {
    auto* true_bb = branch->GetTrueBB();
    auto* false_bb = branch->GetFalseBB();
    if (branch->GetParent() != bb) {
        LOG_ERROR("BB of branch inst is unknown");
        return false;
    }
    if (true_bb != nullptr) {
        if (!bbs.contains(true_bb)) {
            LOG_ERROR("true BB of branch inst is unknown");
            return false;
        }
        bb->AddEdge(true_bb);
    }
    if (false_bb != nullptr && false_bb != true_bb) {
        if (!bbs.contains(false_bb)) {
            LOG_ERROR("false BB of branch inst is unknown");
            return false;
        }
        bb->AddEdge(false_bb);
    }
    return true;
}

bool SetDepsImpl(
        const std::unordered_set<const BasicBlock*>& bbs,
        ir::BasicBlock* bb,
        ir::SwitchInst* inst
) {
    if (inst->GetParent() != bb) {
        LOG_ERROR("BB of switch inst is unknown");
        return false;
    }

    auto* default_bb = inst->GetDefaultBB();
    if (default_bb != nullptr) {
        if (!bbs.contains(default_bb)) {
            LOG_ERROR("default BB of switch inst is unknown");
            return false;
        }
        bb->AddEdge(default_bb);
    }
    for (const auto& [key, case_bb] : inst->GetCaseBB()) {
        if (!bbs.contains(case_bb)) {
            LOG_ERROR("case {} BB of switch inst is unknown", key);
            return false;
        }
        bb->AddEdge(case_bb);
    }
    return true;
}
}  // namespace

bool Function::SetBBDeps() {
    // Create set of bb addresses
    auto bbs = std::unordered_set<const BasicBlock*>();
    for (auto&& bb : *this) {
        bbs.insert(&bb);
        bb.ClearPredecessors();
        bb.ClearSuccessors();
    }
    for (auto&& bb : *this) {
        auto* term = bb.GetTerminator();
        if (term == nullptr) {
            LOG_DEBUG("BB {} has no terminator", bb.GetName());
            continue;
        }

        if (term->GetOpcode() == Opcode::Table::Branch) {
            const auto success = SetDepsImpl(bbs, &bb, Cast<BranchInst>(term));
            if (!success) {
                return false;
            }
        } else if (term->GetOpcode() == Opcode::Table::Switch) {
            const auto success = SetDepsImpl(bbs, &bb, Cast<SwitchInst>(term));
            if (!success) {
                return false;
            }
        }
    }
    return true;
}

bool Function::RemoveUnreachableBBs() {
    if (GetEntryBB() == nullptr) {
        LOG_ERROR("entry BB is not set");
        return false;
    }
    if (GetNumBBs() == 1) {
        return true;
    }

    auto reachable_bbs = std::unordered_set<const BasicBlock*>();
    auto queue = std::queue<const BasicBlock*>();
    reachable_bbs.insert(GetEntryBB());
    queue.emplace(GetEntryBB());
    while (!queue.empty()) {
        const auto* bb = queue.front();
        queue.pop();
        for (auto itr = bb->succ_begin(); itr != bb->succ_end(); ++itr) {
            const auto* next = *itr;
            if (!reachable_bbs.contains(next)) {
                reachable_bbs.insert(next);
                queue.push(next);
            }
        }
    }

    auto unreachable_bbs = std::unordered_set<BasicBlock*>();
    for (auto&& bb : *this) {
        // Do not remove 'bb' here, as doing so would change the container on which this loop
        // depends.
        if (!reachable_bbs.contains(&bb)) {
            unreachable_bbs.insert(&bb);
        }
    }
    for (auto* bb : unreachable_bbs) {
        bb->ClearPredecessors();
        bb->ClearSuccessors();
        RemoveFromParent(bb);
    }
    return true;
}

bool Function::RemoveEmptyBBs() {
    auto remove_bbs = std::unordered_set<BasicBlock*>();
    for (auto&& bb : *this) {
        // Do not remove 'bb' here, as doing so would change the container on which this loop
        // depends.
        if (
                bb.Size() == 1 &&  // bb contains only terminator instruction
                DynCastIfPresent<BranchInst>(bb.GetTerminator()) != nullptr
                &&  // terminator is unconditional branch
                &bb != GetEntryBB() &&  // bb is not entry point
                bb.NumSuccessors() <= 1 &&  // bb has at most one successors
                bb.NumPredecessors() <= 1  // bb has at most one predecessors
        ) {
            remove_bbs.insert(&bb);
        }
    }
    for (auto* bb : remove_bbs) {
        auto* pred = bb->GetUniquePredecessor();
        auto* succ = bb->GetUniqueSuccessor();
        if (pred != nullptr && succ != nullptr) {
            pred->RemoveSuccessor(bb);
            succ->RemovePredecessor(bb);
            pred->AddEdge(succ);

            auto* inst = pred->GetTerminator();
            if (auto* terminator = DynCastIfPresent<BranchInst>(inst)) {
                terminator->ReplaceBB(bb, succ);
            } else if (auto* terminator = DynCastIfPresent<SwitchInst>(inst)) {
                terminator->ReplaceBB(bb, succ);
            }
        }

        RemoveFromParent(bb);
    }
    return true;
}

std::size_t Function::GetInstructionCount() const {
    return std::accumulate(
            bb_list_.begin(),
            bb_list_.end(),
            std::size_t{0},
            [](std::size_t acc, const auto& bb) { return acc + bb->Size(); }
    );
}

bool Function::DoesMeasurement() const {
    return std::ranges::any_of(bb_list_, [](const auto& bb) { return bb->DoesMeasurement(); });
}

bool Function::IsDAG() const {
    auto graph = DiGraph();
    auto mp = std::unordered_map<const BasicBlock*, DiGraph::IdType>();
    const auto get_id = [&mp](const ir::BasicBlock* bb) {
        if (mp.contains(bb)) {
            return mp[bb];
        }
        return mp[bb] = static_cast<DiGraph::IdType>(mp.size());
    };
    // Add node
    for (const auto& bb : *this) {
        graph.AddNode(get_id(&bb));
    }
    // Add edge
    for (const auto& bb : *this) {
        for (auto itr = bb.succ_begin(); itr != bb.succ_end(); ++itr) {
            graph.AddEdge(get_id(&bb), get_id(*itr));
        }
    }
    return graph.Topsort();
}

std::vector<std::pair<BasicBlock*, std::uint32_t>> Function::GetDepthOrderBB() const {
    if (!IsDAG()) {
        throw std::runtime_error("the dependency graph of BB must be DAG");
    }
    if (GetEntryBB() == nullptr) {
        throw std::runtime_error("entry BB is not set");
    }

    auto depth_map = std::unordered_map<BasicBlock*, std::uint32_t>();
    auto queue = std::queue<BasicBlock*>();
    queue.push(GetEntryBB());
    while (!queue.empty()) {
        auto* const bb = queue.front();
        queue.pop();
        const auto depth = depth_map[bb];
        for (auto itr = bb->succ_begin(); itr != bb->succ_end(); ++itr) {
            auto* next = *itr;
            if (depth_map.contains(next)) {
                depth_map[next] = std::max(depth_map[next], depth + 1);
            } else {
                depth_map[next] = depth + 1;
                queue.push(next);
            }
        }
    }

    auto ret = std::vector<std::pair<BasicBlock*, std::uint32_t>>();
    ret.reserve(depth_map.size());
    for (auto& [bb, depth] : depth_map) {
        ret.emplace_back(bb, depth);
    }
    std::sort(ret.begin(), ret.end(), [](const auto& lhs, const auto& rhs) {
        return std::get<1>(lhs) < std::get<1>(rhs);
    });
    return ret;
}

std::unordered_map<const BasicBlock*, math::Formula> Function::ConditionOfBB() const {
    if (!IsDAG()) {
        throw std::runtime_error("the dependency graph of BB must be DAG");
    }
    if (GetEntryBB() == nullptr) {
        throw std::runtime_error("entry BB is not set");
    }

    const auto depth_map = GetDepthOrderBB();

    auto paths = std::unordered_map<const BasicBlock*, math::Formula>();
    const auto add_new_path = [&paths](const BasicBlock* bb, const math::Formula& f) {
        if (paths.contains(bb)) {
            paths[bb] |= f;
        } else {
            paths[bb] = f;
        }
    };
    add_new_path(GetEntryBB(), math::Formula());

    for (const auto& [bb, _depth] : depth_map) {
        auto& f = paths[bb];
        f.Minimize();

        if (const auto* br = DynCastIfPresent<BranchInst>(bb->GetTerminator())) {
            if (br->IsConditional()) {
                const auto var = math::Var(br->GetCondition().id);
                const auto* true_bb = br->GetTrueBB();
                const auto* false_bb = br->GetFalseBB();
                add_new_path(true_bb, f & math::Lit(var, true));
                add_new_path(false_bb, f & math::Lit(var, false));
            } else {
                const auto* next_bb = br->GetTrueBB();
                add_new_path(next_bb, f);
            }
        } else if (const auto* sw = DynCastIfPresent<SwitchInst>(bb->GetTerminator())) {
            const auto num_values = sw->GetValue().size();
            if (num_values >= 15) {
                throw std::runtime_error(
                        "Size of 'value' of SwitchInst exceeds the allowed limit (15)."
                );
            }

            for (auto i = std::uint32_t{0}; i < std::uint32_t{1} << num_values; ++i) {
                auto term = math::Term();
                for (auto j = std::uint32_t{0}; j < num_values; ++j) {
                    const auto sign = ((i >> j) & 1) == 1;
                    term &= math::Lit(math::Var(j), sign);
                }
                auto* next_bb = sw->GetNext(i);
                add_new_path(next_bb, math::Formula(term));
            }
        }
    }

    return paths;
}

std::unique_ptr<Function> RemoveFromParent(Function* func) {
    if (func == nullptr || !func->HasParent()) {
        return nullptr;
    }

    auto* module = func->GetParent();
    const auto map_itr = module->ptr2itr_.find(func);
    assert(map_itr != module->ptr2itr_.end() && "func must be registered in parent");

    const auto itr = map_itr->second;
    auto ret = std::move(*itr);
    ret->SetParent(nullptr);
    module->function_list_.erase(itr);
    module->ptr2itr_.erase(map_itr);
    return ret;
}

void DeleteFunction(Function* func) {
    (void)RemoveFromParent(func);
}
}  // namespace qret::ir
