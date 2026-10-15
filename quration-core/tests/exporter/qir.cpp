// Assert the exported QIR text contains the required constructs (not a line-by-line golden match).
// End-to-end semantic equivalence is verified separately against QDK in tests/qir_equiv.

#include "qret/exporter/qir.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <vector>

#include "qret/base/portable_function.h"
#include "qret/error.h"
#include "qret/exception.h"
#include "qret/frontend/attribute.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/control_flow.h"
#include "qret/frontend/intrinsic.h"
#include "qret/ir/context.h"
#include "qret/ir/module.h"

using namespace qret;
using namespace qret::ir;

namespace {
// True if every `part` occurs in `text`, each strictly after the previous one (order-preserving
// substring search). Lets a test assert a call sequence without pinning the exact full output.
bool ContainsInOrder(const std::string& text, const std::vector<std::string>& parts) {
    auto pos = std::size_t{0};
    for (const auto& part : parts) {
        const auto found = text.find(part, pos);
        if (found == std::string::npos) {
            return false;
        }
        pos = found + part.size();
    }
    return true;
}

struct BellPairGen : frontend::CircuitGenerator {
    explicit BellPairGen(frontend::CircuitBuilder* builder)
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
        frontend::gate::H(qs[0]);
        frontend::gate::CX(qs[1], qs[0]);  // target=q1, control=q0
        frontend::gate::Measure(qs[0], rs[0]);
        frontend::gate::Measure(qs[1], rs[1]);
        return EndCircuitDefinition();
    }
};

struct RotationGlobalPhaseGen : frontend::CircuitGenerator {
    explicit RotationGlobalPhaseGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return "main";
    }
    std::string GetCacheKey() const override {
        return GetName();
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 1, Attribute::Operate);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        const auto qs = GetQubits(0);
        frontend::gate::RZ(qs[0], 1.5);
        frontend::gate::GlobalPhase(GetBuilder(), 0.5);
        return EndCircuitDefinition();
    }
};

struct SubCircuitGen : frontend::CircuitGenerator {
    explicit SubCircuitGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return "sub";
    }
    std::string GetCacheKey() const override {
        return GetName();
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 1, Attribute::Operate);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        const auto qs = GetQubits(0);
        frontend::gate::X(qs[0]);
        return EndCircuitDefinition();
    }
};
struct MainCallsSubGen : frontend::CircuitGenerator {
    explicit MainCallsSubGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return "main";
    }
    std::string GetCacheKey() const override {
        return GetName();
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 1, Attribute::Operate);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        const auto qs = GetQubits(0);
        auto* sub_circuit = SubCircuitGen(GetBuilder()).Generate();
        (*sub_circuit)(qs[0]);
        return EndCircuitDefinition();
    }
};

// Measures q0 into r0, then If(r0) { X(q0) } EndIf(r0) with no `else` arm.
struct IfNoElseGen : frontend::CircuitGenerator {
    explicit IfNoElseGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return "main";
    }
    std::string GetCacheKey() const override {
        return GetName();
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 1, Attribute::Operate);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        const auto qs = GetQubits(0);
        const auto r0 = GetTemporalRegister();
        frontend::gate::Measure(qs[0], r0);
        frontend::control_flow::If(r0);
        frontend::gate::X(qs[0]);
        frontend::control_flow::EndIf(r0);
        return EndCircuitDefinition();
    }
};

// Measures q0, then real control_flow::If/Else flips q1 or q2.
struct IfElseCircuitGen : frontend::CircuitGenerator {
    explicit IfElseCircuitGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
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
};

struct CCYCCZGen : frontend::CircuitGenerator {
    explicit CCYCCZGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
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
        frontend::gate::CCY(qs[0], qs[1], qs[2]);
        frontend::gate::CCZ(qs[0], qs[1], qs[2]);
        return EndCircuitDefinition();
    }
};

struct MCXGen : frontend::CircuitGenerator {
    explicit MCXGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
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
        frontend::gate::MCX(qs[0], qs.Range(1, 2));
        return EndCircuitDefinition();
    }
};

// Measures q0,q1 into a switch value; Switch dispatches among 3 cases + default, each flipping q2.
struct SwitchGen : frontend::CircuitGenerator {
    explicit SwitchGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
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
        const auto rs = GetTemporalRegisters(2);
        frontend::gate::Measure(qs[0], rs[0]);
        frontend::gate::Measure(qs[1], rs[1]);

        frontend::control_flow::Switch(rs);
        frontend::control_flow::Case(rs, 0);
        frontend::gate::X(qs[2]);
        frontend::control_flow::Case(rs, 1);
        frontend::gate::Y(qs[2]);
        frontend::control_flow::Case(rs, 2);
        frontend::gate::Z(qs[2]);
        frontend::control_flow::Default(rs);
        frontend::gate::H(qs[2]);
        frontend::control_flow::EndSwitch(rs);

        return EndCircuitDefinition();
    }
};

// A sub-circuit with an Output register argument, measuring q into it (`ptr %rN`).
struct SubCircuitWithOutputRegGen : frontend::CircuitGenerator {
    explicit SubCircuitWithOutputRegGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return "measure_into_arg";
    }
    std::string GetCacheKey() const override {
        return GetName();
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 1, Attribute::Operate);
        arg.Add("r", Type::Register, 1, Attribute::Output);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        const auto qs = GetQubits(0);
        const auto rs = GetRegisters(1);
        frontend::gate::Measure(qs[0], rs[0]);
        return EndCircuitDefinition();
    }
};
struct MainCallsSubWithOutputRegGen : frontend::CircuitGenerator {
    explicit MainCallsSubWithOutputRegGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return "main";
    }
    std::string GetCacheKey() const override {
        return GetName();
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 1, Attribute::Operate);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        const auto qs = GetQubits(0);
        auto* sub_circuit = SubCircuitWithOutputRegGen(GetBuilder()).Generate();
        const auto r0 = GetTemporalRegister();
        (*sub_circuit)(qs[0], r0);
        return EndCircuitDefinition();
    }
};

// A sub-circuit with a Register argument that's never called anywhere in the module.
struct NeverCalledSubWithRegGen : frontend::CircuitGenerator {
    explicit NeverCalledSubWithRegGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return "orphan";
    }
    std::string GetCacheKey() const override {
        return GetName();
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 1, Attribute::Operate);
        arg.Add("r", Type::Register, 1, Attribute::Output);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        const auto qs = GetQubits(0);
        const auto rs = GetRegisters(1);
        frontend::gate::Measure(qs[0], rs[0]);
        return EndCircuitDefinition();
    }
};
struct MainWithOrphanSubGen : frontend::CircuitGenerator {
    explicit MainWithOrphanSubGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return "main";
    }
    std::string GetCacheKey() const override {
        return GetName();
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 1, Attribute::Operate);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        NeverCalledSubWithRegGen(GetBuilder()).Generate();
        const auto qs = GetQubits(0);
        frontend::gate::X(qs[0]);
        return EndCircuitDefinition();
    }
};

// Exercises every PortableFunction instruction EmitPortableFunctionInstruction knows how to export.
struct ClassicalOpsCoverageGen : frontend::CircuitGenerator {
    explicit ClassicalOpsCoverageGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return "main";
    }
    std::string GetCacheKey() const override {
        return GetName();
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 1, Attribute::Operate);
        arg.Add("r", Type::Register, 1, Attribute::Output);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        const auto qs = GetQubits(0);
        const auto rs = GetRegisters(1);
        const auto r_in = GetTemporalRegister();
        frontend::gate::Measure(qs[0], r_in);

        auto builder = PortableFunctionBuilder();
        const auto in = builder.AddBoolArrayInputVariable("input", 1);
        auto out = builder.AddBoolArrayOutputVariable("output", 1);

        auto bit = builder.AddTmpBoolVariable();
        bit = in[0];

        auto a = builder.AddTmpIntVariable();
        a = 6;
        auto b = builder.AddTmpIntVariable();
        b = 3;
        auto add = a + b;
        auto sub = a - b;
        [[maybe_unused]] const auto mul = a * b;
        [[maybe_unused]] const auto div = a / b;
        [[maybe_unused]] const auto mod = a % b;
        [[maybe_unused]] const auto band = a & b;
        [[maybe_unused]] const auto bor = a | b;
        [[maybe_unused]] const auto bxor = a ^ b;
        [[maybe_unused]] const auto shl = a << b;
        [[maybe_unused]] const auto ashr = a >> b;
        [[maybe_unused]] const auto neg = -a;
        [[maybe_unused]] const auto bnot = ~a;

        auto x = builder.AddTmpBoolVariable();
        x = bit;
        [[maybe_unused]] const auto land = x & bit;
        [[maybe_unused]] const auto lor = x | bit;
        [[maybe_unused]] const auto lxor = x ^ bit;
        [[maybe_unused]] const auto lnot = !x;

        auto int_from_bool = builder.AddTmpIntVariable();
        int_from_bool = x;
        auto bool_from_int = builder.AddTmpBoolVariable();
        bool_from_int = add;
        auto muxed = builder.Mux(bit, a, b);
        auto muxed_copy = builder.AddTmpIntVariable();
        muxed_copy = muxed;

        [[maybe_unused]] const auto eq = add == sub;
        [[maybe_unused]] const auto ne = add != sub;
        [[maybe_unused]] const auto lt = add < sub;
        [[maybe_unused]] const auto le = add <= sub;
        [[maybe_unused]] const auto gt = add > sub;
        out[0] = add >= sub;

        const auto function = PortableAnnotatedFunction(builder.Compile("classical_ops_coverage"));
        frontend::gate::CallCF(function, frontend::Registers(r_in), rs);
        return EndCircuitDefinition();
    }
};
// X, a clean-ancilla optimization hint, then X: the hint has no QIR form and must be dropped.
struct OptHintGen : frontend::CircuitGenerator {
    explicit OptHintGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return "main";
    }
    std::string GetCacheKey() const override {
        return GetName();
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 1, Attribute::Operate);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        const auto qs = GetQubits(0);
        frontend::gate::X(qs[0]);
        frontend::attribute::MarkAsClean(qs[0]);
        frontend::gate::X(qs[0]);
        return EndCircuitDefinition();
    }
};

// A single multi-bit Output register: each bit is recorded individually under one tuple header.
struct MultiBitOutputGen : frontend::CircuitGenerator {
    explicit MultiBitOutputGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return "main";
    }
    std::string GetCacheKey() const override {
        return GetName();
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 3, Attribute::Operate);
        arg.Add("r", Type::Register, 3, Attribute::Output);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        const auto qs = GetQubits(0);
        const auto rs = GetRegisters(1);
        frontend::gate::Measure(qs[0], rs[0]);
        frontend::gate::Measure(qs[1], rs[1]);
        frontend::gate::Measure(qs[2], rs[2]);
        return EndCircuitDefinition();
    }
};

// Covers the single-qubit, rotation, and controlled gates each opcode maps to on export.
struct GateCoverageGen : frontend::CircuitGenerator {
    explicit GateCoverageGen(frontend::CircuitBuilder* builder)
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
        frontend::gate::I(qs[0]);  // identity has no QIR form and is dropped on export
        frontend::gate::Y(qs[0]);
        frontend::gate::Z(qs[0]);
        frontend::gate::S(qs[0]);
        frontend::gate::SDag(qs[0]);
        frontend::gate::T(qs[0]);
        frontend::gate::TDag(qs[0]);
        frontend::gate::RX(qs[0], 0.5);
        frontend::gate::RY(qs[0], 1.25);
        frontend::gate::CY(qs[0], qs[1]);
        frontend::gate::CZ(qs[0], qs[1]);
        return EndCircuitDefinition();
    }
};
}  // namespace

TEST(QirExporter, EmitsSingleQubitRotationAndControlledGateNames) {
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    GateCoverageGen(&builder).Generate();

    const auto text = qir::ExportModuleToQIR(*module);

    EXPECT_TRUE(ContainsInOrder(
            text,
            {"call void @__quantum__qis__y__body", "call void @__quantum__qis__z__body",
             "call void @__quantum__qis__s__body", "call void @__quantum__qis__s__adj",
             "call void @__quantum__qis__t__body", "call void @__quantum__qis__t__adj",
             "call void @__quantum__qis__rx__body(double 0.5", "call void @__quantum__qis__ry__body(double 1.25",
             "call void @__quantum__qis__cy__body", "call void @__quantum__qis__cz__body"}
    )) << text;
    // The identity gate has no QIR instruction and is dropped.
    EXPECT_EQ(std::string::npos, text.find("__quantum__qis__id__body"));
}

TEST(QirExporter, DropsOptimizationHintInstructions) {
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    OptHintGen(&builder).Generate();

    const auto text = qir::ExportModuleToQIR(*module);

    // The clean-ancilla hint has no QIR form, so exactly the two X gates remain, nothing between.
    const auto first = text.find("call void @__quantum__qis__x__body");
    const auto second = text.find("call void @__quantum__qis__x__body", first + 1);
    const auto third = text.find("call void @__quantum__qis__x__body", second + 1);
    EXPECT_NE(std::string::npos, first);
    EXPECT_NE(std::string::npos, second);
    EXPECT_EQ(std::string::npos, third);
}

TEST(QirExporter, MultiBitOutputRegisterRecordsEachBitUnderOneTuple) {
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    MultiBitOutputGen(&builder).Generate();

    const auto text = qir::ExportModuleToQIR(*module);

    // A width-3 Output register is recorded as a 3-element tuple of individual result records.
    EXPECT_TRUE(ContainsInOrder(
            text,
            {"call void @__quantum__rt__tuple_record_output(i64 3,",
             "call void @__quantum__rt__result_record_output(ptr null,",
             "call void @__quantum__rt__result_record_output(ptr inttoptr (i64 1 to ptr),",
             "call void @__quantum__rt__result_record_output(ptr inttoptr (i64 2 to ptr),"}
    )) << text;
}

TEST(QirExporter, BellPairEmitsExpectedCallSequenceAndResultLabels) {
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    BellPairGen(&builder).Generate();

    const auto text = qir::ExportModuleToQIR(*module);

    // H, then CX(target=q1, control=q0) as cnot(control=q0, target=q1), then two measurements and
    // their result recordings, in order.
    EXPECT_TRUE(ContainsInOrder(
            text,
            {"call void @__quantum__qis__h__body(ptr null)",
             "call void @__quantum__qis__cnot__body(ptr null, ptr inttoptr (i64 1 to ptr))",
             "call void @__quantum__qis__mz__body(ptr null, ptr null)",
             "call void @__quantum__qis__mz__body(ptr inttoptr (i64 1 to ptr), ptr inttoptr (i64 1 "
             "to ptr))",
             "call void @__quantum__rt__result_record_output(ptr null, ptr @"}
    )) << text;

    EXPECT_NE(std::string::npos, text.find("\"required_num_qubits\"=\"2\""));
    EXPECT_NE(std::string::npos, text.find("\"required_num_results\"=\"2\""));
    EXPECT_NE(std::string::npos, text.find("r[0]"));
    EXPECT_NE(std::string::npos, text.find("r[1]"));
}

TEST(QirExporter, RotationKeepsThetaAndDropsGlobalPhase) {
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    RotationGlobalPhaseGen(&builder).Generate();

    const auto text = qir::ExportModuleToQIR(*module);

    // theta is preserved as the double argument; the global phase (0.5) has no QIR form, so it is
    // dropped rather than emitted as any instruction.
    EXPECT_NE(std::string::npos, text.find("call void @__quantum__qis__rz__body(double 1.5, ptr null)"));
    EXPECT_EQ(std::string::npos, text.find("double 0.5"));
}

TEST(QirExporter, SubCircuitCallUsesFormalQubitParameters) {
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    MainCallsSubGen(&builder).Generate();

    const auto text = qir::ExportModuleToQIR(*module, "main");

    // The call passes the caller's qubit; the sub-circuit body acts on its own formal parameter
    // `%q0`, not a global qubit id.
    EXPECT_TRUE(ContainsInOrder(
            text,
            {"call void @sub(ptr null)", "define void @sub(ptr %q0)",
             "call void @__quantum__qis__x__body(ptr %q0)"}
    )) << text;
}

TEST(QirExporter, ConditionalBranchUsesReadResult) {
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    IfNoElseGen(&builder).Generate();

    const auto text = qir::ExportModuleToQIR(*module);

    EXPECT_TRUE(ContainsInOrder(
            text,
            {"call void @__quantum__qis__mz__body(ptr null, ptr null)",
             "%c0 = call i1 @__quantum__rt__read_result(ptr null)",
             "br i1 %c0, label %then_0, label %if_cont_0", "then_0:",
             "call void @__quantum__qis__x__body(ptr null)"}
    )) << text;
}

TEST(QirExporter, IfElseEmitsDistinctArmBodies) {
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    IfElseCircuitGen(&builder).Generate();

    const auto text = qir::ExportModuleToQIR(*module);

    // then-arm applies X to q1, else-arm applies X to q2: distinct arm bodies.
    EXPECT_TRUE(ContainsInOrder(
            text,
            {"br i1 %c0, label %then_0, label %else_0", "then_0:",
             "call void @__quantum__qis__x__body(ptr inttoptr (i64 1 to ptr))", "else_0:",
             "call void @__quantum__qis__x__body(ptr inttoptr (i64 2 to ptr))"}
    )) << text;
}

TEST(QirExporter, CCYAndCCZLowerToCliffordConjugatedCCX) {
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    CCYCCZGen(&builder).Generate();

    const auto text = qir::ExportModuleToQIR(*module);

    // No dedicated CCY/CCZ QIS call exists, so each lowers to a Clifford-conjugated CCX.
    EXPECT_TRUE(ContainsInOrder(
            text,
            {"call void @__quantum__qis__s__adj(ptr null)",
             "call void @__quantum__qis__ccx__body(ptr inttoptr (i64 1 to ptr), ptr inttoptr (i64 2 "
             "to ptr), ptr null)",
             "call void @__quantum__qis__s__body(ptr null)",
             "call void @__quantum__qis__h__body(ptr null)",
             "call void @__quantum__qis__ccx__body(ptr inttoptr (i64 1 to ptr), ptr inttoptr (i64 2 "
             "to ptr), ptr null)",
             "call void @__quantum__qis__h__body(ptr null)"}
    )) << text;
}

TEST(QirExporter, MCXIsUnsupported) {
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    MCXGen(&builder).Generate();

    EXPECT_THROW(
            {
                try {
                    qir::ExportModuleToQIR(*module);
                } catch (const QRETError& ex) {
                    EXPECT_EQ(error::UnsupportedQIRInstruction, ex.id());
                    throw;
                }
            },
            QRETError
    );
}

TEST(QirExporter, SwitchEmitsCasesIntOpsAndModuleFlags) {
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    SwitchGen(&builder).Generate();

    const auto text = qir::ExportModuleToQIR(*module);

    // The switch value is assembled from read_result via zext/shl/or, then dispatched to 3 cases.
    EXPECT_TRUE(ContainsInOrder(
            text,
            {"zext i1", "shl i64", "or i64", "switch i64", "i64 0, label %case_0_0",
             "i64 1, label %case_1_0", "i64 2, label %case_2_0"}
    )) << text;
    EXPECT_NE(std::string::npos, text.find("multiple_target_branching"));
    EXPECT_NE(std::string::npos, text.find("int_computations"));
}

TEST(QirExporter, SubCircuitRegisterParametersOmitWriteonly) {
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    MainCallsSubWithOutputRegGen(&builder).Generate();

    const auto text = qir::ExportModuleToQIR(*module);

    EXPECT_NE(std::string::npos, text.find("define void @measure_into_arg(ptr %q0, ptr %r0)"));
    // A register formal parameter is never marked writeonly (it may be read back in-body).
    EXPECT_EQ(std::string::npos, text.find("writeonly %r"));
}

TEST(QirExporter, UncalledRegisterArgSubCircuitExportsFine) {
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    MainWithOrphanSubGen(&builder).Generate();

    const auto text = qir::ExportModuleToQIR(*module);

    EXPECT_NE(std::string::npos, text.find("define void @orphan(ptr %q0, ptr %r0)"));
}

TEST(QirExporter, EmitsEveryPortableFunctionInstruction) {
    auto context = IRContext();
    auto* module = Module::Create("test", context);
    auto builder = frontend::CircuitBuilder(module);
    ClassicalOpsCoverageGen(&builder).Generate();

    const auto text = qir::ExportModuleToQIR(*module);
    for (const auto& expected :
         {"and i64", "or i64", "xor i64", "shl i64", "ashr i64", "add i64", "sub i64", "mul i64",
          "sdiv i64", "srem i64", "sub i64 0,", "and i1", "or i1", "xor i1", "xor i1 %c0, true",
          "zext i1", "icmp eq i64", "icmp ne i64", "icmp slt i64", "icmp sle i64", "icmp sgt i64",
          "icmp sge i64", "select i1"}) {
        EXPECT_NE(std::string::npos, text.find(expected)) << "missing: " << expected;
    }
}
