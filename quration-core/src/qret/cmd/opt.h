/**
 * @file qret/cmd/opt.h
 * @brief Define 'opt' sumcommand in qret.
 */

#ifndef QRET_CMD_OPT_H
#define QRET_CMD_OPT_H

#include <string>

#include "qret/cmd/common.h"

namespace qret::cmd {
class CommandOpt : public SubCommand {
public:
    CommandOpt() = default;

    ReturnStatus Main(int argc, const char** argv) override;

    std::string Name() const override {
        return "opt";
    }
};

ReturnStatus OptIR(
        const std::string& input,
        const std::string& function_name,
        const std::string& output,
        const std::vector<PassConfig>& pass_config
);
}  // namespace qret::cmd

#endif  // QRET_CMD_OPT_H
