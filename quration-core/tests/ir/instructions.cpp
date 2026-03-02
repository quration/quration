#include "qret/ir/instructions.h"

#include <gtest/gtest.h>

#include "qret/ir/basic_block.h"
#include "qret/ir/context.h"
#include "qret/ir/function.h"
#include "qret/ir/instruction.h"
#include "qret/ir/module.h"
#include "qret/ir/value.h"

TEST(TestIR, ClassOf) {
    using namespace qret::ir;

    auto context = IRContext();
    auto* module = Module::Create("module", context);
    auto* func = Function::Create("function", module);
    auto* bb = BasicBlock::Create("entry", func);

    EXPECT_TRUE(MeasurementInst::ClassOf(MeasurementInst::Create(Qubit(0), Register(0), bb)));
    EXPECT_FALSE(UnaryInst::ClassOf(MeasurementInst::Create(Qubit(0), Register(0), bb)));
    EXPECT_FALSE(
            ParametrizedRotationInst::ClassOf(MeasurementInst::Create(Qubit(0), Register(0), bb))
    );
    EXPECT_FALSE(BinaryInst::ClassOf(MeasurementInst::Create(Qubit(0), Register(0), bb)));
    EXPECT_FALSE(TernaryInst::ClassOf(MeasurementInst::Create(Qubit(0), Register(0), bb)));
    EXPECT_FALSE(MultiControlInst::ClassOf(MeasurementInst::Create(Qubit(0), Register(0), bb)));
    EXPECT_FALSE(GlobalPhaseInst::ClassOf(MeasurementInst::Create(Qubit(0), Register(0), bb)));
    EXPECT_FALSE(CallInst::ClassOf(MeasurementInst::Create(Qubit(0), Register(0), bb)));
    EXPECT_FALSE(CallCFInst::ClassOf(MeasurementInst::Create(Qubit(0), Register(0), bb)));
    EXPECT_FALSE(DiscreteDistInst::ClassOf(MeasurementInst::Create(Qubit(0), Register(0), bb)));
    EXPECT_FALSE(BranchInst::ClassOf(MeasurementInst::Create(Qubit(0), Register(0), bb)));
    EXPECT_FALSE(ReturnInst::ClassOf(MeasurementInst::Create(Qubit(0), Register(0), bb)));
    EXPECT_FALSE(CleanInst::ClassOf(MeasurementInst::Create(Qubit(0), Register(0), bb)));
    EXPECT_FALSE(CleanProbInst::ClassOf(MeasurementInst::Create(Qubit(0), Register(0), bb)));
    EXPECT_FALSE(DirtyInst::ClassOf(MeasurementInst::Create(Qubit(0), Register(0), bb)));

    EXPECT_FALSE(MeasurementInst::ClassOf(UnaryInst::Create(Opcode::Table::RX, Qubit(0), bb)));
    EXPECT_TRUE(UnaryInst::ClassOf(UnaryInst::Create(Opcode::Table::RX, Qubit(0), bb)));
    EXPECT_TRUE(
            ParametrizedRotationInst::ClassOf(UnaryInst::Create(Opcode::Table::RX, Qubit(0), bb))
    );
    EXPECT_FALSE(BinaryInst::ClassOf(UnaryInst::Create(Opcode::Table::RX, Qubit(0), bb)));
    EXPECT_FALSE(TernaryInst::ClassOf(UnaryInst::Create(Opcode::Table::RX, Qubit(0), bb)));

    EXPECT_TRUE(UnaryInst::ClassOf(UnaryInst::Create(Opcode::Table::X, Qubit(0), bb)));
    EXPECT_FALSE(
            ParametrizedRotationInst::ClassOf(UnaryInst::Create(Opcode::Table::X, Qubit(0), bb))
    );
}
