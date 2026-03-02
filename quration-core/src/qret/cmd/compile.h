/**
 * @file qret/cmd/compile.h
 * @brief Define 'compile' sumcommand in qret.
 */

#ifndef QRET_CMD_COMPILE_H
#define QRET_CMD_COMPILE_H

#include <string>

#include "qret/cmd/common.h"

namespace qret::cmd {
class CommandCompile : public SubCommand {
public:
    CommandCompile() = default;

    ReturnStatus Main(int argc, const char** argv) override;

    std::string Name() const override {
        return "compile";
    }
};
}  // namespace qret::cmd

#endif  // QRET_CMD_COMPILE_H
