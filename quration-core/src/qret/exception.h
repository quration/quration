/**
 * @file qret/exception.h
 * @brief Base exception class in QRET library.
 */

#ifndef QRET_EXCEPTION_H
#define QRET_EXCEPTION_H

#include <fmt/format.h>

#include <exception>
#include <source_location>
#include <string>

#include "qret/qret_export.h"

namespace qret {
class QRET_EXPORT QRETError : public std::exception {
public:
    explicit QRETError(
            std::uint32_t id,
            const std::string& msg,
            std::source_location loc = std::source_location::current()
    )
        : id_{id}
        , msg_(fmt::format("[id={}] {}", id, msg))
        , loc_(loc) {}

    [[nodiscard]] std::uint32_t id() const noexcept {
        return id_;
    }

    [[nodiscard]] const char* what() const noexcept override {
        return msg_.c_str();
    }

    [[nodiscard]] std::source_location where() const noexcept {
        return loc_;
    }

private:
    std::uint32_t id_;
    std::string msg_;
    std::source_location loc_;
};
}  // namespace qret

#endif  // QRET_EXCEPTION_H
