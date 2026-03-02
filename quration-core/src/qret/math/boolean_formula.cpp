/**
 * @file qret/math/boolean_formula.cpp
 * @brief Boolean formula.
 */

#include "qret/math/boolean_formula.h"

#include <set>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

#include "qret/base/log.h"

namespace qret::math {
std::tuple<Term, Term> Intersection(const Term& lhs, const Term& rhs) {
    auto same = Term();
    auto opposite = Term();

    auto l = lhs.begin();
    auto r = rhs.begin();
    while (l != lhs.end() && r != rhs.end()) {
        if (l->GetVar() == r->GetVar()) {
            if (l->Sign() == r->Sign()) {
                same &= *l;
            } else {
                opposite &= *l;
            }
        }

        // forward iterators
        if (*l <= *r) {
            ++l;
        } else {
            ++r;
        }
    }

    return {same, opposite};
}

void Formula::Minimize() {
    const auto num_vars = RelatedVars().size();
    const auto num_terms = Size();

    for (auto&& term : data_) {
        term.SortAndUnique();
    }

    if (num_vars <= 15) {
        MinimizeByQuineMcCluskey();
    } else if (num_terms <= 10'000) {
        MinimizeByHeuristics();
    } else {
        LOG_WARN("Minimization of large formula is not supported");
    }

    for (auto&& term : data_) {
        term.SortAndUnique();
    }
}

bool MinimizeByHeuristicsImpl(std::vector<Term>& data) {
    const auto num_terms = data.size();
    for (auto i = std::size_t{0}; i < num_terms; ++i) {
        for (auto j = std::size_t{0}; j < num_terms; ++j) {
            if (i == j) {
                continue;
            }

            const auto& lhs = data[i];
            const auto& rhs = data[j];
            const auto [same, opposite] = Intersection(lhs, rhs);

            if (same.Size() == lhs.Size()) {
                // Delete 'rhs'
                std::swap(data[j], data.back());
                data.pop_back();
                return true;
            } else if (same.Size() == rhs.Size()) {
                // Delete 'lhs'
                std::swap(data[i], data.back());
                data.pop_back();
                return true;
            } else if (opposite.Size() == 1 && same.Size() + 1 == lhs.Size()
                       && lhs.Size() == rhs.Size()) {
                if (lhs.Size() == 1) {
                    // All true
                    data.clear();
                    data.emplace_back();
                    return true;
                } else {
                    // Delete opposite from 'lhs' and delete 'rhs'
                    // X = opposite[0]
                    // lhs = same&( X)
                    // rhs = same&(~X)
                    // lhs | rhs = same
                    data[i].Erase(data[i].Find(opposite.Get(0)));
                    std::swap(data[j], data.back());
                    data.pop_back();
                }
                return true;
            }
        }
    }
    auto changed = false;
    return changed;
}

void Formula::MinimizeByHeuristics() {
    while (MinimizeByHeuristicsImpl(data_)) {}
}

std::tuple<ImplicantSet, std::vector<Var>> DivideIntoMinterms(const std::vector<Term>& terms) {
    const auto vars = [&terms]() {
        auto var_set = std::unordered_set<Var>{};
        for (const auto& term : terms) {
            for (const auto& lit : term) {
                var_set.emplace(lit.GetVar());
            }
        }
        return std::vector(var_set.begin(), var_set.end());
    }();
    const auto n_vars = vars.size();

    if (n_vars > 32) {
        throw std::runtime_error(
                "Cannot divide into minimum terms if the number "
                "of variables is larger than 32."
        );
    }

    const auto var_to_id = [&vars, n_vars]() {
        auto var_to_id = std::unordered_map<Var, std::size_t>{};
        for (const auto i : std::views::iota(std::size_t{0}, n_vars)) {
            var_to_id[vars[i]] = i;
        }
        return var_to_id;
    }();

    ImplicantSet min_terms;
    for (const auto& term : terms) {
        auto base = std::uint32_t{0};
        auto var_is_fixed = std::vector<bool>(n_vars, false);
        for (const auto& lit : term) {
            auto id = var_to_id.at(lit.GetVar());
            if (lit.Sign()) {
                base |= std::uint32_t{1} << id;
            }
            var_is_fixed[id] = true;
        }
        auto free_vars = std::vector<std::size_t>{};
        for (const auto id : std::views::iota(std::size_t{0}, n_vars)) {
            if (!var_is_fixed[id]) {
                free_vars.push_back(id);
            }
        }
        const auto n_free_vars = free_vars.size();

        for (const auto s : std::views::iota(std::uint32_t{0}, std::uint32_t{1} << n_free_vars)) {
            auto bit = base;
            for (const auto free_id : std::views::iota(std::size_t{0}, n_free_vars)) {
                if (((s >> free_id) & 1) == 1) {
                    bit |= std::uint32_t{1} << free_vars[free_id];
                }
            }
            min_terms[0].insert(Cube(bit));
        }
    }

    return {min_terms, vars};
}

ImplicantSet GetPrimeImplicants(const ImplicantSet& min_terms) {
    auto current_implicants = min_terms;
    ImplicantSet ret_implicants;

    while (true) {
        bool merged = false;
        ImplicantSet next_implicants;
        ImplicantSet used_implicants;
        for (auto&& [mask, implicants] : current_implicants) {
            while (!implicants.empty()) {
                const auto implicant = *implicants.begin();
                implicants.erase(implicants.begin());

                for (auto i = std::uint32_t{0}; i < 8 * sizeof(mask); ++i) {
                    if ((mask & (1 << i)) > 0) {
                        // 'i' is don't care.
                        continue;
                    }

                    const auto single_bit_diff_cube = Cube(implicant.bits ^ (1 << i), mask);
                    const auto itr = implicants.find(single_bit_diff_cube);

                    if (itr != implicants.end()) {
                        // Found single_bit_diff_cube
                        used_implicants[mask].emplace(implicant);
                        used_implicants[mask].emplace(*itr);
                        merged = true;
                        auto new_cube = Cube(implicant.bits, mask | (1 << i));
                        new_cube.Normalize();
                        next_implicants[mask | (1 << i)].emplace(new_cube);
                        continue;
                    }
                }

                if (!used_implicants[mask].contains(implicant)) {
                    ret_implicants[mask].emplace(implicant);
                }
            }
        }

        std::swap(current_implicants, next_implicants);
        if (!merged) {
            break;
        }
    }

    return ret_implicants;
}

void EraseSetElements(
        std::unordered_set<std::uint32_t>& s,
        const std::unordered_set<std::uint32_t>& d
) {
    if (s.size() <= d.size()) {
        auto itr = s.begin();
        while (itr != s.end()) {
            if (d.contains(*itr)) {
                auto next = std::next(itr);
                s.erase(itr);
                itr = next;
            } else {
                itr++;
            }
        }
    } else {
        for (const auto& x : d) {
            auto itr = s.find(x);
            if (itr != s.end()) {
                s.erase(itr);
            }
        }
    }
}

std::unordered_set<std::size_t> FindCoverSetByPetrickMethod(
        std::vector<std::unordered_set<std::uint32_t>>& table
) {
    if (table.empty()) {
        return {};
    }

    auto petrick_function = std::map<std::uint32_t, std::set<std::size_t>>();
    for (std::size_t i = 0; i < table.size(); i++) {
        for (auto min_term : table[i]) {
            petrick_function[min_term].insert(i);
        }
    }
    auto min_term_list = std::vector<std::uint32_t>();
    for (const auto& [min_term, terms] : petrick_function) {
        min_term_list.emplace_back(min_term);
    }

    // expand petrick function
    auto expanded_formula = std::vector<std::set<std::size_t>>();
    for (const auto& term : petrick_function[min_term_list[0]]) {
        expanded_formula.emplace_back(std::set<std::size_t>{term});
    }
    for (std::size_t i = 1; i < petrick_function.size(); i++) {
        auto new_formula = std::vector<std::set<std::size_t>>();
        for (const auto& term_prod : expanded_formula) {
            for (const auto& term : petrick_function[min_term_list[i]]) {
                new_formula.emplace_back(term_prod);
                new_formula[new_formula.size() - 1].insert(term);
            }
        }
        std::sort(new_formula.begin(), new_formula.end());
        new_formula.erase(std::unique(new_formula.begin(), new_formula.end()), new_formula.end());

        // simplify using X+XY=X
        expanded_formula = std::vector<std::set<std::size_t>>();
        for (const auto& term_i : new_formula) {
            bool is_absorbed = false;
            for (const auto& term_j : new_formula) {
                if (term_i == term_j) {
                    continue;
                }
                if (term_i.size() > term_j.size()
                    && std::includes(term_i.begin(), term_i.end(), term_j.begin(), term_j.end())) {
                    is_absorbed = true;
                    break;
                }
            }
            if (!is_absorbed) {
                expanded_formula.push_back(term_i);
            }
        }
    }

    // find minimum solution
    auto& ans = expanded_formula[0];
    auto min_set_size = expanded_formula[0].size();
    for (const auto& term_prod : expanded_formula) {
        if (term_prod.size() < min_set_size) {
            ans = term_prod;
            min_set_size = term_prod.size();
        }
    }
    auto ret = std::unordered_set<std::size_t>(ans.begin(), ans.end());
    return ret;
}

std::unordered_set<std::size_t> FindCoverSetByGreedySearch(
        std::vector<std::unordered_set<std::uint32_t>>& table
) {
    auto ret = std::unordered_set<std::size_t>();
    while (true) {
        // Find row to cover max elements.
        auto max_itr = table.begin();
        auto max_cover = max_itr->size();
        for (auto itr = table.begin(); itr != table.end(); ++itr) {
            const auto cover = itr->size();
            if (cover > max_cover) {
                max_itr = itr;
                max_cover = cover;
            }
        }

        if (max_cover == 0) {
            break;
        }

        // Add max_itr to cover_set.
        ret.emplace(static_cast<std::size_t>(max_itr - table.begin()));

        // Delete elements in new_cover.
        const auto new_cover = *max_itr;
        *max_itr = {};
        for (auto&& row : table) {
            EraseSetElements(row, new_cover);
        }
    }
    return ret;
}

std::vector<Cube> FindEssentialPrimeImplicants(
        const std::unordered_set<Cube>& min_terms,
        const ImplicantSet& prime_implicants
) {
    const auto num_prime_implicants = [&prime_implicants]() {
        auto sum = std::size_t{0};
        for (const auto& [_, implicants] : prime_implicants) {
            sum += implicants.size();
        }
        return sum;
    }();

    // Rows: prime implicants; Columns: minterms (indices into min_terms).
    auto table = std::vector<std::unordered_set<std::uint32_t>>(
            num_prime_implicants,
            std::unordered_set<std::uint32_t>()
    );

    // Initialize table.
    auto prime_implicant_index = std::size_t{0};
    for (const auto& [_, implicants] : prime_implicants) {
        for (const auto& implicant : implicants) {
            auto& row = table[prime_implicant_index++];

            auto min_term_index = std::uint32_t{0};
            for (const auto& min_term : min_terms) {
                if (implicant.Contains(min_term)) {
                    row.emplace(min_term_index);
                }
                min_term_index++;
            }
        }
    }

    auto petrick_function_size = std::map<std::uint32_t, std::size_t>();
    for (std::size_t i = 0; i < table.size(); i++) {
        for (auto min_term : table[i]) {
            petrick_function_size[min_term]++;
        }
    }
    std::size_t expanded_formula_size = 2e9;
    for (const auto& [_, num_terms] : petrick_function_size) {
        expanded_formula_size /= num_terms;
    }

    // Find essential prime implicants.
    std::unordered_set<std::size_t> indices;
    if (expanded_formula_size > 0) {
        indices = FindCoverSetByPetrickMethod(table);
    } else {
        indices = FindCoverSetByGreedySearch(table);
    }
    // FindCoverSetByExhaustiveSearch();
    // FindCoverSetByPetrickMethod();

    // Collect selected prime implicants in original (flattened) order.
    auto ret = std::vector<Cube>();
    ret.reserve(indices.size());
    prime_implicant_index = std::size_t{0};
    for (const auto& [_, implicants] : prime_implicants) {
        for (const auto& implicant : implicants) {
            if (indices.contains(prime_implicant_index++)) {
                ret.emplace_back(implicant);
            }
        }
    }

    return ret;
}

void Formula::MinimizeByQuineMcCluskey() {
    if (Size() <= 1) {
        return;
    }

    auto [min_terms, vars] = DivideIntoMinterms(data_);
    const auto prime_implicants = GetPrimeImplicants(min_terms);
    const auto min_implicants = FindEssentialPrimeImplicants(min_terms[0], prime_implicants);

    data_.clear();
    for (const auto& implicant : min_implicants) {
        auto term = Term();
        for (auto i = std::uint32_t{0}; i < vars.size(); ++i) {
            const auto var = vars[i];
            const auto c = implicant.Get(static_cast<std::int32_t>(i));
            if (c == '1') {
                term &= Lit(var, true);
            } else if (c == '0') {
                term &= Lit(var, false);
            }
        }

        if (!term.Empty()) {
            data_.emplace_back(term);
        }
    }
}
}  // namespace qret::math
