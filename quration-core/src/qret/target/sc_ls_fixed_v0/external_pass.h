/**
 * @file qret/target/sc_ls_fixed_v0/external_pass.h
 * @brief Pass to run commands defined externally.
 */

#ifndef QRET_TARGET_SC_LS_FIXED_V0_EXTERNAL_PASS_H
#define QRET_TARGET_SC_LS_FIXED_V0_EXTERNAL_PASS_H

#include <stdexcept>
#include <string_view>

#include "qret/codegen/machine_function.h"
#include "qret/codegen/machine_function_pass.h"
#include "qret/qret_export.h"

namespace qret::sc_ls_fixed_v0 {
struct QRET_EXPORT ExternalPass : public MachineFunctionPass {
    static inline char ID = 0;
    ExternalPass(
            std::string_view name,
            std::string_view cmd,
            std::string_view input,
            std::string_view output,
            std::string_view runner
    )
        : MachineFunctionPass(&ID)
        , name_{name}
        , cmd_{cmd}
        , input_{input}
        , output_{output}
        , runner_{runner} {
        if (cmd.empty()) {
            throw std::runtime_error("'cmd' must not be empty");
        }
        if (input.empty()) {
            throw std::runtime_error("'input' must not be empty");
        }
        if (output.empty()) {
            throw std::runtime_error("'output' must not be empty");
        }
    }

    std::string_view GetPassName() const override {
        return name_;
    }
    std::string_view GetPassArgument() const override {
        return name_;
    }
    bool RunOnMachineFunction(MachineFunction& mf) override;

private:
    std::string name_;
    std::string cmd_;
    std::string input_;
    std::string output_;
    std::string runner_;
};
}  // namespace qret::sc_ls_fixed_v0

#endif  // QRET_TARGET_SC_LS_FIXED_V0_EXTERNAL_PASS_H
