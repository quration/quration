/**
 * @file qret/codegen/asm_streamer.h
 * @brief Streaming assembly.
 * @details This file defines a streamer class for assembly output.
 */

#ifndef QRET_CODEGEN_ASM_STREAMER_H
#define QRET_CODEGEN_ASM_STREAMER_H

#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include "qret/qret_export.h"

namespace qret {
class QRET_EXPORT AsmStreamer final {
public:
    AsmStreamer() = default;

    [[nodiscard]] std::string_view GetSeparatorString() const {
        return separator_string_;
    }
    [[nodiscard]] std::string_view GetCommentString() const {
        return comment_string_;
    }
    [[nodiscard]] std::uint32_t GetIndentWidth() const {
        return indent_width_;
    }
    void SetSeparatorString(std::string separator_string) {
        separator_string_ = std::move(separator_string);
    }
    void SetCommentString(std::string comment_string) {
        comment_string_ = std::move(comment_string);
    }
    void SetIndentWidth(const std::uint32_t indent_width) {
        indent_width_ = indent_width;
    }

    void Clear() {
        ClearStream();
        SetRootLevel();
    }
    void ClearStream() {
        out_stream_.str("");
        out_stream_.clear();
    }

    [[nodiscard]] std::string ToString() const;

    void EmitInstruction(std::string_view inst, bool indent = true);
    void EmitRaw(std::string_view msg, bool indent = true);
    void EmitComment(std::string_view comment, bool eol = true);
    void AddBlankLine();

    [[nodiscard]] bool IsRootLevel() const {
        return indent_level_ == 0;
    }
    void SetRootLevel() {
        indent_level_ = 0;
    }
    void IncreaseIndentLevel() {
        indent_level_++;
    }
    void DecreaseIndentLevel() {
        indent_level_ = indent_level_ == 0 ? 0 : indent_level_ - 1;
    }

private:
    std::string separator_string_ = ";";
    std::string comment_string_ = "#";
    std::uint32_t indent_width_ = 2;
    std::stringstream out_stream_;
    std::uint32_t indent_level_ = 0;
};
}  // namespace qret

#endif  // QRET_CODEGEN_ASM_STREAMER_H
