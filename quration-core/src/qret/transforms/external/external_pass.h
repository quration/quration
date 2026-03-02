/**
 * @file qret/transforms/external/external_pass.h
 * @brief Pass to run commands defined externally.
 */

#ifndef QRET_TRANSFORMS_EXTERNAL_EXTERNAL_PASS_H
#define QRET_TRANSFORMS_EXTERNAL_EXTERNAL_PASS_H

#include <stdexcept>
#include <string>
#include <string_view>

#include "qret/ir/function.h"
#include "qret/ir/function_pass.h"
#include "qret/qret_export.h"

namespace qret::ir {
class QRET_EXPORT ExternalPass : public FunctionPass {
public:
    static inline char ID = 0;
    ExternalPass(
            std::string_view name,
            std::string_view cmd,
            std::string_view input,
            std::string_view output,
            std::string_view runner
    )
        : FunctionPass(&ID)
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
    bool RunOnFunction(Function& func) override;

private:
    std::string name_;
    std::string cmd_;
    std::string input_;
    std::string output_;
    std::string runner_;
};
}  // namespace qret::ir

#endif  // QRET_TRANSFORMS_EXTERNAL_EXTERNAL_PASS_H
