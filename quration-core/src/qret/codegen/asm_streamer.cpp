/**
 * @file qret/codegen/asm_streamer.cpp
 * @brief Streaming assembly.
 */

#include "qret/codegen/asm_streamer.h"

#include <cassert>
#include <cstddef>
#include <string>
#include <string_view>

namespace qret {
std::string AsmStreamer::ToString() const {
    return out_stream_.str();
}

void AsmStreamer::EmitInstruction(std::string_view inst, bool indent) {
    if (!IsRootLevel() && indent) {
        const auto indent_width = static_cast<std::size_t>(indent_width_);
        out_stream_ << std::string(static_cast<std::size_t>(indent_level_) * indent_width, ' ');
    }
    out_stream_ << inst << separator_string_ << '\n';
}
void AsmStreamer::EmitRaw(std::string_view msg, bool indent) {
    if (!IsRootLevel() && indent) {
        const auto indent_width = static_cast<std::size_t>(indent_width_);
        out_stream_ << std::string(static_cast<std::size_t>(indent_level_) * indent_width, ' ');
    }
    out_stream_ << msg;
}
void AsmStreamer::EmitComment(std::string_view comment, bool eol) {
    if (!eol) {
        assert(comment.back() == '\n' && "comment not newline terminated");
    }
    out_stream_ << comment_string_ << ' ' << comment;
    if (eol) {
        out_stream_ << '\n';
    }
}
void AsmStreamer::AddBlankLine() {
    out_stream_ << '\n';
}
}  // namespace qret
