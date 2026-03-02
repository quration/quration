/**
 * @file qret/math/boolean_formula.h
 * @brief Boolean formula.
 * @details This file defines boolean formula and provides some basic tools to manipulate them.
 */

#ifndef QRET_MATH_BOOLEAN_FORMULA_H
#define QRET_MATH_BOOLEAN_FORMULA_H

#include <algorithm>
#include <bit>
#include <compare>
#include <cstdint>
#include <optional>
#include <ostream>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include "qret/base/json.h"
#include "qret/qret_export.h"

namespace qret::math {
using IdType = std::uint64_t;

namespace {  // NOLINT
constexpr IdType cast_to_int(bool sign) {
    return sign ? 1 : 0;
}
constexpr bool cast_to_bool(IdType x) {
    return x > 0;
}
}  // namespace

// Forward declarations.
struct Lit;

/**
 * @brief Variable class.
 */
struct QRET_EXPORT Var {
    explicit constexpr Var(IdType x) noexcept
        : x{x} {}
    [[nodiscard]] constexpr std::strong_ordering operator<=>(const Var&) const noexcept = default;
    IdType x;
};
inline std::ostream& operator<<(std::ostream& out, const Var& v) {
    return out << v.x;
}

/**
 * @brief Literal class.
 * @details
 * Literal's 'x' of variable:
 * * not 2  -> 4
 * * 2      -> 5
 * * x      -> 2 * x + 1
 * * not x  -> 2 * x
 */
struct QRET_EXPORT Lit {
    explicit constexpr Lit(Var var, bool sign = true) noexcept
        : x{(2 * var.x) + cast_to_int(sign)} {}
    [[nodiscard]] constexpr std::strong_ordering operator<=>(const Lit&) const noexcept = default;
    [[nodiscard]] constexpr bool Sign() const noexcept {
        return cast_to_bool(x & 1u);
    }
    [[nodiscard]] constexpr Var GetVar() const noexcept {
        return Var{x >> 1u};
    }
    [[nodiscard]] constexpr Lit operator~() const noexcept {
        return Lit(x ^ 1);
    }
    IdType x;

private:
    explicit constexpr Lit(IdType x) noexcept
        : x{x} {}
};
inline std::ostream& operator<<(std::ostream& out, const Lit& l) {
    return out << (l.Sign() ? '+' : '-') << l.GetVar();
}
[[nodiscard]] constexpr Lit operator^(Lit lhs, bool rhs) noexcept {
    lhs.x ^= cast_to_int(rhs);
    return lhs;
}
[[nodiscard]] constexpr Lit operator^(bool lhs, Lit rhs) noexcept {
    return rhs ^ lhs;
}

/**
 * @brief Product-term ("cube") with bit-level don't-care support.
 *
 * A cube encodes an implicant over n variables using two bitmasks:
 *  - @c bits : the 0/1 assignment on care positions
 *  - @c mask : positions with 1 are don't-care ('-'); positions with 0 are care
 *
 * Invariants:
 *  - Non-care positions in @c bits are ignored; prefer calling Normalize() after updates.
 */
struct QRET_EXPORT Cube {
    explicit Cube(std::uint32_t bits, std::uint32_t mask = 0)
        : bits{bits}
        , mask{mask} {}
    static Cube FromLiteral(std::uint32_t lit) {
        return Cube(std::uint32_t{1} << lit);
    }

    [[nodiscard]] std::strong_ordering operator<=>(const Cube&) const noexcept = default;

    /**
     * @brief Count of 1s on care positions.
     * @return Number of set bits in (bits & ~mask).
     */
    std::int32_t NumberOf1() const {
        return std::popcount(bits & (~mask));
    }

    /**
     * @brief Whether position @p i is care (not don't-care).
     * @param i Bit position.
     * @return true if care, false if don't-care.
     */
    bool Care(std::int32_t i) const {
        return ((mask >> i) & 1) == 0;
    }

    /**
     * @brief Value of position @p i (raw @c bits).
     * @param i Bit position.
     * @return true if the bit is 1, false if 0.
     * @note Meaningful only when Care(i) is true.
     */
    bool Bit(std::int32_t i) const {
        return ((bits >> i) & 1) == 1;
    }

    /**
     * @brief Get symbolic value at position @p i.
     * @param i Bit position.
     * @return '1' or '0' if care, '-' if don't-care.
     */
    char Get(std::int32_t i) const {
        if (Care(i)) {
            return Bit(i) ? '1' : '0';
        }
        return '-';
    }

    /**
     * @brief Try merging with another cube that differs in exactly one care bit.
     *
     * Two cubes are mergeable iff they have the same @c mask and differ by exactly one bit
     * within care positions. The merged cube sets that differing position to don't-care.
     *
     * @param other Cube to merge with.
     * @return Merged cube if mergeable; std::nullopt otherwise.
     * @note Complexity: O(1) bitwise operations.
     */
    std::optional<Cube> Merge(Cube other) const {
        if (mask != other.mask) {
            return std::nullopt;
        }

        const auto care_bits = bits & (~mask);
        const auto other_care_bits = other.bits & (~mask);
        const auto diff = care_bits ^ other_care_bits;
        if (std::popcount(diff) != 1) {
            return std::nullopt;
        }

        return Cube{bits, mask | diff};
    }

    /**
     * @brief Check whether this cube contains (covers) @p other.
     *
     * A contains B if this cube's don't-care set is a superset of B's don't-care set
     * and assignments on this cube's care positions match B.
     *
     * @param other Cube possibly covered by this.
     * @return true if this cube covers @p other, false otherwise.
     * @note: Complexity O(1) bitwise operations.
     */
    bool Contains(Cube other) const {
        // mask must be larger.
        if ((mask & other.mask) != other.mask) {
            return false;
        }

        const auto care_bits = bits & (~mask);
        const auto other_care_bits = other.bits & (~mask);
        return care_bits == other_care_bits;
    }

    /**
     * @brief Normalize bits by clearing values on don't-care positions.
     * @post bits == (bits & ~mask)
     */
    void Normalize() {
        bits &= ~mask;
    }

    /**
     * @brief 0/1 assignment; values on don't-care positions are ignored.
     */
    std::uint32_t bits;

    /**
     * @brief  1 = don't-care ('-'), 0 = care.
     */
    std::uint32_t mask;
};
inline std::ostream& operator<<(std::ostream& out, const Cube& c) {
    for (auto i = 31; i >= 0; i--) {
        out << c.Get(i);
    }
    return out;
}
/**
 * @brief Set of implicants grouped by identical mask.
 *
 * Key  : mask (1 = don't-care, 0 = care)<br>
 * Value: a set of cubes having that mask.
 *
 * @note Grouping by mask enables O(N * n) style merging within each group.
 * @pre Cube must be hashable and equality comparable to be used in unordered_set.
 */
using ImplicantSet = std::unordered_map<std::uint32_t, std::unordered_set<Cube>>;

/**
 * @brief Compute prime implicants by iterative merging (Quine-McCluskey combine step).
 *
 * Within each mask group, cubes that differ by exactly one care bit are merged by
 * turning that bit into don't-care. Cubes that were not merged in a round become
 * candidates for prime implicants under their current mask. The process repeats until
 * no merges occur.
 *
 * @param min_terms Initial implicants grouped by mask (typically minterms use mask=0).
 * @return Final implicants after the merge process terminates; these are prime
 *         w.r.t. the performed merges (still grouped by mask).
 * @note Complexity Roughly O(Rounds * sum_group(N_g * n)), where n is the bit-width.
 * @note Returned set may include implicants that cover only don't-care minterms;
 *       filter against the ON-set if needed in the caller.
 */
QRET_EXPORT ImplicantSet GetPrimeImplicants(const ImplicantSet& min_terms);

/**
 * @brief Erase all elements of @p d from the set @p s, choosing a direction based on sizes.
 * @param s Target set to mutate.
 * @param d Elements to remove from @p s.
 * @note Complexity O(min(|s|, |d|) * log |s|) expected with unordered_set operations.
 */
QRET_EXPORT void
EraseSetElements(std::unordered_set<std::uint32_t>& s, const std::unordered_set<std::uint32_t>& d);
/**
 * @brief Find minimum cover set by petrick's method
 *
 * Given a table where each row is a set of covered column indices, repeatedly choose
 * the row with the maximum current coverage, add it to the solution, and remove its
 * covered columns from all rows, until no columns remain.
 *
 * @param table Vector of rows; each row is a set of column indices it covers.
 * @return Indices of selected rows.
 */
QRET_EXPORT std::unordered_set<std::size_t> FindCoverSetByPetrickMethod(
        std::vector<std::unordered_set<std::uint32_t>>& table
);

/**
 * @brief Greedy set cover: pick rows that cover the largest number of remaining columns.
 *
 * Given a table where each row is a set of covered column indices, repeatedly choose
 * the row with the maximum current coverage, add it to the solution, and remove its
 * covered columns from all rows, until no columns remain.
 *
 * @param table Vector of rows; each row is a set of column indices it covers.
 * @return Indices of selected rows (greedy solution).
 * @note: Complexity O(R * (R + C)) in a rough sense, where R=rows, C=columns.
 * @note This is a heuristic and need not be optimal.
 */
QRET_EXPORT std::unordered_set<std::size_t> FindCoverSetByGreedySearch(
        std::vector<std::unordered_set<std::uint32_t>>& table
);

/**
 * @brief Build a PI table and select a covering set of prime implicants.
 *
 * The function flattens @p prime_implicants into rows, builds a coverage table
 * against @p min_terms (columns), and then selects a set of rows via greedy set cover.
 *
 * @param min_terms Set of ON-set minterms (as cubes) to be covered.
 * @param prime_implicants Prime implicants grouped by mask.
 * @return A vector of selected cubes that (greedily) cover all minterms.
 * @note Complexity Table building is O(P * M); greedy cover depends on table sparsity.
 * @note "Essential prime implicants" are not explicitly extracted yet; this currently
 *       returns a greedy cover. Petrick's method could replace the greedy step for
 *       exact minimum covers.
 */
QRET_EXPORT std::vector<Cube> FindEssentialPrimeImplicants(
        const std::unordered_set<Cube>& min_terms,
        const ImplicantSet& prime_implicants
);
}  // namespace qret::math

namespace nlohmann {
template <>
struct adl_serializer<qret::math::Var> {
    static void to_json(qret::Json& j, qret::math::Var v) {
        j = v.x;
    }
    static qret::math::Var from_json(const qret::Json& j) {
        return qret::math::Var{j.template get<qret::math::IdType>()};
    }
};
template <>
struct adl_serializer<qret::math::Lit> {
    static void to_json(qret::Json& j, qret::math::Lit l) {
        j = l.x;
    }
    static qret::math::Lit from_json(const qret::Json& j) {
        auto ret = qret::math::Lit(qret::math::Var(0), false);
        ret.x = j.template get<qret::math::IdType>();
        return ret;
    }
};
template <>
struct adl_serializer<qret::math::Cube> {
    static void to_json(qret::Json& j, qret::math::Cube c) {
        j["bits"] = c.bits;
        j["mask"] = c.mask;
    }
    static qret::math::Cube from_json(const qret::Json& j) {
        return qret::math::Cube{
                j["bits"].template get<std::uint32_t>(),
                j["mask"].template get<std::uint32_t>()
        };
    }
};
}  // namespace nlohmann

template <>
struct std::hash<qret::math::Var> {
    std::size_t operator()(const qret::math::Var& v) const {
        return hash<qret::math::IdType>()(v.x);
    }
};
template <>
struct std::hash<qret::math::Lit> {
    std::size_t operator()(const qret::math::Lit& l) const {
        return hash<qret::math::IdType>()(l.x);
    }
};
template <>
struct std::hash<qret::math::Cube> {
    std::size_t operator()(const qret::math::Cube& c) const {
        return hash<qret::math::IdType>()(
                (static_cast<std::uint64_t>(c.bits) << 32) | (static_cast<std::uint64_t>(c.mask))
        );
    }
};

namespace qret::math {
/**
 * @brief Term class.
 * @details Term is a logical conjunction of literals.
 */
class QRET_EXPORT Term {
public:
    template <
            typename... Lits,
            std::enable_if_t<
                    std::conjunction_v<std::is_same<Lit, std::remove_cvref_t<Lits>>...>,
                    bool> = true>
    explicit Term(Lits&&... lits)
        : data_{lits...} {}
    Term(std::uint32_t enabled, std::uint32_t val) {
        for (; enabled > 0; enabled &= enabled - 1) {
            auto x = static_cast<IdType>(std::countr_zero(enabled));
            *this &= Lit(Var(x), (val >> x & 1) == 1);
        }
    }
    [[nodiscard]] std::strong_ordering operator<=>(const Term&) const noexcept = default;

    [[nodiscard]] auto begin() {
        return data_.begin();
    }
    [[nodiscard]] auto begin() const {
        return data_.begin();
    }
    [[nodiscard]] auto cbegin() const {
        return data_.cbegin();
    }
    [[nodiscard]] auto end() {
        return data_.end();
    }
    [[nodiscard]] auto end() const {
        return data_.end();
    }
    [[nodiscard]] auto cend() const {
        return data_.cend();
    }

    /**
     * @brief Normalize Term.
     */
    void SortAndUnique() {
        std::ranges::sort(begin(), end());
        const auto result = std::ranges::unique(begin(), end());
        data_.erase(result.begin(), result.end());
    }

    /**
     * @brief Clear the term if it has a contradicted variable.
     *
     * @return true if cleared.
     * @return false otherwise.
     */
    bool ClearIfContradict() {
        auto is_contradicted = false;
        SortAndUnique();
        for (auto itr = begin(); itr + 1 < end(); ++itr) {
            const auto x = *itr;
            const auto y = *(itr + 1);
            if (x.GetVar() == y.GetVar()) {
                is_contradicted = true;
                break;
            }
        }
        if (is_contradicted) {
            data_.clear();
        }
        return is_contradicted;
    }

    // getter
    [[nodiscard]] bool Empty() const {
        return data_.empty();
    }
    [[nodiscard]] std::size_t Size() const {
        return data_.size();
    }
    [[nodiscard]] Lit Get(std::size_t i) const {
        return data_[i];
    }

    // algorithms
    [[nodiscard]] auto Contains(Lit l) const {
        return std::ranges::find(data_, l) != end();
    }
    [[nodiscard]] auto Find(Lit l) const {
        return std::ranges::find(data_, l);
    }
    void Erase(auto itr) {
        data_.erase(itr);
    }

    // operator overloads
    Term& operator&=(Lit l) {
        data_.emplace_back(l);
        return *this;
    }
    Term& operator&=(const Term& t) {
        for (const auto& l : t) {
            data_.emplace_back(l);
        }
        return *this;
    }

private:
    std::vector<Lit> data_;
};
inline std::ostream& operator<<(std::ostream& out, const Term& t) {
    for (auto i = std::size_t{0}; i < t.Size(); ++i) {
        out << t.Get(i);
        if (i + 1 != t.Size()) {
            out << '&';
        }
    }
    return out;
}
inline void to_json(Json& j, const Term& t) {
    j = Json::array();
    for (const auto& l : t) {
        j.emplace_back(l);
    }
}
inline void from_json(const Json& j, Term& t) {
    for (const auto& tmp : j) {
        t &= tmp.template get<Lit>();
    }
}
[[nodiscard]] inline Term operator&(Lit lhs, Lit rhs) noexcept {
    return Term(lhs, rhs);
}
[[nodiscard]] inline Term operator&(Term lhs, Lit rhs) noexcept {
    lhs &= rhs;
    return lhs;
}
[[nodiscard]] inline Term operator&(Lit lhs, const Term& rhs) noexcept {
    auto ret = Term(lhs);
    ret &= rhs;
    return ret;
}
[[nodiscard]] inline Term operator&(Term lhs, const Term& rhs) noexcept {
    lhs &= rhs;
    return lhs;
}
/**
 * @brief Calculate variable intersection of terms.
 * @note variables in lhs and rhs must be normalized by calling SortAndUnique.
 *
 * @param lhs the first term.
 * @param rhs the second term.
 * @return std::tuple<Term, Term> tuple of ( literals in 'lhs' that are also included in 'rhs' ,
 * literals in whose negations are included in 'rhs' ) .
 */
QRET_EXPORT std::tuple<Term, Term> Intersection(const Term& lhs, const Term& rhs);

/**
 * @brief Disjunctive normal form (DNF) Formula class.
 * @details Formula is a logical disjunction of terms.
 */
class QRET_EXPORT Formula {
public:
    template <
            typename... Terms,
            std::enable_if_t<
                    std::conjunction_v<std::is_same<Term, std::remove_cvref_t<Terms>>...>,
                    bool> = true>
    explicit Formula(Terms&&... terms)
        : data_{terms...} {
        if (data_.empty()) {}
    }
    [[nodiscard]] std::strong_ordering operator<=>(const Formula&) const noexcept = default;

    [[nodiscard]] auto begin() {
        return data_.begin();
    }
    [[nodiscard]] auto begin() const {
        return data_.begin();
    }
    [[nodiscard]] auto cbegin() const {
        return data_.cbegin();
    }
    [[nodiscard]] auto end() {
        return data_.end();
    }
    [[nodiscard]] auto end() const {
        return data_.end();
    }
    [[nodiscard]] auto cend() const {
        return data_.cend();
    }

    // getter
    [[nodiscard]] bool Empty() const {
        return data_.empty();
    }
    [[nodiscard]] std::size_t Size() const {
        return data_.size();
    }
    [[nodiscard]] const Term& Get(std::size_t i) const {
        return data_[i];
    }
    [[nodiscard]] std::unordered_set<Var> RelatedVars() const {
        auto ret = std::unordered_set<Var>();
        for (const auto& term : *this) {
            for (const auto lit : term) {
                ret.insert(lit.GetVar());
            }
        }
        return ret;
    }

    /**
     * @brief Minimize terms.
     */
    void Minimize();
    /**
     * @brief Minimize terms using heuristic methods.
     */
    void MinimizeByHeuristics();
    /**
     * @brief Minimize terms by Quine-McCluskey algorithm.
     * @todo Implement.
     */
    void MinimizeByQuineMcCluskey();

    // operator overloads
    Formula& operator&=(Lit l) {
        if (Empty()) {
            data_.emplace_back(l);
        } else {
            for (auto&& term : *this) {
                term &= l;
            }
        }
        return *this;
    }
    Formula& operator|=(const Term& term) {
        data_.emplace_back(term);
        return *this;
    }
    Formula& operator|=(const Formula& f) {
        data_.reserve(Size() + f.Size());
        std::ranges::copy(f, std::back_inserter(data_));
        return *this;
    }

private:
    std::vector<Term> data_;
};
inline std::ostream& operator<<(std::ostream& out, const Formula& f) {
    for (auto i = std::size_t{0}; i < f.Size(); ++i) {
        out << '(' << f.Get(i) << ')';
        if (i + 1 != f.Size()) {
            out << '|';
        }
    }
    return out;
}
inline void to_json(Json& j, const Formula& f) {
    j = Json::array();
    for (const auto& t : f) {
        j.emplace_back(t);
    }
}
inline void from_json(const Json& j, Formula& f) {
    for (const auto& tmp : j) {
        f |= tmp.template get<Term>();
    }
}
[[nodiscard]] inline Formula operator&(Formula lhs, Lit rhs) noexcept {
    lhs &= rhs;
    return lhs;
}
[[nodiscard]] inline Formula operator|(const Term& lhs, const Term& rhs) noexcept {
    auto ret = Formula(lhs);
    ret |= rhs;
    return ret;
}
[[nodiscard]] inline Formula operator|(Formula lhs, const Term& rhs) noexcept {
    lhs |= rhs;
    return lhs;
}
[[nodiscard]] inline Formula operator|(const Term& lhs, const Formula& rhs) noexcept {
    auto ret = Formula(lhs);
    ret |= rhs;
    return ret;
}
[[nodiscard]] inline Formula operator|(Formula lhs, const Formula& rhs) noexcept {
    lhs |= rhs;
    return lhs;
}

}  // namespace qret::math

#endif  // QRET_MATH_BOOLEAN_FORMULA_H
