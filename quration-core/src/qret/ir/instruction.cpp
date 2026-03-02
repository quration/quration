/**
 * @file qret/ir/instruction.cpp
 * @brief Opcode and base instruction class.
 */

#include "qret/ir/instruction.h"

#include <cassert>
#include <memory>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "qret/ir/basic_block.h"
#include "qret/ir/function.h"
#include "qret/ir/metadata.h"

namespace qret::ir {
Opcode Opcode::FromString(std::string_view opcode) {
    if (opcode == "Measurement") {
        return Table::Measurement;
    } else if (opcode == "I") {
        return Table::I;
    } else if (opcode == "X") {
        return Table::X;
    } else if (opcode == "Y") {
        return Table::Y;
    } else if (opcode == "Z") {
        return Table::Z;
    } else if (opcode == "H") {
        return Table::H;
    } else if (opcode == "S") {
        return Table::S;
    } else if (opcode == "SDag") {
        return Table::SDag;
    } else if (opcode == "T") {
        return Table::T;
    } else if (opcode == "TDag") {
        return Table::TDag;
    } else if (opcode == "RX") {
        return Table::RX;
    } else if (opcode == "RY") {
        return Table::RY;
    } else if (opcode == "RZ") {
        return Table::RZ;
    } else if (opcode == "CX") {
        return Table::CX;
    } else if (opcode == "CY") {
        return Table::CY;
    } else if (opcode == "CZ") {
        return Table::CZ;
    } else if (opcode == "CCX") {
        return Table::CCX;
    } else if (opcode == "CCY") {
        return Table::CCY;
    } else if (opcode == "CCZ") {
        return Table::CCZ;
    } else if (opcode == "MCX") {
        return Table::MCX;
    } else if (opcode == "GlobalPhase") {
        return Table::GlobalPhase;
    } else if (opcode == "Call") {
        return Table::Call;
    } else if (opcode == "CallCF") {
        return Table::CallCF;
    } else if (opcode == "DiscreteDistribution") {
        return Table::DiscreteDistribution;
    } else if (opcode == "Branch") {
        return Table::Branch;
    } else if (opcode == "Switch") {
        return Table::Switch;
    } else if (opcode == "Return") {
        return Table::Return;
    } else if (opcode == "Clean") {
        return Table::Clean;
    } else if (opcode == "CleanProb") {
        return Table::CleanProb;
    } else if (opcode == "DirtyBegin") {
        return Table::DirtyBegin;
    } else if (opcode == "DirtyEnd") {
        return Table::DirtyEnd;
    }
    throw std::runtime_error(std::string("unknown opcode: ") + opcode.data());
}
void Opcode::Print(std::ostream& out) const {
    switch (x_) {
        case Table::Measurement:
            out << "Measurement";
            return;
        case Table::I:
            out << 'I';
            return;
        case Table::X:
            out << 'X';
            return;
        case Table::Y:
            out << 'Y';
            return;
        case Table::Z:
            out << 'Z';
            return;
        case Table::H:
            out << 'H';
            return;
        case Table::S:
            out << 'S';
            return;
        case Table::SDag:
            out << "SDag";
            return;
        case Table::T:
            out << 'T';
            return;
        case Table::TDag:
            out << "TDag";
            return;
        case Table::RX:
            out << "RX";
            return;
        case Table::RY:
            out << "RY";
            return;
        case Table::RZ:
            out << "RZ";
            return;
        case Table::CX:
            out << "CX";
            return;
        case Table::CY:
            out << "CY";
            return;
        case Table::CZ:
            out << "CZ";
            return;
        case Table::CCX:
            out << "CCX";
            return;
        case Table::CCY:
            out << "CCY";
            return;
        case Table::CCZ:
            out << "CCZ";
            return;
        case Table::MCX:
            out << "MCX";
            return;
        case Table::GlobalPhase:
            out << "GlobalPhase";
            return;
        case Table::Call:
            out << "Call";
            return;
        case Table::CallCF:
            out << "CallCF";
            return;
        case Table::DiscreteDistribution:
            out << "DiscreteDistribution";
            return;
        case Table::Branch:
            out << "Branch";
            return;
        case Table::Switch:
            out << "Switch";
            return;
        case Table::Return:
            out << "Return";
            return;
        case Table::Clean:
            out << "Clean";
            return;
        case Table::CleanProb:
            out << "CleanProb";
            return;
        case Table::DirtyBegin:
            out << "DirtyBegin";
            return;
        case Table::DirtyEnd:
            out << "DirtyEnd";
            return;
        default:
            assert(0 && "unreachable");
    }
}
std::string Opcode::ToString() const {
    auto ss = std::stringstream();
    Print(ss);
    return ss.str();
}
const Function* Instruction::GetFunction() const {
    return GetParent() == nullptr ? nullptr : GetParent()->GetParent();
}
const Module* Instruction::GetModule() const {
    return GetFunction() == nullptr ? nullptr : GetFunction()->GetParent();
}
Instruction* Instruction::GetNextInstruction() const {
    if (!HasParent()) {
        return nullptr;
    }

    const auto* bb = GetParent();
    auto map_itr = bb->ptr2itr_.find(this);
    assert(map_itr != bb->ptr2itr_.end() && "instruction must be registered in parent");

    auto itr = std::next(map_itr->second);
    if (itr == bb->inst_list_.end()) {
        return nullptr;
    }
    return itr->get();
}
const Metadata* Instruction::GetMetadata(const MDKind& kind) const {
    for (auto itr = md_begin(); itr != md_end(); ++itr) {
        if (itr->GetMDKind() == kind) {
            return &(*itr);
        }
    }
    return nullptr;
}
Instruction* Instruction::AddMDAnnotation(std::string_view annot) {
    return AddMetadata(MDAnnotation::Create(annot));
}
Instruction* Instruction::AddMDQubit(const Qubit& q) {
    return AddMetadata(MDQubit::Create(q));
}
Instruction* Instruction::AddMDRegister(const Register& r) {
    return AddMetadata(MDRegister::Create(r));
}
Instruction* Instruction::MarkAsBreakPoint(std::string_view name) {
    return AddMetadata(DIBreakPoint::Create(name));
}
Instruction* Instruction::AddLocation(std::string_view file, std::uint32_t line) {
    return AddMetadata(DILocation::Create(file, line));
}
void Instruction::PrintMetadata(std::ostream& out) const {
    const char* sep = "";
    for (auto itr = md_begin(); itr != md_end(); ++itr) {
        out << std::exchange(sep, ", ");
        itr->PrintAsOperand(out);
    }
}
std::unique_ptr<Instruction> RemoveFromParent(Instruction* inst) {
    if (inst == nullptr || !inst->HasParent()) {
        return nullptr;
    }

    auto* bb = inst->GetParent();
    const auto map_itr = bb->ptr2itr_.find(inst);
    assert(map_itr != bb->ptr2itr_.end() && "instruction must be registered in parent");

    const auto itr = map_itr->second;
    auto ret = std::move(*itr);
    ret->SetParent(nullptr);
    bb->inst_list_.erase(itr);
    bb->ptr2itr_.erase(map_itr);
    return ret;
}
void EraseFromParent(Instruction* inst) {
    if (inst == nullptr || !inst->HasParent()) {
        return;
    }

    auto* bb = inst->GetParent();
    const auto map_itr = bb->ptr2itr_.find(inst);
    assert(map_itr != bb->ptr2itr_.end() && "instruction must be registered in parent");

    const auto itr = map_itr->second;
    bb->inst_list_.erase(itr);
    bb->ptr2itr_.erase(map_itr);
}
void InsertBefore(std::unique_ptr<Instruction>&& inst, Instruction* insert_pos) {
    if (inst == nullptr || insert_pos == nullptr || !insert_pos->HasParent()) {
        return;
    }

    auto* bb = insert_pos->GetParent();
    const auto map_itr = bb->ptr2itr_.find(insert_pos);
    assert(map_itr != bb->ptr2itr_.end() && "instruction must be registered in parent");

    auto* inst_ptr = inst.get();
    inst->SetParent(bb);
    const auto itr = bb->inst_list_.insert(map_itr->second, std::move(inst));
    bb->ptr2itr_[inst_ptr] = itr;
}
void InsertAfter(std::unique_ptr<Instruction>&& inst, Instruction* insert_pos) {
    if (inst == nullptr || insert_pos == nullptr || !insert_pos->HasParent()) {
        return;
    }

    auto* bb = insert_pos->GetParent();
    const auto map_itr = bb->ptr2itr_.find(insert_pos);
    assert(map_itr != bb->ptr2itr_.end() && "instruction must be registered in parent");

    auto* inst_ptr = inst.get();
    inst->SetParent(bb);
    const auto itr = bb->inst_list_.insert(map_itr->second, std::move(inst));
    bb->ptr2itr_[inst_ptr] = itr;
}
void InsertAtEnd(std::unique_ptr<Instruction>&& inst, BasicBlock* bb) {
    if (inst == nullptr || bb == nullptr) {
        return;
    }

    auto* inst_ptr = inst.get();
    inst->SetParent(bb);
    bb->inst_list_.emplace_back(std::move(inst));
    bb->ptr2itr_[inst_ptr] = std::prev(bb->inst_list_.end());
}
void MoveBefore(Instruction* inst, Instruction* move_pos) {
    InsertBefore(RemoveFromParent(inst), move_pos);
}
void MoveAfter(Instruction* inst, Instruction* move_pos) {
    InsertAfter(RemoveFromParent(inst), move_pos);
}
}  // namespace qret::ir
