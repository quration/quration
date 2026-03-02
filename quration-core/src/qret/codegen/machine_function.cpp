/**
 * @file qret/codegen/machine_function.cpp
 * @brief Machine function.
 */

#include "qret/codegen/machine_function.h"

#include <memory>
#include <utility>

namespace qret {
void MachineBasicBlock::ConstructInverseMap() {
    for (auto itr = instructions_.begin(); itr != instructions_.end(); ++itr) {
        mp_.emplace(itr->get(), itr);
    }
}
bool MachineBasicBlock::Contain(const MachineInstruction* inst) const {
    return mp_.contains(inst);
}
void MachineBasicBlock::InsertBefore(
        const MachineInstruction* inst,
        std::unique_ptr<MachineInstruction>&& new_inst
) {
    auto* ptr = new_inst.get();
    auto itr = mp_.at(inst);
    itr = instructions_.insert(itr, std::move(new_inst));
    mp_.emplace(ptr, itr);
}
void MachineBasicBlock::InsertAfter(
        const MachineInstruction* inst,
        std::unique_ptr<MachineInstruction>&& new_inst
) {
    auto* ptr = new_inst.get();
    auto itr = mp_.at(inst);
    ++itr;
    itr = instructions_.insert(itr, std::move(new_inst));
    mp_.emplace(ptr, itr);
}
void MachineBasicBlock::Erase(MachineInstruction* inst) {
    auto itr = mp_.at(inst);
    instructions_.erase(itr);
    mp_.erase(inst);
}
}  // namespace qret
