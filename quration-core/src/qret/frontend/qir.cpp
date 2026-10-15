/**
 * @file qret/frontend/qir.cpp
 * @brief Build circuit from QIR (.ll text).
 */

#include "qret/frontend/qir.h"

#include <fmt/format.h>

#include <bit>
#include <cstdint>
#include <exception>
#include <numbers>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "qret/base/log.h"
#include "qret/base/portable_function.h"
#include "qret/base/string.h"
#include "qret/error.h"
#include "qret/exception.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/control_flow.h"
#include "qret/frontend/intrinsic.h"
#include "qret/ir/value.h"
#include "qret/parser/qir.h"

namespace qret::frontend {
using namespace qret::qir;

namespace {
// Returns the value text after the "ptr" type prefix. The parser drops any parameter attributes,
// so a parsed call argument is always plain "ptr <value>".
std::string StripPtrPrefix(std::string_view arg) {
    if (arg.starts_with("ptr ")) {
        return std::string(arg.substr(4));
    }
    throw QRETError(
            error::MalformedQIR,
            fmt::format("expected a 'ptr' typed argument, got '{}'", arg)
    );
}

std::uint64_t ParsePtrLiteral(std::string_view value_text) {
    if (value_text == "null") {
        return 0;
    }
    static const auto re = std::regex(R"(^inttoptr\s*\(\s*i64\s+(\d+)\s+to\s+ptr\s*\)$)");
    auto m = std::match_results<std::string_view::const_iterator>();
    if (std::regex_match(value_text.begin(), value_text.end(), m, re)) {
        return std::stoull(m[1].str());
    }
    throw QRETError(
            error::MalformedQIR,
            fmt::format("expected a literal pointer value, got '{}'", value_text)
    );
}

double ThetaValue(const ParsedCall& call, std::size_t idx) {
    const auto& arg = call.args.at(idx);
    if (arg.rfind("double ", 0) != 0) {
        throw QRETError(
                error::MalformedQIR,
                fmt::format("expected a 'double' argument, got '{}'", arg)
        );
    }
    const auto value_text = arg.substr(7);
    // LLVM's hex float form (`0x`+16 hex digits) is an IEEE754 bit pattern, not decimal.
    if (value_text.rfind("0x", 0) == 0) {
        const auto bits =
                static_cast<std::uint64_t>(std::stoull(value_text.substr(2), nullptr, 16));
        return std::bit_cast<double>(bits);
    }
    return std::stod(value_text);
}

// A sub-circuit's qubit count is its parameter count; an entry function reads required_num_qubits
// instead, since inferring from usage would undercount an unused qubit.
std::size_t ComputeNumQubits(const ParsedFunction& pf, const ParsedModule& parsed_module) {
    if (!pf.qubit_params.empty()) {
        return pf.qubit_params.size();
    }
    if (!pf.attr_group.has_value()) {
        throw QRETError(
                error::MalformedQIR,
                fmt::format(
                        "function '{}' has no qubit parameters and no attribute group to read "
                        "'required_num_qubits' from",
                        pf.name
                )
        );
    }
    const auto it = parsed_module.attr_group_required_qubits.find(*pf.attr_group);
    if (it == parsed_module.attr_group_required_qubits.end()) {
        throw QRETError(
                error::MalformedQIR,
                fmt::format(
                        "function '{}' (attribute group #{}) is missing 'required_num_qubits'",
                        pf.name,
                        *pf.attr_group
                )
        );
    }
    return it->second;
}

// Returns the value text after the "i1" prefix: "%name", "true", or "false".
std::string StripI1Prefix(std::string_view arg) {
    if (arg.starts_with("i1 ")) {
        return std::string(arg.substr(3));
    }
    throw QRETError(
            error::MalformedQIR,
            fmt::format("expected an 'i1' typed argument, got '{}'", arg)
    );
}

// Returns the value text after the "i64" prefix: "%name" or a decimal integer literal.
std::string StripI64Prefix(std::string_view arg) {
    if (arg.starts_with("i64 ")) {
        return std::string(arg.substr(4));
    }
    throw QRETError(
            error::MalformedQIR,
            fmt::format("expected an 'i64' typed argument, got '{}'", arg)
    );
}

// Recovers a `*_record_output` call's own output label; the parser normalizes both a direct
// global-string reference and a getelementptr cast to the label to the same "ptr @N" form.
std::string ResolveLabel(const ParsedModule& parsed_module, const std::string& label_arg) {
    const auto ref = StripPtrPrefix(label_arg);
    if (!ref.empty() && ref.front() == '@') {
        const auto it = parsed_module.global_string_labels.find(ref.substr(1));
        if (it != parsed_module.global_string_labels.end()) {
            return it->second;
        }
    }
    throw QRETError(
            error::MalformedQIR,
            fmt::format(
                    "record_output label argument '{}' does not reference a known global string "
                    "constant",
                    label_arg
            )
    );
}

// One record_output call's Output slot: `label`/`num_bits` become its Argument (SetArgument);
// `measurement_raw_id`, if traced, lets Generate() redirect that measurement straight into it.
struct RecordOutputSlot {
    std::string label;
    std::size_t num_bits;
    std::optional<std::uint64_t> measurement_raw_id;
};

// Invokes f on each ParsedCall in the block, skipping interleaved classical items.
template <typename F>
void ForEachCall(const ParsedBlock& block, F&& f) {
    for (const auto& item : block.items) {
        if (const auto* call = std::get_if<ParsedCall>(&item)) {
            f(*call);
        }
    }
}

// Maps each read_result's SSA name (`%cN`) to the raw Result id it read.
std::unordered_map<std::string, std::uint64_t> BuildReadResultRawIds(const ParsedFunction& pf) {
    auto ids = std::unordered_map<std::string, std::uint64_t>();
    for (const auto& block : pf.blocks) {
        ForEachCall(block, [&ids](const ParsedCall& c) {
            if (c.function == kReadResultRt || c.function == kReadResultQis) {
                ids.emplace(c.result_var, ParsePtrLiteral(StripPtrPrefix(c.args.at(0))));
            }
        });
    }
    return ids;
}

// Traces a record_output value back to its measurement's raw Result identity, if directly derived.
std::optional<std::uint64_t> TraceRecordOutputToMeasurement(
        const ParsedCall& call,
        const std::unordered_map<std::string, std::uint64_t>& read_result_ids
) {
    if (call.function == kRtResultRecordOutput) {
        const auto value_text = StripPtrPrefix(call.args.at(0));
        if (value_text.empty() || value_text.front() == '%') {
            return std::nullopt;  // a register argument, not a raw measurement identity
        }
        return ParsePtrLiteral(value_text);
    }
    if (call.function != kRtBoolRecordOutput) {
        return std::nullopt;
    }
    const auto value_ssa = StripI1Prefix(call.args.at(0));
    const auto it = read_result_ids.find(value_ssa);
    if (it == read_result_ids.end()) {
        return std::nullopt;  // a literal `true`/`false`, or not directly a read_result value
    }
    return it->second;
}

// Prescans every record_output call in order; used to declare each output's Argument slot and,
// for bool/result kinds, to redirect its producing measurement's target register (Generate()).
std::vector<RecordOutputSlot> ComputeRecordOutputs(
        const ParsedFunction& pf,
        const ParsedModule& parsed_module,
        const std::unordered_map<std::string, std::uint64_t>& read_result_ids
) {
    auto result = std::vector<RecordOutputSlot>();
    for (const auto& block : pf.blocks) {
        ForEachCall(block, [&result, &parsed_module, &read_result_ids](const ParsedCall& call) {
            std::size_t num_bits;
            if (call.function == kRtBoolRecordOutput || call.function == kRtResultRecordOutput) {
                num_bits = 1;
            } else if (call.function == kRtIntRecordOutput) {
                num_bits = 64;
            } else {
                return;
            }
            result.push_back(
                    {ResolveLabel(parsed_module, call.args.at(1)),
                     num_bits,
                     TraceRecordOutputToMeasurement(call, read_result_ids)}
            );
        });
    }
    return result;
}

class SwapGen : public CircuitGenerator {
public:
    explicit SwapGen(CircuitBuilder* builder)
        : CircuitGenerator(builder) {}

    std::string GetName() const override {
        return std::string(kSwap);
    }
    std::string GetCacheKey() const override {
        return std::string(kSwap);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 2, Attribute::Operate);
    }
    Circuit* Generate() const override {
        if (IsCached()) {
            return GetCachedCircuit();
        }
        BeginCircuitDefinition();
        const auto qs = GetQubits(0);
        frontend::gate::CX(qs[0], qs[1]);
        frontend::gate::CX(qs[1], qs[0]);
        frontend::gate::CX(qs[0], qs[1]);
        return EndCircuitDefinition();
    }
};
class ResetGen : public CircuitGenerator {
public:
    explicit ResetGen(CircuitBuilder* builder)
        : CircuitGenerator(builder) {}

    std::string GetName() const override {
        return std::string(kReset);
    }
    std::string GetCacheKey() const override {
        return std::string(kReset);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 1, Attribute::Operate);
    }
    Circuit* Generate() const override {
        if (IsCached()) {
            return GetCachedCircuit();
        }
        BeginCircuitDefinition();
        const auto q = GetQubit(0);
        const auto reg = GetTemporalRegister();
        frontend::gate::Measure(q, reg);
        frontend::control_flow::If(reg);
        frontend::gate::X(q);
        frontend::control_flow::EndIf(reg);
        return EndCircuitDefinition();
    }
};
// Unlike ResetGen, records the measurement outcome in its Output register before resetting.
class MResetGen : public CircuitGenerator {
public:
    explicit MResetGen(CircuitBuilder* builder)
        : CircuitGenerator(builder) {}

    std::string GetName() const override {
        return std::string(kMResetZQis);
    }
    std::string GetCacheKey() const override {
        return std::string(kMResetZQis);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 1, Attribute::Operate);
        arg.Add("r", Type::Register, 1, Attribute::Output);
    }
    Circuit* Generate() const override {
        if (IsCached()) {
            return GetCachedCircuit();
        }
        BeginCircuitDefinition();
        const auto q = GetQubit(0);
        const auto reg = GetRegister(1);
        frontend::gate::Measure(q, reg);
        frontend::control_flow::If(reg);
        frontend::gate::X(q);
        frontend::control_flow::EndIf(reg);
        return EndCircuitDefinition();
    }
};

// Forward declaration
class QirImporter;
class QirFunctionGen;

// Lowers a block's classical QIR instructions (int/icmp/phi/select) into Quration-IR classical
// functions (CallCFInst), over its owner QirFunctionGen's value bindings.
class ClassicalSynthesizer {
public:
    explicit ClassicalSynthesizer(QirFunctionGen& owner)
        : owner_{owner} {}

    // Adds one classical instruction to the buffer, preserving QIR order; a redefined name
    // overwrites in place.
    template <typename T>
    void AddInst(const T& inst) const {
        if (const auto it = buf_index_.find(inst.result_var); it != buf_index_.end()) {
            buf_insts_[it->second] = inst;
        } else {
            buf_index_.emplace(inst.result_var, buf_insts_.size());
            buf_insts_.emplace_back(inst);
        }
    }
    // Emits the buffered instructions as one CallCFInst; a name is exposed as an output only if
    // used later.
    void Flush() const;
    // Copies a committed value into `target` (Quration-IR has no register copy outside CallCFInst).
    void CopyToRegister(const std::string& name, const Registers& target) const;

private:
    // A 2-edge phi's incoming values, split by branch side.
    struct MuxInputs {
        std::string true_value;
        std::string false_value;
    };
    // State threaded through BuildInst and the operand resolvers while flushing one buffer.
    struct FlushState {
        PortableFunctionBuilder& builder;
        const PortableFunctionBuilder::ArrayVariable<PortableFunctionBuilder::Bool>& input_array;
        const std::unordered_map<std::string, std::pair<std::size_t, std::size_t>>& input_slot;
        std::unordered_map<
                std::string,
                PortableFunctionBuilder::PrimitiveVariable<PortableFunctionBuilder::Bool>>
                resolved_bool;
        std::unordered_map<
                std::string,
                PortableFunctionBuilder::PrimitiveVariable<PortableFunctionBuilder::Int>>
                resolved_int;
    };
    // Names the buffer's instructions use but don't define: the synthesized function's inputs.
    struct InputSpec {
        std::vector<std::string> order;  // inputs, in first-use order
        std::unordered_map<std::string, Registers> registers;  // input name -> backing Registers
        std::unordered_map<std::string, std::size_t> local_ref_count;  // uses within the buffer
    };
    // Buffer-defined names used beyond this buffer: the synthesized function's outputs.
    struct OutputSpec {
        std::vector<std::string> names;  // exposed values, in QIR order
        std::unordered_map<std::string, std::size_t> width;
        std::size_t total_bits;
    };

    PortableFunctionBuilder::PrimitiveVariable<PortableFunctionBuilder::Int>
    PackInputAsInt(std::size_t offset, std::size_t width, FlushState& state) const;
    PortableFunctionBuilder::PrimitiveVariable<PortableFunctionBuilder::Bool>
    ResolveBoolOperand(const std::string& operand, FlushState& state) const;
    PortableFunctionBuilder::PrimitiveVariable<PortableFunctionBuilder::Int>
    ResolveIntOperand(const std::string& operand, FlushState& state) const;
    template <typename T>
    static PortableFunctionBuilder::PrimitiveVariable<PortableFunctionBuilder::Bool>
    EmitIcmpComparison(ParsedIcmpInst::Predicate pred, const T& lhs, const T& rhs);
    MuxInputs ResolveMuxInputs(const ParsedPhi& phi) const;
    // Input key for a phi's branch-condition Register, which has no QIR name (keyed on block name).
    static std::string PhiCondInputKey(const std::string& block_name);
    // `icmp ne i64 (zext i1 %x to i64), 0` is exactly `%x`; recognizing it keeps re-import
    // round-trips from rebuilding the value as new instructions each time.
    std::optional<std::string> ZextIcmpNeZeroIdentityOperand(const ParsedIcmpInst& inst) const;
    // Memoized function-wide use count per SSA name; tells Flush which names must be exposed as a
    // committed Register rather than kept as an internal temporary.
    const std::unordered_map<std::string, std::size_t>& GlobalUseCounts() const;
    // Collects the buffer's inputs (used-but-not-defined names) with their backing Registers, and
    // tallies each name's uses within the buffer.
    InputSpec CollectInputs() const;
    // Selects which buffer-defined names must become outputs (used beyond the buffer) and their
    // bit widths, in QIR order.
    OutputSpec DetermineOutputs(
            const std::unordered_map<std::string, std::size_t>& local_ref_count
    ) const;

    using BufferedInst = std::variant<ParsedIntInst, ParsedIcmpInst, ParsedPhi, ParsedSelectInst>;
    template <typename T>
    const T* FindBuffered(const std::string& name) const {
        const auto it = buf_index_.find(name);
        return it == buf_index_.end() ? nullptr : std::get_if<T>(&buf_insts_[it->second]);
    }
    // Builds one buffered instruction into a Bool or Int variable, stored in `state` under its
    // result name. Called in QIR order, so each operand it resolves is already built or an input.
    void BuildInst(const BufferedInst& inst, FlushState& state) const;

    QirFunctionGen& owner_;

    // One boundary-to-boundary span's classical instructions, cleared on flush; an ordered vector
    // preserves QIR order, buf_index_ maps a defined name to its slot.
    mutable std::vector<BufferedInst> buf_insts_;
    mutable std::unordered_map<std::string, std::size_t> buf_index_;
    mutable std::size_t next_classical_id_ = 0;  // for naming synthesized CallCFInst functions
    mutable std::optional<std::unordered_map<std::string, std::size_t>> global_use_counts_;
};

// Generates a qret circuit from one parsed QIR function.
class QirFunctionGen : public CircuitGenerator {
public:
    QirFunctionGen(CircuitBuilder* builder, const ParsedFunction& pf, QirImporter& importer)
        : CircuitGenerator(builder)
        , pf_{pf}
        , importer_{importer}
        , synth_{*this} {}

    std::string GetName() const override {
        return pf_.name;
    }
    std::string GetCacheKey() const override {
        return pf_.name;
    }
    void SetArgument(Argument& arg) const override;
    // Defined out-of-line below QirImporter: needs its complete type to call Module().
    Circuit* Generate() const override;

private:
    friend class ClassicalSynthesizer;

    using BlockMap = std::unordered_map<std::string, const ParsedBlock*>;
    // One arm's fall-through block and its immediate predecessor (differs via empty relay blocks);
    // predecessor is what a `phi` at `join` records as this arm's incoming-block name.
    struct ArmResult {
        std::string join;
        std::string predecessor;
    };
    // A join block's branch condition and True/False predecessor names, recorded by
    // ProcessConditional when it closes the enclosing If/Else.
    struct PhiContext {
        Register cond;
        std::string true_predecessor;
        std::string false_predecessor;
    };

    static const ParsedBlock* LookupBlock(const BlockMap& blocks, const std::string& name) {
        const auto it = blocks.find(name);
        if (it == blocks.end()) {
            throw QRETError(error::MalformedQIR, fmt::format("branch to unknown block '{}'", name));
        }
        return it->second;
    }
    // A revisit means a backward branch/loop, which would send ProcessArm/ProcessStraightLine into
    // an infinite loop.
    void MarkVisited(const std::string& name) const {
        if (!visited_blocks_.insert(name).second) {
            throw QRETError(
                    error::UnsupportedQIRInstruction,
                    fmt::format(
                            "block '{}' is reached more than once; backward branches/loops are not "
                            "supported by the QIR importer",
                            name
                    )
            );
        }
    }

    Qubit ResolveQubitValue(const std::string& value_text) const {
        if (!value_text.empty() && value_text.front() == '%') {
            const auto it = qubit_index_.find(value_text);
            if (it == qubit_index_.end()) {
                throw QRETError(
                        error::MalformedQIR,
                        fmt::format("reference to unknown qubit value '{}'", value_text)
                );
            }
            return GetQubits(0)[it->second];
        }
        const auto id = ParsePtrLiteral(value_text);
        const auto qubits = GetQubits(0);
        if (id >= qubits.Size()) {
            throw QRETError(error::MalformedQIR, fmt::format("qubit id {} is out of range", id));
        }
        return qubits[id];
    }
    Qubit Q(std::size_t idx, const ParsedCall& call) const {
        return ResolveQubitValue(StripPtrPrefix(call.args.at(idx)));
    }

    // An SSA name is one of this function's own register arguments; a literal refers to an earlier
    // one.
    Register ResolveRegisterValue(const std::string& value_text) const {
        if (!value_text.empty() && value_text.front() == '%') {
            const auto it = register_args_.find(value_text);
            if (it == register_args_.end()) {
                throw QRETError(
                        error::MalformedQIR,
                        fmt::format("reference to unknown register value '{}'", value_text)
                );
            }
            return it->second;
        }
        const auto raw_id = ParsePtrLiteral(value_text);
        const auto it = result_slot_.find(raw_id);
        if (it == result_slot_.end()) {
            throw QRETError(
                    error::UnsupportedQIRInstruction,
                    "reference to a result pointer that was not produced by a prior measurement; "
                    "only measurement-based values can be imported from QIR"
            );
        }
        return it->second;
    }
    // Like ResolveRegisterValue, but for an Output argument: a literal gets a fresh register on
    // first use.
    Register ResolveOrCreateRegisterTarget(const std::string& value_text) const {
        if (!value_text.empty() && value_text.front() == '%') {
            return ResolveRegisterValue(value_text);  // own register argument, always pre-bound
        }
        const auto raw_id = ParsePtrLiteral(value_text);
        // A redirected Output register (see Generate()) if uniquely recorded, else a fresh one.
        const auto redirected = recorded_measurement_targets_.find(raw_id);
        const auto reg = redirected != recorded_measurement_targets_.end() ? redirected->second
                                                                           : GetTemporalRegister();
        result_slot_.erase(raw_id);
        result_slot_.emplace(raw_id, reg);
        return reg;
    }

    // Walks call.args in the callee's own declaration order rather than assuming qubits come
    // first: a foreign multi-function QIR (e.g. from pyqir's low-level API) may interleave them.
    void EmitSubCircuitCall(const ParsedFunction& callee_pf, const ParsedCall& call) const {
        // Generate() checks the CircuitBuilder cache itself, so an already-generated callee isn't
        // regenerated.
        auto* callee_circuit = QirFunctionGen(GetBuilder(), callee_pf, importer_).Generate();

        auto q = std::vector<ir::Qubit>();
        auto in_r = std::vector<ir::Register>();
        auto out_r = std::vector<ir::Register>();
        for (auto i = std::size_t{0}; i < callee_pf.raw_params.size(); ++i) {
            const auto value_text = StripPtrPrefix(call.args.at(i));
            switch (*callee_pf.raw_params[i].kind) {
                case ParamKind::Qubit:
                    q.emplace_back(ResolveQubitValue(value_text).GetId());
                    break;
                case ParamKind::RegisterInput:
                    in_r.emplace_back(ResolveRegisterValue(value_text).GetId());
                    break;
                case ParamKind::RegisterOutput:
                    out_r.emplace_back(ResolveOrCreateRegisterTarget(value_text).GetId());
                    break;
                default:
                    throw QRETError(error::MalformedQIR, "unresolved sub-circuit parameter kind");
            }
        }
        callee_circuit->CallImpl(q, in_r, out_r);
    }
    // Quration-IR has no 2-qubit rotation, so rzz/rxx/ryy decompose on import; this is their shared
    // CNOT/RZ core, with rxx/ryy sandwiching a Clifford basis change (H gives X, S*H gives Y).
    void EmitRzz(const Qubit& q0, const Qubit& q1, double theta) const {
        frontend::gate::CX(q1, q0);
        frontend::gate::RZ(q1, theta);
        frontend::gate::CX(q1, q0);
    }
    void EmitMeasureCall(const ParsedCall& call) const {
        frontend::gate::Measure(
                Q(0, call),
                ResolveOrCreateRegisterTarget(StripPtrPrefix(call.args.at(1)))
        );
    }
    // Routes one call to its intrinsic handler, or to EmitSubCircuitCall for a user function.
    // Defined out-of-line below QirImporter: needs its complete type for
    // MarkBarrierDropped()/AllFunctions().
    void DispatchCall(const ParsedCall& call) const;

    // A name's registers: an already-committed classical value (classical_values_) or a direct
    // measurement (i1_vars_) - the only two shapes an external name can take.
    Registers ResolveExternalRegisters(const std::string& name) const {
        if (const auto it = classical_values_.find(name); it != classical_values_.end()) {
            return it->second;
        }
        if (const auto it = i1_vars_.find(name); it != i1_vars_.end()) {
            return Registers(it->second);
        }
        throw QRETError(
                error::UnsupportedQIRInstruction,
                fmt::format(
                        "value '{}' was not produced by read_result or a recognized classical "
                        "instruction chain",
                        name
                )
        );
    }
    // Processes one call item: record_output calls are resolved here directly, everything else
    // delegates to DispatchCall. Defined out-of-line below.
    void ProcessCall(const ParsedCall& call) const;
    // Walks one block's items, buffering classical instructions and flushing at each `calls`-item
    // boundary (and at the block's end), since a measurement there can rebind result_slot_.
    void ProcessBlockItems(const ParsedBlock& block) const {
        for (const auto& item : block.items) {
            std::visit(
                    [this](const auto& value) {
                        using T = std::decay_t<decltype(value)>;
                        if constexpr (std::is_same_v<T, ParsedCall>) {
                            synth_.Flush();
                            ProcessCall(value);
                        } else {
                            synth_.AddInst(value);
                        }
                    },
                    item
            );
        }
        // A buffer still open at the block's end (e.g. feeding its own branch condition) is
        // confirmed here.
        synth_.Flush();
    }
    void RegisterPhiContext(
            const std::string& join,
            Register cond,
            const std::string& true_predecessor,
            const std::string& false_predecessor
    ) const {
        phi_context_.erase(join);
        phi_context_.emplace(join, PhiContext{cond, true_predecessor, false_predecessor});
    }

    // A branch condition comes directly from read_result, or from a buffer already confirmed by
    // ProcessBlockItems, sitting in classical_values_.
    Register ResolveConditionRegister(const std::string& name) const {
        if (const auto it = i1_vars_.find(name); it != i1_vars_.end()) {
            return it->second;
        }
        if (const auto it = classical_values_.find(name); it != classical_values_.end()) {
            return it->second[0];
        }
        throw QRETError(
                error::UnsupportedQIRInstruction,
                fmt::format(
                        "value '{}' was not produced directly by read_result or by a preceding "
                        "classical computation",
                        name
                )
        );
    }
    // The join block is found from each arm's actual branch target, not QIR text order: Else()
    // creates the `else` block after `If`'s own continuation block.
    std::string ProcessConditional(const BlockMap& blocks, const ParsedBlock& block) const {
        const auto cond_reg = ResolveConditionRegister(block.cond_var);

        frontend::control_flow::If(cond_reg);
        const auto true_arm = ProcessArm(blocks, block.target_true, block.name);
        if (true_arm.join == block.target_false) {
            // No `else`: the false target is itself the join point, with `block` as its
            // (empty-arm) predecessor.
            frontend::control_flow::EndIf(cond_reg);
            RegisterPhiContext(true_arm.join, cond_reg, true_arm.predecessor, block.name);
            return true_arm.join;
        }
        frontend::control_flow::Else(cond_reg);
        const auto false_arm = ProcessArm(blocks, block.target_false, block.name);
        if (false_arm.join != true_arm.join) {
            throw QRETError(
                    error::UnsupportedQIRInstruction,
                    "conditional branch arms converge at different blocks; this control-flow "
                    "shape is not supported"
            );
        }
        frontend::control_flow::EndIf(cond_reg);
        RegisterPhiContext(true_arm.join, cond_reg, true_arm.predecessor, false_arm.predecessor);
        return true_arm.join;
    }
    // Processes each case arm plus the mandatory default, returning their join block. Unlike
    // ProcessConditional, skips RegisterPhiContext, which only fits If/Else's 2-predecessor shape.
    std::string ProcessSwitch(const BlockMap& blocks, const ParsedBlock& block) const {
        const auto value = ResolveExternalRegisters(block.switch_value);

        frontend::control_flow::Switch(value);
        auto join = std::string();
        for (const auto& [key, target] : block.switch_cases) {
            frontend::control_flow::Case(value, key);
            const auto arm = ProcessArm(blocks, target, block.name);
            if (join.empty()) {
                join = arm.join;
            } else if (arm.join != join) {
                throw QRETError(
                        error::UnsupportedQIRInstruction,
                        "switch arms converge at different blocks; this control-flow shape is not "
                        "supported"
                );
            }
        }
        frontend::control_flow::Default(value);
        // An empty default arm's label is the join block itself; skip it, or ProcessArm would
        // process the join's own body and throw on the Return that ends it.
        if (block.switch_default != join) {
            const auto default_arm = ProcessArm(blocks, block.switch_default, block.name);
            if (default_arm.join != join) {
                throw QRETError(
                        error::UnsupportedQIRInstruction,
                        "switch arms converge at different blocks; this control-flow shape is not "
                        "supported"
                );
            }
        }
        frontend::control_flow::EndSwitch(value);
        return join;
    }
    // One arm: a block chain ending in an unconditional branch, returning ArmResult. A
    // Return-terminated block is left unvisited here - only ProcessStraightLine visits it, once.
    ArmResult ProcessArm(
            const BlockMap& blocks,
            const std::string& start_name,
            const std::string& entry_predecessor
    ) const {
        auto current = start_name;
        auto predecessor = entry_predecessor;
        while (true) {
            if (LookupBlock(blocks, current)->term == ParsedBlock::Term::Return) {
                return {current, predecessor};
            }
            MarkVisited(current);
            const auto* block = LookupBlock(blocks, current);
            ProcessBlockItems(*block);
            switch (block->term) {
                case ParsedBlock::Term::BrUncond:
                    return {block->target_true, current};
                case ParsedBlock::Term::BrCond:
                    current = ProcessConditional(blocks, *block);
                    predecessor = current;
                    continue;
                case ParsedBlock::Term::Switch:
                    current = ProcessSwitch(blocks, *block);
                    predecessor = current;
                    continue;
                default:
                    throw QRETError(
                            error::MalformedQIR,
                            fmt::format("basic block '{}' has no terminator", current)
                    );
            }
        }
    }
    // The top-level chain of a function body until Return; the one place that visits a
    // Return-terminated block left unvisited by ProcessArm.
    void ProcessStraightLine(const BlockMap& blocks, const std::string& start_name) const {
        auto current = start_name;
        while (true) {
            MarkVisited(current);
            const auto* block = LookupBlock(blocks, current);
            ProcessBlockItems(*block);
            switch (block->term) {
                case ParsedBlock::Term::Return:
                    return;
                case ParsedBlock::Term::BrUncond:
                    current = block->target_true;
                    continue;
                case ParsedBlock::Term::BrCond:
                    current = ProcessConditional(blocks, *block);
                    continue;
                case ParsedBlock::Term::Switch:
                    current = ProcessSwitch(blocks, *block);
                    continue;
                default:
                    throw QRETError(
                            error::MalformedQIR,
                            fmt::format("basic block '{}' has no terminator", current)
                    );
            }
        }
    }

    // Lazily-built, memoized: a read_result's SSA name (`%cN`) -> the raw Result id it read.
    const std::unordered_map<std::string, std::uint64_t>& ReadResultRawIds() const {
        if (!read_result_raw_ids_) {
            read_result_raw_ids_ = BuildReadResultRawIds(pf_);
        }
        return *read_result_raw_ids_;
    }

    const ParsedFunction& pf_;
    QirImporter& importer_;

    mutable std::optional<std::unordered_map<std::string, std::uint64_t>> read_result_raw_ids_;
    mutable std::unordered_map<std::string, std::uint64_t> qubit_index_;  // "%qN" -> local index
    mutable std::unordered_map<std::string, Register> register_args_;  // "%rN" -> own argument
    mutable std::unordered_map<std::uint64_t, Register> result_slot_;  // raw ptr id -> register
    mutable std::unordered_map<std::string, Register> i1_vars_;  // "%cN" -> register
    // raw ptr id -> its uniquely-recorded record_output's declared Register (see Generate()).
    mutable std::unordered_map<std::uint64_t, Register> recorded_measurement_targets_;
    mutable std::unordered_set<std::string> visited_blocks_;

    // Every classical value committed to a Register so far, keyed by SSA name; persists for
    // the whole function.
    mutable std::unordered_map<std::string, Registers> classical_values_;
    mutable std::unordered_map<std::string, PhiContext> phi_context_;  // join block name -> context

    mutable ClassicalSynthesizer synth_;
};

// `entry_name`, if given, must name an existing function. Otherwise uses the entry_point-attributed
// function (QIR's own marker), falling back to a function named "main" for hand-written QIR.
const ParsedFunction* ResolveEntryFunction(
        const ParsedModule& parsed,
        const std::unordered_map<std::string, const ParsedFunction*>& all_functions,
        std::string_view entry_name
) {
    if (!entry_name.empty()) {
        const auto it = all_functions.find(std::string(entry_name));
        if (it == all_functions.end()) {
            throw QRETError(
                    error::MalformedQIR,
                    fmt::format("entry function '{}' not found in QIR", entry_name)
            );
        }
        return it->second;
    }
    const ParsedFunction* found = nullptr;
    for (const auto& pf : parsed.functions) {
        if (pf.attr_group.has_value() && parsed.attr_group_entry_point.contains(*pf.attr_group)) {
            if (found != nullptr) {
                throw QRETError(
                        error::MalformedQIR,
                        "more than one function is marked 'entry_point'"
                );
            }
            found = &pf;
        }
    }
    if (found != nullptr) {
        return found;
    }
    const auto it = all_functions.find("main");
    if (it != all_functions.end()) {
        return it->second;
    }
    throw QRETError(
            error::MalformedQIR,
            "no function is marked 'entry_point' and no fallback 'main' function was found"
    );
}

// Owns everything shared across the import of one module: the lookup table, module, and warnings.
class QirImporter {
public:
    explicit QirImporter(const ParsedModule& parsed_module)
        : module_{parsed_module} {
        for (const auto& pf : module_.functions) {
            all_functions_[pf.name] = &pf;
        }
    }

    // module_ is already known acyclic: ParseQIRText's ClassifyParameters checks the whole call
    // graph up front, since QirFunctionGen::Generate() recurses into callees on demand.
    Circuit* Run(CircuitBuilder& builder, std::string_view entry_name) {
        entry_ = ResolveEntryFunction(module_, all_functions_, entry_name);
        auto* circuit = QirFunctionGen(&builder, *entry_, *this).Generate();
        EmitImportWarnings();
        return circuit;
    }

    const ParsedModule& Module() const {
        return module_;
    }
    const std::unordered_map<std::string, const ParsedFunction*>& AllFunctions() const {
        return all_functions_;
    }
    // `*_record_output` calls are only valid in the entry point function (QIR's Output Recording);
    // used to reject one found in a sub-circuit with a clear message.
    bool IsEntry(const ParsedFunction& pf) const {
        return &pf == entry_;
    }
    // Records a dropped `barrier`; the warning is issued once, after the whole import completes.
    void MarkBarrierDropped() {
        warnings_.barrier_dropped = true;
    }

private:
    struct DroppedConstructWarnings {
        bool barrier_dropped = false;
    };

    void EmitImportWarnings() const {
        if (warnings_.barrier_dropped) {
            LOG_WARN(
                    "Barrier has no equivalent Quration-IR instruction and was dropped during "
                    "import."
            );
        }
    }

    const ParsedModule& module_;
    std::unordered_map<std::string, const ParsedFunction*> all_functions_;
    const ParsedFunction* entry_ = nullptr;
    DroppedConstructWarnings warnings_;
};

void QirFunctionGen::SetArgument(Argument& arg) const {
    arg.Add("q", Type::Qubit, ComputeNumQubits(pf_, importer_.Module()), Attribute::Operate);
    for (auto i = std::size_t{0}; i < pf_.register_params.size(); ++i) {
        arg.Add(fmt::format("r{}", i),
                Type::Register,
                1,
                pf_.register_params[i].is_output ? Attribute::Output : Attribute::Input);
    }
    if (importer_.IsEntry(pf_)) {
        for (const auto& output :
             ComputeRecordOutputs(pf_, importer_.Module(), ReadResultRawIds())) {
            arg.Add(output.label, Type::Register, output.num_bits, Attribute::Output);
        }
    }
}
Circuit* QirFunctionGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();

    for (auto i = std::size_t{0}; i < pf_.qubit_params.size(); ++i) {
        qubit_index_[pf_.qubit_params[i]] = i;
    }
    for (auto i = std::size_t{0}; i < pf_.register_params.size(); ++i) {
        register_args_.emplace(pf_.register_params[i].name, GetRegisters(1 + i)[0]);
    }

    // Redirects a uniquely-recorded measurement's target into its record_output's own Register,
    // avoiding a copy; multiply-recorded ids are left unredirected (DispatchCall throws for those).
    if (importer_.IsEntry(pf_)) {
        const auto record_outputs =
                ComputeRecordOutputs(pf_, importer_.Module(), ReadResultRawIds());
        auto raw_id_use_count = std::unordered_map<std::uint64_t, std::size_t>();
        for (const auto& output : record_outputs) {
            if (output.measurement_raw_id) {
                ++raw_id_use_count[*output.measurement_raw_id];
            }
        }
        for (const auto& output : record_outputs) {
            if (output.measurement_raw_id && raw_id_use_count[*output.measurement_raw_id] == 1) {
                recorded_measurement_targets_.emplace(
                        *output.measurement_raw_id,
                        GetRegisters(output.label)[0]
                );
            }
        }
    }

    if (pf_.blocks.empty()) {
        throw QRETError(
                error::MalformedQIR,
                fmt::format("function '{}' has no basic blocks", pf_.name)
        );
    }

    auto blocks_by_name = BlockMap();
    for (const auto& pb : pf_.blocks) {
        blocks_by_name[pb.name] = &pb;
    }
    ProcessStraightLine(blocks_by_name, pf_.blocks.front().name);

    return EndCircuitDefinition();
}
void QirFunctionGen::ProcessCall(const ParsedCall& call) const {
    const auto& fn = call.function;
    if (fn == kRtTupleRecordOutput || fn == kRtArrayRecordOutput) {
        LOG_DEBUG("qret::qir: skip runtime call '{}' while importing QIR", fn);
        return;
    }
    if (fn != kRtBoolRecordOutput && fn != kRtResultRecordOutput && fn != kRtIntRecordOutput) {
        DispatchCall(call);
        return;
    }
    if (!importer_.IsEntry(pf_)) {
        throw QRETError(
                error::MalformedQIR,
                fmt::format(
                        "'{}' is only valid within the entry point function's final block "
                        "(QIR spec's Output Recording); found in sub-circuit '{}'",
                        fn,
                        pf_.name
                )
        );
    }
    if (fn == kRtBoolRecordOutput || fn == kRtResultRecordOutput) {
        // If Generate() already redirected this measurement here (recorded_measurement_targets_),
        // nothing further to emit.
        const auto raw_id = TraceRecordOutputToMeasurement(call, ReadResultRawIds());
        if (raw_id && recorded_measurement_targets_.contains(*raw_id)) {
            return;
        }
        if (raw_id) {
            throw QRETError(
                    error::UnsupportedQIRInstruction,
                    fmt::format(
                            "'{}' is recorded by more than one record_output call; multiply-"
                            "recorded output values are not yet supported",
                            fn
                    )
            );
        }
        if (fn == kRtResultRecordOutput) {
            // A Result-typed value is only ever a raw measurement identity, so a failed trace here
            // means malformed QIR.
            throw QRETError(
                    error::MalformedQIR,
                    "result_record_output's value must be a raw measurement result"
            );
        }
        const auto label = ResolveLabel(importer_.Module(), call.args.at(1));
        synth_.CopyToRegister(StripI1Prefix(call.args.at(0)), GetRegisters(label));
        return;
    }
    // kRtIntRecordOutput is always classical-expression-derived: an i64 is never a direct
    // measurement.
    const auto label = ResolveLabel(importer_.Module(), call.args.at(1));
    synth_.CopyToRegister(StripI64Prefix(call.args.at(0)), GetRegisters(label));
}
void QirFunctionGen::DispatchCall(const ParsedCall& call) const {
    const auto& fn = call.function;
    // record_output-family calls never reach here; ProcessCall resolves them and delegates
    // the rest.
    if (fn == kRtInitialize) {
        LOG_DEBUG("qret::qir: skip runtime call '{}' while importing QIR", fn);
        return;
    }
    if (fn == kRtQubitAllocate || fn == kRtQubitRelease) {
        throw QRETError(
                error::UnsupportedQIRInstruction,
                "dynamic qubit allocation/release is not supported by the QIR importer"
        );
    }
    if (fn == kBarrierQis) {
        importer_.MarkBarrierDropped();
        return;
    }
    if (fn == kReadResultRt || fn == kReadResultQis) {
        const auto reg = ResolveRegisterValue(StripPtrPrefix(call.args.at(0)));
        // Register has no assignment operator, so re-binding a key uses erase+emplace, not
        // insert_or_assign/operator[].
        i1_vars_.erase(call.result_var);
        i1_vars_.emplace(call.result_var, reg);
        return;
    }
    if (fn == kMz || fn == kMQis) {
        EmitMeasureCall(call);
        return;
    }
    if (fn == kMResetZQis) {
        const auto reg = ResolveOrCreateRegisterTarget(StripPtrPrefix(call.args.at(1)));
        (*MResetGen(GetBuilder()).Generate())(Q(0, call), reg);
        return;
    }
    // No-argument single-qubit gates; kSAdj/kTAdj are S/T's own dagger intrinsics, not aliases.
    static const auto kUnaryGate =
            std::unordered_map<std::string_view, decltype(&frontend::gate::X)>{
                    {kI, frontend::gate::I},
                    {kX, frontend::gate::X},
                    {kY, frontend::gate::Y},
                    {kZ, frontend::gate::Z},
                    {kH, frontend::gate::H},
                    {kS, frontend::gate::S},
                    {kSAdj, frontend::gate::SDag},
                    {kT, frontend::gate::T},
                    {kTAdj, frontend::gate::TDag},
            };
    if (const auto it = kUnaryGate.find(fn); it != kUnaryGate.end()) {
        it->second(Q(0, call));
        return;
    }
    if (fn == kSXQis) {
        frontend::gate::RX(Q(0, call), std::numbers::pi / 2);
        frontend::gate::GlobalPhase(GetBuilder(), std::numbers::pi / 4);
        return;
    }

    // Rotation gates: qubit is call.args[1], angle is call.args[0].
    if (fn == kRX) {
        frontend::gate::RX(Q(1, call), ThetaValue(call, 0));
        return;
    }
    if (fn == kRY) {
        frontend::gate::RY(Q(1, call), ThetaValue(call, 0));
        return;
    }
    if (fn == kRZ) {
        frontend::gate::RZ(Q(1, call), ThetaValue(call, 0));
        return;
    }
    if (fn == kRzzQis) {
        EmitRzz(Q(1, call), Q(2, call), ThetaValue(call, 0));
        return;
    }
    if (fn == kRxxQis) {
        frontend::gate::H(Q(1, call));
        frontend::gate::H(Q(2, call));
        EmitRzz(Q(1, call), Q(2, call), ThetaValue(call, 0));
        frontend::gate::H(Q(1, call));
        frontend::gate::H(Q(2, call));
        return;
    }
    if (fn == kRyyQis) {
        frontend::gate::SDag(Q(1, call));
        frontend::gate::SDag(Q(2, call));
        frontend::gate::H(Q(1, call));
        frontend::gate::H(Q(2, call));
        EmitRzz(Q(1, call), Q(2, call), ThetaValue(call, 0));
        frontend::gate::H(Q(1, call));
        frontend::gate::H(Q(2, call));
        frontend::gate::S(Q(1, call));
        frontend::gate::S(Q(2, call));
        return;
    }

    // 2-controlled-target gates; QIR orders (control, target), Quration orders (target, control).
    // kCXQis is qsc's own name for the same cnot intrinsic as kCNot.
    static const auto kBinaryGate =
            std::unordered_map<std::string_view, decltype(&frontend::gate::CX)>{
                    {kCNot, frontend::gate::CX},
                    {kCXQis, frontend::gate::CX},
                    {kCY, frontend::gate::CY},
                    {kCZ, frontend::gate::CZ},
            };
    if (const auto it = kBinaryGate.find(fn); it != kBinaryGate.end()) {
        it->second(Q(1, call), Q(0, call));
        return;
    }
    // 2-controlled gates; QIR orders (control0, control1, target), Quration orders
    // (target, control0, control1).
    static const auto kTernaryGate =
            std::unordered_map<std::string_view, decltype(&frontend::gate::CCX)>{
                    {kCCX, frontend::gate::CCX},
                    {kCCY, frontend::gate::CCY},
                    {kCCZ, frontend::gate::CCZ},
            };
    if (const auto it = kTernaryGate.find(fn); it != kTernaryGate.end()) {
        it->second(Q(2, call), Q(0, call), Q(1, call));
        return;
    }
    if (fn == kSwap) {
        auto qs = Qubits(Q(0, call));
        qs += Q(1, call);
        (*SwapGen(GetBuilder()).Generate())(qs);
        return;
    }
    if (fn == kReset) {
        (*ResetGen(GetBuilder()).Generate())(Q(0, call));
        return;
    }

    const auto& all_functions = importer_.AllFunctions();
    const auto callee_it = all_functions.find(fn);
    if (callee_it != all_functions.end()) {
        EmitSubCircuitCall(*callee_it->second, call);
        return;
    }

    throw QRETError(
            error::UnsupportedQIRInstruction,
            fmt::format("unsupported or unknown QIR function call: '{}'", fn)
    );
}

PortableFunctionBuilder::PrimitiveVariable<PortableFunctionBuilder::Int>
ClassicalSynthesizer::PackInputAsInt(
        std::size_t offset,
        std::size_t width,
        FlushState& state
) const {
    auto result = state.builder.AddTmpIntVariable();
    result = state.input_array[offset];
    for (auto i = std::size_t{1}; i < width; ++i) {
        auto bit = state.builder.AddTmpIntVariable();
        bit = state.input_array[offset + i];
        result |= bit << static_cast<PortableFunctionBuilder::Int>(i);
    }
    return result;
}
PortableFunctionBuilder::PrimitiveVariable<PortableFunctionBuilder::Bool>
ClassicalSynthesizer::ResolveBoolOperand(const std::string& operand, FlushState& state) const {
    if (!operand.empty() && operand.front() == '%') {
        // A buffered value is already built (SSA: defined before use); otherwise it is an input.
        if (const auto it = state.resolved_bool.find(operand); it != state.resolved_bool.end()) {
            return it->second;
        }
        const auto slot = state.input_slot.find(operand);
        if (slot == state.input_slot.end()) {
            throw QRETError(
                    error::UnsupportedQIRInstruction,
                    fmt::format(
                            "value '{}' was not produced by read_result or a recognized classical "
                            "instruction chain",
                            operand
                    )
            );
        }
        const auto [offset, width] = slot->second;
        if (width != 1) {
            throw QRETError(
                    error::UnsupportedQIRInstruction,
                    fmt::format("value '{}' is not Bool-valued", operand)
            );
        }
        auto result = state.input_array[offset];
        state.resolved_bool.emplace(operand, result);
        return result;
    }
    if (operand != "true" && operand != "false") {
        throw QRETError(
                error::MalformedQIR,
                fmt::format(
                        "expected a Bool operand ('%name', 'true', or 'false'), got '{}'",
                        operand
                )
        );
    }
    // PortableFunctionBuilder has no Bool-literal constructor, so "true"/"false" goes via an
    // Int literal followed by IntToBool.
    auto as_int = state.builder.AddTmpIntVariable();
    as_int = static_cast<PortableFunctionBuilder::Int>(operand == "true" ? 1 : 0);
    auto as_bool = state.builder.AddTmpBoolVariable();
    as_bool = as_int;
    return as_bool;
}
PortableFunctionBuilder::PrimitiveVariable<PortableFunctionBuilder::Int>
ClassicalSynthesizer::ResolveIntOperand(const std::string& operand, FlushState& state) const {
    if (!operand.empty() && operand.front() == '%') {
        // A buffered value is already built (SSA: defined before use); otherwise it is an input.
        if (const auto it = state.resolved_int.find(operand); it != state.resolved_int.end()) {
            return it->second;
        }
        const auto slot = state.input_slot.find(operand);
        if (slot == state.input_slot.end()) {
            throw QRETError(
                    error::UnsupportedQIRInstruction,
                    fmt::format(
                            "value '{}' was not produced by read_result or a recognized classical "
                            "instruction chain",
                            operand
                    )
            );
        }
        const auto [offset, width] = slot->second;
        auto result = PackInputAsInt(offset, width, state);
        state.resolved_int.emplace(operand, result);
        return result;
    }
    auto literal = state.builder.AddTmpIntVariable();
    literal = static_cast<PortableFunctionBuilder::Int>(std::stoll(operand));
    return literal;
}
template <typename T>
PortableFunctionBuilder::PrimitiveVariable<PortableFunctionBuilder::Bool>
ClassicalSynthesizer::EmitIcmpComparison(
        ParsedIcmpInst::Predicate pred,
        const T& lhs,
        const T& rhs
) {
    switch (pred) {
        case ParsedIcmpInst::Predicate::Eq:
            return lhs == rhs;
        case ParsedIcmpInst::Predicate::Ne:
            return lhs != rhs;
        case ParsedIcmpInst::Predicate::Slt:
            return lhs < rhs;
        case ParsedIcmpInst::Predicate::Sle:
            return lhs <= rhs;
        case ParsedIcmpInst::Predicate::Sgt:
            return lhs > rhs;
        case ParsedIcmpInst::Predicate::Sge:
            return lhs >= rhs;
        default:
            throw QRETError(error::MalformedQIR, "unknown icmp predicate");
    }
}
ClassicalSynthesizer::MuxInputs ClassicalSynthesizer::ResolveMuxInputs(const ParsedPhi& phi) const {
    if (phi.incoming.size() != 2) {
        throw QRETError(
                error::UnsupportedQIRInstruction,
                fmt::format(
                        "phi '{}' has {} incoming edges; only exactly 2 (a simple if/else "
                        "merge) is supported",
                        phi.result_var,
                        phi.incoming.size()
                )
        );
    }
    const auto ctx_it = owner_.phi_context_.find(phi.block_name);
    if (ctx_it == owner_.phi_context_.end()) {
        throw QRETError(
                error::UnsupportedQIRInstruction,
                fmt::format(
                        "phi '{}' in block '{}' has no recognized enclosing branch condition",
                        phi.result_var,
                        phi.block_name
                )
        );
    }
    const auto& [value0, block0] = phi.incoming[0];
    const auto& [value1, block1] = phi.incoming[1];
    const auto& ctx = ctx_it->second;
    if (block0 == ctx.true_predecessor && block1 == ctx.false_predecessor) {
        return {value0, value1};
    }
    if (block1 == ctx.true_predecessor && block0 == ctx.false_predecessor) {
        return {value1, value0};
    }
    throw QRETError(
            error::UnsupportedQIRInstruction,
            fmt::format(
                    "phi '{}' incoming blocks ('{}', '{}') do not match the enclosing "
                    "conditional's arms ('{}', '{}')",
                    phi.result_var,
                    block0,
                    block1,
                    ctx.true_predecessor,
                    ctx.false_predecessor
            )
    );
}
std::string ClassicalSynthesizer::PhiCondInputKey(const std::string& block_name) {
    return fmt::format("$cond:{}", block_name);
}
std::optional<std::string> ClassicalSynthesizer::ZextIcmpNeZeroIdentityOperand(
        const ParsedIcmpInst& inst
) const {
    if (inst.is_bool_typed || inst.pred != ParsedIcmpInst::Predicate::Ne || inst.rhs != "0") {
        return std::nullopt;
    }
    const auto* zext = FindBuffered<ParsedIntInst>(inst.lhs);
    if (zext == nullptr || zext->kind != ParsedIntInst::Kind::Zext) {
        return std::nullopt;
    }
    return zext->operand0;
}
void ClassicalSynthesizer::BuildInst(const BufferedInst& buffered, FlushState& state) const {
    std::visit(
            [&state, this](const auto& inst) {
                using T = std::decay_t<decltype(inst)>;
                if constexpr (std::is_same_v<T, ParsedIcmpInst>) {
                    // An icmp result is always Bool.
                    if (const auto identity = ZextIcmpNeZeroIdentityOperand(inst)) {
                        state.resolved_bool.emplace(
                                inst.result_var,
                                ResolveBoolOperand(*identity, state)
                        );
                        return;
                    }
                    auto result = state.builder.AddTmpBoolVariable();
                    if (inst.is_bool_typed) {
                        const auto lhs = ResolveBoolOperand(inst.lhs, state);
                        const auto rhs = ResolveBoolOperand(inst.rhs, state);
                        result = EmitIcmpComparison(inst.pred, lhs, rhs);
                    } else {
                        const auto lhs = ResolveIntOperand(inst.lhs, state);
                        const auto rhs = ResolveIntOperand(inst.rhs, state);
                        result = EmitIcmpComparison(inst.pred, lhs, rhs);
                    }
                    state.resolved_bool.emplace(inst.result_var, result);
                } else if constexpr (std::is_same_v<T, ParsedIntInst>) {
                    if (inst.kind == ParsedIntInst::Kind::Zext) {
                        auto result = state.builder.AddTmpIntVariable();
                        result = ResolveBoolOperand(inst.operand0, state);
                        state.resolved_int.emplace(inst.result_var, result);
                    } else if (inst.is_bool_typed) {
                        const auto lhs = ResolveBoolOperand(inst.operand0, state);
                        const auto rhs = ResolveBoolOperand(inst.operand1, state);
                        auto result = state.builder.AddTmpBoolVariable();
                        switch (inst.kind) {
                            case ParsedIntInst::Kind::And:
                                result = lhs & rhs;
                                break;
                            case ParsedIntInst::Kind::Or:
                                result = lhs | rhs;
                                break;
                            case ParsedIntInst::Kind::Xor:
                                // `xor i1 %x, true` is the logical-not idiom; no dedicated case.
                                result = lhs ^ rhs;
                                break;
                            default:
                                throw QRETError(
                                        error::UnsupportedQIRInstruction,
                                        fmt::format(
                                                "value '{}' is not a Bool-valued classical "
                                                "instruction",
                                                inst.result_var
                                        )
                                );
                        }
                        state.resolved_bool.emplace(inst.result_var, result);
                    } else {
                        const auto lhs = ResolveIntOperand(inst.operand0, state);
                        const auto rhs = ResolveIntOperand(inst.operand1, state);
                        auto result = state.builder.AddTmpIntVariable();
                        switch (inst.kind) {
                            case ParsedIntInst::Kind::Shl:
                                result = lhs << rhs;
                                break;
                            case ParsedIntInst::Kind::Ashr:
                                result = lhs >> rhs;
                                break;
                            case ParsedIntInst::Kind::Add:
                                result = lhs + rhs;
                                break;
                            case ParsedIntInst::Kind::Sub:
                                result = lhs - rhs;
                                break;
                            case ParsedIntInst::Kind::Mul:
                                result = lhs * rhs;
                                break;
                            case ParsedIntInst::Kind::Sdiv:
                                result = lhs / rhs;
                                break;
                            case ParsedIntInst::Kind::Srem:
                                result = lhs % rhs;
                                break;
                            case ParsedIntInst::Kind::And:
                                result = lhs & rhs;
                                break;
                            case ParsedIntInst::Kind::Or:
                                result = lhs | rhs;
                                break;
                            case ParsedIntInst::Kind::Xor:
                                result = lhs ^ rhs;
                                break;
                            default:
                                throw QRETError(
                                        error::MalformedQIR,
                                        "unreachable classical instruction kind in BuildInst"
                                );
                        }
                        state.resolved_int.emplace(inst.result_var, result);
                    }
                } else if constexpr (std::is_same_v<T, ParsedPhi>) {
                    const auto mux = ResolveMuxInputs(inst);
                    const auto cond_offset =
                            state.input_slot.at(PhiCondInputKey(inst.block_name)).first;
                    auto cond = state.builder.AddTmpBoolVariable();
                    cond = state.input_array[cond_offset];
                    if (inst.is_bool_typed) {
                        const auto true_value = ResolveBoolOperand(mux.true_value, state);
                        const auto false_value = ResolveBoolOperand(mux.false_value, state);
                        // Mux() returns a lazy reference through a pooled variable recycled at
                        // expression end; copy it into a dedicated temporary so it survives.
                        auto result = state.builder.AddTmpBoolVariable();
                        result = state.builder.Mux(cond, true_value, false_value);
                        state.resolved_bool.emplace(inst.result_var, result);
                    } else {
                        const auto true_value = ResolveIntOperand(mux.true_value, state);
                        const auto false_value = ResolveIntOperand(mux.false_value, state);
                        auto result = state.builder.AddTmpIntVariable();
                        result = state.builder.Mux(cond, true_value, false_value);
                        state.resolved_int.emplace(inst.result_var, result);
                    }
                } else if constexpr (std::is_same_v<T, ParsedSelectInst>) {
                    const auto cond = ResolveBoolOperand(inst.cond, state);
                    if (inst.is_bool_typed) {
                        const auto true_value = ResolveBoolOperand(inst.true_value, state);
                        const auto false_value = ResolveBoolOperand(inst.false_value, state);
                        auto result = state.builder.AddTmpBoolVariable();
                        result = state.builder.Mux(cond, true_value, false_value);
                        state.resolved_bool.emplace(inst.result_var, result);
                    } else {
                        const auto true_value = ResolveIntOperand(inst.true_value, state);
                        const auto false_value = ResolveIntOperand(inst.false_value, state);
                        auto result = state.builder.AddTmpIntVariable();
                        result = state.builder.Mux(cond, true_value, false_value);
                        state.resolved_int.emplace(inst.result_var, result);
                    }
                }
            },
            buffered
    );
}
const std::unordered_map<std::string, std::size_t>& ClassicalSynthesizer::GlobalUseCounts() const {
    if (!global_use_counts_) {
        auto counts = std::unordered_map<std::string, std::size_t>();
        const auto add_use = [&counts](const std::string& operand) {
            if (!operand.empty() && operand.front() == '%') {
                ++counts[operand];
            }
        };
        for (const auto& block : owner_.pf_.blocks) {
            for (const auto& item : block.items) {
                std::visit(
                        [&add_use](const auto& value) {
                            using T = std::decay_t<decltype(value)>;
                            if constexpr (std::is_same_v<T, ParsedIntInst>) {
                                add_use(value.operand0);
                                add_use(value.operand1);
                            } else if constexpr (std::is_same_v<T, ParsedIcmpInst>) {
                                add_use(value.lhs);
                                add_use(value.rhs);
                            } else if constexpr (std::is_same_v<T, ParsedPhi>) {
                                for (const auto& [incoming_value, predecessor] : value.incoming) {
                                    add_use(incoming_value);
                                }
                            } else if constexpr (std::is_same_v<T, ParsedSelectInst>) {
                                add_use(value.cond);
                                add_use(value.true_value);
                                add_use(value.false_value);
                            } else if constexpr (std::is_same_v<T, ParsedCall>) {
                                if (value.function == kRtBoolRecordOutput) {
                                    add_use(StripI1Prefix(value.args.at(0)));
                                } else if (value.function == kRtIntRecordOutput) {
                                    add_use(StripI64Prefix(value.args.at(0)));
                                }
                            }
                        },
                        item
                );
            }
            if (block.term == ParsedBlock::Term::BrCond) {
                add_use(block.cond_var);
            } else if (block.term == ParsedBlock::Term::Switch) {
                add_use(block.switch_value);
            }
        }
        global_use_counts_ = std::move(counts);
    }
    return *global_use_counts_;
}
ClassicalSynthesizer::InputSpec ClassicalSynthesizer::CollectInputs() const {
    const auto is_local = [this](const std::string& name) { return buf_index_.contains(name); };
    auto plan = InputSpec();
    auto add_input = [&plan](const std::string& key, const Registers& regs) {
        if (plan.registers.contains(key)) {
            return;
        }
        plan.order.emplace_back(key);
        plan.registers.emplace(key, regs);
    };
    // Tally each operand's use within the buffer, so DetermineOutputs can spot uses beyond it.
    auto add_operand_input = [&plan, &is_local, &add_input, this](const std::string& operand) {
        if (operand.empty() || operand.front() != '%') {
            return;
        }
        ++plan.local_ref_count[operand];
        if (is_local(operand)) {
            return;
        }
        add_input(operand, owner_.ResolveExternalRegisters(operand));
    };
    for (const auto& buffered : buf_insts_) {
        std::visit(
                [&add_operand_input, &add_input, this](const auto& inst) {
                    using T = std::decay_t<decltype(inst)>;
                    if constexpr (std::is_same_v<T, ParsedIntInst>) {
                        add_operand_input(inst.operand0);
                        add_operand_input(inst.operand1);
                    } else if constexpr (std::is_same_v<T, ParsedIcmpInst>) {
                        add_operand_input(inst.lhs);
                        add_operand_input(inst.rhs);
                    } else if constexpr (std::is_same_v<T, ParsedPhi>) {
                        for (const auto& [value, predecessor] : inst.incoming) {
                            add_operand_input(value);
                        }
                        const auto ctx_it = owner_.phi_context_.find(inst.block_name);
                        if (ctx_it == owner_.phi_context_.end()) {
                            throw QRETError(
                                    error::UnsupportedQIRInstruction,
                                    fmt::format(
                                            "phi '{}' in block '{}' has no recognized enclosing "
                                            "branch condition",
                                            inst.result_var,
                                            inst.block_name
                                    )
                            );
                        }
                        add_input(PhiCondInputKey(inst.block_name), Registers(ctx_it->second.cond));
                    } else if constexpr (std::is_same_v<T, ParsedSelectInst>) {
                        // Unlike a phi's condition, a select's is a plain Bool operand: just
                        // another input-or-local reference, like true/false.
                        add_operand_input(inst.cond);
                        add_operand_input(inst.true_value);
                        add_operand_input(inst.false_value);
                    }
                },
                buffered
        );
    }
    return plan;
}
ClassicalSynthesizer::OutputSpec ClassicalSynthesizer::DetermineOutputs(
        const std::unordered_map<std::string, std::size_t>& local_ref_count
) const {
    auto plan = OutputSpec{{}, {}, 0};
    auto add_output = [&plan](const std::string& name, std::size_t width) {
        plan.names.emplace_back(name);
        plan.width.emplace(name, width);
        plan.total_bits += width;
    };
    // Exposed as a committed Register only if referenced outside this buffer; else it stays an
    // internal temporary.
    const auto needs_exposure = [&local_ref_count, this](const std::string& name) {
        const auto& global_counts = GlobalUseCounts();
        const auto global_it = global_counts.find(name);
        const auto global_count =
                global_it == global_counts.end() ? std::size_t{0} : global_it->second;
        const auto local_it = local_ref_count.find(name);
        const auto local_count =
                local_it == local_ref_count.end() ? std::size_t{0} : local_it->second;
        return global_count > local_count;
    };
    for (const auto& buffered : buf_insts_) {
        std::visit(
                [&needs_exposure, &add_output](const auto& inst) {
                    using T = std::decay_t<decltype(inst)>;
                    if (!needs_exposure(inst.result_var)) {
                        return;
                    }
                    // An icmp result is always 1-bit Bool; the others carry is_bool_typed.
                    if constexpr (std::is_same_v<T, ParsedIcmpInst>) {
                        add_output(inst.result_var, 1);
                    } else {
                        add_output(
                                inst.result_var,
                                inst.is_bool_typed ? std::size_t{1} : std::size_t{64}
                        );
                    }
                },
                buffered
        );
    }
    return plan;
}
void ClassicalSynthesizer::Flush() const {
    if (buf_insts_.empty()) {
        return;
    }
    const auto input_spec = CollectInputs();

    // Lay out the inputs as one contiguous bit-array: input_slot maps each name to its
    // (offset, width), input_bits are the real Registers wired to the call.
    auto input_slot = std::unordered_map<std::string, std::pair<std::size_t, std::size_t>>();
    auto total_input_bits = std::size_t{0};
    // Registers has no accessible default constructor, so accumulation starts from the first
    // input's own Registers, not an empty placeholder (see the no-input fallback below).
    auto input_bits = std::optional<Registers>();
    for (const auto& key : input_spec.order) {
        const auto& regs = input_spec.registers.at(key);
        const auto width = static_cast<std::size_t>(regs.Size());
        input_slot.emplace(key, std::make_pair(total_input_bits, width));
        total_input_bits += width;
        if (!input_bits) {
            input_bits.emplace(regs);
        } else {
            *input_bits += regs;
        }
    }

    auto builder = PortableFunctionBuilder();
    const auto input_array = builder.AddBoolArrayInputVariable("input", total_input_bits);

    const auto output_spec = DetermineOutputs(input_spec.local_ref_count);
    auto output_array = builder.AddBoolArrayOutputVariable("output", output_spec.total_bits);

    auto state = FlushState{builder, input_array, input_slot, {}, {}};
    // Build every buffered instruction in QIR order; SSA guarantees each operand is already built
    // (or is an input). Values used by no output stay as unused temporaries, mirroring the source.
    for (const auto& buffered : buf_insts_) {
        BuildInst(buffered, state);
    }

    // Wire each exposed value into the output array.
    auto output_offsets = std::unordered_map<std::string, std::size_t>();
    auto output_offset = std::size_t{0};
    for (const auto& name : output_spec.names) {
        const auto width = output_spec.width.at(name);
        output_offsets.emplace(name, output_offset);
        if (width == 1) {
            output_array[output_offset] = state.resolved_bool.at(name);
        } else {
            const auto value = state.resolved_int.at(name);
            for (auto i = std::size_t{0}; i < width; ++i) {
                output_array[output_offset + i] =
                        (value >> static_cast<PortableFunctionBuilder::Int>(i)) & 1;
            }
        }
        output_offset += width;
    }

    // Compile the classical function and emit the call wiring its input/output Registers.
    const auto function = PortableAnnotatedFunction(
            builder.Compile(fmt::format("imported_classical_{}", next_classical_id_++))
    );
    auto output_targets = owner_.GetTemporalRegisters(output_spec.total_bits);
    // No inputs (e.g. a buffer computing purely from literals): Range(0, 0) is a
    // validly-empty Registers sharing its builder.
    frontend::gate::CallCF(
            function,
            input_bits ? *input_bits : output_targets.Range(0, 0),
            output_targets
    );

    // Commit each output to a named Register for later references, then clear the buffer.
    for (const auto& name : output_spec.names) {
        const auto width = output_spec.width.at(name);
        const auto offset = output_offsets.at(name);
        owner_.classical_values_.erase(name);
        owner_.classical_values_.emplace(name, output_targets.Range(offset, width));
    }

    buf_insts_.clear();
    buf_index_.clear();
}
void ClassicalSynthesizer::CopyToRegister(const std::string& name, const Registers& target) const {
    const auto source = owner_.ResolveExternalRegisters(name);
    const auto width = static_cast<std::size_t>(source.Size());
    auto builder = PortableFunctionBuilder();
    const auto in = builder.AddBoolArrayInputVariable("input", width);
    auto out = builder.AddBoolArrayOutputVariable("output", width);
    for (auto i = std::size_t{0}; i < width; ++i) {
        out[i] = in[i];
    }
    const auto function = PortableAnnotatedFunction(
            builder.Compile(fmt::format("imported_classical_{}", next_classical_id_++))
    );
    frontend::gate::CallCF(function, source, target);
}

Circuit* BuildModuleFromParsed(
        const ParsedModule& parsed,
        CircuitBuilder& builder,
        std::string_view entry_name
) {
    try {
        return QirImporter(parsed).Run(builder, entry_name);
    } catch (const QRETError&) {
        throw;
    } catch (const std::exception& ex) {
        throw QRETError(error::MalformedQIR, fmt::format("failed to import QIR: {}", ex.what()));
    }
}
}  // namespace

Circuit* BuildCircuitFromQIR(
        std::string_view qir_text,
        CircuitBuilder& builder,
        std::string_view entry_name
) {
    return BuildModuleFromParsed(qret::qir::ParseQIRText(qir_text), builder, entry_name);
}
Circuit* BuildCircuitFromQIRFile(
        std::string_view path,
        CircuitBuilder& builder,
        std::string_view entry_name
) {
    return BuildCircuitFromQIR(LoadFile(std::string(path)), builder, entry_name);
}
}  // namespace qret::frontend
