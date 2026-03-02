#include <gtest/gtest.h>

#include <unordered_set>

#include "qret/analysis/visualizer.h"
#include "qret/base/cast.h"
#include "qret/ir/basic_block.h"
#include "qret/ir/context.h"
#include "qret/ir/function.h"
#include "qret/ir/instruction.h"
#include "qret/ir/instructions.h"
#include "qret/ir/module.h"
#include "qret/ir/value.h"
#include "qret/math/boolean_formula.h"

using namespace qret;
using namespace qret::ir;

Function* CreateFunction(IRContext& ctx) {
    // A
    // | \
    // B  C (empty)
    // |  |
    // D  E (empty)  F
    // | /          /
    // G (empty)

    auto* module = Module::Create("module", ctx);
    auto* func = Function::Create("function", module);

    auto* A = BasicBlock::Create("A", func);
    auto* B = BasicBlock::Create("B", func);
    auto* C = BasicBlock::Create("C", func);
    auto* D = BasicBlock::Create("D", func);
    auto* E = BasicBlock::Create("E", func);
    auto* F = BasicBlock::Create("F", func);
    auto* G = BasicBlock::Create("G", func);
    func->SetEntryBB(A);

    // Insert hadamard
    for (auto* bb : {A, B, D, F}) {
        UnaryInst::Create(Opcode::Table::H, Qubit(0), bb);
    }

    // Insert branch inst
    BranchInst::Create(B, C, Register(0), A);
    BranchInst::Create(D, B);
    BranchInst::Create(E, C);
    BranchInst::Create(G, D);
    BranchInst::Create(G, E);
    BranchInst::Create(G, F);
    ReturnInst::Create(G);

    return func;
}
Function* CreateNonDAGFunction(IRContext& ctx) {
    // A
    // |  \
    // B <- C (empty)
    // |  / (D to C)
    // D
    // |
    // E

    auto* module = Module::Create("module", ctx);
    auto* func = Function::Create("function", module);

    auto* A = BasicBlock::Create("A", func);
    auto* B = BasicBlock::Create("B", func);
    auto* C = BasicBlock::Create("C", func);
    auto* D = BasicBlock::Create("D", func);
    auto* E = BasicBlock::Create("E", func);
    func->SetEntryBB(A);

    // Insert hadamard
    for (auto* bb : {A, B, D, E}) {
        UnaryInst::Create(Opcode::Table::H, Qubit(0), bb);
    }

    // Insert branch inst
    BranchInst::Create(B, C, Register(0), A);
    BranchInst::Create(D, B);
    BranchInst::Create(B, C);
    BranchInst::Create(C, E, Register(1), D);
    ReturnInst::Create(E);

    return func;
}
Function* CreateComplexDAGFunction(IRContext& ctx) {
    //     A(0)
    //    /  \
    //   B(1) C(2)
    //  / \   |  \
    // D(3) E  F  G
    // | \ /  | /
    // H  I   J
    // |  /  /
    // K

    auto* module = Module::Create("module", ctx);
    auto* func = Function::Create("function", module);

    auto* A = BasicBlock::Create("A", func);
    auto* B = BasicBlock::Create("B", func);
    auto* C = BasicBlock::Create("C", func);
    auto* D = BasicBlock::Create("D", func);
    auto* E = BasicBlock::Create("E", func);
    auto* F = BasicBlock::Create("F", func);
    auto* G = BasicBlock::Create("G", func);
    auto* H = BasicBlock::Create("H", func);
    auto* I = BasicBlock::Create("I", func);
    auto* J = BasicBlock::Create("J", func);
    auto* K = BasicBlock::Create("K", func);
    func->SetEntryBB(A);

    // Insert branch inst
    BranchInst::Create(B, C, Register(0), A);
    BranchInst::Create(D, E, Register(1), B);
    BranchInst::Create(F, G, Register(2), C);
    BranchInst::Create(H, I, Register(3), D);
    BranchInst::Create(I, E);
    BranchInst::Create(J, F);
    BranchInst::Create(J, G);
    BranchInst::Create(K, H);
    BranchInst::Create(K, I);
    BranchInst::Create(K, J);
    ReturnInst::Create(K);

    return func;
}
Function* CreateSwitchFunction(IRContext& ctx) {
    //     A(01)
    //    /  \ \ \
    //   B(2) C D |
    //  / \   | / /
    // E   F  G  /
    // \  /  /  /
    //  \ | /  /
    //     H

    auto* module = Module::Create("module", ctx);
    auto* func = Function::Create("function", module);

    auto* A = BasicBlock::Create("A", func);
    auto* B = BasicBlock::Create("B", func);
    auto* C = BasicBlock::Create("C", func);
    auto* D = BasicBlock::Create("D", func);
    auto* E = BasicBlock::Create("E", func);
    auto* F = BasicBlock::Create("F", func);
    auto* G = BasicBlock::Create("G", func);
    auto* H = BasicBlock::Create("H", func);
    func->SetEntryBB(A);

    // Insert branch inst
    SwitchInst::Create({Register(0), Register(1)}, H, {{0, B}, {1, C}, {2, D}}, A);
    BranchInst::Create(E, F, Register(2), B);
    BranchInst::Create(G, C);
    BranchInst::Create(G, D);
    BranchInst::Create(H, E);
    BranchInst::Create(H, F);
    BranchInst::Create(H, G);
    ReturnInst::Create(H);

    return func;
}
Function* CreateDuplicateSwitchFunction(IRContext& ctx) {
    //     A(01)
    //    || \\
    //     B  ||
    //     \ //
    //      C

    auto* module = Module::Create("module", ctx);
    auto* func = Function::Create("function", module);

    auto* A = BasicBlock::Create("A", func);
    auto* B = BasicBlock::Create("B", func);
    auto* C = BasicBlock::Create("C", func);

    func->SetEntryBB(A);

    // Insert branch inst
    SwitchInst::Create({Register(0), Register(1)}, C, {{0, B}, {3, C}, {2, B}}, A);
    BranchInst::Create(C, B);
    ReturnInst::Create(C);

    return func;
}
Function* CreateComplexDAGWithSwitchFunction(IRContext& ctx) {
    //     A(0,1,2)
    //    /  \    \  \   \
    //   B(3) C(4)  X  Y  Z
    //  / \   |  \  \ /   |
    // D(5) E  F  G  W   /
    // | \ /  | /   /  /
    // H  I   J    /  /
    // |  /  /    /  /
    // K

    auto* module = Module::Create("module", ctx);
    auto* func = Function::Create("function", module);

    auto* A = BasicBlock::Create("A", func);
    auto* X = BasicBlock::Create("X", func);
    auto* Y = BasicBlock::Create("Y", func);
    auto* Z = BasicBlock::Create("Z", func);
    auto* W = BasicBlock::Create("W", func);
    auto* B = BasicBlock::Create("B", func);
    auto* C = BasicBlock::Create("C", func);
    auto* D = BasicBlock::Create("D", func);
    auto* E = BasicBlock::Create("E", func);
    auto* F = BasicBlock::Create("F", func);
    auto* G = BasicBlock::Create("G", func);
    auto* H = BasicBlock::Create("H", func);
    auto* I = BasicBlock::Create("I", func);
    auto* J = BasicBlock::Create("J", func);
    auto* K = BasicBlock::Create("K", func);
    func->SetEntryBB(A);

    // Insert branch and switch inst
    SwitchInst::Create(
            {Register(0), Register(1), Register(2)},
            Z,
            {
                    {0, B},
                    {3, C},
                    {4, X},
                    {7, Y},
            },
            A
    );
    BranchInst::Create(W, X);
    BranchInst::Create(W, Y);
    BranchInst::Create(K, Z);
    BranchInst::Create(K, W);
    BranchInst::Create(D, E, Register(3), B);
    BranchInst::Create(F, G, Register(4), C);
    BranchInst::Create(H, I, Register(5), D);
    BranchInst::Create(I, E);
    BranchInst::Create(J, F);
    BranchInst::Create(J, G);
    BranchInst::Create(K, H);
    BranchInst::Create(K, I);
    BranchInst::Create(K, J);
    ReturnInst::Create(K);

    return func;
}

TEST(DAG, Dependency) {
    // A
    // | \
    // B  C (empty)
    // |  |
    // D  E (empty)  F
    // | /          /
    // G (empty)
    IRContext ctx;
    auto* func = CreateFunction(ctx);

    ASSERT_TRUE(func->SetBBDeps());
    EXPECT_EQ(7, func->GetNumBBs());
    EXPECT_TRUE(func->IsDAG());

    {
        auto* bb = func->SearchBB("A");
        ASSERT_EQ(0, std::distance(bb->pred_begin(), bb->pred_end()));
        ASSERT_EQ(2, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("B", (*bb->succ_begin())->GetName());
        EXPECT_EQ("C", (*std::next(bb->succ_begin()))->GetName());
    }
    {
        auto* bb = func->SearchBB("B");
        ASSERT_EQ(1, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("A", (*bb->pred_begin())->GetName());
        ASSERT_EQ(1, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("D", (*bb->succ_begin())->GetName());
    }
    {
        auto* bb = func->SearchBB("C");
        ASSERT_EQ(1, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("A", (*bb->pred_begin())->GetName());
        ASSERT_EQ(1, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("E", (*bb->succ_begin())->GetName());
    }
    {
        auto* bb = func->SearchBB("D");
        ASSERT_EQ(1, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("B", (*bb->pred_begin())->GetName());
        ASSERT_EQ(1, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("G", (*bb->succ_begin())->GetName());
    }
    {
        auto* bb = func->SearchBB("E");
        ASSERT_EQ(1, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("C", (*bb->pred_begin())->GetName());
        ASSERT_EQ(1, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("G", (*bb->succ_begin())->GetName());
    }
    {
        auto* bb = func->SearchBB("F");
        ASSERT_EQ(0, std::distance(bb->pred_begin(), bb->pred_end()));
        ASSERT_EQ(1, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("G", (*bb->succ_begin())->GetName());
    }
    {
        auto* bb = func->SearchBB("G");
        ASSERT_EQ(3, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("D", (*bb->pred_begin())->GetName());
        EXPECT_EQ("E", (*std::next(bb->pred_begin()))->GetName());
        EXPECT_EQ("F", (*std::next(bb->pred_begin(), 2))->GetName());
        ASSERT_EQ(0, std::distance(bb->succ_begin(), bb->succ_end()));
    }
}
TEST(DAG, RemoveUnreachableBBs) {
    // A
    // | \
    // B  C (empty)
    // |  |
    // D  E (empty)
    // | /
    // G (empty)
    IRContext ctx;
    auto* func = CreateFunction(ctx);

    ASSERT_TRUE(func->SetBBDeps());
    ASSERT_TRUE(func->RemoveUnreachableBBs());
    EXPECT_EQ(6, func->GetNumBBs());
    EXPECT_TRUE(func->IsDAG());

    {
        auto* bb = func->SearchBB("A");
        ASSERT_EQ(0, std::distance(bb->pred_begin(), bb->pred_end()));
        ASSERT_EQ(2, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("B", (*bb->succ_begin())->GetName());
        EXPECT_EQ("C", (*std::next(bb->succ_begin()))->GetName());
    }
    {
        auto* bb = func->SearchBB("B");
        ASSERT_EQ(1, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("A", (*bb->pred_begin())->GetName());
        ASSERT_EQ(1, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("D", (*bb->succ_begin())->GetName());
    }
    {
        auto* bb = func->SearchBB("C");
        ASSERT_EQ(1, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("A", (*bb->pred_begin())->GetName());
        ASSERT_EQ(1, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("E", (*bb->succ_begin())->GetName());
    }
    {
        auto* bb = func->SearchBB("D");
        ASSERT_EQ(1, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("B", (*bb->pred_begin())->GetName());
        ASSERT_EQ(1, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("G", (*bb->succ_begin())->GetName());
    }
    {
        auto* bb = func->SearchBB("E");
        ASSERT_EQ(1, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("C", (*bb->pred_begin())->GetName());
        ASSERT_EQ(1, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("G", (*bb->succ_begin())->GetName());
    }
    { EXPECT_EQ(nullptr, func->SearchBB("F")); }
    {
        auto* bb = func->SearchBB("G");
        ASSERT_EQ(2, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("D", (*bb->pred_begin())->GetName());
        EXPECT_EQ("E", (*std::next(bb->pred_begin()))->GetName());
        ASSERT_EQ(0, std::distance(bb->succ_begin(), bb->succ_end()));
    }
}
TEST(DAG, RemoveEmptyBBs) {
    // A
    // | \
    // B  |
    // |  |
    // D  |  F
    // | /  /
    // G (empty)
    IRContext ctx;
    auto* func = CreateFunction(ctx);

    ASSERT_TRUE(func->SetBBDeps());
    ASSERT_TRUE(func->RemoveEmptyBBs());
    EXPECT_EQ(5, func->GetNumBBs());
    EXPECT_TRUE(func->IsDAG());

    {
        auto* bb = func->SearchBB("A");
        ASSERT_EQ(0, std::distance(bb->pred_begin(), bb->pred_end()));
        ASSERT_EQ(2, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("B", (*bb->succ_begin())->GetName());
        EXPECT_EQ("G", (*std::next(bb->succ_begin()))->GetName());

        const auto* inst = Cast<BranchInst>(bb->GetTerminator());
        EXPECT_TRUE(inst->IsConditional());
        EXPECT_EQ("B", inst->GetTrueBB()->GetName());
        EXPECT_EQ("G", inst->GetFalseBB()->GetName());
    }
    {
        auto* bb = func->SearchBB("B");
        ASSERT_EQ(1, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("A", (*bb->pred_begin())->GetName());
        ASSERT_EQ(1, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("D", (*bb->succ_begin())->GetName());
    }
    { EXPECT_EQ(nullptr, func->SearchBB("C")); }
    {
        auto* bb = func->SearchBB("D");
        ASSERT_EQ(1, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("B", (*bb->pred_begin())->GetName());
        ASSERT_EQ(1, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("G", (*bb->succ_begin())->GetName());
    }
    { EXPECT_EQ(nullptr, func->SearchBB("E")); }
    {
        auto* bb = func->SearchBB("F");
        ASSERT_EQ(0, std::distance(bb->pred_begin(), bb->pred_end()));
        ASSERT_EQ(1, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("G", (*bb->succ_begin())->GetName());
    }
    {
        auto* bb = func->SearchBB("G");
        ASSERT_EQ(3, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("D", (*bb->pred_begin())->GetName());
        EXPECT_EQ("F", (*std::next(bb->pred_begin()))->GetName());
        EXPECT_EQ("A", (*std::next(bb->pred_begin(), 2))->GetName());
        ASSERT_EQ(0, std::distance(bb->succ_begin(), bb->succ_end()));
    }
}
TEST(DAG, RemoveUnreachableBBsAndEmptyBBs) {
    // A
    // | \
    // B  |
    // |  |
    // D  |
    // | /
    // G (empty)
    IRContext ctx;
    auto* func = CreateFunction(ctx);

    ASSERT_TRUE(func->SetBBDeps());
    ASSERT_TRUE(func->RemoveUnreachableBBs());
    ASSERT_TRUE(func->RemoveEmptyBBs());
    EXPECT_EQ(4, func->GetNumBBs());
    EXPECT_TRUE(func->IsDAG());

    {
        auto* bb = func->SearchBB("A");
        ASSERT_EQ(0, std::distance(bb->pred_begin(), bb->pred_end()));
        ASSERT_EQ(2, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("B", (*bb->succ_begin())->GetName());
        EXPECT_EQ("G", (*std::next(bb->succ_begin()))->GetName());

        const auto* inst = Cast<BranchInst>(bb->GetTerminator());
        EXPECT_TRUE(inst->IsConditional());
        EXPECT_EQ("B", inst->GetTrueBB()->GetName());
        EXPECT_EQ("G", inst->GetFalseBB()->GetName());
    }
    {
        auto* bb = func->SearchBB("B");
        ASSERT_EQ(1, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("A", (*bb->pred_begin())->GetName());
        ASSERT_EQ(1, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("D", (*bb->succ_begin())->GetName());
    }
    { EXPECT_EQ(nullptr, func->SearchBB("C")); }
    {
        auto* bb = func->SearchBB("D");
        ASSERT_EQ(1, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("B", (*bb->pred_begin())->GetName());
        ASSERT_EQ(1, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("G", (*bb->succ_begin())->GetName());
    }
    { EXPECT_EQ(nullptr, func->SearchBB("E")); }
    { EXPECT_EQ(nullptr, func->SearchBB("F")); }
    {
        auto* bb = func->SearchBB("G");
        ASSERT_EQ(2, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("D", (*bb->pred_begin())->GetName());
        EXPECT_EQ("A", (*std::next(bb->pred_begin()))->GetName());
        ASSERT_EQ(0, std::distance(bb->succ_begin(), bb->succ_end()));
    }
}
TEST(NonDAG, Dependency) {
    // A
    // |  \
    // B <- C (empty)
    // |  / (D to C)
    // D
    // |
    // E
    IRContext ctx;
    auto* func = CreateNonDAGFunction(ctx);

    ASSERT_TRUE(func->SetBBDeps());
    EXPECT_EQ(5, func->GetNumBBs());
    EXPECT_FALSE(func->IsDAG());

    {
        auto* bb = func->SearchBB("A");
        ASSERT_EQ(0, std::distance(bb->pred_begin(), bb->pred_end()));
        ASSERT_EQ(2, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("B", (*bb->succ_begin())->GetName());
        EXPECT_EQ("C", (*std::next(bb->succ_begin()))->GetName());
    }
    {
        auto* bb = func->SearchBB("B");
        ASSERT_EQ(2, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("A", (*bb->pred_begin())->GetName());
        EXPECT_EQ("C", (*std::next(bb->pred_begin()))->GetName());
        ASSERT_EQ(1, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("D", (*bb->succ_begin())->GetName());
    }
    {
        auto* bb = func->SearchBB("C");
        ASSERT_EQ(2, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("A", (*bb->pred_begin())->GetName());
        EXPECT_EQ("D", (*std::next(bb->pred_begin()))->GetName());
        ASSERT_EQ(1, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("B", (*bb->succ_begin())->GetName());
    }
    {
        auto* bb = func->SearchBB("D");
        ASSERT_EQ(1, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("B", (*bb->pred_begin())->GetName());
        ASSERT_EQ(2, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("C", (*bb->succ_begin())->GetName());
        EXPECT_EQ("E", (*std::next(bb->succ_begin()))->GetName());
    }
    {
        auto* bb = func->SearchBB("E");
        ASSERT_EQ(1, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("D", (*bb->pred_begin())->GetName());
        ASSERT_EQ(0, std::distance(bb->succ_begin(), bb->succ_end()));
    }
}
TEST(ComplexDAGWithSwitch, Condition) {
    //     A(0,1,2)
    //    /  \    \  \   \
    //   B(3) C(4)  X  Y  Z
    //  / \   |  \  \ /   |
    // D(5) E  F  G  W   /
    // | \ /  | /   /  /
    // H  I   J    /  /
    // |  /  /    /  /
    // K

    IRContext ctx;
    auto* func = CreateComplexDAGWithSwitchFunction(ctx);

    ASSERT_TRUE(func->SetBBDeps());
    EXPECT_EQ(15, func->GetNumBBs());
    EXPECT_TRUE(func->IsDAG());

    const auto t = std::vector<math::Lit>{
            math::Lit(math::Var(0)),
            math::Lit(math::Var(1)),
            math::Lit(math::Var(2)),
            math::Lit(math::Var(3)),
            math::Lit(math::Var(4)),
            math::Lit(math::Var(5)),
    };

    const auto condition_map = func->ConditionOfBB();
    {
        const auto& condition = condition_map.at(func->SearchBB("A"));
        EXPECT_EQ(0, condition.Size());
    }
    {
        // A ---> B (if value == 0)
        const auto& condition = condition_map.at(func->SearchBB("B"));
        EXPECT_EQ(1, condition.Size());
        const auto& term = *condition.begin();
        EXPECT_EQ((~t[0]) & (~t[1]) & (~t[2]), term);
    }
    {
        // A ---> C (if value == 3)
        const auto& condition = condition_map.at(func->SearchBB("C"));
        EXPECT_EQ(1, condition.Size());
        const auto& term = *condition.begin();
        EXPECT_EQ((t[0]) & (t[1]) & (~t[2]), term);
    }
    {
        // A ---> X (if value == 4)
        const auto& condition = condition_map.at(func->SearchBB("X"));
        EXPECT_EQ(1, condition.Size());
        const auto& term = *condition.begin();
        EXPECT_EQ((~t[0]) & (~t[1]) & (t[2]), term);
    }
    {
        // A ---> Y (if value == 7)
        const auto& condition = condition_map.at(func->SearchBB("Y"));
        EXPECT_EQ(1, condition.Size());
        const auto& term = *condition.begin();
        EXPECT_EQ((t[0]) & (t[1]) & (t[2]), term);
    }
    {
        // A ---> Z (if value == 1,2,5,6)
        // 100, 010, 101, 011
        // => r2 does not matter.
        // (-r0 AND r1) OR (r0 and -r1)
        const auto& condition = condition_map.at(func->SearchBB("Z"));
        std::cout << "Z: " << condition << std::endl;
        ASSERT_EQ(2, condition.Size());
        const auto expected = std::set<math::Term>{~t[0] & t[1], t[0] & ~t[1]};
        const auto& first_term = *condition.begin();
        EXPECT_TRUE(expected.contains(first_term));
        const auto& second_term = *std::next(condition.begin());
        EXPECT_TRUE(expected.contains(second_term));
    }
    {
        // X -> W, Y -> W
        const auto& condition = condition_map.at(func->SearchBB("W"));
        ASSERT_EQ(2, condition.Size());
        const auto expected =
                std::set<math::Term>{(~t[0]) & (~t[1]) & (t[2]), (t[0]) & (t[1]) & (t[2])};
        const auto& first_term = *condition.begin();
        EXPECT_TRUE(expected.contains(first_term));
        const auto& second_term = *std::next(condition.begin());
        EXPECT_TRUE(expected.contains(second_term));
    }
    {
        const auto& condition = condition_map.at(func->SearchBB("D"));
        EXPECT_EQ(1, condition.Size());
        const auto& term = *condition.begin();
        EXPECT_EQ((~t[0]) & (~t[1]) & (~t[2]) & (t[3]), term);
    }
    {
        const auto& condition = condition_map.at(func->SearchBB("E"));
        EXPECT_EQ(1, condition.Size());
        const auto& term = *condition.begin();
        EXPECT_EQ((~t[0]) & (~t[1]) & (~t[2]) & (~t[3]), term);
    }
    {
        const auto& condition = condition_map.at(func->SearchBB("F"));
        EXPECT_EQ(1, condition.Size());
        const auto& term = *condition.begin();
        EXPECT_EQ((t[0]) & (t[1]) & (~t[2]) & (t[4]), term);
    }
    {
        const auto& condition = condition_map.at(func->SearchBB("G"));
        EXPECT_EQ(1, condition.Size());
        const auto& term = *condition.begin();
        EXPECT_EQ((t[0]) & (t[1]) & (~t[2]) & (~t[4]), term);
    }
    {
        const auto& condition = condition_map.at(func->SearchBB("H"));
        EXPECT_EQ(1, condition.Size());
        const auto& term = *condition.begin();
        EXPECT_EQ((~t[0]) & (~t[1]) & (~t[2]) & (t[3]) & (t[5]), term);
    }
    {
        const auto& condition = condition_map.at(func->SearchBB("I"));
        std::cout << "I: " << condition << std::endl;
        EXPECT_EQ(2, condition.Size());
        const auto expected1 = std::set<math::Term>{
                (~t[0]) & (~t[1]) & (~t[2]) & (t[3]) & (~t[5]),  // D -> I
                (~t[0]) & (~t[1]) & (~t[2]) & (~t[3])  // E -> I
        };
        const auto expected2 = std::set<math::Term>{
                (~t[0]) & (~t[1]) & (~t[2]) & (~t[3]) & (t[5]),  //
                (~t[0]) & (~t[1]) & (~t[2]) & (~t[5])  //
        };
        const auto& first_term = *condition.begin();
        EXPECT_TRUE(expected1.contains(first_term) || expected2.contains(first_term));
        const auto& second_term = *std::next(condition.begin());
        EXPECT_TRUE(expected1.contains(second_term) || expected2.contains(second_term));
    }
    {
        const auto& condition = condition_map.at(func->SearchBB("J"));
        EXPECT_EQ(1, condition.Size());
        const auto& term = *condition.begin();
        EXPECT_EQ((t[0]) & (t[1]) & (~t[2]), term);
    }
    {
        const auto& condition = condition_map.at(func->SearchBB("K"));
        EXPECT_EQ(0, condition.Size());
    }
}
TEST(DAG, SwitchDependency) {
    //     A(01)
    //    /  \ \ \
    //   B(2) C D |
    //  / \   | / /
    // E   F  G  /
    // \  /  /  /
    //  \ | /  /
    //     H

    IRContext ctx;
    auto* func = CreateSwitchFunction(ctx);

    for (const auto& bb : *func) {
        std::cout << bb.GetName() << std::endl;
        for (const auto& inst : bb) {
            std::cout << "  ";
            inst.Print(std::cout);
            std::cout << std::endl;
        }
    }

    ASSERT_TRUE(func->SetBBDeps());
    EXPECT_EQ(8, func->GetNumBBs());
    EXPECT_TRUE(func->IsDAG());

    {
        auto* bb = func->SearchBB("A");
        ASSERT_EQ(0, std::distance(bb->pred_begin(), bb->pred_end()));
        ASSERT_EQ(4, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("H", (*(bb->succ_begin()))->GetName());
        EXPECT_EQ("B", (*std::next(bb->succ_begin()))->GetName());
        EXPECT_EQ("C", (*std::next(bb->succ_begin(), 2))->GetName());
        EXPECT_EQ("D", (*std::next(bb->succ_begin(), 3))->GetName());
    }
    {
        auto* bb = func->SearchBB("B");
        ASSERT_EQ(1, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("A", (*bb->pred_begin())->GetName());
        ASSERT_EQ(2, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("E", (*bb->succ_begin())->GetName());
        EXPECT_EQ("F", (*std::next(bb->succ_begin()))->GetName());
    }
    {
        auto* bb = func->SearchBB("C");
        ASSERT_EQ(1, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("A", (*bb->pred_begin())->GetName());
        ASSERT_EQ(1, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("G", (*bb->succ_begin())->GetName());
    }
    {
        auto* bb = func->SearchBB("D");
        ASSERT_EQ(1, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("A", (*bb->pred_begin())->GetName());
        ASSERT_EQ(1, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("G", (*bb->succ_begin())->GetName());
    }
    {
        auto* bb = func->SearchBB("E");
        ASSERT_EQ(1, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("B", (*bb->pred_begin())->GetName());
        ASSERT_EQ(1, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("H", (*bb->succ_begin())->GetName());
    }
    {
        auto* bb = func->SearchBB("F");
        ASSERT_EQ(1, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("B", (*bb->pred_begin())->GetName());
        ASSERT_EQ(1, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("H", (*bb->succ_begin())->GetName());
    }
    {
        auto* bb = func->SearchBB("G");
        ASSERT_EQ(2, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("C", (*bb->pred_begin())->GetName());
        EXPECT_EQ("D", (*std::next(bb->pred_begin()))->GetName());
        ASSERT_EQ(1, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("H", (*bb->succ_begin())->GetName());
    }
    {
        auto* bb = func->SearchBB("H");
        ASSERT_EQ(4, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("A", (*bb->pred_begin())->GetName());
        EXPECT_EQ("E", (*std::next(bb->pred_begin()))->GetName());
        EXPECT_EQ("F", (*std::next(bb->pred_begin(), 2))->GetName());
        EXPECT_EQ("G", (*std::next(bb->pred_begin(), 3))->GetName());
        ASSERT_EQ(0, std::distance(bb->succ_begin(), bb->succ_end()));
    }
}
TEST(DAG, DuplicateSwitchDependency) {
    //     A(01)
    //    || \\
    //     B  ||
    //     \ //
    //      C

    IRContext ctx;
    auto* func = CreateDuplicateSwitchFunction(ctx);

    for (const auto& bb : *func) {
        std::cout << bb.GetName() << std::endl;
        for (const auto& inst : bb) {
            std::cout << "  ";
            inst.Print(std::cout);
            std::cout << std::endl;
        }
    }

    ASSERT_TRUE(func->SetBBDeps());
    EXPECT_EQ(3, func->GetNumBBs());
    EXPECT_TRUE(func->IsDAG());

    {
        auto* bb = func->SearchBB("A");
        ASSERT_EQ(0, std::distance(bb->pred_begin(), bb->pred_end()));
        ASSERT_EQ(2, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("C", (*(bb->succ_begin()))->GetName());
        EXPECT_EQ("B", (*std::next(bb->succ_begin()))->GetName());
    }
    {
        auto* bb = func->SearchBB("B");
        ASSERT_EQ(1, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("A", (*bb->pred_begin())->GetName());
        ASSERT_EQ(1, std::distance(bb->succ_begin(), bb->succ_end()));
        EXPECT_EQ("C", (*bb->succ_begin())->GetName());
    }
    {
        auto* bb = func->SearchBB("C");
        ASSERT_EQ(2, std::distance(bb->pred_begin(), bb->pred_end()));
        EXPECT_EQ("A", (*bb->pred_begin())->GetName());
        EXPECT_EQ("B", (*std::next(bb->pred_begin()))->GetName());
        ASSERT_EQ(0, std::distance(bb->succ_begin(), bb->succ_end()));
    }

    std::cout << GenCFG(*func) << std::endl;
}
