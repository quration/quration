/**
 * @file qret/base/system.h
 * @brief Wrapper of OS-dependent functionality.
 * @details This file provides a portable way of using OS-dependent functionality.
 */

#ifndef QRET_BASE_SYSTEM_H
#define QRET_BASE_SYSTEM_H

#include <cstdint>
#include <string>

namespace qret {
/**
 * @brief Run the command.
 *
 * @param cmd command
 * @param out captured stdout of the command
 * @param exit exit status of the command
 */
void RunCommand(const std::string& cmd, std::string& out, std::int32_t& exit);
}  // namespace qret

#endif  // QRET_BASE_SYSTEM_H
