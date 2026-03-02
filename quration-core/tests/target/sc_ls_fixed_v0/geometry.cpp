#include "qret/target/sc_ls_fixed_v0/geometry.h"

#include <gtest/gtest.h>

#include <unordered_map>

using namespace qret::sc_ls_fixed_v0;

TEST(Vec2D, Hashable) {
    using T = Vec2D<std::int32_t>;
    auto mp = std::unordered_map<T, std::uint64_t>();
    mp[T{-1, 1}] = 10;
    mp[T{-1, 2}] = 20;
    EXPECT_EQ(10, mp.at(T{-1, 1}));
    EXPECT_EQ(20, mp.at(T{-1, 2}));
    EXPECT_FALSE(mp.contains(T{10, 10}));
}
TEST(Vec3D, Hashable) {
    using T = Vec3D<std::int32_t>;
    auto mp = std::unordered_map<T, std::uint64_t>();
    mp[T{-1, 1, 0}] = 10;
    mp[T{-1, 2, 0}] = 20;
    EXPECT_EQ(10, mp.at(T{-1, 1, 0}));
    EXPECT_EQ(20, mp.at(T{-1, 2, 0}));
    EXPECT_FALSE(mp.contains(T{10, 10, 0}));
}
