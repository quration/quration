/**
 * @file qret/base/list.h
 * @brief List operators.
 * @details This file defines utility functions for operating on std::list.
 */

#ifndef QRET_BASE_LIST_H
#define QRET_BASE_LIST_H

#include <list>
#include <memory>

namespace qret {
/**
 * @brief Swap two elements in a std::list in constant time (O(1)) without copying or moving them.
 *
 * This function swaps the positions of the nodes referenced by @p lhs and @p rhs
 * using std::list::splice. The elements themselves are neither copied nor moved;
 * only the internal links of the list are adjusted. As a result, all iterators
 * remain valid, except for those pointing to erased nodes (none are erased here).
 *
 * @tparam T Value type of the list.
 * @tparam Allocator Allocator type of the list.
 * @param l   The list containing both elements to be swapped.
 * @param lhs Iterator pointing to the first element to swap.
 * @param rhs Iterator pointing to the second element to swap.
 *
 * @note Both @p lhs and @p rhs must be valid iterators into @p l and must not be equal to end().
 * @note Complexity: O(1).
 * @note No element is copied or moved.
 */
template <typename T, typename Allocator = std::allocator<T>>
void SwapListNodes(
        std::list<T, Allocator>& l,
        typename std::list<T, Allocator>::iterator lhs,
        typename std::list<T, Allocator>::iterator rhs
) noexcept {
    if (lhs == rhs) {
        return;
    }

    auto lhs_next = std::next(lhs);
    auto rhs_next = std::next(rhs);

    // Adjacent case: lhs immediately before rhs
    if (lhs_next == rhs) {
        // Move rhs before lhs
        l.splice(lhs, l, rhs);
        return;
    }

    // Adjacent case: rhs immediately before lhs
    if (rhs_next == lhs) {
        // Move lhs before rhs
        l.splice(rhs, l, lhs);
        return;
    }

    // Non-adjacent case:
    l.splice(lhs, l, rhs);
    l.splice(rhs_next, l, lhs);
}
}  // namespace qret

#endif  // QRET_BASE_LIST_H
