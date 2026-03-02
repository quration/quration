/**
 * @file qret/frontend/control_flow.cpp
 * @brief Manipulators of control flow in quantum circuits.
 */

#include "qret/frontend/control_flow.h"

#include <fmt/core.h>

#include <cassert>
#include <stdexcept>
#include <vector>

#include "qret/frontend/argument.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/ir/basic_block.h"
#include "qret/ir/instruction.h"
#include "qret/ir/instructions.h"
#include "qret/ir/value.h"

namespace qret::frontend::control_flow {
namespace {
ir::Register GetR(const Register& r) {
    return {r.GetId()};
}
std::vector<ir::Register> GetR(const Registers& rs) {
    auto ret = std::vector<ir::Register>{};
    for (const auto& r : rs) {
        ret.emplace_back(GetR(r));
    }
    return ret;
}
}  // namespace

void If(const Register& cond) {
    const auto* build_context = cond.GetBuilder()->GetCurrentBuildContext();
    If(cond, fmt::format("{}", build_context->num_if));
}
void If(const Register& cond, std::string branch_name) {
    // Create BBs.
    auto* circuit = cond.GetBuilder()->GetCurrentCircuit()->GetIR();
    auto* if_true = ir::BasicBlock::Create(fmt::format("then_{}", branch_name), circuit);
    auto* if_cont = ir::BasicBlock::Create(fmt::format("if_cont_{}", branch_name), circuit);

    // Update current build context.
    auto* update_context = cond.GetBuilder()->GetCurrentBuildContext();
    update_context->num_if++;
    update_context->branch.emplace_back(
            std::make_unique<BranchContext>(BranchContext{
                    .cond = cond,
                    .branch_name = branch_name,
                    .bb_to_insert_branch = update_context->insertion_point,
                    .if_true = if_true,
                    .if_cont = if_cont,
            })
    );
    update_context->insertion_point = if_true;
}
void Else(const Register& cond) {
    const auto* build_context = cond.GetBuilder()->GetCurrentBuildContext();
    if (!build_context->IsBranching()) {
        throw std::runtime_error("If is not called.");
    }

    const auto* branch_context = build_context->branch.back().get();
    if (!branch_context->IsIfTrueBranch()) {
        throw std::runtime_error("If is not called.");
    }
    if (!branch_context->SameCond(cond)) {
        throw std::runtime_error("cond is different from before.");
    }

    // Create if_false BB.
    auto* circuit = cond.GetBuilder()->GetCurrentCircuit()->GetIR();
    auto* if_false =
            ir::BasicBlock::Create(fmt::format("else_{}", branch_context->branch_name), circuit);

    // Insert unconditional branch from if_true to if_cont.
    ir::BranchInst::Create(branch_context->if_cont, build_context->insertion_point);
    build_context->insertion_point->AddEdge(branch_context->if_cont);

    // Update current build context.
    auto* update_context = cond.GetBuilder()->GetCurrentBuildContext();
    update_context->branch.back()->if_false = if_false;
    update_context->insertion_point = if_false;
}
void EndIf(const Register& cond) {
    const auto* build_context = cond.GetBuilder()->GetCurrentBuildContext();
    if (!build_context->IsBranching()) {
        throw std::runtime_error("If is not called");
    }

    const auto* branch_context = build_context->branch.back().get();
    if (!branch_context->SameCond(cond)) {
        throw std::runtime_error("cond is different from before.");
    }

    // Insert BranchInst of this branch.
    if (branch_context->if_false == nullptr) {
        ir::BranchInst::Create(
                branch_context->if_true,
                branch_context->if_cont,
                GetR(cond),
                branch_context->bb_to_insert_branch
        );
        branch_context->bb_to_insert_branch->AddEdge(branch_context->if_true);
        branch_context->bb_to_insert_branch->AddEdge(branch_context->if_cont);
    } else {
        ir::BranchInst::Create(
                branch_context->if_true,
                branch_context->if_false,
                GetR(cond),
                branch_context->bb_to_insert_branch
        );
        branch_context->bb_to_insert_branch->AddEdge(branch_context->if_true);
        branch_context->bb_to_insert_branch->AddEdge(branch_context->if_false);
    }

    // Insert unconditional branch from if_true or if_false to if_cont.
    auto* if_cont = branch_context->if_cont;
    ir::BranchInst::Create(if_cont, build_context->insertion_point);

    // Update current build context.
    auto* update_context = cond.GetBuilder()->GetCurrentBuildContext();
    update_context->branch.pop_back();
    update_context->insertion_point = if_cont;
}
void Switch(const Registers& cond) {
    const auto* build_context = cond.GetBuilder()->GetCurrentBuildContext();
    Switch(cond, fmt::format("{}", build_context->num_switch));
}
void Switch(const Registers& cond, std::string branch_name) {
    // Create BBs.
    auto* circuit = cond.GetBuilder()->GetCurrentCircuit()->GetIR();
    auto* switch_cont = ir::BasicBlock::Create(fmt::format("switch_cont_{}", branch_name), circuit);

    // Update current build context.
    auto* update_context = cond.GetBuilder()->GetCurrentBuildContext();
    update_context->num_switch++;
    update_context->branch.emplace_back(
            std::make_unique<BranchContext>(BranchContext{
                    .cond = cond,
                    .branch_name = branch_name,
                    .bb_to_insert_branch = update_context->insertion_point,
                    .switch_cont = switch_cont,
            })
    );
    update_context->insertion_point = nullptr;
}
void Case(const Registers& value, std::uint64_t key) {
    const auto* build_context = value.GetBuilder()->GetCurrentBuildContext();
    if (!build_context->IsBranching()) {
        throw std::runtime_error("Switch is not called.");
    }

    const auto* branch_context = build_context->branch.back().get();
    if (!branch_context->SameValue(value)) {
        throw std::runtime_error("value is different from before.");
    }
    if (branch_context->case_bb.contains(key)) {
        throw std::runtime_error("Duplicate case key.");
    }

    // Create case BB.
    auto* circuit = value.GetBuilder()->GetCurrentCircuit()->GetIR();
    auto* case_bb = ir::BasicBlock::Create(
            fmt::format("case_{}_{}", key, branch_context->branch_name),
            circuit
    );

    // Insert unconditional branch from to switch_cont.
    if (build_context->insertion_point != nullptr) {
        ir::BranchInst::Create(branch_context->switch_cont, build_context->insertion_point);
        build_context->insertion_point->AddEdge(branch_context->switch_cont);
    }

    // Update current build context.
    auto* update_context = value.GetBuilder()->GetCurrentBuildContext();
    update_context->branch.back()->case_bb[key] = case_bb;
    update_context->insertion_point = case_bb;
}
void Default(const Registers& value) {
    const auto* build_context = value.GetBuilder()->GetCurrentBuildContext();
    if (!build_context->IsBranching()) {
        throw std::runtime_error("Switch is not called");
    }

    const auto* branch_context = build_context->branch.back().get();
    if (!branch_context->SameValue(value)) {
        throw std::runtime_error("value is different from before.");
    }

    // Create default BB.
    auto* circuit = value.GetBuilder()->GetCurrentCircuit()->GetIR();
    auto* default_bb =
            ir::BasicBlock::Create(fmt::format("default_{}", branch_context->branch_name), circuit);

    // Insert unconditional branch from to switch_cont.
    if (build_context->insertion_point != nullptr) {
        ir::BranchInst::Create(branch_context->switch_cont, build_context->insertion_point);
        build_context->insertion_point->AddEdge(branch_context->switch_cont);
    }

    // Update current build context.
    auto* update_context = value.GetBuilder()->GetCurrentBuildContext();
    update_context->branch.back()->default_bb = default_bb;
    update_context->insertion_point = default_bb;
}
void EndSwitch(const Registers& value) {
    const auto* build_context = value.GetBuilder()->GetCurrentBuildContext();
    if (!build_context->IsBranching()) {
        throw std::runtime_error("Switch is not called");
    }

    const auto* branch_context = build_context->branch.back().get();
    if (!branch_context->SameValue(value)) {
        throw std::runtime_error("value is different from before.");
    }

    if (branch_context->default_bb == nullptr) {
        throw std::runtime_error("Default is not set in Switch");
    }

    // Insert SwitchInst of this branch.
    ir::SwitchInst::Create(
            GetR(value),
            branch_context->default_bb,
            branch_context->case_bb,
            branch_context->bb_to_insert_branch
    );
    branch_context->bb_to_insert_branch->AddEdge(branch_context->default_bb);
    for (const auto& [_, case_bb] : branch_context->case_bb) {
        branch_context->bb_to_insert_branch->AddEdge(case_bb);
    }

    // Insert unconditional branch from to switch_cont.
    auto* switch_cont = branch_context->switch_cont;
    ir::BranchInst::Create(switch_cont, build_context->insertion_point);

    // Update current build context.
    auto* update_context = value.GetBuilder()->GetCurrentBuildContext();
    update_context->branch.pop_back();
    update_context->insertion_point = switch_cont;
}
}  // namespace qret::frontend::control_flow
