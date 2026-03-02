/**
 * @file qret/codegen/machine_function_pass.h
 * @brief Pass for MachineFunctions.
 * @details This file defines the MachineFunctionPass class.
 */

#ifndef QRET_CODEGEN_MACHINE_FUNCTION_PASS_H
#define QRET_CODEGEN_MACHINE_FUNCTION_PASS_H

#include <chrono>  // NOLINT
#include <map>
#include <ratio>  // NOLINT
#include <vector>

#include "qret/codegen/machine_function.h"
#include "qret/pass.h"
#include "qret/qret_export.h"

namespace qret {
// Forward declarations
class TargetMachine;
/**
 * @brief This class provides convenient creation of passes that operate on the MachineFunction
 * instances.
 */
class QRET_EXPORT MachineFunctionPass : public Pass {
public:
    explicit MachineFunctionPass(PassIDType pass_id)
        : Pass(pass_id, PassKind::MachineFunction) {}
    ~MachineFunctionPass() override = default;

    [[nodiscard]] PassManagerType GetPassManagerType() const override {
        return PassManagerType::MFPassManager;
    }
    virtual bool RunOnMachineFunction(MachineFunction& mf) = 0;
};

struct QRET_EXPORT MFAnalysis {
    using Time = std::chrono::duration<double, std::milli>;

    std::vector<const MachineFunctionPass*> run_order;
    std::map<const MachineFunctionPass*, Time> elapsed_ms;
};

class QRET_EXPORT MFPassManager : public PassManager {
public:
    MFPassManager() = default;

    void Run(MachineFunction& mf);
    const MFAnalysis& GetAnalysis() const {
        return analysis_;
    }
    MFAnalysis& GetMutAnalysis() {
        return analysis_;
    }

private:
    MFAnalysis analysis_;
};
}  // namespace qret

#endif  // QRET_TARGET_MACHINE_FUNCTION_PASS_H
