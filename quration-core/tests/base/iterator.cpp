#include "qret/base/iterator.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <list>
#include <memory>
#include <string>

TEST(ListIterator, IteratorConcept) {
    using A = qret::ListIterator<std::int32_t, false>;
    static_assert(std::indirectly_readable<A>);
    static_assert(std::indirectly_writable<A, std::int32_t>);
    static_assert(std::bidirectional_iterator<A>);

    using B = qret::ListIterator<std::int32_t, true>;
    static_assert(std::indirectly_readable<B>);
    // static_assert(std::indirectly_writable<B, std::int32_t>);  // compile error
    static_assert(std::bidirectional_iterator<B>);
}
TEST(ListIterator, Basic) {
    auto list = std::list<std::unique_ptr<std::string>>();
    list.emplace_back(std::make_unique<std::string>("10"));
    list.emplace_back(std::make_unique<std::string>("20"));
    list.emplace_back(std::make_unique<std::string>("-10"));

    auto bitr = qret::ListIterator<std::string, true>(list.begin());
    auto eitr = qret::ListIterator<std::string, true>(list.end());

    EXPECT_STREQ("10", (*bitr).c_str());
    EXPECT_STREQ("10", bitr->c_str());
    bitr++;
    EXPECT_STREQ("20", (*bitr).c_str());
    EXPECT_STREQ("20", bitr->c_str());
    ++bitr;
    EXPECT_STREQ("-10", (*bitr).c_str());
    EXPECT_STREQ("-10", bitr->c_str());
    ++bitr;
    EXPECT_EQ(eitr, bitr);
}
