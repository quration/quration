/**
 * @file qret/base/log.h
 * @brief Logger.
 * @details This file defines a simple logger class.
 */

#ifndef QRET_BASE_LOG_H
#define QRET_BASE_LOG_H

#include <fmt/format.h>

#include <array>
#include <chrono>  // NOLINT
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "qret/qret_export.h"

namespace qret {
/**
 * @brief Log level.
 */
enum class LogLevel : std::uint8_t {
    Fatal = 0,
    Error = 1,
    Warn = 2,
    Info = 3,
    Debug = 4,
};
/**
 * @brief Logger class intended for use as a singleton.
 */
struct QRET_EXPORT Logger {
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    static void Log(LogLevel level, std::string_view msg) {
        GetLogger()->LogConsoleImpl(level, msg);
        GetLogger()->LogFileImpl(level, msg);
    }

    // Set option
    static void SetLogLevel(LogLevel new_log_level) {
        GetLogger()->log_level_ = new_log_level;
    }
    static void DisableColorfulOutput() {
        GetLogger()->color_ = false;
    }
    static void EnableColorfulOutput() {
        GetLogger()->color_ = true;
    }
    static void DisableConsoleOutput() {
        GetLogger()->console_output_ = false;
    }
    static void EnableConsoleOutput() {
        GetLogger()->console_output_ = true;
    }
    static void DisableFileOutput() {
        GetLogger()->file_output_ = false;
    }
    static void EnableFileOutput(const std::string& path) {
        GetLogger()->file_output_ = true;
        GetLogger()->ofs_ = std::ofstream(path);
        if (!GetLogger()->ofs_.good()) {
            throw std::runtime_error("file open error: " + path);
        }
    }

    // Get option
    [[nodiscard]] static bool Verbose() {
        return OutputToConsole() || OutputToFile();
    }
    [[nodiscard]] static bool OutputToConsole() {
        return GetLogger()->console_output_;
    }
    [[nodiscard]] static bool OutputToFile() {
        return GetLogger()->file_output_;
    }
    [[nodiscard]] static LogLevel GetLogLevel() {
        return GetLogger()->log_level_;
    }

private:
    Logger() = default;
    ~Logger() = default;

    /**
     * @brief Get a pointer to the singleton Logger.
     */
    static Logger* GetLogger();

    static std::string GetLogPrefix(LogLevel level, bool color) {
        using namespace std::chrono;
        static constexpr auto Name =
                std::array<const char*, 5>{"FATAL", "ERROR", "WARN ", "INFO ", "DEBUG"};
        static constexpr auto ColorfulName = std::array<const char*, 5>{
                "\033[1;31mFATAL\033[0m",  // Light Red     1;31
                "\033[0;31mERROR\033[0m",  // Red           0;31
                "\033[1;33mWARN\033[0m ",  // Yellow        1;33
                "\033[0;32mINFO\033[0m ",  // Green         0;32
                "\033[0;37mDEBUG\033[0m"  // Light Gray    0;37
        };

        const auto t = system_clock::to_time_t(system_clock::now());
        const auto& lt = *std::localtime(&t);
        return fmt::format(
                "{:04}-{:02}-{:02} {:02}:{:02}:{:02} - {} - ",  // Format
                lt.tm_year + 1900,
                lt.tm_mon + 1,
                lt.tm_mday,
                lt.tm_hour,
                lt.tm_min,
                lt.tm_sec,  // Date
                color ? ColorfulName[static_cast<std::size_t>(level)]
                      : Name[static_cast<std::size_t>(level)]  // Log level
        );
    }
    void LogConsoleImpl(LogLevel level, std::string_view msg) {
        if (!console_output_) {
            return;
        }
        if (level > log_level_) {
            return;
        }
        std::cout << GetLogPrefix(level, color_) << msg << std::endl;
    }
    void LogFileImpl(LogLevel level, std::string_view msg) {
        if (!file_output_) {
            return;
        }
        if (!ofs_) {
            return;
        }
        if (level > log_level_) {
            return;
        }
        ofs_ << GetLogPrefix(level, false) << msg << std::endl;
    }

    LogLevel log_level_ =
#ifdef NDEBUG
            LogLevel::Info;
#else
            LogLevel::Debug;
#endif
    bool color_ = false;
    bool console_output_ =
#ifdef NDEBUG
            false;
#else
            false;  // true;
#endif
    bool file_output_ = false;
    std::ofstream ofs_;
};
}  // namespace qret

/**
 * @brief Log a message at the FATAL level.
 */
#define LOG_FATAL(MSG, ...)                                                                    \
    do {                                                                                       \
        qret::Logger::Log(qret::LogLevel::Fatal, fmt::format(MSG __VA_OPT__(, ) __VA_ARGS__)); \
    } while (0)
/**
 * @brief Log a message at the ERROR level.
 */
#define LOG_ERROR(MSG, ...)                                                                    \
    do {                                                                                       \
        qret::Logger::Log(qret::LogLevel::Error, fmt::format(MSG __VA_OPT__(, ) __VA_ARGS__)); \
    } while (0)
/**
 * @brief Log a message at the WARN level.
 */
#define LOG_WARN(MSG, ...)                                                                    \
    do {                                                                                      \
        qret::Logger::Log(qret::LogLevel::Warn, fmt::format(MSG __VA_OPT__(, ) __VA_ARGS__)); \
    } while (0)
/**
 * @brief Log a message at the INFO level.
 */
#define LOG_INFO(MSG, ...)                                                                    \
    do {                                                                                      \
        qret::Logger::Log(qret::LogLevel::Info, fmt::format(MSG __VA_OPT__(, ) __VA_ARGS__)); \
    } while (0)
/**
 * @brief Log a message at the DEBUG level.
 */
#define LOG_DEBUG(MSG, ...)                                                                    \
    do {                                                                                       \
        qret::Logger::Log(qret::LogLevel::Debug, fmt::format(MSG __VA_OPT__(, ) __VA_ARGS__)); \
    } while (0)

#endif  // QRET_BASE_LOG_H
