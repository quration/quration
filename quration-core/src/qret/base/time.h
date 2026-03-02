/**
 * @file qret/base/time.h
 * @brief Time related utility functions.
 */

#ifndef QRET_BASE_TIME_H
#define QRET_BASE_TIME_H

#include <fmt/format.h>

#include <chrono>  // NOLINT
#include <ctime>
#include <string>

#include "qret/qret_export.h"

namespace qret {
QRET_EXPORT inline std::string GetCurrentTime() {
    const auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    const auto& lt = *std::localtime(&t);
    return fmt::format(
            "{:04}-{:02}-{:02}T{:02}:{:02}:{:02}",
            lt.tm_year + 1900,
            lt.tm_mon + 1,
            lt.tm_mday,
            lt.tm_hour,
            lt.tm_min,
            lt.tm_sec
    );
}
}  // namespace qret

#endif  // QRET_BASE_TIME_H
