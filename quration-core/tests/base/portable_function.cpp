#include "qret/base/portable_function.h"

#include <gtest/gtest.h>

#include <functional>
#include <random>

#include "qret/base/json.h"

using namespace qret;

TEST(PortableFunction, RegisterSize) {
    auto bool_type = PortableFunction::RegisterType::Bool();
    ASSERT_THROW(auto sz = bool_type.ArraySize(), std::runtime_error);
    ASSERT_EQ(bool_type.BitSize(), 1);
    auto bool_array_type = PortableFunction::RegisterType::BoolArray(5);
    ASSERT_EQ(bool_array_type.ArraySize(), 5);
    ASSERT_EQ(bool_array_type.BitSize(), 5);
    auto int_type = PortableFunction::RegisterType::Int();
    ASSERT_THROW(auto sz = int_type.ArraySize(), std::runtime_error);
    ASSERT_EQ(int_type.BitSize(), 64);
    auto int_array_type = PortableFunction::RegisterType::IntArray(5);
    ASSERT_EQ(int_array_type.ArraySize(), 5);
    ASSERT_EQ(int_array_type.BitSize(), 64 * 5);
}

TEST(PortableFunction, Initialize) {
    ASSERT_NO_THROW(PortableFunction("function", {}, {}, {}));
    ASSERT_NO_THROW(PortableFunction(
            "function",
            {
                    {"i1", PortableFunction::RegisterType::Bool()},
                    {"i2", PortableFunction::RegisterType::BoolArray(2)},
                    {"i3", PortableFunction::RegisterType::Int()},
                    {"i4", PortableFunction::RegisterType::IntArray(4)},
            },
            {{"o1", PortableFunction::RegisterType::Int()}},
            {{"t1", PortableFunction::RegisterType::BoolArray(64)},
             {"t2", PortableFunction::RegisterType::IntArray(6)}}
    ));
    // input and output has same register name
    ASSERT_THROW(
            PortableFunction(
                    "function",
                    {
                            {"i1", PortableFunction::RegisterType::Bool()},
                            {"i2", PortableFunction::RegisterType::BoolArray(2)},
                            {"i3", PortableFunction::RegisterType::Int()},
                            {"i4", PortableFunction::RegisterType::IntArray(4)},
                    },
                    {{"i1", PortableFunction::RegisterType::Int()}},
                    {{"t1", PortableFunction::RegisterType::BoolArray(64)},
                     {"t2", PortableFunction::RegisterType::IntArray(6)}}
            ),
            std::runtime_error
    );
    // input and tmp_value has same register name
    ASSERT_THROW(
            PortableFunction(
                    "function",
                    {
                            {"i1", PortableFunction::RegisterType::Bool()},
                            {"i2", PortableFunction::RegisterType::BoolArray(2)},
                            {"i3", PortableFunction::RegisterType::Int()},
                            {"i4", PortableFunction::RegisterType::IntArray(4)},
                    },
                    {{"o1", PortableFunction::RegisterType::Int()}},
                    {{"i1", PortableFunction::RegisterType::BoolArray(64)},
                     {"t2", PortableFunction::RegisterType::IntArray(6)}}
            ),
            std::runtime_error
    );
    // output and tmp_value has same register name
    ASSERT_THROW(
            PortableFunction(
                    "function",
                    {
                            {"i1", PortableFunction::RegisterType::Bool()},
                            {"i2", PortableFunction::RegisterType::BoolArray(2)},
                            {"i3", PortableFunction::RegisterType::Int()},
                            {"i4", PortableFunction::RegisterType::IntArray(4)},
                    },
                    {{"o1", PortableFunction::RegisterType::Int()}},
                    {{"o1", PortableFunction::RegisterType::BoolArray(64)},
                     {"t2", PortableFunction::RegisterType::IntArray(6)}}
            ),
            std::runtime_error
    );
}

TEST(PortableFunction, AddInstruction) {
    auto function = PortableFunction(
            "function",
            {{"i", PortableFunction::RegisterType::Bool()}},
            {{"o", PortableFunction::RegisterType::Int()}},
            {{"tb", PortableFunction::RegisterType::Bool()},
             {"tba", PortableFunction::RegisterType::BoolArray(5)},
             {"ti", PortableFunction::RegisterType::Int()},
             {"tia", PortableFunction::RegisterType::IntArray(5)}}
    );
    // register is exist
    ASSERT_NO_THROW(function.IntToBool({"i"}, 0));
    ASSERT_NO_THROW(function.CopyInt({"o"}, 0));
    ASSERT_NO_THROW(function.CopyBool({"tb"}, 0));
    ASSERT_THROW(function.CopyBool({"not_found_name"}, 0), std::runtime_error);
    // array access is valid
    ASSERT_NO_THROW(function.CopyBool({"tba", 0}, 0));
    ASSERT_THROW(function.CopyBool({"tba", "tb"}, 0), std::runtime_error);
    ASSERT_NO_THROW(function.CopyBool({{"tba"}, {"ti"}}, 0));
    ASSERT_THROW(function.CopyBool({"tba", "tba"}, 0), std::runtime_error);
    ASSERT_THROW(function.CopyBool({"tba", "tia"}, 0), std::runtime_error);
    ASSERT_NO_THROW(function.CopyInt({"tia", 0}, 0));
    ASSERT_THROW(function.CopyBool({"tb", 0}, 0), std::runtime_error);
    ASSERT_THROW(function.CopyInt({"ti", 0}, 0), std::runtime_error);

    using namespace std::placeholders;

    // Assign Instruction
    auto instAddFuncList = std::vector<std::function<
            void(PortableFunction*,
                 const PortableFunction::RegisterAccessor&,
                 const PortableFunction::RegisterAccessor&)>>{

            &PortableFunction::CopyBool,
            &PortableFunction::CopyInt,
            &PortableFunction::BoolToInt,
            &PortableFunction::IntToBool
    };
    for (auto func = std::size_t{0}; func < instAddFuncList.size(); func++) {
        auto instAddFunc = instAddFuncList[func];
        auto dst_is_bool = func == 0 || func == 3;
        auto src_is_bool = func == 0 || func == 2;
        auto accessors = std::vector<PortableFunction::RegisterAccessor>{
                0,
                {"tb"},
                {"ti"},
                {"tba"},
                {"tbi"}
        };
        for (auto dst = std::size_t{0}; dst < accessors.size(); dst++) {
            for (auto src = std::size_t{0}; dst < accessors.size(); dst++) {
                if (dst == 0 || dst >= 3 || src >= 3 || (dst_is_bool && dst == 2)
                    || (!dst_is_bool && dst == 1) || (src_is_bool && src == 2)
                    || (!src_is_bool && src == 1)) {
                    ASSERT_THROW(
                            instAddFunc(&function, accessors[dst], accessors[src]),
                            std::runtime_error
                    );
                } else {
                    ASSERT_NO_THROW(instAddFunc(&function, accessors[dst], accessors[src]));
                }
            }
        }
    }

    // UnaryBoolInstruction
    for (std::function<
                 void(PortableFunction*,
                      const PortableFunction::RegisterAccessor&,
                      const PortableFunction::RegisterAccessor&)> instAddFunc :
         {&PortableFunction::LNot}) {
        // dst variation
        ASSERT_THROW(instAddFunc(&function, 0, 0), std::runtime_error);
        ASSERT_NO_THROW(instAddFunc(&function, {"tb"}, 0));
        ASSERT_THROW(instAddFunc(&function, {"ti"}, 0), std::runtime_error);
        ASSERT_THROW(instAddFunc(&function, {"tba"}, 0), std::runtime_error);
        ASSERT_THROW(instAddFunc(&function, {"tia"}, 0), std::runtime_error);
        // src variation
        ASSERT_NO_THROW(instAddFunc(&function, {"tb"}, 0));
        ASSERT_NO_THROW(instAddFunc(&function, {"tb"}, {"tb"}));
        ASSERT_THROW(instAddFunc(&function, {"tb"}, {"ti"}), std::runtime_error);
        ASSERT_THROW(instAddFunc(&function, {"tb"}, {"tba"}), std::runtime_error);
        ASSERT_THROW(instAddFunc(&function, {"tb"}, {"tia"}), std::runtime_error);
    }

    // BinaryBoolInstruction
    for (std::function<
                 void(PortableFunction*,
                      const PortableFunction::RegisterAccessor&,
                      const PortableFunction::RegisterAccessor&,
                      const PortableFunction::RegisterAccessor&)> instAddFunc :
         {&PortableFunction::LAnd, &PortableFunction::LOr, &PortableFunction::LXor}) {
        // dst variation
        ASSERT_THROW(instAddFunc(&function, 0, 0, 0), std::runtime_error);
        ASSERT_NO_THROW(instAddFunc(&function, {"tb"}, 0, 0));
        ASSERT_THROW(instAddFunc(&function, {"ti"}, 0, 0), std::runtime_error);
        ASSERT_THROW(instAddFunc(&function, {"tba"}, 0, 0), std::runtime_error);
        ASSERT_THROW(instAddFunc(&function, {"tia"}, 0, 0), std::runtime_error);
        // src1 variation
        ASSERT_NO_THROW(instAddFunc(&function, {"tb"}, 0, 0));
        ASSERT_NO_THROW(instAddFunc(&function, {"tb"}, {"tb"}, 0));
        ASSERT_THROW(instAddFunc(&function, {"tb"}, {"ti"}, 0), std::runtime_error);
        ASSERT_THROW(instAddFunc(&function, {"tb"}, {"tba"}, 0), std::runtime_error);
        ASSERT_THROW(instAddFunc(&function, {"tb"}, {"tia"}, 0), std::runtime_error);
        // src2 variation
        ASSERT_NO_THROW(instAddFunc(&function, {"tb"}, 0, 0));
        ASSERT_NO_THROW(instAddFunc(&function, {"tb"}, 0, {"tb"}));
        ASSERT_THROW(instAddFunc(&function, {"tb"}, 0, {"ti"}), std::runtime_error);
        ASSERT_THROW(instAddFunc(&function, {"tb"}, 0, {"tba"}), std::runtime_error);
        ASSERT_THROW(instAddFunc(&function, {"tb"}, 0, {"tia"}), std::runtime_error);
    }

    // UnaryIntInstruction
    for (std::function<
                 void(PortableFunction*,
                      const PortableFunction::RegisterAccessor&,
                      const PortableFunction::RegisterAccessor&)> instAddFunc :
         {&PortableFunction::BNot, &PortableFunction::Neg}) {
        // dst variation
        ASSERT_THROW(instAddFunc(&function, 0, 0), std::runtime_error);
        ASSERT_THROW(instAddFunc(&function, {"tb"}, 0), std::runtime_error);
        ASSERT_NO_THROW(instAddFunc(&function, {"ti"}, 0));
        ASSERT_THROW(instAddFunc(&function, {"tba"}, 0), std::runtime_error);
        ASSERT_THROW(instAddFunc(&function, {"tia"}, 0), std::runtime_error);
        // src variation
        ASSERT_NO_THROW(instAddFunc(&function, {"ti"}, 0));
        ASSERT_THROW(instAddFunc(&function, {"ti"}, {"tb"}), std::runtime_error);
        ASSERT_NO_THROW(instAddFunc(&function, {"ti"}, {"ti"}));
        ASSERT_THROW(instAddFunc(&function, {"ti"}, {"tba"}), std::runtime_error);
        ASSERT_THROW(instAddFunc(&function, {"ti"}, {"tia"}), std::runtime_error);
    }

    // BinaryBoolInstruction
    for (std::function<
                 void(PortableFunction*,
                      const PortableFunction::RegisterAccessor&,
                      const PortableFunction::RegisterAccessor&,
                      const PortableFunction::RegisterAccessor&)> instAddFunc :
         {&PortableFunction::BAnd,
          &PortableFunction::BOr,
          &PortableFunction::BXor,
          &PortableFunction::LShift,
          &PortableFunction::RShift,
          &PortableFunction::Add,
          &PortableFunction::Sub,
          &PortableFunction::Mul,
          &PortableFunction::Div,
          &PortableFunction::Div}) {
        // dst variation
        ASSERT_THROW(instAddFunc(&function, 0, 0, 0), std::runtime_error);
        ASSERT_THROW(instAddFunc(&function, {"tb"}, 0, 0), std::runtime_error);
        ASSERT_NO_THROW(instAddFunc(&function, {"ti"}, 0, 0));
        ASSERT_THROW(instAddFunc(&function, {"tba"}, 0, 0), std::runtime_error);
        ASSERT_THROW(instAddFunc(&function, {"tia"}, 0, 0), std::runtime_error);
        // src1 variation
        ASSERT_NO_THROW(instAddFunc(&function, {"ti"}, 0, 0));
        ASSERT_THROW(instAddFunc(&function, {"ti"}, {"tb"}, 0), std::runtime_error);
        ASSERT_NO_THROW(instAddFunc(&function, {"ti"}, {"ti"}, 0));
        ASSERT_THROW(instAddFunc(&function, {"ti"}, {"tba"}, 0), std::runtime_error);
        ASSERT_THROW(instAddFunc(&function, {"ti"}, {"tia"}, 0), std::runtime_error);
        // src2 variation
        ASSERT_NO_THROW(instAddFunc(&function, {"ti"}, 0, 0));
        ASSERT_THROW(instAddFunc(&function, {"ti"}, 0, {"tb"}), std::runtime_error);
        ASSERT_NO_THROW(instAddFunc(&function, {"ti"}, 0, {"ti"}));
        ASSERT_THROW(instAddFunc(&function, {"ti"}, 0, {"tba"}), std::runtime_error);
        ASSERT_THROW(instAddFunc(&function, {"ti"}, 0, {"tia"}), std::runtime_error);
    }

    // CompareInstruction
    for (std::function<
                 void(PortableFunction*,
                      const PortableFunction::RegisterAccessor&,
                      const PortableFunction::RegisterAccessor&,
                      const PortableFunction::RegisterAccessor&)> instAddFunc :
         {&PortableFunction::Eq,
          &PortableFunction::Ne,
          &PortableFunction::Lt,
          &PortableFunction::Le,
          &PortableFunction::Gt,
          &PortableFunction::Ge}) {
        // dst variation
        ASSERT_THROW(instAddFunc(&function, 0, 0, 0), std::runtime_error);
        ASSERT_NO_THROW(instAddFunc(&function, {"tb"}, 0, 0));
        ASSERT_THROW(instAddFunc(&function, {"ti"}, 0, 0), std::runtime_error);
        ASSERT_THROW(instAddFunc(&function, {"tba"}, 0, 0), std::runtime_error);
        ASSERT_THROW(instAddFunc(&function, {"tia"}, 0, 0), std::runtime_error);
        // src1 variation
        ASSERT_NO_THROW(instAddFunc(&function, {"tb"}, 0, 0));
        ASSERT_THROW(instAddFunc(&function, {"tb"}, {"tb"}, 0), std::runtime_error);
        ASSERT_NO_THROW(instAddFunc(&function, {"tb"}, {"ti"}, 0));
        ASSERT_THROW(instAddFunc(&function, {"tb"}, {"tba"}, 0), std::runtime_error);
        ASSERT_THROW(instAddFunc(&function, {"tb"}, {"tia"}, 0), std::runtime_error);
        // src2 variation
        ASSERT_NO_THROW(instAddFunc(&function, {"tb"}, 0, 0));
        ASSERT_THROW(instAddFunc(&function, {"tb"}, 0, {"tb"}), std::runtime_error);
        ASSERT_NO_THROW(instAddFunc(&function, {"tb"}, 0, {"ti"}));
        ASSERT_THROW(instAddFunc(&function, {"tb"}, 0, {"tba"}), std::runtime_error);
        ASSERT_THROW(instAddFunc(&function, {"tb"}, 0, {"tia"}), std::runtime_error);
    }
}

TEST(PortableFunction, Serialize) {
    auto function = PortableFunction(
            "function",
            {{"ib", PortableFunction::RegisterType::Bool()},
             {"ii1", PortableFunction::RegisterType::Int()},
             {"ii2", PortableFunction::RegisterType::Int()},
             {"iba", PortableFunction::RegisterType::BoolArray(2)}},
            {{"oi", PortableFunction::RegisterType::Int()},
             {"oba1", PortableFunction::RegisterType::BoolArray(3)},
             {"oba2", PortableFunction::RegisterType::BoolArray(1)},
             {"oia", PortableFunction::RegisterType::IntArray(2)}},
            {{"tb1", PortableFunction::RegisterType::Bool()},
             {"tb2", PortableFunction::RegisterType::Bool()},
             {"tb3", PortableFunction::RegisterType::Bool()},
             {"ti1", PortableFunction::RegisterType::Int()},
             {"ti2", PortableFunction::RegisterType::Int()},
             {"ti3", PortableFunction::RegisterType::Int()},
             {"ti4", PortableFunction::RegisterType::Int()}}
    );
    function.Le({"oba1", 0}, {"ii1"}, 4);
    function.Mul({"ti1"}, {"ii2"}, 2);
    function.Mod({"ti3"}, {"ti1"}, 5);
    function.Gt({"tb1"}, {"ti3"}, {"ti1"});
    function.LAnd({"tb1"}, {"ib"}, {"tb1"});
    function.LNot({"tb2"}, {"tb1"});
    function.LXor({"tb3"}, true, {"tb2"});
    function.LOr({"oba1", 1}, false, {"tb3"});
    function.Add({"ti2"}, -4, 3);
    function.Sub({"ti4"}, {"ti2"}, 8);
    function.Eq({"oba1", 2}, 5, 0);
    function.Div({"oia", 0}, 4, {"ti4"});
    function.Lt({"tb1"}, {"ii2"}, {"ii1"});
    function.CopyBool({"ib"}, true);
    function.CopyInt({"oia", 2}, -1);
    function.BoolToInt({"ti3"}, {"tb1"});
    function.CopyBool({"oba1", 1}, {"iba", "ti3"});
    function.BXor({"oia", 0}, 4, 7);
    function.BNot({"oia", 1}, {"oia", 0});
    function.BAnd({"ti3"}, {"ti1"}, 1);
    function.Ge({"tb1"}, 4, {"ti2"});
    function.BOr({"ti2"}, {"ti1"}, {"ti4"});
    function.Ne({"oba1", 2}, -2, 99);

    const auto serialized = Json(function);
    auto function2 = PortableFunction();
    from_json(serialized, function2);
    const auto serialized2 = Json(function2);
    EXPECT_EQ(serialized, serialized2);
}

TEST(PortableFunction, BuilderDuplicatedVariable) {
    auto builder = PortableFunctionBuilder{};
    auto i = builder.AddBoolInputVariable("i");
    auto o = builder.AddBoolOutputVariable("o");
    ASSERT_THROW(auto i2 = builder.AddIntInputVariable("i"), std::runtime_error);
    ASSERT_THROW(auto o2 = builder.AddIntOutputVariable("o"), std::runtime_error);
    ASSERT_THROW(auto i2 = builder.AddIntOutputVariable("i"), std::runtime_error);
    ASSERT_THROW(auto o2 = builder.AddIntInputVariable("o"), std::runtime_error);
}

TEST(PortableFunction, BuilderOperators) {
    auto builder = PortableFunctionBuilder{};
    auto bool1 = builder.AddBoolInputVariable("bool1");
    auto bool2 = builder.AddBoolInputVariable("bool2");
    auto int1 = builder.AddIntInputVariable("int1");
    auto int2 = builder.AddIntInputVariable("int2");
    auto inta = builder.AddIntArrayInputVariable("inta", 5);
    auto bool3 = bool1;
    bool2 = true;
    bool3 = 35;
    bool1 = !bool1;
    bool1 = bool1 & bool2;
    bool1 = bool1 & int1;
    bool1 = bool1 & false;
    bool1 = int1 & bool1;
    bool1 = int1 & int2;
    bool1 = int1 & 5;
    bool1 = true & bool3;
    bool2 = 3 & int1;
    bool1 &= bool1;
    bool1 = bool1 | bool1;
    bool1 |= bool1;
    bool1 = bool1 ^ bool1;
    bool1 ^= bool1;
    bool1 = bool1 << bool1;
    bool1 <<= bool1;
    bool1 = bool1 >> bool1;
    bool1 >>= bool1;
    bool1 = bool1 + bool1;
    bool1 += bool1;
    bool1 = bool1 - bool1;
    bool1 -= bool1;
    bool1 = bool1 * bool1;
    bool1 *= bool1;
    bool1 = bool1 / bool1;
    bool1 /= bool1;
    bool1 = bool1 % bool1;
    bool1 %= bool1;
    bool1 = bool1 == bool1;
    bool1 = bool1 != bool1;
    bool1 = bool1 < bool1;
    bool1 = bool1 <= bool1;
    bool1 = bool1 > bool1;
    bool1 = bool1 >= bool1;

    int1 = inta[2];
    int1 = inta[bool1];
    int1 = inta[inta[inta[inta[bool2]]]];

    auto function = builder.Compile("operator_test");
}

TEST(PortableFunction, UnaryInstruction) {
    auto builder = PortableFunctionBuilder{};
    auto b1 = builder.AddBoolInputVariable("b1");
    auto b2 = builder.AddBoolOutputVariable("b2");
    auto i1 = builder.AddIntArrayInputVariable("i1", 2);
    auto i2 = builder.AddIntArrayOutputVariable("i2", 2);
    b2 = !b1;
    i2[0] = ~i1[0];
    i2[1] = -i1[1];
    auto function = builder.Compile("test");
    std::mt19937_64 mt(0);
    for (int i = 0; i < 10; i++) {
        auto b1 = (bool)(mt() & 1);
        auto i1 = std::vector{(std::int64_t)mt(), (std::int64_t)mt()};
        auto output = function.Run({{"b1", b1}, {"i1", i1}});
        auto b2 = output.GetBool("b2");
        auto i2 = output.GetIntArray("i2");
        ASSERT_EQ(b2, !b1);
        ASSERT_EQ(i2[0], ~i1[0]);
        ASSERT_EQ(i2[1], -i1[1]);
    }
}

TEST(PortableFunction, BinaryBoolInstruction) {
    auto builder = PortableFunctionBuilder{};
    auto b1 = builder.AddBoolArrayInputVariable("b1", 3);
    auto b2 = builder.AddBoolArrayInputVariable("b2", 3);
    auto b3 = builder.AddBoolArrayOutputVariable("b3", 3);
    b3[0] = b1[0] & b2[0];
    b3[1] = b1[1] | b2[1];
    b3[2] = b1[2] ^ b2[2];
    auto function = builder.Compile("test");
    std::mt19937_64 mt(0);
    for (int i = 0; i < 10; i++) {
        auto b1 = std::vector<bool>{(bool)(mt() & 1), (bool)(mt() & 1), (bool)(mt() & 1)};
        auto b2 = std::vector<bool>{(bool)(mt() & 1), (bool)(mt() & 1), (bool)(mt() & 1)};
        auto b3 = function.Run({{"b1", b1}, {"b2", b2}}).GetBoolArray("b3");
        ASSERT_EQ(b3[0], b1[0] & b2[0]);
        ASSERT_EQ(b3[1], b1[1] | b2[1]);
        ASSERT_EQ(b3[2], b1[2] ^ b2[2]);
    }
}

TEST(PortableFunction, ShiftInstruction) {
    auto builder = PortableFunctionBuilder{};
    auto i1 = builder.AddIntArrayInputVariable("i1", 2);
    auto i2 = builder.AddIntArrayInputVariable("i2", 2);
    auto i3 = builder.AddIntArrayOutputVariable("i3", 2);
    i3[0] = i1[0] << i2[0];
    i3[1] = i1[1] >> i2[1];
    auto function = builder.Compile("test");
    std::mt19937_64 mt(0);
    for (int i = 0; i < 10; i++) {
        auto i1 = std::vector<std::int64_t>{(std::int64_t)mt(), (std::int64_t)mt()};
        auto i2 = std::vector<std::int64_t>{(std::int64_t)(mt() % 60), (std::int64_t)(mt() & 60)};
        auto i3 = function.Run({{"i1", i1}, {"i2", i2}}).GetIntArray("i3");
        ASSERT_EQ(i3[0], i1[0] << i2[0]);
        ASSERT_EQ(i3[1], i1[1] >> i2[1]);
    }
}

TEST(PortableFunction, BinaryIntInstruction) {
    auto builder = PortableFunctionBuilder{};
    auto i1 = builder.AddIntArrayInputVariable("i1", 8);
    auto i2 = builder.AddIntArrayInputVariable("i2", 8);
    auto i3 = builder.AddIntArrayOutputVariable("i3", 8);
    i3[0] = i1[0] & i2[0];
    i3[1] = i1[1] | i2[1];
    i3[2] = i1[2] ^ i2[2];
    i3[3] = i1[3] + i2[3];
    i3[4] = i1[4] - i2[4];
    i3[5] = i1[5] * i2[5];
    i3[6] = i1[6] / i2[6];
    i3[7] = i1[7] % i2[7];
    auto function = builder.Compile("test");
    std::mt19937_64 mt(0);
    for (int i = 0; i < 10; i++) {
        auto i1 = std::vector<std::int64_t>(8);
        auto i2 = std::vector<std::int64_t>(8);
        for (int j = 0; j < 8; j++) {
            i1[j] = mt(), i2[j] = mt();
        }
        auto i3 = function.Run({{"i1", i1}, {"i2", i2}}).GetIntArray("i3");
        ASSERT_EQ(i3[0], i1[0] & i2[0]);
        ASSERT_EQ(i3[1], i1[1] | i2[1]);
        ASSERT_EQ(i3[2], i1[2] ^ i2[2]);
        ASSERT_EQ(i3[3], i1[3] + i2[3]);
        ASSERT_EQ(i3[4], i1[4] - i2[4]);
        ASSERT_EQ(i3[5], i1[5] * i2[5]);
        ASSERT_EQ(i3[6], i1[6] / i2[6]);
        ASSERT_EQ(i3[7], i1[7] % i2[7]);
    }
}

TEST(PortableFunction, CompareInstruction) {
    auto builder = PortableFunctionBuilder{};
    auto i1 = builder.AddIntArrayInputVariable("i1", 8);
    auto i2 = builder.AddIntArrayInputVariable("i2", 8);
    auto eq = builder.AddBoolArrayOutputVariable("eq", 64);
    auto ne = builder.AddBoolArrayOutputVariable("ne", 64);
    auto lt = builder.AddBoolArrayOutputVariable("lt", 64);
    auto le = builder.AddBoolArrayOutputVariable("le", 64);
    auto gt = builder.AddBoolArrayOutputVariable("gt", 64);
    auto ge = builder.AddBoolArrayOutputVariable("ge", 64);
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++) {
            eq[i * 8 + j] = i1[i] == i2[j];
            ne[i * 8 + j] = i1[i] != i2[j];
            lt[i * 8 + j] = i1[i] < i2[j];
            le[i * 8 + j] = i1[i] <= i2[j];
            gt[i * 8 + j] = i1[i] > i2[j];
            ge[i * 8 + j] = i1[i] >= i2[j];
        }
    auto function = builder.Compile("test");
    std::mt19937_64 mt(0);
    for (int i = 0; i < 10; i++) {
        auto i1 = std::vector<std::int64_t>(8);
        for (int j = 0; j < 8; j++) {
            i1[j] = mt();
        }
        auto i2 = i1;
        std::ranges::shuffle(i2, mt);
        auto output = function.Run({{"i1", i1}, {"i2", i2}});
        auto eq = output.GetBoolArray("eq");
        auto ne = output.GetBoolArray("ne");
        auto lt = output.GetBoolArray("lt");
        auto le = output.GetBoolArray("le");
        auto gt = output.GetBoolArray("gt");
        auto ge = output.GetBoolArray("ge");
        for (int i = 0; i < 8; i++)
            for (int j = 0; j < 8; j++) {
                ASSERT_EQ(eq[i * 8 + j], i1[i] == i2[j]);
                ASSERT_EQ(ne[i * 8 + j], i1[i] != i2[j]);
                ASSERT_EQ(lt[i * 8 + j], i1[i] < i2[j]);
                ASSERT_EQ(le[i * 8 + j], i1[i] <= i2[j]);
                ASSERT_EQ(gt[i * 8 + j], i1[i] > i2[j]);
                ASSERT_EQ(ge[i * 8 + j], i1[i] >= i2[j]);
            }
    }
}

TEST(PortableFunction, ComplexArraySubscription) {
    auto builder = PortableFunctionBuilder{};
}

TEST(PortableFunction, AllXor) {
    auto builder = PortableFunctionBuilder{};
    auto arr = builder.AddBoolArrayInputVariable("arr", 10);
    auto res = builder.AddBoolOutputVariable("res");
    res = 0;
    for (int i = 0; i < 10; i++) {
        res ^= arr[i];
    }
    auto function = builder.Compile("all_xor");

    std::mt19937 mt(0);
    for (int i = 0; i < 10; i++) {
        auto input = std::vector<bool>(10);
        for (int j = 0; j < 10; j++)
            input[j] = mt() & 1;
        auto test = function.Run({{"arr", input}}).GetBool("res");
        auto expected = std::accumulate(input.begin(), input.end(), false, std::bit_xor());
        ASSERT_EQ(test, expected);
    }
}

TEST(PortableFunction, CollatzStep) {
    auto builder = PortableFunctionBuilder{};
    auto x = builder.AddIntInputVariable("x");
    builder.AddIntOutputVariable("res") = builder.Mux(x % 2 == 0, x / 2, 3 * x + 1);
    auto function = builder.Compile("collatz");

    std::mt19937_64 mt(0);
    for (int i = 0; i < 10; i++) {
        auto input = std::abs((std::int64_t)mt());
        auto test = function.Run({{"x", input}}).GetInt("res");
        auto expected = input % 2 == 0 ? input / 2 : 3 * input + 1;
        ASSERT_EQ(test, expected);
    }
}

TEST(PortableFunction, ModPow) {
    auto builder = PortableFunctionBuilder{};
    auto a = builder.AddIntInputVariable("a");
    auto n = builder.AddIntInputVariable("n");
    auto m = builder.AddIntInputVariable("m");
    auto res = builder.AddIntOutputVariable("res");
    res = 1;
    for (int i = 0; i < 60; i++) {
        (res *= builder.Mux(n & 1, a, 1)) %= m;
        (a *= a) %= m;
        n >>= 1;
    }
    auto function = builder.Compile("modpow");

    std::mt19937_64 mt(0);
    for (int i = 0; i < 10; i++) {
        std::int64_t a = mt() % 1000000000;
        std::int64_t n = mt() % 1000000000000000000;
        std::int64_t m = mt() % 1000000000;

        auto test = function.Run({{"a", a}, {"n", n}, {"m", m}}).GetInt("res");
        auto res = (std::int64_t)1;
        for (int i = 0; i < 60; i++) {
            (res *= (n & 1 ? a : 1)) %= m;
            (a *= a) %= m;
            n >>= 1;
        }
        ASSERT_EQ(test, res);
    }
}

TEST(PortableFunction, Exception) {
    {
        auto builder = PortableFunctionBuilder{};
        auto a = builder.AddIntInputVariable("a");
        auto b = builder.AddIntOutputVariable("b");
        EXPECT_THROW({ b = a / 0; }, std::runtime_error);
    }
    {
        auto builder = PortableFunctionBuilder{};
        auto a = builder.AddIntInputVariable("a");
        auto b = builder.AddIntOutputVariable("b");
        EXPECT_THROW({ b = a % 0; }, std::runtime_error);
    }
    {
        auto builder = PortableFunctionBuilder{};
        auto a = builder.AddIntInputVariable("a");
        auto b = builder.AddIntInputVariable("b");
        auto c = builder.AddIntOutputVariable("c");
        c = a / b;
        auto func = builder.Compile("");
        EXPECT_NO_THROW(func.Run({{"a", 2}, {"b", 1}}));
        EXPECT_THROW(func.Run({{"a", 2}, {"b", 0}}), std::runtime_error);
    }
    {
        auto builder = PortableFunctionBuilder{};
        auto a = builder.AddIntInputVariable("a");
        auto b = builder.AddIntInputVariable("b");
        auto c = builder.AddIntOutputVariable("c");
        c = a % b;
        auto func = builder.Compile("");
        EXPECT_NO_THROW(func.Run({{"a", 2}, {"b", 1}}));
        EXPECT_THROW(func.Run({{"a", 2}, {"b", 0}}), std::runtime_error);
    }
    {
        auto builder = PortableFunctionBuilder{};
        auto a = builder.AddIntArrayInputVariable("a", 2);
        auto b = builder.AddIntOutputVariable("b");
        EXPECT_THROW({ b = a[2]; }, std::runtime_error);
    }
    {
        auto builder = PortableFunctionBuilder{};
        auto a = builder.AddIntArrayInputVariable("a", 2);
        auto b = builder.AddIntInputVariable("b");
        auto c = builder.AddIntOutputVariable("c");
        c = a[b];
        auto func = builder.Compile("");
        EXPECT_NO_THROW(func.Run({{"a", std::vector<std::int64_t>{0, 1}}, {"b", 1}}));
        EXPECT_THROW(
                func.Run({{"a", std::vector<std::int64_t>{0, 1}}, {"b", 2}}),
                std::runtime_error
        );
    }
}
