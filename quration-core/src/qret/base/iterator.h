/**
 * @file qret/base/iterator.h
 * @brief Iterator of list.
 * @details This file defines a iterator of std::list<std::unique_ptr<T>>.
 */

#ifndef QRET_BASE_ITERATOR_H
#define QRET_BASE_ITERATOR_H

#include <cstddef>
#include <iterator>
#include <list>
#include <memory>
#include <type_traits>

namespace qret {
namespace impl {
template <typename T, bool IsConst, bool IsReverse>
struct ListIteratorTraits;
template <typename T, bool IsReverse>
struct ListIteratorTraits<T, false, IsReverse> {
    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using iterator = std::conditional_t<
            IsReverse,
            typename std::list<std::unique_ptr<T>>::reverse_iterator,
            typename std::list<std::unique_ptr<T>>::iterator>;
};
template <typename T, bool IsReverse>
struct ListIteratorTraits<T, true, IsReverse> {
    using value_type = T;
    using pointer = const T*;
    using reference = const T&;
    using iterator = std::conditional_t<
            IsReverse,
            typename std::list<std::unique_ptr<T>>::const_reverse_iterator,
            typename std::list<std::unique_ptr<T>>::const_iterator>;
};
}  // namespace impl

/**
 * @brief Iterator class of std::list<std::unique_ptr<T>> .
 */
template <typename T, bool IsConst, bool IsReverse = false>
class ListIterator {
    using Traits = impl::ListIteratorTraits<T, IsConst, IsReverse>;

public:
    using self_type = ListIterator;
    using value_type = Traits::value_type;
    using pointer = Traits::pointer;
    using reference = Traits::reference;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::bidirectional_iterator_tag;

    ListIterator() = default;
    explicit ListIterator(Traits::iterator itr)
        : itr_{itr} {}

    self_type& operator++() {  // Prefix increment
        ++itr_;
        return *this;
    }
    self_type operator++(int) {  // Postfix increment
        auto ret = *this;
        ++itr_;
        return ret;
    }
    self_type& operator--() {  // Prefix decrement
        --itr_;
        return *this;
    }
    self_type operator--(int) {  // Postfix decrement
        auto ret = *this;
        --itr_;
        return ret;
    }
    /**
     * @brief Returns reference (T& or const T&)
     * @details The default iterator of std::list<std::unique_ptr<T>> returns std::unique_ptr<T>& or
     * const std::unique_ptr<T>& when referencing, but this iterator class returns T& or const T&.
     *
     * @return reference
     */
    reference operator*() const noexcept {
        return **itr_;
    }
    pointer operator->() const noexcept {
        return itr_->get();
    }

    bool operator==(const ListIterator&) const = default;
    bool operator!=(const ListIterator&) const = default;

    pointer GetPointer() const {
        return itr_->get();
    }
    Traits::iterator GetRaw() const {
        return itr_;
    }

private:
    Traits::iterator itr_;
};
}  // namespace qret

#endif  // QRET_BASE_ITERATOR_H
