/**
 * @file qret/target/sc_ls_fixed_v0/instruction.cpp
 * @brief Instruction.
 */

#include "qret/target/sc_ls_fixed_v0/instruction.h"

#include <fmt/format.h>

#include <array>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

#include "qret/base/tribool.h"
#include "qret/target/sc_ls_fixed_v0/symbol.h"
#include "qret/target/sc_ls_fixed_v0/topology.h"

namespace qret::sc_ls_fixed_v0 {
namespace {
using InstructionTypeName = std::pair<ScLsInstructionType, std::string_view>;
constexpr auto kInstructionTypeNames = std::array<InstructionTypeName, 25>{
        InstructionTypeName{ScLsInstructionType::ALLOCATE, "ALLOCATE"},
        InstructionTypeName{
                ScLsInstructionType::ALLOCATE_MAGIC_FACTORY,
                "ALLOCATE_MAGIC_FACTORY",
        },
        InstructionTypeName{
                ScLsInstructionType::ALLOCATE_ENTANGLEMENT_FACTORY,
                "ALLOCATE_ENTANGLEMENT_FACTORY",
        },
        InstructionTypeName{ScLsInstructionType::DEALLOCATE, "DEALLOCATE"},
        InstructionTypeName{ScLsInstructionType::INIT_ZX, "INIT_ZX"},
        InstructionTypeName{ScLsInstructionType::MEAS_ZX, "MEAS_ZX"},
        InstructionTypeName{ScLsInstructionType::MEAS_Y, "MEAS_Y"},
        InstructionTypeName{ScLsInstructionType::TWIST, "TWIST"},
        InstructionTypeName{ScLsInstructionType::HADAMARD, "HADAMARD"},
        InstructionTypeName{ScLsInstructionType::ROTATE, "ROTATE"},
        InstructionTypeName{ScLsInstructionType::LATTICE_SURGERY, "LATTICE_SURGERY"},
        InstructionTypeName{ScLsInstructionType::LATTICE_SURGERY_MAGIC, "LATTICE_SURGERY_MAGIC"},
        InstructionTypeName{
                ScLsInstructionType::LATTICE_SURGERY_MULTINODE,
                "LATTICE_SURGERY_MULTINODE",
        },
        InstructionTypeName{ScLsInstructionType::MOVE, "MOVE"},
        InstructionTypeName{ScLsInstructionType::MOVE_MAGIC, "MOVE_MAGIC"},
        InstructionTypeName{ScLsInstructionType::MOVE_ENTANGLEMENT, "MOVE_ENTANGLEMENT"},
        InstructionTypeName{ScLsInstructionType::CNOT, "CNOT"},
        InstructionTypeName{ScLsInstructionType::CNOT_TRANS, "CNOT_TRANS"},
        InstructionTypeName{ScLsInstructionType::SWAP_TRANS, "SWAP_TRANS"},
        InstructionTypeName{ScLsInstructionType::MOVE_TRANS, "MOVE_TRANS"},
        InstructionTypeName{ScLsInstructionType::XOR, "XOR"},
        InstructionTypeName{ScLsInstructionType::AND, "AND"},
        InstructionTypeName{ScLsInstructionType::OR, "OR"},
        InstructionTypeName{ScLsInstructionType::PROBABILITY_HINT, "PROBABILITY_HINT"},
        InstructionTypeName{ScLsInstructionType::AWAIT_CORRECTION, "AWAIT_CORRECTION"},
};

const std::unordered_map<std::string, ScLsInstructionType>& GetInstructionTypeMap() {
    static const auto table = [] {
        auto ret = std::unordered_map<std::string, ScLsInstructionType>{};
        ret.reserve(kInstructionTypeNames.size());
        for (const auto& [type, name] : kInstructionTypeNames) {
            ret.emplace(std::string(name), type);
        }
        return ret;
    }();
    return table;
}
}  // namespace

std::string ToString(ScLsInstructionType t) {
    for (const auto& [type, name] : kInstructionTypeNames) {
        if (type == t) {
            return std::string(name);
        }
    }
    throw std::runtime_error(
            fmt::format("unknown instruction type: {}", static_cast<std::int32_t>(t))
    );
}
std::ostream& operator<<(std::ostream& out, ScLsInstructionType t) {
    return out << ToString(t);
}
ScLsInstructionType ScLsInstructionTypeFromString(const std::string& s) {
    if (s == "ALLOCATE_2Q") {
        throw std::runtime_error("unsupported instruction: ALLOCATE_2Q");
    }

    const auto& table = GetInstructionTypeMap();
    const auto itr = table.find(s);
    if (itr != table.end()) {
        return itr->second;
    }
    throw std::runtime_error(fmt::format("unknown ScLsInstructionType: {}", s));
}
std::string ScLsInstructionBase::ToStringWithMD() const {
    auto ss = std::stringstream();
    ss << ToString();
    ss << "  #";
    PrintMetadata(ss);
    return ss.str();
}
std::unique_ptr<ScLsInstructionBase> ScLsInstructionBase::FromJson(const Json& j) {
    const auto type = ScLsInstructionTypeFromString(j.at("type").template get<std::string>());
    switch (type) {
        case ScLsInstructionType::ALLOCATE:
            return Allocate::FromJson(j);
        case ScLsInstructionType::ALLOCATE_MAGIC_FACTORY:
            return AllocateMagicFactory::FromJson(j);
        case ScLsInstructionType::ALLOCATE_ENTANGLEMENT_FACTORY:
            return AllocateEntanglementFactory::FromJson(j);
        case ScLsInstructionType::DEALLOCATE:
            return DeAllocate::FromJson(j);
        case ScLsInstructionType::INIT_ZX:
            return InitZX::FromJson(j);
        case ScLsInstructionType::MEAS_ZX:
            return MeasZX::FromJson(j);
        case ScLsInstructionType::MEAS_Y:
            return MeasY::FromJson(j);
        case ScLsInstructionType::TWIST:
            return Twist::FromJson(j);
        case ScLsInstructionType::HADAMARD:
            return Hadamard::FromJson(j);
        case ScLsInstructionType::ROTATE:
            return Rotate::FromJson(j);
        case ScLsInstructionType::LATTICE_SURGERY:
            return LatticeSurgery::FromJson(j);
        case ScLsInstructionType::LATTICE_SURGERY_MAGIC:
            return LatticeSurgeryMagic::FromJson(j);
        case ScLsInstructionType::LATTICE_SURGERY_MULTINODE:
            return LatticeSurgeryMultinode::FromJson(j);
        case ScLsInstructionType::MOVE:
            return Move::FromJson(j);
        case ScLsInstructionType::MOVE_MAGIC:
            return MoveMagic::FromJson(j);
        case ScLsInstructionType::MOVE_ENTANGLEMENT:
            return MoveEntanglement::FromJson(j);
        case ScLsInstructionType::CNOT:
            return Cnot::FromJson(j);
        case ScLsInstructionType::CNOT_TRANS:
            return CnotTrans::FromJson(j);
        case ScLsInstructionType::SWAP_TRANS:
            return SwapTrans::FromJson(j);
        case ScLsInstructionType::MOVE_TRANS:
            return MoveTrans::FromJson(j);
        case ScLsInstructionType::XOR:
        case ScLsInstructionType::AND:
        case ScLsInstructionType::OR:
            return ClassicalOperation::FromJson(j);
        case ScLsInstructionType::PROBABILITY_HINT:
            return ProbabilityHint::FromJson(j);
        case ScLsInstructionType::AWAIT_CORRECTION:
            return AwaitCorrection::FromJson(j);
        default:
            throw std::logic_error("unreachable");
    }
}

//--------------------------------------------------//
// Implement individual instructions.
//--------------------------------------------------//

std::string Allocate::ToString() const {
    auto ss = std::stringstream();
    ss << Type() << ' ' << Qubit() << ' ' << Dest() << ' ' << Dir();
    PrintConditionIfExists(ss);
    return ss.str();
}
Json Allocate::ToJson() const {
    auto j = DefaultJson();
    j["dest"] = Dest();
    j["dir"] = Dir();
    return j;
}
std::unique_ptr<Allocate> Allocate::FromJson(const Json& j) {
    const auto qubit = j.at("qtarget")[0].template get<QSymbol>();
    const auto dest = j.at("dest").template get<Coord3D>();
    const auto dir = j.at("dir").template get<std::uint32_t>();
    const auto condition = JsonToT<CSymbol>(j, "condition");
    auto ret = New(qubit, dest, dir, condition);
    ret->SetMetadata(j);
    return ret;
}
std::string AllocateMagicFactory::ToString() const {
    auto ss = std::stringstream();
    ss << Type() << " " << MagicFactory() << " " << Dest();
    PrintConditionIfExists(ss);
    return ss.str();
}
Json AllocateMagicFactory::ToJson() const {
    auto j = DefaultJson();
    j["dest"] = Dest();
    return j;
}
std::unique_ptr<AllocateMagicFactory> AllocateMagicFactory::FromJson(const Json& j) {
    const auto magic_factory = j.at("mtarget")[0].template get<MSymbol>();
    const auto dest = j.at("dest").template get<Coord3D>();
    const auto condition = JsonToT<CSymbol>(j, "condition");
    auto ret = New(magic_factory, dest, condition);
    ret->SetMetadata(j);
    return ret;
}
std::string AllocateEntanglementFactory::ToString() const {
    auto ss = std::stringstream();
    ss << Type() << " " << EntanglementFactory1() << " " << Dest1() << " " << EntanglementFactory2()
       << " " << Dest2();
    PrintConditionIfExists(ss);
    return ss.str();
}
Json AllocateEntanglementFactory::ToJson() const {
    auto j = DefaultJson();
    j["dest1"] = Dest1();
    j["dest2"] = Dest2();
    return j;
}
std::unique_ptr<AllocateEntanglementFactory> AllocateEntanglementFactory::FromJson(const Json& j) {
    const auto entanglement_factory1 = j.at("etarget")[0].template get<ESymbol>();
    const auto entanglement_factory2 = j.at("etarget")[1].template get<ESymbol>();
    const auto dest1 = j.at("dest1").template get<Coord3D>();
    const auto dest2 = j.at("dest2").template get<Coord3D>();
    const auto condition = JsonToT<CSymbol>(j, "condition");
    auto ret = New(entanglement_factory1, dest1, entanglement_factory2, dest2, condition);
    ret->SetMetadata(j);
    return ret;
}
std::string DeAllocate::ToString() const {
    auto ss = std::stringstream();
    ss << Type() << ' ' << Qubit();
    PrintConditionIfExists(ss);
    return ss.str();
}
Json DeAllocate::ToJson() const {
    auto j = DefaultJson();
    return j;
}
std::unique_ptr<DeAllocate> DeAllocate::FromJson(const Json& j) {
    const auto qubit = j.at("qtarget")[0].template get<QSymbol>();
    const auto condition = JsonToT<CSymbol>(j, "condition");
    auto ret = New(qubit, condition);
    ret->SetMetadata(j);
    return ret;
}
std::string InitZX::ToString() const {
    auto ss = std::stringstream();
    ss << Type() << ' ' << Qubit() << ' ' << ZX();
    PrintConditionIfExists(ss);
    return ss.str();
}
Json InitZX::ToJson() const {
    auto j = DefaultJson();
    j["zx"] = ZX();
    return j;
}
std::unique_ptr<InitZX> InitZX::FromJson(const Json& j) {
    const auto qubit = j.at("qtarget")[0].template get<QSymbol>();
    const auto zx = j.at("zx").template get<std::uint32_t>();
    const auto condition = JsonToT<CSymbol>(j, "condition");
    auto ret = New(qubit, zx, condition);
    ret->SetMetadata(j);
    return ret;
}
std::string MeasZX::ToString() const {
    auto ss = std::stringstream();
    ss << Type() << ' ' << Qubit() << ' ' << ZX() << ' ' << CDest();
    PrintConditionIfExists(ss);
    return ss.str();
}
Json MeasZX::ToJson() const {
    auto j = DefaultJson();
    j["zx"] = ZX();
    return j;
}
std::unique_ptr<MeasZX> MeasZX::FromJson(const Json& j) {
    const auto qubit = j.at("qtarget")[0].template get<QSymbol>();
    const auto zx = j.at("zx").template get<std::uint32_t>();
    const auto cdest = j.at("ccreate")[0].template get<CSymbol>();
    const auto condition = JsonToT<CSymbol>(j, "condition");
    auto ret = New(qubit, zx, cdest, condition);
    ret->SetMetadata(j);
    return ret;
}
std::array<Coord3D, 3> MeasY::Ancillae(const Coord3D& coord, std::uint32_t dir) {
    // dir=00  dir=01  dir=10  dir=11
    // =================================
    // AA      AA      QA      AQ
    // QA      AQ      AA      AA

    if (dir == 0) {
        return {coord + Coord2D::Right(),
                coord + Coord2D::Up(),
                coord + Coord2D::Right() + Coord2D::Up()};
    } else if (dir == 1) {
        return {coord + Coord2D::Left(),
                coord + Coord2D::Up(),
                coord + Coord2D::Left() + Coord2D::Up()};
    } else if (dir == 2) {
        return {coord + Coord2D::Right(),
                coord + Coord2D::Down(),
                coord + Coord2D::Right() + Coord2D::Down()};
    } else if (dir == 3) {
        return {coord + Coord2D::Left(),
                coord + Coord2D::Down(),
                coord + Coord2D::Left() + Coord2D::Down()};
    }
    throw std::runtime_error("'dir' must be smaller than 4");
}
std::string MeasY::ToString() const {
    auto ss = std::stringstream();
    ss << Type() << ' ' << Qubit() << ' ' << Dir() << ' ' << CDest();
    PrintConditionIfExists(ss);
    return ss.str();
}
Json MeasY::ToJson() const {
    auto j = DefaultJson();
    j["dir"] = Dir();
    return j;
}
std::unique_ptr<MeasY> MeasY::FromJson(const Json& j) {
    const auto qubit = j.at("qtarget")[0].template get<QSymbol>();
    const auto dir = j.at("dir").template get<std::uint32_t>();
    const auto cdest = j.at("ccreate")[0].template get<CSymbol>();
    const auto condition = JsonToT<CSymbol>(j, "condition");
    auto ret = New(qubit, dir, cdest, condition);
    ret->SetMetadata(j);
    return ret;
}
std::vector<std::uint32_t> Twist::PossibleDirections(std::uint32_t qubit_dir, Pauli pauli) {
    // qubit_dir
    // dir=0  dir=1
    // ============
    //   X      Z
    //  ZQZ    XQX
    //   X      Z

    // twist dir
    // dir=00  dir=01  dir=10  dir=11
    // =================================
    //         A               Q
    // QA      Q       AQ      A

    assert(qubit_dir < 2 && "direction of qubit must be smaller than 2.");
    if (pauli.IsX()) {
        if (qubit_dir == 0) {
            return {1, 3};
        } else {
            return {0, 2};
        }
    } else if (pauli.IsZ()) {
        if (qubit_dir == 0) {
            return {0, 2};
        } else {
            return {1, 3};
        }
    }
    return {0, 1, 2, 3};
}
Coord3D Twist::GetAncilla(const Coord3D& coord, std::uint32_t dir) {
    // dir=00  dir=01  dir=10  dir=11
    // =================================
    //         A               Q
    // QA      Q       AQ      A

    if (dir == 0) {
        return coord + Coord2D::Right();
    } else if (dir == 1) {
        return coord + Coord2D::Up();
    } else if (dir == 2) {
        return coord + Coord2D::Left();
    } else if (dir == 3) {
        return coord + Coord2D::Down();
    }
    throw std::runtime_error("'dir' must be smaller than 4");
}
std::string Twist::ToString() const {
    auto ss = std::stringstream();
    ss << Type() << ' ' << Qubit() << ' ' << Dir();
    PrintConditionIfExists(ss);
    return ss.str();
}
Json Twist::ToJson() const {
    auto j = DefaultJson();
    j["dir"] = Dir();
    if (GetTargetPauli()) {
        j["metadata"]["pauli"] = *GetTargetPauli();
    }
    return j;
}
std::unique_ptr<Twist> Twist::FromJson(const Json& j) {
    const auto qubit = j.at("qtarget")[0].template get<QSymbol>();
    const auto dir = j.at("dir").template get<std::uint32_t>();
    const auto condition = JsonToT<CSymbol>(j, "condition");
    auto ret = New(qubit, dir, condition);
    ret->SetMetadata(j);
    if (j.contains("metadata") && j.at("metadata").contains("pauli")) {
        ret->SetTargetPauli(j.at("metadata").at("pauli").template get<Pauli>());
    }
    return ret;
}
std::string Hadamard::ToString() const {
    auto ss = std::stringstream();
    ss << Type() << ' ' << Qubit();
    PrintConditionIfExists(ss);
    return ss.str();
}
Json Hadamard::ToJson() const {
    auto j = DefaultJson();
    return j;
}
std::unique_ptr<Hadamard> Hadamard::FromJson(const Json& j) {
    const auto qubit = j.at("qtarget")[0].template get<QSymbol>();
    const auto condition = JsonToT<CSymbol>(j, "condition");
    auto ret = New(qubit, condition);
    ret->SetMetadata(j);
    return ret;
}
Coord3D Rotate::Ancilla(const Coord3D& coord, std::uint32_t dir) {
    if (dir == 0) {
        return coord + Coord2D::Right();
    } else if (dir == 1) {
        return coord + Coord2D::Up();
    } else if (dir == 2) {
        return coord + Coord2D::Left();
    } else if (dir == 3) {
        return coord + Coord2D::Down();
    }
    throw std::runtime_error("'dir' must be smaller than 4");
}
std::string Rotate::ToString() const {
    auto ss = std::stringstream();
    ss << Type() << ' ' << Qubit() << ' ' << Dir();
    PrintConditionIfExists(ss);
    return ss.str();
}
Json Rotate::ToJson() const {
    auto j = DefaultJson();
    j["dir"] = Dir();
    return j;
}
std::unique_ptr<Rotate> Rotate::FromJson(const Json& j) {
    const auto qubit = j.at("qtarget")[0].template get<QSymbol>();
    const auto dir = j.at("dir").template get<std::uint32_t>();
    const auto condition = JsonToT<CSymbol>(j, "condition");
    auto ret = New(qubit, dir, condition);
    ret->SetMetadata(j);
    return ret;
}
std::string LatticeSurgery::ToString() const {
    auto ss = std::stringstream();
    ss << Type() << ' ';
    PrintList(ss, QubitList());
    ss << ' ';
    PrintList(ss, BasisList());
    ss << ' ';
    PrintList(ss, Path());
    ss << ' ' << CDest();
    PrintConditionIfExists(ss);
    return ss.str();
}
Json LatticeSurgery::ToJson() const {
    auto j = DefaultJson();
    j["basis_list"] = BasisList();
    return j;
}
std::unique_ptr<LatticeSurgery> LatticeSurgery::FromJson(const Json& j) {
    const auto qubit_list = JsonToT<QSymbol>(j, "qtarget");
    const auto basis_list = JsonToT<Pauli>(j, "basis_list");
    const auto path = JsonToT<Coord3D>(j, "ancilla");
    const auto cdest = j.at("ccreate")[0].template get<CSymbol>();
    const auto condition = JsonToT<CSymbol>(j, "condition");
    auto ret = New(qubit_list, basis_list, path, cdest, condition);
    ret->SetMetadata(j);
    return ret;
}
std::string LatticeSurgeryMagic::ToString() const {
    auto ss = std::stringstream();
    ss << Type() << ' ';
    PrintList(ss, QubitList());
    ss << ' ';
    PrintList(ss, BasisList());
    ss << ' ';
    PrintList(ss, Path());
    ss << ' ' << CDest() << ' ' << MagicFactory();
    PrintConditionIfExists(ss);
    return ss.str();
}
Json LatticeSurgeryMagic::ToJson() const {
    auto j = DefaultJson();
    j["basis_list"] = BasisList();
    return j;
}
std::unique_ptr<LatticeSurgeryMagic> LatticeSurgeryMagic::FromJson(const Json& j) {
    const auto qubit_list = JsonToT<QSymbol>(j, "qtarget");
    const auto basis_list = JsonToT<Pauli>(j, "basis_list");
    const auto path = JsonToT<Coord3D>(j, "ancilla");
    const auto cdest = j.at("ccreate")[0].template get<CSymbol>();
    const auto magic_factory = j.at("mtarget")[0].template get<MSymbol>();
    const auto condition = JsonToT<CSymbol>(j, "condition");
    auto ret = New(qubit_list, basis_list, path, cdest, magic_factory, condition);
    ret->SetMetadata(j);
    return ret;
}
std::string LatticeSurgeryMultinode::ToString() const {
    auto ss = std::stringstream();
    ss << Type() << ' ';
    PrintList(ss, QubitList());
    ss << ' ';
    PrintList(ss, BasisList());
    ss << ' ';
    PrintList(ss, Path());
    ss << ' ' << CDest();
    ss << ' ';
    PrintList(ss, MTarget());
    ss << ' ';
    PrintList(ss, ETarget());
    ss << ' ';
    PrintList(ss, EHTarget());
    PrintConditionIfExists(ss);
    return ss.str();
}
Json LatticeSurgeryMultinode::ToJson() const {
    auto j = DefaultJson();
    j["basis_list"] = BasisList();
    return j;
}
std::unique_ptr<LatticeSurgeryMultinode> LatticeSurgeryMultinode::FromJson(const Json& j) {
    const auto qubit_list = JsonToT<QSymbol>(j, "qtarget");
    const auto basis_list = JsonToT<Pauli>(j, "basis_list");
    const auto path = JsonToT<Coord3D>(j, "ancilla");
    const auto cdest = j.at("ccreate")[0].template get<CSymbol>();
    const auto m_factory_list = JsonToT<MSymbol>(j, "mtarget");
    const auto e_factory_list = JsonToT<ESymbol>(j, "etarget");
    const auto e_handle_list = JsonToT<EHandle>(j, "ehtarget");
    const auto condition = JsonToT<CSymbol>(j, "condition");
    auto ret =
            New(qubit_list,
                basis_list,
                path,
                cdest,
                m_factory_list,
                e_factory_list,
                e_handle_list,
                condition);
    ret->SetMetadata(j);
    return ret;
}
std::string Move::ToString() const {
    auto ss = std::stringstream();
    ss << Type() << ' ' << Qubit() << ' ' << QDest() << ' ';
    PrintList(ss, Path());
    PrintConditionIfExists(ss);
    return ss.str();
}
Json Move::ToJson() const {
    auto j = DefaultJson();
    return j;
}
std::unique_ptr<Move> Move::FromJson(const Json& j) {
    const auto qubit = j.at("qtarget")[0].template get<QSymbol>();
    const auto qdest = j.at("qtarget")[1].template get<QSymbol>();
    const auto path = JsonToT<Coord3D>(j, "ancilla");
    const auto condition = JsonToT<CSymbol>(j, "condition");
    auto ret = New(qubit, qdest, path, condition);
    ret->SetMetadata(j);
    return ret;
}
std::string MoveMagic::ToString() const {
    auto ss = std::stringstream();
    ss << Type() << ' ' << MagicFactory() << ' ' << QDest() << ' ';
    PrintList(ss, Path());
    PrintConditionIfExists(ss);
    return ss.str();
}
Json MoveMagic::ToJson() const {
    auto j = DefaultJson();
    return j;
}
std::unique_ptr<MoveMagic> MoveMagic::FromJson(const Json& j) {
    const auto magic_factory = j.at("mtarget")[0].template get<MSymbol>();
    const auto qdest = j.at("qtarget")[0].template get<QSymbol>();
    const auto path = JsonToT<Coord3D>(j, "ancilla");
    const auto condition = JsonToT<CSymbol>(j, "condition");
    auto ret = New(magic_factory, qdest, path, condition);
    ret->SetMetadata(j);
    return ret;
}
std::string MoveEntanglement::ToString() const {
    auto ss = std::stringstream();
    ss << Type() << ' ' << EFactory() << ' ' << GetEHandle() << ' ' << QDest() << ' ';
    PrintList(ss, Path());
    PrintConditionIfExists(ss);
    return ss.str();
}
Json MoveEntanglement::ToJson() const {
    auto j = DefaultJson();
    return j;
}
std::unique_ptr<MoveEntanglement> MoveEntanglement::FromJson(const Json& j) {
    const auto e_factory = j.at("etarget")[0].template get<ESymbol>();
    const auto e_handle = j.at("ehtarget")[0].template get<EHandle>();
    const auto qdest = j.at("qtarget")[0].template get<QSymbol>();
    const auto path = JsonToT<Coord3D>(j, "ancilla");
    const auto condition = JsonToT<CSymbol>(j, "condition");
    auto ret = New(e_factory, e_handle, qdest, path, condition);
    ret->SetMetadata(j);
    return ret;
}
std::string Cnot::ToString() const {
    auto ss = std::stringstream();
    ss << Type() << ' ' << Qubit1() << ' ' << Qubit2() << ' ';
    PrintList(ss, Path());
    PrintConditionIfExists(ss);
    return ss.str();
}
Json Cnot::ToJson() const {
    auto j = DefaultJson();
    return j;
}
std::unique_ptr<Cnot> Cnot::FromJson(const Json& j) {
    const auto qubit1 = j.at("qtarget")[0].template get<QSymbol>();
    const auto qubit2 = j.at("qtarget")[1].template get<QSymbol>();
    const auto path = JsonToT<Coord3D>(j, "ancilla");
    const auto condition = JsonToT<CSymbol>(j, "condition");
    auto ret = New(qubit1, qubit2, path, condition);
    ret->SetMetadata(j);
    return ret;
}
std::string CnotTrans::ToString() const {
    auto ss = std::stringstream();
    ss << Type() << ' ' << Qubit1() << ' ' << Qubit2();
    PrintConditionIfExists(ss);
    return ss.str();
}
Json CnotTrans::ToJson() const {
    auto j = DefaultJson();
    return j;
}
std::unique_ptr<CnotTrans> CnotTrans::FromJson(const Json& j) {
    const auto qubit1 = j.at("qtarget")[0].template get<QSymbol>();
    const auto qubit2 = j.at("qtarget")[1].template get<QSymbol>();
    const auto condition = JsonToT<CSymbol>(j, "condition");
    auto ret = New(qubit1, qubit2, condition);
    ret->SetMetadata(j);
    return ret;
}
std::string SwapTrans::ToString() const {
    auto ss = std::stringstream();
    ss << Type() << ' ' << Qubit1() << ' ' << Qubit2();
    PrintConditionIfExists(ss);
    return ss.str();
}
Json SwapTrans::ToJson() const {
    auto j = DefaultJson();
    return j;
}
std::unique_ptr<SwapTrans> SwapTrans::FromJson(const Json& j) {
    const auto qubit1 = j.at("qtarget")[0].template get<QSymbol>();
    const auto qubit2 = j.at("qtarget")[1].template get<QSymbol>();
    const auto condition = JsonToT<CSymbol>(j, "condition");
    auto ret = New(qubit1, qubit2, condition);
    ret->SetMetadata(j);
    return ret;
}
std::string MoveTrans::ToString() const {
    auto ss = std::stringstream();
    ss << Type() << ' ' << Qubit() << ' ' << QDest();
    PrintConditionIfExists(ss);
    return ss.str();
}
Json MoveTrans::ToJson() const {
    auto j = DefaultJson();
    return j;
}
std::unique_ptr<MoveTrans> MoveTrans::FromJson(const Json& j) {
    const auto qubit = j.at("qtarget")[0].template get<QSymbol>();
    const auto qdest = j.at("qtarget")[1].template get<QSymbol>();
    const auto condition = JsonToT<CSymbol>(j, "condition");
    auto ret = New(qubit, qdest, condition);
    ret->SetMetadata(j);
    return ret;
}
std::string ClassicalOperation::ToString() const {
    auto ss = std::stringstream();
    ss << Type() << ' ';
    PrintList(ss, CDepend());
    ss << ' ';
    ss << CDest();
    PrintConditionIfExists(ss);
    return ss.str();
}
Json ClassicalOperation::ToJson() const {
    auto j = DefaultJson();
    return j;
}
std::unique_ptr<ClassicalOperation> ClassicalOperation::FromJson(const Json& j) {
    const auto type = ScLsInstructionTypeFromString(j.at("type").template get<std::string>());
    const auto ccreate = JsonToT<CSymbol>(j, "ccreate");
    const auto cdepend = JsonToT<CSymbol>(j, "cdepend");
    const auto condition = JsonToT<CSymbol>(j, "condition");

    if (ccreate.size() != 1) {
        throw std::runtime_error(
                fmt::format("invalid classical operation json: ccreate.size()={}", ccreate.size())
        );
    }

    if (cdepend.empty()) {
        throw std::runtime_error(
                fmt::format("invalid classical operation json: cdepend.size()={}", cdepend.size())
        );
    }

    const auto& register_list = cdepend;
    const auto cdest = ccreate.front();

    auto ret = New(type, register_list, cdest, condition);
    ret->SetMetadata(j);
    return ret;
}
std::string ProbabilityHint::ToString() const {
    auto ss = std::stringstream();
    ss << Type() << ' ' << CDest() << ' ' << Prob();
    PrintConditionIfExists(ss);
    if (GetSampledValue() == Tribool::True) {
        ss << " (Sampled 1)";
    } else if (GetSampledValue() == Tribool::False) {
        ss << " (Sampled 0)";
    }
    return ss.str();
}
Json ProbabilityHint::ToJson() const {
    auto j = DefaultJson();
    j["prob"] = Prob();
    j["sampled_value"] = ::qret::ToString(sampled_value_);
    return j;
}
std::unique_ptr<ProbabilityHint> ProbabilityHint::FromJson(const Json& j) {
    const auto cdest = j.at("cdepend")[0].template get<CSymbol>();
    const auto prob = j.at("prob").template get<double>();
    const auto condition = JsonToT<CSymbol>(j, "condition");
    auto ret = New(cdest, prob, condition);
    ret->SetSampledValue(TriboolFromString(j.at("sampled_value").template get<std::string>()));
    ret->SetMetadata(j);
    return ret;
}
std::string AwaitCorrection::ToString() const {
    auto ss = std::stringstream();
    ss << Type() << ' ';
    PrintList(ss, QTarget());
    PrintConditionIfExists(ss);
    return ss.str();
}
Json AwaitCorrection::ToJson() const {
    auto j = DefaultJson();
    return j;
}
std::unique_ptr<AwaitCorrection> AwaitCorrection::FromJson(const Json& j) {
    const auto qs = JsonToT<QSymbol>(j, "qtarget");
    const auto condition = JsonToT<CSymbol>(j, "condition");
    auto ret = New(qs, condition);
    ret->SetMetadata(j);
    return ret;
}

#pragma region machine function util
void DumpMachineFunctionSortedByBeat(std::ostream& out, const MachineFunction& mf) {
    using T = std::pair<const ScLsInstructionBase*, std::size_t>;
    auto comp = [](const T& lhs, const T& rhs) {
        const auto [lhs_inst, lhs_index] = lhs;
        const auto [rhs_inst, rhs_index] = rhs;
        return lhs_inst->Metadata().beat < rhs_inst->Metadata().beat
                || (lhs_inst->Metadata().beat == rhs_inst->Metadata().beat
                    && lhs_index < rhs_index);
    };
    auto insts = std::set<T, decltype(comp)>{};
    auto index = std::size_t{0};
    for (const auto& mbb : mf) {
        for (const auto& minst : mbb) {
            const auto* inst = static_cast<ScLsInstructionBase*>(minst.get());
            insts.emplace(inst, index++);
        }
    }
    for (const auto& inst : insts) {
        out << fmt::format("[BEAT {:3}] ", std::get<0>(inst)->Metadata().beat)
            << std::get<0>(inst)->ToString() << std::endl;
    }
}
#pragma endregion
}  // namespace qret::sc_ls_fixed_v0
