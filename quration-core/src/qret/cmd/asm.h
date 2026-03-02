/**
 * @file qret/cmd/asm.h
 * @brief Define 'asm' subcommand in qret.
 */

#ifndef QRET_CMD_ASM_H
#define QRET_CMD_ASM_H

#include <string>

#include "qret/cmd/common.h"

namespace qret::cmd {
class CommandAsm : public SubCommand {
public:
    CommandAsm() = default;

    ReturnStatus Main(int argc, const char** argv) override;

    std::string Name() const override {
        return "asm";
    }
};

ReturnStatus Asm(
        const std::string& input,
        const std::string& output,
        const std::string& source,
        const std::string& target,
        bool print_metadata
);
}  // namespace qret::cmd

#endif  // QRET_CMD_ASM_H
