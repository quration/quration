#include "qret/parser/openqasm2.h"

#include <gtest/gtest.h>

#include <iterator>
#include <stdexcept>
#include <string>

TEST(OpenQASM2, FileNotFound) {
    if (!qret::openqasm2::CanParseOpenQASM2()) {
        GTEST_SKIP() << "Skip OpenQASM2 test";
    }
    EXPECT_THROW(
            qret::openqasm2::ParseOpenQASM2File(
                    "quration-core/tests/data/OpenQASM2/file_not_found.qasm"
            ),
            std::runtime_error
    );
}
TEST(OpenQASM2, NoHeader) {
    if (!qret::openqasm2::CanParseOpenQASM2()) {
        GTEST_SKIP() << "Skip OpenQASM2 test";
    }
    EXPECT_THROW(
            qret::openqasm2::ParseOpenQASM2File(
                    "quration-core/tests/data/OpenQASM2/error_no_header.qasm"
            ),
            std::runtime_error
    );
}
TEST(OpenQASM2, NoSemiColon) {
    if (!qret::openqasm2::CanParseOpenQASM2()) {
        GTEST_SKIP() << "Skip OpenQASM2 test";
    }
    EXPECT_THROW(
            qret::openqasm2::ParseOpenQASM2File(
                    "quration-core/tests/data/OpenQASM2/error_no_semi_colon.qasm"
            ),
            std::runtime_error
    );
}
TEST(OpenQASM2, InvalidExp) {
    if (!qret::openqasm2::CanParseOpenQASM2()) {
        GTEST_SKIP() << "Skip OpenQASM2 test";
    }
    EXPECT_THROW(
            qret::openqasm2::ParseOpenQASM2File(
                    "quration-core/tests/data/OpenQASM2/error_invalid_exp.qasm"
            ),
            std::runtime_error
    );
}

TEST(OpenQASM2, ParseFromStringWithComment) {
    if (!qret::openqasm2::CanParseOpenQASM2()) {
        GTEST_SKIP() << "Skip OpenQASM2 test";
    }
    const auto qasm = std::string(
            "OPENQASM 2.0;\n"
            "qreg q[1]; // this is a comment\n"
            "creg c[1];\n"
            "U(pi,0,pi) q[0];\n"
            "measure q[0] -> c[0];\n"
    );

    const auto program = qret::openqasm2::ParseOpenQASM2(qasm);
    EXPECT_TRUE(program.IsQASM2());
    EXPECT_EQ(4, program.sts.size());
}

TEST(OpenQASM2, ParsePowerPrecedence) {
    if (!qret::openqasm2::CanParseOpenQASM2()) {
        GTEST_SKIP() << "Skip OpenQASM2 test";
    }

    const auto qasm = std::string(
            "OPENQASM 2.0;\n"
            "qreg q[1];\n"
            "U(2*3^2,0,0) q[0];\n"
    );

    const auto program = qret::openqasm2::ParseOpenQASM2(qasm);
    ASSERT_EQ(2, program.sts.size());

    const auto& uop =
            *static_cast<const qret::openqasm2::Uop*>(std::next(program.sts.begin(), 1)->get());
    ASSERT_EQ(3, uop.params.size());

    const auto& exp = uop.params.front();
    ASSERT_TRUE(exp->IsBinary());
    EXPECT_EQ(qret::openqasm2::Exp::Type::mul, exp->t);
    const auto& rhs = std::get<1>(exp->GetBinary());
    ASSERT_TRUE(rhs->IsBinary());
    EXPECT_EQ(qret::openqasm2::Exp::Type::pow, rhs->t);
}

TEST(OpenQASM2, ParsePowerRightAssociative) {
    if (!qret::openqasm2::CanParseOpenQASM2()) {
        GTEST_SKIP() << "Skip OpenQASM2 test";
    }

    const auto qasm = std::string(
            "OPENQASM 2.0;\n"
            "qreg q[1];\n"
            "U(2^3^2,0,0) q[0];\n"
    );

    const auto program = qret::openqasm2::ParseOpenQASM2(qasm);
    ASSERT_EQ(2, program.sts.size());

    const auto& uop =
            *static_cast<const qret::openqasm2::Uop*>(std::next(program.sts.begin(), 1)->get());
    const auto& exp = uop.params.front();
    ASSERT_TRUE(exp->IsBinary());
    EXPECT_EQ(qret::openqasm2::Exp::Type::pow, exp->t);
    const auto& rhs = std::get<1>(exp->GetBinary());
    ASSERT_TRUE(rhs->IsBinary());
    EXPECT_EQ(qret::openqasm2::Exp::Type::pow, rhs->t);
}

TEST(OpenQASM2, ParseUnaryMinusBeforeFunction) {
    if (!qret::openqasm2::CanParseOpenQASM2()) {
        GTEST_SKIP() << "Skip OpenQASM2 test";
    }

    const auto qasm = std::string(
            "OPENQASM 2.0;\n"
            "qreg q[1];\n"
            "U(-sin(pi/2),0,0) q[0];\n"
    );

    const auto program = qret::openqasm2::ParseOpenQASM2(qasm);
    ASSERT_EQ(2, program.sts.size());

    const auto& uop =
            *static_cast<const qret::openqasm2::Uop*>(std::next(program.sts.begin(), 1)->get());
    const auto& exp = uop.params.front();
    ASSERT_TRUE(exp->IsUnary());
    EXPECT_EQ(qret::openqasm2::Exp::Type::minus, exp->t);
    ASSERT_TRUE(exp->GetUnary()->IsFunc());
    EXPECT_EQ(qret::openqasm2::Exp::Type::sin, exp->GetUnary()->t);
}

TEST(OpenQASM2, ParseBarrierInGateDecl) {
    if (!qret::openqasm2::CanParseOpenQASM2()) {
        GTEST_SKIP() << "Skip OpenQASM2 test";
    }

    const auto qasm = std::string(
            "OPENQASM 2.0;\n"
            "gate g a,b { barrier a,b; CX a,b; }\n"
            "qreg q[2];\n"
            "g q[0],q[1];\n"
    );

    const auto program = qret::openqasm2::ParseOpenQASM2(qasm);
    ASSERT_EQ(3, program.sts.size());

    const auto& gate_decl =
            *static_cast<const qret::openqasm2::GateDecl*>(program.sts.front().get());
    ASSERT_FALSE(gate_decl.gops.empty());
    const auto& barrier =
            *static_cast<const qret::openqasm2::Barrier*>(gate_decl.gops.front().get());
    ASSERT_EQ(2, barrier.args.size());
    EXPECT_EQ("a", barrier.args[0].symbol);
    EXPECT_EQ("b", barrier.args[1].symbol);
}

TEST(OpenQASM2, ParseImportAlias) {
    if (!qret::openqasm2::CanParseOpenQASM2()) {
        GTEST_SKIP() << "Skip OpenQASM2 test";
    }

    const auto qasm = std::string(
            "OPENQASM 2.0;\n"
            "import \"qelib1.inc\" ;\n"
            "qreg q[1];\n"
            "x q[0];\n"
    );

    const auto program = qret::openqasm2::ParseOpenQASM2(qasm);
    ASSERT_EQ(1, program.incls.size());
    EXPECT_EQ("qelib1.inc", program.incls.front());
}

TEST(OpenQASM2, ParseOpenQASM3DeclSyntax) {
    if (!qret::openqasm2::CanParseOpenQASM2()) {
        GTEST_SKIP() << "Skip OpenQASM2 test";
    }

    const auto qasm = std::string(
            "OPENQASM 3.0;\n"
            "qubit[2] q;\n"
            "bit[2] c;\n"
    );

    const auto program = qret::openqasm2::ParseOpenQASM2(qasm);
    ASSERT_TRUE(program.IsQASM3());
    ASSERT_EQ(2, program.sts.size());

    const auto& qdecl = *static_cast<const qret::openqasm2::Decl*>(program.sts.front().get());
    EXPECT_TRUE(qdecl.is_q);
    EXPECT_EQ("q", qdecl.symbol);
    EXPECT_EQ(2, qdecl.num);

    // cdecl is a Windows reserved keyword, so we use 'bit_decl' here.
    const auto& bit_decl =
            *static_cast<const qret::openqasm2::Decl*>(std::next(program.sts.begin(), 1)->get());
    EXPECT_FALSE(bit_decl.is_q);
    EXPECT_EQ("c", bit_decl.symbol);
    EXPECT_EQ(2, bit_decl.num);
}
