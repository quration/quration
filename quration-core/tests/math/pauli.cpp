#include "qret/math/pauli.h"

#include <gtest/gtest.h>

#include <variant>

using namespace qret::math;

TEST(PauliStringTest, ConstructionAndToString) {
    PauliString p0("XYZI");
    EXPECT_EQ(p0.GetNumQubits(), 4);
    EXPECT_EQ(p0.GetSign(), 0);
    EXPECT_STREQ(p0.ToString().c_str(), "+1 XYZI");

    PauliString p1("YZXI", 1);  // sign = i
    EXPECT_EQ(p1.GetSign(), 1);
    EXPECT_STREQ(p1.ToString().c_str(), "+i YZXI");
}

TEST(PauliStringTest, SingleQubitMultiplicationAllCases) {
    // Pauli multiplication phase table:
    //
    //     | I  X  Y  Z
    //   -----------------
    //  I | 0  0  0  0
    //  X | 0  0  1  1   (X*Y= iZ, X*Z=-iY)
    //  Y | 0  3  0  1   (Y*X=-iZ, Y*Z= iX)
    //  Z | 0  3  3  0   (Z*X= iY, Z*Y=-iX)
    //
    // 0: 1, 1: i, 2: -1, 3: -i

    const char* paulis = "IXYZ";
    const char* expected_out[4][4] = {
            {"+1 I", "+1 X", "+1 Y", "+1 Z"},  // I * (IXYZ)
            {"+1 X", "+1 I", "+i Z", "-i Y"},  // X * (IXYZ)
            {"+1 Y", "-i Z", "+1 I", "+i X"},  // Y * (IXYZ)
            {"+1 Z", "+i Y", "-i X", "+1 I"},  // Z * (IXYZ)
    };

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            auto p1 = PauliString(std::string(1, paulis[i]));
            auto p2 = PauliString(std::string(1, paulis[j]));
            auto result = p1 * p2;

            SCOPED_TRACE(
                    "Single-qubit multiplication: " + std::string(1, paulis[i]) + "*"
                    + std::string(1, paulis[j])
            );

            EXPECT_STREQ(result.ToString().c_str(), expected_out[i][j]);
        }
    }
}

TEST(PauliStringTest, SignAndInverse) {
    PauliString p1("Y", 3);  // -i Y
    EXPECT_EQ(p1.GetSign(), 3);
    EXPECT_STREQ(p1.ToString().c_str(), "-i Y");

    // Inverse flips the sign mod 4
    p1.Inverse();
    EXPECT_EQ(p1.GetSign(), 1);  // i Y
    EXPECT_STREQ(p1.ToString().c_str(), "+i Y");
}

TEST(PauliStringTest, MultiplicationMultiQubitAndPhase) {
    PauliString p1("XIZ");
    PauliString p2("IXZ", 1);  // sign = i
    PauliString result = p1 * p2;
    EXPECT_EQ(result.GetSign(), 1);
    EXPECT_STREQ(result.ToString().c_str(), "+i XXI");

    // Identity string should not affect result
    PauliString id("III");
    result = p1 * id;
    EXPECT_EQ(result.GetSign(), 0);
    EXPECT_STREQ(result.ToString().c_str(), "+1 XIZ");
}

TEST(PauliStringTest, MultiplicationPhaseAccumulation) {
    PauliString x1("X");
    PauliString x2("X");
    PauliString result = x1 * x2;
    EXPECT_EQ(result.GetSign(), 0);
    EXPECT_STREQ(result.ToString().c_str(), "+1 I");

    // (X * Z) * Y = (-i Y) * Y = -i (Y * Y) = -i I
    PauliString z("Z");
    PauliString y("Y");
    result = ((x1 * z) * y);
    EXPECT_EQ(result.GetSign(), 3);
    EXPECT_STREQ(result.ToString().c_str(), "-i I");
}

TEST(PauliStringTest, CommutationSingleQubit) {
    PauliString x1("X"), x2("X");
    PauliString z1("Z"), z2("Z");
    PauliString y("Y");

    EXPECT_TRUE(x1.IsCommuteWith(x2));  // [X, X] = 0
    EXPECT_TRUE(z1.IsCommuteWith(z2));  // [Z, Z] = 0
    EXPECT_FALSE(x1.IsCommuteWith(z1));  // [X, Z] != 0
    EXPECT_FALSE(z1.IsCommuteWith(x1));  // [Z, X] != 0

    EXPECT_FALSE(y.IsCommuteWith(x1));  // [Y, X] != 0
    EXPECT_FALSE(y.IsCommuteWith(z1));  // [Y, Z] != 0
    EXPECT_TRUE(y.IsCommuteWith(y));  // [Y, Y] = 0
}

TEST(PauliStringTest, CommutationMultiQubit) {
    PauliString xx("XX");
    PauliString zz("ZZ");
    EXPECT_TRUE(xx.IsCommuteWith(zz));  // Even anti-commute positions: commute

    // XZ and ZX (commute: even anti-commute positions)
    PauliString xz("XZ");
    PauliString zx("ZX");
    EXPECT_TRUE(xz.IsCommuteWith(zx));

    // XI and IX (commute)
    PauliString xi("XI");
    PauliString ix("IX");
    EXPECT_TRUE(xi.IsCommuteWith(ix));
}

TEST(PauliStringTest, InvalidInputThrows) {
    EXPECT_THROW(PauliString("AXZ"), std::runtime_error);
    EXPECT_THROW(PauliString("qzi"), std::runtime_error);
}

TEST(PauliStringTest, IdentityString) {
    PauliString id("III");
    PauliString p("XIZ");
    auto res = id * p;
    EXPECT_STREQ(res.ToString().c_str(), "+1 XIZ");
    res = p * id;
    EXPECT_STREQ(res.ToString().c_str(), "+1 XIZ");
}

TEST(PauliStringTest, SelfMultiplicationIsIdentity) {
    PauliString p1("XYZ");
    PauliString res = p1 * p1;
    EXPECT_STREQ(res.ToString().c_str(), "+1 III");
}

TEST(PauliStringTest, LongRandomPauliMultiplication) {
    // This checks basic algebraic properties even for long strings.
    PauliString a("XYZXYZXYZX");
    PauliString b("YZXZXYXZXY");
    PauliString c("ZXYYZZXIXI");

    // Check (A*B)*C == A*(B*C)
    PauliString lhs = (a * b) * c;
    PauliString rhs = a * (b * c);
    EXPECT_EQ(lhs.GetSign(), rhs.GetSign());
    EXPECT_STREQ(lhs.ToString().c_str(), rhs.ToString().c_str());

    // Check self-inverse: (A * A).Inverse() == I
    PauliString self_inv = a * a;
    self_inv.Inverse();
    EXPECT_EQ(self_inv.ToString().substr(self_inv.ToString().find_first_of("IXYZ")), "IIIIIIIIII");
    EXPECT_EQ(self_inv.GetSign(), 0);
}

TEST(PauliCircuitTest, ToStandardFormat) {
    // Following tests are based on figure 4 of A Game of Surface Codes: Large-Scale Quantum
    // Computing with Lattice Surgery: https://quantum-journal.org/papers/q-2019-03-05-128/

    static constexpr auto pi_over_4 = PauliRotation::Theta::pi_over_4;
    static constexpr auto pi_over_8 = PauliRotation::Theta::pi_over_8;

    auto circuit = PauliCircuit({
            // 1
            PauliRotation(pi_over_8, "ZIII"),
            // 2
            PauliRotation(pi_over_4, "IXZI"),
            PauliRotation(pi_over_4, "IXII", 2),
            PauliRotation(pi_over_4, "IIZI", 2),
            // 3
            PauliRotation(pi_over_4, "IIIX", 2),
            // 4
            PauliRotation(pi_over_4, "XZII"),
            PauliRotation(pi_over_4, "XIII", 2),
            PauliRotation(pi_over_4, "IZII", 2),
            // 5
            PauliRotation(pi_over_4, "IIXI"),
            // 6
            PauliRotation(pi_over_8, "IIIZ"),
            // 7
            PauliRotation(pi_over_4, "XIIZ"),
            PauliRotation(pi_over_4, "XIII", 2),
            PauliRotation(pi_over_4, "IIIZ", 2),
            // 10
            PauliRotation(pi_over_8, "IIZI"),
            // 8
            PauliRotation(pi_over_8, "ZIII"),
            // 9
            PauliRotation(pi_over_4, "IZII"),
            // 11
            PauliRotation(pi_over_4, "IIIZ"),
            // 12
            PauliRotation(pi_over_4, "XIII", 2),
            // 13
            PauliRotation(pi_over_4, "IXII"),
            // 14
            PauliRotation(pi_over_4, "IIXI"),
            // 15
            PauliRotation(pi_over_4, "IIIX"),
    });
    // Add measurements
    for (auto i = std::size_t{0}; i < 4; ++i) {
        auto pauli = std::string("IIII");
        pauli[i] = 'Z';
        circuit.EmplaceBack(PauliMeasurement(pauli));
    }

    EXPECT_TRUE(circuit.IsValid());

    // std::cout << "Before:" << std::endl;
    // for (const auto& x : circuit) { std::cout << "  " << x.ToString() << std::endl; }

    circuit.ToStandardFormat();

    EXPECT_TRUE(circuit.IsValid());
    EXPECT_TRUE(circuit.IsStandardFormat());

    using Variant = std::variant<PauliRotation, PauliMeasurement>;
    auto itr = circuit.begin();
    EXPECT_EQ(Variant(PauliRotation(pi_over_8, "ZIII")), (itr++)->value);
    EXPECT_EQ(Variant(PauliRotation(pi_over_8, "IIIY", 2)), (itr++)->value);
    EXPECT_EQ(Variant(PauliRotation(pi_over_8, "IXYI")), (itr++)->value);
    EXPECT_EQ(Variant(PauliRotation(pi_over_8, "ZZZY", 2)), (itr++)->value);
    EXPECT_EQ(Variant(PauliMeasurement("YZZY")), (itr++)->value);
    EXPECT_EQ(Variant(PauliMeasurement("XXII")), (itr++)->value);
    EXPECT_EQ(Variant(PauliMeasurement("IIZI", 2)), (itr++)->value);
    EXPECT_EQ(Variant(PauliMeasurement("XIIX")), (itr++)->value);
    EXPECT_EQ(itr, circuit.end());

    // std::cout << "Standard form:" << std::endl;
    // for (const auto& x : circuit) { std::cout << "  " << x.ToString() << std::endl; }
}
