/**
 * @file qret/parser/qir.cpp
 * @brief QIR (.ll text) parser.
 */

#include "qret/parser/qir.h"

#include <fmt/format.h>

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "qret/error.h"
#include "qret/exception.h"

#ifdef USE_PEGTL
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
#endif
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <tao/pegtl/string_input.hpp>
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#endif

namespace qret::qir {

// Forward declaration
ParsedModule ParseQIRTextImpl(std::string_view text);

bool CanParseQIR() {
#ifdef USE_PEGTL
    return true;
#else
    return false;
#endif
}
ParsedModule ParseQIRText(std::string_view text) {
    if (!CanParseQIR()) {
        throw std::runtime_error("Define USE_PEGTL to parse QIR");
    }
    return ParseQIRTextImpl(text);
}

const std::unordered_map<std::string_view, IntrinsicSignature>& IntrinsicCatalog() {
    constexpr auto arg_q = IntrinsicArg{ArgRole::Qubit, "ptr"};
    constexpr auto arg_rw = IntrinsicArg{ArgRole::RegisterWrite, "ptr writeonly"};
    constexpr auto arg_rro = IntrinsicArg{ArgRole::RegisterRead, "ptr readonly"};
    constexpr auto arg_rr = IntrinsicArg{ArgRole::RegisterRead, "ptr"};
    constexpr auto arg_d = IntrinsicArg{ArgRole::Unclassified, "double"};
    constexpr auto arg_i64 = IntrinsicArg{ArgRole::Unclassified, "i64"};
    constexpr auto arg_i1 = IntrinsicArg{ArgRole::Unclassified, "i1"};
    constexpr auto arg_p = IntrinsicArg{ArgRole::Unclassified, "ptr"};
    // Measurements are irreversible, so their declaration carries the " #1" attribute group.
    static const auto catalog = std::unordered_map<std::string_view, IntrinsicSignature>{
            {kI, {{arg_q}, "void", ""}},
            {kX, {{arg_q}, "void", ""}},
            {kY, {{arg_q}, "void", ""}},
            {kZ, {{arg_q}, "void", ""}},
            {kH, {{arg_q}, "void", ""}},
            {kS, {{arg_q}, "void", ""}},
            {kSAdj, {{arg_q}, "void", ""}},
            {kT, {{arg_q}, "void", ""}},
            {kTAdj, {{arg_q}, "void", ""}},
            {kSXQis, {{arg_q}, "void", ""}},
            {kReset, {{arg_q}, "void", ""}},
            {kRX, {{arg_d, arg_q}, "void", ""}},
            {kRY, {{arg_d, arg_q}, "void", ""}},
            {kRZ, {{arg_d, arg_q}, "void", ""}},
            {kCNot, {{arg_q, arg_q}, "void", ""}},
            {kCXQis, {{arg_q, arg_q}, "void", ""}},
            {kCY, {{arg_q, arg_q}, "void", ""}},
            {kCZ, {{arg_q, arg_q}, "void", ""}},
            {kSwap, {{arg_q, arg_q}, "void", ""}},
            {kRxxQis, {{arg_d, arg_q, arg_q}, "void", ""}},
            {kRyyQis, {{arg_d, arg_q, arg_q}, "void", ""}},
            {kRzzQis, {{arg_d, arg_q, arg_q}, "void", ""}},
            {kCCX, {{arg_q, arg_q, arg_q}, "void", ""}},
            {kCCY, {{arg_q, arg_q, arg_q}, "void", ""}},
            {kCCZ, {{arg_q, arg_q, arg_q}, "void", ""}},
            {kMz, {{arg_q, arg_rw}, "void", " #1"}},
            {kMQis, {{arg_q, arg_rw}, "void", " #1"}},
            {kMResetZQis, {{arg_q, arg_rw}, "void", " #1"}},
            {kReadResultRt, {{arg_rro}, "i1", ""}},
            {kReadResultQis, {{arg_rro}, "i1", ""}},
            {kRtInitialize, {{arg_p}, "void", ""}},
            {kRtResultRecordOutput, {{arg_rr, arg_p}, "void", ""}},
            {kRtBoolRecordOutput, {{arg_i1, arg_p}, "void", ""}},
            {kRtIntRecordOutput, {{arg_i64, arg_p}, "void", ""}},
            {kRtTupleRecordOutput, {{arg_i64, arg_p}, "void", ""}},
            {kRtArrayRecordOutput, {{arg_i64, arg_p}, "void", ""}},
    };
    return catalog;
}

//--------------------------------------------------//
// Implementation of the QIR parser using PEGTL
//--------------------------------------------------//

#ifndef USE_PEGTL

ParsedModule ParseQIRTextImpl(std::string_view /*text*/) {
    throw std::runtime_error("unimplemented");
}

#else  // USE_PEGTL is defined

namespace pegtl = tao::pegtl;
namespace parse_tree = pegtl::parse_tree;

// Everything below is internal to this translation unit; only ParseQIRTextImpl (which the header's
// ParseQIRText forwards to) needs external linkage, so it stays outside this anonymous namespace.
namespace {

//--------------------------------------------------//
// PEG rule
//--------------------------------------------------//

// utils
// LLVM comments (`; ...`) can appear wherever whitespace can, so they're folded into space0/space1.
struct line_comment : pegtl::seq<pegtl::one<';'>, pegtl::until<pegtl::eolf>> {};
struct space0 : pegtl::star<pegtl::sor<pegtl::space, line_comment>> {};
struct space1 : pegtl::plus<pegtl::sor<pegtl::space, line_comment>> {};
struct ident_char : pegtl::sor<pegtl::alnum, pegtl::one<'_', '.'>> {};

// keywords
struct define_kw : pegtl::string<'d', 'e', 'f', 'i', 'n', 'e'> {};
struct void_kw : pegtl::string<'v', 'o', 'i', 'd'> {};
struct i64_kw : pegtl::string<'i', '6', '4'> {};
struct i1_kw : pegtl::string<'i', '1'> {};
// Distinguishes which pointer spelling matched: v1's typed pointers name a parameter's Qubit/
// Register kind directly, v2's opaque `ptr`/`i8*` needs ClassifyParameters to infer it instead.
struct qubit_ptr_ty
    : pegtl::seq<pegtl::one<'%'>, pegtl::string<'Q', 'u', 'b', 'i', 't'>, pegtl::one<'*'>> {};
struct result_ptr_ty
    : pegtl::seq<pegtl::one<'%'>, pegtl::string<'R', 'e', 's', 'u', 'l', 't'>, pegtl::one<'*'>> {};
struct opaque_ptr_ty : pegtl::sor<
                               pegtl::string<'p', 't', 'r'>,
                               pegtl::seq<pegtl::string<'i', '8'>, pegtl::one<'*'>>> {};
struct ptr_kw : pegtl::sor<qubit_ptr_ty, result_ptr_ty, opaque_ptr_ty> {};
struct null_kw : pegtl::string<'n', 'u', 'l', 'l'> {};
struct inttoptr_kw : pegtl::string<'i', 'n', 't', 't', 'o', 'p', 't', 'r'> {};
struct to_kw : pegtl::string<'t', 'o'> {};
struct double_kw : pegtl::string<'d', 'o', 'u', 'b', 'l', 'e'> {};
struct call_kw : pegtl::string<'c', 'a', 'l', 'l'> {};
// LLVM's optional tail-call marker before `call`; carries no meaning here, so ignored.
struct tail_marker_kw : pegtl::sor<
                                pegtl::string<'m', 'u', 's', 't', 't', 'a', 'i', 'l'>,
                                pegtl::string<'n', 'o', 't', 'a', 'i', 'l'>,
                                pegtl::string<'t', 'a', 'i', 'l'>> {};
struct zext_kw : pegtl::string<'z', 'e', 'x', 't'> {};
struct shl_kw : pegtl::string<'s', 'h', 'l'> {};
struct ashr_kw : pegtl::string<'a', 's', 'h', 'r'> {};
struct add_kw : pegtl::string<'a', 'd', 'd'> {};
struct sub_kw : pegtl::string<'s', 'u', 'b'> {};
struct mul_kw : pegtl::string<'m', 'u', 'l'> {};
struct sdiv_kw : pegtl::string<'s', 'd', 'i', 'v'> {};
struct srem_kw : pegtl::string<'s', 'r', 'e', 'm'> {};
struct and_kw : pegtl::string<'a', 'n', 'd'> {};
struct or_kw : pegtl::string<'o', 'r'> {};
struct xor_kw : pegtl::string<'x', 'o', 'r'> {};
struct phi_kw : pegtl::string<'p', 'h', 'i'> {};
struct true_kw : pegtl::string<'t', 'r', 'u', 'e'> {};
struct false_kw : pegtl::string<'f', 'a', 'l', 's', 'e'> {};
// i1/i64 keyword for and/or/xor/icmp/phi; captured as a leaf, unlike the bare i1_kw/i64_kw.
struct bool_or_int_type_kw : pegtl::sor<i1_kw, i64_kw> {};
struct br_kw : pegtl::string<'b', 'r'> {};
struct label_kw : pegtl::string<'l', 'a', 'b', 'e', 'l'> {};
struct ret_kw : pegtl::string<'r', 'e', 't'> {};
struct switch_kw : pegtl::string<'s', 'w', 'i', 't', 'c', 'h'> {};
struct attributes_kw : pegtl::string<'a', 't', 't', 'r', 'i', 'b', 'u', 't', 'e', 's'> {};
// `getelementptr` cast of a `[N x i8]` global string down to `i8*`/`ptr` (qsc's v1 label form).
struct getelementptr_kw
    : pegtl::string<'g', 'e', 't', 'e', 'l', 'e', 'm', 'e', 'n', 't', 'p', 't', 'r'> {};
struct inbounds_kw : pegtl::string<'i', 'n', 'b', 'o', 'u', 'n', 'd', 's'> {};
struct i8_kw : pegtl::string<'i', '8'> {};
struct internal_kw : pegtl::string<'i', 'n', 't', 'e', 'r', 'n', 'a', 'l'> {};
struct constant_kw : pegtl::string<'c', 'o', 'n', 's', 't', 'a', 'n', 't'> {};

// leaves
struct nninteger : pegtl::plus<pegtl::digit> {};
struct local_name : pegtl::seq<pegtl::one<'%'>, pegtl::plus<ident_char>> {};
struct ref_name : pegtl::plus<ident_char> {};
// A decimal integer literal (optionally negative), used by call args and the classical-op chain.
struct int_literal : pegtl::seq<pegtl::opt<pegtl::one<'-'>>, nninteger> {};

// A double literal: decimal, inf/nan, or LLVM's hex form for values that don't round-trip exactly.
struct float_sign : pegtl::one<'+', '-'> {};
struct float_special : pegtl::sor<pegtl::string<'i', 'n', 'f'>, pegtl::string<'n', 'a', 'n'>> {};
struct float_hex : pegtl::seq<pegtl::string<'0', 'x'>, pegtl::rep<16, pegtl::xdigit>> {};
struct float_decimal
    : pegtl::seq<
              pegtl::plus<pegtl::digit>,
              pegtl::opt<pegtl::one<'.'>, pegtl::star<pegtl::digit>>,
              pegtl::opt<pegtl::one<'e', 'E'>, pegtl::opt<float_sign>, pegtl::plus<pegtl::digit>>> {
};
struct float_literal
    : pegtl::seq<pegtl::opt<float_sign>, pegtl::sor<float_hex, float_special, float_decimal>> {};

// quoted string content, for the `attributes` group
struct attr_key : pegtl::star<pegtl::not_one<'"'>> {};
struct attr_value : pegtl::star<pegtl::not_one<'"'>> {};

// A parameter attribute (bare like `writeonly`, or quoted like pyqir's `"writeonly"`); dropped,
// since classification never uses it. The bare form excludes the keywords that begin a pointer
// value.
struct ptr_attr
    : pegtl::sor<
              pegtl::seq<
                      pegtl::not_at<pegtl::sor<null_kw, inttoptr_kw, getelementptr_kw>>,
                      pegtl::plus<pegtl::alpha>>,
              pegtl::seq<pegtl::one<'"'>, pegtl::star<pegtl::not_one<'"'>>, pegtl::one<'"'>>> {};
struct ptr_attrs : pegtl::star<ptr_attr, space1> {};

// operand values shared by call arguments and the zext/shl/or/switch chain
struct at_ref : pegtl::seq<pegtl::one<'@'>, ref_name> {};
struct inttoptr_value : pegtl::seq<
                                inttoptr_kw,
                                space0,
                                pegtl::one<'('>,
                                space0,
                                i64_kw,
                                space1,
                                nninteger,
                                space1,
                                to_kw,
                                space1,
                                ptr_kw,
                                space0,
                                pegtl::one<')'>> {};
// `[N x i8]`, the array type of a string-constant global (e.g. a `*_record_output` label).
struct i8_array_ty : pegtl::seq<
                             pegtl::one<'['>,
                             space0,
                             nninteger,
                             space1,
                             pegtl::one<'x'>,
                             space1,
                             i8_kw,
                             space0,
                             pegtl::one<']'>> {};
// The array-of-i8 pointer type in a `getelementptr`: typed (v1) or opaque `ptr` (v2).
struct i8_array_ptr_ty : pegtl::sor<pegtl::seq<i8_array_ty, pegtl::one<'*'>>, ptr_kw> {};
struct gep_label_value : pegtl::seq<
                                 getelementptr_kw,
                                 space1,
                                 inbounds_kw,
                                 space0,
                                 pegtl::one<'('>,
                                 space0,
                                 i8_array_ty,
                                 space0,
                                 pegtl::one<','>,
                                 space0,
                                 i8_array_ptr_ty,
                                 space1,
                                 at_ref,
                                 space0,
                                 pegtl::one<','>,
                                 space0,
                                 i64_kw,
                                 space1,
                                 nninteger,
                                 space0,
                                 pegtl::one<','>,
                                 space0,
                                 i64_kw,
                                 space1,
                                 nninteger,
                                 space0,
                                 pegtl::one<')'>> {};
struct ptr_value : pegtl::sor<inttoptr_value, gep_label_value, null_kw, at_ref, local_name> {};
struct ptr_arg : pegtl::seq<ptr_kw, space1, ptr_attrs, ptr_value> {};
struct double_arg : pegtl::seq<double_kw, space1, float_literal> {};
// `i64`/`i1` call args: a literal, or an SSA name (e.g. int/bool_record_output's value arg).
struct i64_arg : pegtl::seq<i64_kw, space1, pegtl::sor<local_name, int_literal>> {};
struct i1_arg : pegtl::seq<i1_kw, space1, pegtl::sor<local_name, true_kw, false_kw>> {};
struct call_arg : pegtl::sor<ptr_arg, double_arg, i64_arg, i1_arg> {};
struct call_args : pegtl::opt<pegtl::list<call_arg, pegtl::seq<space0, pegtl::one<','>, space0>>> {
};

// a branch/switch target: "label %name"
struct label_ref : pegtl::seq<label_kw, space1, pegtl::one<'%'>, ref_name> {};

// instructions
struct call_void : pegtl::seq<
                           pegtl::opt<tail_marker_kw, space1>,
                           call_kw,
                           space1,
                           void_kw,
                           space1,
                           at_ref,
                           space0,
                           pegtl::one<'('>,
                           space0,
                           call_args,
                           space0,
                           pegtl::one<')'>> {};
// Shared "%name = " prefix, factored out so assign_inst can commit to it via `must<>` and report
// an unrecognized RHS keyword's position precisely, instead of the whole statement's start.
struct assign_prefix : pegtl::seq<local_name, space0, pegtl::one<'='>, space0> {};
struct call_i1_rhs : pegtl::seq<
                             pegtl::opt<tail_marker_kw, space1>,
                             call_kw,
                             space1,
                             i1_kw,
                             space1,
                             at_ref,
                             space0,
                             pegtl::one<'('>,
                             space0,
                             call_args,
                             space0,
                             pegtl::one<')'>> {};
struct zext_rhs
    : pegtl::seq<zext_kw, space1, i1_kw, space1, local_name, space1, to_kw, space1, i64_kw> {};
// An operand for binary ops/icmp/phi: an SSA name, an int literal, or (i1-typed forms) true/false.
struct value_ref : pegtl::sor<local_name, int_literal, true_kw, false_kw> {};
// i64 binary op `<op> [flags]? i64 %a, <b>`; overflow flags are accepted but ignored.
template <typename OpKw>
struct binary_intonly_op_rhs : pegtl::seq<
                                       OpKw,
                                       pegtl::opt<
                                               space1,
                                               pegtl::sor<
                                                       pegtl::string<'n', 'u', 'w'>,
                                                       pegtl::string<'n', 's', 'w'>,
                                                       pegtl::string<'e', 'x', 'a', 'c', 't'>>>,
                                       space1,
                                       i64_kw,
                                       space1,
                                       value_ref,
                                       space0,
                                       pegtl::one<','>,
                                       space0,
                                       value_ref> {};
// and/or/xor: `<op> (i1|i64) %a, <b>`; both types, no overflow flags.
template <typename OpKw>
struct binary_boolable_op_rhs : pegtl::seq<
                                        OpKw,
                                        space1,
                                        bool_or_int_type_kw,
                                        space1,
                                        value_ref,
                                        space0,
                                        pegtl::one<','>,
                                        space0,
                                        value_ref> {};
using shl_rhs = binary_intonly_op_rhs<shl_kw>;
using ashr_rhs = binary_intonly_op_rhs<ashr_kw>;
using add_rhs = binary_intonly_op_rhs<add_kw>;
using sub_rhs = binary_intonly_op_rhs<sub_kw>;
using mul_rhs = binary_intonly_op_rhs<mul_kw>;
using sdiv_rhs = binary_intonly_op_rhs<sdiv_kw>;
using srem_rhs = binary_intonly_op_rhs<srem_kw>;
using and_rhs = binary_boolable_op_rhs<and_kw>;
using or_rhs = binary_boolable_op_rhs<or_kw>;
using xor_rhs = binary_boolable_op_rhs<xor_kw>;
// `icmp`, a candidate branch-condition source; only signed predicates are accepted.
struct icmp_kw : pegtl::string<'i', 'c', 'm', 'p'> {};
struct icmp_pred_kw : pegtl::sor<
                              pegtl::string<'s', 'g', 't'>,
                              pegtl::string<'s', 'g', 'e'>,
                              pegtl::string<'s', 'l', 't'>,
                              pegtl::string<'s', 'l', 'e'>,
                              pegtl::string<'e', 'q'>,
                              pegtl::string<'n', 'e'>> {};
struct icmp_rhs : pegtl::seq<
                          icmp_kw,
                          space1,
                          icmp_pred_kw,
                          space1,
                          bool_or_int_type_kw,
                          space1,
                          local_name,
                          space0,
                          pegtl::one<','>,
                          space0,
                          value_ref> {};
// A `phi`; any edge count parses (the 2-edge restriction is enforced later, not here).
struct phi_incoming : pegtl::seq<
                              pegtl::one<'['>,
                              space0,
                              value_ref,
                              space0,
                              pegtl::one<','>,
                              space0,
                              pegtl::one<'%'>,
                              ref_name,
                              space0,
                              pegtl::one<']'>> {};
struct phi_rhs : pegtl::seq<
                         phi_kw,
                         space1,
                         bool_or_int_type_kw,
                         space1,
                         phi_incoming,
                         space0,
                         pegtl::star<pegtl::one<','>, space0, phi_incoming, space0>> {};
// `select i1 <cond>, <ty> <true>, <ty> <false>`, the shape Mux's array read produces.
struct select_kw : pegtl::string<'s', 'e', 'l', 'e', 'c', 't'> {};
struct select_rhs : pegtl::seq<
                            select_kw,
                            space1,
                            i1_kw,
                            space1,
                            value_ref,
                            space0,
                            pegtl::one<','>,
                            space0,
                            bool_or_int_type_kw,
                            space1,
                            value_ref,
                            space0,
                            pegtl::one<','>,
                            space0,
                            bool_or_int_type_kw,
                            space1,
                            value_ref> {};
// The recognized RHS keywords for an assignment; see assign_prefix above for why this is separate.
struct assign_rhs : pegtl::sor<
                            call_i1_rhs,
                            zext_rhs,
                            shl_rhs,
                            ashr_rhs,
                            add_rhs,
                            sub_rhs,
                            mul_rhs,
                            sdiv_rhs,
                            srem_rhs,
                            and_rhs,
                            or_rhs,
                            xor_rhs,
                            icmp_rhs,
                            phi_rhs,
                            select_rhs> {};
struct assign_inst : pegtl::seq<assign_prefix, pegtl::must<assign_rhs>> {};
struct instruction : pegtl::sor<call_void, assign_inst> {};

// terminators
struct br_uncond : pegtl::seq<br_kw, space1, label_ref> {};
struct br_cond : pegtl::seq<
                         br_kw,
                         space1,
                         i1_kw,
                         space1,
                         local_name,
                         space0,
                         pegtl::one<','>,
                         space0,
                         label_ref,
                         space0,
                         pegtl::one<','>,
                         space0,
                         label_ref> {};
struct ret_inst : pegtl::sor<
                          pegtl::seq<ret_kw, space1, i64_kw, space1, pegtl::one<'0'>>,
                          pegtl::seq<ret_kw, space1, void_kw>> {};
struct switch_case
    : pegtl::seq<i64_kw, space1, nninteger, space0, pegtl::one<','>, space0, label_ref> {};
struct switch_inst : pegtl::seq<
                             switch_kw,
                             space1,
                             i64_kw,
                             space1,
                             local_name,
                             space0,
                             pegtl::one<','>,
                             space0,
                             label_ref,
                             space0,
                             pegtl::one<'['>,
                             space0,
                             pegtl::star<switch_case, space0>,
                             pegtl::one<']'>> {};
struct terminator : pegtl::sor<br_cond, br_uncond, ret_inst, switch_inst> {};

// A label + body + terminator; wrapped in `must<>` so failures report their real position.
struct basic_block : pegtl::seq<
                             ref_name,
                             space0,
                             pegtl::one<':'>,
                             space0,
                             pegtl::must<pegtl::star<instruction, space0>, terminator>> {};

// function definition
struct function_param : pegtl::seq<ptr_kw, space1, ptr_attrs, local_name> {};
struct param_list
    : pegtl::opt<pegtl::list<function_param, pegtl::seq<space0, pegtl::one<','>, space0>>> {};
struct attr_group_ref : pegtl::seq<pegtl::one<'#'>, nninteger> {};
struct return_type : pegtl::sor<i64_kw, void_kw> {};
// Wrapped in `must<>` so a malformed function body is a hard error, not swallowed by skip_line.
struct function_def : pegtl::seq<
                              define_kw,
                              pegtl::must<
                                      space1,
                                      // pyqir emits the `internal` linkage keyword on its
                                      // sub-circuits (`define internal void @sub(...)`); other
                                      // observed output omits it.
                                      pegtl::opt<internal_kw, space1>,
                                      return_type,
                                      space1,
                                      at_ref,
                                      space0,
                                      pegtl::one<'('>,
                                      space0,
                                      param_list,
                                      space0,
                                      pegtl::one<')'>,
                                      pegtl::opt<space0, attr_group_ref>,
                                      space0,
                                      pegtl::one<'{'>,
                                      space0,
                                      pegtl::star<basic_block, space0>,
                                      pegtl::one<'}'>>> {};

// A quoted string attribute (`"key"` optionally `="value"`); the only kind this parser reads.
struct attr_item : pegtl::seq<
                           pegtl::one<'"'>,
                           attr_key,
                           pegtl::one<'"'>,
                           pegtl::opt<
                                   space0,
                                   pegtl::one<'='>,
                                   space0,
                                   pegtl::one<'"'>,
                                   attr_value,
                                   pegtl::one<'"'>>> {};
// A bare LLVM enum attribute (`nounwind`, `align(8)`), accepted and dropped.
struct enum_attr
    : pegtl::seq<
              pegtl::plus<pegtl::sor<pegtl::alnum, pegtl::one<'_', '-'>>>,
              pegtl::opt<pegtl::one<'('>, pegtl::star<pegtl::not_one<')'>>, pegtl::one<')'>>> {};
struct attr_list : pegtl::opt<pegtl::list<pegtl::sor<attr_item, enum_attr>, space1>> {};
// `attributes #N = {...}`, read for "required_num_qubits"/"entry_point". Wrapped in `must<>` so a
// malformed group is a hard error, not swallowed by skip_line.
struct attr_decl : pegtl::seq<
                           attributes_kw,
                           pegtl::must<
                                   space1,
                                   pegtl::one<'#'>,
                                   nninteger,
                                   space0,
                                   pegtl::one<'='>,
                                   space0,
                                   pegtl::one<'{'>,
                                   space0,
                                   attr_list,
                                   space0,
                                   pegtl::one<'}'>>> {};

// LLVM's C-string-constant escaping: `\XX` (two hex digits) for one byte, anything else verbatim.
struct str_escape : pegtl::seq<pegtl::one<'\\'>, pegtl::xdigit, pegtl::xdigit> {};
struct c_string_content : pegtl::star<pegtl::sor<str_escape, pegtl::not_one<'"', '\\'>>> {};
struct c_string_literal
    : pegtl::seq<pegtl::one<'c'>, pegtl::one<'"'>, c_string_content, pegtl::one<'"'>> {};
// `@N = internal constant [...] c"..."`: a record_output label global; parsed only for its text.
struct global_str_decl : pegtl::seq<
                                 at_ref,
                                 space0,
                                 pegtl::one<'='>,
                                 space0,
                                 internal_kw,
                                 space1,
                                 constant_kw,
                                 space1,
                                 i8_array_ty,
                                 space1,
                                 c_string_literal> {};

// a line this parser has no dedicated grammar for; consumed whole and ignored.
struct skip_line : pegtl::seq<pegtl::not_at<pegtl::eof>, pegtl::until<pegtl::eolf>> {};
struct top_level_item : pegtl::sor<function_def, attr_decl, global_str_decl, skip_line> {};
struct module_body : pegtl::star<space0, top_level_item> {};

// grammar
struct grammar : pegtl::must<module_body, space0, pegtl::eof> {};

template <typename Rule>
using selector = parse_tree::selector<
        Rule,
        parse_tree::store_content::on<
                nninteger,
                local_name,
                ref_name,
                float_literal,
                attr_key,
                attr_value,
                icmp_pred_kw,
                int_literal,
                bool_or_int_type_kw,
                true_kw,
                false_kw,
                c_string_content>,
        parse_tree::remove_content::on<
                qubit_ptr_ty,
                result_ptr_ty,
                opaque_ptr_ty,
                ptr_arg,
                double_arg,
                i64_arg,
                i1_arg,
                call_void,
                call_i1_rhs,
                zext_rhs,
                shl_rhs,
                ashr_rhs,
                add_rhs,
                sub_rhs,
                mul_rhs,
                sdiv_rhs,
                srem_rhs,
                and_rhs,
                or_rhs,
                xor_rhs,
                icmp_rhs,
                phi_incoming,
                phi_rhs,
                select_rhs,
                assign_inst,
                br_uncond,
                br_cond,
                ret_inst,
                switch_case,
                switch_inst,
                basic_block,
                function_param,
                function_def,
                attr_item,
                attr_decl,
                global_str_decl,
                grammar>>;

// Position is exact only at must<>-wrapped points (basic_block/function_def/grammar); other
// failures may report an earlier position.
// Named `error_messages`, not `error`, to avoid shadowing the `qret::error::*` namespace below.
struct error_messages {
    template <typename>
    static constexpr bool raise_on_failure = false;

    template <typename>
    static constexpr auto message = "unsupported or malformed QIR syntax";
};
// The only rules that can actually raise at each must<> above; everything else keeps the generic
// message.
template <>
constexpr auto error_messages::message<terminator> =
        "expected a recognized instruction or a block terminator (br/ret/switch); this may also be "
        "a valid but unsupported instruction (e.g. store, or another instruction with no result)";
// assign_inst commits after "%name = ", so this points right at the unrecognized RHS keyword.
template <>
constexpr auto error_messages::message<assign_rhs> =
        "expected a recognized instruction after '='; this may also be a valid but unsupported "
        "instruction (e.g. alloca, load, or a floating-point instruction)";
template <>
constexpr auto error_messages::message<return_type> =
        "expected a function return type (i64 or void)";
template <>
constexpr auto error_messages::message<at_ref> = "expected a function name ('@name')";
template <>
constexpr auto error_messages::message<pegtl::eof> = "unexpected content after the module body";
template <typename Rule>
using control_with_messages = pegtl::must_if<error_messages>::control<Rule>;

//--------------------------------------------------//
// From the parse tree to ParsedModule
//--------------------------------------------------//

using ParseNode = std::unique_ptr<parse_tree::node>;

template <typename Rule>
bool HasChild(const ParseNode& n) {
    for (const auto& child : n->children) {
        if (child->is_type<Rule>()) {
            return true;
        }
    }
    return false;
}
template <typename Rule>
const ParseNode* FindChild(const ParseNode& n) {
    for (const auto& child : n->children) {
        if (child->is_type<Rule>()) {
            return &child;
        }
    }
    return nullptr;
}

// Reconstructs the ptr value text as the exporter emits it ("null"/"%name"/"@name"/"inttoptr").
// ref_name is checked first since gep_label_value carries both, and must normalize like "@name".
std::string ReconstructPtrValue(const ParseNode& n) {
    if (const auto* global_ref = FindChild<ref_name>(n)) {
        return fmt::format("@{}", (*global_ref)->string());
    }
    if (const auto* literal = FindChild<nninteger>(n)) {
        return fmt::format("inttoptr (i64 {} to ptr)", (*literal)->string());
    }
    if (const auto* var = FindChild<local_name>(n)) {
        return (*var)->string();
    }
    return "null";
}
// Reconstructs one call argument's text, matching the exporter's single-space form exactly.
std::string ParseCallArg(const ParseNode& n) {
    if (n->is_type<ptr_arg>()) {
        // Attributes (writeonly, readonly, ...) never affect classification, so they're dropped.
        return fmt::format("ptr {}", ReconstructPtrValue(n));
    }
    if (n->is_type<double_arg>()) {
        return fmt::format("double {}", n->children.at(0)->string());
    }
    if (n->is_type<i64_arg>()) {
        return fmt::format("i64 {}", n->children.at(0)->string());
    }
    if (!n->is_type<i1_arg>()) {
        throw QRETError(error::MalformedQIR, "expected a ptr/double/i64/i1 typed call argument");
    }
    return fmt::format("i1 {}", n->children.at(0)->string());
}
std::vector<std::string> ParseCallArgs(const ParseNode& n, std::size_t start) {
    auto args = std::vector<std::string>();
    args.reserve(n->children.size() - start);
    for (auto i = start; i < n->children.size(); ++i) {
        args.emplace_back(ParseCallArg(n->children[i]));
    }
    return args;
}
ParsedCall ParseCallVoid(const ParseNode& n) {
    auto call = ParsedCall();
    call.function = n->children.at(0)->string();
    call.args = ParseCallArgs(n, 1);
    return call;
}
ParsedCall ParseCallI1(const std::string& result_var, const ParseNode& n) {
    auto call = ParsedCall();
    call.result_var = result_var;
    call.function = n->children.at(0)->string();
    call.args = ParseCallArgs(n, 1);
    return call;
}
ParsedIntInst ParseZext(const std::string& result_var, const ParseNode& n) {
    return ParsedIntInst{
            ParsedIntInst::Kind::Zext,
            result_var,
            n->children.at(0)->string(),
            "",
            false
    };
}
// shl/ashr/add/sub/mul/sdiv/srem: children [operand0, operand1], always i64-typed.
ParsedIntInst
ParseIntOnlyInst(ParsedIntInst::Kind kind, const std::string& result_var, const ParseNode& n) {
    return ParsedIntInst{
            kind,
            result_var,
            n->children.at(0)->string(),
            n->children.at(1)->string(),
            false
    };
}
// and/or/xor: children [type, operand0, operand1], type is "i1" or "i64".
ParsedIntInst
ParseBoolableInst(ParsedIntInst::Kind kind, const std::string& result_var, const ParseNode& n) {
    return ParsedIntInst{
            kind,
            result_var,
            n->children.at(1)->string(),
            n->children.at(2)->string(),
            n->children.at(0)->string() == "i1"
    };
}
ParsedIcmpInst::Predicate ParseIcmpPredicate(const std::string& text) {
    if (text == "eq") {
        return ParsedIcmpInst::Predicate::Eq;
    }
    if (text == "ne") {
        return ParsedIcmpInst::Predicate::Ne;
    }
    if (text == "slt") {
        return ParsedIcmpInst::Predicate::Slt;
    }
    if (text == "sle") {
        return ParsedIcmpInst::Predicate::Sle;
    }
    if (text == "sgt") {
        return ParsedIcmpInst::Predicate::Sgt;
    }
    if (text == "sge") {
        return ParsedIcmpInst::Predicate::Sge;
    }
    throw QRETError(error::MalformedQIR, fmt::format("unknown icmp predicate '{}'", text));
}
ParsedIcmpInst ParseIcmp(const std::string& result_var, const ParseNode& n) {
    return ParsedIcmpInst{
            ParseIcmpPredicate(n->children.at(0)->string()),
            result_var,
            n->children.at(2)->string(),
            n->children.at(3)->string(),
            n->children.at(1)->string() == "i1"
    };
}
// `block_name`, already known by ParseBasicBlock, lets the frontend map predecessors to True/False.
ParsedPhi
ParsePhi(const std::string& result_var, const ParseNode& n, const std::string& block_name) {
    auto phi = ParsedPhi();
    phi.result_var = result_var;
    phi.block_name = block_name;
    phi.is_bool_typed = n->children.at(0)->string() == "i1";
    for (auto i = std::size_t{1}; i < n->children.size(); ++i) {
        const auto& incoming = n->children[i];
        phi.incoming.emplace_back(
                incoming->children.at(0)->string(),
                incoming->children.at(1)->string()
        );
    }
    return phi;
}
// Children: [0]=cond, [1..2]=true type/value, [3..4]=false type/value.
ParsedSelectInst ParseSelect(const std::string& result_var, const ParseNode& n) {
    auto op = ParsedSelectInst();
    op.result_var = result_var;
    op.cond = n->children.at(0)->string();
    op.is_bool_typed = n->children.at(1)->string() == "i1";
    op.true_value = n->children.at(2)->string();
    op.false_value = n->children.at(4)->string();
    return op;
}
ParsedBlock ParseBasicBlock(const ParseNode& n) {
    auto block = ParsedBlock();
    block.name = n->children.at(0)->string();
    for (auto i = std::size_t{1}; i < n->children.size(); ++i) {
        const auto& child = n->children[i];
        if (child->is_type<call_void>()) {
            block.items.emplace_back(ParseCallVoid(child));
        } else if (child->is_type<assign_inst>()) {
            // children: [0] = result_var (from assign_prefix), [1] = the matched RHS node.
            const auto& result_var = child->children.at(0)->string();
            const auto& rhs = child->children.at(1);
            if (rhs->is_type<call_i1_rhs>()) {
                block.items.emplace_back(ParseCallI1(result_var, rhs));
            } else if (rhs->is_type<zext_rhs>()) {
                block.items.emplace_back(ParseZext(result_var, rhs));
            } else if (rhs->is_type<shl_rhs>()) {
                block.items.emplace_back(
                        ParseIntOnlyInst(ParsedIntInst::Kind::Shl, result_var, rhs)
                );
            } else if (rhs->is_type<ashr_rhs>()) {
                block.items.emplace_back(
                        ParseIntOnlyInst(ParsedIntInst::Kind::Ashr, result_var, rhs)
                );
            } else if (rhs->is_type<add_rhs>()) {
                block.items.emplace_back(
                        ParseIntOnlyInst(ParsedIntInst::Kind::Add, result_var, rhs)
                );
            } else if (rhs->is_type<sub_rhs>()) {
                block.items.emplace_back(
                        ParseIntOnlyInst(ParsedIntInst::Kind::Sub, result_var, rhs)
                );
            } else if (rhs->is_type<mul_rhs>()) {
                block.items.emplace_back(
                        ParseIntOnlyInst(ParsedIntInst::Kind::Mul, result_var, rhs)
                );
            } else if (rhs->is_type<sdiv_rhs>()) {
                block.items.emplace_back(
                        ParseIntOnlyInst(ParsedIntInst::Kind::Sdiv, result_var, rhs)
                );
            } else if (rhs->is_type<srem_rhs>()) {
                block.items.emplace_back(
                        ParseIntOnlyInst(ParsedIntInst::Kind::Srem, result_var, rhs)
                );
            } else if (rhs->is_type<and_rhs>()) {
                block.items.emplace_back(
                        ParseBoolableInst(ParsedIntInst::Kind::And, result_var, rhs)
                );
            } else if (rhs->is_type<or_rhs>()) {
                block.items.emplace_back(
                        ParseBoolableInst(ParsedIntInst::Kind::Or, result_var, rhs)
                );
            } else if (rhs->is_type<xor_rhs>()) {
                block.items.emplace_back(
                        ParseBoolableInst(ParsedIntInst::Kind::Xor, result_var, rhs)
                );
            } else if (rhs->is_type<icmp_rhs>()) {
                block.items.emplace_back(ParseIcmp(result_var, rhs));
            } else if (rhs->is_type<phi_rhs>()) {
                block.items.emplace_back(ParsePhi(result_var, rhs, block.name));
            } else if (rhs->is_type<select_rhs>()) {
                block.items.emplace_back(ParseSelect(result_var, rhs));
            }
        } else if (child->is_type<br_uncond>()) {
            block.term = ParsedBlock::Term::BrUncond;
            block.target_true = child->children.at(0)->string();
        } else if (child->is_type<br_cond>()) {
            block.term = ParsedBlock::Term::BrCond;
            block.cond_var = child->children.at(0)->string();
            block.target_true = child->children.at(1)->string();
            block.target_false = child->children.at(2)->string();
        } else if (child->is_type<ret_inst>()) {
            block.term = ParsedBlock::Term::Return;
        } else if (child->is_type<switch_inst>()) {
            block.term = ParsedBlock::Term::Switch;
            block.switch_value = child->children.at(0)->string();
            block.switch_default = child->children.at(1)->string();
            for (auto j = std::size_t{2}; j < child->children.size(); ++j) {
                const auto& case_node = child->children[j];
                block.switch_cases.emplace_back(
                        std::stoull(case_node->children.at(0)->string()),
                        case_node->children.at(1)->string()
                );
            }
        }
    }
    return block;
}
// v1's typed pointer already answers Qubit-vs-Register; v2's opaque `ptr`/`i8*` is resolved later
// by ClassifyParameters, from how the parameter is used in its own function's body.
ParsedParam ParseFunctionParam(const ParseNode& n) {
    auto declared_type = DeclaredPtrType::Opaque;
    if (HasChild<qubit_ptr_ty>(n)) {
        declared_type = DeclaredPtrType::Qubit;
    } else if (HasChild<result_ptr_ty>(n)) {
        declared_type = DeclaredPtrType::Result;
    }
    const auto* name_node = FindChild<local_name>(n);
    if (name_node == nullptr) {
        throw QRETError(error::MalformedQIR, "function parameter is missing a name");
    }
    return {(*name_node)->string(), declared_type, std::nullopt};
}
ParsedFunction ParseFunctionDef(const ParseNode& n) {
    auto func = ParsedFunction();
    for (const auto& child : n->children) {
        if (child->is_type<ref_name>()) {
            func.name = child->string();
        } else if (child->is_type<function_param>()) {
            func.raw_params.emplace_back(ParseFunctionParam(child));
        } else if (child->is_type<nninteger>()) {
            func.attr_group = static_cast<std::uint32_t>(std::stoul(child->string()));
        } else if (child->is_type<basic_block>()) {
            func.blocks.emplace_back(ParseBasicBlock(child));
        }
    }
    return func;
}
// Decodes str_escape's "\XX" sequences to bytes, then strips qsc's trailing "\00" terminator.
std::string DecodeCString(std::string_view raw) {
    auto out = std::string();
    out.reserve(raw.size());
    for (std::size_t i = 0; i < raw.size();) {
        if (raw[i] == '\\' && i + 3 <= raw.size()) {
            out.push_back(
                    static_cast<char>(std::stoul(std::string(raw.substr(i + 1, 2)), nullptr, 16))
            );
            i += 3;
        } else {
            out.push_back(raw[i]);
            ++i;
        }
    }
    if (!out.empty() && out.back() == '\0') {
        out.pop_back();
    }
    return out;
}
ParsedModule ParseModuleNode(const ParseNode& n) {
    auto parsed = ParsedModule();
    for (const auto& child : n->children) {
        if (child->is_type<function_def>()) {
            parsed.functions.emplace_back(ParseFunctionDef(child));
            continue;
        }
        if (child->is_type<global_str_decl>()) {
            const auto& index = (*FindChild<ref_name>(child))->string();
            const auto& content = (*FindChild<c_string_content>(child))->string();
            parsed.global_string_labels[index] = DecodeCString(content);
            continue;
        }
        if (!child->is_type<attr_decl>()) {
            continue;
        }
        const auto group_id =
                static_cast<std::uint32_t>(std::stoul(child->children.at(0)->string()));
        for (auto i = std::size_t{1}; i < child->children.size(); ++i) {
            const auto& item = child->children[i];
            const auto& key = item->children.at(0)->string();
            if (item->children.size() > 1 && key == kAttrRequiredNumQubits) {
                parsed.attr_group_required_qubits[group_id] =
                        std::stoull(item->children.at(1)->string());
            } else if (key == kAttrEntryPoint) {
                parsed.attr_group_entry_point.insert(group_id);
            }
        }
    }
    return parsed;
}

// A callee's role for `arg_index`: a known QIS/RT intrinsic's fixed signature (from the shared
// IntrinsicCatalog), or (for a module-local sub-circuit) its own already-classified parameter kind.
std::optional<ArgRole> LookupArgRole(
        std::string_view callee_name,
        std::size_t arg_index,
        const std::unordered_map<std::string, ParsedFunction*>& by_name
) {
    if (const auto it = by_name.find(std::string(callee_name)); it != by_name.end()) {
        const auto& params = it->second->raw_params;
        if (arg_index >= params.size() || !params[arg_index].kind) {
            return std::nullopt;
        }
        switch (*params[arg_index].kind) {
            case ParamKind::Qubit:
                return ArgRole::Qubit;
            case ParamKind::RegisterInput:
                return ArgRole::RegisterRead;
            case ParamKind::RegisterOutput:
                return ArgRole::RegisterWrite;
            default:
                throw QRETError(error::MalformedQIR, "unresolved sub-circuit parameter kind");
        }
    }
    const auto& catalog = IntrinsicCatalog();
    const auto it = catalog.find(callee_name);
    if (it == catalog.end() || arg_index >= it->second.args.size()) {
        return std::nullopt;
    }
    return it->second.args[arg_index].role;
}
// The role of `param_name`'s first occurrence as a call argument in `func`'s body, in text order;
// occurrences that settle nothing (Unclassified, or an unrecognized callee) are skipped over.
std::optional<ArgRole> FindFirstUsageRole(
        const ParsedFunction& func,
        std::string_view param_name,
        const std::unordered_map<std::string, ParsedFunction*>& by_name
) {
    for (const auto& block : func.blocks) {
        for (const auto& item : block.items) {
            const auto* call = std::get_if<ParsedCall>(&item);
            if (call == nullptr) {
                continue;
            }
            for (auto i = std::size_t{0}; i < call->args.size(); ++i) {
                if (!call->args[i].ends_with(param_name)) {
                    continue;
                }
                const auto role = LookupArgRole(call->function, i, by_name);
                if (role && *role != ArgRole::Unclassified) {
                    return role;
                }
            }
        }
    }
    return std::nullopt;
}
// Resolves every one of `func`'s own parameters into a ParamKind and derives qubit_params/
// register_params from it. `by_name`'s entries for functions `func` calls must already be
// classified (ResolveParameters visits callees before their callers).
void ClassifyOwnParameters(
        ParsedFunction& func,
        const std::unordered_map<std::string, ParsedFunction*>& by_name
) {
    for (auto& param : func.raw_params) {
        if (param.declared_type == DeclaredPtrType::Qubit) {
            param.kind = ParamKind::Qubit;
            continue;
        }
        const auto role = FindFirstUsageRole(func, param.name, by_name);
        if (!role) {
            throw QRETError(
                    error::MalformedQIR,
                    fmt::format(
                            "parameter '{}' of function '{}' is never used in a way that "
                            "determines whether it's a Qubit or a Register",
                            param.name,
                            func.name
                    )
            );
        }
        if (*role == ArgRole::Qubit) {
            if (param.declared_type == DeclaredPtrType::Result) {
                throw QRETError(
                        error::MalformedQIR,
                        fmt::format(
                                "parameter '{}' of function '{}' is declared %Result* but used "
                                "as a Qubit",
                                param.name,
                                func.name
                        )
                );
            }
            param.kind = ParamKind::Qubit;
        } else if (*role == ArgRole::RegisterRead) {
            param.kind = ParamKind::RegisterInput;
        } else {
            param.kind = ParamKind::RegisterOutput;
        }
    }
    for (const auto& param : func.raw_params) {
        switch (*param.kind) {
            case ParamKind::Qubit:
                func.qubit_params.emplace_back(param.name);
                break;
            case ParamKind::RegisterInput:
                func.register_params.emplace_back(param.name, false);
                break;
            case ParamKind::RegisterOutput:
                func.register_params.emplace_back(param.name, true);
                break;
            default:
                throw QRETError(error::MalformedQIR, "unresolved sub-circuit parameter kind");
        }
    }
}
// A parameter can be passed straight through to another sub-circuit without ever reaching a known
// QIS/RT call directly (e.g. an OpenQASM `gate` nesting another `gate`), so callees are classified
// before their callers; `done` memoizes a function shared by multiple call sites.
void ResolveParameters(
        ParsedFunction* func,
        const std::unordered_map<std::string, ParsedFunction*>& by_name,
        std::unordered_set<ParsedFunction*>& done
) {
    if (!done.insert(func).second) {
        return;
    }
    for (const auto& block : func->blocks) {
        for (const auto& item : block.items) {
            const auto* call = std::get_if<ParsedCall>(&item);
            if (call == nullptr) {
                continue;
            }
            if (const auto it = by_name.find(call->function); it != by_name.end()) {
                ResolveParameters(it->second, by_name, done);
            }
        }
    }
    ClassifyOwnParameters(*func, by_name);
}
// The sole acyclic check for a parsed QIR module: every ParsedModule this parser hands out is
// guaranteed acyclic, so frontend's recursive circuit generation doesn't need its own check.
void ClassifyParameters(ParsedModule& module) {
    auto by_name = std::unordered_map<std::string, ParsedFunction*>();
    for (auto& func : module.functions) {
        by_name[func.name] = &func;
    }

    auto all_nodes = std::vector<const ParsedFunction*>();
    auto callees = std::unordered_map<const ParsedFunction*, std::vector<const ParsedFunction*>>();
    for (const auto& func : module.functions) {
        all_nodes.emplace_back(&func);
        for (const auto& block : func.blocks) {
            for (const auto& item : block.items) {
                const auto* call = std::get_if<ParsedCall>(&item);
                if (call == nullptr) {
                    continue;
                }
                if (const auto it = by_name.find(call->function); it != by_name.end()) {
                    callees[&func].emplace_back(it->second);
                }
            }
        }
    }
    CheckCallGraphIsAcyclic<ParsedFunction>(all_nodes, callees, [](const ParsedFunction* f) {
        return f->name;
    });

    auto done = std::unordered_set<ParsedFunction*>();
    for (auto& func : module.functions) {
        ResolveParameters(&func, by_name, done);
    }
}

// The source line containing byte offset `pos`, for display alongside a parse error.
std::string_view LineContaining(std::string_view text, std::size_t byte_offset) {
    const auto prev_newline = text.rfind('\n', byte_offset == 0 ? 0 : byte_offset - 1);
    const auto line_start = prev_newline == std::string_view::npos ? 0 : prev_newline + 1;
    const auto next_newline = text.find('\n', byte_offset);
    const auto line_end = next_newline == std::string_view::npos ? text.size() : next_newline;
    return text.substr(line_start, line_end - line_start);
}

}  // namespace

ParsedModule ParseQIRTextImpl(std::string_view text) {
    try {
        auto in = pegtl::string_input(text, "qir");
        const auto root =
                pegtl::parse_tree::parse<grammar, selector, pegtl::nothing, control_with_messages>(
                        in
                );
        // Real syntax failures surface as pegtl::parse_error below (with a line/column); reaching
        // here means an internal invariant broke (parse() returned without a grammar root).
        if (!root || root->children.empty() || !root->children.front()->is_type<grammar>()) {
            throw QRETError(
                    error::MalformedQIR,
                    "internal error: QIR parser produced no parse tree"
            );
        }
        auto module = ParseModuleNode(root->children.front());
        ClassifyParameters(module);
        return module;
    } catch (const pegtl::parse_error& ex) {
        // what()'s "qir:L:C: msg" label isn't a filename; reformat using pegtl's own position.
        if (!ex.positions().empty()) {
            const auto& pos = ex.positions().front();
            const auto caret = std::string(pos.column > 0 ? pos.column - 1 : 0, ' ') + "^";
            throw QRETError(
                    error::MalformedQIR,
                    fmt::format(
                            "line {}, column {}: {}\n  {}\n  {}",
                            pos.line,
                            pos.column,
                            ex.message(),
                            LineContaining(text, pos.byte),
                            caret
                    )
            );
        }
        throw QRETError(error::MalformedQIR, fmt::format("parse error: {}", ex.what()));
    } catch (const QRETError&) {
        throw;
    } catch (const std::exception& ex) {
        throw QRETError(error::MalformedQIR, fmt::format("parse error: {}", ex.what()));
    }
}
#endif  // USE_PEGTL

}  // namespace qret::qir
