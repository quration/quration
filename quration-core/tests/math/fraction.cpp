#include "qret/math/fraction.h"

#include <gtest/gtest.h>

#include "qret/base/type.h"

using namespace qret;
using namespace qret::math;

TEST(ToContinuedFraction, Basic) {
    EXPECT_EQ((std::vector<BigInt>{0}), ToContinuedFraction(BigFraction(0)).a);
    EXPECT_EQ((std::vector<BigInt>{2}), ToContinuedFraction(BigFraction(2)).a);
    EXPECT_EQ((std::vector<BigInt>{-2}), ToContinuedFraction(BigFraction(-2)).a);
    EXPECT_EQ((std::vector<BigInt>{0, 2}), ToContinuedFraction(BigFraction(1, 2)).a);
    EXPECT_EQ((std::vector<BigInt>{1, 2, 2}), ToContinuedFraction(BigFraction(7, 5)).a);
    EXPECT_EQ((std::vector<BigInt>{0, 2, 3}), ToContinuedFraction(BigFraction(3, 7)).a);
    EXPECT_EQ((std::vector<BigInt>{-10, 1, 2}), ToContinuedFraction(BigFraction(-28, 3)).a);
    EXPECT_EQ((std::vector<BigInt>{0, 5, 1, 84, 2}), ToContinuedFraction(BigFraction(171, 1024)).a);
    EXPECT_EQ(
            (std::vector<BigInt>{4, 5, 1, 84, 2}),
            ToContinuedFraction(BigFraction(171 + (4 * 1024), 1024)).a
    );
    EXPECT_EQ(
            (std::vector<BigInt>{-1, 5, 1, 84, 2}),
            ToContinuedFraction(BigFraction(-(1024 - 171), 1024)).a
    );
}
TEST(FromContinuedFraction, Basic) {
    EXPECT_EQ(BigFraction(0), FromContinuedFraction(std::vector<BigInt>{0}));
    EXPECT_EQ(BigFraction(2), FromContinuedFraction(std::vector<BigInt>{2}));
    EXPECT_EQ(BigFraction(-2), FromContinuedFraction(std::vector<BigInt>{-2}));
    EXPECT_EQ(BigFraction(1, 2), FromContinuedFraction(std::vector<BigInt>{0, 2}));
    EXPECT_EQ(BigFraction(7, 5), FromContinuedFraction(std::vector<BigInt>{1, 2, 2}));
    EXPECT_EQ(BigFraction(3, 7), FromContinuedFraction(std::vector<BigInt>{0, 2, 3}));
    EXPECT_EQ(BigFraction(-28, 3), FromContinuedFraction(std::vector<BigInt>{-10, 1, 2}));
    EXPECT_EQ(BigFraction(171, 1024), FromContinuedFraction(std::vector<BigInt>{0, 5, 1, 84, 2}));
    EXPECT_EQ(
            BigFraction(171 + (4 * 1024), 1024),
            FromContinuedFraction(std::vector<BigInt>{4, 5, 1, 84, 2})
    );
    EXPECT_EQ(
            BigFraction(-(1024 - 171), 1024),
            FromContinuedFraction(std::vector<BigInt>{-1, 5, 1, 84, 2})
    );
}
TEST(FromContinuedFraction, InvalidElement) {
    EXPECT_THROW(FromContinuedFraction(std::vector<BigInt>{-3, 0}), std::runtime_error);
    EXPECT_THROW(FromContinuedFraction(std::vector<BigInt>{3, 4, 0}), std::runtime_error);
    EXPECT_THROW(FromContinuedFraction(std::vector<BigInt>{3, 0, 4}), std::runtime_error);
    EXPECT_THROW(FromContinuedFraction(std::vector<BigInt>{0, 0}), std::runtime_error);
}
