#include "qret/math/boolean_formula.h"

#include <gtest/gtest.h>

#include <array>
#include <sstream>

#include "qret/base/json.h"

using namespace qret;
using namespace qret::math;

template <typename T>
std::string ToString(T&& x) {
    auto ss = std::stringstream();
    ss << x;
    return ss.str();
}

TEST(Var, Serialization) {
    const auto v = Var(1487);

    EXPECT_STREQ("1487", ToString(v).c_str());
    EXPECT_EQ(v, Json(v).template get<Var>());
}
TEST(Lit, Serialization) {
    {
        const auto l = Lit(Var(1487), true);

        EXPECT_STREQ("+1487", ToString(l).c_str());
        EXPECT_EQ(l, Json(l).template get<Lit>());
    }
    {
        const auto l = Lit(Var(1487), false);

        EXPECT_STREQ("-1487", ToString(l).c_str());
        EXPECT_EQ(l, Json(l).template get<Lit>());
    }
}
TEST(Term, Serialization) {
    auto t = Lit(Var(1487), true) & Lit(Var(0), true) & Lit(Var(10), false) & Lit(Var(0), false)
            & Lit(Var(111), true);
    EXPECT_STREQ("+1487&+0&-10&-0&+111", ToString(t).c_str());
    EXPECT_EQ(t, Json(t).template get<Term>());
    EXPECT_EQ(5, t.Size());
    EXPECT_FALSE(t.Empty());

    t.SortAndUnique();
    EXPECT_STREQ("-0&+0&-10&+111&+1487", ToString(t).c_str());
    EXPECT_EQ(t, Json(t).template get<Term>());
    EXPECT_EQ(5, t.Size());
    EXPECT_FALSE(t.Empty());

    t.ClearIfContradict();
    EXPECT_STREQ("", ToString(t).c_str());
    EXPECT_EQ(t, Json(t).template get<Term>());
    EXPECT_EQ(0, t.Size());
    EXPECT_TRUE(t.Empty());
}
TEST(Formula, Serialization) {
    const auto f =
            (Lit(Var(1487), true) & Lit(Var(10), false) & Lit(Var(0), false) & Lit(Var(111), true))
            | (Lit(Var(5), true) & Lit(Var(2), false));
    EXPECT_STREQ("(+1487&-10&-0&+111)|(+5&-2)", ToString(f).c_str());
    EXPECT_EQ(f, Json(f).template get<Formula>());
    EXPECT_EQ(2, f.Size());
    EXPECT_FALSE(f.Empty());
}
TEST(Formula, MinimizeByHeuristics) {
    const auto v = std::array<Var, 4>{Var(0), Var(1), Var(2), Var(3)};

    // trivial cases
    {
        auto f = Formula();
        EXPECT_EQ(0, f.Size());
        f.MinimizeByHeuristics();
        EXPECT_EQ(0, f.Size());
    }
    {
        auto f = Formula(Term(Lit(v[0])));
        EXPECT_EQ(1, f.Size());
        f.MinimizeByHeuristics();
        EXPECT_EQ(1, f.Size());
    }
    {
        auto f = Formula(Term(Lit(v[0])), Term(Lit(v[0]), Lit(v[1])));
        EXPECT_EQ(2, f.Size());
        f.MinimizeByHeuristics();
        EXPECT_EQ(1, f.Size());
    }

    //     A(0)
    //    /  \
    //   B(1) C(2)
    //  / \   |  \
    // D(3) E F   G
    // | \ /  | /
    // H  I   J
    // |  /  /
    // K

    // Formula at 'J'
    {
        auto J =
                Formula(Term(Lit(v[0], false) & Lit(v[2])),
                        Term(Lit(v[0], false) & Lit(v[2], false)));
        std::cout << J << std::endl;
        EXPECT_EQ(2, J.Size());
        J.MinimizeByHeuristics();
        std::cout << J << std::endl;
        EXPECT_EQ(1, J.Size());
    }

    // Formula at 'I'
    {
        auto I =
                Formula(Term(Lit(v[0]) & Lit(v[1]) & Lit(v[3], false)),
                        Term(Lit(v[0]) & Lit(v[1], false)));
        std::cout << I << std::endl;
        EXPECT_EQ(2, I.Size());
        I.MinimizeByHeuristics();
        std::cout << I << std::endl;
        EXPECT_EQ(2, I.Size());
    }

    // Formula at 'K'
    {
        auto K = Formula(
                // H
                Term(Lit(v[0]) & Lit(v[1]) & Lit(v[3])),
                // I
                Term(Lit(v[0]) & Lit(v[1]) & Lit(v[3], false)),
                Term(Lit(v[0]) & Lit(v[1], false)),
                // J
                Term(Lit(v[0], false) & Lit(v[2])),
                Term(Lit(v[0], false) & Lit(v[2], false))
        );
        std::cout << K << std::endl;
        EXPECT_EQ(5, K.Size());
        K.MinimizeByHeuristics();
        std::cout << K << std::endl;
        EXPECT_EQ(1, K.Size());
    }
}

TEST(Formula, MinimizeByQuineMcCluskey) {
    const auto v = std::array<Var, 4>{Var(0), Var(1), Var(2), Var(3)};

    // trivial cases
    {
        auto f = Formula();
        EXPECT_EQ(0, f.Size());
        f.MinimizeByQuineMcCluskey();
        EXPECT_EQ(0, f.Size());
    }
    {
        auto f = Formula(Term(Lit(v[0])));
        EXPECT_EQ(1, f.Size());
        f.MinimizeByQuineMcCluskey();
        EXPECT_EQ(1, f.Size());
    }
    {
        auto f = Formula(Term(Lit(v[0])), Term(Lit(v[0]), Lit(v[1])));
        EXPECT_EQ(2, f.Size());
        f.MinimizeByQuineMcCluskey();
        EXPECT_EQ(1, f.Size());
    }

    //     A(0)
    //    /  \
    //   B(1) C(2)
    //  / \   |  \
    // D(3) E F   G
    // | \ /  | /
    // H  I   J
    // |  /  /
    // K

    // Formula at 'J'
    {
        auto J =
                Formula(Term(Lit(v[0], false) & Lit(v[2])),
                        Term(Lit(v[0], false) & Lit(v[2], false)));
        std::cout << J << std::endl;
        EXPECT_EQ(2, J.Size());
        J.MinimizeByQuineMcCluskey();
        std::cout << J << std::endl;
        EXPECT_EQ(1, J.Size());
    }

    // Formula at 'I'
    {
        auto I =
                Formula(Term(Lit(v[0]) & Lit(v[1]) & Lit(v[3], false)),
                        Term(Lit(v[0]) & Lit(v[1], false)));
        std::cout << I << std::endl;
        EXPECT_EQ(2, I.Size());
        I.MinimizeByQuineMcCluskey();
        std::cout << I << std::endl;
        EXPECT_EQ(2, I.Size());
    }

    // Formula at 'K'
    {
        auto K = Formula(
                // H
                Term(Lit(v[0]) & Lit(v[1]) & Lit(v[3])),
                // I
                Term(Lit(v[0]) & Lit(v[1]) & Lit(v[3], false)),
                Term(Lit(v[0]) & Lit(v[1], false)),
                // J
                Term(Lit(v[0], false) & Lit(v[2])),
                Term(Lit(v[0], false) & Lit(v[2], false))
        );
        std::cout << K << std::endl;
        EXPECT_EQ(5, K.Size());
        K.MinimizeByQuineMcCluskey();
        std::cout << K << std::endl;
        EXPECT_EQ(0, K.Size());
    }
}
TEST(FindCoverSet, PetrickMethod) {
    {
        auto table = std::vector<std::unordered_set<std::uint32_t>>{};
        auto res = FindCoverSetByPetrickMethod(table);
        EXPECT_EQ(res.size(), 0);
    }
    {
        auto table = std::vector<std::unordered_set<std::uint32_t>>{{0, 1}};
        auto res = FindCoverSetByPetrickMethod(table);
        EXPECT_EQ(res.size(), 1);
    }
    {
        auto table = std::vector<std::unordered_set<std::uint32_t>>{{0, 1}, {}};
        auto res = FindCoverSetByPetrickMethod(table);
        EXPECT_EQ(res.size(), 1);
    }
    {
        auto table = std::vector<std::unordered_set<std::uint32_t>>{{1}, {1, 4}, {2}};
        auto res = FindCoverSetByPetrickMethod(table);
        EXPECT_EQ(res.size(), 2);
    }
    {
        auto table = std::vector<std::unordered_set<std::uint32_t>>{{1}, {3}, {5}};
        auto res = FindCoverSetByPetrickMethod(table);
        EXPECT_EQ(res.size(), 3);
    }
    {
        auto table = std::vector<std::unordered_set<std::uint32_t>>{
                {10, 11},
                {0, 1},
                {2, 6},
                {6, 7},
                {11, 15},
                {7, 15},
                {0, 2, 8, 10}
        };
        auto res = FindCoverSetByPetrickMethod(table);
        EXPECT_EQ(res.size(), 4);
    }
}
