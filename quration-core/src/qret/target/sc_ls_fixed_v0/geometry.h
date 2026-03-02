/**
 * @file qret/target/sc_ls_fixed_v0/geometry.h
 * @brief Basic vector classes.
 */

#ifndef QRET_TARGET_SC_LS_FIXED_V0_GEOMETRY_H
#define QRET_TARGET_SC_LS_FIXED_V0_GEOMETRY_H

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <nlohmann/adl_serializer.hpp>

#include <bitset>
#include <type_traits>

#include "qret/base/json.h"
#include "qret/qret_export.h"

namespace qret::sc_ls_fixed_v0 {
/**
 * @brief 2D vector class with integer elements.
 */
template <typename T>
struct QRET_EXPORT Vec2D {
    T x = 0;
    T y = 0;

    static Vec2D<T> Right() {
        return {1, 0};
    }
    static Vec2D<T> Left() {
        return {-1, 0};
    }
    static Vec2D<T> Up() {
        return {0, 1};
    }
    static Vec2D<T> Down() {
        return {0, -1};
    }

    /**
     * @brief Rotate the vector 90 degrees clockwise around the origin.
     */
    constexpr Vec2D& Rot90() {
        std::tie(x, y) = std::pair<T, T>{-y, x};
        return *this;
    }
    /**
     * @brief Rotate the vector 180 degrees clockwise around the origin.
     */
    constexpr Vec2D& Rot180() {
        std::tie(x, y) = std::pair<T, T>{-x, -y};
        return *this;
    }
    /**
     * @brief Rotate the vector 270 degrees clockwise around the origin.
     */
    constexpr Vec2D& Rot270() {
        std::tie(x, y) = std::pair<T, T>{y, -x};
        return *this;
    }
    [[nodiscard]] std::string ToString() const {
        return fmt::format("({},{})", x, y);
    }

    [[nodiscard]] T Abs() const {
        return std::abs(x) + std::abs(y);
    }

    constexpr auto operator<=>(const Vec2D&) const = default;

    constexpr Vec2D operator+() const {
        return {x, y};
    }
    constexpr Vec2D operator-() const {
        return {-x, -y};
    };
    constexpr Vec2D& operator+=(const Vec2D& rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }
    constexpr Vec2D& operator-=(const Vec2D& rhs) {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }
    constexpr Vec2D& operator*=(const T m) {
        x *= m;
        y *= m;
        return *this;
    }
};
/**
 * @brief Calculate the inner product of two 2D vectors.
 *
 * @param lhs the first vector.
 * @param rhs the second vector.
 * @return T inner product.
 */
template <typename T>
constexpr T InnerProduct(const Vec2D<T>& lhs, const Vec2D<T>& rhs) {
    return (lhs.x * rhs.x) + (lhs.y * rhs.y);
}
/**
 * @brief Calculate the cross product of two 2D vectors.
 *
 * @param lhs the first vector.
 * @param rhs the second vector.
 * @return T cross product.
 */
template <typename T>
constexpr T CrossProduct(const Vec2D<T>& lhs, const Vec2D<T>& rhs) {
    return (lhs.x * rhs.y) - (lhs.y * rhs.x);
}
template <typename T>
constexpr Vec2D<T> operator+(const Vec2D<T>& lhs, const Vec2D<T>& rhs) {
    auto ret = lhs;
    ret += rhs;
    return ret;
}
template <typename T>
constexpr Vec2D<T> operator-(const Vec2D<T>& lhs, const Vec2D<T>& rhs) {
    auto ret = lhs;
    ret -= rhs;
    return ret;
}
template <typename T>
constexpr Vec2D<T> operator*(const T m, const Vec2D<T>& v) {
    auto ret = v;
    ret *= m;
    return ret;
}
template <typename T>
constexpr Vec2D<T> operator*(const Vec2D<T>& v, const T m) {
    auto ret = v;
    ret *= m;
    return ret;
}
template <typename T>
inline std::ostream& operator<<(std::ostream& out, const Vec2D<T>& v) {
    return out << v.ToString();
}
template <typename T>
inline void to_json(Json& j, const Vec2D<T>& v) {
    j = {v.x, v.y};
}
template <typename T>
inline void from_json(const Json& j, Vec2D<T>& v) {
    j.at(0).get_to(v.x);
    j.at(1).get_to(v.y);
}
/**
 * @brief Check if three points are collinear and ordered.
 *
 * @details This function determines whether three given points 'a', 'b', and 'c' are collinear and
 * if point 'b' lies between points 'a' and 'c' in the specified order.
 *
 * @param a the first point.
 * @param b the second point.
 * @param c the third point.
 * @return true if the points are collinear and point 'b' lies between points 'a' and 'c' in the
 * given order.
 * @return false otherwise.
 */
template <typename T>
constexpr bool AreCollinearOrdered(const Vec2D<T>& a, const Vec2D<T>& b, const Vec2D<T>& c) {
    // Calculate cross product of 'a - b' and 'a - c'.
    if (CrossProduct(a - b, a - c) != 0) {
        return false;
    }

    // Check if point 'b' lies between points 'a' and 'c' in the given order.
    const auto x_is_in_order = (std::min(a.x, c.x) <= b.x) && (b.x <= std::max(a.x, c.x));
    const auto y_is_in_order = (std::min(a.y, c.y) <= b.y) && (b.y <= std::max(a.y, c.y));
    return x_is_in_order && y_is_in_order;
}
/**
 * @brief Check if three points are aligned on the x-axis and ordered.
 *
 * @details This function determines whether three given points 'a', 'b', and 'c' are aligned on the
 * x-axis and if point 'b' lies between points 'a' and 'c' in the specified order.
 *
 * @param a the first point.
 * @param b the second point.
 * @param c the third point.
 * @return true if the points are aligned on the x-axis and point 'b' lies between points 'a' and
 * 'c' in the given order.
 * @return false otherwise.
 */
template <typename T>
constexpr bool AreOnXAxisOrdered(const Vec2D<T>& a, const Vec2D<T>& b, const Vec2D<T>& c) {
    // Check if all points have the same 'y'.
    if (a.y != b.y || a.y != c.y) {
        return false;
    }
    // Check if point 'b' lies between points 'a' and 'c' in the given order.
    const auto x_is_in_order = (std::min(a.x, c.x) <= b.x) && (b.x <= std::max(a.x, c.x));
    return x_is_in_order;
}
/**
 * @brief Check if three points are aligned on the y-axis and ordered.
 *
 * @details This function determines whether three given points 'a', 'b', and 'c' are aligned on the
 * y-axis and if point 'b' lies between points 'a' and 'c' in the specified order.
 *
 * @param a the first point.
 * @param b the second point.
 * @param c the third point.
 * @return true if the points are aligned on the y-axis and point 'b' lies between points 'a' and
 * 'c' in the given order.
 * @return false otherwise.
 */
template <typename T>
constexpr bool AreOnYAxisOrdered(const Vec2D<T>& a, const Vec2D<T>& b, const Vec2D<T>& c) {
    // Check if all points have the same 'x'.
    if (a.x != b.x || a.x != c.x) {
        return false;
    }
    // Check if point 'b' lies between points 'a' and 'c' in the given order.
    const auto y_is_in_order = (std::min(a.y, c.y) <= b.y) && (b.y <= std::max(a.y, c.y));
    return y_is_in_order;
}
/**
 * @brief 3D vector class with integer elements.
 */
template <typename T>
struct QRET_EXPORT Vec3D {
    T x = 0;
    T y = 0;
    T z = 0;

    [[nodiscard]] constexpr Vec2D<T> XY() const {
        return {x, y};
    }
    [[nodiscard]] std::string ToString() const {
        return fmt::format("({},{},{})", x, y, z);
    }

    [[nodiscard]] T Abs() const {
        return std::abs(x) + std::abs(y) + std::abs(z);
    }

    constexpr auto operator<=>(const Vec3D&) const = default;

    constexpr Vec3D& operator+=(const Vec2D<T>& rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }
    constexpr Vec3D& operator-=(const Vec2D<T>& rhs) {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }
};
template <typename T>
constexpr Vec3D<T> operator+(const Vec3D<T>& lhs, const Vec2D<T>& rhs) {
    auto ret = lhs;
    ret += rhs;
    return ret;
}
template <typename T>
constexpr Vec3D<T> operator-(const Vec3D<T>& lhs, const Vec2D<T>& rhs) {
    auto ret = lhs;
    ret -= rhs;
    return ret;
}
template <typename T>
inline std::ostream& operator<<(std::ostream& out, const Vec3D<T>& v) {
    return out << v.ToString();
}
template <typename T>
inline void to_json(Json& j, const Vec3D<T>& v) {
    j = {v.x, v.y, v.z};
}
template <typename T>
inline void from_json(const Json& j, Vec3D<T>& v) {
    j.at(0).get_to(v.x);
    j.at(1).get_to(v.y);
    j.at(2).get_to(v.z);
}
/**
 * @brief Check if three points are aligned on the x-axis and ordered.
 *
 * @details This function determines whether three given points 'a', 'b', and 'c' are aligned on the
 * x-axis and if point 'b' lies between points 'a' and 'c' in the specified order.
 *
 * @param a the first point.
 * @param b the second point.
 * @param c the third point.
 * @return true if the points are aligned on the x-axis and point 'b' lies between points 'a' and
 * 'c' in the given order.
 * @return false otherwise.
 */
template <typename T>
constexpr bool AreOnXAxisOrdered(const Vec3D<T>& a, const Vec3D<T>& b, const Vec3D<T>& c) {
    // Check if all points have the same 'y'.
    if (a.y != b.y || a.y != c.y) {
        return false;
    }
    // Check if all points have the same 'z'.
    if (a.z != b.z || a.z != c.z) {
        return false;
    }
    // Check if point 'b' lies between points 'a' and 'c' in the given order.
    const auto x_is_in_order = (std::min(a.x, c.x) <= b.x) && (b.x <= std::max(a.x, c.x));
    return x_is_in_order;
}
/**
 * @brief Check if three points are aligned on the y-axis and ordered.
 *
 * @details This function determines whether three given points 'a', 'b', and 'c' are aligned on the
 * y-axis and if point 'b' lies between points 'a' and 'c' in the specified order.
 *
 * @param a the first point.
 * @param b the second point.
 * @param c the third point.
 * @return true if the points are aligned on the y-axis and point 'b' lies between points 'a' and
 * 'c' in the given order.
 * @return false otherwise.
 */
template <typename T>
constexpr bool AreOnYAxisOrdered(const Vec3D<T>& a, const Vec3D<T>& b, const Vec3D<T>& c) {
    // Check if all points have the same 'x'.
    if (a.x != b.x || a.x != c.x) {
        return false;
    }
    // Check if all points have the same 'z'.
    if (a.z != b.z || a.z != c.z) {
        return false;
    }
    // Check if point 'b' lies between points 'a' and 'c' in the given order.
    const auto y_is_in_order = (std::min(a.y, c.y) <= b.y) && (b.y <= std::max(a.y, c.y));
    return y_is_in_order;
}
template <typename T, template <typename> typename Vec>
constexpr std::array<Vec<T>, 2> GetHorizontalNeighbors(const Vec<T>& x) {
    return {x + Vec2D<T>::Right(), x + Vec2D<T>::Left()};
}
template <typename T, template <typename> typename Vec>
constexpr std::array<Vec<T>, 2> GetVerticalNeighbors(const Vec<T>& x) {
    return {x + Vec2D<T>::Up(), x + Vec2D<T>::Down()};
}
template <typename T, template <typename> typename Vec>
constexpr std::array<Vec<T>, 4> GetNeighbors(const Vec<T>& x) {
    return {x + Vec2D<T>::Right(), x + Vec2D<T>::Left(), x + Vec2D<T>::Up(), x + Vec2D<T>::Down()};
}
}  // namespace qret::sc_ls_fixed_v0

template <typename T>
struct std::hash<::qret::sc_ls_fixed_v0::Vec2D<T>> {  // NOLINT
    std::size_t operator()(const ::qret::sc_ls_fixed_v0::Vec2D<T>& v) const {
        using UT = std::make_unsigned_t<T>;
        constexpr auto W = 8 * sizeof(T);

        UT ux, uy;  // NOLINT
        std::memcpy(&ux, &v.x, sizeof(T));
        std::memcpy(&uy, &v.y, sizeof(T));
        auto bx = std::bitset<3 * W>(ux);
        auto by = std::bitset<3 * W>(uy);
        return std::hash<std::bitset<3 * W>>()(bx ^ (by << W));
    }
};
template <typename T>
struct fmt::formatter<::qret::sc_ls_fixed_v0::Vec2D<T>> : ostream_formatter {};
template <typename T>
struct std::hash<::qret::sc_ls_fixed_v0::Vec3D<T>> {  // NOLINT
    std::size_t operator()(const ::qret::sc_ls_fixed_v0::Vec3D<T>& v) const {
        using UT = std::make_unsigned_t<T>;
        constexpr auto W = 8 * sizeof(T);

        UT ux, uy, uz;  // NOLINT
        std::memcpy(&ux, &v.x, sizeof(T));
        std::memcpy(&uy, &v.y, sizeof(T));
        std::memcpy(&uz, &v.z, sizeof(T));
        auto bx = std::bitset<3 * W>(ux);
        auto by = std::bitset<3 * W>(uy);
        auto bz = std::bitset<3 * W>(uz);
        return std::hash<std::bitset<3 * W>>()(bx ^ (by << W) ^ (bz << (2 * W)));
    }
};
template <typename T>
struct fmt::formatter<::qret::sc_ls_fixed_v0::Vec3D<T>> : ostream_formatter {};

#endif  // QRET_TARGET_SC_LS_FIXED_V0_GEOMETRY_H
