/**
 * @file qret/exporter/qir.cpp
 * @brief Export Quration-IR to QIR (.ll text).
 */

#include "qret/exporter/qir.h"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "qret/base/cast.h"
#include "qret/base/log.h"
#include "qret/base/portable_function.h"
#include "qret/error.h"
#include "qret/exception.h"
#include "qret/ir/basic_block.h"
#include "qret/ir/function.h"
#include "qret/ir/instruction.h"
#include "qret/ir/instructions.h"
#include "qret/ir/value.h"
#include "qret/parser/qir.h"

namespace qret::qir {
namespace {
std::string FormatDouble(double value) {
    auto s = fmt::format("{:.17g}", value);
    const auto has_float_syntax = s.find('.') != std::string::npos
            || s.find('e') != std::string::npos || s.find('E') != std::string::npos
            || s.find("inf") != std::string::npos || s.find("nan") != std::string::npos;
    if (!has_float_syntax) {
        s += ".0";
    }
    return s;
}

// Replaces characters LLVM identifiers can't contain, and prefixes names starting with a digit.
// Not injective ("foo-bar" and "foo_bar" collide), so callers uniquify via MakeUnique.
std::string SanitizeIdentifier(std::string_view name) {
    auto out = std::string();
    out.reserve(name.size());
    for (const char c : name) {
        const auto ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
                || c == '_' || c == '.';
        out += ok ? c : '_';
    }
    if (out.empty() || (out.front() >= '0' && out.front() <= '9')) {
        out = "v_" + out;
    }
    return out;
}

// Appends _1, _2, ... to `base` until it is absent from `used`, then records and returns it.
std::string MakeUnique(std::string base, std::unordered_set<std::string>& used) {
    if (used.insert(base).second) {
        return base;
    }
    for (auto suffix = std::uint32_t{1};; ++suffix) {
        auto candidate = fmt::format("{}_{}", base, suffix);
        if (used.insert(candidate).second) {
            return candidate;
        }
    }
}

// Escapes a byte string for an LLVM `c"..."` literal. `\XX` denotes one byte, so the decoded
// length is unchanged and callers keep sizing the array as text.size() + 1.
std::string EscapeLLVMString(std::string_view text) {
    auto out = std::string();
    out.reserve(text.size());
    for (const char c : text) {
        const auto byte = static_cast<unsigned char>(c);
        const auto is_verbatim = byte >= 0x20 && byte <= 0x7E && byte != '"' && byte != '\\';
        if (is_verbatim) {
            out += c;
        } else {
            out += fmt::format("\\{:02X}", byte);
        }
    }
    return out;
}

// Builds the `declare` line from the intrinsic's shared signature (see IntrinsicCatalog), so the
// declared parameter types stay in lockstep with how the importer classifies the same intrinsic.
std::string DeclareLineFor(std::string_view name) {
    const auto& catalog = IntrinsicCatalog();
    const auto it = catalog.find(name);
    if (it == catalog.end()) {
        throw QRETError(
                error::UnsupportedQIRInstruction,
                fmt::format("no known QIR declaration for '{}'", name)
        );
    }
    const auto& sig = it->second;
    auto param_types = std::vector<std::string_view>();
    param_types.reserve(sig.args.size());
    for (const auto& arg : sig.args) {
        param_types.emplace_back(arg.decl_type);
    }
    return fmt::format(
            "declare {} @{}({}){}",
            sig.return_type,
            name,
            fmt::join(param_types, ", "),
            sig.attrs
    );
}

/**
 * @brief Emits a Quration-IR module as QIR (.ll) text.
 */
class QirExporter {
public:
    QirExporter(const ir::Module& module, std::string_view entry_name)
        : module_{module} {
        if (entry_name.empty()) {
            // ir::Module has no dedicated entry-function field; default to the first one.
            if (module_.begin() == module_.end()) {
                throw QRETError(error::MalformedQIR, "module has no functions to export");
            }
            entry_ = &*module_.begin();
        } else {
            entry_ = module_.GetFunction(entry_name);
            if (entry_ == nullptr) {
                throw QRETError(
                        error::MalformedQIR,
                        fmt::format("entry function '{}' not found in module", entry_name)
                );
            }
        }
    }

    std::string Run() {
        RejectRecursiveCalls();
        RegisterFunctionNames();

        auto functions_text = std::ostringstream();
        EmitFunction(entry_, functions_text);
        for (const auto& func : module_) {
            if (&func == entry_) {
                continue;
            }
            EmitFunction(&func, functions_text);
        }

        auto out = std::ostringstream();
        EmitGlobals(out);
        out << functions_text.str();
        EmitDeclares(out);
        EmitAttributeGroups(out);
        EmitModuleFlags(out);
        EmitExportWarnings();
        return out.str();
    }

private:
    // QIR forbids recursion, so the module's call graph is checked for cycles up front.
    void RejectRecursiveCalls() const {
        auto all_nodes = std::vector<const ir::Function*>();
        auto callees = std::unordered_map<const ir::Function*, std::vector<const ir::Function*>>();
        for (const auto& func : module_) {
            all_nodes.emplace_back(&func);
            for (const auto& bb : func) {
                for (const auto& inst : bb) {
                    if (inst.IsCall()) {
                        callees[&func].emplace_back(Cast<ir::CallInst>(&inst)->GetCallee());
                    }
                }
            }
        }
        qret::qir::CheckCallGraphIsAcyclic<ir::Function>(
                all_nodes,
                callees,
                [](const ir::Function* f) { return f->GetName(); }
        );
    }

    // Assigns @-names in definition order (entry first) so the first definition keeps the
    // un-suffixed name, regardless of call-vs-definition emission order.
    void RegisterFunctionNames() {
        UniqueFunctionName(entry_);
        for (const auto& func : module_) {
            if (&func != entry_) {
                UniqueFunctionName(&func);
            }
        }
    }
    // Module-unique @-name, cached so definition and call sites agree.
    std::string UniqueFunctionName(const ir::Function* func) {
        const auto it = function_names_.find(func);
        if (it != function_names_.end()) {
            return it->second;
        }
        auto name = MakeUnique(SanitizeIdentifier(func->GetName()), used_function_names_);
        return function_names_.emplace(func, std::move(name)).first->second;
    }
    // Function-local %-label; LLVM labels are function-scoped, so the maps are cleared per function.
    std::string UniqueBlockLabel(const ir::BasicBlock* bb) {
        const auto it = block_labels_.find(bb);
        if (it != block_labels_.end()) {
            return it->second;
        }
        auto label = MakeUnique(SanitizeIdentifier(bb->GetName()), used_block_labels_);
        return block_labels_.emplace(bb, std::move(label)).first->second;
    }

    std::string PtrValue(std::uint64_t id) const {
        return id == 0 ? std::string("null") : fmt::format("inttoptr (i64 {} to ptr)", id);
    }
    std::string PtrArg(std::uint64_t id) const {
        return fmt::format("ptr {}", PtrValue(id));
    }
    // Entry function: qubit id is the physical address. Sub-circuit: a formal `ptr` parameter.
    std::string QubitRef(const ir::Function* func, std::uint64_t id) const {
        if (func == entry_) {
            return PtrArg(id);
        }
        return fmt::format("ptr %q{}", id);
    }

    // Used by GetRegisterIsOutput, since register count alone doesn't say Input vs Output.
    const ir::CallInst* FindAnyCallTo(const ir::Function* callee) const {
        for (const auto& func : module_) {
            for (const auto& bb : func) {
                for (const auto& inst : bb) {
                    if (inst.IsCall() && Cast<ir::CallInst>(&inst)->GetCallee() == callee) {
                        return Cast<ir::CallInst>(&inst);
                    }
                }
            }
        }
        return nullptr;
    }
    // ir::Function doesn't track Input/Output per id; recovered from a call site's input count.
    const std::vector<bool>& GetRegisterIsOutput(const ir::Function* callee) {
        const auto cached = register_is_output_.find(callee);
        if (cached != register_is_output_.end()) {
            return cached->second;
        }
        const auto* call = FindAnyCallTo(callee);
        if (call == nullptr) {
            throw QRETError(
                    error::UnsupportedQIRInstruction,
                    fmt::format(
                            "sub-circuit '{}' has classical register arguments but is never called "
                            "in the module; its Input/Output attribution cannot be determined",
                            callee->GetName()
                    )
            );
        }
        auto is_output = std::vector<bool>(callee->GetNumRegisters(), false);
        for (auto i = call->GetInput().size(); i < callee->GetNumRegisters(); ++i) {
            is_output[i] = true;
        }
        return register_is_output_.emplace(callee, std::move(is_output)).first->second;
    }

    std::uint64_t AssignResultId(const ir::Function* func, std::uint64_t reg_id) {
        const auto id = next_result_id_++;
        result_ids_[{func, reg_id}] = id;
        return id;
    }
    std::uint64_t LookupResultId(const ir::Function* func, std::uint64_t reg_id) const {
        const auto it = result_ids_.find({func, reg_id});
        if (it == result_ids_.end()) {
            throw QRETError(
                    error::UnsupportedQIRInstruction,
                    fmt::format(
                            "register r{} in function '{}' is read before being produced by a "
                            "measurement; only measurement-derived register values can be exported "
                            "to QIR",
                            reg_id,
                            func->GetName()
                    )
            );
        }
        return it->second;
    }

    // Mirrors QubitRef: own register uses its `ptr` param, else a literal Result identity.
    std::string RegisterRefRead(const ir::Function* func, std::uint64_t reg_id) const {
        if (func != entry_ && reg_id < func->GetNumRegisters()) {
            return fmt::format("ptr %r{}", reg_id);
        }
        return PtrArg(LookupResultId(func, reg_id));
    }
    // Like RegisterRefRead, but assigns a fresh identity on first write instead of requiring one.
    std::string RegisterRefWrite(const ir::Function* func, std::uint64_t reg_id) {
        if (func != entry_ && reg_id < func->GetNumRegisters()) {
            return fmt::format("ptr %r{}", reg_id);
        }
        const auto it = result_ids_.find({func, reg_id});
        const auto id = it != result_ids_.end() ? it->second : AssignResultId(func, reg_id);
        return PtrArg(id);
    }

    std::string MakeGlobalLabel(std::string_view text) {
        const auto index = global_labels_.size();
        global_labels_.emplace_back(
                fmt::format(
                        "@{} = internal constant [{} x i8] c\"{}\\00\"",
                        index,
                        text.size() + 1,
                        EscapeLLVMString(text)
                )
        );
        return fmt::format("ptr @{}", index);
    }

    void
    EmitCall(std::ostringstream& out, std::string_view fn, const std::vector<std::string>& args) {
        used_functions_.emplace(fn);
        out << fmt::format("  call void @{}({})\n", fn, fmt::to_string(fmt::join(args, ", ")));
    }
    std::string EmitReadResult(std::ostringstream& out, std::string_view result_arg) {
        used_functions_.emplace(kReadResultRt);
        const auto result_ssa = fmt::format("%c{}", next_tmp_id_++);
        out << fmt::format("  {} = call i1 @{}({})\n", result_ssa, kReadResultRt, result_arg);
        return result_ssa;
    }
    // Reuses a CallCF-computed SSA name if there is one; otherwise emits a read_result call.
    std::string
    ResolveConditionSSA(const ir::Function* func, std::uint64_t reg_id, std::ostringstream& out) {
        const auto it = callcf_output_ssa_.find({func, reg_id});
        if (it != callcf_output_ssa_.end()) {
            return it->second;
        }
        return EmitReadResult(out, RegisterRefRead(func, reg_id));
    }
    // Key into reg_ssa: a scalar's DirectAccess name, or "arr[idx]" for a constant-index element.
    static std::string PortableOperandKey(const PortableFunction::RegisterAccessor& accessor) {
        if (accessor.IsDirectAccess()) {
            return accessor.GetDirectAccess();
        }
        const auto [array_name, idx] = accessor.GetIndexedAccessByImmediate();
        return fmt::format("{}[{}]", array_name, idx);
    }
    static const std::string&
    LookupSSA(const std::unordered_map<std::string, std::string>& reg_ssa, const std::string& key) {
        const auto it = reg_ssa.find(key);
        if (it == reg_ssa.end()) {
            throw QRETError(
                    error::UnsupportedQIRInstruction,
                    fmt::format("register '{}' has no SSA value bound while exporting to QIR", key)
            );
        }
        return it->second;
    }
    // The element type of `array_name` as declared on `function` (its Input/Output/TmpVariable
    // maps), rather than sniffing PortableFunctionBuilder's internal naming convention.
    static bool IsBoolArrayName(const PortableFunction& function, const std::string& array_name) {
        for (const auto* registers :
             {&function.Input(), &function.Output(), &function.TmpVariable()}) {
            if (const auto it = registers->find(array_name); it != registers->end()) {
                return it->second.IsBoolArray();
            }
        }
        throw QRETError(
                error::UnsupportedQIRInstruction,
                fmt::format(
                        "array '{}' is not a declared register of its PortableFunction",
                        array_name
                )
        );
    }
    // Resolves an operand into QIR text; Mux's internal array read becomes an inline `select`.
    std::string ResolvePortableOperand(
            const PortableFunction& function,
            const PortableFunction::RegisterAccessor& accessor,
            const std::unordered_map<std::string, std::string>& reg_ssa,
            std::ostringstream& out
    ) {
        if (accessor.IsImmediate()) {
            return std::to_string(accessor.GetImmediate());
        }
        if (accessor.IsDirectAccess()) {
            return LookupSSA(reg_ssa, accessor.GetDirectAccess());
        }
        if (accessor.IsIndexedAccessByImmediate()) {
            const auto [array_name, idx] = accessor.GetIndexedAccessByImmediate();
            return LookupSSA(reg_ssa, fmt::format("{}[{}]", array_name, idx));
        }
        const auto [array_name, idx_reg_name] = accessor.GetIndexedAccessByRegister();
        const auto* type = IsBoolArrayName(function, array_name) ? "i1" : "i64";
        // The index register is always Int; convert it back to i1 for `select`'s condition.
        const auto idx_int_ssa = LookupSSA(reg_ssa, idx_reg_name);
        const auto cond_ssa = fmt::format("%v{}", next_tmp_id_++);
        out << fmt::format("  {} = icmp ne i64 {}, 0\n", cond_ssa, idx_int_ssa);
        const auto false_value = LookupSSA(reg_ssa, fmt::format("{}[0]", array_name));
        const auto true_value = LookupSSA(reg_ssa, fmt::format("{}[1]", array_name));
        const auto selected_ssa = fmt::format("%v{}", next_tmp_id_++);
        out << fmt::format(
                "  {} = select i1 {}, {} {}, {} {}\n",
                selected_ssa,
                cond_ssa,
                type,
                true_value,
                type,
                false_value
        );
        return selected_ssa;
    }
    // Emits one PortableFunction instruction as at most one QIR line (Copy* needs none).
    void EmitPortableFunctionInstruction(
            const PortableFunction& function,
            const std::string& op,
            const std::vector<PortableFunction::RegisterAccessor>& operands,
            std::unordered_map<std::string, std::string>& reg_ssa,
            std::ostringstream& out
    ) {
        static const auto kUnary = std::unordered_map<std::string, std::string>{
                {"BoolToInt", "zext i1 {} to i64"},
                {"IntToBool", "icmp ne i64 {}, 0"},
                {"LNot", "xor i1 {}, true"},
                {"BNot", "xor i64 {}, -1"},
                {"Neg", "sub i64 0, {}"},
        };
        static const auto kBinary = std::unordered_map<std::string, std::string>{
                {"LAnd", "and i1"},
                {"LOr", "or i1"},
                {"LXor", "xor i1"},
                {"BAnd", "and i64"},
                {"BOr", "or i64"},
                {"BXor", "xor i64"},
                {"LShift", "shl i64"},
                {"RShift", "ashr i64"},
                {"Add", "add i64"},
                {"Sub", "sub i64"},
                {"Mul", "mul i64"},
                {"Div", "sdiv i64"},
                {"Mod", "srem i64"},
        };
        static const auto kCompare = std::unordered_map<std::string, std::string>{
                {"Eq", "eq"},
                {"Ne", "ne"},
                {"Lt", "slt"},
                {"Le", "sle"},
                {"Gt", "sgt"},
                {"Ge", "sge"},
        };
        const auto dst_key = PortableOperandKey(operands.at(0));
        if (op == "CopyBool" || op == "CopyInt") {
            reg_ssa[dst_key] = ResolvePortableOperand(function, operands.at(1), reg_ssa, out);
            return;
        }
        if (const auto it = kUnary.find(op); it != kUnary.end()) {
            const auto src = ResolvePortableOperand(function, operands.at(1), reg_ssa, out);
            const auto result_ssa = fmt::format("%v{}", next_tmp_id_++);
            out << "  " << result_ssa << " = " << fmt::format(fmt::runtime(it->second), src)
                << "\n";
            reg_ssa[dst_key] = result_ssa;
            return;
        }
        const auto lhs_accessor = operands.at(1);
        const auto rhs_accessor = operands.at(2);
        if (const auto it = kBinary.find(op); it != kBinary.end()) {
            const auto lhs = ResolvePortableOperand(function, lhs_accessor, reg_ssa, out);
            const auto rhs = ResolvePortableOperand(function, rhs_accessor, reg_ssa, out);
            const auto result_ssa = fmt::format("%v{}", next_tmp_id_++);
            out << fmt::format("  {} = {} {}, {}\n", result_ssa, it->second, lhs, rhs);
            reg_ssa[dst_key] = result_ssa;
            return;
        }
        if (const auto it = kCompare.find(op); it != kCompare.end()) {
            const auto lhs = ResolvePortableOperand(function, lhs_accessor, reg_ssa, out);
            const auto rhs = ResolvePortableOperand(function, rhs_accessor, reg_ssa, out);
            const auto result_ssa = fmt::format("%v{}", next_tmp_id_++);
            out << fmt::format("  {} = icmp {} i64 {}, {}\n", result_ssa, it->second, lhs, rhs);
            reg_ssa[dst_key] = result_ssa;
            return;
        }
        throw QRETError(
                error::UnsupportedQIRInstruction,
                fmt::format("PortableFunction instruction '{}' cannot be exported to QIR", op)
        );
    }
    // Inline-expands a CallCFInst, recording each output's SSA name so it skips read_result later.
    void EmitCallCF(const ir::Function* func, const ir::CallCFInst& c, std::ostringstream& out) {
        used_int_computations_ = true;
        const auto& function = c.GetFunction();
        const auto& input = c.GetInput();
        const auto& output = c.GetOutput();

        auto reg_ssa = std::unordered_map<std::string, std::string>();
        for (auto i = std::size_t{0}; i < input.size(); ++i) {
            reg_ssa[fmt::format("input[{}]", i)] = ResolveConditionSSA(func, input[i].id, out);
        }
        for (const auto& [op, operands] : function.DumpInstructions()) {
            EmitPortableFunctionInstruction(function, op, operands, reg_ssa, out);
        }
        for (auto i = std::size_t{0}; i < output.size(); ++i) {
            callcf_output_ssa_[{func, output[i].id}] =
                    LookupSSA(reg_ssa, fmt::format("output[{}]", i));
        }
    }
    // Combines `bit_regs` (LSB0) into one i64 via ResolveConditionSSA + zext, then shl/or per bit.
    std::string EmitCombinedI64Value(
            const ir::Function* func,
            const std::vector<ir::Register>& bit_regs,
            std::ostringstream& out
    ) {
        auto combined = std::string();
        for (auto i = std::size_t{0}; i < bit_regs.size(); ++i) {
            const auto bit = ResolveConditionSSA(func, bit_regs[i].id, out);
            const auto ext = fmt::format("%v{}", next_tmp_id_++);
            out << fmt::format("  {} = zext i1 {} to i64\n", ext, bit);
            if (i == 0) {
                combined = ext;
                continue;
            }
            const auto shifted = fmt::format("%v{}", next_tmp_id_++);
            out << fmt::format("  {} = shl i64 {}, {}\n", shifted, ext, i);
            const auto merged = fmt::format("%v{}", next_tmp_id_++);
            out << fmt::format("  {} = or i64 {}, {}\n", merged, combined, shifted);
            combined = merged;
        }
        return combined;
    }
    void EmitUnaryGate(const ir::Function* func, const ir::UnaryInst& u, std::ostringstream& out) {
        using Op = ir::Opcode::Table;
        const auto q = QubitRef(func, u.GetQubit().id);
        switch (u.GetOpcode().GetCode()) {
            case Op::I:
                LOG_DEBUG("qret::qir: skip identity gate while exporting to QIR");
                return;
            case Op::X:
                EmitCall(out, kX, {q});
                return;
            case Op::Y:
                EmitCall(out, kY, {q});
                return;
            case Op::Z:
                EmitCall(out, kZ, {q});
                return;
            case Op::H:
                EmitCall(out, kH, {q});
                return;
            case Op::S:
                EmitCall(out, kS, {q});
                return;
            case Op::SDag:
                EmitCall(out, kSAdj, {q});
                return;
            case Op::T:
                EmitCall(out, kT, {q});
                return;
            case Op::TDag:
                EmitCall(out, kTAdj, {q});
                return;
            default:
                throw QRETError(
                        error::UnsupportedQIRInstruction,
                        fmt::format(
                                "opcode '{}' cannot be exported to QIR",
                                u.GetOpcode().ToString()
                        )
                );
        }
    }
    void EmitRotation(
            const ir::Function* func,
            const ir::ParametrizedRotationInst& r,
            std::ostringstream& out
    ) {
        using Op = ir::Opcode::Table;
        const auto q = QubitRef(func, r.GetQubit().id);
        const auto theta = fmt::format("double {}", FormatDouble(r.GetTheta().value));
        switch (r.GetOpcode().GetCode()) {
            case Op::RX:
                EmitCall(out, kRX, {theta, q});
                return;
            case Op::RY:
                EmitCall(out, kRY, {theta, q});
                return;
            case Op::RZ:
                EmitCall(out, kRZ, {theta, q});
                return;
            default:
                throw QRETError(
                        error::UnsupportedQIRInstruction,
                        fmt::format(
                                "opcode '{}' cannot be exported to QIR",
                                r.GetOpcode().ToString()
                        )
                );
        }
    }
    void
    EmitBinaryGate(const ir::Function* func, const ir::BinaryInst& b, std::ostringstream& out) {
        using Op = ir::Opcode::Table;
        // Quration BinaryInst stores (target=q0, control=q1); QIR expects (control, target).
        const auto target = QubitRef(func, b.GetQubit0().id);
        const auto control = QubitRef(func, b.GetQubit1().id);
        switch (b.GetOpcode().GetCode()) {
            case Op::CX:
                EmitCall(out, kCNot, {control, target});
                return;
            case Op::CY:
                EmitCall(out, kCY, {control, target});
                return;
            case Op::CZ:
                EmitCall(out, kCZ, {control, target});
                return;
            default:
                throw QRETError(
                        error::UnsupportedQIRInstruction,
                        fmt::format(
                                "opcode '{}' cannot be exported to QIR",
                                b.GetOpcode().ToString()
                        )
                );
        }
    }
    void
    EmitTernaryGate(const ir::Function* func, const ir::TernaryInst& t, std::ostringstream& out) {
        using Op = ir::Opcode::Table;
        // Quration stores (target=q0, ctrl0=q1, ctrl1=q2); QIR wants (ctrl0, ctrl1, target).
        const auto target = QubitRef(func, t.GetQubit0().id);
        const auto c0 = QubitRef(func, t.GetQubit1().id);
        const auto c1 = QubitRef(func, t.GetQubit2().id);
        switch (t.GetOpcode().GetCode()) {
            case Op::CCX:
                EmitCall(out, kCCX, {c0, c1, target});
                return;
            // No dedicated QIS call for CCY/CCZ: CCY = S.CCX.SDag, CCZ = H.CCX.H.
            case Op::CCY:
                EmitCall(out, kSAdj, {target});
                EmitCall(out, kCCX, {c0, c1, target});
                EmitCall(out, kS, {target});
                return;
            case Op::CCZ:
                EmitCall(out, kH, {target});
                EmitCall(out, kCCX, {c0, c1, target});
                EmitCall(out, kH, {target});
                return;
            default:
                throw QRETError(
                        error::UnsupportedQIRInstruction,
                        fmt::format(
                                "opcode '{}' cannot be exported to QIR",
                                t.GetOpcode().ToString()
                        )
                );
        }
    }
    // QIR has no instruction for a global phase, so it's dropped; EmitExportWarnings warns once.
    void EmitGlobalPhase(const ir::GlobalPhaseInst&, std::ostringstream&) {
        warnings_.global_phase_dropped = true;
    }
    void EmitMeasurement(
            const ir::Function* func,
            const ir::MeasurementInst& m,
            std::ostringstream& out
    ) {
        const auto qubit_ref = QubitRef(func, m.GetQubit().id);
        const auto result_ref = RegisterRefWrite(func, m.GetRegister().id);
        used_functions_.emplace(kMz);
        used_mz_ = true;
        out << fmt::format("  call void @{}({}, {})\n", kMz, qubit_ref, result_ref);
    }
    void EmitCallInst(const ir::Function* func, const ir::CallInst& c, std::ostringstream& out) {
        auto args = std::vector<std::string>();
        args.reserve(c.GetOperate().size() + c.GetCallee()->GetNumRegisters());
        for (const auto& qubit : c.GetOperate()) {
            args.emplace_back(QubitRef(func, qubit.id));
        }
        const auto* callee = c.GetCallee();
        if (callee->GetNumRegisters() > 0) {
            const auto& is_output = GetRegisterIsOutput(callee);
            // is_output is recovered from just one representative call to callee, so a call whose
            // own Input/Output split disagrees would otherwise drive input_it/output_it past their
            // end() below (undefined behavior).
            const auto num_output = static_cast<std::size_t>(std::ranges::count(is_output, true));
            if (c.GetOutput().size() != num_output
                || c.GetInput().size() != is_output.size() - num_output) {
                throw QRETError(
                        error::UnsupportedQIRInstruction,
                        fmt::format(
                                "call to '{}' has an Input/Output split inconsistent with other "
                                "calls to the same sub-circuit; QIR export requires a uniform "
                                "split",
                                callee->GetName()
                        )
                );
            }
            auto input_it = c.GetInput().begin();
            auto output_it = c.GetOutput().begin();
            for (auto i = std::size_t{0}; i < callee->GetNumRegisters(); ++i) {
                if (is_output[i]) {
                    args.emplace_back(RegisterRefWrite(func, output_it->id));
                    ++output_it;
                } else {
                    args.emplace_back(RegisterRefRead(func, input_it->id));
                    ++input_it;
                }
            }
        }
        out << fmt::format(
                "  call void @{}({})\n",
                UniqueFunctionName(c.GetCallee()),
                fmt::to_string(fmt::join(args, ", "))
        );
    }
    void EmitBranch(const ir::Function* func, const ir::BranchInst& br, std::ostringstream& out) {
        if (br.IsUnconditional()) {
            out << fmt::format("  br label %{}\n", UniqueBlockLabel(br.GetTrueBB()));
            return;
        }
        const auto cond_ssa = ResolveConditionSSA(func, br.GetCondition().id, out);
        out << fmt::format(
                "  br i1 {}, label %{}, label %{}\n",
                cond_ssa,
                UniqueBlockLabel(br.GetTrueBB()),
                UniqueBlockLabel(br.GetFalseBB())
        );
    }
    // GetCaseBB() is a std::map, so cases are emitted key-ascending (deterministic).
    void EmitSwitch(const ir::Function* func, const ir::SwitchInst& sw, std::ostringstream& out) {
        used_switch_ = true;
        used_int_computations_ = true;
        const auto value = EmitCombinedI64Value(func, sw.GetValue(), out);
        out << fmt::format(
                "  switch i64 {}, label %{} [\n",
                value,
                UniqueBlockLabel(sw.GetDefaultBB())
        );
        for (const auto& [case_value, case_bb] : sw.GetCaseBB()) {
            out << fmt::format(
                    "    i64 {}, label %{}\n",
                    case_value,
                    UniqueBlockLabel(case_bb)
            );
        }
        out << "  ]\n";
    }
    // bool_record_output if the value came from classical computation, else result_record_output.
    void EmitBoolRegisterOutput(
            const ir::Function* func,
            const std::string& label,
            std::uint64_t reg_id,
            std::ostringstream& out
    ) {
        if (callcf_output_ssa_.contains({func, reg_id})) {
            used_functions_.emplace(kRtBoolRecordOutput);
            const auto ssa = ResolveConditionSSA(func, reg_id, out);
            out << fmt::format("  call void @{}(i1 {}, {})\n", kRtBoolRecordOutput, ssa, label);
        } else {
            used_functions_.emplace(kRtResultRecordOutput);
            out << fmt::format(
                    "  call void @{}({}, {})\n",
                    kRtResultRecordOutput,
                    PtrArg(LookupResultId(func, reg_id)),
                    label
            );
        }
    }
    // Emits one record_output per named Output register in declared name/order/width, or nothing
    // when there are none.
    void EmitNamedRecordOutputs(const ir::Function* func, std::ostringstream& out) {
        const auto& named_registers = func->GetArgument().registers;
        if (named_registers.empty()) {
            return;
        }
        used_functions_.emplace(kRtTupleRecordOutput);

        const auto tuple_tag = MakeGlobalLabel(fmt::format("t{}", named_registers.size()));
        out << fmt::format(
                "  call void @{}(i64 {}, {})\n",
                kRtTupleRecordOutput,
                named_registers.size(),
                tuple_tag
        );
        auto reg_id = std::uint64_t{0};
        for (const auto& [name, size] : named_registers) {
            if (size == 1) {
                EmitBoolRegisterOutput(func, MakeGlobalLabel(name), reg_id, out);
                ++reg_id;
                continue;
            }
            // Quration-IR draws no packed-int vs bit-array distinction, so a width > 1 register is
            // emitted uniformly as a tuple of its individual bits.
            used_functions_.emplace(kRtTupleRecordOutput);
            out << fmt::format(
                    "  call void @{}(i64 {}, {})\n",
                    kRtTupleRecordOutput,
                    size,
                    MakeGlobalLabel(name)
            );
            for (auto i = std::uint64_t{0}; i < size; ++i) {
                EmitBoolRegisterOutput(
                        func,
                        MakeGlobalLabel(fmt::format("{}[{}]", name, i)),
                        reg_id + i,
                        out
                );
            }
            reg_id += size;
        }
    }
    void EmitTerminator(
            const ir::Function* func,
            const ir::Instruction& inst,
            bool is_entry,
            std::ostringstream& out
    ) {
        using Op = ir::Opcode::Table;
        switch (inst.GetOpcode().GetCode()) {
            case Op::Return:
                if (is_entry) {
                    EmitNamedRecordOutputs(func, out);
                    out << "  ret i64 0\n";
                } else {
                    out << "  ret void\n";
                }
                return;
            case Op::Branch:
                EmitBranch(func, *Cast<ir::BranchInst>(&inst), out);
                return;
            case Op::Switch:
                EmitSwitch(func, *Cast<ir::SwitchInst>(&inst), out);
                return;
            default:
                throw QRETError(
                        error::UnsupportedQIRInstruction,
                        fmt::format(
                                "opcode '{}' cannot be exported to QIR",
                                inst.GetOpcode().ToString()
                        )
                );
        }
    }

    void EmitInstruction(
            const ir::Function* func,
            const ir::Instruction& inst,
            bool is_entry,
            std::ostringstream& out
    ) {
        if (inst.IsOptHint()) {
            LOG_DEBUG("qret::qir: skip optimization-hint instruction while exporting to QIR");
            return;
        }
        if (inst.IsMeasurement()) {
            EmitMeasurement(func, *Cast<ir::MeasurementInst>(&inst), out);
            return;
        }
        if (inst.IsParametrizedRotation()) {
            EmitRotation(func, *Cast<ir::ParametrizedRotationInst>(&inst), out);
            return;
        }
        if (inst.IsUnary()) {
            EmitUnaryGate(func, *Cast<ir::UnaryInst>(&inst), out);
            return;
        }
        if (inst.IsBinary()) {
            EmitBinaryGate(func, *Cast<ir::BinaryInst>(&inst), out);
            return;
        }
        if (inst.IsTernary()) {
            EmitTernaryGate(func, *Cast<ir::TernaryInst>(&inst), out);
            return;
        }
        if (inst.IsMultiControl()) {
            throw QRETError(
                    error::UnsupportedQIRInstruction,
                    "MCX (multi-controlled gate) cannot be exported to QIR"
            );
        }
        if (inst.IsStateInvariant()) {
            EmitGlobalPhase(*Cast<ir::GlobalPhaseInst>(&inst), out);
            return;
        }
        if (inst.IsCall()) {
            EmitCallInst(func, *Cast<ir::CallInst>(&inst), out);
            return;
        }
        if (inst.IsClassical()) {
            EmitCallCF(func, *Cast<ir::CallCFInst>(&inst), out);
            return;
        }
        if (inst.IsRandom()) {
            throw QRETError(
                    error::UnsupportedQIRInstruction,
                    "classical random number generation (DiscreteDistribution) cannot be exported "
                    "to QIR"
            );
        }
        if (inst.IsTerminator()) {
            EmitTerminator(func, inst, is_entry, out);
            return;
        }
        throw QRETError(
                error::UnsupportedQIRInstruction,
                fmt::format("opcode '{}' cannot be exported to QIR", inst.GetOpcode().ToString())
        );
    }

    void EmitFunction(const ir::Function* func, std::ostringstream& out) {
        const auto is_entry = func == entry_;

        block_labels_.clear();
        used_block_labels_.clear();

        const auto name = UniqueFunctionName(func);
        if (is_entry) {
            out << fmt::format("define i64 @{}() #0 {{\n", name);
        } else {
            auto params = std::vector<std::string>();
            for (auto i = std::size_t{0}; i < func->GetNumQubits(); ++i) {
                params.emplace_back(fmt::format("ptr %q{}", i));
            }
            // No `writeonly`: an Output register may still be read back within its own body (e.g.
            // measure then branch on the same register), so it isn't reliably write-only there.
            for (auto i = std::size_t{0}; i < func->GetNumRegisters(); ++i) {
                params.emplace_back(fmt::format("ptr %r{}", i));
            }
            out << fmt::format(
                    "define void @{}({}) {{\n",
                    name,
                    fmt::to_string(fmt::join(params, ", "))
            );
        }

        // Moves the Return block to the end for text emission: control_flow::If reserves it up
        // front, so it would otherwise be emitted before the arms that branch into it.
        auto ordered_bbs = std::vector<const ir::BasicBlock*>();
        for (const auto& bb : *func) {
            ordered_bbs.emplace_back(&bb);
        }
        std::stable_partition(ordered_bbs.begin(), ordered_bbs.end(), [](const ir::BasicBlock* bb) {
            const auto* terminator = bb->GetTerminator();
            return terminator == nullptr
                    || terminator->GetOpcode().GetCode() != ir::Opcode::Table::Return;
        });

        // Pre-assign in emission order so the first block keeps the un-suffixed name.
        for (const auto* bb : ordered_bbs) {
            UniqueBlockLabel(bb);
        }

        for (const auto* bb : ordered_bbs) {
            out << UniqueBlockLabel(bb) << ":\n";
            if (is_entry && bb == func->GetEntryBB()) {
                EmitCall(out, kRtInitialize, {"ptr null"});
            }
            for (const auto& inst : *bb) {
                EmitInstruction(func, inst, is_entry, out);
            }
        }

        out << "}\n";
    }

    void EmitGlobals(std::ostringstream& out) const {
        for (const auto& line : global_labels_) {
            out << line << "\n";
        }
    }
    void EmitDeclares(std::ostringstream& out) const {
        for (const auto& name : used_functions_) {
            out << DeclareLineFor(name) << "\n";
        }
    }
    void EmitAttributeGroups(std::ostringstream& out) const {
        out << fmt::format(
                "attributes #0 = {{ \"{}\" \"{}\"=\"{}\" \"{}\"=\"schema_id\" \"{}\"=\"{}\" "
                "\"{}\"=\"{}\" }}\n",
                kAttrEntryPoint,
                kAttrQirProfiles,
                kProfileAdaptive,
                kAttrOutputLabelingSchema,
                kAttrRequiredNumQubits,
                entry_->GetNumQubits(),
                kAttrRequiredNumResults,
                next_result_id_
        );
        if (used_mz_) {
            out << fmt::format("attributes #1 = {{ \"{}\" }}\n", kAttrIrreversible);
        }
    }
    void EmitModuleFlags(std::ostringstream& out) const {
        auto flags = std::vector<std::string>();
        flags.emplace_back(
                fmt::format("!{{i32 1, !\"{}\", i32 {}}}", kFlagQirMajorVersion, kQirMajorVersion)
        );
        flags.emplace_back(
                fmt::format("!{{i32 7, !\"{}\", i32 {}}}", kFlagQirMinorVersion, kQirMinorVersion)
        );
        flags.emplace_back(
                fmt::format("!{{i32 1, !\"{}\", i1 false}}", kFlagDynamicQubitManagement)
        );
        flags.emplace_back(
                fmt::format("!{{i32 1, !\"{}\", i1 false}}", kFlagDynamicResultManagement)
        );
        if (std::next(module_.begin()) != module_.end()) {
            flags.emplace_back(fmt::format("!{{i32 1, !\"{}\", i1 true}}", kFlagIrFunctions));
        }
        if (used_switch_) {
            flags.emplace_back(
                    fmt::format("!{{i32 1, !\"{}\", i1 true}}", kFlagMultipleTargetBranching)
            );
        }
        if (used_int_computations_) {
            const auto int_types_node_index = flags.size() + 1;
            flags.emplace_back(
                    fmt::format(
                            "!{{i32 5, !\"{}\", !{}}}",
                            kFlagIntComputations,
                            int_types_node_index
                    )
            );
        }

        out << "!llvm.module.flags = !{";
        for (auto i = std::size_t{0}; i < flags.size(); ++i) {
            out << (i > 0 ? ", " : "") << "!" << i;
        }
        out << "}\n";
        for (auto i = std::size_t{0}; i < flags.size(); ++i) {
            out << "!" << i << " = " << flags[i] << "\n";
        }
        if (used_int_computations_) {
            out << "!" << flags.size() << " = !{!\"i64\"}\n";
        }
    }

    void EmitExportWarnings() const {
        if (warnings_.global_phase_dropped) {
            LOG_WARN(
                    "Global phase has no equivalent QIR instruction and was dropped during export."
            );
        }
    }

    // A Register is only unique within its owning Function, so both result_ids_ and
    // callcf_output_ssa_ key on the (Function, raw register id) pair.
    using FuncRegKey = std::pair<const ir::Function*, std::uint64_t>;

    const ir::Module& module_;
    const ir::Function* entry_ = nullptr;

    std::map<FuncRegKey, std::uint64_t> result_ids_;
    std::uint64_t next_result_id_ = 0;
    std::uint64_t next_tmp_id_ = 0;
    std::unordered_map<const ir::Function*, std::vector<bool>> register_is_output_;

    std::set<std::string> used_functions_;
    std::vector<std::string> global_labels_;
    // @-name uniquification (module-scoped).
    std::unordered_map<const ir::Function*, std::string> function_names_;
    std::unordered_set<std::string> used_function_names_;
    // %-label uniquification (function-scoped); cleared per EmitFunction.
    std::unordered_map<const ir::BasicBlock*, std::string> block_labels_;
    std::unordered_set<std::string> used_block_labels_;
    bool used_mz_ = false;
    bool used_int_computations_ = false;  // set by EmitCallCF/EmitSwitch
    bool used_switch_ = false;  // set by EmitSwitch
    // Maps a Register written by an already-exported CallCFInst to its resulting QIR SSA name.
    std::map<FuncRegKey, std::string> callcf_output_ssa_;

    // Groups warnings about dropped constructs so a future one is just another field here.
    struct DroppedConstructWarnings {
        bool global_phase_dropped = false;
    };
    DroppedConstructWarnings warnings_;
};
}  // namespace

std::string ExportModuleToQIR(const ir::Module& module, std::string_view entry_function_name) {
    return QirExporter(module, entry_function_name).Run();
}
}  // namespace qret::qir
