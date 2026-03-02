/**
 * @file qret/codegen/asm_printer.cpp
 * @brief Print assembly.
 */

#include "qret/codegen/asm_printer.h"

#include <memory>
#include <stdexcept>
#include <utility>

#include "qret/codegen/asm_streamer.h"
#include "qret/codegen/machine_function.h"
#include "qret/codegen/machine_function_pass.h"

namespace qret {
AsmPrinter::AsmPrinter(std::unique_ptr<AsmStreamer>&& streamer)
    : MachineFunctionPass(&ID)
    , streamer_{std::move(streamer)} {}
bool AsmPrinter::RunOnMachineFunction(MachineFunction& mf) {
    SetupMachineFunction(mf);
    EmitStartOfAsmFile();
    EmitFunctionBody();
    EmitEndOfAsmFile();
    return false;
}
void AsmPrinter::SetupMachineFunction(const MachineFunction& mf) {
    mf_ = &mf;
}
void AsmPrinter::EmitFunctionBody() {
    EmitFunctionHeader();
    EmitFunctionBodyStart();
    for (const auto& mbb : *mf_) {
        EmitBasicBlockStart(mbb);
        for (const auto& inst : mbb) {
            EmitInstruction(inst.get());
        }
        EmitBasicBlockEnd(mbb);
    }
    EmitFunctionBodyEnd();
}

void AsmPrinter::EmitInstruction(const MachineInstruction*) {
    throw std::runtime_error("EmitInstruction not implemented");
}
}  // namespace qret
