/**
 * @file qret/target/sc_ls_fixed_v0/asm_printer.cpp
 * @brief Print SC_LS_FIXED_V0 instructions as assembly.
 */

#include "qret/codegen/asm_printer.h"

#include <fmt/format.h>

#include <memory>
#include <utility>

#include "qret/base/option.h"
#include "qret/base/time.h"
#include "qret/codegen/asm_streamer.h"
#include "qret/codegen/machine_function.h"
#include "qret/target/sc_ls_fixed_v0/constants.h"
#include "qret/target/sc_ls_fixed_v0/instruction.h"
#include "qret/target/sc_ls_fixed_v0/sc_ls_fixed_v0_target_machine.h"
#include "qret/target/sc_ls_fixed_v0/target_info/sc_ls_fixed_v0_target_info.h"
#include "qret/target/target_registry.h"
#include "qret/version.h"

namespace qret::sc_ls_fixed_v0 {
namespace {
class ScLsAsmPrinter : public AsmPrinter {
public:
    explicit ScLsAsmPrinter(std::unique_ptr<AsmStreamer>&& streamer)
        : AsmPrinter(std::move(streamer)) {}

    void EmitStartOfAsmFile() override;
    void EmitInstruction(const MachineInstruction* inst) override;
};
static auto X = RegisterAsmPrinter<ScLsAsmPrinter>(GetSC_LS_FIXED_V0Target());
static Opt<bool> PrintInstMetadata(
        "sc_ls_fixed_v0-print-inst-metadata",
        false,
        "Print metadata of instructions (beat and z coordinate).",
        OptionHidden::Hidden
);

void ScLsAsmPrinter::EmitStartOfAsmFile() {
    const auto& target = static_cast<const ScLsFixedV0TargetMachine*>(mf_->GetTarget());

    streamer_->EmitComment("------------------------------------------------------------");
    streamer_->EmitComment(
            fmt::format("Compilation to {} by QRET.", ScLsFixedV0TargetMachine::Name)
    );
    streamer_->EmitComment("Notes:");
    streamer_->EmitComment(
            fmt::format(
                    "Assembly of {} has {} reserved csymbols.",
                    ScLsFixedV0TargetMachine::Name,
                    NumReservedCSymbols
            )
    );
    streamer_->EmitComment("- C0 is always 0.");
    streamer_->EmitComment("- C1 is always 1.");
    streamer_->EmitComment("- C2-C9 are reserved for future use.");
    streamer_->EmitComment("------------------------------------------------------------");

    streamer_->EmitComment(fmt::format("header_schema: {}", AsmHeaderSchemaVersion));
    streamer_->EmitComment("qret:");
    streamer_->EmitComment(
            fmt::format(
                    "  version: {}.{}.{}",
                    QRET_VERSION_MAJOR,
                    QRET_VERSION_MINOR,
                    QRET_VERSION_PATCH
            )
    );
    streamer_->EmitComment(fmt::format("created_at: {}", GetCurrentTime()));
    streamer_->EmitComment("csymbols:");
    streamer_->EmitComment("  constants: { C0: 0, C1: 1 }");
    streamer_->EmitComment("  reserved: [C2, C3, C4, C5, C6, C7, C8, C9]");

    // TODO: add compiler info (passes, seed, ...)
    // TODO: add assembly info (passes, seed, ...)
    // TODO: add program info (name, uuid, description, ...)
    // TODO: add source info (language, ...).

    const auto& machine_option = target->machine_option;
    streamer_->EmitComment("target:");
    streamer_->EmitComment(fmt::format("  type: {}", ToString(machine_option.type)));
    streamer_->EmitComment(
            fmt::format(
                    "  use_magic_state_cultivation: {}",
                    machine_option.use_magic_state_cultivation
            )
    );
    streamer_->EmitComment(
            fmt::format("  magic_factory_seed_offset: {}", machine_option.magic_factory_seed_offset)
    );
    streamer_->EmitComment(
            fmt::format(
                    "  magic_generation_period: {}  # cycles",
                    machine_option.magic_generation_period
            )
    );
    streamer_->EmitComment(
            fmt::format(
                    "  prob_magic_state_creation: {:.2f}",
                    machine_option.prob_magic_state_creation
            )
    );
    streamer_->EmitComment(
            fmt::format("  maximum_magic_state_stock: {}", machine_option.maximum_magic_state_stock)
    );
    streamer_->EmitComment(
            fmt::format(
                    "  entanglement_generation_period: {}  # cycles",
                    machine_option.entanglement_generation_period
            )
    );
    streamer_->EmitComment(
            fmt::format(
                    "  maximum_entangled_state_stock: {}",
                    machine_option.maximum_entangled_state_stock
            )
    );
    streamer_->EmitComment(
            fmt::format("  reaction_time: {}  # cycles", machine_option.reaction_time)
    );

    const auto& topology = target->topology;
    streamer_->EmitComment("topology:");
    streamer_->EmitComment("  legend:");
    streamer_->EmitComment(fmt::format("    free: {}", ScLsPlane::Free));
    streamer_->EmitComment(fmt::format("    banned: {}", ScLsPlane::Banned));
    streamer_->EmitComment(fmt::format("    qubit: {}", ScLsPlane::Qubit));
    streamer_->EmitComment(fmt::format("    magic_factory: {}", ScLsPlane::MagicFactory));
    streamer_->EmitComment(
            fmt::format("    entanglement_factory: {}", ScLsPlane::EntanglementFactory)
    );
    streamer_->EmitComment("  orientation:");
    streamer_->EmitComment("    origin: { x: 0, y: 0 }");
    streamer_->EmitComment("    x_increases: right");
    streamer_->EmitComment("    y_increases: up");
    streamer_->EmitComment("    row_order_in_layout: high_to_low");
    for (const auto& grid : *topology) {
        streamer_->EmitComment(fmt::format("  grid_{}_{}:", grid.GetMinZ(), grid.GetMaxZ()));
        streamer_->EmitComment(fmt::format("    max_x: {}", grid.GetMaxX()));
        streamer_->EmitComment(fmt::format("    max_y: {}", grid.GetMaxY()));
        streamer_->EmitComment(fmt::format("    min_z: {}", grid.GetMinZ()));
        streamer_->EmitComment(fmt::format("    max_z: {}", grid.GetMaxZ()));
        for (const auto& plane : grid) {
            streamer_->EmitComment(fmt::format("    plane_{}:", plane.GetZ()));
            streamer_->EmitComment(fmt::format("      z: {}", plane.GetZ()));
            streamer_->EmitComment(fmt::format("      free: {}", plane.NumFree()));
            streamer_->EmitComment(fmt::format("      banned: {}", plane.NumBanned()));
            streamer_->EmitComment(fmt::format("      qubit: {}", plane.NumQubits()));
            streamer_->EmitComment(
                    fmt::format("      magic_factory: {}", plane.NumMagicFactories())
            );
            streamer_->EmitComment(
                    fmt::format("      entanglement_factory: {}", plane.NumEntanglementFactories())
            );
            streamer_->EmitComment("      layout:");
            for (auto y = plane.GetMaxY() - 1; y >= 0; --y) {
                streamer_->EmitComment(fmt::format("        y{}: {}", y, plane.GetLine(y)));
            }
        }
    }

    streamer_->AddBlankLine();
    streamer_->AddBlankLine();
}
void ScLsAsmPrinter::EmitInstruction(const MachineInstruction* tmp) {
    const auto* inst = static_cast<const ScLsInstructionBase*>(tmp);
    if (PrintInstMetadata) {
        streamer_->EmitInstruction(inst->ToStringWithMD());
    } else {
        streamer_->EmitInstruction(inst->ToString());
    }
}
}  // namespace
}  // namespace qret::sc_ls_fixed_v0
