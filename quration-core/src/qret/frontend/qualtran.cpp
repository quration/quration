/**
 * @file qret/frontend/qualtran.cpp
 * @brief Qualtran BloqLibrary → Quration IR conversion.
 */

#include "qret/frontend/qualtran.h"

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <numbers>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_set>
#include <vector>

#include "qret/base/log.h"
#include "qret/error.h"
#include "qret/exception.h"
#include "qret/frontend/argument.h"
#include "qret/frontend/attribute.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/control_flow.h"
#include "qret/frontend/functor.h"
#include "qret/frontend/intrinsic.h"
#include "qret/ir/value.h"
#include "qret/parser/qualtran.h"

#include "qualtran/protos/bloq.pb.h"
#include "qualtran/protos/registers.pb.h"

namespace qpb = ::qualtran;

namespace qret::frontend {

namespace {

// (instance_id, register_name, flat_index) -> Qubit
using SoquetKey = std::tuple<std::int32_t, std::string, std::int32_t>;
using WireMap = std::map<SoquetKey, Qubit>;
// bloq_id -> BloqWithDecomposition*
using BloqTable = std::map<std::int32_t, const qpb::BloqLibrary_BloqWithDecomposition*>;

// ─────────────────────────────────────────────────────────────
// Qualtran bloq names.
// ─────────────────────────────────────────────────────────────

// Gates
constexpr std::string_view kHadamardBloqName = "qualtran.bloqs.basic_gates.hadamard.Hadamard";
constexpr std::string_view kXGateBloqName = "qualtran.bloqs.basic_gates.x_basis.XGate";
constexpr std::string_view kYGateBloqName = "qualtran.bloqs.basic_gates.y_gate.YGate";
constexpr std::string_view kZGateBloqName = "qualtran.bloqs.basic_gates.z_basis.ZGate";
constexpr std::string_view kIdentityBloqName = "qualtran.bloqs.basic_gates.identity.Identity";
constexpr std::string_view kTGateBloqName = "qualtran.bloqs.basic_gates.t_gate.TGate";
constexpr std::string_view kSGateBloqName = "qualtran.bloqs.basic_gates.s_gate.SGate";
constexpr std::string_view kCNOTBloqName = "qualtran.bloqs.basic_gates.cnot.CNOT";
constexpr std::string_view kCZBloqName = "qualtran.bloqs.basic_gates.z_basis.CZ";
constexpr std::string_view kCYGateBloqName = "qualtran.bloqs.basic_gates.y_gate.CYGate";
constexpr std::string_view kToffoliBloqName = "qualtran.bloqs.basic_gates.toffoli.Toffoli";
constexpr std::string_view kRxBloqName = "qualtran.bloqs.basic_gates.rotation.Rx";
constexpr std::string_view kRyBloqName = "qualtran.bloqs.basic_gates.rotation.Ry";
constexpr std::string_view kRzBloqName = "qualtran.bloqs.basic_gates.rotation.Rz";
constexpr std::string_view kGlobalPhaseBloqName =
        "qualtran.bloqs.basic_gates.global_phase.GlobalPhase";
constexpr std::string_view kCHadamardBloqName = "qualtran.bloqs.basic_gates.hadamard.CHadamard";
constexpr std::string_view kTwoBitSwapBloqName = "qualtran.bloqs.basic_gates.swap.TwoBitSwap";
constexpr std::string_view kTwoBitCSwapBloqName = "qualtran.bloqs.basic_gates.swap.TwoBitCSwap";
constexpr std::string_view kSwapBloqName = "qualtran.bloqs.basic_gates.swap.Swap";
constexpr std::string_view kCSwapBloqName = "qualtran.bloqs.basic_gates.swap.CSwap";
constexpr std::string_view kZPowGateBloqName = "qualtran.bloqs.basic_gates.rotation.ZPowGate";
constexpr std::string_view kXPowGateBloqName = "qualtran.bloqs.basic_gates.rotation.XPowGate";
constexpr std::string_view kYPowGateBloqName = "qualtran.bloqs.basic_gates.rotation.YPowGate";
constexpr std::string_view kSU2RotationGateBloqName =
        "qualtran.bloqs.basic_gates.su2_rotation.SU2RotationGate";
constexpr std::string_view kCRzBloqName = "qualtran.bloqs.basic_gates.rotation.CRz";
constexpr std::string_view kCZPowGateBloqName = "qualtran.bloqs.basic_gates.rotation.CZPowGate";
constexpr std::string_view kAndBloqName = "qualtran.bloqs.mcmt.and_bloq.And";

// Bookkeeping (wire-renaming, no gate emitted)
constexpr std::string_view kSplitBloqName = "qualtran.bloqs.bookkeeping.split.Split";
constexpr std::string_view kJoinBloqName = "qualtran.bloqs.bookkeeping.join.Join";
constexpr std::string_view kCastBloqName = "qualtran.bloqs.bookkeeping.cast.Cast";
constexpr std::string_view kSplit2BloqName = "qualtran.bloqs.bookkeeping.partition.Split2";
constexpr std::string_view kJoin2BloqName = "qualtran.bloqs.bookkeeping.partition.Join2";
constexpr std::string_view kPartitionBloqName = "qualtran.bloqs.bookkeeping.partition.Partition";

// State/Effect (qubit producers/consumers)
constexpr std::string_view kZeroStateBloqName = "qualtran.bloqs.basic_gates.z_basis.ZeroState";
constexpr std::string_view kOneStateBloqName = "qualtran.bloqs.basic_gates.z_basis.OneState";
constexpr std::string_view kPlusStateBloqName = "qualtran.bloqs.basic_gates.x_basis.PlusState";
constexpr std::string_view kMinusStateBloqName = "qualtran.bloqs.basic_gates.x_basis.MinusState";
constexpr std::string_view kAllocateBloqName = "qualtran.bloqs.bookkeeping.allocate.Allocate";
constexpr std::string_view kZeroEffectBloqName = "qualtran.bloqs.basic_gates.z_basis.ZeroEffect";
constexpr std::string_view kOneEffectBloqName = "qualtran.bloqs.basic_gates.z_basis.OneEffect";
constexpr std::string_view kPlusEffectBloqName = "qualtran.bloqs.basic_gates.x_basis.PlusEffect";
constexpr std::string_view kMinusEffectBloqName = "qualtran.bloqs.basic_gates.x_basis.MinusEffect";
constexpr std::string_view kFreeBloqName = "qualtran.bloqs.bookkeeping.free.Free";

// Functor wrappers (Adjoint/Controlled)
constexpr std::string_view kAdjointBloqName = "qualtran._infra.adjoint.Adjoint";
constexpr std::string_view kControlledBloqName = "qualtran._infra.controlled.Controlled";

// ─────────────────────────────────────────────────────────────
// Proto access helpers.
// ─────────────────────────────────────────────────────────────

std::int32_t FlatIndex(const qpb::Soquet& soquet) {
    if (soquet.index_size() > 0) {
        return soquet.index(0);
    }
    return 0;
}

std::size_t IntOrSympyToSize(const qpb::Register& reg, const qpb::IntOrSympy& val) {
    if (!val.has_int_val()) {
        throw QRETError(
                error::UnsupportedQualtranBloq,
                fmt::format(
                        "register '{}' has a symbolic shape/bitsize, which is unsupported",
                        reg.name()
                )
        );
    }
    return static_cast<std::size_t>(val.int_val());
}

// reg.dtype() is one QCDType shared by every element, not a per-element list.
std::size_t ComputeDtypeBitsize(const qpb::Register& reg) {
    if (!reg.has_dtype()) {
        return std::size_t{1};
    }
    const auto& dt = reg.dtype();
    switch (dt.val_case()) {
        case qpb::QDataType::kQbit:
            return std::size_t{1};  // QBit has no bitsize field of its own: fixed at 1.
        case qpb::QDataType::kQany:
            return IntOrSympyToSize(reg, dt.qany().bitsize());
        case qpb::QDataType::kQuint:
            return IntOrSympyToSize(reg, dt.quint().bitsize());
        case qpb::QDataType::kQint:
            return IntOrSympyToSize(reg, dt.qint().bitsize());
        case qpb::QDataType::kQintOnesComp:
            return IntOrSympyToSize(reg, dt.qint_ones_comp().bitsize());
        case qpb::QDataType::kBquint:
            return IntOrSympyToSize(reg, dt.bquint().bitsize());
        case qpb::QDataType::kQfxp:
            return IntOrSympyToSize(reg, dt.qfxp().bitsize());
        case qpb::QDataType::kQmontgomeryUint:
            return IntOrSympyToSize(reg, dt.qmontgomery_uint().bitsize());
        default:
            throw QRETError(
                    error::UnsupportedQualtranBloq,
                    fmt::format("register '{}' has an unrecognized QDataType", reg.name())
            );
    }
}

// Registers are never zero-sized; an empty shape means a single element of the dtype's bitsize.
std::size_t ComputeRegisterSize(const qpb::Register& reg) {
    auto size = ComputeDtypeBitsize(reg);
    for (const auto& dim : reg.shape()) {
        size *= IntOrSympyToSize(reg, dim);
    }
    return size;
}

// Calls fn(left_flat_index, right_flat_index), expanding a bundle edge into one call per index.
template <typename Fn>
void ForEachFlatIndex(const qpb::Soquet& left, const qpb::Soquet& right, Fn&& fn) {
    const auto reg_size = ComputeRegisterSize(left.register_());
    const auto r_idx = FlatIndex(right);
    if (reg_size > 1 && left.index_size() == 0) {
        for (auto k = std::int32_t{0}; k < static_cast<std::int32_t>(reg_size); ++k) {
            fn(k, r_idx + k);
        }
        return;
    }
    fn(FlatIndex(left), r_idx);
}

const qpb::BloqArg* FindArg(const qpb::Bloq& bloq, std::string_view name) {
    for (const auto& bloq_arg : bloq.args()) {
        if (bloq_arg.name() == name) {
            return &bloq_arg;
        }
    }
    return nullptr;
}

// Qualtran always serializes a set field, so a missing arg is an error, not a default.
double ArgToDouble(const qpb::Bloq& bloq, std::string_view arg_name) {
    const auto* arg = FindArg(bloq, arg_name);
    if (arg == nullptr) {
        throw QRETError(
                error::MalformedQualtranProto,
                fmt::format("missing arg '{}' on bloq '{}'", arg_name, bloq.name())
        );
    }
    if (arg->has_float_val()) {
        return arg->float_val();
    }
    if (arg->has_int_val()) {
        return static_cast<double>(arg->int_val());
    }
    throw QRETError(
            error::MalformedQualtranProto,
            fmt::format("arg '{}' on bloq '{}' is not a numeric value", arg_name, bloq.name())
    );
}

std::int32_t ArgToInt(const qpb::Bloq& bloq, std::string_view arg_name) {
    const auto* arg = FindArg(bloq, arg_name);
    if (arg == nullptr) {
        throw QRETError(
                error::MalformedQualtranProto,
                fmt::format("missing arg '{}' on bloq '{}'", arg_name, bloq.name())
        );
    }
    if (!arg->has_int_val()) {
        throw QRETError(
                error::UnsupportedQualtranBloq,
                fmt::format(
                        "arg '{}' on bloq '{}' is symbolic, which is unsupported",
                        arg_name,
                        bloq.name()
                )
        );
    }
    return static_cast<std::int32_t>(arg->int_val());
}

bool IsAliasOnlyBloq(const qpb::Bloq& bloq) {
    const auto& name = bloq.name();
    return name == kSplitBloqName || name == kJoinBloqName || name == kCastBloqName
            || name == kSplit2BloqName || name == kJoin2BloqName || name == kPartitionBloqName;
}

bool IsNewQubitProducerBloq(std::string_view name) {
    return name == kZeroStateBloqName || name == kOneStateBloqName || name == kPlusStateBloqName
            || name == kMinusStateBloqName || name == kAllocateBloqName;
}

bool IsQubitConsumerBloq(std::string_view name) {
    return name == kZeroEffectBloqName || name == kOneEffectBloqName || name == kPlusEffectBloqName
            || name == kMinusEffectBloqName || name == kFreeBloqName;
}

// A State/Effect acting against its raw role (odd unwrap depth) is read via its counterpart name.
std::string_view OppositeStateEffectBloqName(std::string_view name) {
    static constexpr std::array<std::pair<std::string_view, std::string_view>, 5> kPairs = {
            {{kZeroStateBloqName, kZeroEffectBloqName},
             {kOneStateBloqName, kOneEffectBloqName},
             {kPlusStateBloqName, kPlusEffectBloqName},
             {kMinusStateBloqName, kMinusEffectBloqName},
             {kAllocateBloqName, kFreeBloqName}}
    };
    for (const auto& [state_name, effect_name] : kPairs) {
        if (name == state_name) {
            return effect_name;
        }
        if (name == effect_name) {
            return state_name;
        }
    }
    throw std::runtime_error("internal error: unhandled State/Effect bloq name");
}

// True for And(uncompute=0), the direction that allocates a fresh ancilla target.
bool IsAndCompute(const qpb::Bloq& bloq) {
    if (bloq.name() != kAndBloqName) {
        return false;
    }
    const auto* arg = FindArg(bloq, "uncompute");
    return (arg == nullptr || (arg->has_int_val() && arg->int_val() == 0));  // default: False
}

bool IsDirtyAllocate(const qpb::Bloq& bloq) {
    if (bloq.name() != kAllocateBloqName) {
        return false;
    }
    const auto* arg = FindArg(bloq, "dirty");
    return arg != nullptr && arg->has_int_val() && arg->int_val() != 0;
}

bool IsDirtyFree(const qpb::Bloq& bloq) {
    if (bloq.name() != kFreeBloqName) {
        return false;
    }
    const auto* arg = FindArg(bloq, "dirty");
    return arg != nullptr && arg->has_int_val() && arg->int_val() != 0;
}

// True when bloq carries its own "is_adjoint" arg set (TGate/SGate's own dagger flag).
// Unrelated to the Adjoint(subbloq) wrapper bloq.
bool HasAdjointFlag(const qpb::Bloq& bloq) {
    const auto* arg = FindArg(bloq, "is_adjoint");
    return arg != nullptr && arg->has_int_val() && arg->int_val() != 0;
}

std::int32_t GetSubbloqId(const qpb::Bloq& bloq) {
    const auto* arg = FindArg(bloq, "subbloq");
    if (arg == nullptr || !arg->has_subbloq()) {
        throw QRETError(
                error::MalformedQualtranProto,
                fmt::format("bloq '{}' missing 'subbloq' arg", bloq.name())
        );
    }
    return arg->subbloq();
}

const qpb::Bloq& LookupBloqInTable(const BloqTable& bloq_table, std::int32_t bloq_id) {
    const auto it = bloq_table.find(bloq_id);
    if (it == bloq_table.end()) {
        throw QRETError(
                error::MalformedQualtranProto,
                fmt::format("bloq_id {} not found in table", bloq_id)
        );
    }
    return it->second->bloq();
}

Qubit LookupWire(
        const WireMap& wire_map,
        std::int32_t inst_id,
        std::string_view reg,
        std::int32_t idx = 0
) {
    const auto it = wire_map.find({inst_id, std::string(reg), idx});
    if (it == wire_map.end()) {
        throw QRETError(
                error::MalformedQualtranProto,
                fmt::format("wire not found for register '{}' on instance {}", reg, inst_id)
        );
    }
    return it->second;
}

// ─────────────────────────────────────────────────────────────
// Adjoint(...)/Controlled(...) wrapper unwrapping.
// ─────────────────────────────────────────────────────────────

// Adjoint(...)/Controlled(...) wrappers all share the same bloq.name(), so fold in subbloq's name.
std::string ResolveCircuitName(const BloqTable& bloq_table, const qpb::Bloq& bloq) {
    if (bloq.name() == kAdjointBloqName) {
        return fmt::format(
                "__adjoint__{}",
                ResolveCircuitName(bloq_table, LookupBloqInTable(bloq_table, GetSubbloqId(bloq)))
        );
    }
    if (bloq.name() == kControlledBloqName) {
        return fmt::format(
                "__controlled__{}",
                ResolveCircuitName(bloq_table, LookupBloqInTable(bloq_table, GetSubbloqId(bloq)))
        );
    }
    return bloq.name();
}

// Innermost non-Adjoint bloq reached through nested Adjoint(...) wrappers, plus unwrap depth.
struct UnwrappedAdjoint {
    std::int32_t bloq_id;
    const qpb::Bloq* bloq;  // pointer, not reference, so this can be a std::map value in BloqDag
    std::size_t depth;

    bool IsOddDepth() const {
        return depth % 2 == 1;
    }
};

UnwrappedAdjoint UnwrapAdjoint(const BloqTable& bloq_table, std::int32_t bloq_id) {
    const auto& bloq = LookupBloqInTable(bloq_table, bloq_id);
    if (bloq.name() != kAdjointBloqName) {
        return {.bloq_id = bloq_id, .bloq = &bloq, .depth = 0};
    }
    const auto inner = UnwrapAdjoint(bloq_table, GetSubbloqId(bloq));
    return {.bloq_id = inner.bloq_id, .bloq = inner.bloq, .depth = inner.depth + 1};
}

// An odd unwrap depth flips And's raw compute/uncompute direction.
bool UnwrappedAndActsAsCompute(const UnwrappedAdjoint& unwrapped) {
    return IsAndCompute(*unwrapped.bloq) != unwrapped.IsOddDepth();
}

// An odd unwrap depth flips State/Effect's raw producer/consumer role.
bool UnwrappedStateEffectActsAsProducer(const UnwrappedAdjoint& unwrapped) {
    const auto is_state_origin = IsNewQubitProducerBloq(unwrapped.bloq->name());
    return is_state_origin ? !unwrapped.IsOddDepth() : unwrapped.IsOddDepth();
}

// The name a State/Effect bloq presents as once its odd-depth role flip is resolved (matches
// EmitAdjointDispatch's own choice); unaffected otherwise, since And is handled separately and
// every other kind is unaffected by Adjoint's LEFT/RIGHT swap.
std::string_view UnwrappedStateEffectName(const UnwrappedAdjoint& unwrapped) {
    const auto& name = unwrapped.bloq->name();
    if (!unwrapped.IsOddDepth() || (!IsNewQubitProducerBloq(name) && !IsQubitConsumerBloq(name))) {
        return name;
    }
    return OppositeStateEffectBloqName(name);
}

// unwrapped.bloq's own "dirty" arg (Allocate's or Free's, whichever it raw-is), unaffected by
// which effective role (producer/consumer) the odd-depth flip currently has it playing.
bool UnwrappedStateEffectIsDirty(const UnwrappedAdjoint& unwrapped) {
    return IsDirtyAllocate(*unwrapped.bloq) || IsDirtyFree(*unwrapped.bloq);
}

// Whether producer's fresh qubit may be declared CleanAncilla given consumer, resolving both
// through any Adjoint(...) wrapping first (a plain bloq unwraps to itself at depth 0).
bool IsCleanTerminalConsumer(const UnwrappedAdjoint& producer, const UnwrappedAdjoint& consumer) {
    if (producer.bloq->name() == kAndBloqName && consumer.bloq->name() == kAndBloqName
        && UnwrappedAndActsAsCompute(producer) && !UnwrappedAndActsAsCompute(consumer)) {
        return true;
    }
    const auto p = UnwrappedStateEffectName(producer);
    const auto c = UnwrappedStateEffectName(consumer);
    const auto producer_is_true_zero = p == kZeroStateBloqName
            || (p == kAllocateBloqName && !UnwrappedStateEffectIsDirty(producer));
    const auto consumer_has_verify_hint = c == kZeroEffectBloqName
            || (c == kFreeBloqName && !UnwrappedStateEffectIsDirty(consumer));
    return producer_is_true_zero && consumer_has_verify_hint;
}

// True unless unwrapped is AliasOnly/And/State/Effect; Gate/Composite terminals behave like a
// plain forward call for fresh-RIGHT-register bookkeeping.
bool UnwrapsToGateOrComposite(const UnwrappedAdjoint& unwrapped) {
    return !IsAliasOnlyBloq(*unwrapped.bloq) && unwrapped.bloq->name() != kAndBloqName
            && !IsNewQubitProducerBloq(unwrapped.bloq->name())
            && !IsQubitConsumerBloq(unwrapped.bloq->name());
}

struct ControlRegSpec {
    std::string reg_name;
    std::vector<std::int64_t> cvs;
};
struct ControlSpecInfo {
    std::int32_t subbloq_id;
    std::vector<ControlRegSpec> ctrl_regs;
};

// Controlled.signature flattens ctrl_regs + subbloq.signature, so nested layers can be read off
// directly without per-layer instances.
struct UnwrappedControlled {
    std::int32_t bloq_id;
    const qpb::Bloq* bloq;
    std::vector<ControlRegSpec> ctrl_regs;
    // True after an odd number of Adjoint(...) layers: Controlled(Adjoint(X)) ==
    // Adjoint(Controlled(X)).
    bool is_adjoint = false;
};

// Assumes the .npy version 1.0 / descr='<i8' layout Qualtran's CtrlSpec.cvs always serializes to.
std::vector<std::int64_t>
ParseNpyInt64Array(std::string_view npy_bytes, std::size_t expected_count) {
    constexpr std::size_t kHeaderLenOffset = 8;  // magic(6) + version(2)
    constexpr std::size_t kHeaderTextOffset = kHeaderLenOffset + 2;
    const auto header_len =
            static_cast<std::size_t>(static_cast<std::uint8_t>(npy_bytes[kHeaderLenOffset]))
            | (static_cast<std::size_t>(static_cast<std::uint8_t>(npy_bytes[kHeaderLenOffset + 1]))
               << 8);
    const auto data_offset = kHeaderTextOffset + header_len;
    auto values = std::vector<std::int64_t>(expected_count);
    for (auto i = std::size_t{0}; i < expected_count; ++i) {
        auto raw = std::uint64_t{0};
        for (auto b = std::size_t{0}; b < sizeof(std::int64_t); ++b) {
            raw |= static_cast<std::uint64_t>(static_cast<std::uint8_t>(
                           npy_bytes[data_offset + i * sizeof(std::int64_t) + b]
                   )
                   ) << (8 * b);
        }
        values[i] = static_cast<std::int64_t>(raw);
    }
    return values;
}

ControlSpecInfo ExtractControlSpecInfo(const qpb::Bloq& bloq, const qpb::Bloq& subbloq) {
    const auto& ctrl_spec = FindArg(bloq, "ctrl_spec")->ctrl_spec();
    for (const auto& qdtype : ctrl_spec.qdtypes()) {
        if (!qdtype.has_qbit()) {
            throw QRETError(
                    error::UnsupportedQualtranBloq,
                    "Controlled with a non-QBit control register dtype (e.g. integer-equality "
                    "control) is unsupported"
            );
        }
    }

    // ctrl registers are named dynamically ("ctrl"/"ctrl1"/...), so find them by set difference.
    auto subbloq_reg_names = std::unordered_set<std::string>{};
    for (const auto& reg : subbloq.registers().registers()) {
        subbloq_reg_names.insert(reg.name());
    }
    auto new_regs = std::vector<const qpb::Register*>{};
    for (const auto& reg : bloq.registers().registers()) {
        if (!subbloq_reg_names.contains(reg.name())) {
            new_regs.emplace_back(&reg);
        }
    }

    auto ctrl_regs = std::vector<ControlRegSpec>{};
    for (auto i = std::size_t{0}; i < new_regs.size(); ++i) {
        const auto cvs = ParseNpyInt64Array(
                ctrl_spec.cvs(static_cast<int>(i)).ndarray(),
                ComputeRegisterSize(*new_regs[i])
        );
        for (const auto cv : cvs) {
            if (cv != 0 && cv != 1) {
                throw QRETError(
                        error::UnsupportedQualtranBloq,
                        "Controlled with a control value outside {0,1} is unsupported"
                );
            }
        }
        ctrl_regs.emplace_back(new_regs[i]->name(), cvs);
    }
    return {.subbloq_id = GetSubbloqId(bloq), .ctrl_regs = ctrl_regs};
}

// Unwraps nested Controlled(...), outermost ctrl_regs first, plus a trailing Adjoint(...) layer
// (Controlled(Adjoint(subbloq)) is serialized as one, never as another ctrl layer).
UnwrappedControlled UnwrapControlled(const BloqTable& bloq_table, std::int32_t bloq_id) {
    const auto& bloq = LookupBloqInTable(bloq_table, bloq_id);
    const auto subbloq_id = GetSubbloqId(bloq);
    const auto& subbloq = LookupBloqInTable(bloq_table, subbloq_id);
    const auto info = ExtractControlSpecInfo(bloq, subbloq);
    if (subbloq.name() == kAdjointBloqName) {
        const auto unwrapped_adjoint = UnwrapAdjoint(bloq_table, subbloq_id);
        return {.bloq_id = unwrapped_adjoint.bloq_id,
                .bloq = unwrapped_adjoint.bloq,
                .ctrl_regs = info.ctrl_regs,
                .is_adjoint = unwrapped_adjoint.IsOddDepth()};
    }
    if (subbloq.name() != kControlledBloqName) {
        return {.bloq_id = subbloq_id, .bloq = &subbloq, .ctrl_regs = info.ctrl_regs};
    }
    auto inner = UnwrapControlled(bloq_table, subbloq_id);
    inner.ctrl_regs.insert(inner.ctrl_regs.begin(), info.ctrl_regs.begin(), info.ctrl_regs.end());
    return inner;
}

// ─────────────────────────────────────────────────────────────
// BloqKind classification: what role a bloq_id plays, independent of where it is used.
// ─────────────────────────────────────────────────────────────

enum class BloqKind : std::uint8_t {
    Gate,
    AliasOnly,
    NewQubitProducer,
    QubitConsumer,
    AdjointWrapper,
    ControlledWrapper,
    Composite,
    Unsupported,
};

BloqKind ClassifyBloqKind(const qpb::BloqLibrary_BloqWithDecomposition& entry) {
    // Even an Adjoint/Controlled wrapper with a decomposition is a Composite: Qualtran expanded it.
    if (entry.decomposition_size() > 0) {
        return BloqKind::Composite;
    }
    const auto& bloq = entry.bloq();
    const auto& name = bloq.name();
    if (IsAliasOnlyBloq(bloq)) {
        return BloqKind::AliasOnly;
    }
    // Unlike And(uncompute), And(compute)'s "target" is a fresh RIGHT-only register.
    if (IsAndCompute(bloq)) {
        return BloqKind::NewQubitProducer;
    }
    if (IsNewQubitProducerBloq(name)) {
        return BloqKind::NewQubitProducer;
    }
    if (IsQubitConsumerBloq(name)) {
        return BloqKind::QubitConsumer;
    }
    if (name == kAdjointBloqName) {
        return BloqKind::AdjointWrapper;
    }
    if (name == kControlledBloqName) {
        return BloqKind::ControlledWrapper;
    }
    static const auto kGateNames = std::unordered_set<std::string_view>{
            kHadamardBloqName,  kXGateBloqName,      kYGateBloqName,
            kZGateBloqName,     kIdentityBloqName,   kTGateBloqName,
            kSGateBloqName,     kCNOTBloqName,       kCZBloqName,
            kCYGateBloqName,    kToffoliBloqName,    kRxBloqName,
            kRyBloqName,        kRzBloqName,         kGlobalPhaseBloqName,
            kCHadamardBloqName, kTwoBitSwapBloqName, kTwoBitCSwapBloqName,
            kSwapBloqName,      kCSwapBloqName,      kZPowGateBloqName,
            kXPowGateBloqName,  kYPowGateBloqName,   kSU2RotationGateBloqName,
            kCRzBloqName,       kCZPowGateBloqName,
    };
    if (name == kAndBloqName || kGateNames.contains(name)) {
        return BloqKind::Gate;
    }
    return BloqKind::Unsupported;
}

// True iff the unwrapped terminal gets a standalone Circuit (Composite or ControlledWrapper).
// A Gate terminal is emitted inline instead, so it has none.
bool UnwrapsToCircuitBuiltRole(const BloqTable& bloq_table, const UnwrappedAdjoint& unwrapped) {
    if (!UnwrapsToGateOrComposite(unwrapped)) {
        return false;
    }
    const auto terminal_kind = ClassifyBloqKind(*bloq_table.at(unwrapped.bloq_id));
    return terminal_kind == BloqKind::Composite || terminal_kind == BloqKind::ControlledWrapper;
}

// ─────────────────────────────────────────────────────────────
// Outer DAG: bloq_id -> bloq_id references, used to find the single root and reject bad input.
// ─────────────────────────────────────────────────────────────

struct BloqDag {
    std::int32_t root_bloq_id = 0;
    // Non-owning: entries point into the BloqLibrary passed to BuildBloqDag, which must outlive
    // this BloqDag.
    BloqTable table;
    // Unwrap results, computed once per bloq_id instead of at every consumer call site.
    std::map<std::int32_t, UnwrappedAdjoint> adjoint_unwraps;
    std::map<std::int32_t, UnwrappedControlled> controlled_unwraps;
};

struct DependencyBloqIds {
    std::vector<std::int32_t> structural;  // decomposition/subbloq: must be built or inlined
    std::vector<std::int32_t> referenced_only;  // root-detection only: bloq_counts() keys, etc.
};

DependencyBloqIds
CollectDependencyBloqIds(const qpb::BloqLibrary_BloqWithDecomposition& entry, BloqKind kind) {
    auto out = DependencyBloqIds{};
    if (kind == BloqKind::AdjointWrapper || kind == BloqKind::ControlledWrapper) {
        out.structural.emplace_back(GetSubbloqId(entry.bloq()));
        return out;
    }
    if (kind != BloqKind::Composite) {
        return out;
    }
    for (const auto& conn : entry.decomposition()) {
        for (const auto* soquet : {&conn.left(), &conn.right()}) {
            if (soquet->has_bloq_instance()) {
                out.structural.emplace_back(soquet->bloq_instance().bloq_id());
            }
        }
    }
    for (const auto& [bloq_id, _] : entry.bloq_counts()) {
        out.referenced_only.emplace_back(bloq_id);
    }
    const auto& bloq = entry.bloq();
    if (bloq.name() == kAdjointBloqName || bloq.name() == kControlledBloqName) {
        // ResolveCircuitName still reads this by name for the "__adjoint__"/"__controlled__"
        // prefix.
        out.referenced_only.emplace_back(GetSubbloqId(bloq));
    }
    return out;
}

BloqDag BuildBloqDag(const qpb::BloqLibrary& lib) {
    auto dag = BloqDag{};
    for (const auto& entry : lib.table()) {
        dag.table[entry.bloq_id()] = &entry;
    }

    auto referenced = std::set<std::int32_t>{};
    auto structural_dependencies = std::set<std::int32_t>{};
    for (const auto& [bloq_id, entry] : dag.table) {
        const auto kind = ClassifyBloqKind(*entry);
        if (kind == BloqKind::AdjointWrapper) {
            dag.adjoint_unwraps.emplace(bloq_id, UnwrapAdjoint(dag.table, bloq_id));
        } else if (kind == BloqKind::ControlledWrapper) {
            dag.controlled_unwraps.emplace(bloq_id, UnwrapControlled(dag.table, bloq_id));
        }
        const auto dependencies = CollectDependencyBloqIds(*entry, kind);
        for (const auto dependency_id : dependencies.structural) {
            referenced.insert(dependency_id);
            structural_dependencies.insert(dependency_id);
        }
        for (const auto dependency_id : dependencies.referenced_only) {
            referenced.insert(dependency_id);
        }
    }

    auto roots = std::vector<std::int32_t>{};
    for (const auto& [bloq_id, _] : dag.table) {
        if (!referenced.contains(bloq_id)) {
            roots.emplace_back(bloq_id);
        }
    }
    if (roots.empty()) {
        // A finite DAG always has an unreferenced node; none found means a structural cycle.
        throw QRETError(error::MalformedQualtranProto, "circular reference among bloq_id");
    }
    if (roots.size() > 1) {
        throw QRETError(
                error::MalformedQualtranProto,
                fmt::format(
                        "multiple root candidates found ({} bloqs); multi-root libraries are not "
                        "yet supported",
                        roots.size()
                )
        );
    }
    dag.root_bloq_id = roots.front();
    structural_dependencies.insert(dag.root_bloq_id);

    for (const auto bloq_id : structural_dependencies) {
        if (ClassifyBloqKind(*dag.table.at(bloq_id)) == BloqKind::Unsupported) {
            throw QRETError(
                    error::UnsupportedQualtranBloq,
                    fmt::format(
                            "unsupported Qualtran bloq: '{}'. "
                            "If this bloq is a sub-circuit, ensure max_depth in bloqs_to_proto() "
                            "is large enough to include its full decomposition.",
                            LookupBloqInTable(dag.table, bloq_id).name()
                    )
            );
        }
    }
    return dag;
}

// ─────────────────────────────────────────────────────────────
// Ancilla management
// ─────────────────────────────────────────────────────────────

enum class AncillaKind : std::uint8_t { Clean, Operate, Dirty };

// Slices ancilla qubits out of the "anc_clean"/"anc_operate"/"anc_dirty" pool arguments.
class AncillaCursor {
public:
    // Builds pools sized by totals, skipping kinds with count 0 (or absent).
    static AncillaCursor FromTotals(
            const std::map<AncillaKind, std::size_t>& totals,
            const std::function<Qubits(std::string_view)>& get_qubits
    ) {
        const auto pool_for = [&totals, &get_qubits](AncillaKind kind, std::string_view name) {
            const auto it = totals.find(kind);
            return (it != totals.end() && it->second > 0) ? std::make_optional(get_qubits(name))
                                                          : std::nullopt;
        };
        return AncillaCursor(
                pool_for(AncillaKind::Clean, "anc_clean"),
                pool_for(AncillaKind::Operate, "anc_operate"),
                pool_for(AncillaKind::Dirty, "anc_dirty")
        );
    }

    // Builds pools for whichever pool names arg declares.
    static AncillaCursor FromArgument(
            const Circuit::Argument& arg,
            const std::function<Qubits(std::string_view)>& get_qubits
    ) {
        const auto pool_for = [&arg, &get_qubits](std::string_view name) {
            return arg.Contains(name) ? std::make_optional(get_qubits(name)) : std::nullopt;
        };
        return AncillaCursor(pool_for("anc_clean"), pool_for("anc_operate"), pool_for("anc_dirty"));
    }

    AncillaCursor(
            std::optional<Qubits> clean_pool,
            std::optional<Qubits> operate_pool,
            std::optional<Qubits> dirty_pool
    )
        : clean_pool_(std::move(clean_pool))
        , operate_pool_(std::move(operate_pool))
        , dirty_pool_(std::move(dirty_pool)) {}
    Qubit TakeOne(AncillaKind kind) {
        auto& pool = PoolFor(kind);
        auto& offset = OffsetFor(kind);
        if (!pool.has_value() || offset >= pool->Size()) {
            throw std::runtime_error("internal error: ancilla pool exhausted");
        }
        return (*pool)[offset++];
    }

    // Asserts every declared pool was fully drawn by TakeOne, catching an under-draw the way an
    // over-draw already fails loudly.
    void RequireFullyConsumed() const {
        for (const auto& [pool, offset] :
             {std::pair{&clean_pool_, clean_offset_},
              std::pair{&operate_pool_, operate_offset_},
              std::pair{&dirty_pool_, dirty_offset_}}) {
            if (pool->has_value() && offset != (*pool)->Size()) {
                throw std::runtime_error("internal error: ancilla pool not fully consumed");
            }
        }
    }

private:
    std::optional<Qubits>& PoolFor(AncillaKind kind) {
        switch (kind) {
            case AncillaKind::Clean:
                return clean_pool_;
            case AncillaKind::Operate:
                return operate_pool_;
            case AncillaKind::Dirty:
                return dirty_pool_;
            default:
                throw std::runtime_error("internal error: unhandled AncillaKind");
        }
    }
    std::size_t& OffsetFor(AncillaKind kind) {
        switch (kind) {
            case AncillaKind::Clean:
                return clean_offset_;
            case AncillaKind::Operate:
                return operate_offset_;
            case AncillaKind::Dirty:
                return dirty_offset_;
            default:
                throw std::runtime_error("internal error: unhandled AncillaKind");
        }
    }

    std::optional<Qubits> clean_pool_;
    std::optional<Qubits> operate_pool_;
    std::optional<Qubits> dirty_pool_;
    std::size_t clean_offset_ = 0;
    std::size_t operate_offset_ = 0;
    std::size_t dirty_offset_ = 0;
};

Circuit::Attribute AttributeForAncillaKind(AncillaKind kind) {
    switch (kind) {
        case AncillaKind::Clean:
            return Circuit::Attribute::CleanAncilla;
        case AncillaKind::Operate:
            return Circuit::Attribute::Operate;
        case AncillaKind::Dirty:
            return Circuit::Attribute::DirtyAncilla;
        default:
            throw std::runtime_error("internal error: unhandled AncillaKind");
    }
}

void DeclareAncillaPools(Circuit::Argument& arg, const std::map<AncillaKind, std::size_t>& total) {
    static constexpr std::array<std::pair<AncillaKind, const char*>, 3> kPools = {
            {{AncillaKind::Clean, "anc_clean"},
             {AncillaKind::Operate, "anc_operate"},
             {AncillaKind::Dirty, "anc_dirty"}}
    };
    for (const auto& [kind, name] : kPools) {
        if (const auto it = total.find(kind); it != total.end() && it->second > 0) {
            arg.Add(name, Circuit::Type::Qubit, it->second, AttributeForAncillaKind(kind));
        }
    }
}

// The target gate's own scratch qubit count when Controlled, excluding the ctrl ladder.
std::size_t ControlledTargetOwnScratchCountFor(const qpb::Bloq& bloq) {
    if (bloq.name() == kZPowGateBloqName || bloq.name() == kTGateBloqName
        || bloq.name() == kSGateBloqName) {
        return 1;
    }
    if (bloq.name() == kToffoliBloqName) {
        return 2;
    }
    return 0;
}

// Must mirror EmitControlledWrapper's own ancilla_cursor draws: ladder + target's own scratch.
std::size_t ScratchAncillaCountForControlled(const UnwrappedControlled& unwrapped) {
    auto total_cv_count = std::size_t{0};
    for (const auto& reg : unwrapped.ctrl_regs) {
        total_cv_count += reg.cvs.size();
    }
    const auto ladder_count = (total_cv_count > 1) ? (total_cv_count - 1) : std::size_t{0};
    return ladder_count + ControlledTargetOwnScratchCountFor(*unwrapped.bloq);
}

// Quration-only scratch ancillae a gate needs beyond what Qualtran's own signature declares.
std::size_t ScratchAncillaCountFor(const qpb::Bloq& bloq) {
    if (bloq.name() == kCZPowGateBloqName) {
        return 1;
    }
    return 0;
}

const qpb::Register& FindRegisterOnSide(const qpb::Bloq& bloq, qpb::Register::Side side) {
    const qpb::Register* found = nullptr;
    for (const auto& reg : bloq.registers().registers()) {
        if (reg.side() != side) {
            continue;
        }
        if (found != nullptr) {
            throw QRETError(
                    error::MalformedQualtranProto,
                    fmt::format(
                            "bloq '{}' has more than one register on the same side",
                            bloq.name()
                    )
            );
        }
        found = &reg;
    }
    if (found == nullptr) {
        throw QRETError(
                error::MalformedQualtranProto,
                fmt::format("bloq '{}' has no register on the expected side", bloq.name())
        );
    }
    return *found;
}

// The newly allocated qubit register of a NewQubitProducer bloq.
const qpb::Register& NewQubitRegister(const qpb::Bloq& bloq) {
    return FindRegisterOnSide(bloq, qpb::Register::RIGHT);
}

// Flattens instance_id's single register on the given side, already-bound in wire_map, into Qubits.
Qubits FlattenRegisterQubits(
        const qpb::Bloq& bloq,
        qpb::Register::Side side,
        std::int32_t instance_id,
        const WireMap& wire_map
) {
    const auto& reg = FindRegisterOnSide(bloq, side);
    const auto reg_size = ComputeRegisterSize(reg);
    auto qs = Qubits{LookupWire(wire_map, instance_id, reg.name(), 0)};
    for (auto k = std::int32_t{1}; k < static_cast<std::int32_t>(reg_size); ++k) {
        qs += LookupWire(wire_map, instance_id, reg.name(), k);
    }
    return qs;
}

// ─────────────────────────────────────────────────────────────
// Preset CircuitGenerator classes for parameterless atomic Qualtran gates.
// ─────────────────────────────────────────────────────────────

class CHadamardGen final : public CircuitGenerator {
public:
    static inline const char* Name = "CHadamard";
    explicit CHadamardGen(CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return Name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("ctrl", Type::Qubit, 1, Attribute::Operate);
        arg.Add("target", Type::Qubit, 1, Attribute::Operate);
    }
    Circuit* Generate() const override {
        if (IsCached()) {
            return GetCachedCircuit();
        }
        BeginCircuitDefinition();
        const auto ctrl = GetQubit("ctrl");
        const auto tgt = GetQubit("target");
        gate::SDag(tgt);
        gate::H(tgt);
        gate::TDag(tgt);
        gate::CX(tgt, ctrl);
        gate::T(tgt);
        gate::H(tgt);
        gate::S(tgt);
        return EndCircuitDefinition();
    }
    // The [SDag,H,TDag,CX,T,H,S] sequence is its own inverse (reversed + each instruction
    // inverted reproduces the same list), so Adjoint(CHadamard) is just CHadamard itself.
    Circuit* GenerateAdjoint() const override {
        return GetCachedCircuit();
    }
};

class TwoBitSwapGen final : public CircuitGenerator {
public:
    static inline const char* Name = "TwoBitSwap";
    explicit TwoBitSwapGen(CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return Name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("x", Type::Qubit, 1, Attribute::Operate);
        arg.Add("y", Type::Qubit, 1, Attribute::Operate);
    }
    Circuit* Generate() const override {
        if (IsCached()) {
            return GetCachedCircuit();
        }
        BeginCircuitDefinition();
        const auto x = GetQubit("x");
        const auto y = GetQubit("y");
        gate::CX(y, x);
        gate::CX(x, y);
        gate::CX(y, x);
        return EndCircuitDefinition();
    }
    // [CX,CX,CX] is its own inverse.
    Circuit* GenerateAdjoint() const override {
        return GetCachedCircuit();
    }
};

class TwoBitCSwapGen final : public CircuitGenerator {
public:
    static inline const char* Name = "TwoBitCSwap";
    explicit TwoBitCSwapGen(CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return Name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("ctrl", Type::Qubit, 1, Attribute::Operate);
        arg.Add("x", Type::Qubit, 1, Attribute::Operate);
        arg.Add("y", Type::Qubit, 1, Attribute::Operate);
    }
    Circuit* Generate() const override {
        if (IsCached()) {
            return GetCachedCircuit();
        }
        BeginCircuitDefinition();
        const auto ctrl = GetQubit("ctrl");
        const auto x = GetQubit("x");
        const auto y = GetQubit("y");
        gate::CX(y, x);
        gate::CCX(x, ctrl, y);
        gate::CX(y, x);
        return EndCircuitDefinition();
    }
    // [CX,CCX,CX] is its own inverse (symmetric around the self-adjoint CCX).
    Circuit* GenerateAdjoint() const override {
        return GetCachedCircuit();
    }
};

class SwapGen final : public CircuitGenerator {
public:
    explicit SwapGen(CircuitBuilder* builder, std::int32_t bitsize)
        : CircuitGenerator(builder)
        , bitsize_(bitsize) {}
    std::string GetName() const override {
        return fmt::format("Swap_bitsize{}", bitsize_);
    }
    std::string GetCacheKey() const override {
        return GetName();
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("x", Type::Qubit, static_cast<std::size_t>(bitsize_), Attribute::Operate);
        arg.Add("y", Type::Qubit, static_cast<std::size_t>(bitsize_), Attribute::Operate);
    }
    Circuit* Generate() const override {
        if (IsCached()) {
            return GetCachedCircuit();
        }
        BeginCircuitDefinition();
        const auto x = GetQubits("x");
        const auto y = GetQubits("y");
        auto two_bit_gen = TwoBitSwapGen(GetBuilder());
        auto* preset = two_bit_gen.Generate();
        for (auto i = std::int32_t{0}; i < bitsize_; ++i) {
            (*preset)(x[static_cast<std::uint64_t>(i)], y[static_cast<std::uint64_t>(i)]);
        }
        return EndCircuitDefinition();
    }
    // A parallel application of self-adjoint TwoBitSwap instances is itself self-adjoint.
    Circuit* GenerateAdjoint() const override {
        return GetCachedCircuit();
    }

private:
    std::int32_t bitsize_;
};

class CSwapGen final : public CircuitGenerator {
public:
    explicit CSwapGen(CircuitBuilder* builder, std::int32_t bitsize)
        : CircuitGenerator(builder)
        , bitsize_(bitsize) {}
    std::string GetName() const override {
        return fmt::format("CSwap_bitsize{}", bitsize_);
    }
    std::string GetCacheKey() const override {
        return GetName();
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("ctrl", Type::Qubit, 1, Attribute::Operate);
        arg.Add("x", Type::Qubit, static_cast<std::size_t>(bitsize_), Attribute::Operate);
        arg.Add("y", Type::Qubit, static_cast<std::size_t>(bitsize_), Attribute::Operate);
    }
    Circuit* Generate() const override {
        if (IsCached()) {
            return GetCachedCircuit();
        }
        BeginCircuitDefinition();
        const auto ctrl = GetQubit("ctrl");
        const auto x = GetQubits("x");
        const auto y = GetQubits("y");
        auto two_bit_gen = TwoBitCSwapGen(GetBuilder());
        auto* preset = two_bit_gen.Generate();
        for (auto i = std::int32_t{0}; i < bitsize_; ++i) {
            (*preset)(ctrl, x[static_cast<std::uint64_t>(i)], y[static_cast<std::uint64_t>(i)]);
        }
        return EndCircuitDefinition();
    }
    // A parallel application of self-adjoint TwoBitCSwap instances is itself self-adjoint.
    Circuit* GenerateAdjoint() const override {
        return GetCachedCircuit();
    }

private:
    std::int32_t bitsize_;
};

// Forward declaration: AndComputeGen and AndUncomputeGen are each other's adjoint.
class AndUncomputeGen;

// Mirrors And.decompose_from_registers()'s compute branch.
// See Babbush et al. 2018, arXiv:1805.03662 for detail.
class AndComputeGen final : public CircuitGenerator {
public:
    explicit AndComputeGen(CircuitBuilder* builder, std::int32_t cv1, std::int32_t cv2)
        : CircuitGenerator(builder)
        , cv1_(cv1)
        , cv2_(cv2) {}
    std::string GetName() const override {
        return fmt::format("And_cv{}{}__compute", cv1_, cv2_);
    }
    std::string GetCacheKey() const override {
        return GetName();
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("ctrl0", Type::Qubit, 1, Attribute::Operate);
        arg.Add("ctrl1", Type::Qubit, 1, Attribute::Operate);
        arg.Add("target", Type::Qubit, 1, Attribute::Operate);
    }
    Circuit* Generate() const override {
        if (IsCached()) {
            return GetCachedCircuit();
        }
        BeginCircuitDefinition();
        const auto ctrl0 = GetQubit("ctrl0");
        const auto ctrl1 = GetQubit("ctrl1");
        const auto target = GetQubit("target");
        attribute::MarkAsClean(target);
        if (cv1_ == 0) {
            gate::X(ctrl0);
        }
        if (cv2_ == 0) {
            gate::X(ctrl1);
        }
        gate::H(target);
        gate::T(target);
        gate::CX(target, ctrl0);
        gate::CX(target, ctrl1);
        gate::CX(ctrl0, target);
        gate::CX(ctrl1, target);
        gate::TDag(ctrl0);
        gate::TDag(ctrl1);
        gate::T(target);
        gate::CX(ctrl0, target);
        gate::CX(ctrl1, target);
        gate::H(target);
        gate::S(target);
        if (cv1_ == 0) {
            gate::X(ctrl0);
        }
        if (cv2_ == 0) {
            gate::X(ctrl1);
        }
        return EndCircuitDefinition();
    }
    Circuit* GenerateAdjoint() const override;

private:
    std::int32_t cv1_;
    std::int32_t cv2_;
};

// Mirrors decompose_from_registers()'s uncompute branch.
// See Gidney 2019, algassert.com/post/1903 for detail.
class AndUncomputeGen final : public CircuitGenerator {
public:
    explicit AndUncomputeGen(CircuitBuilder* builder, std::int32_t cv1, std::int32_t cv2)
        : CircuitGenerator(builder)
        , cv1_(cv1)
        , cv2_(cv2) {}
    std::string GetName() const override {
        return fmt::format("And_cv{}{}__uncompute", cv1_, cv2_);
    }
    std::string GetCacheKey() const override {
        return GetName();
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("ctrl0", Type::Qubit, 1, Attribute::Operate);
        arg.Add("ctrl1", Type::Qubit, 1, Attribute::Operate);
        arg.Add("target", Type::Qubit, 1, Attribute::Operate);
    }
    Circuit* Generate() const override {
        if (IsCached()) {
            return GetCachedCircuit();
        }
        BeginCircuitDefinition();
        const auto ctrl0 = GetQubit("ctrl0");
        const auto ctrl1 = GetQubit("ctrl1");
        const auto target = GetQubit("target");
        if (cv1_ == 0) {
            gate::X(ctrl0);
        }
        if (cv2_ == 0) {
            gate::X(ctrl1);
        }
        const auto r = GetTemporalRegister();
        gate::H(target);
        gate::Measure(target, r);
        control_flow::If(r);
        {
            gate::X(target);
            gate::CZ(ctrl0, ctrl1);
        }
        control_flow::EndIf(r);
        if (cv1_ == 0) {
            gate::X(ctrl0);
        }
        if (cv2_ == 0) {
            gate::X(ctrl1);
        }
        attribute::MarkAsClean(target);
        return EndCircuitDefinition();
    }
    Circuit* GenerateAdjoint() const override {
        return AndComputeGen(GetBuilder(), cv1_, cv2_).Generate();
    }

private:
    std::int32_t cv1_;
    std::int32_t cv2_;
};

Circuit* AndComputeGen::GenerateAdjoint() const {
    return AndUncomputeGen(GetBuilder(), cv1_, cv2_).Generate();
}

// ─────────────────────────────────────────────────────────────
// Gate emission: inlined instruction sequences for gates that are not preset CircuitGenerators.
// ─────────────────────────────────────────────────────────────

// ZPowGate is atomic in Qualtran; this reproduces cirq.ZPowGate's unitary as GlobalPhase + Rz.
void EmitZPowGate(CircuitBuilder* builder, const Qubit& q, double exponent, double eps) {
    gate::GlobalPhase(builder, exponent / 2.0, eps);
    gate::RZ(q, std::numbers::pi * exponent, eps);
}

// XPowGate is atomic in Qualtran; this reproduces cirq.XPowGate's unitary as GlobalPhase + Rx.
void EmitXPowGate(
        CircuitBuilder* builder,
        const Qubit& q,
        double exponent,
        double global_shift,
        double eps
) {
    gate::GlobalPhase(builder, exponent * (global_shift + 0.5), eps);
    gate::RX(q, std::numbers::pi * exponent, eps);
}

// YPowGate is atomic in Qualtran; this reproduces cirq.YPowGate's unitary as GlobalPhase + Ry.
void EmitYPowGate(
        CircuitBuilder* builder,
        const Qubit& q,
        double exponent,
        double global_shift,
        double eps
) {
    gate::GlobalPhase(builder, exponent * (global_shift + 0.5), eps);
    gate::RY(q, std::numbers::pi * exponent, eps);
}

// Mirrors Qualtran's SU2RotationGate.build_composite_bloq().
void EmitSU2RotationGate(
        CircuitBuilder* builder,
        const Qubit& q,
        double theta,
        double phi,
        double lambd,
        double global_shift,
        double eps
) {
    const auto sub_eps = eps / 3.0;
    gate::GlobalPhase(
            builder,
            0.5 + global_shift / std::numbers::pi + lambd / (2.0 * std::numbers::pi)
                    + phi / (2.0 * std::numbers::pi),
            eps
    );
    gate::RZ(q, std::numbers::pi / 2.0 - lambd, sub_eps);
    gate::RX(q, 2.0 * theta, sub_eps);
    gate::RZ(q, std::numbers::pi / 2.0 - phi, sub_eps);
}

// Mirrors Qualtran's CRz.build_composite_bloq().
void EmitCRzGate(const Qubit& ctrl, const Qubit& q, double angle, double eps) {
    gate::RZ(q, angle / 2.0, eps / 2.0);
    gate::CX(q, ctrl);
    gate::RZ(q, -angle / 2.0, eps / 2.0);
    gate::CX(q, ctrl);
}

void EmitCRyGate(const Qubit& ctrl, const Qubit& q, double angle, double eps) {
    gate::RY(q, angle / 2.0, eps / 2.0);
    gate::CX(q, ctrl);
    gate::RY(q, -angle / 2.0, eps / 2.0);
    gate::CX(q, ctrl);
}

void EmitCRxGate(const Qubit& ctrl, const Qubit& q, double angle, double eps) {
    gate::H(q);
    EmitCRzGate(ctrl, q, angle, eps);
    gate::H(q);
}

// Mirrors Qualtran's CZPowGate.build_composite_bloq().
void EmitCZPowGate(
        CircuitBuilder* builder,
        const Qubit& q0,
        const Qubit& q1,
        const Qubit& anc,
        double exponent,
        double eps
) {
    (*AndComputeGen(builder, 1, 1).Generate())(q0, q1, anc);
    EmitZPowGate(builder, anc, exponent, eps);
    (*AndUncomputeGen(builder, 1, 1).Generate())(q0, q1, anc);
}

// No Controlled(XPowGate) decomposition exists; EmitXPowGate's GlobalPhase+Rx becomes CRx + a
// ZPowGate on ctrl for the GlobalPhase term.
void EmitCXPowGate(
        CircuitBuilder* builder,
        const Qubit& ctrl,
        const Qubit& q,
        double exponent,
        double global_shift,
        double eps
) {
    EmitCRxGate(ctrl, q, std::numbers::pi * exponent, eps);
    EmitZPowGate(builder, ctrl, exponent * (global_shift + 0.5), eps);
}

// No Controlled(YPowGate) decomposition exists; EmitYPowGate's GlobalPhase+Ry becomes CRy + a
// ZPowGate on ctrl for the GlobalPhase term.
void EmitCYPowGate(
        CircuitBuilder* builder,
        const Qubit& ctrl,
        const Qubit& q,
        double exponent,
        double global_shift,
        double eps
) {
    EmitCRyGate(ctrl, q, std::numbers::pi * exponent, eps);
    EmitZPowGate(builder, ctrl, exponent * (global_shift + 0.5), eps);
}

// No Controlled(SU2RotationGate) decomposition exists; EmitSU2RotationGate's Rz-Rx-Rz+GlobalPhase
// becomes CRz-C[Rx]-CRz + a ZPowGate on ctrl for the GlobalPhase term.
void EmitControlledSU2RotationGate(
        CircuitBuilder* builder,
        const Qubit& ctrl,
        const Qubit& q,
        double theta,
        double phi,
        double lambd,
        double global_shift,
        double eps
) {
    const auto sub_eps = eps / 3.0;
    EmitCRzGate(ctrl, q, std::numbers::pi / 2.0 - lambd, sub_eps);
    EmitCRxGate(ctrl, q, 2.0 * theta, sub_eps);
    EmitCRzGate(ctrl, q, std::numbers::pi / 2.0 - phi, sub_eps);
    EmitZPowGate(
            builder,
            ctrl,
            0.5 + global_shift / std::numbers::pi + lambd / (2.0 * std::numbers::pi)
                    + phi / (2.0 * std::numbers::pi),
            eps
    );
}

// Dispatches an And instance (either direction) to the matching preset.
ir::CallInst* EmitAndInst(
        CircuitBuilder* builder,
        const qpb::Bloq& bloq,
        std::int32_t instance_id,
        const WireMap& wire_map
) {
    const auto cv1 = ArgToInt(bloq, "cv1");
    const auto cv2 = ArgToInt(bloq, "cv2");
    const auto ctrl0 = LookupWire(wire_map, instance_id, "ctrl", 0);
    const auto ctrl1 = LookupWire(wire_map, instance_id, "ctrl", 1);
    // compute pre-binds this via EmitNewQubitProducer; uncompute resolves it as any incoming edge.
    const auto target = LookupWire(wire_map, instance_id, "target");
    if (IsAndCompute(bloq)) {
        return (*AndComputeGen(builder, cv1, cv2).Generate())(ctrl0, ctrl1, target);
    }
    return (*AndUncomputeGen(builder, cv1, cv2).Generate())(ctrl0, ctrl1, target);
}

// Takes a name, not a bloq, so an AdjointWrapper's effective-producer Effect (no real State bloq
// to read) can share this with the ordinary NewQubitProducer path.
void EmitStateInit(std::string_view state_name, bool is_dirty, const Qubits& qs) {
    if (is_dirty) {
        return;  // MarkAsDirtyBegin comes from anc_dirty's own pool attribute, not here.
    }
    for (const auto& q : qs) {
        attribute::MarkAsClean(q);
    }
    if (state_name == kOneStateBloqName) {
        gate::X(qs[0]);
    } else if (state_name == kPlusStateBloqName) {
        gate::H(qs[0]);
    } else if (state_name == kMinusStateBloqName) {
        gate::X(qs[0]);
        gate::H(qs[0]);
    }
}

// Takes a name, not a bloq, so an AdjointWrapper's effective-consumer State (no real Effect bloq
// to read) can share this with the ordinary QubitConsumer path.
void EmitQubitConsumerGates(std::string_view effect_name, bool is_dirty, const Qubits& qs) {
    if (effect_name == kFreeBloqName && is_dirty) {
        return;  // MarkAsDirtyEnd comes from anc_dirty's own pool attribute, not here.
    }
    if (effect_name != kZeroEffectBloqName && effect_name != kFreeBloqName) {
        // Trusts its basis-state contract without verification; nothing else to emit.
        LOG_WARN(
                "Qualtran '{}' has no Quration IR counterpart; trusting its basis-state contract "
                "without emitting any instruction.",
                effect_name
        );
        return;
    }
    // ZeroEffect/Free(dirty=False) assert clean unconditionally, like AndComputeGen's own target.
    for (const auto& q : qs) {
        attribute::MarkAsClean(q);
    }
}

// is_adjoint requests this gate's own adjoint (e.g. an AdjointWrapper's unwrapped Gate terminal).
// Branches with no is_adjoint-dependent code are self-adjoint, so they simply ignore it.
void EmitGate(
        CircuitBuilder* builder,
        const qpb::Bloq& bloq,
        std::int32_t instance_id,
        const WireMap& wire_map,
        AncillaCursor& ancilla_cursor,
        bool is_adjoint = false
) {
    const auto& name = bloq.name();
    const auto sign = is_adjoint ? -1.0 : 1.0;

    auto lookup_qubit = [&wire_map,
                         instance_id](std::string_view reg, std::int32_t idx = 0) -> Qubit {
        return LookupWire(wire_map, instance_id, reg, idx);
    };
    auto lookup_qubits = [&lookup_qubit](std::string_view reg, std::int32_t n) -> Qubits {
        auto qs = Qubits{lookup_qubit(reg, 0)};
        for (auto i = std::int32_t{1}; i < n; ++i) {
            qs += lookup_qubit(reg, i);
        }
        return qs;
    };

    if (name == kHadamardBloqName) {
        gate::H(lookup_qubit("q"));
    } else if (name == kXGateBloqName) {
        gate::X(lookup_qubit("q"));
    } else if (name == kYGateBloqName) {
        gate::Y(lookup_qubit("q"));
    } else if (name == kZGateBloqName) {
        gate::Z(lookup_qubit("q"));
    } else if (name == kIdentityBloqName) {
        gate::I(lookup_qubit("q"));
    } else if (name == kTGateBloqName) {
        (HasAdjointFlag(bloq) != is_adjoint) ? gate::TDag(lookup_qubit("q"))
                                             : gate::T(lookup_qubit("q"));
    } else if (name == kSGateBloqName) {
        (HasAdjointFlag(bloq) != is_adjoint) ? gate::SDag(lookup_qubit("q"))
                                             : gate::S(lookup_qubit("q"));
    } else if (name == kCNOTBloqName) {
        gate::CX(lookup_qubit("target"), lookup_qubit("ctrl"));
    } else if (name == kCZBloqName) {
        gate::CZ(lookup_qubit("q2"), lookup_qubit("q1"));
    } else if (name == kCYGateBloqName) {
        gate::CY(lookup_qubit("target"), lookup_qubit("ctrl"));
    } else if (name == kToffoliBloqName) {
        gate::CCX(lookup_qubit("target"), lookup_qubit("ctrl", 0), lookup_qubit("ctrl", 1));
    } else if (name == kRxBloqName) {
        gate::RX(lookup_qubit("q"), sign * ArgToDouble(bloq, "angle"), ArgToDouble(bloq, "eps"));
    } else if (name == kRyBloqName) {
        gate::RY(lookup_qubit("q"), sign * ArgToDouble(bloq, "angle"), ArgToDouble(bloq, "eps"));
    } else if (name == kRzBloqName) {
        gate::RZ(lookup_qubit("q"), sign * ArgToDouble(bloq, "angle"), ArgToDouble(bloq, "eps"));
    } else if (name == kGlobalPhaseBloqName) {
        gate::GlobalPhase(builder, sign * ArgToDouble(bloq, "exponent"), ArgToDouble(bloq, "eps"));
    } else if (name == kCHadamardBloqName) {
        auto gen = CHadamardGen(builder);
        (*gen.Generate())(lookup_qubit("ctrl"), lookup_qubit("target"));
    } else if (name == kTwoBitSwapBloqName) {
        auto gen = TwoBitSwapGen(builder);
        (*gen.Generate())(lookup_qubit("x"), lookup_qubit("y"));
    } else if (name == kTwoBitCSwapBloqName) {
        auto gen = TwoBitCSwapGen(builder);
        (*gen.Generate())(lookup_qubit("ctrl"), lookup_qubit("x"), lookup_qubit("y"));
    } else if (name == kSwapBloqName) {
        const auto bitsize = ArgToInt(bloq, "bitsize");
        auto gen = SwapGen(builder, bitsize);
        (*gen.Generate())(lookup_qubits("x", bitsize), lookup_qubits("y", bitsize));
    } else if (name == kCSwapBloqName) {
        const auto bitsize = ArgToInt(bloq, "bitsize");
        auto gen = CSwapGen(builder, bitsize);
        (*gen.Generate())(
                lookup_qubit("ctrl"),
                lookup_qubits("x", bitsize),
                lookup_qubits("y", bitsize)
        );
    } else if (name == kZPowGateBloqName) {
        EmitZPowGate(
                builder,
                lookup_qubit("q"),
                sign * ArgToDouble(bloq, "exponent"),
                ArgToDouble(bloq, "eps")
        );
    } else if (name == kXPowGateBloqName) {
        EmitXPowGate(
                builder,
                lookup_qubit("q"),
                sign * ArgToDouble(bloq, "exponent"),
                ArgToDouble(bloq, "global_shift"),
                ArgToDouble(bloq, "eps")
        );
    } else if (name == kYPowGateBloqName) {
        EmitYPowGate(
                builder,
                lookup_qubit("q"),
                sign * ArgToDouble(bloq, "exponent"),
                ArgToDouble(bloq, "global_shift"),
                ArgToDouble(bloq, "eps")
        );
    } else if (name == kSU2RotationGateBloqName) {
        // SU2RotationGate.adjoint(): theta unchanged, phi<->lambd swapped and negated.
        const auto theta = ArgToDouble(bloq, "theta");
        const auto phi = ArgToDouble(bloq, "phi");
        const auto lambd = ArgToDouble(bloq, "lambd");
        const auto global_shift = ArgToDouble(bloq, "global_shift");
        EmitSU2RotationGate(
                builder,
                lookup_qubit("q"),
                theta,
                is_adjoint ? -lambd : phi,
                is_adjoint ? -phi : lambd,
                is_adjoint ? -global_shift : global_shift,
                ArgToDouble(bloq, "eps")
        );
    } else if (name == kCZPowGateBloqName) {
        EmitCZPowGate(
                builder,
                lookup_qubit("q", 0),
                lookup_qubit("q", 1),
                ancilla_cursor.TakeOne(AncillaKind::Clean),
                sign * ArgToDouble(bloq, "exponent"),
                ArgToDouble(bloq, "eps")
        );
    } else if (name == kCRzBloqName) {
        EmitCRzGate(
                lookup_qubit("ctrl"),
                lookup_qubit("q"),
                sign * ArgToDouble(bloq, "angle"),
                ArgToDouble(bloq, "eps")
        );
    } else if (name == kAndBloqName) {
        EmitAndInst(builder, bloq, instance_id, wire_map);
    } else {
        throw QRETError(
                error::UnsupportedQualtranBloq,
                fmt::format("unsupported Qualtran gate: '{}'", name)
        );
    }
}

// ─────────────────────────────────────────────────────────────
// Circuit generation.
// ─────────────────────────────────────────────────────────────

// Flattens bloq's registers on the given side into (reg_name, idx) pairs, in declared order.
std::vector<std::pair<std::string, std::int32_t>>
FlatSoquets(const qpb::Bloq& bloq, qpb::Register::Side side) {
    auto flat = std::vector<std::pair<std::string, std::int32_t>>{};
    for (const auto& reg : bloq.registers().registers()) {
        if (reg.side() != side) {
            continue;
        }
        const auto reg_size = ComputeRegisterSize(reg);
        for (auto k = std::int32_t{0}; k < static_cast<std::int32_t>(reg_size); ++k) {
            flat.emplace_back(reg.name(), k);
        }
    }
    return flat;
}

// No gate emitted: RIGHT wires alias the same Qubits as LEFT, just redistributed.
void ExpandAliasWires(std::int32_t inst_id, const qpb::Bloq& bloq, WireMap& wire_map) {
    auto flat_qubits = std::vector<Qubit>{};
    for (const auto& [reg_name, idx] : FlatSoquets(bloq, qpb::Register::LEFT)) {
        flat_qubits.emplace_back(LookupWire(wire_map, inst_id, reg_name, idx));
    }
    const auto right_flat = FlatSoquets(bloq, qpb::Register::RIGHT);
    if (right_flat.size() != flat_qubits.size()) {
        throw QRETError(
                error::MalformedQualtranProto,
                fmt::format(
                        "alias bloq '{}' instance {}: LEFT qubit count {} does not match "
                        "RIGHT qubit count {}",
                        bloq.name(),
                        inst_id,
                        flat_qubits.size(),
                        right_flat.size()
                )
        );
    }
    for (auto pos = std::size_t{0}; pos < right_flat.size(); ++pos) {
        wire_map.emplace(
                SoquetKey{inst_id, right_flat[pos].first, right_flat[pos].second},
                flat_qubits[pos]
        );
    }
}

// Builds and memoizes each bloq_id's Generator/Circuit on first request, recursing into whatever
// structural dependency it needs. The root is built separately, never through this Converter.
class Converter {
public:
    explicit Converter(CircuitBuilder* builder, const BloqDag& dag)
        : builder_(builder)
        , dag_(dag) {}

    // Argument only; never Generates. Builds the Generator on first use if not built yet.
    Circuit::Argument GetArgumentOf(std::int32_t bloq_id);

    // Generates on first use, then reuses the cached Circuit*.
    Circuit* GetOrGenerate(std::int32_t bloq_id);

    CircuitBuilder* GetBuilder() const {
        return builder_;
    }
    const BloqDag& GetDag() const {
        return dag_;
    }

private:
    CircuitGenerator& EnsureGenerator(std::int32_t bloq_id);

    CircuitBuilder* builder_;
    const BloqDag& dag_;
    std::map<std::int32_t, std::unique_ptr<CircuitGenerator>> generator_cache_;
    std::map<std::int32_t, Circuit*> circuit_cache_;
    std::set<std::int32_t> building_;  // bloq_ids under construction; re-entry means a cycle.
};

// ─────────────────────────────────────────────────────────────
// AdjointWrapper/ControlledWrapper dispatch: shared by BloqCircuitGen's DAG-loop and leaf paths.
// ─────────────────────────────────────────────────────────────

using ResolveFreshQubitFn = std::function<Qubit(const SoquetKey&)>;

// Calls sub_circuit with bloq's registers, then ancilla_cursor's pools sized to sub_circuit's own.
ir::CallInst* EmitCallWithForwardedPools(
        std::int32_t inst_id,
        const qpb::Bloq& bloq,
        Circuit* sub_circuit,
        WireMap& wire_map,
        AncillaCursor& ancilla_cursor,
        const ResolveFreshQubitFn& resolve_fresh_qubit
) {
    auto q_vec = std::vector<ir::Qubit>{};
    for (const auto& reg : bloq.registers().registers()) {
        const auto& reg_name = reg.name();
        const auto reg_size = ComputeRegisterSize(reg);
        for (auto idx = std::int32_t{0}; idx < static_cast<std::int32_t>(reg_size); ++idx) {
            if (reg.side() != qpb::Register::RIGHT) {
                q_vec.emplace_back(LookupWire(wire_map, inst_id, reg_name, idx).GetId());
                continue;
            }
            const auto key = SoquetKey{inst_id, reg_name, idx};
            const auto qubit = resolve_fresh_qubit(key);
            wire_map.emplace(key, qubit);
            q_vec.emplace_back(qubit.GetId());
        }
    }
    const auto& sub_arg = sub_circuit->GetArgument();
    for (const auto& [pool_name, pool_kind] :
         {std::pair{"anc_clean", AncillaKind::Clean},
          std::pair{"anc_operate", AncillaKind::Operate},
          std::pair{"anc_dirty", AncillaKind::Dirty}}) {
        if (!sub_arg.Contains(pool_name)) {
            continue;
        }
        const auto anc_size = sub_arg.GetSize(sub_arg.GetArgIdx(pool_name));
        for (auto k = std::size_t{0}; k < anc_size; ++k) {
            q_vec.emplace_back(ancilla_cursor.TakeOne(pool_kind).GetId());
        }
    }
    return sub_circuit->CallImpl(q_vec, {}, {});
}

// Dispatches an AdjointWrapper instance's unwrapped role: AliasOnly, And, State/Effect, or
// Gate/Composite (calls the unwrapped terminal's Circuit, then conditionally ReplaceWithAdjoint).
void EmitAdjointDispatch(
        Converter* converter,
        std::int32_t inst_id,
        std::int32_t bloq_id,
        const qpb::Bloq& bloq,
        WireMap& wire_map,
        AncillaCursor& ancilla_cursor,
        // Supplies a fresh RIGHT-only register's Qubit: wire_map lookup for a leaf root (already
        // bound by GenerateLeaf), or qubit_sources_/ancilla_cursor for an inner instance.
        const ResolveFreshQubitFn& resolve_fresh_qubit
) {
    auto* builder = converter->GetBuilder();
    const auto& unwrapped = converter->GetDag().adjoint_unwraps.at(bloq_id);
    if (IsAliasOnlyBloq(*unwrapped.bloq)) {
        ExpandAliasWires(inst_id, bloq, wire_map);
        return;
    }
    if (unwrapped.bloq->name() == kAndBloqName) {
        if (UnwrappedAndActsAsCompute(unwrapped)) {
            // target is a fresh output on the wrapping bloq's own (flip-consistent) RIGHT side.
            const auto& reg = NewQubitRegister(bloq);
            const auto key = SoquetKey{inst_id, reg.name(), 0};
            wire_map.emplace(key, resolve_fresh_qubit(key));
        }
        auto* call = EmitAndInst(builder, *unwrapped.bloq, inst_id, wire_map);
        if (unwrapped.IsOddDepth()) {
            impl::ReplaceWithAdjoint(builder, call);
        }
        return;
    }
    // A Gate terminal's registers are unchanged under Adjoint (LEFT/RIGHT swap only, all THRU), so
    // EmitGate runs in-place -- except kAndBloqName, whose flip needs UnwrappedAndActsAsCompute.
    if (ClassifyBloqKind(*converter->GetDag().table.at(unwrapped.bloq_id)) == BloqKind::Gate) {
        EmitGate(
                builder,
                *unwrapped.bloq,
                inst_id,
                wire_map,
                ancilla_cursor,
                /*is_adjoint=*/unwrapped.IsOddDepth()
        );
        return;
    }
    if (IsNewQubitProducerBloq(unwrapped.bloq->name())
        || IsQubitConsumerBloq(unwrapped.bloq->name())) {
        const auto is_dirty = UnwrappedStateEffectIsDirty(unwrapped);
        const auto acts_as_producer = UnwrappedStateEffectActsAsProducer(unwrapped);
        if (acts_as_producer) {
            // A fresh output, unlike the consumer branch below: bind it into wire_map first.
            const auto& reg = NewQubitRegister(bloq);
            const auto reg_size = ComputeRegisterSize(reg);
            for (auto k = std::int32_t{0}; k < static_cast<std::int32_t>(reg_size); ++k) {
                const auto key = SoquetKey{inst_id, reg.name(), k};
                wire_map.emplace(key, resolve_fresh_qubit(key));
            }
            const auto qs = FlattenRegisterQubits(bloq, qpb::Register::RIGHT, inst_id, wire_map);
            EmitStateInit(UnwrappedStateEffectName(unwrapped), is_dirty, qs);
        } else {
            const auto qs = FlattenRegisterQubits(bloq, qpb::Register::LEFT, inst_id, wire_map);
            EmitQubitConsumerGates(UnwrappedStateEffectName(unwrapped), is_dirty, qs);
        }
        return;
    }
    auto* call = EmitCallWithForwardedPools(
            inst_id,
            bloq,
            converter->GetOrGenerate(unwrapped.bloq_id),
            wire_map,
            ancilla_cursor,
            resolve_fresh_qubit
    );
    if (unwrapped.IsOddDepth()) {
        impl::ReplaceWithAdjoint(builder, call);
    }
}

// Emit the controlled gate of target_gate with And gates.
// Used for T, TDag, S and SDag.
void EmitControlledGateViaAnd(
        CircuitBuilder* builder,
        const Qubit& q,
        const Qubit& ctrl,
        const Qubit& anc,
        ir::UnaryInst* (*target_gate)(const Qubit&)
) {
    (*AndComputeGen(builder, 1, 1).Generate())(q, ctrl, anc);
    target_gate(anc);
    (*AndUncomputeGen(builder, 1, 1).Generate())(q, ctrl, anc);
}

// Recorded so TearDownAndLadder can replay each step's AndUncomputeGen in reverse.
struct AndLadderStep {
    Qubit ctrl0;
    Qubit ctrl1;
    Qubit anc;
    std::int32_t cv0;
    std::int32_t cv1;
};

// Mirrors Qualtran's ControlledViaAnd.build_composite_bloq.
// N==1 (single_bit_negative_wrap) is a bare X-wrap, never routed through the N>=2 AND-ladder.
struct AndLadderResult {
    Qubit eff_ctrl;
    bool single_bit_negative_wrap;
    std::vector<AndLadderStep> steps;
};

// Reduces multiple control qubits to a single effective control qubit by taking their pairwise
// AND, so downstream code only has to control on one qubit regardless of N.
AndLadderResult BuildAndLadder(
        CircuitBuilder* builder,
        const Qubits& ctrl,
        const std::vector<std::int64_t>& cvs,
        AncillaCursor& ancilla_cursor
) {
    if (cvs.size() == 1) {
        if (cvs[0] == 0) {
            gate::X(ctrl[0]);
            return {ctrl[0], true, {}};
        }
        return {ctrl[0], false, {}};
    }
    auto steps = std::vector<AndLadderStep>{};
    auto prev = std::optional<Qubit>{ctrl[0]};
    auto prev_cv = static_cast<std::int32_t>(cvs[0]);
    for (auto i = std::size_t{1}; i < cvs.size(); ++i) {
        const auto anc = ancilla_cursor.TakeOne(AncillaKind::Clean);
        const auto cv = static_cast<std::int32_t>(cvs[i]);
        (*AndComputeGen(builder, prev_cv, cv).Generate())(*prev, ctrl[i], anc);
        steps.emplace_back(*prev, ctrl[i], anc, prev_cv, cv);
        prev.emplace(anc);
        prev_cv = 1;
    }
    return {*prev, false, steps};
}

// Undoes a BuildAndLadder reduction so its ancillae can be returned clean.
void TearDownAndLadder(CircuitBuilder* builder, const AndLadderResult& ladder) {
    if (ladder.single_bit_negative_wrap) {
        gate::X(ladder.eff_ctrl);
        return;
    }
    for (auto it = ladder.steps.rbegin(); it != ladder.steps.rend(); ++it) {
        (*AndUncomputeGen(builder, it->cv0, it->cv1).Generate())(it->ctrl0, it->ctrl1, it->anc);
    }
}

using LookupQubitFn = std::function<Qubit(std::string_view, std::int32_t)>;

// eff_ctrl is already reduced to one qubit via BuildAndLadder. lookup_qubit resolves subbloq's
// registers from wire_map, or from a throwaway Circuit's own args when EmitControlledWrapper needs
// the whole gate as one Circuit to adjoint (the Controlled(Adjoint(subbloq)) path).
void EmitControlledSubbloqGate(
        CircuitBuilder* builder,
        const qpb::Bloq& subbloq,
        const Qubit& eff_ctrl,
        const LookupQubitFn& lookup_qubit,
        AncillaCursor& ancilla_cursor
) {
    const auto lookup_qubits = [&lookup_qubit](std::string_view reg, std::int32_t n) -> Qubits {
        auto qs = Qubits{lookup_qubit(reg, 0)};
        for (auto i = std::int32_t{1}; i < n; ++i) {
            qs += lookup_qubit(reg, i);
        }
        return qs;
    };
    const auto& name = subbloq.name();
    if (name == kXGateBloqName) {
        gate::CX(lookup_qubit("q", 0), eff_ctrl);
    } else if (name == kYGateBloqName) {
        gate::CY(lookup_qubit("q", 0), eff_ctrl);
    } else if (name == kZGateBloqName) {
        gate::CZ(lookup_qubit("q", 0), eff_ctrl);
    } else if (name == kIdentityBloqName) {
        gate::I(lookup_qubit("q", 0));
    } else if (name == kHadamardBloqName) {
        auto gen = CHadamardGen(builder);
        (*gen.Generate())(eff_ctrl, lookup_qubit("q", 0));
    } else if (name == kCNOTBloqName) {
        gate::CCX(lookup_qubit("target", 0), eff_ctrl, lookup_qubit("ctrl", 0));
    } else if (name == kCZBloqName) {
        gate::CCZ(lookup_qubit("q2", 0), eff_ctrl, lookup_qubit("q1", 0));
    } else if (name == kCYGateBloqName) {
        gate::CCY(lookup_qubit("target", 0), eff_ctrl, lookup_qubit("ctrl", 0));
    } else if (name == kTwoBitSwapBloqName) {
        auto gen = TwoBitCSwapGen(builder);
        (*gen.Generate())(eff_ctrl, lookup_qubit("x", 0), lookup_qubit("y", 0));
    } else if (name == kSwapBloqName) {
        const auto bitsize = ArgToInt(subbloq, "bitsize");
        auto gen = CSwapGen(builder, bitsize);
        (*gen.Generate())(eff_ctrl, lookup_qubits("x", bitsize), lookup_qubits("y", bitsize));
    } else if (name == kRzBloqName) {
        EmitCRzGate(
                eff_ctrl,
                lookup_qubit("q", 0),
                ArgToDouble(subbloq, "angle"),
                ArgToDouble(subbloq, "eps")
        );
    } else if (name == kRyBloqName) {
        EmitCRyGate(
                eff_ctrl,
                lookup_qubit("q", 0),
                ArgToDouble(subbloq, "angle"),
                ArgToDouble(subbloq, "eps")
        );
    } else if (name == kRxBloqName) {
        EmitCRxGate(
                eff_ctrl,
                lookup_qubit("q", 0),
                ArgToDouble(subbloq, "angle"),
                ArgToDouble(subbloq, "eps")
        );
    } else if (name == kZPowGateBloqName) {
        EmitCZPowGate(
                builder,
                lookup_qubit("q", 0),
                eff_ctrl,
                ancilla_cursor.TakeOne(AncillaKind::Clean),
                ArgToDouble(subbloq, "exponent"),
                ArgToDouble(subbloq, "eps")
        );
    } else if (name == kTGateBloqName || name == kSGateBloqName) {
        const auto is_adjoint = HasAdjointFlag(subbloq);
        ir::UnaryInst* (*target_gate)(const Qubit&) = nullptr;
        if (name == kTGateBloqName) {
            target_gate = is_adjoint ? gate::TDag : gate::T;
        } else {
            target_gate = is_adjoint ? gate::SDag : gate::S;
        }
        EmitControlledGateViaAnd(
                builder,
                lookup_qubit("q", 0),
                eff_ctrl,
                ancilla_cursor.TakeOne(AncillaKind::Clean),
                target_gate
        );
    } else if (name == kXPowGateBloqName) {
        EmitCXPowGate(
                builder,
                eff_ctrl,
                lookup_qubit("q", 0),
                ArgToDouble(subbloq, "exponent"),
                ArgToDouble(subbloq, "global_shift"),
                ArgToDouble(subbloq, "eps")
        );
    } else if (name == kYPowGateBloqName) {
        EmitCYPowGate(
                builder,
                eff_ctrl,
                lookup_qubit("q", 0),
                ArgToDouble(subbloq, "exponent"),
                ArgToDouble(subbloq, "global_shift"),
                ArgToDouble(subbloq, "eps")
        );
    } else if (name == kGlobalPhaseBloqName) {
        EmitZPowGate(
                builder,
                eff_ctrl,
                ArgToDouble(subbloq, "exponent"),
                ArgToDouble(subbloq, "eps")
        );
    } else if (name == kSU2RotationGateBloqName) {
        EmitControlledSU2RotationGate(
                builder,
                eff_ctrl,
                lookup_qubit("q", 0),
                ArgToDouble(subbloq, "theta"),
                ArgToDouble(subbloq, "phi"),
                ArgToDouble(subbloq, "lambd"),
                ArgToDouble(subbloq, "global_shift"),
                ArgToDouble(subbloq, "eps")
        );
    } else if (name == kToffoliBloqName) {
        // Extends the ladder with Toffoli's own two (always-positive) ctrl qubits.
        const auto toffoli_ladder = BuildAndLadder(
                builder,
                Qubits{eff_ctrl} + lookup_qubits("ctrl", 2),
                {1, 1, 1},
                ancilla_cursor
        );
        gate::CX(lookup_qubit("target", 0), toffoli_ladder.eff_ctrl);
        TearDownAndLadder(builder, toffoli_ladder);
    } else {
        throw QRETError(
                error::UnsupportedQualtranBloq,
                fmt::format("unsupported Controlled(subbloq): subbloq is '{}'", name)
        );
    }
}

// Builds Controlled(subbloq) in a throwaway Circuit, then adjoints the single resulting CallInst:
// Controlled(Adjoint(subbloq)) == Adjoint(Controlled(subbloq)). The ctrl ladder is self-adjoint,
// so it stays outside this Circuit.
void EmitAdjointedControlledSubbloqGate(
        CircuitBuilder* builder,
        const BloqDag& dag,
        const qpb::Bloq& subbloq,
        const Qubit& eff_ctrl,
        const LookupQubitFn& lookup_qubit,
        AncillaCursor& ancilla_cursor,
        std::size_t scratch_count
) {
    auto* circuit = builder->CreateCircuitWithoutCaching(
            // Permanent, like Adjoint(Composite)'s "__adjoint__<name>".
            fmt::format("__controlled__{}", ResolveCircuitName(dag.table, subbloq))
    );
    auto& arg = circuit->GetMutArgument();
    arg.Add("eff_ctrl", Circuit::Type::Qubit, 1, Circuit::Attribute::Operate);
    for (const auto& reg : subbloq.registers().registers()) {
        arg.Add(reg.name(),
                Circuit::Type::Qubit,
                ComputeRegisterSize(reg),
                Circuit::Attribute::Operate);
    }
    if (scratch_count > 0) {
        // Declared as this Circuit's own argument, not forwarded from ancilla_cursor: a fresh
        // Circuit's argument qubits are caller-chosen. Safe as CleanAncilla here (unlike
        // BeginCircuitDefinition, this path doesn't auto-emit MarkAsClean for it).
        arg.Add("anc_clean", Circuit::Type::Qubit, scratch_count, Circuit::Attribute::CleanAncilla);
    }

    builder->BeginCircuitDefinition(circuit);
    auto local_offset = std::map<std::string, std::size_t>{};
    auto next_id = std::size_t{1};  // id 0 is eff_ctrl
    for (const auto& reg : subbloq.registers().registers()) {
        local_offset.emplace(reg.name(), next_id);
        next_id += ComputeRegisterSize(reg);
    }
    auto local_ancilla_cursor = AncillaCursor(
            (scratch_count > 0) ? std::make_optional(builder->GetQubits(next_id, scratch_count))
                                : std::nullopt,
            std::nullopt,
            std::nullopt
    );
    const auto local_lookup_qubit =
            [builder, &local_offset](std::string_view reg, std::int32_t idx = 0) -> Qubit {
        return builder->GetQubit(local_offset.at(std::string(reg)) + static_cast<std::size_t>(idx));
    };
    EmitControlledSubbloqGate(
            builder,
            subbloq,
            builder->GetQubit(0),
            local_lookup_qubit,
            local_ancilla_cursor
    );
    local_ancilla_cursor.RequireFullyConsumed();
    builder->EndCircuitDefinition(circuit);

    auto q_vec = std::vector<ir::Qubit>{};
    q_vec.emplace_back(eff_ctrl.GetId());
    for (const auto& reg : subbloq.registers().registers()) {
        const auto reg_size = ComputeRegisterSize(reg);
        for (auto k = std::int32_t{0}; k < static_cast<std::int32_t>(reg_size); ++k) {
            q_vec.emplace_back(lookup_qubit(reg.name(), k).GetId());
        }
    }
    for (auto k = std::size_t{0}; k < scratch_count; ++k) {
        q_vec.emplace_back(ancilla_cursor.TakeOne(AncillaKind::Clean).GetId());
    }
    auto* call = circuit->CallImpl(q_vec, {}, {});
    impl::ReplaceWithAdjoint(builder, call);
}

// Adjointed if is_adjoint, since Controlled(Adjoint(subbloq)) == Adjoint(Controlled(subbloq));
// the ctrl ladder is self-adjoint, so it stays outside.
void EmitControlledWrapper(
        CircuitBuilder* builder,
        const BloqDag& dag,
        std::int32_t inst_id,
        std::int32_t bloq_id,
        WireMap& wire_map,
        AncillaCursor& ancilla_cursor
) {
    const auto& unwrapped = dag.controlled_unwraps.at(bloq_id);
    auto ctrl = std::optional<Qubits>{};
    auto cvs = std::vector<std::int64_t>{};
    for (const auto& reg : unwrapped.ctrl_regs) {
        for (auto i = std::size_t{0}; i < reg.cvs.size(); ++i) {
            const auto q =
                    LookupWire(wire_map, inst_id, reg.reg_name, static_cast<std::int32_t>(i));
            if (!ctrl.has_value()) {
                ctrl.emplace(q);
            } else {
                *ctrl += q;
            }
        }
        cvs.insert(cvs.end(), reg.cvs.begin(), reg.cvs.end());
    }
    const auto ladder = BuildAndLadder(builder, *ctrl, cvs, ancilla_cursor);
    const auto& eff_ctrl = ladder.eff_ctrl;

    const auto& subbloq = *unwrapped.bloq;
    const auto lookup_qubit = [&wire_map,
                               inst_id](std::string_view reg, std::int32_t idx = 0) -> Qubit {
        return LookupWire(wire_map, inst_id, reg, idx);
    };
    if (IsAliasOnlyBloq(subbloq)) {
        // ctrl belongs to the wrapping Controlled bloq, not subbloq, so it passes through
        // untouched -- adjointing AliasOnly is a no-op, so is_adjoint doesn't matter here.
        ExpandAliasWires(inst_id, LookupBloqInTable(dag.table, bloq_id), wire_map);
    } else if (unwrapped.is_adjoint) {
        EmitAdjointedControlledSubbloqGate(
                builder,
                dag,
                subbloq,
                eff_ctrl,
                lookup_qubit,
                ancilla_cursor,
                ControlledTargetOwnScratchCountFor(subbloq)
        );
    } else {
        EmitControlledSubbloqGate(builder, subbloq, eff_ctrl, lookup_qubit, ancilla_cursor);
    }

    TearDownAndLadder(builder, ladder);
}

enum class WireTerminal : std::uint8_t { None, Boundary, QubitConsumer };
struct WireTraceResult {
    WireTerminal kind = WireTerminal::None;
    std::string boundary_reg = "";  // valid iff kind == Boundary
    std::int32_t boundary_idx = 0;  // valid iff kind == Boundary
    // valid iff kind == QubitConsumer; depth 0 (i.e. itself) unless reached through Adjoint(...).
    UnwrappedAdjoint consumer{.bloq_id = 0, .bloq = nullptr, .depth = 0};
};

// Where a Connection's LEFT soquet forwards to: a RightDangle boundary, or another instance.
struct ForwardTarget {
    bool is_boundary = false;
    std::int32_t inst_id = 0;  // valid iff !is_boundary
    std::string reg_name;
    std::int32_t idx = 0;
};

// Maps an AliasOnly bloq's LEFT (reg_name, idx) to its RIGHT counterpart at the same flat position.
std::pair<std::string, std::int32_t>
AliasOnlyForwardRemap(const qpb::Bloq& bloq, std::string_view reg_name, std::int32_t idx) {
    const auto left_flat = FlatSoquets(bloq, qpb::Register::LEFT);
    const auto pos_it =
            std::find_if(left_flat.begin(), left_flat.end(), [&reg_name, idx](const auto& e) {
                return e.first == reg_name && e.second == idx;
            });
    if (pos_it == left_flat.end()) {
        throw QRETError(
                error::MalformedQualtranProto,
                fmt::format(
                        "alias bloq '{}' has no LEFT soquet '{}'[{}]",
                        bloq.name(),
                        reg_name,
                        idx
                )
        );
    }
    const auto pos = static_cast<std::size_t>(std::distance(left_flat.begin(), pos_it));
    const auto right_flat = FlatSoquets(bloq, qpb::Register::RIGHT);
    if (pos >= right_flat.size()) {
        throw QRETError(
                error::MalformedQualtranProto,
                fmt::format(
                        "alias bloq '{}': LEFT qubit count does not match RIGHT qubit count",
                        bloq.name()
                )
        );
    }
    return right_flat[pos];
}

// A newly-produced qubit's resolved origin: a boundary register (pool_kind == nullopt) or a pool.
struct QubitSourceResolution {
    std::optional<AncillaKind> pool_kind;
    std::string boundary_reg = "";  // valid iff !pool_kind.has_value()
    std::int32_t boundary_idx = 0;  // valid iff !pool_kind.has_value()
};

// Builds a single-instance Circuit for an atomic bloq (decomposition_size() == 0): an atomic
// root, or (via Converter, converter_ == nullptr) an AdjointWrapper's unwrapped Gate terminal.
class BloqCircuitGen final : public CircuitGenerator {
public:
    explicit BloqCircuitGen(
            CircuitBuilder* builder,
            std::string_view circuit_name,
            const qpb::BloqLibrary_BloqWithDecomposition& entry,
            Converter* converter
    )
        : CircuitGenerator(builder)
        , name_(circuit_name)
        , entry_(entry)
        , converter_(converter) {
        if (entry_.decomposition_size() == 0) {
            return;  // Atomic root/leaf: none of the DAG-based fields below apply.
        }
        // Computed once here, in dependency order, instead of recomputed by SetArgument/Generate.
        bloq_id_of_ = BuildBloqIdOf();
        forward_edges_ = BuildForwardEdges();
        total_scratch_ancilla_count_ = ComputeTotalScratchAncillaCount();
        qubit_sources_ = ComputeQubitSources();
        ancilla_totals_ = ComputeAncillaTotals();
    }

    std::string GetName() const override {
        return name_;
    }

    void SetArgument(Argument& arg) const override {
        if (entry_.decomposition_size() == 0) {
            SetArgumentForLeaf(arg);
            return;
        }
        for (const auto& reg : entry_.bloq().registers().registers()) {
            arg.Add(reg.name(), Type::Qubit, ComputeRegisterSize(reg), Attribute::Operate);
        }
        DeclareAncillaPools(arg, ancilla_totals_);
    }

    Circuit* Generate() const override {
        if (entry_.decomposition_size() == 0) {
            return GenerateLeaf();
        }
        BeginCircuitDefinition();
        auto wire_map = SeedFromLeftDangle();
        auto ancilla_cursor =
                AncillaCursor::FromTotals(ancilla_totals_, [this](std::string_view name) {
                    return GetQubits(name);
                });
        for (const auto& [inst_id, bloq_id] : bloq_id_of_) {
            ResolveIncomingEdges(inst_id, wire_map);
            const auto& sub_bloq = LookupBloq(bloq_id);
            switch (ClassifyBloqKind(LookupEntry(bloq_id))) {
                case BloqKind::AliasOnly:
                    ExpandAliasWires(inst_id, sub_bloq, wire_map);
                    break;
                case BloqKind::Gate:
                    EmitGate(GetBuilder(), sub_bloq, inst_id, wire_map, ancilla_cursor);
                    break;
                case BloqKind::NewQubitProducer:
                    EmitNewQubitProducer(inst_id, sub_bloq, wire_map, ancilla_cursor);
                    break;
                case BloqKind::QubitConsumer:
                    EmitQubitConsumerGates(
                            sub_bloq.name(),
                            IsDirtyFree(sub_bloq),
                            FlattenRegisterQubits(sub_bloq, qpb::Register::LEFT, inst_id, wire_map)
                    );
                    break;
                case BloqKind::Composite:
                    EmitCallWithForwardedPools(
                            inst_id,
                            sub_bloq,
                            converter_->GetOrGenerate(bloq_id),
                            wire_map,
                            ancilla_cursor,
                            [this, &ancilla_cursor](const SoquetKey& key) {
                                return ResolveOutputQubit(qubit_sources_.at(key), ancilla_cursor);
                            }
                    );
                    break;
                case BloqKind::AdjointWrapper:
                    EmitAdjointDispatch(
                            converter_,
                            inst_id,
                            bloq_id,
                            sub_bloq,
                            wire_map,
                            ancilla_cursor,
                            [this, &ancilla_cursor](const SoquetKey& key) {
                                return ResolveOutputQubit(qubit_sources_.at(key), ancilla_cursor);
                            }
                    );
                    break;
                case BloqKind::ControlledWrapper:
                    EmitControlledWrapper(
                            GetBuilder(),
                            converter_->GetDag(),
                            inst_id,
                            bloq_id,
                            wire_map,
                            ancilla_cursor
                    );
                    break;
                default:  // Unsupported is rejected earlier; nothing else reaches this Composite.
                    throw std::runtime_error("internal error: unhandled BloqKind");
            }
        }
        EmitBloqCounts();
        ancilla_cursor.RequireFullyConsumed();
        return EndCircuitDefinition();
    }

private:
    const qpb::BloqLibrary_BloqWithDecomposition& LookupEntry(std::int32_t bloq_id) const {
        const auto it = converter_->GetDag().table.find(bloq_id);
        if (it == converter_->GetDag().table.end()) {
            throw QRETError(
                    error::MalformedQualtranProto,
                    fmt::format("bloq_id {} not found in table", bloq_id)
            );
        }
        return *it->second;
    }
    const qpb::Bloq& LookupBloq(std::int32_t bloq_id) const {
        return LookupEntry(bloq_id).bloq();
    }

    // Ascending instance_id is topological order: Qualtran's BloqBuilder assigns it in add() order.
    std::map<std::int32_t, std::int32_t> BuildBloqIdOf() const {
        auto m = std::map<std::int32_t, std::int32_t>{};
        for (const auto& conn : entry_.decomposition()) {
            for (const auto* soquet : {&conn.left(), &conn.right()}) {
                if (soquet->has_bloq_instance()) {
                    const auto& inst = soquet->bloq_instance();
                    m.emplace(inst.instance_id(), inst.bloq_id());
                }
            }
        }
        return m;
    }

    std::map<SoquetKey, ForwardTarget> BuildForwardEdges() const {
        auto edges = std::map<SoquetKey, ForwardTarget>{};
        for (const auto& conn : entry_.decomposition()) {
            const auto& left = conn.left();
            const auto& right = conn.right();
            if (!left.has_bloq_instance()) {
                continue;  // traces only start at a producer instance's key, never at LeftDangle.
            }
            const auto l_inst = left.bloq_instance().instance_id();
            const auto& l_reg = left.register_().name();
            const auto is_boundary = right.has_dangling_t() && right.dangling_t() == "RightDangle";
            const auto r_inst = right.has_bloq_instance() ? right.bloq_instance().instance_id() : 0;
            const auto& r_reg = right.register_().name();
            ForEachFlatIndex(
                    left,
                    right,
                    [&edges, l_inst, &l_reg, is_boundary, r_inst, &r_reg](
                            std::int32_t l_idx,
                            std::int32_t r_idx
                    ) {
                        edges.emplace(
                                SoquetKey{l_inst, l_reg, l_idx},
                                ForwardTarget{
                                        .is_boundary = is_boundary,
                                        .inst_id = r_inst,
                                        .reg_name = r_reg,
                                        .idx = r_idx
                                }
                        );
                    }
            );
        }
        return edges;
    }

    // A THRU register needs no special case: only AliasOnly and the two terminal kinds do.
    WireTraceResult TraceForward(SoquetKey key) const {
        while (true) {
            const auto bloq_id_it = bloq_id_of_.find(std::get<0>(key));
            if (bloq_id_it == bloq_id_of_.end()) {
                return {};
            }
            const auto& sub_entry = LookupEntry(bloq_id_it->second);
            const auto& sub_bloq = sub_entry.bloq();
            const auto kind = ClassifyBloqKind(sub_entry);
            if (kind == BloqKind::QubitConsumer
                || (sub_bloq.name() == kAndBloqName && !IsAndCompute(sub_bloq))) {
                return {.kind = WireTerminal::QubitConsumer,
                        .consumer = {.bloq_id = bloq_id_it->second, .bloq = &sub_bloq, .depth = 0}};
            }
            auto is_alias_only = (kind == BloqKind::AliasOnly);
            // A wrapper's raw BloqKind hides its effective role on the wire; unwrapping reveals it.
            if (kind == BloqKind::AdjointWrapper) {
                const auto& unwrapped =
                        converter_->GetDag().adjoint_unwraps.at(sub_entry.bloq_id());
                if (IsAliasOnlyBloq(*unwrapped.bloq)) {
                    is_alias_only = true;
                } else if (unwrapped.bloq->name() == kAndBloqName
                           && !UnwrappedAndActsAsCompute(unwrapped)) {
                    return {.kind = WireTerminal::QubitConsumer, .consumer = unwrapped};
                } else if ((IsNewQubitProducerBloq(unwrapped.bloq->name())
                            || IsQubitConsumerBloq(unwrapped.bloq->name()))
                           && !UnwrappedStateEffectActsAsProducer(unwrapped)) {
                    return {.kind = WireTerminal::QubitConsumer, .consumer = unwrapped};
                }
                // Otherwise Gate/Composite: an ordinary THRU pass-through, no special handling.
            } else if (kind == BloqKind::ControlledWrapper) {
                const auto& unwrapped =
                        converter_->GetDag().controlled_unwraps.at(sub_entry.bloq_id());
                if (IsAliasOnlyBloq(*unwrapped.bloq)) {
                    is_alias_only = true;
                } else if (unwrapped.bloq->name() == kAndBloqName
                           && !IsAndCompute(*unwrapped.bloq)) {
                    // Controlled doesn't flip LEFT/RIGHT like Adjoint does, so depth 0 (raw) here.
                    return {.kind = WireTerminal::QubitConsumer,
                            .consumer = {
                                    .bloq_id = unwrapped.bloq_id,
                                    .bloq = unwrapped.bloq,
                                    .depth = 0
                            }};
                }
            }
            if (is_alias_only) {
                const auto remapped =
                        AliasOnlyForwardRemap(sub_entry.bloq(), std::get<1>(key), std::get<2>(key));
                key = {std::get<0>(key), remapped.first, remapped.second};
            }
            const auto edge_it = forward_edges_.find(key);
            if (edge_it == forward_edges_.end()) {
                return {};
            }
            if (edge_it->second.is_boundary) {
                return {.kind = WireTerminal::Boundary,
                        .boundary_reg = edge_it->second.reg_name,
                        .boundary_idx = edge_it->second.idx};
            }
            key = {edge_it->second.inst_id, edge_it->second.reg_name, edge_it->second.idx};
        }
    }

    QubitSourceResolution ResolveNewQubitSource(
            const UnwrappedAdjoint& producer,
            const WireTraceResult& trace,
            bool is_dirty
    ) const {
        if (trace.kind == WireTerminal::Boundary) {
            return {.pool_kind = std::nullopt,
                    .boundary_reg = trace.boundary_reg,
                    .boundary_idx = trace.boundary_idx};
        }
        if (is_dirty) {
            return {.pool_kind = AncillaKind::Dirty};
        }
        if (trace.kind == WireTerminal::QubitConsumer
            && IsCleanTerminalConsumer(producer, trace.consumer)) {
            return {.pool_kind = AncillaKind::Clean};
        }
        return {.pool_kind = AncillaKind::Operate};
    }

    // Resolves every RIGHT-only register on bloq to either a boundary binding or a pool kind.
    void ComputeRightOnlySources(
            const qpb::Bloq& bloq,
            std::int32_t inst_id,
            std::map<SoquetKey, QubitSourceResolution>& sources
    ) const {
        for (const auto& reg : bloq.registers().registers()) {
            if (reg.side() != qpb::Register::RIGHT) {
                continue;
            }
            const auto reg_size = ComputeRegisterSize(reg);
            for (auto k = std::int32_t{0}; k < static_cast<std::int32_t>(reg_size); ++k) {
                const auto key = SoquetKey{inst_id, reg.name(), k};
                const auto trace = TraceForward(key);
                auto resolution = QubitSourceResolution{.pool_kind = AncillaKind::Operate};
                if (trace.kind == WireTerminal::Boundary) {
                    resolution.pool_kind = std::nullopt;
                    resolution.boundary_reg = trace.boundary_reg;
                    resolution.boundary_idx = trace.boundary_idx;
                }
                sources.emplace(key, resolution);
            }
        }
    }

    // Resolves every NewQubitProducer/called-Composite's fresh output to a boundary or pool kind.
    std::map<SoquetKey, QubitSourceResolution> ComputeQubitSources() const {
        auto sources = std::map<SoquetKey, QubitSourceResolution>{};
        for (const auto& [inst_id, bloq_id] : bloq_id_of_) {
            const auto& sub_entry = LookupEntry(bloq_id);
            const auto kind = ClassifyBloqKind(sub_entry);
            if (kind == BloqKind::NewQubitProducer) {
                const auto& bloq = sub_entry.bloq();
                const auto& reg = NewQubitRegister(bloq);
                const auto reg_size = ComputeRegisterSize(reg);
                const auto is_dirty = IsDirtyAllocate(bloq);
                const auto producer =
                        UnwrappedAdjoint{.bloq_id = bloq_id, .bloq = &bloq, .depth = 0};
                for (auto k = std::int32_t{0}; k < static_cast<std::int32_t>(reg_size); ++k) {
                    const auto key = SoquetKey{inst_id, reg.name(), k};
                    sources.emplace(
                            key,
                            ResolveNewQubitSource(producer, TraceForward(key), is_dirty)
                    );
                }
            } else if (kind == BloqKind::Composite) {
                ComputeRightOnlySources(sub_entry.bloq(), inst_id, sources);
            } else if (kind == BloqKind::AdjointWrapper) {
                const auto& unwrapped = converter_->GetDag().adjoint_unwraps.at(bloq_id);
                if (IsAliasOnlyBloq(*unwrapped.bloq)) {
                    continue;  // ExpandAliasWires-equivalent on the wrapping bloq; no fresh qubit.
                }
                if (UnwrapsToGateOrComposite(unwrapped)) {
                    // Adjoint's serialized sides are flip-consistent: same as an ordinary
                    // Composite.
                    ComputeRightOnlySources(sub_entry.bloq(), inst_id, sources);
                    continue;
                }
                // And/State-Effect: a fresh register exists only in the effective producer role.
                const auto& sub_regs = sub_entry.bloq().registers().registers();
                const auto has_right_register = std::ranges::any_of(sub_regs, [](const auto& reg) {
                    return reg.side() == qpb::Register::RIGHT;
                });
                if (!has_right_register) {
                    continue;
                }
                const auto& reg = NewQubitRegister(sub_entry.bloq());
                const auto reg_size = ComputeRegisterSize(reg);
                const auto is_dirty = UnwrappedStateEffectIsDirty(unwrapped);
                for (auto k = std::int32_t{0}; k < static_cast<std::int32_t>(reg_size); ++k) {
                    const auto key = SoquetKey{inst_id, reg.name(), k};
                    sources.emplace(
                            key,
                            ResolveNewQubitSource(unwrapped, TraceForward(key), is_dirty)
                    );
                }
            }
        }
        return sources;
    }

    std::size_t ComputeTotalScratchAncillaCount() const {
        auto seen = std::set<std::int32_t>{};  // a bloq_instance can appear on both sides of a
                                               // Connection
        auto total = std::size_t{0};
        for (const auto& conn : entry_.decomposition()) {
            for (const auto* soquet : {&conn.left(), &conn.right()}) {
                if (!soquet->has_bloq_instance()
                    || !seen.insert(soquet->bloq_instance().instance_id()).second) {
                    continue;
                }
                const auto bloq_id = soquet->bloq_instance().bloq_id();
                const auto kind = ClassifyBloqKind(LookupEntry(bloq_id));
                if (kind == BloqKind::ControlledWrapper) {
                    total += ScratchAncillaCountForControlled(
                            converter_->GetDag().controlled_unwraps.at(bloq_id)
                    );
                } else if (kind == BloqKind::Gate) {
                    // Composite/ControlledWrapper terminals are counted via ComputeAncillaTotals's
                    // forwarding loop instead; only a Gate terminal (no Generator to ask) needs it
                    // here.
                    total += ScratchAncillaCountFor(LookupBloq(bloq_id));
                } else if (kind == BloqKind::AdjointWrapper) {
                    const auto& unwrapped = converter_->GetDag().adjoint_unwraps.at(bloq_id);
                    if (!UnwrapsToCircuitBuiltRole(converter_->GetDag().table, unwrapped)
                        && UnwrapsToGateOrComposite(unwrapped)) {
                        total += ScratchAncillaCountFor(*unwrapped.bloq);
                    }
                }
            }
        }
        return total;
    }

    // Combines qubit_sources_, the scratch count, and every called Composite's forwarded pools.
    std::map<AncillaKind, std::size_t> ComputeAncillaTotals() const {
        auto total = std::map<AncillaKind, std::size_t>{};
        for (const auto& [key, resolution] : qubit_sources_) {
            if (resolution.pool_kind.has_value()) {
                ++total[*resolution.pool_kind];
            }
        }
        if (total_scratch_ancilla_count_ > 0) {
            total[AncillaKind::Clean] += total_scratch_ancilla_count_;
        }
        for (const auto& [inst_id, bloq_id] : bloq_id_of_) {
            // The bloq_id whose Argument this instance forwards ancilla pools from, if any.
            const auto kind = ClassifyBloqKind(LookupEntry(bloq_id));
            auto forwarded_bloq_id = std::optional<std::int32_t>{};
            if (kind == BloqKind::Composite) {
                forwarded_bloq_id = bloq_id;
            } else if (kind == BloqKind::AdjointWrapper) {
                const auto& unwrapped = converter_->GetDag().adjoint_unwraps.at(bloq_id);
                if (UnwrapsToCircuitBuiltRole(converter_->GetDag().table, unwrapped)) {
                    forwarded_bloq_id = unwrapped.bloq_id;
                }
            }
            if (!forwarded_bloq_id.has_value()) {
                continue;
            }
            const auto sub_arg = converter_->GetArgumentOf(*forwarded_bloq_id);
            for (const auto& [pool_name, pool_kind] :
                 {std::pair{"anc_clean", AncillaKind::Clean},
                  std::pair{"anc_operate", AncillaKind::Operate},
                  std::pair{"anc_dirty", AncillaKind::Dirty}}) {
                if (sub_arg.Contains(pool_name)) {
                    total[pool_kind] += sub_arg.GetSize(sub_arg.GetArgIdx(pool_name));
                }
            }
        }
        return total;
    }

    Qubit ResolveOutputQubit(
            const QubitSourceResolution& resolution,
            AncillaCursor& ancilla_cursor
    ) const {
        if (resolution.pool_kind.has_value()) {
            return ancilla_cursor.TakeOne(*resolution.pool_kind);
        }
        // Reuses this Composite's own boundary argument qubit instead of drawing from a pool.
        const auto& boundary_reg = resolution.boundary_reg;
        for (const auto& reg : entry_.bloq().registers().registers()) {
            if (reg.name() != boundary_reg) {
                continue;
            }
            return (ComputeRegisterSize(reg) > 1)
                    ? GetQubits(boundary_reg)[static_cast<std::uint64_t>(resolution.boundary_idx)]
                    : GetQubit(boundary_reg);
        }
        throw QRETError(
                error::MalformedQualtranProto,
                fmt::format("boundary register '{}' not found on this Composite", boundary_reg)
        );
    }

    // Binds the output register into wire_map, then emits whatever gates the specific bloq needs.
    void EmitNewQubitProducer(
            std::int32_t inst_id,
            const qpb::Bloq& bloq,
            WireMap& wire_map,
            AncillaCursor& ancilla_cursor
    ) const {
        const auto& reg = NewQubitRegister(bloq);
        const auto reg_size = ComputeRegisterSize(reg);
        for (auto k = std::int32_t{0}; k < static_cast<std::int32_t>(reg_size); ++k) {
            const auto key = SoquetKey{inst_id, reg.name(), k};
            wire_map.emplace(key, ResolveOutputQubit(qubit_sources_.at(key), ancilla_cursor));
        }
        if (bloq.name() == kAndBloqName) {
            EmitGate(GetBuilder(), bloq, inst_id, wire_map, ancilla_cursor);
            return;
        }
        EmitStateInit(
                bloq.name(),
                IsDirtyAllocate(bloq),
                FlattenRegisterQubits(bloq, qpb::Register::RIGHT, inst_id, wire_map)
        );
    }

    WireMap SeedFromLeftDangle() const {
        auto wire_map = WireMap{};
        for (const auto& conn : entry_.decomposition()) {
            const auto& left = conn.left();
            const auto& right = conn.right();
            if (!left.has_dangling_t() || left.dangling_t() != "LeftDangle") {
                continue;
            }
            if (!right.has_bloq_instance()) {
                continue;
            }
            const auto& reg_name = left.register_().name();
            const auto reg_size = ComputeRegisterSize(left.register_());
            const auto r_inst = right.bloq_instance().instance_id();
            const auto& r_reg = right.register_().name();
            const auto qubits =
                    (reg_size > 1) ? std::make_optional(GetQubits(reg_name)) : std::nullopt;
            ForEachFlatIndex(
                    left,
                    right,
                    [this, &wire_map, &reg_name, &r_reg, r_inst, &qubits](
                            std::int32_t l_idx,
                            std::int32_t r_idx
                    ) {
                        const auto qb = qubits.has_value()
                                ? (*qubits)[static_cast<std::uint64_t>(l_idx)]
                                : GetQubit(reg_name);
                        wire_map.emplace(SoquetKey{r_inst, r_reg, r_idx}, qb);
                    }
            );
        }
        return wire_map;
    }

    // Forwards each incoming edge's already-resolved upstream wire (producers are visited first).
    void ResolveIncomingEdges(std::int32_t inst_id, WireMap& wire_map) const {
        for (const auto& conn : entry_.decomposition()) {
            const auto& left = conn.left();
            const auto& right = conn.right();
            if (!right.has_bloq_instance() || right.bloq_instance().instance_id() != inst_id) {
                continue;
            }
            if (!left.has_bloq_instance()) {
                continue;  // LeftDangle edges are already handled by SeedFromLeftDangle.
            }
            const auto l_inst = left.bloq_instance().instance_id();
            const auto& l_reg = left.register_().name();
            const auto& r_reg = right.register_().name();
            ForEachFlatIndex(
                    left,
                    right,
                    [&wire_map, l_inst, &l_reg, inst_id, &r_reg](
                            std::int32_t l_idx,
                            std::int32_t r_idx
                    ) {
                        const auto it = wire_map.find({l_inst, l_reg, l_idx});
                        if (it == wire_map.end()) {
                            throw QRETError(
                                    error::MalformedQualtranProto,
                                    fmt::format(
                                            "unresolved wire from instance {} register '{}'[{}]",
                                            l_inst,
                                            l_reg,
                                            l_idx
                                    )
                            );
                        }
                        wire_map.emplace(SoquetKey{inst_id, r_reg, r_idx}, it->second);
                    }
            );
        }
    }

    // GlobalPhase, having no registers to wire up, is the only bloq routed here, not decomposition.
    void EmitBloqCounts() const {
        auto decomposition_bloq_ids = std::unordered_set<std::int32_t>{};
        for (const auto& [inst_id, bloq_id] : bloq_id_of_) {
            decomposition_bloq_ids.insert(bloq_id);
        }
        for (const auto& [bloq_id, count] : entry_.bloq_counts()) {
            const auto& sub_bloq = LookupBloq(bloq_id);
            if (sub_bloq.name() != kGlobalPhaseBloqName) {
                // Warn unless already emitted via decomposition (else e.g. ArbitraryClifford).
                if (!decomposition_bloq_ids.contains(bloq_id)) {
                    LOG_WARN(
                            "ignoring bloq_counts entry for '{}' (only GlobalPhase is "
                            "materialized)",
                            sub_bloq.name()
                    );
                }
                continue;
            }
            if (!count.has_int_val()) {
                throw QRETError(
                        error::UnsupportedQualtranBloq,
                        "bloq_counts entry for GlobalPhase has a symbolic count, which is "
                        "unsupported"
                );
            }

            // Emit GlobalPhase.
            auto no_ancilla = AncillaCursor(std::nullopt, std::nullopt, std::nullopt);
            auto empty_wire_map = WireMap{};
            for (auto i = std::int64_t{0}; i < count.int_val(); ++i) {
                EmitGate(GetBuilder(), sub_bloq, 0, empty_wire_map, no_ancilla);
            }
            no_ancilla.RequireFullyConsumed();
        }
    }

    // decomposition_size()==0 counterpart of SetArgument, used when this bloq is itself atomic
    // (a root, or an atomic ControlledWrapper terminal referenced through Converter).
    void SetArgumentForLeaf(Argument& arg) const {
        const auto& bloq = entry_.bloq();
        const auto bloq_id = entry_.bloq_id();
        const auto is_alias_only = IsAliasOnlyBloq(bloq);
        for (const auto& reg : bloq.registers().registers()) {
            if (is_alias_only && reg.side() == qpb::Register::RIGHT) {
                continue;  // a view into LEFT/THRU qubits, not an independent Operate argument.
            }
            arg.Add(reg.name(), Type::Qubit, ComputeRegisterSize(reg), Attribute::Operate);
        }
        if (bloq.name() == kAdjointBloqName) {
            const auto& unwrapped = converter_->GetDag().adjoint_unwraps.at(bloq_id);
            if (UnwrapsToCircuitBuiltRole(converter_->GetDag().table, unwrapped)) {
                const auto sub_arg = converter_->GetArgumentOf(unwrapped.bloq_id);
                for (const auto& [pool_name, pool_kind] :
                     {std::pair{"anc_clean", AncillaKind::Clean},
                      std::pair{"anc_operate", AncillaKind::Operate},
                      std::pair{"anc_dirty", AncillaKind::Dirty}}) {
                    if (sub_arg.Contains(pool_name)) {
                        arg.Add(pool_name,
                                Type::Qubit,
                                sub_arg.GetSize(sub_arg.GetArgIdx(pool_name)),
                                AttributeForAncillaKind(pool_kind));
                    }
                }
            } else if (const auto n = ScratchAncillaCountFor(*unwrapped.bloq); n > 0) {
                // A Gate terminal's own scratch (e.g. CZPowGate's clean ancilla), forwarded the
                // same way ControlledWrapper's own scratch is below.
                arg.Add("anc_clean", Type::Qubit, n, Attribute::CleanAncilla);
            }
            return;
        }
        if (bloq.name() == kControlledBloqName) {
            const auto n = ScratchAncillaCountForControlled(
                    converter_->GetDag().controlled_unwraps.at(bloq_id)
            );
            if (n > 0) {
                arg.Add("anc_clean", Type::Qubit, n, Attribute::CleanAncilla);
            }
            return;
        }
        if (const auto n = ScratchAncillaCountFor(bloq); n > 0) {
            arg.Add("anc_clean", Type::Qubit, n, Attribute::CleanAncilla);
        }
    }

    // decomposition_size()==0 counterpart of Generate, used when this bloq is itself atomic
    // (a root, or an atomic ControlledWrapper terminal referenced through Converter).
    Circuit* GenerateLeaf() const {
        BeginCircuitDefinition();
        const auto& bloq = entry_.bloq();
        const auto bloq_id = entry_.bloq_id();
        if (IsAliasOnlyBloq(bloq)) {
            return EndCircuitDefinition();  // trivial identity, no gate to emit
        }
        // instance_id is fixed at 0: an atomic leaf has no connections of its own to number one.
        auto wire_map = WireMap{};
        for (const auto& reg : bloq.registers().registers()) {
            const auto& reg_name = reg.name();
            const auto reg_size = ComputeRegisterSize(reg);
            if (reg_size > 1) {
                const auto qs = GetQubits(reg_name);
                for (auto idx = std::size_t{0}; idx < reg_size; ++idx) {
                    wire_map.emplace(
                            SoquetKey{0, reg_name, static_cast<std::int32_t>(idx)},
                            qs[idx]
                    );
                }
            } else {
                wire_map.emplace(SoquetKey{0, reg_name, 0}, GetQubit(reg_name));
            }
        }
        if (bloq.name() == kAdjointBloqName) {
            const auto& unwrapped = converter_->GetDag().adjoint_unwraps.at(bloq_id);
            const auto sub_arg_holder =
                    UnwrapsToCircuitBuiltRole(converter_->GetDag().table, unwrapped)
                    ? std::make_optional(converter_->GetArgumentOf(unwrapped.bloq_id))
                    : std::nullopt;
            const auto* sub_arg = sub_arg_holder.has_value() ? &*sub_arg_holder : nullptr;
            // A Gate terminal has no Circuit/Argument to read, so its scratch is read back from
            // this root's own anc_clean argument instead (mirrors SetArgumentForLeaf).
            const auto get_qubits = [this](std::string_view name) { return GetQubits(name); };
            auto ancilla_cursor = (sub_arg != nullptr)
                    ? AncillaCursor::FromArgument(*sub_arg, get_qubits)
                    : AncillaCursor::FromTotals(
                              {{AncillaKind::Clean, ScratchAncillaCountFor(*unwrapped.bloq)}},
                              get_qubits
                      );
            EmitAdjointDispatch(
                    converter_,
                    0,
                    bloq_id,
                    bloq,
                    wire_map,
                    ancilla_cursor,
                    // A root's own registers are already bound above, so read back, not resolved.
                    [&wire_map](const SoquetKey& key) { return wire_map.at(key); }
            );
            ancilla_cursor.RequireFullyConsumed();
            return EndCircuitDefinition();
        }
        if (bloq.name() == kControlledBloqName) {
            const auto n = ScratchAncillaCountForControlled(
                    converter_->GetDag().controlled_unwraps.at(bloq_id)
            );
            auto ancilla_cursor = AncillaCursor::FromTotals(
                    {{AncillaKind::Clean, n}},
                    [this](std::string_view name) { return GetQubits(name); }
            );
            EmitControlledWrapper(
                    GetBuilder(),
                    converter_->GetDag(),
                    0,
                    bloq_id,
                    wire_map,
                    ancilla_cursor
            );
            ancilla_cursor.RequireFullyConsumed();
            return EndCircuitDefinition();
        }
        if (IsNewQubitProducerBloq(bloq.name())) {
            EmitStateInit(
                    bloq.name(),
                    IsDirtyAllocate(bloq),
                    FlattenRegisterQubits(bloq, qpb::Register::RIGHT, 0, wire_map)
            );
            return EndCircuitDefinition();
        }
        if (IsQubitConsumerBloq(bloq.name())) {
            EmitQubitConsumerGates(
                    bloq.name(),
                    IsDirtyFree(bloq),
                    FlattenRegisterQubits(bloq, qpb::Register::LEFT, 0, wire_map)
            );
            return EndCircuitDefinition();
        }
        auto ancilla_cursor = AncillaCursor::FromTotals(
                {{AncillaKind::Clean, ScratchAncillaCountFor(bloq)}},
                [this](std::string_view name) { return GetQubits(name); }
        );
        EmitGate(GetBuilder(), bloq, 0, wire_map, ancilla_cursor);
        ancilla_cursor.RequireFullyConsumed();
        return EndCircuitDefinition();
    }

    std::string name_;
    const qpb::BloqLibrary_BloqWithDecomposition& entry_;
    Converter* converter_;
    std::map<std::int32_t, std::int32_t> bloq_id_of_;
    std::map<SoquetKey, ForwardTarget> forward_edges_;
    std::size_t total_scratch_ancilla_count_;
    std::map<SoquetKey, QubitSourceResolution> qubit_sources_;
    std::map<AncillaKind, std::size_t> ancilla_totals_;
};

// bloq_id is a Composite or an atomic ControlledWrapper terminal; BloqCircuitGen's constructor
// dispatches on entry_.decomposition_size() itself, so both build identically.
CircuitGenerator& Converter::EnsureGenerator(std::int32_t bloq_id) {
    if (const auto it = generator_cache_.find(bloq_id); it != generator_cache_.end()) {
        return *it->second;
    }
    if (!building_.insert(bloq_id).second) {
        throw QRETError(error::MalformedQualtranProto, "circular reference among bloq_id");
    }
    const auto& entry = *dag_.table.at(bloq_id);
    auto generator = std::make_unique<BloqCircuitGen>(
            builder_,
            ResolveCircuitName(dag_.table, entry.bloq()),
            entry,
            this
    );
    building_.erase(bloq_id);
    const auto [it, _] = generator_cache_.emplace(bloq_id, std::move(generator));
    return *it->second;
}

Circuit::Argument Converter::GetArgumentOf(std::int32_t bloq_id) {
    if (const auto cache_it = circuit_cache_.find(bloq_id); cache_it != circuit_cache_.end()) {
        return cache_it->second->GetArgument();
    }
    auto arg = Circuit::Argument{};
    EnsureGenerator(bloq_id).SetArgument(arg);
    return arg;
}

Circuit* Converter::GetOrGenerate(std::int32_t bloq_id) {
    if (const auto cache_it = circuit_cache_.find(bloq_id); cache_it != circuit_cache_.end()) {
        return cache_it->second;
    }
    auto* circuit = EnsureGenerator(bloq_id).Generate();
    circuit_cache_.emplace(bloq_id, circuit);
    return circuit;
}

}  // anonymous namespace

// ─────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────
Circuit* BuildCircuitFromBloqLibrary(
        const qpb::BloqLibrary& lib,
        CircuitBuilder& builder,
        std::string_view entry_name
) {
    const auto dag = BuildBloqDag(lib);
    const auto& root_entry = *dag.table.at(dag.root_bloq_id);
    // Built even for an atomic root: its Gate/Composite branch may need to ask the Converter.
    auto converter = Converter(&builder, dag);
    auto gen = BloqCircuitGen(&builder, entry_name, root_entry, &converter);
    return gen.Generate();
}

Circuit* BuildCircuitFromQualtranJson(
        const std::string& bloq_library_json,
        CircuitBuilder& builder,
        std::string_view entry_name
) {
    const auto lib = qret::qualtran::ParseQualtranJson(bloq_library_json);
    return BuildCircuitFromBloqLibrary(lib, builder, entry_name);
}

}  // namespace qret::frontend
