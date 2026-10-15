#include "qret/parser/qir.h"

#include <gtest/gtest.h>

#include <string>
#include <variant>
#include <vector>

#include "qret/error.h"
#include "qret/exception.h"

using qret::QRETError;
using qret::qir::ParsedBlock;
using qret::qir::ParsedCall;
using qret::qir::ParsedIcmpInst;
using qret::qir::ParsedIntInst;
using qret::qir::ParsedPhi;
using qret::qir::ParsedSelectInst;
using qret::qir::ParseQIRText;

namespace {
// Extracts every T-typed item from a block's items, in original order.
template <typename T>
std::vector<T> ItemsOfType(const ParsedBlock& block) {
    auto result = std::vector<T>();
    for (const auto& item : block.items) {
        if (const auto* value = std::get_if<T>(&item)) {
            result.push_back(*value);
        }
    }
    return result;
}
}  // namespace

TEST(QirParser, ParsesFunctionWithCallsAndAttributeGroup) {
    if (!qret::qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR parser test";
    }
    const auto text = std::string(R"(
define i64 @main() #0 {
entry:
  call void @__quantum__rt__initialize(ptr null)
  call void @__quantum__qis__h__body(ptr null)
  call void @__quantum__qis__cnot__body(ptr null, ptr inttoptr (i64 1 to ptr))
  call void @__quantum__qis__mz__body(ptr null, ptr writeonly null)
  %c0 = call i1 @__quantum__qis__read_result__body(ptr null)
  ret i64 0
}
attributes #0 = { "entry_point" "required_num_qubits"="2" "required_num_results"="1" }
)");

    const auto module = ParseQIRText(text);
    ASSERT_EQ(1, module.functions.size());
    const auto& func = module.functions.front();
    EXPECT_EQ("main", func.name);
    ASSERT_TRUE(func.attr_group.has_value());
    EXPECT_EQ(0, *func.attr_group);
    ASSERT_EQ(1u, module.attr_group_required_qubits.count(0));
    EXPECT_EQ(2u, module.attr_group_required_qubits.at(0));

    ASSERT_EQ(1, func.blocks.size());
    const auto& block = func.blocks.front();
    EXPECT_EQ("entry", block.name);
    const auto calls = ItemsOfType<ParsedCall>(block);
    ASSERT_EQ(5, calls.size());
    EXPECT_EQ("__quantum__rt__initialize", calls[0].function);
    EXPECT_EQ((std::vector<std::string>{"ptr null"}), calls[0].args);
    EXPECT_EQ("__quantum__qis__cnot__body", calls[2].function);
    EXPECT_EQ((std::vector<std::string>{"ptr null", "ptr inttoptr (i64 1 to ptr)"}), calls[2].args);
    EXPECT_EQ("ptr null", calls[3].args.at(1));
    EXPECT_EQ("%c0", calls[4].result_var);
    EXPECT_EQ("__quantum__qis__read_result__body", calls[4].function);
    EXPECT_EQ(ParsedBlock::Term::Return, block.term);
}

TEST(QirParser, ParsesMultiLineSwitchBlock) {
    if (!qret::qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR parser test";
    }
    const auto text = std::string(R"(
define i64 @main() #0 {
entry:
  %v0 = zext i1 %c0 to i64
  %v1 = zext i1 %c1 to i64
  %v2 = shl i64 %v1, 1
  %v3 = or i64 %v0, %v2
  switch i64 %v3, label %default [
    i64 0, label %case0
    i64 1, label %case1
  ]
case0:
  ret i64 0
case1:
  ret i64 0
default:
  ret i64 0
}
)");

    const auto module = ParseQIRText(text);
    ASSERT_EQ(1, module.functions.size());
    const auto& func = module.functions.front();
    ASSERT_EQ(4, func.blocks.size());

    const auto& entry = func.blocks[0];
    const auto int_ops = ItemsOfType<ParsedIntInst>(entry);
    ASSERT_EQ(4, int_ops.size());
    EXPECT_EQ(ParsedIntInst::Kind::Zext, int_ops[0].kind);
    EXPECT_EQ("%v0", int_ops[0].result_var);
    EXPECT_EQ("%c0", int_ops[0].operand0);
    EXPECT_EQ(ParsedIntInst::Kind::Shl, int_ops[2].kind);
    EXPECT_EQ("%v1", int_ops[2].operand0);
    EXPECT_EQ("1", int_ops[2].operand1);
    EXPECT_EQ(ParsedIntInst::Kind::Or, int_ops[3].kind);

    EXPECT_EQ(ParsedBlock::Term::Switch, entry.term);
    EXPECT_EQ("%v3", entry.switch_value);
    EXPECT_EQ("default", entry.switch_default);
    ASSERT_EQ(2, entry.switch_cases.size());
    EXPECT_EQ(0u, entry.switch_cases[0].first);
    EXPECT_EQ("case0", entry.switch_cases[0].second);
    EXPECT_EQ(1u, entry.switch_cases[1].first);
    EXPECT_EQ("case1", entry.switch_cases[1].second);
}

TEST(QirParser, ParsesArithmeticAndBitwiseOps) {
    if (!qret::qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR parser test";
    }
    const auto text = std::string(R"(
define i64 @main() #0 {
entry:
  %v0 = add i64 %a, 1
  %v1 = sub i64 %v0, %b
  %v2 = mul i64 %v1, %v1
  %v3 = sdiv i64 %v2, -3
  %v4 = srem i64 %v3, %v2
  %v5 = shl i64 %v4, %v0
  %v6 = ashr i64 %v5, 2
  %v7 = and i64 %v6, %v6
  %v8 = or i64 %v7, 1
  %v9 = xor i64 %v8, -1
  ret i64 0
}
)");

    const auto module = ParseQIRText(text);
    const auto& block = module.functions.front().blocks.front();
    const auto int_ops = ItemsOfType<ParsedIntInst>(block);
    ASSERT_EQ(10, int_ops.size());
    EXPECT_EQ(ParsedIntInst::Kind::Add, int_ops[0].kind);
    EXPECT_EQ("%a", int_ops[0].operand0);
    EXPECT_EQ("1", int_ops[0].operand1);
    EXPECT_EQ(ParsedIntInst::Kind::Sub, int_ops[1].kind);
    EXPECT_EQ("%b", int_ops[1].operand1);
    EXPECT_EQ(ParsedIntInst::Kind::Mul, int_ops[2].kind);
    EXPECT_EQ(ParsedIntInst::Kind::Sdiv, int_ops[3].kind);
    EXPECT_EQ("-3", int_ops[3].operand1);
    EXPECT_EQ(ParsedIntInst::Kind::Srem, int_ops[4].kind);
    // shl's shift amount may now be a register, not just a decimal literal.
    EXPECT_EQ(ParsedIntInst::Kind::Shl, int_ops[5].kind);
    EXPECT_EQ("%v0", int_ops[5].operand1);
    EXPECT_EQ(ParsedIntInst::Kind::Ashr, int_ops[6].kind);
    EXPECT_EQ(ParsedIntInst::Kind::And, int_ops[7].kind);
    EXPECT_FALSE(int_ops[7].is_bool_typed);
    // or's second operand may now be a literal, not just a register.
    EXPECT_EQ(ParsedIntInst::Kind::Or, int_ops[8].kind);
    EXPECT_EQ("1", int_ops[8].operand1);
    EXPECT_EQ(ParsedIntInst::Kind::Xor, int_ops[9].kind);
    EXPECT_EQ("-1", int_ops[9].operand1);
}

// `items` preserves calls/int_ops/icmp_ops in their original interleaved order.
TEST(QirParser, RecordsItemOrderInterleavingCallsAndClassicalOps) {
    if (!qret::qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR parser test";
    }
    const auto text = std::string(R"(
define i64 @main() #0 {
entry:
  %v0 = add i64 %a, 1
  call void @__quantum__qis__h__body(ptr null)
  %v1 = mul i64 %v0, 2
  %v2 = icmp eq i64 %v1, 3
  ret i64 0
}
)");

    const auto module = ParseQIRText(text);
    const auto& block = module.functions.front().blocks.front();
    ASSERT_EQ(4, block.items.size());
    ASSERT_TRUE(std::holds_alternative<ParsedIntInst>(block.items[0]));
    EXPECT_EQ("%v0", std::get<ParsedIntInst>(block.items[0]).result_var);
    EXPECT_TRUE(std::holds_alternative<ParsedCall>(block.items[1]));
    ASSERT_TRUE(std::holds_alternative<ParsedIntInst>(block.items[2]));
    EXPECT_EQ("%v1", std::get<ParsedIntInst>(block.items[2]).result_var);
    EXPECT_TRUE(std::holds_alternative<ParsedIcmpInst>(block.items[3]));
}

TEST(QirParser, ParsesBoolTypedAndOrXorWithTrueFalseLiterals) {
    if (!qret::qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR parser test";
    }
    const auto text = std::string(R"(
define i64 @main() #0 {
entry:
  %v0 = and i1 %a, %b
  %v1 = or i1 %v0, false
  %v2 = xor i1 %v1, true
  ret i64 0
}
)");

    const auto module = ParseQIRText(text);
    const auto& block = module.functions.front().blocks.front();
    const auto int_ops = ItemsOfType<ParsedIntInst>(block);
    ASSERT_EQ(3, int_ops.size());
    EXPECT_EQ(ParsedIntInst::Kind::And, int_ops[0].kind);
    EXPECT_TRUE(int_ops[0].is_bool_typed);
    EXPECT_EQ(ParsedIntInst::Kind::Or, int_ops[1].kind);
    EXPECT_TRUE(int_ops[1].is_bool_typed);
    EXPECT_EQ("false", int_ops[1].operand1);
    // `xor %x, true` is the logical-not idiom; no dedicated "not" grammar node is needed.
    EXPECT_EQ(ParsedIntInst::Kind::Xor, int_ops[2].kind);
    EXPECT_TRUE(int_ops[2].is_bool_typed);
    EXPECT_EQ("true", int_ops[2].operand1);
}

TEST(QirParser, ParsesIcmpAgainstBoolTypeAndRegister) {
    if (!qret::qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR parser test";
    }
    const auto text = std::string(R"(
define i64 @main() #0 {
entry:
  %v0 = icmp eq i1 %a, %b
  %v1 = icmp eq i1 %a, false
  %v2 = icmp slt i64 %c, %d
  ret i64 0
}
)");

    const auto module = ParseQIRText(text);
    const auto& block = module.functions.front().blocks.front();
    const auto icmp_ops = ItemsOfType<ParsedIcmpInst>(block);
    ASSERT_EQ(3, icmp_ops.size());
    EXPECT_TRUE(icmp_ops[0].is_bool_typed);
    EXPECT_EQ("%a", icmp_ops[0].lhs);
    EXPECT_EQ("%b", icmp_ops[0].rhs);
    EXPECT_TRUE(icmp_ops[1].is_bool_typed);
    EXPECT_EQ("false", icmp_ops[1].rhs);
    EXPECT_FALSE(icmp_ops[2].is_bool_typed);
    EXPECT_EQ(ParsedIcmpInst::Predicate::Slt, icmp_ops[2].pred);
    EXPECT_EQ("%d", icmp_ops[2].rhs);
}

TEST(QirParser, ParsesPhiWithTwoIncomingEdges) {
    if (!qret::qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR parser test";
    }
    const auto text = std::string(R"(
define i64 @main() #0 {
entry:
  br i1 %c, label %then, label %else
then:
  br label %join
else:
  br label %join
join:
  %v0 = phi i64 [0, %then], [%x, %else]
  %v1 = phi i1 [true, %then], [%y, %else]
  ret i64 0
}
)");

    const auto module = ParseQIRText(text);
    const auto& join = module.functions.front().blocks.at(3);
    EXPECT_EQ("join", join.name);
    const auto phis = ItemsOfType<ParsedPhi>(join);
    ASSERT_EQ(2, phis.size());
    const auto& int_phi = phis[0];
    EXPECT_EQ("%v0", int_phi.result_var);
    EXPECT_EQ("join", int_phi.block_name);
    EXPECT_FALSE(int_phi.is_bool_typed);
    ASSERT_EQ(2, int_phi.incoming.size());
    EXPECT_EQ("0", int_phi.incoming[0].first);
    EXPECT_EQ("then", int_phi.incoming[0].second);
    EXPECT_EQ("%x", int_phi.incoming[1].first);
    EXPECT_EQ("else", int_phi.incoming[1].second);
    const auto& bool_phi = phis[1];
    EXPECT_TRUE(bool_phi.is_bool_typed);
    EXPECT_EQ("true", bool_phi.incoming[0].first);
    EXPECT_EQ("%y", bool_phi.incoming[1].first);
}

TEST(QirParser, ParsesSelectWithI64AndI1Operands) {
    if (!qret::qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR parser test";
    }
    const auto text = std::string(R"(
define i64 @main() #0 {
entry:
  %v0 = select i1 %c, i64 %a, i64 3
  %v1 = select i1 %d, i1 true, i1 %b
  ret i64 0
}
)");

    const auto module = ParseQIRText(text);
    const auto& block = module.functions.front().blocks.front();
    const auto select_ops = ItemsOfType<ParsedSelectInst>(block);
    ASSERT_EQ(2, select_ops.size());
    const auto& int_select = select_ops[0];
    EXPECT_EQ("%v0", int_select.result_var);
    EXPECT_EQ("%c", int_select.cond);
    EXPECT_FALSE(int_select.is_bool_typed);
    EXPECT_EQ("%a", int_select.true_value);
    EXPECT_EQ("3", int_select.false_value);
    const auto& bool_select = select_ops[1];
    EXPECT_EQ("%d", bool_select.cond);
    EXPECT_TRUE(bool_select.is_bool_typed);
    EXPECT_EQ("true", bool_select.true_value);
    EXPECT_EQ("%b", bool_select.false_value);
}

// LLVM comments can appear wherever whitespace can, e.g. `; preds = %block` after a label.
TEST(QirParser, AcceptsCommentsAfterBlockLabels) {
    if (!qret::qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR parser test";
    }
    const auto text = std::string(R"(
define i64 @main() #0 {
entry:                                            ; preds = %none
  br label %join
join:                                             ; preds = %entry
  ret i64 0
}
)");

    const auto module = ParseQIRText(text);
    ASSERT_EQ(2, module.functions.front().blocks.size());
    EXPECT_EQ("join", module.functions.front().blocks[1].name);
}

// bool_/int_record_output take an SSA i1/i64 arg, unlike result_record_output's ptr arg.
TEST(QirParser, AcceptsI1AndI64SsaValuedCallArguments) {
    if (!qret::qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR parser test";
    }
    const auto text = std::string(R"(
define i64 @main() #0 {
entry:
  %c0 = call i1 @__quantum__rt__read_result(ptr null)
  %v0 = add i64 1, 2
  call void @__quantum__rt__bool_record_output(i1 %c0, ptr null)
  call void @__quantum__rt__int_record_output(i64 %v0, ptr null)
  ret i64 0
}
)");

    const auto module = ParseQIRText(text);
    const auto& block = module.functions.front().blocks.front();
    const auto calls = ItemsOfType<ParsedCall>(block);
    ASSERT_EQ(3, calls.size());
    EXPECT_EQ("__quantum__rt__bool_record_output", calls[1].function);
    EXPECT_EQ((std::vector<std::string>{"i1 %c0", "ptr null"}), calls[1].args);
    EXPECT_EQ("__quantum__rt__int_record_output", calls[2].function);
    EXPECT_EQ((std::vector<std::string>{"i64 %v0", "ptr null"}), calls[2].args);
}

// LLVM prints a `double` as hex (`0x`+IEEE754 bits) when it doesn't round-trip through decimal.
TEST(QirParser, AcceptsHexFloatLiteral) {
    if (!qret::qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR parser test";
    }
    const auto text = std::string(R"(
define i64 @main() #0 {
entry:
  call void @__quantum__qis__ry__body(double 0x400921FB54442D18, ptr null)
  ret i64 0
}
)");

    const auto module = ParseQIRText(text);
    const auto& block = module.functions.front().blocks.front();
    const auto calls = ItemsOfType<ParsedCall>(block);
    ASSERT_EQ(1, calls.size());
    EXPECT_EQ((std::vector<std::string>{"double 0x400921FB54442D18", "ptr null"}), calls[0].args);
}

// QIR v1 spells pointers as `%Qubit*`/`%Result*`/`i8*` instead of v2's `ptr`; both must parse.
TEST(QirParser, AcceptsTypedPointerSyntax) {
    if (!qret::qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR parser test";
    }
    const auto text = std::string(R"(
%Result = type opaque
%Qubit = type opaque

define i64 @main() #0 {
entry:
  call void @__quantum__rt__initialize(i8* null)
  call void @__quantum__qis__x__body(%Qubit* inttoptr (i64 0 to %Qubit*))
  call void @__quantum__qis__m__body(%Qubit* inttoptr (i64 0 to %Qubit*), %Result* inttoptr (i64 0 to %Result*))
  %c0 = call i1 @__quantum__rt__read_result(%Result* inttoptr (i64 0 to %Result*))
  ret i64 0
}
)");

    const auto module = ParseQIRText(text);
    const auto& block = module.functions.front().blocks.front();
    const auto calls = ItemsOfType<ParsedCall>(block);
    ASSERT_EQ(4, calls.size());
    EXPECT_EQ("__quantum__qis__x__body", calls[1].function);
    EXPECT_EQ((std::vector<std::string>{"ptr inttoptr (i64 0 to ptr)"}), calls[1].args);
    EXPECT_EQ("__quantum__qis__m__body", calls[2].function);
    EXPECT_EQ(
            (std::vector<std::string>{"ptr inttoptr (i64 0 to ptr)", "ptr inttoptr (i64 0 to ptr)"}),
            calls[2].args
    );
}

// A label arg: a `getelementptr`-cast global string (typed) or a direct `ptr @N` (opaque).
TEST(QirParser, AcceptsGetelementptrWrappedRecordOutputLabel) {
    if (!qret::qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR parser test";
    }
    const auto text = std::string(R"(
@0 = internal constant [6 x i8] c"1_t0b\00"
@1 = internal constant [6 x i8] c"2_t1r\00"

define i64 @main() #0 {
entry:
  call void @__quantum__rt__bool_record_output(i1 true, i8* getelementptr inbounds ([6 x i8], [6 x i8]* @0, i64 0, i64 0))
  call void @__quantum__rt__result_record_output(i8* null, ptr @1)
  ret i64 0
}
)");

    const auto module = ParseQIRText(text);
    ASSERT_EQ(2, module.global_string_labels.size());
    EXPECT_EQ("1_t0b", module.global_string_labels.at("0"));
    EXPECT_EQ("2_t1r", module.global_string_labels.at("1"));

    const auto& block = module.functions.front().blocks.front();
    const auto calls = ItemsOfType<ParsedCall>(block);
    ASSERT_EQ(2, calls.size());
    EXPECT_EQ((std::vector<std::string>{"i1 true", "ptr @0"}), calls[0].args);
    EXPECT_EQ((std::vector<std::string>{"ptr null", "ptr @1"}), calls[1].args);
}

TEST(QirParser, ParsesConditionalBranch) {
    if (!qret::qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR parser test";
    }
    const auto text = std::string(R"(
define i64 @main() #0 {
entry:
  %c0 = call i1 @__quantum__qis__read_result__body(ptr null)
  br i1 %c0, label %then, label %else
then:
  br label %join
else:
  br label %join
join:
  ret i64 0
}
)");

    const auto module = ParseQIRText(text);
    const auto& entry = module.functions.front().blocks.at(0);
    EXPECT_EQ(ParsedBlock::Term::BrCond, entry.term);
    EXPECT_EQ("%c0", entry.cond_var);
    EXPECT_EQ("then", entry.target_true);
    EXPECT_EQ("else", entry.target_false);
}

TEST(QirParser, AcceptsReadonlyCallArgument) {
    if (!qret::qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR parser test";
    }
    const auto text = std::string(R"(
define i64 @main() #0 {
entry:
  %c0 = call i1 @__quantum__rt__read_result(ptr readonly null)
  ret i64 0
}
)");

    const auto module = ParseQIRText(text);
    const auto& block = module.functions.front().blocks.front();
    const auto calls = ItemsOfType<ParsedCall>(block);
    ASSERT_EQ(1, calls.size());
    EXPECT_EQ("__quantum__rt__read_result", calls[0].function);
    // `readonly` is recognized but discarded, matching the exporter's own read_result call sites.
    EXPECT_EQ((std::vector<std::string>{"ptr null"}), calls[0].args);
}

TEST(QirParser, AcceptsTailCallMarkers) {
    if (!qret::qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR parser test";
    }
    const auto text = std::string(R"(
define i64 @main() #0 {
entry:
  tail call void @__quantum__qis__h__body(ptr null)
  musttail call void @__quantum__qis__x__body(ptr null)
  notail call void @__quantum__qis__z__body(ptr null)
  %c0 = tail call i1 @__quantum__qis__read_result__body(ptr null)
  ret i64 0
}
)");

    const auto module = ParseQIRText(text);
    const auto& block = module.functions.front().blocks.front();
    const auto calls = ItemsOfType<ParsedCall>(block);
    ASSERT_EQ(4, calls.size());
    // The tail-call marker is recognized but discarded from both the function name and arguments.
    EXPECT_EQ("__quantum__qis__h__body", calls[0].function);
    EXPECT_EQ("__quantum__qis__x__body", calls[1].function);
    EXPECT_EQ("__quantum__qis__z__body", calls[2].function);
    EXPECT_EQ("%c0", calls[3].result_var);
    EXPECT_EQ("__quantum__qis__read_result__body", calls[3].function);
}

// Parameter names carry no meaning (unlike this exporter's own %q/%r convention): pyqir's
// low-level API names sub-circuit parameters %0, %1, ... and this must classify them all the same.
TEST(QirParser, ClassifiesParametersFromUsageNotName) {
    if (!qret::qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR parser test";
    }
    const auto text = std::string(R"(
define void @flip_and_measure(ptr %0, ptr %1, ptr %2) {
entry:
  call void @__quantum__qis__x__body(ptr %0)
  call void @__quantum__qis__mz__body(ptr %0, ptr %2)
  %c0 = call i1 @__quantum__rt__read_result(ptr %1)
  ret void
}
)");

    const auto module = ParseQIRText(text);
    ASSERT_EQ(1, module.functions.size());
    const auto& func = module.functions.front();
    EXPECT_EQ("flip_and_measure", func.name);
    ASSERT_EQ(1, func.qubit_params.size());
    EXPECT_EQ("%0", func.qubit_params[0]);
    ASSERT_EQ(2, func.register_params.size());
    EXPECT_EQ("%1", func.register_params[0].name);
    EXPECT_FALSE(func.register_params[0].is_output);
    EXPECT_EQ("%2", func.register_params[1].name);
    EXPECT_TRUE(func.register_params[1].is_output);
}

TEST(QirParser, UnrecognizedInstructionIsMalformed) {
    if (!qret::qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR parser test";
    }
    const auto text = std::string(R"(
define i64 @main() #0 {
entry:
  this is not an instruction
}
)");

    try {
        ParseQIRText(text);
        FAIL() << "expected QRETError";
    } catch (const QRETError& ex) {
        EXPECT_EQ(qret::error::MalformedQIR, ex.id());
    }
}

// `alloca`/`store`/`load`/`fadd` are out of scope (arrays/float PortableFunction can't represent).
// They're absent from the grammar, so they fail here rather than being silently skipped.
TEST(QirParser, AllocaIsMalformed) {
    if (!qret::qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR parser test";
    }
    const auto text = std::string(R"(
define i64 @main() #0 {
entry:
  %p = alloca i64
  ret i64 0
}
)");

    try {
        ParseQIRText(text);
        FAIL() << "expected QRETError";
    } catch (const QRETError& ex) {
        EXPECT_EQ(qret::error::MalformedQIR, ex.id());
    }
}

TEST(QirParser, FaddIsMalformed) {
    if (!qret::qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR parser test";
    }
    const auto text = std::string(R"(
define i64 @main() #0 {
entry:
  %v = fadd double 1.0, 2.0
  ret i64 0
}
)");

    try {
        ParseQIRText(text);
        FAIL() << "expected QRETError";
    } catch (const QRETError& ex) {
        EXPECT_EQ(qret::error::MalformedQIR, ex.id());
    }
}

// Bare LLVM enum attributes (`nounwind`, `align(8)`) are valid QIR this parser doesn't model; they
// are accepted and ignored, and the quoted attributes in the same group are still read.
TEST(QirParser, IgnoresUnquotedAttributesButReadsQuotedOnes) {
    if (!qret::qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR parser test";
    }
    const auto text = std::string(R"(
define i64 @main() #0 {
entry:
  ret i64 0
}
attributes #0 = { "entry_point" nounwind align(8) "required_num_qubits"="3" }
)");

    const auto module = ParseQIRText(text);
    EXPECT_EQ(1u, module.attr_group_entry_point.count(0));
    ASSERT_EQ(1u, module.attr_group_required_qubits.count(0));
    EXPECT_EQ(3u, module.attr_group_required_qubits.at(0));
}

// Any parameter name is syntactically valid now; a never-used parameter still fails, but because
// nothing in the body says whether it's a Qubit or a Register, not because of its name.
TEST(QirParser, UnusedParameterCannotBeClassified) {
    if (!qret::qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR parser test";
    }
    const auto text = std::string(R"(
define void @sub(ptr %x0) {
entry:
  ret void
}
)");
    EXPECT_THROW(ParseQIRText(text), QRETError);
}

// writeonly, readonly, and pyqir's quoted `add_string_attribute` form are all consumed and
// dropped; classification only ever looks at how a parameter is actually used.
TEST(QirParser, ToleratesArbitraryParameterAttributes) {
    if (!qret::qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR parser test";
    }
    const auto text = std::string(R"(
define void @flip(ptr writeonly nocapture %0, ptr "another_attr" %1) {
entry:
  call void @__quantum__qis__x__body(ptr %0)
  call void @__quantum__qis__mz__body(ptr %0, ptr %1)
  ret void
}
)");

    const auto module = ParseQIRText(text);
    const auto& func = module.functions.front();
    ASSERT_EQ(1, func.qubit_params.size());
    EXPECT_EQ("%0", func.qubit_params[0]);
    ASSERT_EQ(1, func.register_params.size());
    EXPECT_TRUE(func.register_params[0].is_output);
}

// A quoted call-argument attribute (pyqir's `add_string_attribute` form), unlike the bare
// `readonly`/`writeonly` keywords, has never been exercised here; it's dropped the same way.
TEST(QirParser, AcceptsQuotedCallArgumentAttribute) {
    if (!qret::qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR parser test";
    }
    const auto text = std::string(R"(
define i64 @main() #0 {
entry:
  %c0 = call i1 @__quantum__rt__read_result(ptr "writeonly" null)
  ret i64 0
}
)");

    const auto module = ParseQIRText(text);
    const auto& block = module.functions.front().blocks.front();
    const auto calls = ItemsOfType<ParsedCall>(block);
    ASSERT_EQ(1, calls.size());
    EXPECT_EQ((std::vector<std::string>{"ptr null"}), calls[0].args);
}
