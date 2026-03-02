/**
 * @file qret/analysis/counter.cpp
 * @brief Count the number of instructions in a func.
 */

#include "qret/analysis/counter.h"

#include "qret/base/cast.h"
#include "qret/ir/basic_block.h"
#include "qret/ir/function.h"
#include "qret/ir/instruction.h"
#include "qret/ir/instructions.h"

namespace qret {
void InstCount::Print(std::ostream& out, bool ignore_callee) const {
    out << "num_bbs: " << num_bbs << "\n";
    out << "num_instructions: " << num_instructions << "\n";
    out << "counter:\n";
    for (const auto& [key, count] : counter) {
        const auto& [code, callee] = key;
        const auto opcode = ir::Opcode(code);
        out << "  ";
        if (opcode.IsCall() && !ignore_callee) {
            ir::Opcode(code).Print(out);
            out << " (" << callee->GetName() << "): " << count << "\n";
        } else {
            ir::Opcode(code).Print(out);
            out << ": " << count << "\n";
        }
    }
}

namespace {
void CountBB(InstCount& count, const ir::BasicBlock& bb) {
    count.num_bbs += 1;
    for (const auto& inst : bb) {
        count.num_instructions++;

        const auto opcode = inst.GetOpcode();
        auto key = std::pair<ir::Opcode::Table, const ir::Function*>(opcode.GetCode(), nullptr);
        if (opcode.IsCall()) {
            const auto& call = *Cast<ir::CallInst>(&inst);
            key.second = call.GetCallee();
        }

        const auto itr = count.counter.find(key);
        if (itr == count.counter.end()) {
            count.counter.emplace(key, 1);
        } else {
            itr->second++;
        }
    }
}
}  // namespace

InstCount CountInst(const ir::BasicBlock& bb) {
    auto ret = InstCount();
    CountBB(ret, bb);
    return ret;
}

InstCount CountInst(const ir::Function& func) {
    auto ret = InstCount();
    for (const auto& bb : func) {
        CountBB(ret, bb);
    }
    return ret;
}
}  // namespace qret
