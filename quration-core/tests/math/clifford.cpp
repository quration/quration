#include "qret/math/clifford.h"

#include <gtest/gtest.h>

#include <fmt/format.h>

#include "qret/math/pauli.h"

using namespace qret::math;

TEST(StabilizerTableau, FromPauliString) {
    EXPECT_EQ(StabilizerTableau::X(1, 0), StabilizerTableau::FromPauliString(PauliString::X(1, 0)));
    EXPECT_EQ(
            StabilizerTableau::X(10, 2),
            StabilizerTableau::FromPauliString(PauliString::X(10, 2))
    );
    EXPECT_EQ(StabilizerTableau::Y(1, 0), StabilizerTableau::FromPauliString(PauliString::Y(1, 0)));
    EXPECT_EQ(StabilizerTableau::Z(1, 0), StabilizerTableau::FromPauliString(PauliString::Z(1, 0)));
}
TEST(StabilizerTableau, X) {
    const auto x = StabilizerTableau::X(1, 0);

    auto p = PauliString("i");

    x.Apply(p);
    EXPECT_STREQ(p.ToString().c_str(), "+1 I");
    p = PauliString("x");
    x.Apply(p);
    EXPECT_STREQ(p.ToString().c_str(), "+1 X");
    p = PauliString("y");
    x.Apply(p);
    EXPECT_STREQ(p.ToString().c_str(), "-1 Y");
    p = PauliString("z");
    x.Apply(p);
    EXPECT_STREQ(p.ToString().c_str(), "-1 Z");
}
TEST(StabilizerTableau, Y) {
    const auto y = StabilizerTableau::Y(1, 0);

    auto p = PauliString("i");

    y.Apply(p);
    EXPECT_STREQ(p.ToString().c_str(), "+1 I");
    p = PauliString("x");
    y.Apply(p);
    EXPECT_STREQ(p.ToString().c_str(), "-1 X");
    p = PauliString("y");
    y.Apply(p);
    EXPECT_STREQ(p.ToString().c_str(), "+1 Y");
    p = PauliString("z");
    y.Apply(p);
    EXPECT_STREQ(p.ToString().c_str(), "-1 Z");
}
TEST(StabilizerTableau, Z) {
    const auto z = StabilizerTableau::Z(1, 0);

    auto p = PauliString("i");

    z.Apply(p);
    EXPECT_STREQ(p.ToString().c_str(), "+1 I");
    p = PauliString("x");
    z.Apply(p);
    EXPECT_STREQ(p.ToString().c_str(), "-1 X");
    p = PauliString("y");
    z.Apply(p);
    EXPECT_STREQ(p.ToString().c_str(), "-1 Y");
    p = PauliString("z");
    z.Apply(p);
    EXPECT_STREQ(p.ToString().c_str(), "+1 Z");
}
TEST(StabilizerTableau, H) {
    const auto h = StabilizerTableau::H(1, 0);

    auto p = PauliString("i");

    h.Apply(p);
    EXPECT_STREQ(p.ToString().c_str(), "+1 I");
    p = PauliString("x");
    h.Apply(p);
    EXPECT_STREQ(p.ToString().c_str(), "+1 Z");
    p = PauliString("y");
    h.Apply(p);
    EXPECT_STREQ(p.ToString().c_str(), "-1 Y");
    p = PauliString("z");
    h.Apply(p);
    EXPECT_STREQ(p.ToString().c_str(), "+1 X");
}
TEST(StabilizerTableau, S) {
    const auto s = StabilizerTableau::S(1, 0);

    auto p = PauliString("i");

    s.Apply(p);
    EXPECT_STREQ(p.ToString().c_str(), "+1 I");
    p = PauliString("x");
    s.Apply(p);
    EXPECT_STREQ(p.ToString().c_str(), "+1 Y");
    p = PauliString("y");
    s.Apply(p);
    EXPECT_STREQ(p.ToString().c_str(), "-1 X");
    p = PauliString("z");
    s.Apply(p);
    EXPECT_STREQ(p.ToString().c_str(), "+1 Z");
}
TEST(StabilizerTableau, SDag) {
    const auto sdag = StabilizerTableau::SDag(1, 0);

    auto p = PauliString("i");

    sdag.Apply(p);
    EXPECT_STREQ(p.ToString().c_str(), "+1 I");
    p = PauliString("x");
    sdag.Apply(p);
    EXPECT_STREQ(p.ToString().c_str(), "-1 Y");
    p = PauliString("y");
    sdag.Apply(p);
    EXPECT_STREQ(p.ToString().c_str(), "+1 X");
    p = PauliString("z");
    sdag.Apply(p);
    EXPECT_STREQ(p.ToString().c_str(), "+1 Z");
}
TEST(StabilizerTableau, CX) {
    const auto pauli_pattern = {'I', 'X', 'Z', 'Y'};

    const auto cx = StabilizerTableau::CX(2, 0, 1);

    const auto h = StabilizerTableau::H(2, 0);
    const auto cz = StabilizerTableau::CZ(2, 0, 1);

    for (const auto p : pauli_pattern) {
        for (const auto q : pauli_pattern) {
            const auto pauli = PauliString(std::string{p, q});

            // Case 1: Apply CX
            auto pauli1 = pauli;
            cx.Apply(pauli1);

            // Case 2: Apply H + CZ + H
            auto pauli2 = pauli;
            h.Apply(pauli2);
            cz.Apply(pauli2);
            h.Apply(pauli2);

            EXPECT_EQ(pauli1, pauli2) << fmt::format(
                    "pauli1={} (CX), pauli2={} (H + CZ + H) (before: {})",
                    pauli1.ToString(),
                    pauli2.ToString(),
                    pauli.ToString()
            );
        }
    }
}
TEST(StabilizerTableau, CY) {
    const auto pauli_pattern = {'I', 'X', 'Z', 'Y'};

    const auto cy = StabilizerTableau::CY(2, 0, 1);

    const auto s = StabilizerTableau::S(2, 0);
    const auto sdag = StabilizerTableau::SDag(2, 0);
    const auto cx = StabilizerTableau::CX(2, 0, 1);

    for (const auto p : pauli_pattern) {
        for (const auto q : pauli_pattern) {
            const auto pauli = PauliString(std::string{p, q});

            // Case 1: Apply CY
            auto pauli1 = pauli;
            cy.Apply(pauli1);

            // Case 2: Apply S + CX + SDag
            auto pauli2 = pauli;
            s.Apply(pauli2);
            cx.Apply(pauli2);
            sdag.Apply(pauli2);

            EXPECT_STREQ(pauli1.ToString().c_str(), pauli2.ToString().c_str()) << fmt::format(
                    "pauli1={} (CY), pauli2={} (S + CX + SDag) (before: {})",
                    pauli1.ToString(),
                    pauli2.ToString(),
                    pauli.ToString()
            );
        }
    }
}
