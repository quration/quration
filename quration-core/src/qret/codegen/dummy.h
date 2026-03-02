/**
 * @file qret/codegen/dummy.h
 * @brief Dummy pass.
 */

#ifndef QRET_CODEGEN_DUMMY_H
#define QRET_CODEGEN_DUMMY_H

#include <string>
#include <string_view>

#include "qret/codegen/machine_function.h"
#include "qret/codegen/machine_function_pass.h"

namespace qret {
/**
 * @brief Dummy pass.
 * @details
 * DummyPass is a dummy that is used to import an externally implemented pass. It is not intended to
 * be executed.
 */
class DummyPass : public MachineFunctionPass {
public:
    static inline char ID = 0;
    explicit DummyPass(std::string_view name)
        : MachineFunctionPass(&ID)
        , name_{name} {}

    std::string_view GetPassName() const override {
        return name_;
    }
    std::string_view GetPassArgument() const override {
        return name_;
    }
    bool RunOnMachineFunction(MachineFunction& /* mf */) override {
        bool changed = false;
        return changed;
    }

private:
    std::string name_;
};
}  // namespace qret

#endif  // QRET_CODEGEN_DUMMY_H
