#include "qret/base/option.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <variant>

using namespace qret;

static Opt<std::string> string1("s1", "string one", "string1", OptionHidden::Hidden);
static Opt<std::string> string2("s2", "string two", "string1", OptionHidden::Hidden);
static Opt<std::int64_t> int1("i1", 123, "int1", OptionHidden::Hidden);
static Opt<std::int64_t> int2("i2", 456, "int2", OptionHidden::Hidden);
static Opt<bool> bool1("b1", true, "bool1", OptionHidden::Hidden);
static Opt<bool> bool2("b2", false, "bool2", OptionHidden::Hidden);

TEST(Opt, Basic) {
    const auto& options = *qret::OptionStorage::GetOptionStorage();
    EXPECT_TRUE(options.Contains("s1"));
    EXPECT_TRUE(options.Contains("s2"));
    EXPECT_TRUE(options.Contains("i1"));
    EXPECT_TRUE(options.Contains("i2"));
    EXPECT_TRUE(options.Contains("b1"));
    EXPECT_TRUE(options.Contains("b2"));

    // Check string1.
    EXPECT_EQ("s1", string1.GetOption().GetCommand());
    EXPECT_EQ("string one", string1.GetOption().GetDefaultValue());
    EXPECT_EQ("string1", string1.GetOption().GetDescription());
    EXPECT_EQ(OptionHidden::Hidden, string1.GetOption().GetHiddenFlag());
    // Check int1.
    EXPECT_EQ("i1", int1.GetOption().GetCommand());
    EXPECT_EQ(123, int1.GetOption().GetDefaultValue());
    EXPECT_EQ("int1", int1.GetOption().GetDescription());
    EXPECT_EQ(OptionHidden::Hidden, int1.GetOption().GetHiddenFlag());
    // Check bool1.
    EXPECT_EQ("b1", bool1.GetOption().GetCommand());
    EXPECT_EQ(true, bool1.GetOption().GetDefaultValue());
    EXPECT_EQ("bool1", bool1.GetOption().GetDescription());
    EXPECT_EQ(OptionHidden::Hidden, bool1.GetOption().GetHiddenFlag());

    // Do some operations
    std::string str1 = "string one";
    EXPECT_EQ(str1, string1.Get());
    EXPECT_NE(str1.c_str(), string2.Get());
    EXPECT_EQ(123456, int1 * 1000 + int2);
    EXPECT_TRUE(bool1);
    EXPECT_FALSE(bool2);
}
TEST(Opt, Duplicate) {
    EXPECT_THROW(
            Opt<std::string> duplicate_string("s1", "string one", "string1", OptionHidden::Hidden),
            std::logic_error
    );
    EXPECT_THROW(Opt<std::int64_t> int1("i1", 123, "int1", OptionHidden::Hidden), std::logic_error);
    EXPECT_THROW(Opt<bool> bool1("b1", true, "bool1", OptionHidden::Hidden), std::logic_error);
}
TEST(Opt, SetValue) {
    const auto& options = *qret::OptionStorage::GetOptionStorage();
    auto& temps = *std::get<Option<std::string>*>(options.At("s1"));
    std::string message = "value changed";
    temps.SetValue(message);
    EXPECT_EQ(message, string1.Get());

    auto& tempi = *std::get<Option<std::int64_t>*>(options.At("i1"));
    tempi.SetValue(314);
    EXPECT_EQ(314, int1);

    auto& tempb = *std::get<Option<bool>*>(options.At("b1"));
    tempb.SetValue(false);
    EXPECT_EQ(false, bool1);
}
TEST(OptionStorage, Basic) {
    auto storage = OptionStorage();
    auto o0 = Option<std::int32_t>("o0", 10, "option 0", OptionHidden::ReallyHidden);
    storage.AddOption(&o0);
    auto o1 = Option<std::string>("o1", "-120", "option 1", OptionHidden::Hidden);
    storage.AddOption(&o1);
    auto o2 = Option<bool>("o2", false, "option 2", OptionHidden::NotHidden);
    storage.AddOption(&o2);

    EXPECT_EQ(3, storage.Size());

    for (const auto [command, option_ptr] : storage) {
        if (command == "o0") {
            EXPECT_TRUE(std::holds_alternative<Option<std::int32_t>*>(option_ptr));
            const auto* ptr = std::get<Option<std::int32_t>*>(option_ptr);
            EXPECT_EQ(o0.GetDefaultValue(), ptr->GetDefaultValue());
        } else if (command == "o1") {
            EXPECT_TRUE(std::holds_alternative<Option<std::string>*>(option_ptr));
            const auto* ptr = std::get<Option<std::string>*>(option_ptr);
            EXPECT_EQ(o1.GetDefaultValue(), ptr->GetDefaultValue());
        } else if (command == "o2") {
            EXPECT_TRUE(std::holds_alternative<Option<bool>*>(option_ptr));
            const auto* ptr = std::get<Option<bool>*>(option_ptr);
            EXPECT_EQ(o2.GetDefaultValue(), ptr->GetDefaultValue());
        } else {
            ASSERT_TRUE(false);
        }
    }
}
