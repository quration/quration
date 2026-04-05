/**
 * @file qret/ir/json.cpp
 * @brief Json serializer of IR module.
 */

#include "qret/ir/json.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "qret/base/cast.h"
#include "qret/base/json.h"
#include "qret/base/log.h"
#include "qret/base/portable_function.h"
#include "qret/base/time.h"
#include "qret/ir/basic_block.h"
#include "qret/ir/instruction.h"
#include "qret/ir/instructions.h"
#include "qret/ir/metadata.h"
#include "qret/ir/value.h"
#include "qret/version.h"

namespace qret::ir {
//--------------------------------------------------//
// Serialization
//--------------------------------------------------//

void to_json(Json& j, const Module& module) {
    // FIXME: currently ignore context_
    // Set metadata.
    j["metadata"]["format"] = "IR";
    j["metadata"]["schema_version"] = IRJsonSchemaVersion;
    j["metadata"]["qret_version"] =
            fmt::format("{}.{}.{}", QRET_VERSION_MAJOR, QRET_VERSION_MINOR, QRET_VERSION_PATCH),
    j["metadata"]["created_at"] = fmt::format("{}", GetCurrentTime());

    // Set module data.
    j["name"] = module.GetName();
    j["circuit_list"] = Json::array();
    for (const auto& func : module) {
        auto tmp = Json();
        to_json(tmp, func);
        j["circuit_list"].emplace_back(tmp);
    }
}
void to_json(Json& j, const Function& func) {
    // FIXME: currently ignore parent_
    j["name"] = func.GetName();
    j["entry_point"] = func.GetEntryBB()->GetName();
    j["bb_list"] = Json::array();
    for (const auto& bb : func) {
        auto tmp = Json();
        to_json(tmp, bb);
        j["bb_list"].emplace_back(tmp);
    }
    const auto& argument = func.GetArgument();
    j["argument"]["num_qubits"] = func.GetNumQubits();
    for (const auto& [name, size] : argument.qubits) {
        j["argument"]["qubits"][name] = size;
    }
    j["argument"]["num_registers"] = func.GetNumRegisters();
    for (const auto& [name, size] : argument.registers) {
        j["argument"]["registers"][name] = size;
    }
    j["num_tmp_registers"] = func.GetNumTmpRegisters();
}
void to_json(Json& j, const BasicBlock& bb) {
    // FIXME: currently ignore parent_
    j["name"] = bb.GetName();
    j["inst_list"] = Json::array();
    for (const auto& inst : bb) {
        auto tmp = Json();
        to_json(tmp, inst);
        j["inst_list"].emplace_back(tmp);
    }
    j["predecessors"] = Json::array();
    for (auto itr = bb.pred_begin(); itr != bb.pred_end(); ++itr) {
        j["predecessors"].emplace_back((*itr)->GetName());
    }
    j["successors"] = Json::array();
    for (auto itr = bb.succ_begin(); itr != bb.succ_end(); ++itr) {
        j["successors"].emplace_back((*itr)->GetName());
    }
}
void to_json(Json& j, const Instruction& inst) {
    // FIXME: currently ignore parent_
    if (const auto* i = DynCast<MeasurementInst>(&inst)) {
        to_json(j, *i);
    } else if (const auto* i = DynCast<UnaryInst>(&inst)) {
        to_json(j, *i);
    } else if (const auto* i = DynCast<BinaryInst>(&inst)) {
        to_json(j, *i);
    } else if (const auto* i = DynCast<TernaryInst>(&inst)) {
        to_json(j, *i);
    } else if (const auto* i = DynCast<MultiControlInst>(&inst)) {
        to_json(j, *i);
    } else if (const auto* i = DynCast<GlobalPhaseInst>(&inst)) {
        to_json(j, *i);
    } else if (const auto* i = DynCast<CallInst>(&inst)) {
        to_json(j, *i);
    } else if (const auto* i = DynCast<CallCFInst>(&inst)) {
        to_json(j, *i);
    } else if (const auto* i = DynCast<DiscreteDistInst>(&inst)) {
        to_json(j, *i);
    } else if (const auto* i = DynCast<BranchInst>(&inst)) {
        to_json(j, *i);
    } else if (const auto* i = DynCast<SwitchInst>(&inst)) {
        to_json(j, *i);
    } else if (const auto* i = DynCast<ReturnInst>(&inst)) {
        to_json(j, *i);
    } else if (const auto* i = DynCast<CleanInst>(&inst)) {
        to_json(j, *i);
    } else if (const auto* i = DynCast<CleanProbInst>(&inst)) {
        to_json(j, *i);
    } else if (const auto* i = DynCast<DirtyInst>(&inst)) {
        to_json(j, *i);
    } else {
        assert(0 && "unreachable");
    }
    // metadata
    if (inst.HasMetadata()) {
        j["metadata"] = Json::array();
        for (auto itr = inst.md_begin(); itr != inst.md_end(); ++itr) {
            auto tmp = Json();
            to_json(tmp, *itr);
            j["metadata"].emplace_back(tmp);
        }
    }
}
void to_json(Json& j, const MeasurementInst& inst) {
    to_json(j, inst.GetOpcode());
    j["q"] = inst.GetQubit().id;
    j["r"] = inst.GetRegister().id;
}
void to_json(Json& j, const UnaryInst& inst) {
    if (const auto* i = DynCast<ParametrizedRotationInst>(&inst)) {
        to_json(j, *i);
    } else {
        to_json(j, inst.GetOpcode());
        j["q"] = inst.GetQubit().id;
    }
}
void to_json(Json& j, const ParametrizedRotationInst& inst) {
    to_json(j, inst.GetOpcode());
    j["q"] = inst.GetQubit().id;
    j["theta"] = inst.GetTheta();
}
void to_json(Json& j, const BinaryInst& inst) {
    to_json(j, inst.GetOpcode());
    j["q0"] = inst.GetQubit0().id;
    j["q1"] = inst.GetQubit1().id;
}
void to_json(Json& j, const TernaryInst& inst) {
    to_json(j, inst.GetOpcode());
    j["q0"] = inst.GetQubit0().id;
    j["q1"] = inst.GetQubit1().id;
    j["q2"] = inst.GetQubit2().id;
}
void to_json(Json& j, const MultiControlInst& inst) {
    to_json(j, inst.GetOpcode());
    j["target"] = inst.GetTarget().id;
    j["control"] = Json::array();
    for (const auto& c : inst.GetControl()) {
        j["control"].emplace_back(c.id);
    }
}
void to_json(Json& j, const GlobalPhaseInst& inst) {
    to_json(j, inst.GetOpcode());
    j["theta"] = inst.GetTheta();
}
void to_json(Json& j, const CallInst& inst) {
    to_json(j, inst.GetOpcode());
    j["callee"] = inst.GetCallee()->GetName();
    j["operate"] = Json::array();
    for (const auto& q : inst.GetOperate()) {
        j["operate"].emplace_back(q.id);
    }
    j["input"] = Json::array();
    for (const auto& r : inst.GetInput()) {
        j["input"].emplace_back(r.id);
    }
    j["output"] = Json::array();
    for (const auto& r : inst.GetOutput()) {
        j["output"].emplace_back(r.id);
    }
}
void to_json(Json& j, const CallCFInst& inst) {
    to_json(j, inst.GetOpcode());
    j["function"] = inst.GetFunction();
    j["input"] = Json::array();
    for (const auto& r : inst.GetInput()) {
        j["input"].emplace_back(r.id);
    }
    j["output"] = Json::array();
    for (const auto& r : inst.GetOutput()) {
        j["output"].emplace_back(r.id);
    }
}
void to_json(Json& j, const DiscreteDistInst& inst) {
    to_json(j, inst.GetOpcode());
    j["weights"] = inst.GetWeights();
    j["registers"] = Json::array();
    for (const auto& r : inst.GetRegisters()) {
        j["registers"].emplace_back(r.id);
    }
}
void to_json(Json& j, const BranchInst& inst) {
    to_json(j, inst.GetOpcode());
    j["num_successors"] = inst.GetNumSuccessors();
    if (inst.IsUnconditional()) {
        j["if_true"] = inst.GetTrueBB()->GetName();
    } else {
        j["condition"] = inst.GetCondition().id;
        j["if_true"] = inst.GetTrueBB()->GetName();
        j["if_false"] = inst.GetFalseBB()->GetName();
    }
}
void to_json(Json& j, const SwitchInst& inst) {
    to_json(j, inst.GetOpcode());
    j["registers"] = Json::array();
    for (const auto& r : inst.GetValue()) {
        j["registers"].emplace_back(r.id);
    }
    j["default"] = inst.GetDefaultBB()->GetName();
    j["case"] = Json();
    for (const auto& [key, case_bb] : inst.GetCaseBB()) {
        j["case"][key] = case_bb->GetName();
    }
}
void to_json(Json& j, const ReturnInst& inst) {
    to_json(j, inst.GetOpcode());
}
void to_json(Json& j, const CleanInst& inst) {
    to_json(j, inst.GetOpcode());
    j["q"] = inst.GetQubit().id;
}
void to_json(Json& j, const CleanProbInst& inst) {
    to_json(j, inst.GetOpcode());
    j["q"] = inst.GetQubit().id;
    j["prob"] = inst.GetCleanProb();
}
void to_json(Json& j, const DirtyInst& inst) {
    to_json(j, inst.GetOpcode());
    j["q"] = inst.GetQubit().id;
}
void to_json(Json& j, const FloatWithPrecision& v) {
    j["value"] = v.value;
    j["precision"] = v.precision;
}
void to_json(Json& j, const Opcode& opcode) {
    j["opcode"] = opcode.ToString();
}
void to_json(Json& j, const Metadata& metadata) {
    if (metadata.GetMDKind() == MDKind::Annotation) {
        to_json(j, *static_cast<const MDAnnotation*>(&metadata));
    } else if (metadata.GetMDKind() == MDKind::Qubit) {
        to_json(j, *static_cast<const MDQubit*>(&metadata));
    } else if (metadata.GetMDKind() == MDKind::Register) {
        to_json(j, *static_cast<const MDRegister*>(&metadata));
    } else if (metadata.GetMDKind() == MDKind::BreakPoint) {
        to_json(j, *static_cast<const DIBreakPoint*>(&metadata));
    } else if (metadata.GetMDKind() == MDKind::Location) {
        to_json(j, *static_cast<const DILocation*>(&metadata));
    } else {
        throw std::logic_error("unreachable");
    }
}
void to_json(Json& j, const MDAnnotation& md_annot) {
    j["kind"] = MDKindToString(md_annot.GetMDKind());
    j["annot"] = md_annot.GetAnnotation();
}
void to_json(Json& j, const MDQubit& md_qubit) {
    j["kind"] = MDKindToString(md_qubit.GetMDKind());
    j["q"] = md_qubit.GetQubit().id;
}
void to_json(Json& j, const MDRegister& md_register) {
    j["kind"] = MDKindToString(md_register.GetMDKind());
    j["r"] = md_register.GetRegister().id;
}
void to_json(Json& j, const DIBreakPoint& di_breakpoint) {
    j["kind"] = MDKindToString(di_breakpoint.GetMDKind());
    j["name"] = di_breakpoint.GetName();
}
void to_json(Json& j, const DILocation& di_location) {
    j["kind"] = MDKindToString(di_location.GetMDKind());
    j["file"] = di_location.GetFile();
    j["line"] = di_location.GetLine();
}

//--------------------------------------------------//
// Deserialization
//--------------------------------------------------//

using BBMap = std::unordered_map<std::string, BasicBlock*>;

namespace {
using CFGEdge = std::pair<std::string, std::string>;
struct CFGEdgeHash {
    std::size_t operator()(const CFGEdge& edge) const {
        const auto h0 = std::hash<std::string>{}(edge.first);
        const auto h1 = std::hash<std::string>{}(edge.second);
        return h0 ^ (h1 << 1);
    }
};

std::string FormatNames(const std::vector<std::string>& names) {
    std::ostringstream oss;
    for (auto i = std::size_t{0}; i < names.size(); ++i) {
        if (i != 0) {
            oss << ", ";
        }
        oss << fmt::format("\"{}\"", names[i]);
    }
    return oss.str();
}
std::string FormatEdges(const std::vector<CFGEdge>& edges) {
    std::ostringstream oss;
    for (auto i = std::size_t{0}; i < edges.size(); ++i) {
        if (i != 0) {
            oss << ", ";
        }
        oss << fmt::format("{} -> {}", edges[i].first, edges[i].second);
    }
    return oss.str();
}

Function* FindFunctionOrThrow(
        const FunctionMap& fmap,
        std::string_view function_name,
        std::string_view context
) {
    const auto itr = fmap.find(std::string(function_name));
    if (itr == fmap.end()) {
        throw std::runtime_error(
                fmt::format("Unknown function '{}' while loading {}.", function_name, context)
        );
    }
    return itr->second;
}
BasicBlock* FindBBOrThrow(const BBMap& bmap, std::string_view bb_name, std::string_view context) {
    const auto itr = bmap.find(std::string(bb_name));
    if (itr == bmap.end()) {
        throw std::runtime_error(
                fmt::format("Unknown basic block '{}' while loading {}.", bb_name, context)
        );
    }
    return itr->second;
}

std::vector<std::string> GetNameArray(const Json& array_json) {
    auto names = std::vector<std::string>{};
    for (const auto& name_json : array_json) {
        names.emplace_back(name_json.get<std::string>());
    }
    return names;
}

const Json& GetSwitchRegistersJson(
        const Json& inst_json,
        std::string_view function_name,
        std::string_view bb_name
) {
    if (inst_json.contains("registers")) {
        return inst_json.at("registers");
    }
    throw std::runtime_error(
            fmt::format(
                    "Switch instruction must contain 'registers' field (func: {}, bb: {}).",
                    function_name,
                    bb_name
            )
    );
}

void ValidateRegisterRange(
        const Register& r,
        const Function& func,
        std::string_view function_name,
        std::string_view bb_name,
        std::string_view inst_name,
        std::string_view operand_name
) {
    const auto upper_bound = func.GetNumRegisters() + func.GetNumTmpRegisters();
    if (r.id >= upper_bound) {
        throw std::runtime_error(
                fmt::format(
                        "Register operand '{}' in {} is out of range (func: {}, bb: {}, id: {}, "
                        "valid: [0, {})).",
                        operand_name,
                        inst_name,
                        function_name,
                        bb_name,
                        r.id,
                        upper_bound
                )
        );
    }
}
void ValidateQubitRange(
        const Qubit& q,
        const Function& func,
        std::string_view function_name,
        std::string_view bb_name,
        std::string_view inst_name,
        std::string_view operand_name
) {
    if (q.id >= func.GetNumQubits()) {
        throw std::runtime_error(
                fmt::format(
                        "Qubit operand '{}' in {} is out of range (func: {}, bb: {}, id: {}, "
                        "valid: [0, {})).",
                        operand_name,
                        inst_name,
                        function_name,
                        bb_name,
                        q.id,
                        func.GetNumQubits()
                )
        );
    }
}

void ValidateReturnConstraint(const Function& func) {
    const auto function_name = std::string(func.GetName());
    auto num_returns = std::size_t{0};
    const BasicBlock* return_bb = nullptr;
    for (const auto& bb : func) {
        for (const auto& inst : bb) {
            if (inst.GetOpcode() == Opcode::Table::Return) {
                num_returns++;
                return_bb = &bb;
            }
        }
    }
    if (num_returns != 1) {
        throw std::runtime_error(
                fmt::format(
                        "Function '{}' must contain exactly one Return instruction, but found {}.",
                        function_name,
                        num_returns
                )
        );
    }
    assert(return_bb != nullptr);

    auto reachable = std::unordered_set<const BasicBlock*>{};
    auto queue = std::queue<const BasicBlock*>{};
    reachable.insert(return_bb);
    queue.push(return_bb);
    while (!queue.empty()) {
        const auto* bb = queue.front();
        queue.pop();
        for (auto itr = bb->pred_begin(); itr != bb->pred_end(); ++itr) {
            const auto* pred = *itr;
            if (!reachable.contains(pred)) {
                reachable.insert(pred);
                queue.push(pred);
            }
        }
    }

    auto invalid = std::vector<std::string>{};
    for (const auto& bb : func) {
        if (!reachable.contains(&bb)) {
            invalid.emplace_back(std::string(bb.GetName()));
        }
    }
    if (!invalid.empty()) {
        throw std::runtime_error(
                fmt::format(
                        "All BBs in function '{}' must eventually reach the Return BB '{}'. "
                        "Unreachable-to-return BBs: {}",
                        function_name,
                        return_bb->GetName(),
                        FormatNames(invalid)
                )
        );
    }
}
void ValidateCallArgumentSize(const Module& module) {
    for (const auto& caller : module) {
        for (const auto& bb : caller) {
            for (const auto& inst : bb) {
                const auto* call = DynCastIfPresent<CallInst>(&inst);
                if (call == nullptr) {
                    continue;
                }
                const auto* callee = call->GetCallee();
                if (callee == nullptr) {
                    throw std::runtime_error(
                            fmt::format(
                                    "Call instruction has null callee (caller: {}, bb: {}).",
                                    caller.GetName(),
                                    bb.GetName()
                            )
                    );
                }
                if (call->GetOperate().size() != callee->GetNumQubits()) {
                    throw std::runtime_error(
                            fmt::format(
                                    "Call argument size mismatch: callee '{}' expects {} qubits, "
                                    "but {} were provided (caller: {}, bb: {}).",
                                    callee->GetName(),
                                    callee->GetNumQubits(),
                                    call->GetOperate().size(),
                                    caller.GetName(),
                                    bb.GetName()
                            )
                    );
                }
                if (call->GetInput().size() + call->GetOutput().size()
                    != callee->GetNumRegisters()) {
                    throw std::runtime_error(
                            fmt::format(
                                    "Call argument size mismatch: callee '{}' expects {} registers "
                                    "(input+output), but {} were provided (caller: {}, bb: {}).",
                                    callee->GetName(),
                                    callee->GetNumRegisters(),
                                    call->GetInput().size() + call->GetOutput().size(),
                                    caller.GetName(),
                                    bb.GetName()
                            )
                    );
                }
            }
        }
    }
}

void ValidateFunctionJsonWarnings(const Json& function_json) {
    const auto function_name = function_json.at("name").get<std::string>();
    const auto& bb_list = function_json.at("bb_list");

    auto bb_names = std::unordered_set<std::string>{};
    for (const auto& bb_json : bb_list) {
        const auto bb_name = bb_json.at("name").get<std::string>();
        if (!bb_names.emplace(bb_name).second) {
            throw std::runtime_error(
                    fmt::format(
                            "Function '{}' contains duplicate BB name: '{}'.",
                            function_name,
                            bb_name
                    )
            );
        }
    }

    auto edges_from_pred = std::unordered_set<CFGEdge, CFGEdgeHash>{};
    auto edges_from_succ = std::unordered_set<CFGEdge, CFGEdgeHash>{};

    for (const auto& bb_json : bb_list) {
        const auto bb_name = bb_json.at("name").get<std::string>();
        const auto& inst_list = bb_json.at("inst_list");

        auto found_terminator = false;
        for (auto i = std::size_t{0}; i < inst_list.size(); ++i) {
            auto opcode = Opcode{Opcode::Table::UnaryOpsBegin};
            from_json(inst_list[i], opcode);
            if (opcode.IsTerminator()) {
                found_terminator = true;
                if (i + 1 < inst_list.size()) {
                    LOG_WARN(
                            "terminator appears before the end of BB (func: {}, bb: {}, index: "
                            "{}).",
                            function_name,
                            bb_name,
                            i
                    );
                }
            }
        }
        if (!found_terminator) {
            LOG_WARN("BB has no terminator (func: {}, bb: {}).", function_name, bb_name);
        } else if (!inst_list.empty()) {
            auto last_opcode = Opcode{Opcode::Table::UnaryOpsBegin};
            from_json(inst_list.back(), last_opcode);
            if (!last_opcode.IsTerminator()) {
                LOG_WARN(
                        "last instruction is not a terminator (func: {}, bb: {}).",
                        function_name,
                        bb_name
                );
            }
        }

        auto seen = std::unordered_set<std::string>{};
        for (const auto& pred_name : GetNameArray(bb_json.at("predecessors"))) {
            if (!seen.emplace(pred_name).second) {
                LOG_WARN(
                        "duplicated predecessor '{}' (func: {}, bb: {}).",
                        pred_name,
                        function_name,
                        bb_name
                );
                continue;
            }
            if (!bb_names.contains(pred_name)) {
                LOG_WARN(
                        "unknown predecessor '{}' (func: {}, bb: {}).",
                        pred_name,
                        function_name,
                        bb_name
                );
                continue;
            }
            edges_from_pred.emplace(pred_name, bb_name);
        }

        seen.clear();
        for (const auto& succ_name : GetNameArray(bb_json.at("successors"))) {
            if (!seen.emplace(succ_name).second) {
                LOG_WARN(
                        "duplicated successor '{}' (func: {}, bb: {}).",
                        succ_name,
                        function_name,
                        bb_name
                );
                continue;
            }
            if (!bb_names.contains(succ_name)) {
                LOG_WARN(
                        "unknown successor '{}' (func: {}, bb: {}).",
                        succ_name,
                        function_name,
                        bb_name
                );
                continue;
            }
            edges_from_succ.emplace(bb_name, succ_name);
        }

        auto inferred = std::unordered_set<std::string>{};
        if (!inst_list.empty()) {
            auto last_opcode = Opcode{Opcode::Table::UnaryOpsBegin};
            from_json(inst_list.back(), last_opcode);
            if (last_opcode.GetCode() == Opcode::Table::Branch) {
                const auto& inst_json = inst_list.back();
                const auto num_successors = inst_json.at("num_successors").get<std::int32_t>();
                if (num_successors == 1) {
                    inferred.emplace(inst_json.at("if_true").get<std::string>());
                } else if (num_successors == 2) {
                    inferred.emplace(inst_json.at("if_true").get<std::string>());
                    inferred.emplace(inst_json.at("if_false").get<std::string>());
                }
            } else if (last_opcode.GetCode() == Opcode::Table::Switch) {
                const auto& inst_json = inst_list.back();
                inferred.emplace(inst_json.at("default").get<std::string>());
                for (const auto& [_, target_json] : inst_json.at("case").items()) {
                    inferred.emplace(target_json.get<std::string>());
                }
            }
        }

        auto declared = std::unordered_set<std::string>{};
        for (const auto& succ_name : GetNameArray(bb_json.at("successors"))) {
            if (bb_names.contains(succ_name)) {
                declared.emplace(succ_name);
            }
        }
        if (declared != inferred) {
            auto declared_vec = std::vector<std::string>(declared.begin(), declared.end());
            auto inferred_vec = std::vector<std::string>(inferred.begin(), inferred.end());
            std::sort(declared_vec.begin(), declared_vec.end());
            std::sort(inferred_vec.begin(), inferred_vec.end());
            LOG_WARN(
                    "declared successors do not match terminator targets (func: {}, bb: {}, "
                    "declared: [{}], terminator: [{}]).",
                    function_name,
                    bb_name,
                    FormatNames(declared_vec),
                    FormatNames(inferred_vec)
            );
        }
    }

    auto pred_only = std::vector<CFGEdge>{};
    for (const auto& edge : edges_from_pred) {
        if (!edges_from_succ.contains(edge)) {
            pred_only.emplace_back(edge);
        }
    }
    if (!pred_only.empty()) {
        std::sort(pred_only.begin(), pred_only.end());
        LOG_WARN(
                "predecessor/successor mismatch: edges declared only in predecessors (func: {}): "
                "{}",
                function_name,
                FormatEdges(pred_only)
        );
    }

    auto succ_only = std::vector<CFGEdge>{};
    for (const auto& edge : edges_from_succ) {
        if (!edges_from_pred.contains(edge)) {
            succ_only.emplace_back(edge);
        }
    }
    if (!succ_only.empty()) {
        std::sort(succ_only.begin(), succ_only.end());
        LOG_WARN(
                "predecessor/successor mismatch: edges declared only in successors (func: {}): {}",
                function_name,
                FormatEdges(succ_only)
        );
    }
}
}  // namespace

// Forward declarations
void CreateBB(const Json& j, const BBMap& bmap, const FunctionMap& fmap);
void InsertInst(const Json& j, BasicBlock* parent, const BBMap& bmap, const FunctionMap& fmap);

void LoadFunction(const Json& j, const FunctionMap& fmap) {
    const auto function_name = j.at("name").get<std::string>();
    auto* func = FindFunctionOrThrow(fmap, function_name, "LoadFunction");

    ValidateFunctionJsonWarnings(j);

    const auto& argument_json = j.at("argument");
    const auto declared_num_qubits = argument_json.at("num_qubits").get<std::size_t>();
    const auto declared_num_registers = argument_json.at("num_registers").get<std::size_t>();

    auto qubit_names = std::unordered_set<std::string>{};
    auto loaded_num_qubits = std::size_t{0};
    if (argument_json.contains("qubits")) {
        for (const auto& [qname, qsize_json] : argument_json.at("qubits").items()) {
            const auto qsize = qsize_json.get<std::size_t>();
            if (!qubit_names.emplace(qname).second) {
                throw std::runtime_error(
                        fmt::format(
                                "Function '{}' has duplicated qubit argument name '{}'.",
                                function_name,
                                qname
                        )
                );
            }
            func->AddQubit(qname, qsize);
            loaded_num_qubits += qsize;
        }
    }
    if (loaded_num_qubits != declared_num_qubits) {
        throw std::runtime_error(
                fmt::format(
                        "Function '{}' has inconsistent qubit argument size: declared {}, loaded "
                        "{}.",
                        function_name,
                        declared_num_qubits,
                        loaded_num_qubits
                )
        );
    }

    auto register_names = std::unordered_set<std::string>{};
    auto loaded_num_registers = std::size_t{0};
    if (argument_json.contains("registers")) {
        for (const auto& [rname, rsize_json] : argument_json.at("registers").items()) {
            const auto rsize = rsize_json.get<std::size_t>();
            if (!register_names.emplace(rname).second) {
                throw std::runtime_error(
                        fmt::format(
                                "Function '{}' has duplicated register argument name '{}'.",
                                function_name,
                                rname
                        )
                );
            }
            func->AddRegister(rname, rsize);
            loaded_num_registers += rsize;
        }
    }
    if (loaded_num_registers != declared_num_registers) {
        throw std::runtime_error(
                fmt::format(
                        "Function '{}' has inconsistent register argument size: declared {}, "
                        "loaded {}.",
                        function_name,
                        declared_num_registers,
                        loaded_num_registers
                )
        );
    }
    func->SetNumTmpRegisters(j.at("num_tmp_registers").get<std::size_t>());

    // Create empty BBs at first.
    auto bmap = BBMap();
    const auto& bb_list = j.at("bb_list");
    if (bb_list.empty()) {
        throw std::runtime_error(
                fmt::format("Function '{}' must contain at least one BB.", function_name)
        );
    }
    for (const auto& tmp : bb_list) {
        const auto bb_name = tmp.at("name").get<std::string>();
        if (bmap.contains(bb_name)) {
            throw std::runtime_error(
                    fmt::format(
                            "Function '{}' contains duplicate BB name: '{}'.",
                            function_name,
                            bb_name
                    )
            );
        }
        auto* bb = BasicBlock::Create(bb_name, func);
        bmap[bb_name] = bb;
    }

    // Set entry point.
    auto* entry_point = FindBBOrThrow(bmap, j.at("entry_point").get<std::string>(), "entry point");
    func->SetEntryBB(entry_point);

    // Create BB contents.
    for (const auto& tmp : bb_list) {
        CreateBB(tmp, bmap, fmap);
    }

    // Set BB dependencies.
    if (!func->SetBBDeps()) {
        throw std::runtime_error(
                fmt::format("Failed to set BB dependency graph in function '{}'.", function_name)
        );
    }
    ValidateReturnConstraint(*func);
    func->RemoveUnreachableBBs();
    func->RemoveEmptyBBs();

    if (!func->IsDAG()) {
        LOG_WARN("the dependency graph of BB is not DAG (func: {})", func->GetName());
    }
}
void CreateBB(const Json& j, const BBMap& bmap, const FunctionMap& fmap) {
    const auto name = j.at("name").get<std::string>();
    auto* bb = FindBBOrThrow(bmap, name, "CreateBB");
    for (const auto& tmp : j.at("inst_list")) {
        InsertInst(tmp, bb, bmap, fmap);
    }
    // Do not load predecessors/successors directly.
    // Dependencies are rebuilt from terminator instructions by Function::SetBBDeps().
}
void InsertInst(const Json& j, BasicBlock* parent, const BBMap& bmap, const FunctionMap& fmap) {
    if (parent == nullptr || parent->GetParent() == nullptr) {
        throw std::runtime_error("InsertInst requires BB with a parent function");
    }
    const auto* func = parent->GetParent();
    const auto function_name = std::string(func->GetName());
    const auto bb_name = std::string(parent->GetName());

    Opcode opcode = Opcode::Table::UnaryOpsBegin;
    from_json(j, opcode);
    const auto code = opcode.GetCode();

    if (opcode.IsMeasurement()) {
        const auto q = Qubit{j.at("q").get<std::uint64_t>()};
        const auto r = Register{j.at("r").get<std::uint64_t>()};
        ValidateQubitRange(q, *func, function_name, bb_name, "Measurement", "q");
        ValidateRegisterRange(r, *func, function_name, bb_name, "Measurement", "r");
        MeasurementInst::Create(q, r, parent);
    } else if (opcode.IsUnary()) {
        const auto q = Qubit{j.at("q").get<std::uint64_t>()};
        ValidateQubitRange(q, *func, function_name, bb_name, "Unary", "q");
        if (opcode.IsParametrizedRotation()) {
            const auto theta = j.at("theta").get<FloatWithPrecision>();
            ParametrizedRotationInst::Create(opcode, q, theta, parent);
        } else {
            UnaryInst::Create(opcode, q, parent);
        }
    } else if (opcode.IsBinary()) {
        const auto q0 = Qubit{j.at("q0").get<std::uint64_t>()};
        const auto q1 = Qubit{j.at("q1").get<std::uint64_t>()};
        ValidateQubitRange(q0, *func, function_name, bb_name, "Binary", "q0");
        ValidateQubitRange(q1, *func, function_name, bb_name, "Binary", "q1");
        BinaryInst::Create(opcode, q0, q1, parent);
    } else if (opcode.IsTernary()) {
        const auto q0 = Qubit{j.at("q0").get<std::uint64_t>()};
        const auto q1 = Qubit{j.at("q1").get<std::uint64_t>()};
        const auto q2 = Qubit{j.at("q2").get<std::uint64_t>()};
        ValidateQubitRange(q0, *func, function_name, bb_name, "Ternary", "q0");
        ValidateQubitRange(q1, *func, function_name, bb_name, "Ternary", "q1");
        ValidateQubitRange(q2, *func, function_name, bb_name, "Ternary", "q2");
        TernaryInst::Create(opcode, q0, q1, q2, parent);
    } else if (opcode.IsMultiControl()) {
        const auto t = Qubit{j.at("target").get<std::uint64_t>()};
        ValidateQubitRange(t, *func, function_name, bb_name, "MultiControl", "target");
        auto control = std::vector<Qubit>();
        for (const auto& tmp : j.at("control")) {
            const auto c = Qubit{tmp.get<std::uint64_t>()};
            ValidateQubitRange(c, *func, function_name, bb_name, "MultiControl", "control");
            control.emplace_back(c);
        }
        MultiControlInst::Create(opcode, t, control, parent);
    } else if (opcode.IsStateInvariant()) {
        const auto theta = j.at("theta").get<FloatWithPrecision>();
        GlobalPhaseInst::Create(theta, parent);
    } else if (opcode.IsCall()) {
        const auto callee_name = j.at("callee").get<std::string>();
        auto* callee = FindFunctionOrThrow(
                fmap,
                callee_name,
                fmt::format("Call instruction (func: {}, bb: {})", function_name, bb_name)
        );
        auto operate = std::vector<Qubit>();
        for (const auto& tmp : j.at("operate")) {
            const auto q = Qubit{tmp.get<std::uint64_t>()};
            ValidateQubitRange(q, *func, function_name, bb_name, "Call", "operate");
            operate.emplace_back(q);
        }
        auto input = std::vector<Register>();
        for (const auto& tmp : j.at("input")) {
            const auto r = Register{tmp.get<std::uint64_t>()};
            ValidateRegisterRange(r, *func, function_name, bb_name, "Call", "input");
            input.emplace_back(r);
        }
        auto output = std::vector<Register>();
        for (const auto& tmp : j.at("output")) {
            const auto r = Register{tmp.get<std::uint64_t>()};
            ValidateRegisterRange(r, *func, function_name, bb_name, "Call", "output");
            output.emplace_back(r);
        }
        CallInst::Create(callee, operate, input, output, parent);
    } else if (opcode.IsClassical()) {
        auto function = j.at("function").get<PortableAnnotatedFunction>();
        auto input = std::vector<Register>();
        for (const auto& tmp : j.at("input")) {
            const auto r = Register{tmp.get<std::uint64_t>()};
            ValidateRegisterRange(r, *func, function_name, bb_name, "CallCF", "input");
            input.emplace_back(r);
        }
        auto output = std::vector<Register>();
        for (const auto& tmp : j.at("output")) {
            const auto r = Register{tmp.get<std::uint64_t>()};
            ValidateRegisterRange(r, *func, function_name, bb_name, "CallCF", "output");
            output.emplace_back(r);
        }
        CallCFInst::Create(function, input, output, parent);
    } else if (opcode.IsRandom()) {
        auto weights = j.at("weights").get<std::vector<double>>();
        auto registers = std::vector<Register>();
        for (const auto& tmp : j.at("registers")) {
            const auto r = Register{tmp.get<std::uint64_t>()};
            ValidateRegisterRange(
                    r,
                    *func,
                    function_name,
                    bb_name,
                    "DiscreteDistribution",
                    "registers"
            );
            registers.emplace_back(r);
        }
        DiscreteDistInst::Create(weights, registers, parent);
    } else if (opcode.IsTerminator()) {
        if (code == Opcode::Table::Branch) {
            const auto num = j.at("num_successors").get<std::int32_t>();
            const auto if_true_name = j.at("if_true").get<std::string>();
            auto* if_true = FindBBOrThrow(
                    bmap,
                    if_true_name,
                    fmt::format("Branch if_true (func: {}, bb: {})", function_name, bb_name)
            );
            if (num == 1) {
                BranchInst::Create(if_true, parent);
            } else if (num == 2) {
                const auto condition = Register{j.at("condition").get<std::uint64_t>()};
                const auto if_false_name = j.at("if_false").get<std::string>();
                auto* if_false = FindBBOrThrow(
                        bmap,
                        if_false_name,
                        fmt::format("Branch if_false (func: {}, bb: {})", function_name, bb_name)
                );
                ValidateRegisterRange(
                        condition,
                        *func,
                        function_name,
                        bb_name,
                        "Branch",
                        "condition"
                );
                BranchInst::Create(if_true, if_false, condition, parent);
            } else {
                throw std::runtime_error(
                        fmt::format(
                                "Branch num_successors must be 1 or 2, but got {} (func: {}, bb: "
                                "{}).",
                                num,
                                function_name,
                                bb_name
                        )
                );
            }
        } else if (code == Opcode::Table::Switch) {
            auto registers = std::vector<Register>{};
            for (const auto& tmp : GetSwitchRegistersJson(j, function_name, bb_name)) {
                const auto r = Register{tmp.get<std::uint64_t>()};
                ValidateRegisterRange(r, *func, function_name, bb_name, "Switch", "registers");
                registers.emplace_back(r);
            }
            auto* default_bb = FindBBOrThrow(
                    bmap,
                    j.at("default").get<std::string>(),
                    fmt::format("Switch default (func: {}, bb: {})", function_name, bb_name)
            );
            auto case_bb = std::map<std::uint64_t, BasicBlock*>{};
            for (const auto& [key, tmp] : j.at("case").items()) {
                case_bb[std::stoull(key)] = FindBBOrThrow(
                        bmap,
                        tmp.get<std::string>(),
                        fmt::format(
                                "Switch case {} (func: {}, bb: {})",
                                key,
                                function_name,
                                bb_name
                        )
                );
            }
            SwitchInst::Create(registers, default_bb, case_bb, parent);
        } else if (code == Opcode::Table::Return) {
            ReturnInst::Create(parent);
        } else {
            assert(0 && "unreachable");
        }
    } else if (opcode.IsOptHint()) {
        if (code == Opcode::Table::Clean) {
            const auto q = Qubit{j.at("q").get<std::uint64_t>()};
            ValidateQubitRange(q, *func, function_name, bb_name, "Clean", "q");
            CleanInst::Create(q, parent);
        } else if (code == Opcode::Table::CleanProb) {
            const auto q = Qubit{j.at("q").get<std::uint64_t>()};
            const auto prob = j.at("prob").get<FloatWithPrecision>();
            ValidateQubitRange(q, *func, function_name, bb_name, "CleanProb", "q");
            CleanProbInst::Create(q, prob, parent);
        } else if (code == Opcode::Table::DirtyBegin || code == Opcode::Table::DirtyEnd) {
            const auto q = Qubit{j.at("q").get<std::uint64_t>()};
            ValidateQubitRange(q, *func, function_name, bb_name, "Dirty", "q");
            DirtyInst::Create(opcode, q, parent);
        } else {
            assert(0 && "unreachable");
        }
    } else {
        assert(0 && "unreachable");
    }
    if (j.contains("metadata")) {
        for (const auto& md_json : j["metadata"]) {
            const auto kind = md_json["kind"].get<std::string>();
            if (kind == "Annotation") {
                (*parent->rbegin()).AddMDAnnotation(md_json["annot"].get<std::string>());
            } else if (kind == "Qubit") {
                (*parent->rbegin()).AddMDQubit(Qubit(md_json["q"].get<std::uint64_t>()));
            } else if (kind == "Register") {
                (*parent->rbegin()).AddMDRegister(Register(md_json["r"].get<std::uint64_t>()));
            } else if (kind == "Breakpoint") {
                (*parent->rbegin()).MarkAsBreakPoint(md_json["name"].get<std::string>());
            } else if (kind == "Location") {
                (*parent->rbegin())
                        .AddLocation(
                                md_json["file"].get<std::string>(),
                                md_json["line"].get<std::uint32_t>()
                        );
            } else {
                throw std::logic_error("unreachable");
            }
        }
    }
}
void LoadJson(const Json& j, IRContext& context) {
    if (!j.contains("name")) {
        throw std::runtime_error("JSON does not contain module name");
    }
    auto name = j["name"].get<std::string>();
    auto* module = Module::Create(name, context);

    // Load contents of a module.
    LoadJsonImpl(j, *module);
}
void LoadJsonImpl(const Json& j, Module& module) {
    module.SetName(j["name"].template get<std::string>());
    LOG_DEBUG("Loading module {}", module.GetName());

    auto fmap = FunctionMap();

    // Load functions already registered in context.
    for (auto& tmp : module.GetContext().owned_module) {
        for (auto& func : *tmp) {
            fmap[std::string(func.GetName())] = &func;
        }
    }

    // Create empty functions first.
    for (const auto& tmp : j["circuit_list"]) {
        if (!tmp.contains("name")) {
            throw std::runtime_error("JSON does not contain func name");
        }

        const auto function_name = tmp["name"].get<std::string>();
        if (fmap.contains(function_name)) {
            throw std::runtime_error(
                    fmt::format("JSON contains duplicate function name: {}", function_name)
            );
        }

        auto* func = Function::Create(function_name, &module);
        fmap[function_name] = func;
    }

    // Load function contents.
    for (const auto& tmp : j["circuit_list"]) {
        LOG_DEBUG("Loading module {}", module.GetName());
        LoadFunction(tmp, fmap);
    }

    ValidateCallArgumentSize(module);
}
void from_json(const Json& j, FloatWithPrecision& v) {
    j.at("value").get_to(v.value);
    j.at("precision").get_to(v.precision);
}
void from_json(const Json& j, Opcode& opcode) {
    opcode = Opcode::FromString(j.at("opcode").get<std::string>());
}
}  // namespace qret::ir
