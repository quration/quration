/**
 * @file qret/cmd/profile.h
 * @brief Define 'profile' subcommand in qret.
 */

#ifndef QRET_CMD_PROFILE_H
#define QRET_CMD_PROFILE_H

#include <string>

#include "qret/cmd/common.h"

namespace qret::cmd {
class CommandProfile : public SubCommand {
public:
    CommandProfile() = default;

    ReturnStatus Main(int argc, const char** argv) override;

    std::string Name() const override {
        return "profile";
    }
};

ReturnStatus Profile(
        const std::string& input,
        const std::string& output,
        const std::string& source,
        const std::string& format
);
}  // namespace qret::cmd

#endif  // QRET_CMD_PROFILE_H
