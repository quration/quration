/**
 * @file qret/codegen/asm_printer.h
 * @brief Print assembly.
 * @details This file contains a class to be used as the base class for target specific
 * asm writers. This class primarily handles common functionality used by
 * all asm writers.
 */

#ifndef QRET_CODEGEN_ASM_PRINTER_H
#define QRET_CODEGEN_ASM_PRINTER_H

#include <memory>

#include "qret/codegen/asm_streamer.h"
#include "qret/codegen/machine_function.h"
#include "qret/codegen/machine_function_pass.h"
#include "qret/qret_export.h"

namespace qret {
/**
 * @brief This class is intended to be used as a driving class for all asm writers.
 */
class QRET_EXPORT AsmPrinter : public MachineFunctionPass {
public:
    static inline char ID = 0;
    explicit AsmPrinter(std::unique_ptr<AsmStreamer>&& streamer);

    [[nodiscard]] const AsmStreamer* GetStreamer() const {
        return streamer_.get();
    }

    /**
     * @brief Emit the specified function out to the streamer.
     */
    bool RunOnMachineFunction(MachineFunction& mf) override;

    /**
     * @brief This should be called when a new MachineFunction is being processed from
     * RunOnMachineFunction.
     */
    virtual void SetupMachineFunction(const MachineFunction& mf);
    void EmitFunctionBody();

    //--------------------------------------------------//
    // Overridable Hooks
    //--------------------------------------------------//

    virtual void EmitStartOfAsmFile() {}
    virtual void EmitEndOfAsmFile() {}
    virtual void EmitFunctionHeader() {}
    virtual void EmitFunctionBodyStart() {}
    virtual void EmitFunctionBodyEnd() {}
    virtual void EmitBasicBlockStart(const MachineBasicBlock&) {}
    virtual void EmitBasicBlockEnd(const MachineBasicBlock&) {}
    /**
     * @brief Emits a instruction to a streamer.
     * @warning Targets should implement this to emit instructions.
     */
    virtual void EmitInstruction(const MachineInstruction* inst);

protected:
    std::unique_ptr<AsmStreamer> streamer_;
    const MachineFunction* mf_ = nullptr;
};
}  // namespace qret

#endif  // QRET_CODEGEN_ASM_PRINTER_H
