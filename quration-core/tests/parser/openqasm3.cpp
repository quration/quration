#include "qret/parser/openqasm3.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

TEST(OpenQASM3, RequireVersion3Header) {
    if (!qret::openqasm3::CanParseOpenQASM3()) {
        GTEST_SKIP() << "Skip OpenQASM3 test";
    }

    EXPECT_THROW(
            qret::openqasm3::ParseOpenQASM3(
                    "OPENQASM 2.0;\n"
                    "qreg q[1];\n"
            ),
            std::runtime_error
    );
}

TEST(OpenQASM3, ParseFileWithStdGates) {
    if (!qret::openqasm3::CanParseOpenQASM3()) {
        GTEST_SKIP() << "Skip OpenQASM3 test";
    }

    const auto program =
            qret::openqasm3::ParseOpenQASM3File("quration-core/tests/data/OpenQASM3/x.qasm");
    EXPECT_TRUE(program.IsQASM3());
    ASSERT_EQ(1, program.incls.size());
    EXPECT_EQ("stdgates.inc", program.incls.front());
}
