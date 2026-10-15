/**
 * @file qret/parser/qir.h
 * @brief QIR (.ll text) parser.
 * @details This file defines a parser of the static-allocation subset of QIR that
 * qret::qir::ExportModuleToQIR (see qret/exporter/qir.h) emits, plus the name constants and
 * call-graph check shared with the exporter.
 */

#ifndef QRET_PARSER_QIR_H
#define QRET_PARSER_QIR_H

#include <fmt/format.h>

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "qret/error.h"
#include "qret/exception.h"
#include "qret/qret_export.h"

namespace qret::qir {
//--------------------------------------------------//
// Name constants shared by the exporter and importer, following QIR's naming convention.
//--------------------------------------------------//

// QIS (Quantum Instruction Set) functions: `__quantum__qis__*`
inline constexpr std::string_view kI = "__quantum__qis__id__body";
inline constexpr std::string_view kX = "__quantum__qis__x__body";
inline constexpr std::string_view kY = "__quantum__qis__y__body";
inline constexpr std::string_view kZ = "__quantum__qis__z__body";
inline constexpr std::string_view kH = "__quantum__qis__h__body";
inline constexpr std::string_view kS = "__quantum__qis__s__body";
inline constexpr std::string_view kSAdj = "__quantum__qis__s__adj";
inline constexpr std::string_view kT = "__quantum__qis__t__body";
inline constexpr std::string_view kTAdj = "__quantum__qis__t__adj";
inline constexpr std::string_view kRX = "__quantum__qis__rx__body";
inline constexpr std::string_view kRY = "__quantum__qis__ry__body";
inline constexpr std::string_view kRZ = "__quantum__qis__rz__body";
inline constexpr std::string_view kCNot = "__quantum__qis__cnot__body";
// qsc's alias for kCNot.
inline constexpr std::string_view kCXQis = "__quantum__qis__cx__body";
inline constexpr std::string_view kCY = "__quantum__qis__cy__body";
inline constexpr std::string_view kCZ = "__quantum__qis__cz__body";
inline constexpr std::string_view kCCX = "__quantum__qis__ccx__body";
inline constexpr std::string_view kCCY = "__quantum__qis__ccy__body";
inline constexpr std::string_view kCCZ = "__quantum__qis__ccz__body";
inline constexpr std::string_view kSwap = "__quantum__qis__swap__body";
// Two-qubit Pauli rotations, recognized on import only.
inline constexpr std::string_view kRxxQis = "__quantum__qis__rxx__body";
inline constexpr std::string_view kRyyQis = "__quantum__qis__ryy__body";
inline constexpr std::string_view kRzzQis = "__quantum__qis__rzz__body";
// sqrt(X), recognized on import only.
inline constexpr std::string_view kSXQis = "__quantum__qis__sx__body";
// A scheduling hint with no effect on the state.
inline constexpr std::string_view kBarrierQis = "__quantum__qis__barrier__body";
inline constexpr std::string_view kMz = "__quantum__qis__mz__body";
// qsc's alias for kMz (a plain measurement).
inline constexpr std::string_view kMQis = "__quantum__qis__m__body";
// Measure-and-reset fused into one call (Q#'s MResetZ).
inline constexpr std::string_view kMResetZQis = "__quantum__qis__mresetz__body";
inline constexpr std::string_view kReset = "__quantum__qis__reset__body";
// The qis-namespace read_result alias (cf. kReadResultRt).
inline constexpr std::string_view kReadResultQis = "__quantum__qis__read_result__body";

// Runtime functions: `__quantum__rt__*`
inline constexpr std::string_view kRtInitialize = "__quantum__rt__initialize";
inline constexpr std::string_view kRtResultRecordOutput = "__quantum__rt__result_record_output";
inline constexpr std::string_view kRtBoolRecordOutput = "__quantum__rt__bool_record_output";
inline constexpr std::string_view kRtIntRecordOutput = "__quantum__rt__int_record_output";
inline constexpr std::string_view kRtArrayRecordOutput = "__quantum__rt__array_record_output";
inline constexpr std::string_view kRtTupleRecordOutput = "__quantum__rt__tuple_record_output";
inline constexpr std::string_view kRtQubitAllocate = "__quantum__rt__qubit_allocate";
inline constexpr std::string_view kRtQubitRelease = "__quantum__rt__qubit_release";
// The spec-compliant read_result name (cf. kReadResultQis).
inline constexpr std::string_view kReadResultRt = "__quantum__rt__read_result";

// Entry point / function attributes
inline constexpr std::string_view kAttrEntryPoint = "entry_point";
inline constexpr std::string_view kAttrQirProfiles = "qir_profiles";
inline constexpr std::string_view kProfileAdaptive = "adaptive_profile";
inline constexpr std::string_view kAttrRequiredNumQubits = "required_num_qubits";
inline constexpr std::string_view kAttrRequiredNumResults = "required_num_results";
inline constexpr std::string_view kAttrOutputLabelingSchema = "output_labeling_schema";
inline constexpr std::string_view kAttrIrreversible = "irreversible";

// Module flags: `!llvm.module.flags`
inline constexpr std::string_view kFlagQirMajorVersion = "qir_major_version";
inline constexpr std::string_view kFlagQirMinorVersion = "qir_minor_version";
inline constexpr std::string_view kFlagDynamicQubitManagement = "dynamic_qubit_management";
inline constexpr std::string_view kFlagDynamicResultManagement = "dynamic_result_management";
inline constexpr std::string_view kFlagIrFunctions = "ir_functions";
inline constexpr std::string_view kFlagIntComputations = "int_computations";
// The module flag marking use of `switch` (multi-target branching).
inline constexpr std::string_view kFlagMultipleTargetBranching = "multiple_target_branching";

inline constexpr std::int32_t kQirMajorVersion = 2;
inline constexpr std::int32_t kQirMinorVersion = 1;

//--------------------------------------------------//
// QIS/RT intrinsic catalog: one source of truth for each intrinsic's argument structure, shared by
// the importer (to classify a sub-circuit parameter from how it's passed to an intrinsic) and the
// exporter (to emit each intrinsic's `declare` line).
//--------------------------------------------------//

/**
 * @brief The role an intrinsic gives an argument position; non-qubit/result positions (a rotation
 * angle, a recorded value, an output label) are Unclassified.
 */
enum class QRET_EXPORT ArgRole : std::uint8_t { Qubit, RegisterRead, RegisterWrite, Unclassified };

/**
 * @brief One argument of an intrinsic: its role (for import classification) and the LLVM type text
 * the exporter declares it with.
 */
struct QRET_EXPORT IntrinsicArg {
    ArgRole role;
    std::string_view decl_type;  // "ptr", "ptr writeonly", "ptr readonly", "double", "i64", or "i1"
};

/**
 * @brief A QIS/RT intrinsic's full signature.
 */
struct QRET_EXPORT IntrinsicSignature {
    std::vector<IntrinsicArg> args;
    std::string_view return_type;  // "void", "i64", or "i1"
    std::string_view attrs;  // trailing function attributes such as " #1", else ""
};

/**
 * @brief The shared intrinsic catalog: name -> signature for every recognized QIS/RT function.
 */
QRET_EXPORT const std::unordered_map<std::string_view, IntrinsicSignature>& IntrinsicCatalog();

//--------------------------------------------------//
// Parser
//--------------------------------------------------//

/**
 * @brief Whether QIR text can be parsed (false unless built with USE_PEGTL).
 */
QRET_EXPORT bool CanParseQIR();

//--------------------------------------------------//
// AST
//--------------------------------------------------//

/**
 * @brief A `call` instruction: `call void @fn(args)` or `%result = call i1 @fn(args)`.
 */
struct QRET_EXPORT ParsedCall {
    std::string function;
    std::vector<std::string> args;
    std::string result_var;  // e.g. "%c0" for "%c0 = call i1 @...(...)"; empty otherwise.
};

/**
 * @brief A classical op over i1/i64 values (`zext` or a binary op); operands are an SSA name or
 * literal.
 */
struct QRET_EXPORT ParsedIntInst {
    enum class Kind : std::uint8_t { Zext, Shl, Ashr, Add, Sub, Mul, Sdiv, Srem, And, Or, Xor };

    Kind kind;
    std::string result_var;
    std::string operand0;  // zext: the i1 source; binary ops: the first operand
    std::string operand1;  // zext: unused (empty); binary ops: the second operand
    // Only meaningful for And/Or/Xor: whether this is the i1-typed form rather than i64-typed.
    bool is_bool_typed = false;
};

/**
 * @brief An `icmp`, a candidate branch-condition producer; only signed predicates are accepted.
 */
struct QRET_EXPORT ParsedIcmpInst {
    enum class Predicate : std::uint8_t { Eq, Ne, Slt, Sle, Sgt, Sge };

    Predicate pred;
    std::string result_var;
    std::string lhs;
    std::string rhs;
    bool is_bool_typed = false;  // `icmp ... i1 ...` rather than `icmp ... i64 ...`
};

/**
 * @brief A `phi` merging a value across exactly 2 predecessor blocks (a simple if/else's shape).
 */
struct QRET_EXPORT ParsedPhi {
    std::string result_var;
    std::string block_name;  // maps predecessors to the enclosing conditional's True/False arm
    std::vector<std::pair<std::string, std::string>> incoming;  // (value, predecessor block) pairs
    bool is_bool_typed = false;  // `phi i1 ...` rather than `phi i64 ...`
};

/**
 * @brief A `select`; unlike `phi`, `cond` is a plain Bool operand (the shape Mux's array read
 * produces).
 */
struct QRET_EXPORT ParsedSelectInst {
    std::string result_var;
    std::string cond;  // always i1-typed; an SSA name, or the keyword literal "true"/"false"
    std::string true_value;
    std::string false_value;
    bool is_bool_typed = false;  // `..., i1 ..., i1 ...` rather than `..., i64 ..., i64 ...`
};

/**
 * @brief A basic block: a label, a straight-line body, and exactly one terminator.
 */
struct QRET_EXPORT ParsedBlock {
    enum class Term : std::uint8_t { Unset, Return, BrUncond, BrCond, Switch };

    using Item =
            std::variant<ParsedCall, ParsedIntInst, ParsedIcmpInst, ParsedPhi, ParsedSelectInst>;

    std::string name;
    std::vector<Item> items;  // in original QIR text order
    Term term = Term::Unset;
    std::string target_true;
    std::string target_false;
    std::string cond_var;
    std::string switch_value;
    std::string switch_default;
    std::vector<std::pair<std::uint64_t, std::string>> switch_cases;
};

/**
 * @brief A sub-circuit parameter's declared pointer type; v1's typed pointers name the answer
 * directly, v2's opaque `ptr`/`i8*` needs usage-based inference (see ClassifyParameters in
 * qir.cpp).
 */
enum class QRET_EXPORT DeclaredPtrType : std::uint8_t { Qubit, Result, Opaque };

/**
 * @brief A parameter's resolved kind, decided by how it's used in its own function's body.
 */
enum class QRET_EXPORT ParamKind : std::uint8_t { Qubit, RegisterInput, RegisterOutput };

/**
 * @brief A sub-circuit's formal parameter, in declaration order; `kind` is empty until parsing
 * finishes classifying the whole module.
 */
struct QRET_EXPORT ParsedParam {
    std::string name;
    DeclaredPtrType declared_type;
    std::optional<ParamKind> kind;
};

/**
 * @brief A sub-circuit's register param; the name carries no meaning, `is_output` comes from its
 * resolved ParamKind.
 */
struct QRET_EXPORT ParsedRegisterParam {
    std::string name;
    bool is_output = false;
};

/**
 * @brief A function definition: the entry function, or a sub-circuit.
 */
struct QRET_EXPORT ParsedFunction {
    std::string name;
    std::vector<ParsedParam> raw_params;  // declaration order, as parsed
    std::vector<std::string> qubit_params;  // derived from raw_params once classified
    std::vector<ParsedRegisterParam> register_params;  // derived from raw_params once classified
    std::optional<std::uint32_t> attr_group;  // the "#N" trailing the `define` line, if any
    std::vector<ParsedBlock> blocks;
};

/**
 * @brief A full parsed QIR module: its functions plus the module-level attribute facts below.
 */
struct QRET_EXPORT ParsedModule {
    std::vector<ParsedFunction> functions;
    // "#N" -> required_num_qubits
    std::unordered_map<std::uint32_t, std::size_t> attr_group_required_qubits;
    // "#N" marked with the "entry_point" attribute
    std::unordered_set<std::uint32_t> attr_group_entry_point;
    // "N" (digits after '@') -> decoded label text of a `*_record_output` call's output label.
    std::unordered_map<std::string, std::string> global_string_labels;
};

/**
 * @brief Parse QIR (.ll) text into a lightweight AST.
 * @details Throws QRETError(error::MalformedQIR, ...) if `text` is not valid QIR, or is outside the
 * subset this parser accepts; QRETError(error::UnsupportedQIRInstruction, ...) for a recursive call
 * graph (rejected by CheckCallGraphIsAcyclic).
 */
QRET_EXPORT ParsedModule ParseQIRText(std::string_view text);

//--------------------------------------------------//
// Call-graph check shared by the QIR parser and exporter.
//--------------------------------------------------//

/**
 * @brief Throw if the call graph (an adjacency list of callees) has a cycle, since the QIR profiles
 * forbid recursive function calls (direct or indirect); this is distinct from backward branching,
 * which is intra-function control flow.
 * @details `get_name` maps a node to its display name for the error message. Used by the parser
 * (over ParsedFunction, per parsed module) and the exporter (over ir::Function).
 */
template <typename T, typename GetName>
void CheckCallGraphIsAcyclic(
        const std::vector<const T*>& all_nodes,
        const std::unordered_map<const T*, std::vector<const T*>>& callees,
        GetName get_name
) {
    auto visiting = std::unordered_set<const T*>();
    auto done = std::unordered_set<const T*>();
    const std::function<void(const T*)> dfs =
            [&visiting, &done, &callees, &get_name, &dfs](const T* f) {
                if (done.contains(f)) {
                    return;
                }
                if (visiting.contains(f)) {
                    throw QRETError(
                            error::UnsupportedQIRInstruction,
                            fmt::format(
                                    "recursive call involving function '{}' is not supported by "
                                    "QIR",
                                    get_name(f)
                            )
                    );
                }
                visiting.insert(f);
                const auto it = callees.find(f);
                if (it != callees.end()) {
                    for (const auto* callee : it->second) {
                        dfs(callee);
                    }
                }
                visiting.erase(f);
                done.insert(f);
            };
    for (const auto* node : all_nodes) {
        dfs(node);
    }
}
}  // namespace qret::qir

#endif  // QRET_PARSER_QIR_H
