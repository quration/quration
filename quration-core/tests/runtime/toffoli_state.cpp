#include "qret/runtime/toffoli_state.h"

#include <gtest/gtest.h>

#include <ranges>
#include <vector>

struct ToffoliStateTest : public ::testing::TestWithParam<std::uint64_t> {
    void SetUp() override {
        // Get the seed value from the fixture's parameters
        seed = GetParam();
        num_qubits = 10;
        eps = 1e-10;
        state = std::make_unique<qret::runtime::ToffoliState>(seed, num_qubits, eps);

        // Apply H and MCX gates after ToffoliState initialization
        std::vector<std::uint64_t> control_qubits;
        for (const auto q : std::views::iota(std::uint64_t{0}, num_qubits / 2)) {
            control_qubits.emplace_back(q);
        }
        for (const auto q : control_qubits) {
            state->H(q);
        }
        state->MCX(num_qubits - 1, control_qubits);
    }

    void TearDown() override {}

    std::uint64_t seed;
    std::uint64_t num_qubits;
    double eps;
    std::unique_ptr<qret::runtime::ToffoliState> state;  // Manage with a smart pointer
};

TEST_P(ToffoliStateTest, Measurement) {
    ASSERT_GE(num_qubits, 4);  // Ensure NumQubits >= 4

    EXPECT_FALSE(state->IsComputationalBasis());
    EXPECT_EQ(std::uint64_t{1} << (num_qubits / 2), state->GetNumSuperpositions());

    EXPECT_TRUE(state->IsSuperposed(0));
    EXPECT_FALSE(state->IsClean(0));
    EXPECT_NEAR(0.5, state->Calc0Prob(0), eps);
    EXPECT_NEAR(0.5, state->Calc1Prob(0), eps);

    EXPECT_FALSE(state->IsSuperposed(num_qubits - 2));
    EXPECT_TRUE(state->IsClean(num_qubits - 2));
    EXPECT_NEAR(1, state->Calc0Prob(num_qubits - 2), eps);
    EXPECT_NEAR(0, state->Calc1Prob(num_qubits - 2), eps);

    EXPECT_TRUE(state->IsSuperposed(num_qubits - 1));
    EXPECT_FALSE(state->IsClean(num_qubits - 1));
    EXPECT_NEAR(1 - std::pow(0.5, num_qubits / 2), state->Calc0Prob(num_qubits - 1), eps);
    EXPECT_NEAR(std::pow(0.5, num_qubits / 2), state->Calc1Prob(num_qubits - 1), eps);

    state->Measure(num_qubits - 1, 10);

    if (state->ReadRegister(10)) {
        ASSERT_EQ(1, state->GetNumSuperpositions());
        EXPECT_NEAR(1.0, state->GetPhase().real(), eps);
        EXPECT_NEAR(0.0, state->GetPhase().imag(), eps);
    } else {
        EXPECT_EQ((std::uint64_t{1} << (num_qubits / 2)) - 1, state->GetNumSuperpositions());
        for (const auto& [coeff, _] : state->GetRawVector()) {
            EXPECT_NEAR(1 / (std::pow(2.0, num_qubits / 2) - 1.0), std::norm(coeff), eps);
        }
    }
}

INSTANTIATE_TEST_SUITE_P(
        ToffoliStateMeasurementWithMultipleSeeds,
        ToffoliStateTest,
        ::testing::Range(std::uint64_t{0}, std::uint64_t{100})
);
