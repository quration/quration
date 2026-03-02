#include "qret/base/list.h"

#include <gtest/gtest.h>

#include <list>

using namespace qret;

namespace {
// Helper to convert a list to a vector for easy comparison in assertions
template <typename T, typename Alloc>
std::vector<T> ToVector(const std::list<T, Alloc>& l) {
    return std::vector<T>(l.begin(), l.end());
}
}  // namespace

TEST(SwapListNodesTest, SwapSameIteratorNoChange) {
    std::list<int> l = {1, 2, 3};
    auto it = std::next(l.begin());
    SwapListNodes(l, it, it);  // Swapping same iterator should do nothing
    EXPECT_EQ(ToVector(l), (std::vector<int>{1, 2, 3}));
}

TEST(SwapListNodesTest, SwapAdjacentLhsBeforeRhs) {
    std::list<int> l = {1, 2, 3};
    auto lhs = l.begin();  // 1
    auto rhs = std::next(lhs);  // 2
    SwapListNodes(l, lhs, rhs);  // Should swap 1 and 2
    EXPECT_EQ(ToVector(l), (std::vector<int>{2, 1, 3}));
    EXPECT_EQ(*lhs, 1);
    EXPECT_EQ(*std::next(lhs), 3);
    EXPECT_EQ(*std::prev(lhs), 2);
    EXPECT_EQ(*rhs, 2);
    EXPECT_EQ(*std::next(rhs), 1);
    EXPECT_EQ(rhs, l.begin());
}

TEST(SwapListNodesTest, SwapAdjacentRhsBeforeLhs) {
    std::list<int> l = {1, 2, 3};
    auto rhs = l.begin();  // 1
    auto lhs = std::next(rhs);  // 2
    SwapListNodes(l, lhs, rhs);  // Should swap 2 and 1
    EXPECT_EQ(ToVector(l), (std::vector<int>{2, 1, 3}));
    EXPECT_EQ(*lhs, 2);
    EXPECT_EQ(*std::next(lhs), 1);
    EXPECT_EQ(lhs, l.begin());
    EXPECT_EQ(*rhs, 1);
    EXPECT_EQ(*std::next(rhs), 3);
    EXPECT_EQ(*std::prev(rhs), 2);
}

TEST(SwapListNodesTest, SwapNonAdjacentMiddleAndEnd) {
    std::list<int> l = {1, 2, 3, 4};
    auto lhs = std::next(l.begin());  // 2
    auto rhs = std::prev(l.end());  // 4
    SwapListNodes(l, lhs, rhs);  // Swap 2 and 4
    EXPECT_EQ(ToVector(l), (std::vector<int>{1, 4, 3, 2}));
    EXPECT_EQ(*lhs, 2);
    EXPECT_EQ(std::next(lhs), l.end());
    EXPECT_EQ(*std::prev(lhs), 3);
    EXPECT_EQ(*rhs, 4);
    EXPECT_EQ(*std::next(rhs), 3);
    EXPECT_EQ(*std::prev(rhs), 1);
}

TEST(SwapListNodesTest, SwapFirstAndLast) {
    std::list<int> l = {1, 2, 3, 4};
    auto lhs = l.begin();  // 1
    auto rhs = std::prev(l.end());  // 4
    SwapListNodes(l, lhs, rhs);
    EXPECT_EQ(ToVector(l), (std::vector<int>{4, 2, 3, 1}));
    EXPECT_EQ(*lhs, 1);
    EXPECT_EQ(std::next(lhs), l.end());
    EXPECT_EQ(*std::prev(lhs), 3);
    EXPECT_EQ(*rhs, 4);
    EXPECT_EQ(*std::next(rhs), 2);
    EXPECT_EQ(rhs, l.begin());
}

TEST(SwapListNodesTest, SwapWithMiddle) {
    std::list<int> l = {10, 20, 30, 40, 50};
    auto lhs = std::next(l.begin());  // 20
    auto rhs = std::next(l.begin(), 3);  // 40
    SwapListNodes(l, lhs, rhs);
    EXPECT_EQ(ToVector(l), (std::vector<int>{10, 40, 30, 20, 50}));
    EXPECT_EQ(*lhs, 20);
    EXPECT_EQ(*std::next(lhs), 50);
    EXPECT_EQ(*std::prev(lhs), 30);
    EXPECT_EQ(*rhs, 40);
    EXPECT_EQ(*std::next(rhs), 30);
    EXPECT_EQ(*std::prev(rhs), 10);
}
