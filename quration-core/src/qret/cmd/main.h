/**
 * @file qret/cmd/main.h
 * @brief Define qret.
 */

#ifndef QRET_CMD_MAIN_H
#define QRET_CMD_MAIN_H

#include <memory>
#include <vector>

#include "qret/cmd/common.h"
#include "qret/qret_export.h"

namespace qret::cmd {
/**
 * @brief Get the sub-commands objects.
 *
 * @return std::vector<std::unique_ptr<SubCommand>> sub-commands
 */
std::vector<std::unique_ptr<SubCommand>> GetSubCommands();

/**
 * @brief Show help of qret cli.
 *
 * @param sub_commands sub-commands
 * @return ReturnStatus return status
 */
ReturnStatus ShowHelp(const std::vector<std::unique_ptr<SubCommand>>& sub_commands);

/**
 * @brief Show version of qret.
 *
 * @return ReturnStatus return status
 */
ReturnStatus ShowVersion();

/**
 * @brief Implementation of `QretMain`.
 *
 * @param argc argument count
 * @param argv argument vector
 * @return ReturnStatus returns status
 */
ReturnStatus QretMainImpl(int argc, const char** argv);

/**
 * @brief Main function of qret cli.
 *
 * @param argc argument count
 * @param argv argument vector
 * @return int 0 if success, otherwise 1
 */
QRET_EXPORT int QretMain(int argc, const char** argv) noexcept;
}  // namespace qret::cmd

#endif  // QRET_CMD_MAIN_H
