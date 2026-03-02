#include <gtest/gtest.h>

#include <stdexcept>

#include "qret/target/sc_ls_fixed_v0/calc_compile_info.h"

using namespace qret;
using namespace qret::sc_ls_fixed_v0;

TEST(QecResourceEstimation, EstimateMinimumCodeDistance) {
    const auto d = CompileInfoWithQecResourceEstimation::EstimateMinimumCodeDistance(
            1e-3,
            0.1,
            1e-6,
            1000
    );
    EXPECT_EQ(d, 13UL);
}

TEST(QecResourceEstimation, EstimateExecutionTimeSec) {
    const auto sec = CompileInfoWithQecResourceEstimation::EstimateExecutionTimeSec(13, 100, 1e-6);
    EXPECT_DOUBLE_EQ(sec, 0.0013);
}

TEST(QecResourceEstimation, EstimatePhysicalQubitCount) {
    const auto count = CompileInfoWithQecResourceEstimation::EstimatePhysicalQubitCount(3, 5);
    EXPECT_EQ(count, 90UL);
}

TEST(QecResourceEstimation, EstimateMinimumCodeDistanceRejectsInvalidLambda) {
    EXPECT_THROW(
            CompileInfoWithQecResourceEstimation::EstimateMinimumCodeDistance(
                    1e-3,
                    1.0,
                    1e-6,
                    1000
            ),
            std::runtime_error
    );
}
