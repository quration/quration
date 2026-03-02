/**
 * @file qret/target/sc_ls_fixed_v0/validation.cpp
 * @brief Validate SC_LS_FIXED_V0 machine function.
 */

#include "qret/target/sc_ls_fixed_v0/validation.h"

#include <fmt/format.h>

#include <cassert>

#include "qret/base/cast.h"
#include "qret/codegen/machine_function.h"
#include "qret/error.h"
#include "qret/target/sc_ls_fixed_v0/constants.h"
#include "qret/target/sc_ls_fixed_v0/exception.h"
#include "qret/target/sc_ls_fixed_v0/instruction.h"
#include "qret/target/sc_ls_fixed_v0/sc_ls_fixed_v0_target_machine.h"
#include "qret/target/sc_ls_fixed_v0/symbol.h"

namespace qret::sc_ls_fixed_v0 {
struct Validator {
    explicit Validator(const ScLsFixedV0TargetMachine& machine)
        : machine(machine) {
        // Add reserved symbols.
        for (CSymbol::IdType i = 0; i < NumReservedCSymbols; ++i) {
            c2id[CSymbol{i}] = nullptr;
        }
    }

    void Validate(const MachineFunction& mf);
    void Validate(const ScLsInstructionBase& inst);

    const ScLsFixedV0TargetMachine& machine;
    std::map<QSymbol, const ScLsInstructionBase*> q2id;
    std::map<CSymbol, const ScLsInstructionBase*> c2id;
    std::map<MSymbol, const ScLsInstructionBase*> m2id;
    std::map<ESymbol, const ScLsInstructionBase*> e2id;

    template <typename Symbol>
    ScLsError UseUnallocatedSymbol(Symbol s, const ScLsInstructionBase& inst) const {
        return ScLsError(
                error::UseUnallocatedSymbol,
                fmt::format(
                        "Found unallocated symbol ({}). [inst={}]",
                        s.ToString(),
                        inst.ToString()
                )
        );
    }
    template <typename Symbol>
    ScLsError ReAllocateSymbol(Symbol s, const ScLsInstructionBase& inst) const {
        if (s.Type() == SymbolType::Classical && s.Id() < NumReservedCSymbols) {
            return ScLsError(
                    error::ReAllocateSymbol,
                    fmt::format(
                            "Reallocate symbol ({}). c0, c1, ..., c{} are reserved. [inst={}]",
                            s.ToString(),
                            NumReservedCSymbols - 1,
                            inst.ToString()
                    )
            );
        }

        return ScLsError(
                error::ReAllocateSymbol,
                fmt::format("Reallocate symbol ({}). [inst={}]", s.ToString(), inst.ToString())
        );
    }
};

void Validator::Validate(const MachineFunction& mf) {
    for (const auto& bb : mf) {
        for (const auto& inst : bb) {
            Validate(*static_cast<const ScLsInstructionBase*>(inst.get()));
        }
    }
}

void Validator::Validate(const ScLsInstructionBase& inst) {
    // Check condition.
    for (const auto& c : inst.Condition()) {
        if (!c2id.contains(c)) {
            throw UseUnallocatedSymbol(c, inst);
        }
    }

    // Check allocation/deallocation.
    if (const auto* i = DynCast<Allocate>(&inst)) {
        const auto q = i->Qubit();
        if (q2id.contains(q)) {
            throw ReAllocateSymbol(q, inst);
        }
        q2id[q] = &inst;
        return;
    } else if (const auto* i = DynCast<DeAllocate>(&inst)) {
        const auto q = i->Qubit();
        if (!q2id.contains(q)) {
            throw UseUnallocatedSymbol(q, inst);
        }
        q2id.erase(q);
        return;
    } else if (const auto* i = DynCast<AllocateMagicFactory>(&inst)) {
        const auto m = i->MagicFactory();
        if (m2id.contains(m)) {
            throw ReAllocateSymbol(m, inst);
        }
        m2id[m] = &inst;
        return;
    } else if (const auto* i = DynCast<AllocateEntanglementFactory>(&inst)) {
        const auto e1 = i->EntanglementFactory1();
        const auto e2 = i->EntanglementFactory2();
        for (const auto e : {e1, e2}) {
            if (e2id.contains(e)) {
                throw ReAllocateSymbol(e, inst);
            }
            e2id[e] = &inst;
        }
        return;
    }

    // Check CDepend.
    for (const auto& c : inst.CDepend()) {
        if (!c2id.contains(c)) {
            throw UseUnallocatedSymbol(c, inst);
        }
    }

    // Check CCreate.
    for (const auto c : inst.CCreate()) {
        // Measurement
        if (c2id.contains(c)) {
            throw ReAllocateSymbol(c, inst);
        }
        c2id[c] = &inst;
    }

    // Check qtarget.
    for (const auto q : inst.QTarget()) {
        if (!q2id.contains(q)) {
            throw UseUnallocatedSymbol(q, inst);
        }
    }

    // MOVE and MOVE_TRANS consume source qubit symbols.
    if (const auto* i = DynCast<Move>(&inst)) {
        const auto src = i->Qubit();
        const auto dst = i->QDest();
        if (src != dst) {
            q2id.erase(src);
        }
    } else if (const auto* i = DynCast<MoveTrans>(&inst)) {
        const auto src = i->Qubit();
        const auto dst = i->QDest();
        if (src != dst) {
            q2id.erase(src);
        }
    }

    // Check mtarget.
    for (const auto m : inst.MTarget()) {
        if (!m2id.contains(m)) {
            throw UseUnallocatedSymbol(m, inst);
        }
    }

    // Check etarget.
    for (const auto e : inst.ETarget()) {
        if (!e2id.contains(e)) {
            throw UseUnallocatedSymbol(e, inst);
        }
    }
}

void Validate(const MachineFunction& mf) {
    auto validator = Validator(*static_cast<const ScLsFixedV0TargetMachine*>(mf.GetTarget()));
    validator.Validate(mf);
}
}  // namespace qret::sc_ls_fixed_v0
