/**
 * @file qret/cmd/parse.h
 * @brief Define 'parse' sumcommand in qret.
 */

#ifndef QRET_CMD_PARSE_H
#define QRET_CMD_PARSE_H

#include <string>

#include "qret/cmd/common.h"

namespace qret::cmd {
class CommandParse : public SubCommand {
public:
    CommandParse() = default;

    ReturnStatus Main(int argc, const char** argv) override;

    std::string Name() const override {
        return "parse";
    }
};
ReturnStatus ParseOpenQASM2(const std::string& input, const std::string& output);
ReturnStatus ParseOpenQASM3(const std::string& input, const std::string& output);
ReturnStatus Parse(const std::string& input, const std::string& output, const std::string& format);
}  // namespace qret::cmd

#endif  // QRET_CMD_PARSE_H
