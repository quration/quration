/**
 * @file qret/target/sc_ls_fixed_v0/runtime_simulation_pruning.cpp
 * @brief Routing.
 */

#include "qret/target/sc_ls_fixed_v0/runtime_simulation_pruning.h"

#include <fmt/format.h>

#include <cassert>
#include <iterator>
#include <optional>
#include <random>
#include <unordered_map>

#include "qret/base/cast.h"
#include "qret/base/log.h"
#include "qret/base/option.h"
#include "qret/base/tribool.h"
#include "qret/codegen/machine_function.h"
#include "qret/pass.h"
#include "qret/target/sc_ls_fixed_v0/constants.h"
#include "qret/target/sc_ls_fixed_v0/instruction.h"
#include "qret/target/sc_ls_fixed_v0/symbol.h"

namespace qret::sc_ls_fixed_v0 {
namespace {
static auto X = RegisterPass<RuntimeSimulationPruning>(
        "RuntimeSimulationPruning",
        "sc_ls_fixed_v0::runtime_simulation_pruning"
);

static Opt<std::uint64_t> PruningSeed(
        "sc_ls_fixed_v0_runtime_simulation_pruning_seed",
        0,
        "Seed",
        OptionHidden::Hidden
);
}  // namespace

class PruningContext {
public:
    explicit PruningContext(std::uint64_t seed)
        : engine_{seed} {
        value_[C0] = Tribool::False;
        value_[C1] = Tribool::True;
    }

    void SetValueByRandom(CSymbol c, double prob) {
        auto dist = std::bernoulli_distribution(prob);
        value_[c] = dist(engine_) ? Tribool::True : Tribool::False;
        LOG_DEBUG("Set value of csymbol {} to {}", c.ToString(), ToString(value_[c]));
    }

    void SetValue(CSymbol c, Tribool value) {
        value_[c] = value;
    }

    Tribool GetValue(CSymbol c) const {
        const auto itr = value_.find(c);
        if (itr == value_.end()) {
            return Tribool::Unknown;
        }
        return itr->second;
    }

private:
    std::mt19937_64 engine_;
    std::unordered_map<CSymbol, Tribool> value_;
};

Tribool CalculateTribool(
        const PruningContext& context,
        const ScLsInstructionType type,
        const std::list<CSymbol>& cs
) {
    if (cs.empty()) {
        return Tribool::Unknown;
    }

    // Use accumulate to fold the values.
    // 1. Start with the value of the first element (cs.front()).
    // 2. Iterate from the second element (std::next(cs.begin())) to the end.
    // 3. Apply the operation (inst.Op) to the accumulated value and the current symbol's value.
    return std::accumulate(
            std::next(cs.begin()),
            cs.end(),
            context.GetValue(cs.front()),
            [&](Tribool acc, const CSymbol& sym) {
                return ClassicalOperation::Op(type, acc, context.GetValue(sym));
            }
    );
}

void Prune(MachineBasicBlock& mbb, MachineInstruction* minst) {
    const auto& inst = *static_cast<ScLsInstructionBase*>(minst);

    // Create new await_correction inst.
    auto await_correction = AwaitCorrection::New(inst.QTarget(), {});

    // Insert
    mbb.InsertBefore(minst, std::move(await_correction));

    // Erase minst.
    mbb.Erase(minst);
}

void SimplifyCondition(const PruningContext& context, ScLsInstructionBase& inst) {
    if (inst.Condition().empty()) {
        return;
    }

    auto found_static = false;
    auto static_value = true;
    auto new_condition = std::list<CSymbol>{};
    for (const auto& c : inst.Condition()) {
        const auto value = context.GetValue(c);
        switch (value) {
            case Tribool::True:
                if (!found_static) {
                    found_static = true;
                    static_value = true;
                } else {
                    static_value = static_value ^ true;
                }
                break;
            case Tribool::False:
                if (!found_static) {
                    found_static = true;
                    static_value = false;
                } else {
                    static_value = static_value ^ false;
                }
                break;
            case Tribool::Unknown:
                new_condition.emplace_back(c);
                break;
            default:
                break;
        }
    }

    if (found_static) {
        new_condition.emplace_back(static_value ? C1 : C0);
        inst.SetCondition(std::move(new_condition));
    }
}

bool RuntimeSimulationPruning::RunOnMachineFunction(MachineFunction& mf) {
    auto changed = true;

    const auto actual_seed = seed ? *seed : PruningSeed;
    LOG_DEBUG("Seed of static condition pruning: {}", actual_seed);
    auto context = PruningContext(actual_seed);

    for (auto& mbb : mf) {
        mbb.ConstructInverseMap();

        for (auto itr = mbb.begin(); itr != mbb.end(); ++itr) {
            auto& inst = *static_cast<ScLsInstructionBase*>(itr->get());

            if (const auto* co = DynCast<ClassicalOperation>(&inst)) {
                // Classical operation.
                context.SetValue(
                        co->CDest(),
                        CalculateTribool(context, co->Type(), co->RegisterList())
                );
            } else if (auto* ph = DynCast<ProbabilityHint>(&inst)) {
                // Probability hint.
                context.SetValueByRandom(ph->CDest(), ph->Prob());
                ph->SetSampledValue(context.GetValue(ph->CDest()));
            }

            // Update condition.
            if (inst.Condition().empty()) {
                continue;
            }
            const auto t = CalculateTribool(context, ScLsInstructionType::XOR, inst.Condition());

            if (t == Tribool::True) {
                inst.SetCondition({});
            } else if (t == Tribool::False) {
                itr = std::next(itr);
                Prune(mbb, &inst);
                itr = std::prev(itr);
            } else if (t == Tribool::Unknown) {
                SimplifyCondition(context, inst);
            }
        }
    }

    return changed;
}
}  // namespace qret::sc_ls_fixed_v0
