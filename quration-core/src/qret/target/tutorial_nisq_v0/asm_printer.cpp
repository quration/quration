/**
 * @file qret/target/tutorial_nisq_v0/asm_printer.cpp
 * @brief Asm printer for TUTORIAL_NISQ_V0.
 */

#include "qret/codegen/asm_printer.h"

#include <memory>
#include <utility>

#include "qret/codegen/asm_streamer.h"
#include "qret/codegen/machine_function.h"
#include "qret/target/target_registry.h"
#include "qret/target/tutorial_nisq_v0/instruction.h"
#include "qret/target/tutorial_nisq_v0/target_info/tutorial_nisq_v0_target_info.h"

namespace qret::tutorial_nisq_v0 {
namespace {
class TutorialNisqV0AsmPrinter final : public qret::AsmPrinter {
public:
    explicit TutorialNisqV0AsmPrinter(std::unique_ptr<qret::AsmStreamer>&& streamer)
        : AsmPrinter(std::move(streamer)) {
        // Tutorial assembly is intended to be visually simple:
        // one instruction per line, no trailing separator.
        streamer_->SetSeparatorString("");
    }

    void EmitStartOfAsmFile() override {
        streamer_->EmitComment("TUTORIAL_NISQ_V0 assembly");
        streamer_->EmitComment("Supported: X Y Z H S SDAG T TDAG RX RY RZ CX CY CZ MX MY MZ");
        streamer_->EmitComment("No control-flow instructions are allowed in this ISA.");
        streamer_->AddBlankLine();
    }

    void EmitInstruction(const qret::MachineInstruction* inst) override {
        const auto* tutorial_inst = static_cast<const TutorialNisqV0Instruction*>(inst);
        streamer_->EmitInstruction(tutorial_inst->ToString());
    }
};

const auto kTutorialNisqV0AsmPrinterRegistered =
        RegisterAsmPrinter<TutorialNisqV0AsmPrinter>(GetTutorialNisqV0Target());
}  // namespace
}  // namespace qret::tutorial_nisq_v0
