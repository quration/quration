/**
 * @file qret/cmd/print.h
 * @brief Define 'print' sumcommand in qret.
 */

#ifndef QRET_CMD_PRINT_H
#define QRET_CMD_PRINT_H

#include <string>

#include "qret/cmd/common.h"

namespace qret::cmd {
class CommandPrint : public SubCommand {
public:
    CommandPrint() = default;

    ReturnStatus Main(int argc, const char** argv) override;

    std::string Name() const override {
        return "print";
    }
};

ReturnStatus Print(
        const std::string& input,
        const std::string& function_name,
        std::uint64_t depth,
        bool print_debug_info
);
}  // namespace qret::cmd

#endif  // QRET_CMD_PRINT_H
