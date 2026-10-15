/**
 * @file qret/cmd/export.h
 * @brief Define 'export' subcommand in qret.
 */

#ifndef QRET_CMD_EXPORT_H
#define QRET_CMD_EXPORT_H

#include <string>

#include "qret/cmd/common.h"

namespace qret::cmd {
class CommandExport : public SubCommand {
public:
    CommandExport() = default;

    ReturnStatus Main(int argc, const char** argv) override;

    std::string Name() const override {
        return "export";
    }
};
ReturnStatus ExportQIR(
        const std::string& input,
        const std::string& output,
        const std::string& entry,
        bool inline_entry
);
ReturnStatus Export(
        const std::string& input,
        const std::string& output,
        const std::string& format,
        const std::string& entry,
        bool inline_entry
);
}  // namespace qret::cmd

#endif  // QRET_CMD_EXPORT_H
