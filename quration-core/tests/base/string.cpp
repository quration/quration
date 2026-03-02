#include "qret/base/string.h"

#include <gtest/gtest.h>

using namespace qret;

TEST(String, Lowercase) {
    EXPECT_STREQ("abc", GetLowerString("ABC").data());
    EXPECT_STREQ("abc", GetLowerString("AbC").data());
    EXPECT_STREQ("abc", GetLowerString("abc").data());
    EXPECT_STREQ("???", GetLowerString("???").data());
}

void MakeImpl(std::vector<std::string>& /* out */) {
    // do nothing
}
template <typename Head, typename... Tail>
void MakeImpl(std::vector<std::string>& out, Head&& head, Tail&&... tail) {
    out.emplace_back(head);
    MakeImpl(out, tail...);
}
template <typename... Args>
std::vector<std::string> Make(Args&... args) {
    auto ret = std::vector<std::string>();
    ret.reserve(sizeof...(args));
    MakeImpl(ret, args...);
    return ret;
}

TEST(String, SplitString) {
    EXPECT_EQ(Make("abcdefghi"), SplitString("abcdefghi", ','));
    EXPECT_EQ(Make(""), SplitString("", ','));
    EXPECT_EQ(Make("", "", "", ""), SplitString(",,,", ','));

    EXPECT_EQ(Make("abc", "def", "ghi"), SplitString("abc,def,ghi", ','));
    EXPECT_EQ(Make("abc", "", "def", "ghi"), SplitString("abc,,def,ghi", ','));
    EXPECT_EQ(Make("abc", "def", "ghi", "", "", ""), SplitString("abc,def,ghi,,,", ','));
    EXPECT_EQ(Make("", "", "", "abc", "def", "ghi"), SplitString(",,,abc,def,ghi", ','));

    EXPECT_EQ(Make("abc", "def", "ghi"), SplitString("abc|def|ghi", '|'));
    EXPECT_EQ(Make("abc", "", "def", "ghi"), SplitString("abc||def|ghi", '|'));
    EXPECT_EQ(Make("abc", "def", "ghi", "", "", ""), SplitString("abc|def|ghi|||", '|'));
    EXPECT_EQ(Make("", "", "", "abc", "def", "ghi"), SplitString("|||abc|def|ghi", '|'));

    // Test examples in docstring
    EXPECT_EQ(Make(""), SplitString("", ','));
    EXPECT_EQ(Make("", "", "", ""), SplitString("^^^", '^'));
    EXPECT_EQ(Make("abc", "def", "ghi"), SplitString("abc,def,ghi", ','));
    EXPECT_EQ(Make("abc", "def", "", "ghi"), SplitString("abc|def||ghi", '|'));
    EXPECT_EQ(Make("", "", "abc", "def", "ghi", "", ""), SplitString("//abc/def/ghi//", '/'));
}
