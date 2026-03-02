/**
 * @file qret/base/cast.h
 * @brief Cast.
 * @details This file defines LLVM-like cast functions for tag-based polymorphism.
 * `To::ClassOf(from)` is required to define.
 */

#ifndef QRET_BASE_CAST_H
#define QRET_BASE_CAST_H

#include <cassert>

namespace qret {
/**
 * @brief Two classes which has relation on tag-based polymorphism (`To::ClassOf(const From*`) is
 * defined).
 */
template <class To, class From>
concept ClassOfIsDefined = requires(const From* from) { To::ClassOf(from); };

/**
 * @brief Return whether `from` object can be regarded as `To` type instance.
 * @details The following is required and checked:
 * - `from` is not null
 */
template <class To, class From>
#if !defined(_MSC_VER)
    requires ClassOfIsDefined<To, From>
#endif
bool IsA(const From* from) {
    assert(from);
    return To::ClassOf(from);
}

/**
 * @brief Return whether `from` is not null and it can be regarded as `To` type instance.
 */
template <class To, class From>
#if !defined(_MSC_VER)
    requires ClassOfIsDefined<To, From>
#endif
bool IsAAndPresent(const From* from) {
    if (!from) {
        return false;
    }
    return IsA<To>(from);
}

/**
 * @brief Safely cast `from` object to `To` type instance.
 * @details The following is required and checked:
 * - `from` is not null
 * - `from` can be regarded as `To` type instance
 */
template <class To, class From>
#if !defined(_MSC_VER)
    requires ClassOfIsDefined<To, From>
#endif
const To* Cast(const From* from) {
    assert(from);
    assert(IsA<To>(from));
    return static_cast<const To*>(from);
}
/**
 * @brief Safely cast `from` object to `To` type instance.
 * @details The following is required and checked:
 * - `from` is not null
 * - `from` can be regarded as `To` type instance
 */
template <class To, class From>
#if !defined(_MSC_VER)
    requires ClassOfIsDefined<To, From>
#endif
To* Cast(From* from) {
    assert(from);
    assert(IsA<To>(from));
    return static_cast<To*>(from);
}

/**
 * @brief Safely cast `from` object to `To` type instance. If `from` cannot be regarded as `To`,
 * null is returned.
 * @details The following is required and checked:
 * - `from` is not null
 */
template <class To, class From>
#if !defined(_MSC_VER)
    requires ClassOfIsDefined<To, From>
#endif
const To* DynCast(const From* from) {
    assert(from);
    if (!IsA<To>(from)) {
        return nullptr;
    }
    return static_cast<const To*>(from);
}
/**
 * @brief Safely cast `from` object to `To` type instance. If `from` cannot be regarded as `To`,
 * null is returned.
 * @details The following is required and checked:
 * - `from` is not null
 */
template <class To, class From>
#if !defined(_MSC_VER)
    requires ClassOfIsDefined<To, From>
#endif
To* DynCast(From* from) {
    assert(from);
    if (!IsA<To>(from)) {
        return nullptr;
    }
    return static_cast<To*>(from);
}

/**
 * @brief Safely cast `from` object to `To` type instance. If `from` is null,
 * null is returned.
 * @details The following is required and checked:
 * - `from` is null or can be regarded as `To` type instance
 */
template <class To, class From>
#if !defined(_MSC_VER)
    requires ClassOfIsDefined<To, From>
#endif
const To* CastIfPresent(const From* from) {
    if (!from) {
        return nullptr;
    }
    assert(IsA<To>(from));
    return static_cast<const To*>(from);
}
/**
 * @brief Safely cast `from` object to `To` type instance. If `from` is null,
 * null is returned.
 * @details The following is required and checked:
 * - `from` is null or can be regarded as `To` type instance
 */
template <class To, class From>
#if !defined(_MSC_VER)
    requires ClassOfIsDefined<To, From>
#endif
To* CastIfPresent(From* from) {
    if (!from) {
        return nullptr;
    }
    assert(IsA<To>(from));
    return static_cast<To*>(from);
}

/**
 * @brief Safely cast `from` object to `To` type instance. If `from` is null or cannot be regarded
 * as `To`, null is returned.
 */
template <class To, class From>
#if !defined(_MSC_VER)
    requires ClassOfIsDefined<To, From>
#endif
const To* DynCastIfPresent(const From* from) {
    if (!from) {
        return nullptr;
    }
    if (!IsA<To>(from)) {
        return nullptr;
    }
    return static_cast<const To*>(from);
}
/**
 * @brief Safely cast `from` object to `To` type instance. If `from` is null or cannot be regarded
 * as `To`, null is returned.
 */
template <class To, class From>
#if !defined(_MSC_VER)
    requires ClassOfIsDefined<To, From>
#endif
To* DynCastIfPresent(From* from) {
    if (!from) {
        return nullptr;
    }
    if (!IsA<To>(from)) {
        return nullptr;
    }
    return static_cast<To*>(from);
}
}  // namespace qret

#endif  // QRET_BASE_CAST_H
