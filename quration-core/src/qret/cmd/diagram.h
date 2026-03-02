/**
 * @file qret/cmd/diagram.h
 * @brief Define 'diagram' sumcommand in qret.
 */

#ifndef QRET_CMD_DIAGRAM_H
#define QRET_CMD_DIAGRAM_H

#include <string>

#include "qret/cmd/common.h"

namespace qret::cmd {
class CommandDiagram : public SubCommand {
public:
    CommandDiagram() = default;

    ReturnStatus Main(int argc, const char** argv) override;

    std::string Name() const override {
        return "diagram";
    }
};
ReturnStatus Diagram(
        const std::string& input,
        const std::string& function_name,
        const std::string& output,
        const std::string& format,
        bool display_num_calls
);
}  // namespace qret::cmd

#endif  // QRET_CMD_DIAGRAM_H
