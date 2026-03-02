/**
 * @file qret/analysis/printer.cpp
 * @brief Print a func in pretty way.
 */

#include "qret/analysis/printer.h"

#include <sstream>
#include <string>

#include "qret/ir/basic_block.h"

namespace qret {
std::string ToString(const ir::BasicBlock& bb, std::uint32_t inst_indent) {
    auto ss = std::stringstream();

    ss << '%' << bb.GetName() << '\n';

    for (const auto& inst : bb) {
        ss << std::string(inst_indent, ' ');
        inst.Print(ss);
        ss << '\n';
    }

    return ss.str();
}

std::string ToString(
        const ir::Function& func,
        std::uint32_t bb_indent,
        std::uint32_t inst_indent,
        bool show_bb_dep
) {
    auto ss = std::stringstream();

    for (const auto& bb : func) {
        ss << '%' << bb.GetName() << '\n';

        if (show_bb_dep) {
            for (auto itr = bb.pred_begin(); itr != bb.pred_end(); ++itr) {
                ss << std::string(bb_indent, ' ') << '%' << bb.GetName() << " <--- " << '%'
                   << (*itr)->GetName() << '\n';
            }
        }

        for (const auto& inst : bb) {
            ss << std::string(inst_indent, ' ');
            inst.Print(ss);
            ss << '\n';
        }
    }
    return ss.str();
}
}  // namespace qret
