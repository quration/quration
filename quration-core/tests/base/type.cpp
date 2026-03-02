#include "qret/base/type.h"

#include <gtest/gtest.h>

using namespace qret;

TEST(Convert, Int) {
    EXPECT_EQ((std::vector<bool>{1, 0, 0, 1}), IntAsBoolArray(9));
    EXPECT_EQ((std::vector<bool>{1, 0, 0, 1, 0, 0, 0}), IntAsBoolArray(9, 7));
    EXPECT_EQ((std::vector<bool>{0, 0, 1, 0, 0, 1, 1}), IntAsBoolArray(100));
    EXPECT_EQ(9, BoolArrayAsInt(std::vector<bool>{1, 0, 0, 1}));
    EXPECT_EQ(100, BoolArrayAsInt(std::vector<bool>{0, 0, 1, 0, 0, 1, 1}));
    for (auto x : {0, 1, 10, 100, 1000}) {
        EXPECT_EQ(x, BoolArrayAsInt(IntAsBoolArray(x)));
    }
}
TEST(Convert, BigInt) {
    EXPECT_EQ(BigInt(9), IntAsBigInt(9));
    EXPECT_EQ((std::vector<bool>{1, 0, 0, 1}), BigIntAsBoolArray(BigInt(9)));
    EXPECT_EQ((std::vector<bool>{0, 0, 1, 0, 0, 1, 1}), BigIntAsBoolArray(BigInt(100)));
    EXPECT_EQ(9, BoolArrayAsBigInt(std::vector<bool>{1, 0, 0, 1}));
    EXPECT_EQ(100, BoolArrayAsBigInt(std::vector<bool>{0, 0, 1, 0, 0, 1, 1}));
    for (auto x : {0, 1, 10, 100, 1000}) {
        EXPECT_EQ(x, BoolArrayAsBigInt(BigIntAsBoolArray(BigInt(x))));
    }
}
TEST(Formatter, ostream) {
    EXPECT_EQ("827813249754334971", fmt::format("{}", BigInt("827813249754334971")));
    EXPECT_EQ("-827813249754334971", fmt::format("{}", BigInt("-827813249754334971")));
    EXPECT_EQ(
            "827813249754334971/237451985537",
            fmt::format("{}", BigFraction(BigInt("827813249754334971"), BigInt("237451985537")))
    );
    EXPECT_EQ(
            "-827813249754334971/237451985537",
            fmt::format("{}", BigFraction(BigInt("-827813249754334971"), BigInt("237451985537")))
    );
}
