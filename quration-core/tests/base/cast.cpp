#include "qret/base/cast.h"

#include <gtest/gtest.h>

// A
// |  \
// B   C
// |
// D
//
// A is base class
// B is derived from A
// C is derived from A
// D is derived from B
enum class Type { A, B, C, D };

struct A {
    A() = default;
    explicit A(Type type)
        : type{type} {};
    template <typename T>
    [[nodiscard]] static bool ClassOf(const T* t) {
        return t->type == Type::A || t->type == Type::B || t->type == Type::C || t->type == Type::D;
    }
    Type type = Type::A;
};
struct B : public A {
    B()
        : A(Type::B) {}
    template <typename T>
    [[nodiscard]] static bool ClassOf(const T* t) {
        return t->type == Type::B || t->type == Type::D;
    }
};
struct C : public A {
    C()
        : A(Type::C) {}
    template <typename T>
    [[nodiscard]] static bool ClassOf(const T* t) {
        return t->type == Type::C;
    }
};
struct D : public B {
    D()
        : B() {
        type = Type::D;
    }
    template <typename T>
    [[nodiscard]] static bool ClassOf(const T* t) {
        return t->type == Type::D;
    }
};

TEST(SafeCast, ClassOfIsDefined) {
    auto a = A{};

    static_assert(qret::ClassOfIsDefined<A, A>);
    static_assert(qret::ClassOfIsDefined<B, A>);
    static_assert(qret::ClassOfIsDefined<C, A>);
    static_assert(qret::ClassOfIsDefined<D, A>);
}
TEST(SafeCast, IsA) {
    {
        auto a = A{};
        EXPECT_TRUE((qret::IsA<A, A>(&a)));
        EXPECT_FALSE((qret::IsA<B, A>(&a)));
        EXPECT_FALSE((qret::IsA<C, A>(&a)));
        EXPECT_FALSE((qret::IsA<D, A>(&a)));
    }
    {
        auto b = B{};
        EXPECT_TRUE((qret::IsA<A, B>(&b)));
        EXPECT_TRUE((qret::IsA<B, B>(&b)));
        EXPECT_FALSE((qret::IsA<C, B>(&b)));
        EXPECT_FALSE((qret::IsA<D, B>(&b)));
    }
    {
        auto c = C{};
        EXPECT_TRUE((qret::IsA<A, C>(&c)));
        EXPECT_FALSE((qret::IsA<B, C>(&c)));
        EXPECT_TRUE((qret::IsA<C, C>(&c)));
        EXPECT_FALSE((qret::IsA<D, C>(&c)));
    }
    {
        auto d = D{};
        EXPECT_TRUE((qret::IsA<A, D>(&d)));
        EXPECT_TRUE((qret::IsA<B, D>(&d)));
        EXPECT_FALSE((qret::IsA<C, D>(&d)));
        EXPECT_TRUE((qret::IsA<D, D>(&d)));
    }
}
TEST(TestCast, IsAPresent) {
    {
        auto a = A{};
        EXPECT_TRUE((qret::IsAAndPresent<A, A>(&a)));
        EXPECT_FALSE((qret::IsAAndPresent<B, A>(&a)));
        EXPECT_FALSE((qret::IsAAndPresent<C, A>(&a)));
        EXPECT_FALSE((qret::IsAAndPresent<D, A>(&a)));
    }
    {
        A* a = nullptr;
        EXPECT_FALSE((qret::IsAAndPresent<A, A>(a)));
        EXPECT_FALSE((qret::IsAAndPresent<B, A>(a)));
        EXPECT_FALSE((qret::IsAAndPresent<C, A>(a)));
        EXPECT_FALSE((qret::IsAAndPresent<D, A>(a)));
    }
    {
        auto b = B{};
        EXPECT_TRUE((qret::IsAAndPresent<A, B>(&b)));
        EXPECT_TRUE((qret::IsAAndPresent<B, B>(&b)));
        EXPECT_FALSE((qret::IsAAndPresent<C, B>(&b)));
        EXPECT_FALSE((qret::IsAAndPresent<D, B>(&b)));
    }
    {
        B* b = nullptr;
        EXPECT_FALSE((qret::IsAAndPresent<A, B>(b)));
        EXPECT_FALSE((qret::IsAAndPresent<B, B>(b)));
        EXPECT_FALSE((qret::IsAAndPresent<C, B>(b)));
        EXPECT_FALSE((qret::IsAAndPresent<D, B>(b)));
    }
    {
        auto c = C{};
        EXPECT_TRUE((qret::IsAAndPresent<A, C>(&c)));
        EXPECT_FALSE((qret::IsAAndPresent<B, C>(&c)));
        EXPECT_TRUE((qret::IsAAndPresent<C, C>(&c)));
        EXPECT_FALSE((qret::IsAAndPresent<D, C>(&c)));
    }
    {
        C* c = nullptr;
        EXPECT_FALSE((qret::IsAAndPresent<A, C>(c)));
        EXPECT_FALSE((qret::IsAAndPresent<B, C>(c)));
        EXPECT_FALSE((qret::IsAAndPresent<C, C>(c)));
        EXPECT_FALSE((qret::IsAAndPresent<D, C>(c)));
    }
    {
        auto d = D{};
        EXPECT_TRUE((qret::IsAAndPresent<A, D>(&d)));
        EXPECT_TRUE((qret::IsAAndPresent<B, D>(&d)));
        EXPECT_FALSE((qret::IsAAndPresent<C, D>(&d)));
        EXPECT_TRUE((qret::IsAAndPresent<D, D>(&d)));
    }
    {
        D* d = nullptr;
        EXPECT_FALSE((qret::IsAAndPresent<A, D>(d)));
        EXPECT_FALSE((qret::IsAAndPresent<B, D>(d)));
        EXPECT_FALSE((qret::IsAAndPresent<C, D>(d)));
        EXPECT_FALSE((qret::IsAAndPresent<D, D>(d)));
    }
}
TEST(SafeCast, Cast) {
    {
        auto b = B{};
        auto* a = qret::Cast<A>(&b);
        EXPECT_EQ(Type::B, a->type);
        EXPECT_EQ(&b, qret::Cast<B>(a));
    }
    {
        auto c = C{};
        auto* a = qret::Cast<A>(&c);
        EXPECT_EQ(Type::C, a->type);
        EXPECT_EQ(&c, qret::Cast<C>(a));
    }
    {
        auto d = D{};
        auto* a = qret::Cast<A>(&d);
        auto* b = qret::Cast<B>(&d);
        EXPECT_EQ(Type::D, a->type);
        EXPECT_EQ(Type::D, b->type);
        EXPECT_EQ(&d, qret::Cast<D>(a));
        EXPECT_EQ(&d, qret::Cast<D>(b));
    }
}
TEST(SafeCast, CastConst) {
    {
        const auto b = B{};
        const auto* a = qret::Cast<A>(&b);
        EXPECT_EQ(Type::B, a->type);
        EXPECT_EQ(&b, qret::Cast<B>(a));
    }
    {
        const auto c = C{};
        const auto* a = qret::Cast<A>(&c);
        EXPECT_EQ(Type::C, a->type);
        EXPECT_EQ(&c, qret::Cast<C>(a));
    }
    {
        const auto d = D{};
        const auto* a = qret::Cast<A>(&d);
        const auto* b = qret::Cast<B>(&d);
        EXPECT_EQ(Type::D, a->type);
        EXPECT_EQ(Type::D, b->type);
        EXPECT_EQ(&d, qret::Cast<D>(a));
        EXPECT_EQ(&d, qret::Cast<D>(b));
    }
}
TEST(SafeCast, CastIfPresent) {
    {
        auto b = B{};
        auto* a = qret::CastIfPresent<A>(&b);
        EXPECT_EQ(Type::B, a->type);
        EXPECT_EQ(&b, qret::CastIfPresent<B>(a));
    }
    {
        auto c = C{};
        auto* a = qret::CastIfPresent<A>(&c);
        EXPECT_EQ(Type::C, a->type);
        EXPECT_EQ(&c, qret::CastIfPresent<C>(a));
    }
    {
        auto d = D{};
        auto* a = qret::CastIfPresent<A>(&d);
        auto* b = qret::CastIfPresent<B>(&d);
        EXPECT_EQ(Type::D, a->type);
        EXPECT_EQ(Type::D, b->type);
        EXPECT_EQ(&d, qret::CastIfPresent<D>(a));
        EXPECT_EQ(&d, qret::CastIfPresent<D>(b));
    }

    // nullptr
    {
        B* b = nullptr;
        EXPECT_EQ(nullptr, qret::CastIfPresent<A>(b));
    }
}
TEST(SafeCast, CastIfPresentConst) {
    {
        const auto b = B{};
        const auto* a = qret::CastIfPresent<A>(&b);
        EXPECT_EQ(Type::B, a->type);
        EXPECT_EQ(&b, qret::CastIfPresent<B>(a));
    }
    {
        const auto c = C{};
        const auto* a = qret::CastIfPresent<A>(&c);
        EXPECT_EQ(Type::C, a->type);
        EXPECT_EQ(&c, qret::CastIfPresent<C>(a));
    }
    {
        const auto d = D{};
        const auto* a = qret::CastIfPresent<A>(&d);
        const auto* b = qret::CastIfPresent<B>(&d);
        EXPECT_EQ(Type::D, a->type);
        EXPECT_EQ(Type::D, b->type);
        EXPECT_EQ(&d, qret::CastIfPresent<D>(a));
        EXPECT_EQ(&d, qret::CastIfPresent<D>(b));
    }

    // nullptr
    {
        const B* b = nullptr;
        EXPECT_EQ(nullptr, qret::CastIfPresent<A>(b));
    }
}
TEST(SafeCast, DynCast) {
    {
        auto b = B{};
        auto* a = qret::DynCast<A>(&b);
        EXPECT_EQ(Type::B, a->type);
        EXPECT_EQ(&b, qret::DynCast<B>(a));
        EXPECT_EQ(nullptr, qret::DynCast<C>(a));
        EXPECT_EQ(nullptr, qret::DynCast<D>(a));
    }
    {
        auto c = C{};
        auto* a = qret::DynCast<A>(&c);
        EXPECT_EQ(Type::C, a->type);
        EXPECT_EQ(&c, qret::DynCast<C>(a));
        EXPECT_EQ(nullptr, qret::DynCast<B>(a));
        EXPECT_EQ(nullptr, qret::DynCast<D>(a));
    }
    {
        auto d = D{};
        auto* a = qret::DynCast<A>(&d);
        auto* b = qret::DynCast<B>(&d);
        EXPECT_EQ(Type::D, a->type);
        EXPECT_EQ(Type::D, b->type);
        EXPECT_EQ(&d, qret::DynCast<D>(a));
        EXPECT_EQ(&d, qret::DynCast<D>(b));
        EXPECT_EQ(nullptr, qret::DynCast<C>(a));
    }
}
TEST(SafeCast, DynCastConst) {
    {
        const auto b = B{};
        const auto* a = qret::DynCast<A>(&b);
        EXPECT_EQ(Type::B, a->type);
        EXPECT_EQ(&b, qret::DynCast<B>(a));
        EXPECT_EQ(nullptr, qret::DynCast<C>(a));
        EXPECT_EQ(nullptr, qret::DynCast<D>(a));
    }
    {
        const auto c = C{};
        const auto* a = qret::DynCast<A>(&c);
        EXPECT_EQ(Type::C, a->type);
        EXPECT_EQ(&c, qret::DynCast<C>(a));
        EXPECT_EQ(nullptr, qret::DynCast<B>(a));
        EXPECT_EQ(nullptr, qret::DynCast<D>(a));
    }
    {
        const auto d = D{};
        const auto* a = qret::DynCast<A>(&d);
        const auto* b = qret::DynCast<B>(&d);
        EXPECT_EQ(Type::D, a->type);
        EXPECT_EQ(Type::D, b->type);
        EXPECT_EQ(&d, qret::DynCast<D>(a));
        EXPECT_EQ(&d, qret::DynCast<D>(b));
        EXPECT_EQ(nullptr, qret::DynCast<C>(a));
    }
}
TEST(SafeCast, DynCastIfPresent) {
    {
        auto b = B{};
        auto* a = qret::DynCastIfPresent<A>(&b);
        EXPECT_EQ(Type::B, a->type);
        EXPECT_EQ(&b, qret::DynCastIfPresent<B>(a));
        EXPECT_EQ(nullptr, qret::DynCastIfPresent<C>(a));
        EXPECT_EQ(nullptr, qret::DynCastIfPresent<D>(a));
    }
    {
        auto c = C{};
        auto* a = qret::DynCastIfPresent<A>(&c);
        EXPECT_EQ(Type::C, a->type);
        EXPECT_EQ(&c, qret::DynCastIfPresent<C>(a));
        EXPECT_EQ(nullptr, qret::DynCastIfPresent<B>(a));
        EXPECT_EQ(nullptr, qret::DynCastIfPresent<D>(a));
    }
    {
        auto d = D{};
        auto* a = qret::DynCastIfPresent<A>(&d);
        auto* b = qret::DynCastIfPresent<B>(&d);
        EXPECT_EQ(Type::D, a->type);
        EXPECT_EQ(Type::D, b->type);
        EXPECT_EQ(&d, qret::DynCastIfPresent<D>(a));
        EXPECT_EQ(&d, qret::DynCastIfPresent<D>(b));
        EXPECT_EQ(nullptr, qret::DynCastIfPresent<C>(a));
    }

    // nullptr
    {
        const B* b = nullptr;
        EXPECT_EQ(nullptr, qret::DynCastIfPresent<A>(b));
    }
}
TEST(SafeCast, DynCastIfPresentConst) {
    {
        const auto b = B{};
        const auto* a = qret::DynCastIfPresent<A>(&b);
        EXPECT_EQ(Type::B, a->type);
        EXPECT_EQ(&b, qret::DynCastIfPresent<B>(a));
        EXPECT_EQ(nullptr, qret::DynCastIfPresent<C>(a));
        EXPECT_EQ(nullptr, qret::DynCastIfPresent<D>(a));
    }
    {
        const auto c = C{};
        const auto* a = qret::DynCastIfPresent<A>(&c);
        EXPECT_EQ(Type::C, a->type);
        EXPECT_EQ(&c, qret::DynCastIfPresent<C>(a));
        EXPECT_EQ(nullptr, qret::DynCastIfPresent<B>(a));
        EXPECT_EQ(nullptr, qret::DynCastIfPresent<D>(a));
    }
    {
        const auto d = D{};
        const auto* a = qret::DynCastIfPresent<A>(&d);
        const auto* b = qret::DynCastIfPresent<B>(&d);
        EXPECT_EQ(Type::D, a->type);
        EXPECT_EQ(Type::D, b->type);
        EXPECT_EQ(&d, qret::DynCastIfPresent<D>(a));
        EXPECT_EQ(&d, qret::DynCastIfPresent<D>(b));
        EXPECT_EQ(nullptr, qret::DynCastIfPresent<C>(a));
    }

    // nullptr
    {
        const B* b = nullptr;
        EXPECT_EQ(nullptr, qret::DynCastIfPresent<A>(b));
    }
}
