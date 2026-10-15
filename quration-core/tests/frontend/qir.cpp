#include "qret/frontend/qir.h"

#include <gtest/gtest.h>

#include <array>
#include <iterator>
#include <numbers>
#include <string>

#include "qret/analysis/counter.h"
#include "qret/base/cast.h"
#include "qret/error.h"
#include "qret/exception.h"
#include "qret/exporter/qir.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/control_flow.h"
#include "qret/frontend/intrinsic.h"
#include "qret/ir/basic_block.h"
#include "qret/ir/context.h"
#include "qret/ir/function.h"
#include "qret/ir/instructions.h"
#include "qret/ir/module.h"
#include "qret/parser/qir.h"
#include "qret/runtime/simulator.h"

using namespace qret;
using namespace qret::ir;

namespace {
struct XThenCxGen : frontend::CircuitGenerator {
    explicit XThenCxGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return "main";
    }
    std::string GetCacheKey() const override {
        return GetName();
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 2, Attribute::Operate);
        arg.Add("r", Type::Register, 2, Attribute::Output);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        const auto qs = GetQubits(0);
        const auto rs = GetRegisters(1);
        frontend::gate::X(qs[0]);
        frontend::gate::CX(qs[1], qs[0]);  // target=q1, control=q0
        frontend::gate::Measure(qs[0], rs[0]);
        frontend::gate::Measure(qs[1], rs[1]);
        return EndCircuitDefinition();
    }
};

// Measures q0 (forced to `cond`); flips q1 if set. No else: false falls through to continuation.
struct MeasureThenMaybeFlipGen : frontend::CircuitGenerator {
    explicit MeasureThenMaybeFlipGen(frontend::CircuitBuilder* builder, bool cond)
        : CircuitGenerator(builder)
        , cond_{cond} {}
    std::string GetName() const override {
        return "main";
    }
    std::string GetCacheKey() const override {
        return GetName();
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 2, Attribute::Operate);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        const auto qs = GetQubits(0);
        if (cond_) {
            frontend::gate::X(qs[0]);
        }
        const auto r0 = GetTemporalRegister();
        frontend::gate::Measure(qs[0], r0);
        frontend::control_flow::If(r0);
        frontend::gate::X(qs[1]);
        frontend::control_flow::EndIf(r0);
        const auto r1 = GetTemporalRegister();
        frontend::gate::Measure(qs[1], r1);
        return EndCircuitDefinition();
    }

private:
    bool cond_;
};

// Builds: measure q0 into r0; If(r0){X(q1)} Else(r0){X(q2)} EndIf(r0); measure q1,q2. Uses the
// real control_flow::If/Else so the exported QIR has the BB order Else() actually produces.
struct IfElseCircuitGen : frontend::CircuitGenerator {
    explicit IfElseCircuitGen(frontend::CircuitBuilder* builder, bool cond)
        : CircuitGenerator(builder)
        , cond_{cond} {}
    std::string GetName() const override {
        return "main";
    }
    std::string GetCacheKey() const override {
        return GetName();
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 3, Attribute::Operate);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        const auto qs = GetQubits(0);
        const auto q0 = qs[0];
        const auto q1 = qs[1];
        const auto q2 = qs[2];

        if (cond_) {
            frontend::gate::X(q0);
        }
        const auto r0 = GetTemporalRegister();
        frontend::gate::Measure(q0, r0);
        frontend::control_flow::If(r0);
        frontend::gate::X(q1);
        frontend::control_flow::Else(r0);
        frontend::gate::X(q2);
        frontend::control_flow::EndIf(r0);

        const auto r1 = GetTemporalRegister();
        const auto r2 = GetTemporalRegister();
        frontend::gate::Measure(q1, r1);
        frontend::gate::Measure(q2, r2);

        return EndCircuitDefinition();
    }

private:
    bool cond_;
};
// A sub-circuit (q, r_in: Input, r_out: Output): flips q if r_in is set, then measures q into
// r_out. Exercises Input (forwarded condition) and Output (aliased back) register arguments.
struct SubCircuitWithRegArgsGen : frontend::CircuitGenerator {
    explicit SubCircuitWithRegArgsGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return "flip_and_measure";
    }
    std::string GetCacheKey() const override {
        return GetName();
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 1, Attribute::Operate);
        arg.Add("r_in", Type::Register, 1, Attribute::Input);
        arg.Add("r_out", Type::Register, 1, Attribute::Output);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        const auto qs = GetQubits(0);
        const auto r_in = GetRegister(1);
        const auto r_out = GetRegister(2);
        frontend::control_flow::If(r_in);
        frontend::gate::X(qs[0]);
        frontend::control_flow::EndIf(r_in);
        frontend::gate::Measure(qs[0], r_out);
        return EndCircuitDefinition();
    }
};
struct MainCallsSubWithRegArgsGen : frontend::CircuitGenerator {
    explicit MainCallsSubWithRegArgsGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return "main";
    }
    std::string GetCacheKey() const override {
        return GetName();
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 2, Attribute::Operate);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        const auto qs = GetQubits(0);
        frontend::gate::X(qs[0]);  // force the condition register to 1
        const auto r_cond = GetTemporalRegister();
        frontend::gate::Measure(qs[0], r_cond);

        auto* sub_circuit = SubCircuitWithRegArgsGen(GetBuilder()).Generate();
        const auto r_result = GetTemporalRegister();
        (*sub_circuit)(qs[1], r_cond, r_result);

        return EndCircuitDefinition();
    }
};

struct IfCase {
    std::string name;
    bool cond;
    bool expected_q1;
};

// Builds MeasureThenMaybeFlipGen(cond), round-trips it, runs it, and returns [q0, q1] registers.
std::array<bool, 2> RunIfNoElseRoundTrip(bool cond) {
    auto context1 = IRContext();
    auto* module1 = Module::Create("original", context1);
    auto builder1 = frontend::CircuitBuilder(module1);
    MeasureThenMaybeFlipGen(&builder1, cond).Generate();

    const auto qir_text = qir::ExportModuleToQIR(*module1);

    auto context2 = IRContext();
    auto* module2 = Module::Create("roundtrip", context2);
    auto builder2 = frontend::CircuitBuilder(module2);
    auto* circuit2 = frontend::BuildCircuitFromQIR(qir_text, builder2);

    auto config = runtime::SimulatorConfig{runtime::SimulatorConfig::StateType::Toffoli};
    auto simulator = runtime::Simulator(config, circuit2->GetIR());
    simulator.RunAll();
    const auto& state = simulator.GetToffoliState();
    return {state.ReadRegister(0), state.ReadRegister(1)};
}

class IfNoElseRoundTrip : public ::testing::TestWithParam<IfCase> {};

struct IfElseCase {
    std::string name;
    bool cond;
    bool expected_then_qubit;  // q1
    bool expected_else_qubit;  // q2
};

// Builds IfElseCircuitGen(cond), round-trips it, and returns [q0, q1(then), q2(else)] registers.
std::array<bool, 3> RunIfElseRoundTrip(bool cond) {
    auto context1 = IRContext();
    auto* module1 = Module::Create("original", context1);
    auto builder1 = frontend::CircuitBuilder(module1);
    IfElseCircuitGen(&builder1, cond).Generate();

    const auto qir_text = qir::ExportModuleToQIR(*module1);

    auto context2 = IRContext();
    auto* module2 = Module::Create("roundtrip", context2);
    auto builder2 = frontend::CircuitBuilder(module2);
    auto* circuit2 = frontend::BuildCircuitFromQIR(qir_text, builder2);

    auto config = runtime::SimulatorConfig{runtime::SimulatorConfig::StateType::Toffoli};
    auto simulator = runtime::Simulator(config, circuit2->GetIR());
    simulator.RunAll();
    const auto& state = simulator.GetToffoliState();
    return {state.ReadRegister(0), state.ReadRegister(1), state.ReadRegister(2)};
}

class IfElseRoundTrip : public ::testing::TestWithParam<IfElseCase> {};

// Measures q0,q1 as a 2-bit switch value; 3 cases (X q2/q3/q4) + empty default; measures q2-q4.
struct SwitchCircuitGen : frontend::CircuitGenerator {
    explicit SwitchCircuitGen(frontend::CircuitBuilder* builder, bool bit0, bool bit1)
        : CircuitGenerator(builder)
        , bit0_{bit0}
        , bit1_{bit1} {}
    std::string GetName() const override {
        return "main";
    }
    std::string GetCacheKey() const override {
        return GetName();
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 5, Attribute::Operate);
        arg.Add("bits", Type::Register, 2, Attribute::Output);
        arg.Add("out", Type::Register, 3, Attribute::Output);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        const auto qs = GetQubits(0);
        if (bit0_) {
            frontend::gate::X(qs[0]);
        }
        if (bit1_) {
            frontend::gate::X(qs[1]);
        }
        const auto rs = GetRegisters(1);
        frontend::gate::Measure(qs[0], rs[0]);
        frontend::gate::Measure(qs[1], rs[1]);

        frontend::control_flow::Switch(rs);
        frontend::control_flow::Case(rs, 0);
        frontend::gate::X(qs[2]);
        frontend::control_flow::Case(rs, 1);
        frontend::gate::X(qs[3]);
        frontend::control_flow::Case(rs, 2);
        frontend::gate::X(qs[4]);
        frontend::control_flow::Default(rs);
        frontend::control_flow::EndSwitch(rs);

        const auto out = GetRegisters(2);
        frontend::gate::Measure(qs[2], out[0]);
        frontend::gate::Measure(qs[3], out[1]);
        frontend::gate::Measure(qs[4], out[2]);
        return EndCircuitDefinition();
    }

private:
    bool bit0_;
    bool bit1_;
};

struct SwitchCase {
    std::string name;
    bool bit0;
    bool bit1;
    std::array<bool, 5> expected;  // [q0, q1, case0-qubit, case1-qubit, case2-qubit]
};

// Builds SwitchCircuitGen(bit0,bit1), round-trips it, and returns [q0,q1,case0,case1,case2].
std::array<bool, 5> RunSwitchRoundTrip(bool bit0, bool bit1) {
    auto context1 = IRContext();
    auto* module1 = Module::Create("original", context1);
    auto builder1 = frontend::CircuitBuilder(module1);
    SwitchCircuitGen(&builder1, bit0, bit1).Generate();

    const auto qir_text = qir::ExportModuleToQIR(*module1);

    auto context2 = IRContext();
    auto* module2 = Module::Create("roundtrip", context2);
    auto builder2 = frontend::CircuitBuilder(module2);
    auto* circuit2 = frontend::BuildCircuitFromQIR(qir_text, builder2);

    auto config = runtime::SimulatorConfig{runtime::SimulatorConfig::StateType::Toffoli};
    auto simulator = runtime::Simulator(config, circuit2->GetIR());
    simulator.RunAll();
    const auto& state = simulator.GetToffoliState();
    return {
            state.ReadRegister(0),
            state.ReadRegister(1),
            state.ReadRegister(2),
            state.ReadRegister(3),
            state.ReadRegister(4),
    };
}

class SwitchRoundTrip : public ::testing::TestWithParam<SwitchCase> {};

// A switch whose case 1 body contains a full if/else, converging before the switch join. `switch`
// QIR is only produced by Quration's own exporter (neither pyqir nor QDK can emit it), so this
// nested compound control flow is exercised by a round-trip (build -> export -> import -> simulate)
// rather than a hand-written fixture.
struct NestedIfInSwitchGen : frontend::CircuitGenerator {
    explicit NestedIfInSwitchGen(frontend::CircuitBuilder* builder, bool select_case1, bool inner)
        : CircuitGenerator(builder)
        , select_case1_{select_case1}
        , inner_{inner} {}
    std::string GetName() const override {
        return "main";
    }
    std::string GetCacheKey() const override {
        return GetName();
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 5, Attribute::Operate);
        arg.Add("sel", Type::Register, 2, Attribute::Output);
        arg.Add("inner", Type::Register, 1, Attribute::Output);
        arg.Add("out", Type::Register, 2, Attribute::Output);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        const auto qs = GetQubits(0);
        const auto sel = GetRegisters(1);
        const auto inner = GetRegisters(2);
        const auto out = GetRegisters(3);

        if (select_case1_) {
            frontend::gate::X(qs[0]);  // selector value 1 (q0 is the low bit)
        }
        frontend::gate::Measure(qs[0], sel[0]);
        frontend::gate::Measure(qs[1], sel[1]);

        frontend::control_flow::Switch(sel);
        frontend::control_flow::Case(sel, 1);
        if (inner_) {
            frontend::gate::X(qs[2]);
        }
        frontend::gate::Measure(qs[2], inner[0]);
        frontend::control_flow::If(inner[0]);
        frontend::gate::X(qs[3]);
        frontend::control_flow::Else(inner[0]);
        frontend::gate::X(qs[4]);
        frontend::control_flow::EndIf(inner[0]);
        frontend::control_flow::Default(sel);
        frontend::control_flow::EndSwitch(sel);

        frontend::gate::Measure(qs[3], out[0]);
        frontend::gate::Measure(qs[4], out[1]);
        return EndCircuitDefinition();
    }

private:
    bool select_case1_;
    bool inner_;
};

struct NestedIfInSwitchCase {
    std::string name;
    bool select_case1;
    bool inner;
    std::array<bool, 5> expected;  // [sel0, sel1, inner, then_target(q3), else_target(q4)]
};

// Builds NestedIfInSwitchGen, round-trips it, and returns [sel0, sel1, inner, q3, q4].
std::array<bool, 5> RunNestedIfInSwitchRoundTrip(bool select_case1, bool inner) {
    auto context1 = IRContext();
    auto* module1 = Module::Create("original", context1);
    auto builder1 = frontend::CircuitBuilder(module1);
    NestedIfInSwitchGen(&builder1, select_case1, inner).Generate();

    const auto qir_text = qir::ExportModuleToQIR(*module1);

    auto context2 = IRContext();
    auto* module2 = Module::Create("roundtrip", context2);
    auto builder2 = frontend::CircuitBuilder(module2);
    auto* circuit2 = frontend::BuildCircuitFromQIR(qir_text, builder2);

    auto config = runtime::SimulatorConfig{runtime::SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 8;
    auto simulator = runtime::Simulator(config, circuit2->GetIR());
    simulator.RunAll();
    const auto& state = simulator.GetToffoliState();
    return {
            state.ReadRegister(0),
            state.ReadRegister(1),
            state.ReadRegister(2),
            state.ReadRegister(3),
            state.ReadRegister(4),
    };
}

class NestedIfInSwitchRoundTrip : public ::testing::TestWithParam<NestedIfInSwitchCase> {};
}  // namespace

TEST(QirFrontend, ImportBellPairText) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    auto* circuit =
            frontend::BuildCircuitFromQIRFile("quration-core/tests/data/qir/bell_pair.ll", builder);

    const auto* func = circuit->GetIR();
    EXPECT_EQ(2, func->GetNumQubits());
    ASSERT_EQ(1, std::distance(func->begin(), func->end()));  // single basic block

    // Walk imported instructions matching bell_pair.ll: initialize/tuple_record_output/
    // result_record_output calls are runtime bookkeeping the importer skips (EmitCall), so absent.
    const auto& bb = *func->begin();
    auto it = bb.begin();

    ASSERT_NE(bb.end(), it);
    EXPECT_EQ(Opcode::Table::H, it->GetOpcode().GetCode());
    EXPECT_EQ(0, Cast<UnaryInst>(&*it)->GetQubit().id);
    ++it;

    ASSERT_NE(bb.end(), it);
    EXPECT_EQ(Opcode::Table::CX, it->GetOpcode().GetCode());
    const auto* cx = Cast<BinaryInst>(&*it);
    EXPECT_EQ(1, cx->GetQubit0().id);  // target
    EXPECT_EQ(0, cx->GetQubit1().id);  // control
    ++it;

    ASSERT_NE(bb.end(), it);
    EXPECT_TRUE(it->IsMeasurement());
    const auto* m0 = Cast<MeasurementInst>(&*it);
    EXPECT_EQ(0, m0->GetQubit().id);
    EXPECT_EQ(0, m0->GetRegister().id);
    ++it;

    ASSERT_NE(bb.end(), it);
    EXPECT_TRUE(it->IsMeasurement());
    const auto* m1 = Cast<MeasurementInst>(&*it);
    EXPECT_EQ(1, m1->GetQubit().id);
    EXPECT_EQ(1, m1->GetRegister().id);
    ++it;

    ASSERT_NE(bb.end(), it);
    EXPECT_EQ(Opcode::Table::Return, it->GetOpcode().GetCode());
    ++it;
    EXPECT_EQ(bb.end(), it);
}

TEST(QirFrontend, UnknownFunctionCallIsUnsupported) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);

    EXPECT_THROW(
            frontend::BuildCircuitFromQIRFile(
                    "quration-core/tests/data/qir/unknown_function_call.ll", builder
            ),
            QRETError
    );
}

// A qis-namespace gate outside the intrinsic catalog is rejected, not silently ignored.
TEST(QirFrontend, UnsupportedQisGateIsRejected) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);

    EXPECT_THROW(
            frontend::BuildCircuitFromQIRFile(
                    "quration-core/tests/data/qir/unsupported_qis_gate.ll", builder
            ),
            QRETError
    );
}

// A valid entry point with no gates imports to a circuit whose only instruction is the Return.
TEST(QirFrontend, ImportsEmptyCircuit) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    auto* circuit =
            frontend::BuildCircuitFromQIRFile("quration-core/tests/data/qir/empty_circuit.ll", builder);

    const auto& bb = *circuit->GetIR()->begin();
    auto it = bb.begin();
    ASSERT_NE(bb.end(), it);
    EXPECT_EQ(Opcode::Table::Return, it->GetOpcode().GetCode());
    EXPECT_EQ(bb.end(), ++it);
}

// sub_a calls sub_b calls sub_a; caught by the call-graph check before Generate() would otherwise
// recurse into it and overflow the C++ call stack.
TEST(QirFrontend, CyclicSubCircuitCallIsUnsupported) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);

    EXPECT_THROW(
            frontend::BuildCircuitFromQIRFile(
                    "quration-core/tests/data/qir/cyclic_sub_circuits.ll", builder
            ),
            QRETError
    );
}

TEST(QirFrontend, RoundTripCXPreservesArgumentOrder) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    // X(q0); CX(target=q1, control=q0); measure both. Deterministic: q0=1, q1=1.
    auto context1 = IRContext();
    auto* module1 = Module::Create("original", context1);
    auto builder1 = frontend::CircuitBuilder(module1);
    XThenCxGen(&builder1).Generate();

    const auto qir_text = qir::ExportModuleToQIR(*module1);

    auto context2 = IRContext();
    auto* module2 = Module::Create("roundtrip", context2);
    auto builder2 = frontend::CircuitBuilder(module2);
    auto* circuit2 = frontend::BuildCircuitFromQIR(qir_text, builder2);

    auto config = runtime::SimulatorConfig{runtime::SimulatorConfig::StateType::Toffoli};
    auto simulator = runtime::Simulator(config, circuit2->GetIR());
    simulator.RunAll();
    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(true, state.ReadRegister(0));
    EXPECT_EQ(true, state.ReadRegister(1));
}

// cond=false: q0 measures 0, so the then arm (X q1) doesn't fire; cond=true: it fires, flipping q1.
TEST_P(IfNoElseRoundTrip, TakesCorrectBranch) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    const auto& c = GetParam();
    const auto regs = RunIfNoElseRoundTrip(c.cond);
    EXPECT_EQ(c.cond, regs[0]);
    EXPECT_EQ(c.expected_q1, regs[1]);
}

INSTANTIATE_TEST_SUITE_P(
        QirFrontend,
        IfNoElseRoundTrip,
        ::testing::Values(
                IfCase{"ConditionFalse", false, false}, IfCase{"ConditionTrue", true, true}
        ),
        [](const ::testing::TestParamInfo<IfCase>& info) { return info.param.name; }
);

// cond=false: the else arm runs (X(q2) fires); cond=true: the then arm runs instead (X(q1) fires).
TEST_P(IfElseRoundTrip, TakesCorrectBranch) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    const auto& c = GetParam();
    const auto regs = RunIfElseRoundTrip(c.cond);
    EXPECT_EQ(c.cond, regs[0]);
    EXPECT_EQ(c.expected_then_qubit, regs[1]);
    EXPECT_EQ(c.expected_else_qubit, regs[2]);
}

INSTANTIATE_TEST_SUITE_P(
        QirFrontend,
        IfElseRoundTrip,
        ::testing::Values(
                IfElseCase{"ConditionFalseTakesElse", false, false, true},
                IfElseCase{"ConditionTrueTakesThen", true, true, false}
        ),
        [](const ::testing::TestParamInfo<IfElseCase>& info) { return info.param.name; }
);

// Confirms switch dispatches to the correct case, or falls through to Default, across all 4 arms.
TEST_P(SwitchRoundTrip, TakesCorrectBranch) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    const auto& c = GetParam();
    EXPECT_EQ(c.expected, RunSwitchRoundTrip(c.bit0, c.bit1));
}

INSTANTIATE_TEST_SUITE_P(
        QirFrontend,
        SwitchRoundTrip,
        ::testing::Values(
                SwitchCase{"CaseZero", false, false, {false, false, true, false, false}},
                SwitchCase{"CaseOne", true, false, {true, false, false, true, false}},
                SwitchCase{"CaseTwo", false, true, {false, true, false, false, true}},
                SwitchCase{"Default", true, true, {true, true, false, false, false}}
        ),
        [](const ::testing::TestParamInfo<SwitchCase>& info) { return info.param.name; }
);

TEST(QirFrontend, BackwardBranchIsUnsupported) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);

    EXPECT_THROW(
            frontend::BuildCircuitFromQIRFile(
                    "quration-core/tests/data/qir/backward_branch.ll", builder
            ),
            QRETError
    );
}

TEST(QirFrontend, RoundTripSubCircuitRegisterArgumentsForwardValues) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    // q0 forced to |1>, so the sub-circuit's Input condition is true: it flips q1 and measures into
    // its Output argument, aliasing main's own register (call args bind by reference).
    auto context1 = IRContext();
    auto* module1 = Module::Create("original", context1);
    auto builder1 = frontend::CircuitBuilder(module1);
    MainCallsSubWithRegArgsGen(&builder1).Generate();

    const auto qir_text = qir::ExportModuleToQIR(*module1);

    auto context2 = IRContext();
    auto* module2 = Module::Create("roundtrip", context2);
    auto builder2 = frontend::CircuitBuilder(module2);
    auto* circuit2 = frontend::BuildCircuitFromQIR(qir_text, builder2);

    auto config = runtime::SimulatorConfig{runtime::SimulatorConfig::StateType::Toffoli};
    auto simulator = runtime::Simulator(config, circuit2->GetIR());
    simulator.RunAll();
    const auto& state = simulator.GetToffoliState();
    // Registers renumber in measurement order: 0=condition (q0), 1=sub-circuit's Output result.
    EXPECT_EQ(true, state.ReadRegister(0));
    EXPECT_EQ(true, state.ReadRegister(1));
}

// X(q0) then swap(q0, q1): the initial |1> ends up on q1, not q0.
TEST(QirFrontend, SwapCallsPresetSwapCircuit) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    auto* circuit = frontend::BuildCircuitFromQIRFile(
            "quration-core/tests/data/qir/swap_preset.ll", builder
    );

    auto it = circuit->GetIR()->begin()->begin();
    ++it;  // the X on q0
    ASSERT_TRUE(it->IsCall());
    EXPECT_EQ(qir::kSwap, Cast<CallInst>(&*it)->GetCallee()->GetName());

    auto config = runtime::SimulatorConfig{runtime::SimulatorConfig::StateType::Toffoli};
    auto simulator = runtime::Simulator(config, circuit->GetIR());
    simulator.RunAll();
    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(false, state.ReadRegister(0));
    EXPECT_EQ(true, state.ReadRegister(1));
}

// X(q0) forces its measurement true; the branch reads it via pyqir's own
// __quantum__qis__read_result__body rather than the exporter's __quantum__rt__read_result naming.
TEST(QirFrontend, ImportsPyqirStyleReadResultName) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    auto* circuit = frontend::BuildCircuitFromQIRFile(
            "quration-core/tests/data/qir/pyqir_style_read_result.ll", builder
    );

    auto config = runtime::SimulatorConfig{runtime::SimulatorConfig::StateType::Toffoli};
    auto simulator = runtime::Simulator(config, circuit->GetIR());
    simulator.RunAll();
    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(true, state.ReadRegister(0));
    EXPECT_EQ(true, state.ReadRegister(1));
}

// A sub-circuit generated by pyqir's low-level API: its Qubit/Register parameters are named
// %0/%1 (not this exporter's own %q/%r convention), and its Result parameter carries no
// `writeonly`. Confirms classification comes from how a parameter is used, not from its name.
TEST(QirFrontend, ImportsPyqirStyleMultiFunctionSubCircuit) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    auto* circuit = frontend::BuildCircuitFromQIRFile(
            "quration-core/tests/data/qir/pyqir_multi_function.ll", builder
    );

    auto config = runtime::SimulatorConfig{runtime::SimulatorConfig::StateType::Toffoli};
    auto simulator = runtime::Simulator(config, circuit->GetIR());
    simulator.RunAll();
    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(true, state.ReadRegister(0));
}

// qdk's full Adaptive profile wraps every intrinsic gate call in its own thin sub-circuit (here
// @X(ptr %var_1)), unlike Base/Adaptive_RI/Adaptive_RIF which always inline them.
TEST(QirFrontend, ImportsQdkAdaptiveGateWrapperFunction) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    auto* circuit = frontend::BuildCircuitFromQIRFile(
            "quration-core/tests/data/qir/qdk_adaptive_gate_wrapper.ll", builder
    );

    auto config = runtime::SimulatorConfig{runtime::SimulatorConfig::StateType::Toffoli};
    auto simulator = runtime::Simulator(config, circuit->GetIR());
    simulator.RunAll();
    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(true, state.ReadRegister(0));
}

// Real qsc output names gates __quantum__qis__cx__body/__m__body instead of Quration's own
// cnot__body/mz__body. Confirms both naming schemes are accepted.
TEST(QirFrontend, ImportsQscStyleCxAndMGateNames) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    auto* circuit = frontend::BuildCircuitFromQIRFile(
            "quration-core/tests/data/qir/qsc_alias_gate_names.ll", builder
    );

    auto config = runtime::SimulatorConfig{runtime::SimulatorConfig::StateType::Toffoli};
    auto simulator = runtime::Simulator(config, circuit->GetIR());
    simulator.RunAll();
    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(true, state.ReadRegister(0));
    EXPECT_EQ(true, state.ReadRegister(1));
}

// qsc's __quantum__qis__mresetz__body fuses a measurement with an immediate reset. Confirms both
// the pre-reset outcome is recorded and the qubit is actually reset (later measures read false).
TEST(QirFrontend, ImportsQscStyleMResetZGate) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    auto* circuit = frontend::BuildCircuitFromQIRFile(
            "quration-core/tests/data/qir/qsc_mresetz.ll", builder
    );

    auto config = runtime::SimulatorConfig{runtime::SimulatorConfig::StateType::Toffoli};
    auto simulator = runtime::Simulator(config, circuit->GetIR());
    simulator.RunAll();
    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(true, state.ReadRegister(0));
    EXPECT_EQ(false, state.ReadRegister(1));
    EXPECT_EQ(true, state.ReadRegister(2));

    // mresetz measures once, not twice (its Result doubles as the reset condition): 1 + 2 mz = 3.
    auto num_measurements = std::size_t{0};
    for (const auto& func : *module) {
        for (const auto& [key, count] : CountInst(func).counter) {
            if (key.first == ir::Opcode::Table::Measurement) {
                num_measurements += count;
            }
        }
    }
    EXPECT_EQ(3, num_measurements);
}

// A bool/result_record_output call carries its own output label; when directly measurement-derived
// (r0 == One reuses r0's read_result), the importer measures straight into a named Output register.
TEST(QirFrontend, ImportsQscBoolRecordOutputsIntoNamedRegisters) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    auto* circuit = frontend::BuildCircuitFromQIRFile(
            "quration-core/tests/data/qir/qsc_bool_record_output.ll", builder
    );

    const auto& registers = circuit->GetIR()->GetArgument().registers;
    ASSERT_EQ(2, registers.size());
    EXPECT_EQ("1_t0b", registers[0].first);
    EXPECT_EQ(1, registers[0].second);
    EXPECT_EQ("2_t1b", registers[1].first);
    EXPECT_EQ(1, registers[1].second);

    auto config = runtime::SimulatorConfig{runtime::SimulatorConfig::StateType::Toffoli};
    auto simulator = runtime::Simulator(config, circuit->GetIR());
    simulator.RunAll();
    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(true, state.ReadRegister(0));
    EXPECT_EQ(false, state.ReadRegister(1));
}

// Re-exporting an imported circuit recovers its original output labels/order via named Output regs.
TEST(QirFrontend, RoundTripBoolRecordOutputsPreservesLabelsAndOrder) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    frontend::BuildCircuitFromQIRFile("quration-core/tests/data/qir/qsc_bool_record_output.ll", builder);

    const auto qir_text = qir::ExportModuleToQIR(*module);
    const auto label0 = qir_text.find(R"(c"1_t0b\00")");
    const auto label1 = qir_text.find(R"(c"2_t1b\00")");
    ASSERT_NE(std::string::npos, label0);
    ASSERT_NE(std::string::npos, label1);
    EXPECT_LT(label0, label1);  // in the original qsc program order, not e.g. reversed

    const auto call0 = qir_text.find("call void @__quantum__rt__result_record_output(ptr null, ptr @1)");
    const auto call1 = qir_text.find(
            "call void @__quantum__rt__result_record_output(ptr inttoptr (i64 1 to ptr), ptr @2)"
    );
    EXPECT_NE(std::string::npos, call0);
    EXPECT_NE(std::string::npos, call1);
    EXPECT_LT(call0, call1);
}

// A measurement drives both the br i1 condition and a later read_result feeding bool_record_output;
// confirms both resolve to the same register (TraceRecordOutputToMeasurement) even across if/else.
TEST(QirFrontend, ImportsQscIfNoElseWithBoolRecordOutput) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    auto* circuit = frontend::BuildCircuitFromQIRFile(
            "quration-core/tests/data/qir/qsc_if_no_else_bool_record_output.ll", builder
    );

    const auto& registers = circuit->GetIR()->GetArgument().registers;
    ASSERT_EQ(2, registers.size());
    EXPECT_EQ("1_t0b", registers[0].first);
    EXPECT_EQ("2_t1b", registers[1].first);
    EXPECT_EQ(0, circuit->GetIR()->GetNumTmpRegisters());

    auto config = runtime::SimulatorConfig{runtime::SimulatorConfig::StateType::Toffoli};
    auto simulator = runtime::Simulator(config, circuit->GetIR());
    simulator.RunAll();
    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(true, state.ReadRegister(0));
    EXPECT_EQ(true, state.ReadRegister(1));
}

// A 3-way if/elif/else desugars into nested if/else; exercises a join-in-Return reached from more
// than one arm (then_0 and the inner if/else's own join converge on the same Return block).
TEST(QirFrontend, ImportsQscMultiwayIfElifElse) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    auto* circuit = frontend::BuildCircuitFromQIRFile(
            "quration-core/tests/data/qir/qsc_multiway_if_elif_else.ll", builder
    );

    const auto& registers = circuit->GetIR()->GetArgument().registers;
    ASSERT_EQ(3, registers.size());
    EXPECT_EQ("1_t0b", registers[0].first);
    EXPECT_EQ("2_t1b", registers[1].first);
    EXPECT_EQ("3_t2b", registers[2].first);
    EXPECT_EQ(0, circuit->GetIR()->GetNumTmpRegisters());

    auto config = runtime::SimulatorConfig{runtime::SimulatorConfig::StateType::Toffoli};
    auto simulator = runtime::Simulator(config, circuit->GetIR());
    simulator.RunAll();
    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(true, state.ReadRegister(0));
    EXPECT_EQ(false, state.ReadRegister(1));
    EXPECT_EQ(true, state.ReadRegister(2));
}

// int_record_output's value must trace back to a measurement through a recognized classical chain;
// a plain literal has no such chain, so it's rejected.
TEST(QirFrontend, LiteralIntRecordOutputIsUnsupported) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    EXPECT_THROW(
            frontend::BuildCircuitFromQIRFile(
                    "quration-core/tests/data/qir/qsc_int_record_output.ll", builder
            ),
            QRETError
    );
}

// A phi computing int_record_output's value is followed by an unrelated measurement, then a second
// computation; exercises the classical buffer's boundary rule around a rebindable result_slot_.
TEST(QirFrontend, ImportsIntComputationInterleavedWithQuantumInstruction) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    auto* circuit = frontend::BuildCircuitFromQIRFile(
            "quration-core/tests/data/qir/qsc_int_computation_interleaved_with_gate.ll", builder
    );

    const auto& registers = circuit->GetIR()->GetArgument().registers;
    ASSERT_EQ(2, registers.size());
    EXPECT_EQ("1_t0i", registers[0].first);
    EXPECT_EQ(64, registers[0].second);
    EXPECT_EQ("2_t1i", registers[1].first);
    EXPECT_EQ(64, registers[1].second);

    auto config = runtime::SimulatorConfig{runtime::SimulatorConfig::StateType::Toffoli};
    auto simulator = runtime::Simulator(config, circuit->GetIR());
    simulator.RunAll();
    const auto& state = simulator.GetToffoliState();
    auto read_int = [&state](std::uint64_t base) {
        auto value = std::uint64_t{0};
        for (auto i = std::uint64_t{0}; i < 64; ++i) {
            if (state.ReadRegister(base + i)) {
                value |= std::uint64_t{1} << i;
            }
        }
        return value;
    };
    // Both q0 and q1 are forced to One, so both phis take their True-arm literal (3, then 7).
    EXPECT_EQ(3, read_int(0));
    EXPECT_EQ(7, read_int(64));
}

// Two independent int-op subtrees share one measured input and are interleaved in QIR order. The
// importer must synthesize the classical function in that source order: a demand-driven
// (output-rooted) build would group each subtree, building mul before shl and sub before or. Also
// checks every op in the chain is present. (Values are covered by the QDK equivalence tests.)
TEST(QirFrontend, ImportsClassicalIntChainInSourceOrder) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    auto* circuit = frontend::BuildCircuitFromQIRFile(
            "quration-core/tests/data/qir/classical_int_chain.ll", builder
    );

    // The classical chain is synthesized into a CallCFInst; collect its op sequence in order.
    auto ops = std::vector<std::string>();
    for (const auto& bb : *circuit->GetIR()) {
        for (const auto& inst : bb) {
            if (const auto* call_cf = DynCast<ir::CallCFInst>(&inst)) {
                for (const auto& [op, operands] : call_cf->GetFunction().DumpInstructions()) {
                    ops.push_back(op);
                }
            }
        }
    }
    // Keep the chain's arithmetic/logical ops (constants lower to CopyInt), in build order.
    const auto is_chain_op = [](const std::string& op) {
        return op == "BoolToInt" || op == "Add" || op == "LShift" || op == "Mul" || op == "BOr"
                || op == "Sub" || op == "BXor";
    };
    auto chain = std::vector<std::string>();
    for (const auto& op : ops) {
        if (is_chain_op(op)) {
            chain.push_back(op);
        }
    }
    // zext/add/shl/mul/or/sub/xor, present and in QIR source order. A demand-driven (output-rooted)
    // build would instead give BoolToInt, Add, Mul, Sub, LShift, BOr, BXor.
    const auto expected =
            std::vector<std::string>{"BoolToInt", "Add", "LShift", "Mul", "BOr", "Sub", "BXor"};
    EXPECT_EQ(chain, expected);
}

// Quration-IR has no 2-qubit rotation, so rzz/rxx/ryy decompose on import; checks the exact
// resulting sequence: CX/RZ/CX for rzz, sandwiched in H (rxx) or SDag+H/H+S (ryy).
TEST(QirFrontend, DecomposesRzzRxxRyyIntoCnotRzCnot) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    auto* circuit = frontend::BuildCircuitFromQIRFile(
            "quration-core/tests/data/qir/rzz_rxx_ryy.ll", builder
    );

    auto it = circuit->GetIR()->begin()->begin();
    const auto expect_cx_rz_cx = [&it](double theta) {
        ASSERT_EQ(Opcode::Table::CX, it->GetOpcode().GetCode());
        EXPECT_EQ(1, Cast<BinaryInst>(&*it)->GetQubit0().id);  // target
        EXPECT_EQ(0, Cast<BinaryInst>(&*it)->GetQubit1().id);  // control
        ++it;
        ASSERT_EQ(Opcode::Table::RZ, it->GetOpcode().GetCode());
        const auto* rz = Cast<ParametrizedRotationInst>(&*it);
        EXPECT_EQ(1, rz->GetQubit().id);
        EXPECT_DOUBLE_EQ(theta, rz->GetTheta().value);
        ++it;
        ASSERT_EQ(Opcode::Table::CX, it->GetOpcode().GetCode());
        ++it;
    };

    // rzz(1.0): CX; RZ(1.0); CX.
    expect_cx_rz_cx(1.0);

    // rxx(2.0): H(q0); H(q1); CX; RZ(2.0); CX; H(q0); H(q1).
    ASSERT_EQ(Opcode::Table::H, it->GetOpcode().GetCode());
    EXPECT_EQ(0, Cast<UnaryInst>(&*it)->GetQubit().id);
    ++it;
    ASSERT_EQ(Opcode::Table::H, it->GetOpcode().GetCode());
    EXPECT_EQ(1, Cast<UnaryInst>(&*it)->GetQubit().id);
    ++it;
    expect_cx_rz_cx(2.0);
    ASSERT_EQ(Opcode::Table::H, it->GetOpcode().GetCode());
    ++it;
    ASSERT_EQ(Opcode::Table::H, it->GetOpcode().GetCode());
    ++it;

    // ryy(3.0): SDag(q0); SDag(q1); H(q0); H(q1); CX; RZ(3.0); CX; H(q0); H(q1); S(q0); S(q1).
    ASSERT_EQ(Opcode::Table::SDag, it->GetOpcode().GetCode());
    EXPECT_EQ(0, Cast<UnaryInst>(&*it)->GetQubit().id);
    ++it;
    ASSERT_EQ(Opcode::Table::SDag, it->GetOpcode().GetCode());
    EXPECT_EQ(1, Cast<UnaryInst>(&*it)->GetQubit().id);
    ++it;
    ASSERT_EQ(Opcode::Table::H, it->GetOpcode().GetCode());
    ++it;
    ASSERT_EQ(Opcode::Table::H, it->GetOpcode().GetCode());
    ++it;
    expect_cx_rz_cx(3.0);
    ASSERT_EQ(Opcode::Table::H, it->GetOpcode().GetCode());
    ++it;
    ASSERT_EQ(Opcode::Table::H, it->GetOpcode().GetCode());
    ++it;
    ASSERT_EQ(Opcode::Table::S, it->GetOpcode().GetCode());
    EXPECT_EQ(0, Cast<UnaryInst>(&*it)->GetQubit().id);
    ++it;
    ASSERT_EQ(Opcode::Table::S, it->GetOpcode().GetCode());
    EXPECT_EQ(1, Cast<UnaryInst>(&*it)->GetQubit().id);
    ++it;

    ASSERT_EQ(Opcode::Table::Return, it->GetOpcode().GetCode());
}

// Each single-qubit Clifford/named gate maps to its own IR opcode.
TEST(QirFrontend, ImportsCliffordSingleQubitGates) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    auto* circuit = frontend::BuildCircuitFromQIRFile(
            "quration-core/tests/data/qir/clifford_gates.ll", builder
    );

    auto it = circuit->GetIR()->begin()->begin();
    for (const auto expected :
         {Opcode::Table::I, Opcode::Table::X, Opcode::Table::Y, Opcode::Table::Z, Opcode::Table::H,
          Opcode::Table::S, Opcode::Table::SDag, Opcode::Table::T, Opcode::Table::TDag}) {
        ASSERT_EQ(expected, it->GetOpcode().GetCode());
        EXPECT_EQ(0, Cast<UnaryInst>(&*it)->GetQubit().id);
        ++it;
    }
    ASSERT_EQ(Opcode::Table::Measurement, it->GetOpcode().GetCode());
}

// Single-qubit rotations import with their angle preserved. Quration-IR has no sx, so it decomposes
// exactly (not up to phase) into RX(pi/2)+GlobalPhase(pi/4).
TEST(QirFrontend, ImportsRotationsAndDecomposesSx) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    auto* circuit = frontend::BuildCircuitFromQIRFile(
            "quration-core/tests/data/qir/rotations_and_sx.ll", builder
    );

    auto it = circuit->GetIR()->begin()->begin();
    ASSERT_EQ(Opcode::Table::RX, it->GetOpcode().GetCode());
    EXPECT_DOUBLE_EQ(0.5, Cast<ParametrizedRotationInst>(&*it)->GetTheta().value);
    ++it;
    ASSERT_EQ(Opcode::Table::RY, it->GetOpcode().GetCode());
    EXPECT_DOUBLE_EQ(1.25, Cast<ParametrizedRotationInst>(&*it)->GetTheta().value);
    ++it;
    ASSERT_EQ(Opcode::Table::RZ, it->GetOpcode().GetCode());
    EXPECT_DOUBLE_EQ(2.0, Cast<ParametrizedRotationInst>(&*it)->GetTheta().value);
    ++it;
    ASSERT_EQ(Opcode::Table::RX, it->GetOpcode().GetCode());  // sx -> RX(pi/2)
    EXPECT_DOUBLE_EQ(std::numbers::pi / 2, Cast<ParametrizedRotationInst>(&*it)->GetTheta().value);
    ++it;
    ASSERT_EQ(Opcode::Table::GlobalPhase, it->GetOpcode().GetCode());
    EXPECT_DOUBLE_EQ(std::numbers::pi / 4, Cast<GlobalPhaseInst>(&*it)->GetTheta().value);
    ++it;
    ASSERT_EQ(Opcode::Table::Measurement, it->GetOpcode().GetCode());
}

// barrier is a scheduling hint with no effect on the state, so the importer drops it.
TEST(QirFrontend, DropsBarrierHint) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    auto* circuit =
            frontend::BuildCircuitFromQIRFile("quration-core/tests/data/qir/barrier.ll", builder);

    auto it = circuit->GetIR()->begin()->begin();
    ASSERT_EQ(Opcode::Table::X, it->GetOpcode().GetCode());
    ++it;
    // barrier leaves no trace between the X and the measurement.
    ASSERT_EQ(Opcode::Table::Measurement, it->GetOpcode().GetCode());
}

// X(q0) then reset(q0): reset drives q0 back to |0> regardless of its prior state.
TEST(QirFrontend, ResetCallsPresetResetCircuit) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    auto* circuit = frontend::BuildCircuitFromQIRFile(
            "quration-core/tests/data/qir/reset_preset.ll", builder
    );

    auto it = circuit->GetIR()->begin()->begin();
    ++it;  // the X on q0
    ASSERT_TRUE(it->IsCall());
    EXPECT_EQ(qir::kReset, Cast<CallInst>(&*it)->GetCallee()->GetName());

    auto config = runtime::SimulatorConfig{runtime::SimulatorConfig::StateType::Toffoli};
    auto simulator = runtime::Simulator(config, circuit->GetIR());
    simulator.RunAll();
    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(false, state.ReadRegister(0));
}

// Both result_record_output calls reference the same raw Result identity (ptr null); redirecting
// either one is ambiguous, so this is rejected rather than picking one arbitrarily.
TEST(QirFrontend, DuplicateRecordOutputOfSameResultIsUnsupported) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);

    EXPECT_THROW(
            frontend::BuildCircuitFromQIRFile(
                    "quration-core/tests/data/qir/duplicate_record_output.ll", builder
            ),
            QRETError
    );
}

// switch's 2 arms converge on a phi at their join block; PhiContext is only recorded by
// If/Else's 2-predecessor merge, so a switch join's phi has no recognized enclosing condition.
TEST(QirFrontend, SwitchJoinPhiIsUnsupported) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);

    EXPECT_THROW(
            frontend::BuildCircuitFromQIRFile(
                    "quration-core/tests/data/qir/switch_join_phi.ll", builder
            ),
            QRETError
    );
}

// The branch condition is `icmp sgt (zext %c0 to i64), 0`, not a direct read_result: exercises the
// classical buffer resolving a br i1 condition rather than only int_record_output/switch values.
TEST(QirFrontend, ClassicalBranchConditionTakesCorrectArm) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    auto* circuit = frontend::BuildCircuitFromQIRFile(
            "quration-core/tests/data/qir/classical_branch_condition.ll", builder
    );

    auto config = runtime::SimulatorConfig{runtime::SimulatorConfig::StateType::Toffoli};
    auto simulator = runtime::Simulator(config, circuit->GetIR());
    simulator.RunAll();
    const auto& state = simulator.GetToffoliState();
    // q0 forced to One, so the icmp condition is true: the then arm (X q1) fires, not else (X q2).
    EXPECT_EQ(true, state.ReadRegister(0));
    EXPECT_EQ(true, state.ReadRegister(1));
    EXPECT_EQ(false, state.ReadRegister(2));
}

TEST_P(NestedIfInSwitchRoundTrip, TakesCorrectArms) {
    if (!qir::CanParseQIR()) {
        GTEST_SKIP() << "Skip QIR frontend test";
    }
    const auto& c = GetParam();
    EXPECT_EQ(c.expected, RunNestedIfInSwitchRoundTrip(c.select_case1, c.inner));
}

INSTANTIATE_TEST_SUITE_P(
        QirFrontend,
        NestedIfInSwitchRoundTrip,
        ::testing::Values(
                // Case 1 selected, inner condition false -> the case's else arm (X q4) fires.
                NestedIfInSwitchCase{"Case1InnerElse", true, false, {true, false, false, false, true}},
                // Case 1 selected, inner condition true -> the case's then arm (X q3) fires.
                NestedIfInSwitchCase{"Case1InnerThen", true, true, {true, false, true, true, false}},
                // Selector 0 -> default arm: the nested if never runs, targets stay unset.
                NestedIfInSwitchCase{"DefaultArm", false, false, {false, false, false, false, false}}
        ),
        [](const auto& info) { return info.param.name; }
);

