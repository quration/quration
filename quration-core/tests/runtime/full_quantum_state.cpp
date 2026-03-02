#include "qret/runtime/full_quantum_state.h"

#include <gtest/gtest.h>

#include <cmath>
#include <complex>
#include <numbers>
#include <vector>

#include "qret/runtime/quantum_state.h"

using namespace qret;

using qret::runtime::FullQuantumState;
using qret::runtime::QuantumState;
static constexpr auto Pi = std::numbers::pi;

using namespace qret;

class FullQuantumStateTest : public ::testing::Test {
protected:
    std::uint64_t seed_ = 12345;

    // Helper to verify complex numbers
    bool IsClose(std::complex<double> actual, std::complex<double> expected, double tol = 1e-9) {
        return std::abs(actual - expected) < tol;
    }

    // Helper to get element from column-major raw pointer
    // dim: dimension of the matrix (2^num_qubits)
    std::complex<double> GetMatrixElement(
            const std::complex<double>* ptr,
            std::size_t dim,
            std::size_t row,
            std::size_t col
    ) {
        // The operation matrix pointer is stored in column-major order.
        return ptr[(col * dim) + row];
    }
};

TEST_F(FullQuantumStateTest, InitializationState) {
    // 1 qubit, save_operation_matrix = false
    auto state = FullQuantumState(seed_, 1, false, false);

    // Initial state |0>, so Prob(1) = 0
    EXPECT_NEAR(state.Calc1Prob(0), 0.0, QuantumState::Eps);

    // Superposition check
    EXPECT_FALSE(state.IsSuperposed(0));
}

TEST_F(FullQuantumStateTest, GateOperationsXHZ) {
    auto state = FullQuantumState(seed_, 1, false, false);

    // X: |0> -> |1>
    state.X(0);
    EXPECT_NEAR(state.Calc1Prob(0), 1.0, QuantumState::Eps);

    // X: |1> -> |0>
    state.X(0);
    EXPECT_NEAR(state.Calc1Prob(0), 0.0, QuantumState::Eps);

    // H: |0> -> |+>
    state.H(0);
    EXPECT_NEAR(state.Calc1Prob(0), 0.5, QuantumState::Eps);
    EXPECT_TRUE(state.IsSuperposed(0));

    // Z: |+> -> |-> (Probabilities don't change)
    state.Z(0);
    EXPECT_NEAR(state.Calc1Prob(0), 0.5, QuantumState::Eps);

    // H: |-> -> |1>
    state.H(0);
    EXPECT_NEAR(state.Calc1Prob(0), 1.0, QuantumState::Eps);
}

TEST_F(FullQuantumStateTest, EntanglementBellState) {
    // 2 qubits
    auto state = FullQuantumState(seed_, 2, false, false);

    // Create Bell pair |Phi+> = (|00> + |11>) / sqrt(2)
    state.H(0);
    state.CX(1, 0);  // Target 1, Control 0

    // Check marginal probabilities
    EXPECT_NEAR(state.Calc1Prob(0), 0.5, QuantumState::Eps);
    EXPECT_NEAR(state.Calc1Prob(1), 0.5, QuantumState::Eps);

    // Measure q0 -> r0
    state.Measure(0, 0);
    bool r0 = state.ReadRegister(0);

    // Measure q1 -> r1
    state.Measure(1, 1);
    bool r1 = state.ReadRegister(1);

    // Should be correlated
    EXPECT_EQ(r0, r1);
}

TEST_F(FullQuantumStateTest, OperationMatrixIdentity) {
    // 1 qubit, enable matrix saving
    auto state = FullQuantumState(seed_, 1, false, true);

    const auto* mat = state.GetOperationMatrix();
    ASSERT_NE(mat, nullptr);

    // Initial matrix should be Identity (2x2)
    // Row 0, Col 0 -> 1
    EXPECT_TRUE(IsClose(GetMatrixElement(mat, 2, 0, 0), 1.0));
    // Row 1, Col 0 -> 0
    EXPECT_TRUE(IsClose(GetMatrixElement(mat, 2, 1, 0), 0.0));
    // Row 0, Col 1 -> 0
    EXPECT_TRUE(IsClose(GetMatrixElement(mat, 2, 0, 1), 0.0));
    // Row 1, Col 1 -> 1
    EXPECT_TRUE(IsClose(GetMatrixElement(mat, 2, 1, 1), 1.0));
}

TEST_F(FullQuantumStateTest, OperationMatrixXGate) {
    // 1 qubit, enable matrix saving
    auto state = FullQuantumState(seed_, 1, false, true);

    // Apply X
    state.X(0);

    const auto* mat = state.GetOperationMatrix();

    // Expected: [0 1; 1 0]
    // (0,0) -> 0
    EXPECT_TRUE(IsClose(GetMatrixElement(mat, 2, 0, 0), 0.0));
    // (1,0) -> 1
    EXPECT_TRUE(IsClose(GetMatrixElement(mat, 2, 1, 0), 1.0));
    // (0,1) -> 1
    EXPECT_TRUE(IsClose(GetMatrixElement(mat, 2, 0, 1), 1.0));
    // (1,1) -> 0
    EXPECT_TRUE(IsClose(GetMatrixElement(mat, 2, 1, 1), 0.0));
}

TEST_F(FullQuantumStateTest, OperationMatrixHAndPhase) {
    // 1 qubit, enable matrix saving
    auto state = FullQuantumState(seed_, 1, false, true);
    double inv_sqrt2 = 1.0 / std::numbers::sqrt2;

    state.H(0);
    // H = 1/sqrt(2) * [1 1; 1 -1]
    const auto* matH = state.GetOperationMatrix();
    EXPECT_TRUE(IsClose(GetMatrixElement(matH, 2, 0, 0), inv_sqrt2));
    EXPECT_TRUE(IsClose(GetMatrixElement(matH, 2, 1, 1), -inv_sqrt2));

    state.S(0);
    // S = [1 0; 0 i]
    // S * H = 1/sqrt(2) * [1 1; i -i]
    const auto* matSH = state.GetOperationMatrix();

    // (0,0) = 1 * inv_sqrt2
    EXPECT_TRUE(IsClose(GetMatrixElement(matSH, 2, 0, 0), inv_sqrt2));
    // (1,0) = i * inv_sqrt2
    EXPECT_TRUE(IsClose(GetMatrixElement(matSH, 2, 1, 0), std::complex<double>(0, inv_sqrt2)));
}

TEST_F(FullQuantumStateTest, OperationMatrixTwoQubitsCX) {
    // 2 qubits, enable matrix saving
    auto state = FullQuantumState(seed_, 2, false, true);
    std::size_t dim = 4;  // 2^2

    // Apply CX(target=1, control=0)
    // Assuming q0 is LSB.
    // Basis: |00>(0), |01>(1), |10>(2), |11>(3) where |q1 q0>
    // Control q0=1 (indices 1 and 3). Target q1 flips.
    // |01> (q1=0, q0=1) -> |11> (q1=1, q0=1) => 1 -> 3
    // |11> (q1=1, q0=1) -> |01> (q1=0, q0=1) => 3 -> 1
    // Others map to themselves.
    state.CX(1, 0);

    const auto* mat = state.GetOperationMatrix();

    // Check mapping logic via matrix columns

    // Col 0 (|00>) -> Row 0
    EXPECT_TRUE(IsClose(GetMatrixElement(mat, dim, 0, 0), 1.0));

    // Col 1 (|01>) -> Row 3 (|11>)
    EXPECT_TRUE(IsClose(GetMatrixElement(mat, dim, 3, 1), 1.0));
    EXPECT_TRUE(IsClose(GetMatrixElement(mat, dim, 1, 1), 0.0));

    // Col 2 (|10>) -> Row 2
    EXPECT_TRUE(IsClose(GetMatrixElement(mat, dim, 2, 2), 1.0));

    // Col 3 (|11>) -> Row 1 (|01>)
    EXPECT_TRUE(IsClose(GetMatrixElement(mat, dim, 1, 3), 1.0));
    EXPECT_TRUE(IsClose(GetMatrixElement(mat, dim, 3, 3), 0.0));
}

TEST_F(FullQuantumStateTest, Registers) {
    auto state = FullQuantumState(seed_, 4, false, false);

    // Test manual register write/read
    state.WriteRegister(0, true);
    EXPECT_TRUE(state.ReadRegister(0));
    state.WriteRegister(0, false);
    EXPECT_FALSE(state.ReadRegister(0));

    // Test bulk write
    // 5 (binary 101) -> r10=1, r11=0, r12=1
    qret::BigInt val = 5;
    state.WriteRegisters(10, 3, val);

    auto regs = state.ReadRegisters(10, 3);
    ASSERT_EQ(regs.size(), 3);
    EXPECT_EQ(regs[0], true);
    EXPECT_EQ(regs[1], false);
    EXPECT_EQ(regs[2], true);
}

TEST_F(FullQuantumStateTest, GlobalPhase) {
    auto state = FullQuantumState(seed_, 1, false, false);

    // Rotation shouldn't affect probability magnitude
    state.RotateGlobalPhase(Pi / 2.0);
    EXPECT_NEAR(state.Calc1Prob(0), 0.0, QuantumState::Eps);

    state.H(0);
    state.RotateGlobalPhase(Pi);
    EXPECT_NEAR(state.Calc1Prob(0), 0.5, QuantumState::Eps);
}
