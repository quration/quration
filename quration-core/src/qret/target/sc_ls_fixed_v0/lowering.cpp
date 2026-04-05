/**
 * @file qret/target/sc_ls_fixed_v0/lowering.cpp
 * @brief Lowering.
 */

#include "qret/target/sc_ls_fixed_v0/lowering.h"

#include <cstdint>
#include <list>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "qret/base/cast.h"
#include "qret/base/log.h"
#include "qret/base/option.h"
#include "qret/codegen/machine_function.h"
#include "qret/ir/basic_block.h"
#include "qret/ir/function.h"
#include "qret/ir/instruction.h"
#include "qret/ir/instructions.h"
#include "qret/ir/value.h"
#include "qret/math/pauli.h"
#include "qret/target/sc_ls_fixed_v0/constants.h"
#include "qret/target/sc_ls_fixed_v0/instruction.h"
#include "qret/target/sc_ls_fixed_v0/pauli.h"
#include "qret/target/sc_ls_fixed_v0/sc_ls_fixed_v0_target_machine.h"
#include "qret/target/sc_ls_fixed_v0/symbol.h"
#include "qret/target/sc_ls_fixed_v0/topology.h"

namespace qret::sc_ls_fixed_v0 {
namespace {
static Opt<std::string> DumpPBCString(
        "sc_ls_fixed_v0_dump_pbc_string",
        "",
        "Dump pauli string if PBC mode is enabled",
        OptionHidden::Hidden
);
}

struct LowerContextOfBB {
    const ScLsFixedV0MachineType machine_type;
    const std::uint64_t csym_offset;
    const std::list<CSymbol> condition;

    std::uint64_t new_csym;

    [[nodiscard]] QSymbol GetQ(ir::Qubit q) const {
        return QSymbol{q.id};
    }
    [[nodiscard]] CSymbol GetC(ir::Register r) const {
        return CSymbol{r.id + csym_offset};
    }
    [[nodiscard]] CSymbol GetNewC() {
        return CSymbol{new_csym++};
    }
};

void ValidateNonGateInst(const bool enable_pbc_mode, const ir::Function& func) {
    auto measured_qubits = std::unordered_set<std::uint64_t>{};
    auto seen_measurement = false;

    for (const auto& bb : func) {
        for (const auto& inst : bb) {
            if (enable_pbc_mode) {
                if (const auto* i = DynCast<ir::MeasurementInst>(&inst); i != nullptr) {
                    seen_measurement = true;
                    measured_qubits.emplace(i->GetQubit().id);
                } else if (inst.IsGate() && seen_measurement) {
                    throw std::runtime_error(
                            "SC_LS_FIXED_V0 lowering error: all measurements must be placed at "
                            "the end of the circuit in PBC mode."
                    );
                }
            }

            if (inst.IsGate()) {
                continue;
            }

            // Check non gate instructions.
            if (DynCast<ir::CallInst>(&inst) != nullptr) {
                throw std::runtime_error(
                        "SC_LS_FIXED_V0 lowering error: Cannot lower to SC_LS_FIXED_V0 if circuit "
                        "has call "
                        "instruction. "
                        "Inline all instructions "
                        "before lowering."
                );
                throw std::runtime_error(
                        fmt::format(
                                "SC_LS_FIXED_V0 lowering error: call instructions are unsupported "
                                "(bb=%{}).Inline all call instructions before lowering to "
                                "SC_LS_FIXED_V0.",
                                bb.GetName()
                        )
                );
            } else if (DynCast<ir::GlobalPhaseInst>(&inst) != nullptr) {
                LOG_WARN("SC_LS_FIXED_V0 lowering: GlobalPhaseInst will be ignored.");
            } else if (const auto* i = DynCast<ir::CallCFInst>(&inst)) {
                LOG_WARN(
                        "SC_LS_FIXED_V0 lowering: CallCFInst (function={}) will be ignored. Output "
                        "values "
                        "are "
                        "not representable in SC_LS_FIXED_V0. If branch/switch depends on these "
                        "values, "
                        "compilation will fail.",
                        i->GetFunction().Name()
                );
            } else if (DynCast<ir::DiscreteDistInst>(&inst) != nullptr) {
                LOG_WARN(
                        "DiscreteDistInst will be ignored. Output values are not representable in "
                        "SC_LS_FIXED_V0. If branch/switch depends on these values, compilation "
                        "will fail."
                );
            }
        }

        // Check terminator.
        if (enable_pbc_mode) {
            if (const auto* i = DynCastIfPresent<ir::BranchInst>(bb.GetTerminator());
                i != nullptr && i->IsConditional()) {
                throw std::runtime_error(
                        fmt::format(
                                "SC_LS_FIXED_V0 lowering error: conditional branches are not "
                                "supported "
                                "for PBC (bb=%{}).",
                                bb.GetName()
                        )
                );
            } else if (DynCastIfPresent<ir::SwitchInst>(bb.GetTerminator()) != nullptr) {
                throw std::runtime_error(
                        fmt::format(
                                "SC_LS_FIXED_V0 lowering error: switch instructions are not "
                                "supported "
                                "for PBC (bb=%{}).",
                                bb.GetName()
                        )
                );
            }
        }
    }

    if (enable_pbc_mode) {
        auto missing = std::vector<std::uint64_t>{};
        for (auto q = std::uint64_t{0}; q < func.GetNumQubits(); ++q) {
            if (!measured_qubits.contains(q)) {
                missing.emplace_back(q);
            }
        }
        if (!missing.empty()) {
            auto ss = std::stringstream{};
            ss << "SC_LS_FIXED_V0 lowering error: all qubits must be measured in PBC mode. "
               << "Missing measurements for q";
            for (auto i = std::size_t{0}; i < missing.size(); ++i) {
                if (i >= 1) {
                    ss << ", q";
                }
                ss << missing[i];
            }
            ss << '.';
            throw std::runtime_error(ss.str());
        }
    }

    (void)func.ConditionOfBB();
}

void LowerMeasurement(
        const LowerContextOfBB& ctx,
        MachineBasicBlock& bb,
        const ir::MeasurementInst* inst
) {
    const auto q = ctx.GetQ(inst->GetQubit());
    const auto c = ctx.GetC(inst->GetRegister());
    bb.EmplaceBack(MeasZX::New(q, MeasZX::Z, c, ctx.condition));
}

void LowerUnary(LowerContextOfBB& ctx, MachineBasicBlock& bb, const ir::UnaryInst* inst) {
    const auto q = ctx.GetQ(inst->GetQubit());
    switch (inst->GetOpcode().GetCode()) {
        case ir::Opcode::Table::H: {
            bb.EmplaceBack(Hadamard::New(q, ctx.condition));
            break;
        }
        case ir::Opcode::Table::S:
        case ir::Opcode::Table::SDag: {
            // SDag = S + Z
            // FIXME: Do not ignore Z.
            auto tmp = Twist::New(q, 0, ctx.condition);
            tmp->SetTargetPauli(Pauli::Z());
            bb.EmplaceBack(std::move(tmp));
            break;
        }
        case ir::Opcode::Table::T:
        case ir::Opcode::Table::TDag: {
            const auto csym = ctx.GetNewC();
            // FIXME: Implement TDag correctly.
            // T ---> Lattice surgery of q (Z) and M + Twist q
            if (ctx.machine_type == ScLsFixedV0MachineType::DistributedDim2
                || ctx.machine_type == ScLsFixedV0MachineType::DistributedDim3) {
                // Use LatticeSurgeryMultinode instruction.
                bb.EmplaceBack(
                        LatticeSurgeryMultinode::New(
                                {q},
                                {Pauli::Z()},
                                {},
                                csym,
                                {MSymbol{0}},
                                {},
                                {},
                                ctx.condition
                        )
                );
                bb.EmplaceBack(ProbabilityHint::New(csym, 0.5, ctx.condition));

                if (ctx.condition.empty()) {
                    auto tmp = Twist::New({q}, 0, {csym});
                    tmp->SetTargetPauli(Pauli::Z());
                    bb.EmplaceBack(std::move(tmp));
                } else {
                    auto twist_condition = ctx.condition;
                    twist_condition.emplace_back(csym);
                    const auto tmp_csym = CSymbol{ctx.new_csym++};
                    bb.EmplaceBack(ClassicalOperation::And(twist_condition, tmp_csym, {}));

                    auto tmp = Twist::New({q}, 0, {tmp_csym});
                    tmp->SetTargetPauli(Pauli::Z());
                    bb.EmplaceBack(std::move(tmp));
                }
            } else {
                bb.EmplaceBack(
                        LatticeSurgeryMagic::New(
                                {q},
                                {Pauli::Z()},
                                {},
                                csym,
                                MSymbol{0},
                                ctx.condition
                        )
                );
                bb.EmplaceBack(ProbabilityHint::New(csym, 0.5, ctx.condition));

                if (ctx.condition.empty()) {
                    auto tmp = Twist::New({q}, 0, {csym});
                    tmp->SetTargetPauli(Pauli::Z());
                    bb.EmplaceBack(std::move(tmp));
                } else {
                    auto twist_condition = ctx.condition;
                    twist_condition.emplace_back(csym);
                    const auto tmp_csym = CSymbol{ctx.new_csym++};
                    bb.EmplaceBack(ClassicalOperation::And(twist_condition, tmp_csym, {}));

                    auto tmp = Twist::New({q}, 0, {tmp_csym});
                    tmp->SetTargetPauli(Pauli::Z());
                    bb.EmplaceBack(std::move(tmp));
                }
            }
            break;
        }
        case ir::Opcode::Table::I:
        case ir::Opcode::Table::X:
        case ir::Opcode::Table::Y:
        case ir::Opcode::Table::Z:
            // FIXME: Do not ignore pauli.
            break;
        case ir::Opcode::Table::RX:
        case ir::Opcode::Table::RY:
        case ir::Opcode::Table::RZ: {
            throw std::runtime_error(
                    "SC_LS_FIXED_V0 lowering error: Cannot lower RX, RY, RZ nUse DecomposeInst "
                    "pass before "
                    "lowering."
            );
        }
        default:
            assert(0 && "unreachable");
    }
}

void LowerBinary(LowerContextOfBB& ctx, MachineBasicBlock& bb, const ir::BinaryInst* inst) {
    const auto q0 = ctx.GetQ(inst->GetQubit0());  // target
    const auto q1 = ctx.GetQ(inst->GetQubit1());  // control
    switch (inst->GetOpcode().GetCode()) {
        case ir::Opcode::Table::CX: {
            bb.EmplaceBack(Cnot::New(q1, q0, {}, ctx.condition));
            break;
        }
        case ir::Opcode::Table::CY:
        case ir::Opcode::Table::CZ:
            throw std::runtime_error(
                    "SC_LS_FIXED_V0 lowering error: Cannot lower CY, CZ. Use DecomposeInst pass "
                    "before "
                    "lowering."
            );
            break;
        default:
            assert(0 && "unreachable");
    }
}

void LowerBB(LowerContextOfBB& ctx, MachineBasicBlock& insert_at_end, const ir::BasicBlock& bb) {
    for (const auto& inst : bb) {
        if (inst.IsMeasurement()) {
            LowerMeasurement(ctx, insert_at_end, Cast<ir::MeasurementInst>(&inst));
        } else if (inst.IsUnary()) {
            LowerUnary(ctx, insert_at_end, Cast<ir::UnaryInst>(&inst));
        } else if (inst.IsBinary()) {
            LowerBinary(ctx, insert_at_end, Cast<ir::BinaryInst>(&inst));
        } else if (inst.IsTernary()) {
            throw std::runtime_error(
                    "SC_LS_FIXED_V0 lowering error: Cannot lower CCX, CCY, CCZ.\nUse DecomposeInst "
                    "pass "
                    "before "
                    "lowering."
            );
        } else if (inst.IsMultiControl()) {
            throw std::runtime_error(
                    "SC_LS_FIXED_V0 lowering error: Cannot lower multi-controlled instructions."
            );
        }
    }
}

void LowerCircuit(
        MachineBasicBlock& bb,
        const ScLsFixedV0MachineType machine_type,
        const std::uint64_t csym_offset,
        const ir::Function& func
) {
    // Since there are no branching instructions in the SC_LS_FIXED_V0 instruction set,
    // branching must be expressed using conditions.
    // Calculates the condition for each basic block in advance.
    const auto cond = func.ConditionOfBB();

    // Do lowering.
    const auto depth_ordered_bbs = func.GetDepthOrderBB();
    auto new_csym = func.GetNumRegisters() + func.GetNumTmpRegisters() + csym_offset;

    for (const auto& [ir_bb, _depth] : depth_ordered_bbs) {
        const auto& formula = cond.at(ir_bb);
        auto or_list = std::list<CSymbol>{};
        for (const auto& term : formula) {
            auto and_list = std::list<CSymbol>{};
            for (const auto& lit : term) {
                const auto csym = CSymbol{lit.GetVar().x + csym_offset};
                if (lit.Sign()) {
                    // Conditioned by 'csym'.
                    and_list.emplace_back(csym);
                } else {
                    // Conditioned by 'not csym'.
                    // Flip csym to tmp by XOR.
                    const auto tmp = CSymbol{new_csym++};

                    // tmp = 1 ^ csym
                    bb.EmplaceBack(
                            ClassicalOperation::Xor({C1, csym}, tmp, {})
                    );  // Condition is none.
                    and_list.emplace_back(tmp);
                }
            }

            if (and_list.size() >= 2) {
                const auto tmp = CSymbol{new_csym++};
                bb.EmplaceBack(ClassicalOperation::And(and_list, tmp, {}));
                or_list.emplace_back(tmp);
            } else {
                or_list.emplace_back(and_list.back());
            }
        }

        auto condition = std::list<CSymbol>{};
        if (or_list.size() >= 2) {
            const auto tmp = CSymbol{new_csym++};
            bb.EmplaceBack(ClassicalOperation::Or(or_list, tmp, {}));
            condition.emplace_back(tmp);
        } else {
            condition = or_list;
        }

        auto ctx = LowerContextOfBB{
                .machine_type = machine_type,
                .csym_offset = csym_offset,
                .condition = condition,
                .new_csym = new_csym,
        };
        LowerBB(ctx, bb, *ir_bb);

        new_csym = ctx.new_csym;
    }
}

namespace pbc {
struct AppendToPauliCircuit {
    static constexpr auto PiOver8 = math::PauliRotation::Theta::pi_over_8;
    static constexpr auto PiOver4 = math::PauliRotation::Theta::pi_over_4;
    static constexpr auto PiOver2 = math::PauliRotation::Theta::pi_over_2;

    math::PauliCircuit& circuit;
    std::size_t num_qubits;

    void Measurement(const ir::MeasurementInst* inst);
    void Unary(const ir::UnaryInst* inst);
    void Binary(const ir::BinaryInst* inst);
    void BB(const ir::BasicBlock* bb);
    void Circuit(const ir::Function* ir_circuit);
};

void AppendToPauliCircuit::Measurement(const ir::MeasurementInst* inst) {
    circuit.EmplaceBack(
            math::PauliMeasurement(math::PauliString::Z(num_qubits, inst->GetQubit().id))
    );
}

void AppendToPauliCircuit::Unary(const ir::UnaryInst* inst) {
    const auto q = inst->GetQubit().id;
    switch (inst->GetOpcode().GetCode()) {
        case ir::Opcode::Table::H: {
            circuit.EmplaceBack(math::PauliRotation(PiOver4, math::PauliString::Z(num_qubits, q)));
            circuit.EmplaceBack(math::PauliRotation(PiOver4, math::PauliString::X(num_qubits, q)));
            circuit.EmplaceBack(math::PauliRotation(PiOver4, math::PauliString::Z(num_qubits, q)));
            break;
        }
        case ir::Opcode::Table::S: {
            circuit.EmplaceBack(math::PauliRotation(PiOver4, math::PauliString::Z(num_qubits, q)));
            break;
        }
        case ir::Opcode::Table::SDag: {
            circuit.EmplaceBack(
                    math::PauliRotation(PiOver4, math::PauliString::Z(num_qubits, q, 2))
            );
            break;
        }
        case ir::Opcode::Table::T: {
            circuit.EmplaceBack(math::PauliRotation(PiOver8, math::PauliString::Z(num_qubits, q)));
            break;
        }
        case ir::Opcode::Table::TDag: {
            circuit.EmplaceBack(
                    math::PauliRotation(PiOver8, math::PauliString::Z(num_qubits, q, 2))
            );
            break;
        }
        case ir::Opcode::Table::X: {
            circuit.EmplaceBack(math::PauliRotation(PiOver2, math::PauliString::X(num_qubits, q)));
            break;
        }
        case ir::Opcode::Table::Y: {
            circuit.EmplaceBack(math::PauliRotation(PiOver2, math::PauliString::Y(num_qubits, q)));
            break;
        }
        case ir::Opcode::Table::Z: {
            circuit.EmplaceBack(math::PauliRotation(PiOver2, math::PauliString::Z(num_qubits, q)));
            break;
        }
        case ir::Opcode::Table::I:
            // Ignore pauli in sc_ls_fixed_v0
            break;
        case ir::Opcode::Table::RX:
        case ir::Opcode::Table::RY:
        case ir::Opcode::Table::RZ: {
            throw std::runtime_error(
                    "SC_LS_FIXED_V0 lowering error: Cannot lower RX, RY, RZ.\nUse DecomposeInst "
                    "pass "
                    "before "
                    "lowering."
            );
        }
        default:
            assert(0 && "unreachable");
    }
}

void AppendToPauliCircuit::Binary(const ir::BinaryInst* inst) {
    const auto q0 = inst->GetQubit0();
    const auto q1 = inst->GetQubit1();
    switch (inst->GetOpcode().GetCode()) {
        case ir::Opcode::Table::CX: {
            auto x = math::PauliString::X(num_qubits, q0.id, 2);
            auto z = math::PauliString::Z(num_qubits, q1.id, 2);
            circuit.EmplaceBack(math::PauliRotation(PiOver4, x * z));
            circuit.EmplaceBack(math::PauliRotation(PiOver4, x));
            circuit.EmplaceBack(math::PauliRotation(PiOver4, z));
            break;
        }
        case ir::Opcode::Table::CY:
        case ir::Opcode::Table::CZ:
            throw std::runtime_error(
                    "SC_LS_FIXED_V0 lowering error: Cannot lower CY, CZ.\nUse DecomposeInst pass "
                    "before "
                    "lowering."
            );
            break;
        default:
            assert(0 && "unreachable");
    }
}

void AppendToPauliCircuit::BB(const ir::BasicBlock* bb) {
    for (const auto& inst : *bb) {
        if (inst.IsMeasurement()) {
            Measurement(Cast<ir::MeasurementInst>(&inst));
        } else if (inst.IsUnary()) {
            Unary(Cast<ir::UnaryInst>(&inst));
        } else if (inst.IsBinary()) {
            Binary(Cast<ir::BinaryInst>(&inst));
        } else if (inst.IsTernary()) {
            throw std::runtime_error(
                    "SC_LS_FIXED_V0 lowering error: Cannot lower CCX, CCY, CZ.\nUse DecomposeInst "
                    "pass "
                    "before "
                    "lowering."
            );
        } else if (inst.IsMultiControl()) {
            throw std::runtime_error(
                    "SC_LS_FIXED_V0 lowering error: Cannot lower multi-controlled instructions."
            );
        }
    }
}

void AppendToPauliCircuit::Circuit(const ir::Function* ir_circuit) {
    for (const auto& bb : *ir_circuit) {
        BB(&bb);
    }
}

std::tuple<std::list<QSymbol>, std::list<Pauli>, std::list<QSymbol>> GetSymbolListFromPauliString(
        const math::PauliString& pauli
) {
    auto qsym_list = std::list<QSymbol>{};
    auto basis_list = std::list<Pauli>{};
    auto y_list = std::list<QSymbol>{};

    for (auto n = std::size_t{0}; n < pauli.GetNumQubits(); ++n) {
        switch (pauli[n]) {
            case 'X': {
                qsym_list.emplace_back(n);
                basis_list.emplace_back(Pauli::X());
                break;
            }
            case 'Z': {
                qsym_list.emplace_back(n);
                basis_list.emplace_back(Pauli::Z());
                break;
            }
            case 'Y': {
                qsym_list.emplace_back(n);
                basis_list.emplace_back(Pauli::X());
                y_list.emplace_back(n);
                break;
            }
            default:
                // Do nothing
                break;
        }
    }
    return {qsym_list, basis_list, y_list};
}

void LowerRotationPi8(
        MachineBasicBlock& bb,
        const math::PauliRotation& rotation,
        MSymbol m,
        QSymbol q_magic_state,
        QSymbol q_correction,
        std::uint64_t cdest
) {
    // Use auto-corrected pi/8 rotation
    // TODO: sign

    auto [qsym_list, basis_list, y_list] = GetSymbolListFromPauliString(rotation.GetPauli());

    // Y to X
    for (const auto q : y_list) {
        auto inst = Twist::New(q, 0, {});
        inst->SetTargetPauli(Pauli::Z());
        bb.EmplaceBack(std::move(inst));
    }

    //--------------------------------------------------//
    // pi/8 rotation
    //--------------------------------------------------//

    // 1. MOVE_MAGIC
    bb.EmplaceBack(MoveMagic::New(m, q_magic_state, {}, {}));

    // 2. LATTICE_SURGERY
    qsym_list.emplace_back(q_magic_state);
    basis_list.emplace_back(Pauli::Z());
    bb.EmplaceBack(LatticeSurgery::New(qsym_list, basis_list, {}, CSymbol{cdest}, {}));

    // 3. LATTICE_SURGERY of q_magic_state (Z) and q_correction (Y)
    {
        auto inst = Twist::New(q_correction, 0, {});
        inst->SetTargetPauli(Pauli::Z());
        bb.EmplaceBack(std::move(inst));
    }
    bb.EmplaceBack(
            LatticeSurgery::New(
                    {q_magic_state, q_correction},
                    {Pauli::Z(), Pauli::X()},
                    {},
                    CSymbol{cdest + 1},
                    {}
            )
    );
    {
        auto inst = Twist::New(q_correction, 0, {});
        inst->SetTargetPauli(Pauli::Z());
        bb.EmplaceBack(std::move(inst));
    }

    // 4. MEAS_X q_magic_state
    bb.EmplaceBack(MeasZX::New(q_magic_state, 1, CSymbol{cdest + 2}, {}));

    // 5. Measure q_correction in X basis conditioned by CSymbol{cdest}
    //    Measure q_correction in Z basis conditioned by CSymbol{cdest + 3} = C1 ^ CSymbol{cdest}
    bb.EmplaceBack(ClassicalOperation::Xor({C1, CSymbol{cdest}}, CSymbol{cdest + 3}, {}));
    bb.EmplaceBack(MeasZX::New(q_correction, 1, CSymbol{cdest + 4}, {CSymbol{cdest}}));
    bb.EmplaceBack(MeasZX::New(q_correction, 0, CSymbol{cdest + 5}, {CSymbol{cdest + 3}}));

    // X to Y
    for (const auto q : y_list) {
        auto inst = Twist::New(q, 0, {});
        inst->SetTargetPauli(Pauli::Z());
        bb.EmplaceBack(std::move(inst));
    }
}

void LowerMeasurement(
        MachineBasicBlock& bb,
        const math::PauliMeasurement& measurement,
        std::size_t i
) {
    // TODO: sign

    const auto [qsym_list, basis_list, y_list] =
            GetSymbolListFromPauliString(measurement.GetPauli());

    // Y to X
    for (const auto q : y_list) {
        auto inst = Twist::New(q, 0, {});
        inst->SetTargetPauli(Pauli::Z());
        bb.EmplaceBack(std::move(inst));
    }

    // Measure X or Z
    if (basis_list.size() == 1) {
        if (basis_list.front() == Pauli::X()) {
            bb.EmplaceBack(MeasZX::New(qsym_list.front(), 1, CSymbol{i}, {}));
        } else if (basis_list.front() == Pauli::Z()) {
            bb.EmplaceBack(MeasZX::New(qsym_list.front(), 0, CSymbol{i}, {}));
        } else {
            throw std::logic_error("Unreachable");
        }
    } else {
        bb.EmplaceBack(LatticeSurgery::New(qsym_list, basis_list, {}, CSymbol{i}, {}));
    }

    // X to Y
    for (const auto q : y_list) {
        auto inst = Twist::New(q, 0, {});
        inst->SetTargetPauli(Pauli::Z());
        bb.EmplaceBack(std::move(inst));
    }
}
void LowerCircuit(
        const std::uint64_t num_magic_factories,
        const std::uint64_t num_qubits,
        const std::uint64_t csym_offset,
        MachineBasicBlock& bb,
        const ir::Function& ir_func
) {
    // TODO: relate measurement instruction and readout register value

    // Pauli circuit
    auto pauli_circuit = math::PauliCircuit();

    auto append = AppendToPauliCircuit(pauli_circuit, ir_func.GetNumQubits());
    append.Circuit(&ir_func);

    pauli_circuit.ToStandardFormat();

    if (!DumpPBCString.Get().empty()) {
        const auto& path = DumpPBCString.Get();
        auto fs = std::ofstream(path);
        if (fs.good()) {
            for (const auto& inst : pauli_circuit) {
                fs << inst.ToString() << '\n';
            }
        } else {
            LOG_ERROR("Failed to open: {}", path);
        }
    }

    const auto m2qa = [num_magic_factories, num_qubits]() {
        auto m2qa = std::unordered_map<MSymbol, std::pair<QSymbol, QSymbol>>{};
        for (std::uint64_t i = 0; i < num_magic_factories; ++i) {
            m2qa[MSymbol{i}] = {QSymbol{num_qubits + (2 * i)}, QSymbol{num_qubits + (2 * i) + 1}};
        }
        return m2qa;
    }();

    // To SC_LS_FIXED_V0 Inst.
    auto magic_factory_index = std::uint64_t{0};
    auto classical_register_index = csym_offset;
    for (const auto& x : pauli_circuit) {
        if (x.IsRotation()) {
            if (x.IsNonClifford()) {
                const auto m = MSymbol{magic_factory_index};
                const auto [q, a] = m2qa.at(m);
                LowerRotationPi8(bb, x.GetRotation(), m, q, a, classical_register_index);
                magic_factory_index = (magic_factory_index + 1) % num_magic_factories;
                classical_register_index += 6;
            } else {
                throw std::logic_error("Unreachable.");
            }
        } else if (x.IsMeasurement()) {
            LowerMeasurement(bb, x.GetMeasurement(), classical_register_index);
            classical_register_index++;
        }
    }
}
}  // namespace pbc

bool Lowering::RunOnMachineFunction(MachineFunction& mf) {
    if (!mf.HasIR()) {
        throw std::runtime_error(
                "SC_LS_FIXED_V0 lowering error: Machine functions must have a pointer of IR "
                "circuit to "
                "compile"
        );
    }

    const auto& func = *mf.GetIR();
    const auto& target = *static_cast<const ScLsFixedV0TargetMachine*>(mf.GetTarget());
    // const auto& option = target.machine_option;
    const auto& topology = target.topology;

    ValidateNonGateInst(target.machine_option.enable_pbc_mode, func);

    const auto num_qubits = func.GetNumQubits();
    std::uint64_t num_magic_factories = 0;
    // std::uint64_t num_entanglement_factories = 0;

    // Create blocks.
    auto& alloc_block = mf.AddBlock();
    auto& inst_block = mf.AddBlock();
    auto& dealloc_block = mf.AddBlock();

    // Set alloc_block.
    for (const auto& grid : *topology) {
        for (const auto& plane : grid) {
            const auto z = plane.GetZ();
            for (auto itr = plane.magic_factories_begin(); itr != plane.magic_factories_end();
                 ++itr) {
                const auto& [p, m] = *itr;
                alloc_block.EmplaceBack(AllocateMagicFactory::New(m, Coord3D(p.x, p.y, z), {}));
                num_magic_factories++;
            }
            for (auto itr = plane.entanglement_factories_begin();
                 itr != plane.entanglement_factories_end();
                 ++itr) {
                const auto& [p, e] = *itr;
                const auto e_pair = topology->GetPair(e);
                const auto& p_pair = topology->GetPlace(e_pair);

                // Do not add pair twice.
                if (e < e_pair) {
                    alloc_block.EmplaceBack(
                            AllocateEntanglementFactory::New(
                                    e,
                                    Coord3D(p.x, p.y, z),
                                    e_pair,
                                    p_pair,
                                    {}
                            )
                    );
                }
                // num_entanglement_factories++;
            }
        }
    }
    for (auto i = std::size_t{0}; i < num_qubits; ++i) {
        alloc_block.EmplaceBack(Allocate::New(QSymbol{i}, Coord3D(), 0, {}));
    }

    // Set dealloc_block.
    for (auto i = std::size_t{0}; i < num_qubits; ++i) {
        dealloc_block.EmplaceBack(DeAllocate::New(QSymbol{i}, {}));
    }

    // Set inst_block.
    if (target.machine_option.enable_pbc_mode) {
        // PBC uses auto-corrected pi/8 rotation lowering.
        // Each non-Clifford rotation consumes two logical qubits:
        //   q_magic_state: holds the magic state
        //   q_correction: used to measure correction bits (X/Z) and drive feed-forward corrections
        // In principle, these should be allocated/deallocated per pi/8 gate, but mapping/routing
        // currently cannot do that, so we pre-allocate 2 * num_magic_factories logical qubits
        // here and reuse them in a factory-cyclic mapping.
        for (std::uint64_t i = 0; i < 2 * num_magic_factories; ++i) {
            alloc_block.EmplaceBack(Allocate::New(QSymbol{num_qubits + i}, Coord3D(), 0, {}));
            dealloc_block.EmplaceBack(DeAllocate::New(QSymbol{num_qubits + i}, {}));
        }

        pbc::LowerCircuit(num_magic_factories, num_qubits, NumReservedCSymbols, inst_block, func);
    } else {
        LowerCircuit(inst_block, target.machine_option.type, NumReservedCSymbols, func);
    }

    auto changed = true;
    return changed;
}
}  // namespace qret::sc_ls_fixed_v0
