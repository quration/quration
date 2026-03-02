#include "qret/base/gridsynth.h"

#include <gtest/gtest.h>

#include <numbers>

using namespace qret;

TEST(ApproximateRzWithCliffordT, SimpleCase) {
    if (!IsGridSynthRunnable()) {
        GTEST_SKIP() << "Skip GridSynth test";
    }

    using std::numbers::pi;
    const auto angles = std::vector<std::pair<double, std::string>>{
            {pi, "SS"},
            {pi / 2, "S"},
            {pi / 4, "T"},
            {0, "I"},
            {-pi / 4, "TSSS"},
            {-pi / 2, "SSS"},
            {-pi, "SS"}
    };
    for (const auto& [theta, answer] : angles) {
        auto output = std::string();
        auto exit = std::int32_t();
        ApproximateRzWithCliffordT(theta, output, exit, 10u, true);
        EXPECT_EQ(0, exit);
        EXPECT_EQ(answer, output);
    }
}
