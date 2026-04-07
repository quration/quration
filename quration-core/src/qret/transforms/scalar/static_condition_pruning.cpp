/**
 * @file qret/transforms/scalar/static_condition_pruning.cpp
 * @brief Prune static branch/switch instructions.
 */

#include "qret/transforms/scalar/static_condition_pruning.h"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <ostream>
#include <random>
#include <stdexcept>
#include <unordered_map>

#include "qret/base/cast.h"
#include "qret/base/log.h"
#include "qret/base/option.h"
#include "qret/base/type.h"
#include "qret/ir/basic_block.h"
#include "qret/ir/instruction.h"
#include "qret/ir/instructions.h"
#include "qret/ir/value.h"

namespace qret::ir {
namespace {
static auto X = RegisterPass<StaticConditionPruningPass>(
        "StaticConditionPruningPass",
        "ir::static_condition_pruning"
);

static Opt<std::uint64_t> SeedOfStaticConditionPruningPass(
        "ir-static-condition-pruning-seed",
        0,
        "Seed of StaticConditionPruningPass",
        OptionHidden::Hidden
);
}  // namespace

/*
 * Compile-time behavior:
 * - This pass rewrites control-flow edges only when the condition is provably
 *   constant for the current block.
 * - We keep a per-basic-block abstract map:
 *      RegisterId -> {True, False, Dynamic}
 *   "Dynamic" means the value cannot be relied upon after merge or side-effectful ops.
 * - The pass does not model quantum amplitudes. It only interprets deterministic
 *   classical updates to decide branch/switch reachability.
 * - The transformation is therefore a conservative constant-folding pass over CFG:
 *   sound (never removes feasible paths), but not complete (may leave more dynamic cases).
 */

enum class RegisterState : std::uint8_t {
    True,
    False,
    Dynamic,
};

std::ostream& operator<<(std::ostream& out, RegisterState state) {
    switch (state) {
        case RegisterState::True:
            out << "true";
            return out;
        case RegisterState::False:
            out << "false";
            return out;
        case RegisterState::Dynamic:
            out << "dynamic";
            return out;
        default:
            break;
    }
    return out;
}

void EraseDynamicRegisterState(std::unordered_map<std::uint64_t, RegisterState>& mp) {
    // Dynamic means "known to vary or unknown."
    // We remove these entries completely so the map contains only predicates
    // that are safe as compile-time constants.
    for (auto it = mp.begin(); it != mp.end();) {
        if (it->second == RegisterState::Dynamic) {
            it = mp.erase(it);
        } else {
            ++it;
        }
    }
}

void SelectNextBranch(BasicBlock* current, BasicBlock* next) {
    // Re-materialize current as an unconditional edge to the proven target.
    // We fully replace the terminator so we don't keep stale CFG metadata
    // from the old conditional form.
    RemoveFromParent(current->GetTerminator());
    BranchInst::Create(next, current);

    // Clear successors.
    current->ClearSuccessors();

    // Set dependency: current -> next
    current->AddEdge(next);
}

void InitializeStaticRegisterFromPred(
        std::unordered_map<const BasicBlock*, std::unordered_map<std::uint64_t, RegisterState>>&
                static_register_per_bb,
        BasicBlock* bb
) {
    // Merge state from all predecessors.
    // A register is static at bb entry only if every incoming predecessor proves
    // the *same* value.
    // If one predecessor is missing the register or disagrees, that register
    // cannot be assumed static after the merge.
    auto& static_register = static_register_per_bb.at(bb);

    auto first = true;
    for (auto itr = bb->pred_begin(); itr != bb->pred_end(); ++itr) {
        const auto* pred = *itr;

        if (!static_register_per_bb.contains(pred)) {
            throw std::logic_error("static_register_per_bb must contain pred.");
        }

        const auto& pred_static_register = static_register_per_bb.at(pred);
        if (first) {
            // Candidate set starts from first predecessor's proof.
            // Every other predecessor can only erase from this set.
            static_register = pred_static_register;
            first = false;
        } else {
            for (auto itr = static_register.begin(); itr != static_register.end();) {
                if (!pred_static_register.contains(itr->first)) {
                    // Absent in one path => unknown on merge.
                    itr->second = RegisterState::Dynamic;
                    ++itr;
                    continue;
                }
                if (itr->second != pred_static_register.at(itr->first)) {
                    // Conflicting proofs => cannot prune on this register.
                    itr->second = RegisterState::Dynamic;
                }
                ++itr;
            }
        }
    }

    EraseDynamicRegisterState(static_register);
}

void CalculateStaticRegisterInBB(
        std::unordered_map<std::uint64_t, RegisterState>& static_register,
        const BasicBlock* bb,
        std::mt19937_64& engine
) {
    // Lightweight abstract interpreter:
    // scans the block once in order and updates the known compile-time state.
    for (const auto& inst : *bb) {
        if (const auto* tmp = DynCast<MeasurementInst>(&inst)) {
            // Register of measurement target not static (dynamic).
            // Runtime result depends on qubit state, so compile-time certainty is lost.
            const auto reg = tmp->GetRegister();
            if (static_register.contains(reg.id)) {
                static_register.erase(reg.id);
            }
        } else if (const auto* tmp = DynCast<CallInst>(&inst)) {
            // Any generic call is treated as potentially side-effectful on outputs.
            // Conservatively mark all outputs as dynamic.
            const auto& output = tmp->GetOutput();

            for (const auto r : output) {
                static_register[r.id] = RegisterState::Dynamic;
            }
        } else if (const auto* tmp = DynCast<CallCFInst>(&inst)) {
            // If all inputs of classical register are static, outputs are also static.
            // Otherwise, all outputs are dynamic.
            // We therefore use an all-inputs-static precondition before
            // evaluating the pure function and writing outputs.
            const auto& input = tmp->GetInput();
            const auto& output = tmp->GetOutput();

            const auto static_function =
                    std::ranges::all_of(input, [&static_register](const auto& r) {
                        return static_register.contains(r.id);
                    });

            if (static_function) {
                auto input_vec = std::vector<bool>(input.size(), false);
                auto output_vec = std::vector<bool>(output.size(), false);

                for (auto i = std::size_t{0}; i < input.size(); ++i) {
                    input_vec[i] = static_register[input[i].id] == RegisterState::True;
                }

                tmp->GetFunction()(input_vec, output_vec);

                // Set output.
                for (auto i = std::size_t{0}; i < output.size(); ++i) {
                    static_register[output[i].id] =
                            output_vec[i] ? RegisterState::True : RegisterState::False;
                }
            } else {
                // Set output as dynamic.
                for (const auto& r : output) {
                    static_register[r.id] = RegisterState::Dynamic;
                }
            }
        } else if (const auto* tmp = DynCast<DiscreteDistInst>(&inst)) {
            // A discrete distribution is a randomized classical instruction.
            // At compile time we use the pass seed and pick one concrete sample so
            // the generated IR can be specialized consistently under that seed.
            const auto& weights = tmp->GetWeights();
            const auto& registers = tmp->GetRegisters();

            auto dist = std::discrete_distribution<std::size_t>(weights.begin(), weights.end());
            const auto value = dist(engine);

            // Set output.
            for (auto i = std::size_t{0}; i < registers.size(); ++i) {
                static_register[registers[i].id] =
                        ((value >> i) & 1) == 1 ? RegisterState::True : RegisterState::False;
            }
        }
    }
}

bool PruneCondition(Function& func, std::mt19937_64& engine) {
    // Returns true when any terminator is rewritten.
    // Caller may rerun pass if fixpoint behavior is required.
    // Depth order gives predecessor information before use in this iteration.
    auto static_register_per_bb = std::
            unordered_map<const BasicBlock*, std::unordered_map<std::uint64_t, RegisterState>>{};
    const auto depth_order_bb = func.GetDepthOrderBB();

    for (auto&& [bb, depth] : depth_order_bb) {
        auto& static_register = static_register_per_bb[bb] = {};

        // Initialize static_register from preds.
        if (depth > 0) {
            InitializeStaticRegisterFromPred(static_register_per_bb, bb);
        }

        // Calculate static_register.
        CalculateStaticRegisterInBB(static_register, bb, engine);

        // Delete items which value is Dynamic.
        EraseDynamicRegisterState(static_register);

        // Prune condition if branch condition is static.
        const auto* terminator = bb->GetTerminator();
        if (const auto* tmp = DynCastIfPresent<BranchInst>(terminator)) {
            // Branch rewrite is safe only if the condition is fully static.
            if (tmp->IsConditional() && static_register.contains(tmp->GetCondition().id)) {
                auto* next_bb = static_register[tmp->GetCondition().id] == RegisterState::True
                        ? tmp->GetTrueBB()
                        : tmp->GetFalseBB();

                SelectNextBranch(bb, next_bb);
                return true;
            }
        } else if (const auto* tmp = DynCastIfPresent<SwitchInst>(terminator)) {
            // Switch rewrite is safe only when every bit is statically known.
            const auto& value = tmp->GetValue();
            if (std::ranges::all_of(value, [&static_register](const auto r) {
                    return static_register.contains(r.id);
                })) {
                auto value_list = std::vector<bool>{};
                for (const auto r : value) {
                    value_list.emplace_back(static_register[r.id] == RegisterState::True);
                }
                const auto value = BoolArrayAsInt(value_list);
                // NOTE: Keep debug logging fmt-compatible across runner toolchains.
                // Some fmt versions cannot format std::vector<bool> directly.
                auto value_bits = std::string{};
                value_bits.reserve(value_list.size());
                for (const auto bit : value_list) {
                    value_bits.push_back(bit ? '1' : '0');
                }

                LOG_DEBUG("value_list(bits): {}", value_bits);
                LOG_DEBUG("value: {}", value);

                auto* next_bb = tmp->GetCaseBB().contains(value) ? tmp->GetCaseBB().at(value)
                                                                 : tmp->GetDefaultBB();

                SelectNextBranch(bb, next_bb);
                return true;
            }
        }
    }

    return false;
}

bool StaticConditionPruningPass::RunOnFunction(Function& func) {
    auto changed = false;

    // Initialize random engine.
    std::mt19937_64 engine(
            seed.has_value() ? seed.value() : SeedOfStaticConditionPruningPass.Get()
    );

    // Run pruning.
    while (PruneCondition(func, engine)) {
        func.RemoveUnreachableBBs();
        changed = true;
    }

    return changed;
}
}  // namespace qret::ir
