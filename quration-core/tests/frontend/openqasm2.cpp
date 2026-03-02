#include "qret/frontend/openqasm2.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "qret/base/type.h"
#include "qret/frontend/builder.h"
#include "qret/ir/context.h"
#include "qret/ir/module.h"
#include "qret/runtime/simulator.h"

using namespace qret;

runtime::Simulator
Simulate(const qret::openqasm2::Program& program, const runtime::SimulatorConfig& config) {
    ir::IRContext context;
    auto* module = ir::Module::Create("simulate", context);
    auto builder = qret::frontend::CircuitBuilder(module);
    auto* circuit = qret::frontend::BuildCircuitFromAST(program, builder);

    auto simulator = runtime::Simulator(config, circuit->GetIR());
    simulator.RunAll();
    return simulator;
}

std::size_t CountCallInst(const ir::Function& func) {
    auto count = std::size_t{0};
    for (const auto& bb : func) {
        for (const auto& inst : bb) {
            if (inst.IsCall()) {
                ++count;
            }
        }
    }
    return count;
}

TEST(OpenQASM2, RunX) {
    if (!qret::openqasm2::CanParseOpenQASM2()) {
        GTEST_SKIP() << "Skip OpenQASM2 test";
    }
    const auto program =
            qret::openqasm2::ParseOpenQASM2File("quration-core/tests/data/OpenQASM2/x.qasm");

    const auto config = runtime::SimulatorConfig{runtime::SimulatorConfig::StateType::Toffoli};
    const auto simulator = Simulate(program, config);

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(1, state.ReadRegister(0));
    EXPECT_EQ(0, state.ReadRegister(1));
}
TEST(OpenQASM2, RunXs) {
    if (!qret::openqasm2::CanParseOpenQASM2()) {
        GTEST_SKIP() << "Skip OpenQASM2 test";
    }
    const auto program =
            qret::openqasm2::ParseOpenQASM2File("quration-core/tests/data/OpenQASM2/xs.qasm");

    const auto config = runtime::SimulatorConfig{runtime::SimulatorConfig::StateType::Toffoli};
    const auto simulator = Simulate(program, config);

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(1, state.ReadRegister(0));
    EXPECT_EQ(1, state.ReadRegister(1));
    EXPECT_EQ(1, state.ReadRegister(2));
}
TEST(OpenQASM2, RCA) {
    if (!qret::openqasm2::CanParseOpenQASM2()) {
        GTEST_SKIP() << "Skip OpenQASM2 test";
    }
    const auto program =
            qret::openqasm2::ParseOpenQASM2File("quration-core/tests/data/OpenQASM2/rca.qasm");

    auto config = runtime::SimulatorConfig{runtime::SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 2;
    const auto simulator = Simulate(program, config);

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(0, state.GetValue(0, 1));  // cin
    EXPECT_EQ(1, state.GetValue(1, 4));  // a
    EXPECT_EQ(0, state.GetValue(5, 4));  // b
    EXPECT_EQ(1, state.GetValue(9, 1));  // cout
    EXPECT_EQ(16, BoolArrayAsInt(state.ReadRegisters(0, 5)));
}

TEST(OpenQASM2, RCAUsesSelectiveInlining) {
    if (!qret::openqasm2::CanParseOpenQASM2()) {
        GTEST_SKIP() << "Skip OpenQASM2 test";
    }
    const auto program =
            qret::openqasm2::ParseOpenQASM2File("quration-core/tests/data/OpenQASM2/rca.qasm");

    ir::IRContext context;
    auto* module = ir::Module::Create("inline-check", context);
    auto builder = qret::frontend::CircuitBuilder(module);
    qret::frontend::BuildCircuitFromAST(program, builder);

    auto num_functions = std::size_t{0};
    auto num_calls = std::size_t{0};
    for (const auto& func : *module) {
        ++num_functions;
        num_calls += CountCallInst(func);
    }
    // Keep high-level calls (majority/unmaj) while removing many std-gate wrapper calls.
    EXPECT_GT(num_functions, 1);
    EXPECT_GT(num_calls, 0);
    EXPECT_LT(num_calls, 20);
}

TEST(OpenQASM2, C4XPreservesCallBoundary) {
    if (!qret::openqasm2::CanParseOpenQASM2()) {
        GTEST_SKIP() << "Skip OpenQASM2 test";
    }
    const auto qasm = std::string(
            "OPENQASM 2.0;\n"
            "include \"qelib1.inc\";\n"
            "qreg q[5];\n"
            "c4x q[0],q[1],q[2],q[3],q[4];\n"
    );
    const auto program = qret::openqasm2::ParseOpenQASM2(qasm);

    ir::IRContext context;
    auto* module = ir::Module::Create("c4x-boundary-check", context);
    auto builder = qret::frontend::CircuitBuilder(module);
    qret::frontend::BuildCircuitFromAST(program, builder);

    auto num_functions = std::size_t{0};
    auto num_calls = std::size_t{0};
    for (const auto& func : *module) {
        ++num_functions;
        num_calls += CountCallInst(func);
    }
    EXPECT_GT(num_functions, 1);
    EXPECT_GT(num_calls, 0);
}

TEST(OpenQASM2, RejectDuplicateRegisterDeclaration) {
    if (!qret::openqasm2::CanParseOpenQASM2()) {
        GTEST_SKIP() << "Skip OpenQASM2 test";
    }
    const auto qasm = std::string(
            "OPENQASM 2.0;\n"
            "qreg q[1];\n"
            "qreg q[2];\n"
    );

    const auto program = qret::openqasm2::ParseOpenQASM2(qasm);
    ir::IRContext context;
    auto* module = ir::Module::Create("duplicate-register", context);
    auto builder = qret::frontend::CircuitBuilder(module);

    EXPECT_THROW(qret::frontend::BuildCircuitFromAST(program, builder), std::runtime_error);
}

TEST(OpenQASM2, RejectDuplicateRegisterAcrossTypes) {
    if (!qret::openqasm2::CanParseOpenQASM2()) {
        GTEST_SKIP() << "Skip OpenQASM2 test";
    }
    const auto qasm = std::string(
            "OPENQASM 2.0;\n"
            "qreg q[1];\n"
            "creg q[1];\n"
    );

    const auto program = qret::openqasm2::ParseOpenQASM2(qasm);
    ir::IRContext context;
    auto* module = ir::Module::Create("duplicate-register-across-types", context);
    auto builder = qret::frontend::CircuitBuilder(module);

    EXPECT_THROW(qret::frontend::BuildCircuitFromAST(program, builder), std::runtime_error);
}

TEST(OpenQASM2, RejectDuplicateGateDeclaration) {
    if (!qret::openqasm2::CanParseOpenQASM2()) {
        GTEST_SKIP() << "Skip OpenQASM2 test";
    }
    const auto qasm = std::string(
            "OPENQASM 2.0;\n"
            "gate myx a { U(pi,0,pi) a; }\n"
            "gate myx a { U(pi,0,pi) a; }\n"
            "qreg q[1];\n"
            "myx q[0];\n"
    );

    const auto program = qret::openqasm2::ParseOpenQASM2(qasm);
    ir::IRContext context;
    auto* module = ir::Module::Create("duplicate-gate", context);
    auto builder = qret::frontend::CircuitBuilder(module);

    EXPECT_THROW(qret::frontend::BuildCircuitFromAST(program, builder), std::runtime_error);
}

TEST(OpenQASM2, ResetQubitToZero) {
    if (!qret::openqasm2::CanParseOpenQASM2()) {
        GTEST_SKIP() << "Skip OpenQASM2 test";
    }

    const auto qasm = std::string(
            "OPENQASM 2.0;\n"
            "qreg q[1];\n"
            "creg c[1];\n"
            "U(pi,0,pi) q[0];\n"
            "reset q[0];\n"
            "measure q[0] -> c[0];\n"
    );
    const auto program = qret::openqasm2::ParseOpenQASM2(qasm);

    const auto config = runtime::SimulatorConfig{runtime::SimulatorConfig::StateType::Toffoli};
    const auto simulator = Simulate(program, config);
    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(0, state.ReadRegister(0));
}

TEST(OpenQASM2, IncludeExternalLibrary) {
    if (!qret::openqasm2::CanParseOpenQASM2()) {
        GTEST_SKIP() << "Skip OpenQASM2 test";
    }

    const auto program = qret::openqasm2::ParseOpenQASM2File(
            "quration-core/tests/data/OpenQASM2/include_external.qasm"
    );

    const auto config = runtime::SimulatorConfig{runtime::SimulatorConfig::StateType::Toffoli};
    const auto simulator = Simulate(program, config);
    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(1, state.ReadRegister(0));
}
