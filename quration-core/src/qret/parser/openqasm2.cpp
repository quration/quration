/**
 * @file qret/parser/openqasm2.cpp
 * @brief OpenQASM2 parser.
 */

#include "qret/parser/openqasm2.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#ifdef USE_PEGTL
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
#include <tao/pegtl.hpp>
#include <tao/pegtl/argv_input.hpp>
#include <tao/pegtl/ascii.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <tao/pegtl/contrib/parse_tree_to_dot.hpp>
#include <tao/pegtl/contrib/trace.hpp>
#include <tao/pegtl/file_input.hpp>
#include <tao/pegtl/rules.hpp>
#include <tao/pegtl/string_input.hpp>
#pragma clang diagnostic pop
#endif

namespace qret::openqasm2 {
//--------------------------------------------------//
// From OpenQASM2 to AST
//--------------------------------------------------//

// Forward declarations
Program ParseOpenQASM2Impl(std::string_view qasm);

namespace {
/**
 * @brief Delete comments in OpenQASM2 string.
 */
template <typename Stream>
std::string DeleteComment(Stream&& stream) {
    auto qasm = std::string();
    auto line = std::string();
    while (std::getline(stream, line)) {
        const auto num = line.size();
        qasm.reserve(qasm.size() + num);
        for (auto i = std::size_t{0}; i < num; ++i) {
            if (i + 1 < num && line[i] == '/' && line[i + 1] == '/') {
                break;
            }
            qasm += line[i];
        }
        qasm += '\n';
    }
    return qasm;
}
}  // namespace

bool CanParseOpenQASM2() {
#ifdef USE_PEGTL
    return true;
#else
    return false;
#endif
}
Program ParseOpenQASM2File(std::string_view file) {
    if (!CanParseOpenQASM2()) {
        throw std::runtime_error("Define USE_PEGTL to parse OpenQASM2");
    }

    auto ifs = std::ifstream{std::string(file)};
    if (!ifs.good()) {
        throw std::runtime_error(fmt::format("file not found: {}", file));
    }
    const auto no_comment_qasm = DeleteComment(ifs);
    auto program = ParseOpenQASM2Impl(no_comment_qasm);

    const auto base = std::filesystem::path(file).parent_path();
    for (auto& incl : program.incls) {
        if (incl == "qelib1.inc" || incl == "stdgates.inc") {
            continue;
        }
        auto incl_path = std::filesystem::path(incl);
        if (!incl_path.is_absolute()) {
            incl_path = base / incl_path;
        }
        incl = incl_path.lexically_normal().string();
    }

    return program;
}
Program ParseOpenQASM2(const std::string& qasm) {
    if (!CanParseOpenQASM2()) {
        throw std::runtime_error("Define USE_PEGTL to parse OpenQASM2");
    }

    auto ss = std::stringstream(qasm.data());
    const auto no_comment_qasm = DeleteComment(ss);
    return ParseOpenQASM2Impl(no_comment_qasm);
}

//--------------------------------------------------//
// Methods of AST classes
//--------------------------------------------------//

void Argument::Print(std::ostream& out) const {
    out << symbol;
    if (index) {
        out << "[" << index.value() << "]";
    }
}
void Exp::Print(std::ostream& out) const {
    if (IsReal()) {
        out << GetReal();
    } else if (IsNNInteger()) {
        out << GetNNInteger();
    } else if (IsPi()) {
        out << "pi";
    } else if (IsId()) {
        out << GetId();
    } else if (IsUnary()) {
        if (t == Type::minus) {
            out << "-";
        }
        out << "(";
        GetUnary()->Print(out);
        out << ")";
    } else if (IsBinary()) {
        const auto& [lhs, rhs] = GetBinary();
        out << "(";
        lhs->Print(out);
        out << ")";
        switch (t) {
            case Type::add:
                out << " + ";
                break;
            case Type::sub:
                out << " - ";
                break;
            case Type::mul:
                out << " * ";
                break;
            case Type::div:
                out << " / ";
                break;
            case Type::pow:
                out << " ^ ";
                break;
            default:
                assert(0 && "unreachable");
        }
        out << "(";
        rhs->Print(out);
        out << ")";
    } else if (IsFunc()) {
        switch (t) {
            case Type::sin:
                out << "sin";
                break;
            case Type::cos:
                out << "cos";
                break;
            case Type::tan:
                out << "tan";
                break;
            case Type::exp:
                out << "exp";
                break;
            case Type::ln:
                out << "ln";
                break;
            case Type::sqrt:
                out << "sqrt";
                break;
            default:
                assert(0 && "unreachable");
        }
        out << "(";
        GetUnary()->Print(out);
        out << ")";
    } else {
        assert(0 && "unreachable");
    }
}
void Decl::Print(std::ostream& out) const {
    out << (is_q ? "qreg " : "creg ") << symbol << "[" << num << "];";
}
void Uop::Print(std::ostream& out) const {
    out << gate << " ";
    if (!params.empty()) {
        const char* sep = "";
        out << "(";
        for (const auto& exp : params) {
            out << std::exchange(sep, ", ");
            exp->Print(out);
        }
        out << ") ";
    }
    if (!args.empty()) {
        const char* sep = "";
        for (const auto& arg : args) {
            out << std::exchange(sep, ", ");
            arg.Print(out);
        }
    }
    out << ";";
}
void Measure::Print(std::ostream& out) const {
    out << "measure ";
    q.Print(out);
    out << " -> ";
    c.Print(out);
    out << ";";
}
void Reset::Print(std::ostream& out) const {
    out << "reset ";
    q.Print(out);
    out << ";";
}
void GateDecl::Print(std::ostream& out) const {
    out << "gate " << name << " ";
    if (!params.empty()) {
        const char* sep = "";
        out << "(";
        for (const auto& exp : params) {
            out << std::exchange(sep, ", ") << exp;
        }
        out << ") ";
    }
    if (!args.empty()) {
        const char* sep = "";
        for (const auto& arg : args) {
            out << std::exchange(sep, ", ") << arg;
        }
        out << " ";
    }
    out << "{";
    for (const auto& st : gops) {
        out << "\n  ";
        st->Print(out);
    }
    out << "\n}";
}
void Branch::Print(std::ostream& out) const {
    out << "if (" << symbol << " == " << value << ") ";
    if (op) {
        op->Print(out);
    }
}
void Barrier::Print(std::ostream& out) const {
    out << "barrier ";
    const char* sep = "";
    for (const auto& arg : args) {
        out << std::exchange(sep, ", ");
        arg.Print(out);
    }
    out << ";";
}
void Program::Print(std::ostream& out) const {
    if (!NoVersion()) {
        out << "OPENQASM " << fmt::format("{:.1f}", version) << ";\n";
    }
    for (const auto& incl : incls) {
        out << "include \"" << incl << "\";\n";
    }
    for (const auto& st : sts) {
        st->Print(out);
        out << "\n";
    }
}

//--------------------------------------------------//
// Implementation of OpenQASM2 parser using PEGTL
//--------------------------------------------------//

#ifndef USE_PEGTL

Program ParseOpenQASM2Impl(std::string_view /*qasm*/) {
    throw std::runtime_error("unimplemented");
}

#else  // USE_PEGTL is defined

namespace pegtl = tao::pegtl;
namespace parse_tree = pegtl::parse_tree;

//--------------------------------------------------//
// PEG rule
//--------------------------------------------------//

// utils
struct space0 : pegtl::star<pegtl::space> {};
struct space1 : pegtl::plus<pegtl::space> {};
struct header : pegtl::string<'O', 'P', 'E', 'N', 'Q', 'A', 'S', 'M'> {};
struct incl : pegtl::string<'i', 'n', 'c', 'l', 'u', 'd', 'e'> {};
struct import_kw : pegtl::string<'i', 'm', 'p', 'o', 'r', 't'> {};
struct measure : pegtl::string<'m', 'e', 'a', 's', 'u', 'r', 'e'> {};
struct reset : pegtl::string<'r', 'e', 's', 'e', 't'> {};
struct barrier : pegtl::string<'b', 'a', 'r', 'r', 'i', 'e', 'r'> {};
struct gate : pegtl::string<'g', 'a', 't', 'e'> {};
struct branch : pegtl::string<'i', 'f'> {};
struct u_gate : pegtl::string<'U'> {};
struct cx_gate : pegtl::string<'C', 'X'> {};
struct qreg : pegtl::string<'q', 'r', 'e', 'g'> {};
struct creg : pegtl::string<'c', 'r', 'e', 'g'> {};
struct qubit_kw : pegtl::string<'q', 'u', 'b', 'i', 't'> {};
struct bit_kw : pegtl::string<'b', 'i', 't'> {};
// struct opaque : pegtl::string<'o', 'p', 'a', 'q', 'u', 'e'> {};
struct pi : pegtl::string<'p', 'i'> {};
struct sin_op : pegtl::string<'s', 'i', 'n'> {};
struct cos_op : pegtl::string<'c', 'o', 's'> {};
struct tan_op : pegtl::string<'t', 'a', 'n'> {};
struct exp_op : pegtl::string<'e', 'x', 'p'> {};
struct ln_op : pegtl::string<'l', 'n'> {};
struct sqrt_op : pegtl::string<'s', 'q', 'r', 't'> {};
struct add_op : pegtl::one<'+'> {};
struct sub_op : pegtl::one<'-'> {};
struct mul_op : pegtl::one<'*'> {};
struct div_op : pegtl::one<'/'> {};
struct pow_op : pegtl::one<'^'> {};
struct unaryop : pegtl::sor<sin_op, cos_op, tan_op, exp_op, ln_op, sqrt_op> {};

// atomics
struct id : pegtl::seq<pegtl::lower, pegtl::star<pegtl::identifier_other>> {};
struct real : pegtl::seq<
                      // ([0-9]+\.[0-9]*|[0-9]*\.[0-9]+)
                      pegtl::sor<
                              pegtl::seq<
                                      pegtl::plus<pegtl::digit>,
                                      pegtl::one<'.'>,
                                      pegtl::star<pegtl::digit>>,
                              pegtl::seq<
                                      pegtl::star<pegtl::digit>,
                                      pegtl::one<'.'>,
                                      pegtl::plus<pegtl::digit>>>,
                      // ([eE][-+]?[0-9]+)?
                      pegtl::opt<
                              pegtl::one<'e', 'E'>,
                              pegtl::opt<pegtl::one<'-', '+'>>,
                              pegtl::plus<pegtl::digit>>> {};
struct nninteger
    : pegtl::sor<pegtl::one<'0'>, pegtl::seq<pegtl::ranges<'1', '9'>, pegtl::star<pegtl::digit>>> {
};
struct path : pegtl::plus<pegtl::sor<
                      pegtl::identifier_other,
                      pegtl::one<'.'>,
                      pegtl::one<'/'>,
                      pegtl::one<'\\'>,
                      pegtl::one<'-'>,
                      pegtl::one<':'>>> {};

// exp, explist
struct exp;  // forward declaration to break the cyclic dependency
struct primary_exp : pegtl::sor<
                             real,
                             nninteger,
                             pi,
                             id,
                             pegtl::seq<pegtl::one<'('>, space0, exp, space0, pegtl::one<')'>>> {};
struct func_exp
    : pegtl::seq<unaryop, space0, pegtl::one<'('>, space0, exp, space0, pegtl::one<')'>> {};
struct unary_exp;  // forward declaration for right-recursive unary minus
struct unary_exp : pegtl::sor<
                           // 1
                           func_exp,
                           // 2
                           pegtl::seq<sub_op, unary_exp>,
                           // 3
                           primary_exp
                           // end
                           > {};
struct pow_exp : pegtl::sor<pegtl::seq<unary_exp, space0, pow_op, space0, pow_exp>, unary_exp> {};
struct mul_exp : pegtl::list<pow_exp, pegtl::seq<space0, pegtl::sor<mul_op, div_op>, space0>> {};
struct exp : pegtl::list<mul_exp, pegtl::seq<space0, pegtl::sor<add_op, sub_op>, space0>> {};
struct explist : pegtl::list<exp, pegtl::seq<space0, pegtl::one<','>, space0>> {};

// argument, idlist, anylist
struct argument
    : pegtl::sor<
              pegtl::seq<id, space0, pegtl::one<'['>, space0, nninteger, space0, pegtl::one<']'>>,
              id> {};
struct idlist : pegtl::list<id, pegtl::seq<space0, pegtl::one<','>, space0>> {};
struct anylist : pegtl::list<argument, pegtl::seq<space0, pegtl::one<','>, space0>> {};

// uop
struct uop : pegtl::sor<
                     // 1
                     pegtl::seq<
                             u_gate,
                             space0,
                             pegtl::one<'('>,
                             space0,
                             explist,
                             space0,
                             pegtl::one<')'>,
                             space0,
                             argument,
                             space0,
                             pegtl::one<';'>>,
                     // 2
                     pegtl::seq<
                             cx_gate,
                             space1,
                             argument,
                             space0,
                             pegtl::one<','>,
                             space0,
                             argument,
                             space0,
                             pegtl::one<';'>>,
                     // 3
                     pegtl::seq<id, space0, anylist, space0, pegtl::one<';'>>,
                     // 4
                     pegtl::seq<
                             id,
                             space0,
                             pegtl::one<'('>,
                             space0,
                             pegtl::one<')'>,
                             space0,
                             anylist,
                             space0,
                             pegtl::one<';'>>,
                     // 5
                     pegtl::seq<
                             id,
                             space0,
                             pegtl::one<'('>,
                             space0,
                             explist,
                             space0,
                             pegtl::one<')'>,
                             space0,
                             anylist,
                             space0,
                             pegtl::one<';'>>
                     // end
                     > {};
struct qop : pegtl::sor<
                     // 1
                     pegtl::seq<
                             measure,
                             space1,
                             argument,
                             space0,
                             pegtl::string<'-', '>'>,
                             space0,
                             argument,
                             space0,
                             pegtl::one<';'>>,
                     // 2
                     pegtl::seq<reset, space1, argument, space0, pegtl::one<';'>>,
                     // 3
                     uop
                     // end
                     > {};

// gop, goplist
struct gop : pegtl::sor<
                     // barrier or
                     pegtl::seq<barrier, space1, idlist, space0, pegtl::one<';'>>,
                     // uop
                     uop> {};
struct goplist : pegtl::list<gop, space0> {};

// gateheader, gatedecl
struct gateheader : pegtl::sor<
                            // 1
                            pegtl::seq<gate, space1, id, space1, idlist, space0, pegtl::one<'{'>>,
                            // 2
                            pegtl::seq<
                                    gate,
                                    space1,
                                    id,
                                    space0,
                                    pegtl::one<'('>,
                                    space0,
                                    pegtl::one<')'>,
                                    space0,
                                    idlist,
                                    space0,
                                    pegtl::one<'{'>>,
                            // 3
                            pegtl::seq<
                                    gate,
                                    space1,
                                    id,
                                    space0,
                                    pegtl::one<'('>,
                                    space0,
                                    idlist,
                                    space0,
                                    pegtl::one<')'>,
                                    space0,
                                    idlist,
                                    space0,
                                    pegtl::one<'{'>>
                            // end
                            > {};
struct gatedecl : pegtl::sor<
                          // 1
                          pegtl::seq<gateheader, space0, pegtl::one<'}'>>,
                          // 2
                          pegtl::seq<gateheader, space0, goplist, space0, pegtl::one<'}'>>> {};

// decl
struct decl : pegtl::sor<
                      pegtl::seq<
                              qreg,
                              space1,
                              id,
                              space0,
                              pegtl::one<'['>,
                              space0,
                              nninteger,
                              space0,
                              pegtl::one<']'>,
                              space0,
                              pegtl::one<';'>>,
                      pegtl::seq<
                              creg,
                              space1,
                              id,
                              space0,
                              pegtl::one<'['>,
                              space0,
                              nninteger,
                              space0,
                              pegtl::one<']'>,
                              space0,
                              pegtl::one<';'>>,
                      pegtl::seq<
                              qubit_kw,
                              space0,
                              pegtl::one<'['>,
                              space0,
                              nninteger,
                              space0,
                              pegtl::one<']'>,
                              space1,
                              id,
                              space0,
                              pegtl::one<';'>>,
                      pegtl::seq<
                              bit_kw,
                              space0,
                              pegtl::one<'['>,
                              space0,
                              nninteger,
                              space0,
                              pegtl::one<']'>,
                              space1,
                              id,
                              space0,
                              pegtl::one<';'>>> {};

// statement (TODO: support opaque syntax)
struct statement : pegtl::sor<
                           // 1
                           decl,
                           // 2
                           gatedecl,
                           // 3
                           pegtl::seq<
                                   branch,
                                   space0,
                                   pegtl::one<'('>,
                                   space0,
                                   id,
                                   space0,
                                   pegtl::string<'=', '='>,
                                   space0,
                                   nninteger,
                                   space0,
                                   pegtl::one<')'>,
                                   space0,
                                   qop>,
                           // 4
                           pegtl::seq<barrier, space1, anylist, space0, pegtl::one<';'>>,
                           // 5
                           qop
                           // end
                           > {};

// program
struct program_header : pegtl::seq<header, space0, real, pegtl::one<';'>> {};
struct include_sentence
    : pegtl::seq<
              pegtl::sor<incl, import_kw>,
              space0,
              pegtl::sor<pegtl::seq<pegtl::one<'"'>, space0, path, space0, pegtl::one<'"'>>, path>,
              space0,
              pegtl::one<';'>> {};
struct program : pegtl::list<statement, space0> {};

// grammar
struct grammar : pegtl::must<
                         space0,
                         pegtl::opt<program_header>,
                         space0,
                         pegtl::star<include_sentence, space0>,
                         space0,
                         program,
                         space0,
                         pegtl::eof> {};

template <typename Rule>
using selector = parse_tree::selector<
        Rule,
        parse_tree::store_content::on<id, real, nninteger, path>,
        parse_tree::remove_content::on<
                // utils
                space0,
                space1,
                header,
                incl,
                import_kw,
                measure,
                reset,
                barrier,
                gate,
                u_gate,
                cx_gate,
                qreg,
                creg,
                qubit_kw,
                bit_kw,
                pi,
                sin_op,
                cos_op,
                tan_op,
                exp_op,
                ln_op,
                sqrt_op,
                add_op,
                sub_op,
                mul_op,
                div_op,
                pow_op,
                unaryop,
                // exp
                exp,
                primary_exp,
                func_exp,
                unary_exp,
                pow_exp,
                mul_exp,
                exp,
                explist,
                // argument, idlist, anylist
                argument,
                idlist,
                anylist,
                // uop, qop
                uop,
                qop,
                // gop, goplist
                gop,
                goplist,
                // gateheader, gatedecl
                gateheader,
                gatedecl,
                // decl
                decl,
                // statement
                statement,
                // program
                program_header,
                include_sentence,
                program,
                // grammar
                grammar>>;

// TODO: Implement error handling of parsing
struct error {
    template <typename>
    static constexpr bool raise_on_failure = false;

    template <typename>
    static constexpr auto message = "parse error";
};
template <typename Rule>
using control_no_msg = pegtl::must_if<error>::control<Rule>;

//--------------------------------------------------//
// Parse functions
//--------------------------------------------------//

using ParseNode = std::unique_ptr<parse_tree::node>;
void ParseIfId(const ParseNode& n, std::string& out) {
    if (n->is_type<id>()) {
        out = n->string();
    }
}
void ParseIfReal(const ParseNode& n, double& out) {
    if (n->is_type<real>()) {
        out = std::stod(n->string());
    }
}
void ParseIfNNInteger(const ParseNode& n, BigInt& out) {
    if (n->is_type<nninteger>()) {
        out.assign(n->string());
    }
}
Argument ParseArgument(const ParseNode& n) {
    assert(n->is_type<argument>());
    if (n->children.size() == 1) {
        // symbol
        return Argument{n->children.front()->string(), std::nullopt};
    } else {
        // symbol[index]
        auto symbol = std::string{};
        auto index = BigInt{0};
        for (const auto& child : n->children) {
            ParseIfId(child, symbol);
            ParseIfNNInteger(child, index);
        }
        return Argument{symbol, static_cast<std::size_t>(index)};
    }
}
std::vector<Argument> ParseAnyList(const ParseNode& n) {
    assert(n->is_type<anylist>());
    auto ret = std::vector<Argument>();
    for (const auto& child : n->children) {
        if (child->is_type<argument>()) {
            ret.emplace_back(ParseArgument(child));
        }
    }
    return ret;
}
std::vector<std::string> ParseIdList(const ParseNode& n) {
    assert(n->is_type<idlist>());
    auto ret = std::vector<std::string>();
    for (const auto& child : n->children) {
        auto tmp = std::string();
        ParseIfId(child, tmp);
        if (!tmp.empty()) {
            ret.emplace_back(tmp);
        }
    }
    return ret;
}
std::unique_ptr<Exp> ParseExp(const ParseNode& n);
std::unique_ptr<Exp> ParsePowExp(const ParseNode& n);
std::unique_ptr<Exp> ParsePrimaryExp(const ParseNode& n) {
    assert(n->is_type<primary_exp>());
    auto ret = std::unique_ptr<Exp>(nullptr);
    for (const auto& child : n->children) {
        if (child->is_type<real>()) {
            auto x = double{0};
            ParseIfReal(child, x);
            ret = Exp::Real(x);
        } else if (child->is_type<nninteger>()) {
            auto x = BigInt{0};
            ParseIfNNInteger(child, x);
            ret = Exp::NNInteger(x);
        } else if (child->is_type<pi>()) {
            ret = Exp::Pi();
        } else if (child->is_type<id>()) {
            auto symbol = std::string();
            ParseIfId(child, symbol);
            ret = Exp::Id(symbol);
        } else if (child->is_type<exp>()) {
            ret = ParseExp(child);
        }
    }
    return ret;
}
std::unique_ptr<Exp> ParseFuncExp(const ParseNode& n) {
    assert(n->is_type<func_exp>());
    auto t = Exp::Type::func_begin;
    auto ret = std::unique_ptr<Exp>(nullptr);
    for (const auto& child : n->children) {
        if (child->is_type<unaryop>()) {
            const auto& type = child->children.front();
            if (type->is_type<sin_op>()) {
                t = Exp::Type::sin;
            } else if (type->is_type<cos_op>()) {
                t = Exp::Type::cos;
            } else if (type->is_type<tan_op>()) {
                t = Exp::Type::tan;
            } else if (type->is_type<exp_op>()) {
                t = Exp::Type::exp;
            } else if (type->is_type<ln_op>()) {
                t = Exp::Type::ln;
            } else if (type->is_type<sqrt_op>()) {
                t = Exp::Type::sqrt;
            }
        } else if (child->is_type<exp>()) {
            ret = Exp::Func(t, ParseExp(child));
        }
    }
    return ret;
}
std::unique_ptr<Exp> ParseUnaryExp(const ParseNode& n) {
    assert(n->is_type<unary_exp>());
    const auto& front = n->children.front();
    if (front->is_type<func_exp>()) {
        return ParseFuncExp(front);
    } else if (front->is_type<primary_exp>()) {
        return ParsePrimaryExp(front);
    } else if (front->is_type<sub_op>()) {
        return Exp::Minus(ParseUnaryExp(n->children[1]));
    }
    assert(0 && "unreachable");
    return nullptr;
}
std::unique_ptr<Exp> ParsePowExp(const ParseNode& n) {
    assert(n->is_type<pow_exp>());
    if (n->children.empty()) {
        throw std::runtime_error("invalid pow_exp");
    }
    auto lhs = ParseUnaryExp(n->children.front());
    if (n->children.size() == 1) {
        return lhs;
    }

    const auto& rhs_node = n->children.back();
    if (!rhs_node->is_type<pow_exp>()) {
        throw std::runtime_error("invalid pow_exp");
    }
    return Exp::Pow(std::move(lhs), ParsePowExp(rhs_node));
}
std::unique_ptr<Exp> ParseMulExp(const ParseNode& n) {
    assert(n->is_type<mul_exp>());
    auto ret = std::unique_ptr<Exp>(nullptr);
    auto mode = 0;  // 1 --> mul, 2 --> div
    for (const auto& child : n->children) {
        if (child->is_type<pow_exp>()) {
            auto tmp = ParsePowExp(child);
            if (mode == 0) {
                ret = std::move(tmp);
            } else if (mode == 1) {
                ret = Exp::Mul(std::move(ret), std::move(tmp));
            } else if (mode == 2) {
                ret = Exp::Div(std::move(ret), std::move(tmp));
            }
            mode = 0;
        } else if (child->is_type<mul_op>()) {
            mode = 1;
        } else if (child->is_type<div_op>()) {
            mode = 2;
        }
    }
    return ret;
}
std::unique_ptr<Exp> ParseExp(const ParseNode& n) {
    assert(n->is_type<exp>());
    auto ret = std::unique_ptr<Exp>(nullptr);
    auto mode = 0;  // 1 --> add, 2 --> sub
    for (const auto& child : n->children) {
        if (child->is_type<mul_exp>()) {
            auto tmp = ParseMulExp(child);
            if (mode == 0) {
                ret = std::move(tmp);
            } else if (mode == 1) {
                ret = Exp::Add(std::move(ret), std::move(tmp));
            } else if (mode == 2) {
                ret = Exp::Sub(std::move(ret), std::move(tmp));
            }
            mode = 0;
        } else if (child->is_type<add_op>()) {
            mode = 1;
        } else if (child->is_type<sub_op>()) {
            mode = 2;
        }
    }
    return ret;
}
std::list<std::unique_ptr<Exp>> ParseExpList(const ParseNode& n) {
    assert(n->is_type<explist>());
    auto ret = std::list<std::unique_ptr<Exp>>();
    for (const auto& child : n->children) {
        if (child->is_type<exp>()) {
            ret.emplace_back(ParseExp(child));
        }
    }
    return ret;
}
std::unique_ptr<Decl> ParseDecl(const ParseNode& n) {
    assert(n->is_type<decl>());
    auto symbol = std::string{};
    auto num = BigInt{0};
    for (const auto& child : n->children) {
        ParseIfId(child, symbol);
        ParseIfNNInteger(child, num);
    }
    if (n->children.front()->is_type<qreg>() || n->children.front()->is_type<qubit_kw>()) {
        return std::unique_ptr<Decl>(new Decl(true, symbol, static_cast<std::size_t>(num)));
    } else {
        return std::unique_ptr<Decl>(new Decl(false, symbol, static_cast<std::size_t>(num)));
    }
}
std::unique_ptr<Measure> ParseMeasure(const ParseNode& n) {
    assert(n->is_type<qop>());
    auto args = std::vector<Argument>();
    args.reserve(2);
    for (const auto& child : n->children) {
        if (child->is_type<argument>()) {
            args.emplace_back(ParseArgument(child));
        }
    }
    assert(args.size() == 2);
    return std::unique_ptr<Measure>(new Measure(args[0], args[1]));
}
std::unique_ptr<Reset> ParseReset(const ParseNode& n) {
    assert(n->is_type<qop>());
    auto arg = Argument();
    for (const auto& child : n->children) {
        if (child->is_type<argument>()) {
            arg = ParseArgument(child);
        }
    }
    return std::unique_ptr<Reset>(new Reset(arg));
}
std::unique_ptr<Uop> ParseUop(const ParseNode& n) {
    assert(n->is_type<uop>());
    auto gate = std::string();
    auto params = std::list<std::unique_ptr<Exp>>();
    auto args = std::vector<Argument>();

    auto mode = std::uint32_t{0};
    for (const auto& child : n->children) {
        if (mode == 0) {
            // Parse the gate name of the uop
            if (child->is_type<u_gate>()) {
                gate = "U";
            } else if (child->is_type<cx_gate>()) {
                gate = "CX";
            } else {
                ParseIfId(child, gate);
            }
            if (!gate.empty()) {
                mode = 1;
            }
        } else {
            if (child->is_type<explist>()) {
                params = ParseExpList(child);
            } else if (child->is_type<anylist>()) {
                args = ParseAnyList(child);
            } else if (child->is_type<argument>()) {
                args.emplace_back(ParseArgument(child));
            }
        }
    }
    return std::unique_ptr<Uop>(new Uop(gate, std::move(params), args));
}
std::unique_ptr<Statement> ParseQop(const ParseNode& n) {
    assert(n->is_type<qop>());
    const auto& front = n->children.front();
    if (front->is_type<measure>()) {
        return ParseMeasure(n);
    } else if (front->is_type<reset>()) {
        return ParseReset(n);
    } else {
        return ParseUop(n->children.front());
    }
}
void ParseGateHeader(
        const ParseNode& n,
        std::string& name,
        std::vector<std::string>& params,
        std::vector<std::string>& args
) {
    assert(n->is_type<gateheader>());
    for (const auto& child : n->children) {
        if (child->is_type<id>()) {
            ParseIfId(child, name);
        } else if (child->is_type<idlist>()) {
            if (args.empty()) {
                args = ParseIdList(child);
            } else {
                args.swap(params);
                args = ParseIdList(child);
            }
        }
    }
}
std::list<std::unique_ptr<Statement>> ParseGopList(const ParseNode& n) {
    assert(n->is_type<goplist>());
    auto ret = std::list<std::unique_ptr<Statement>>();
    for (const auto& child : n->children) {
        if (!child->is_type<gop>()) {
            continue;
        }
        assert(child->is_type<gop>());
        const auto& front = child->children.front();
        if (front->is_type<uop>()) {
            ret.emplace_back(ParseUop(front));
        } else {
            // parse barrier
            auto args = std::vector<Argument>();
            for (const auto& gop_child : child->children) {
                if (gop_child->is_type<idlist>()) {
                    const auto tmp = ParseIdList(gop_child);
                    for (const auto& arg : tmp) {
                        args.emplace_back(arg);
                    }
                }
            }
            ret.emplace_back(std::unique_ptr<Barrier>(new Barrier(args)));
        }
    }
    return ret;
}
std::unique_ptr<GateDecl> ParseGateDecl(const ParseNode& n) {
    assert(n->is_type<gatedecl>());
    auto name = std::string();
    auto params = std::vector<std::string>();
    auto args = std::vector<std::string>();
    auto gops = std::list<std::unique_ptr<Statement>>();
    for (const auto& child : n->children) {
        if (child->is_type<gateheader>()) {
            ParseGateHeader(child, name, params, args);
        } else if (child->is_type<goplist>()) {
            gops = ParseGopList(child);
        }
    }

    return std::unique_ptr<GateDecl>(new GateDecl(name, params, args, std::move(gops)));
}
std::unique_ptr<Branch> ParseBranch(const ParseNode& n) {
    assert(n->is_type<statement>());
    auto symbol = std::string();
    auto value = BigInt{0};
    auto op = std::unique_ptr<Statement>(nullptr);
    for (const auto& child : n->children) {
        if (child->is_type<id>()) {
            ParseIfId(child, symbol);
        } else if (child->is_type<nninteger>()) {
            ParseIfNNInteger(child, value);
        } else if (child->is_type<qop>()) {
            op = ParseQop(child);
        }
    }
    return std::unique_ptr<Branch>(new Branch(symbol, value, std::move(op)));
}
std::unique_ptr<Barrier> ParseBarrier(const ParseNode& n) {
    assert(n->is_type<statement>());
    auto args = std::vector<Argument>();
    for (const auto& child : n->children) {
        if (child->is_type<anylist>()) {
            args = ParseAnyList(child);
        }
    }
    return std::unique_ptr<Barrier>(new Barrier(args));
}
std::unique_ptr<Statement> ParseStatement(const ParseNode& n) {
    assert(n->is_type<statement>());
    const auto& front = n->children.front();
    if (front->is_type<openqasm2::decl>()) {
        return ParseDecl(n->children.front());
    } else if (front->is_type<openqasm2::qop>()) {
        return ParseQop(n->children.front());
    } else if (front->is_type<openqasm2::gatedecl>()) {
        return ParseGateDecl(n->children.front());
    } else if (front->is_type<openqasm2::barrier>()) {
        return ParseBarrier(n);
    } else {
        return ParseBranch(n);
    }
    return nullptr;
}
template <typename In>
Program ParseOpenQASM2Grammar(In&& in) {
    auto ret = Program();
    const auto root = pegtl::parse_tree::parse<
            openqasm2::grammar,
            openqasm2::selector,
            pegtl::nothing,
            openqasm2::control_no_msg>(in);
    if (!root) {
        throw std::runtime_error("parse failed");
    }
    const auto& grammar_node = root->children.front();
    assert(grammar_node->template is_type<grammar>());

    for (const auto& child : grammar_node->children) {
        if (child->template is_type<program_header>()) {
            for (const auto& grch : child->children) {
                ParseIfReal(grch, ret.version);
            }
        } else if (child->template is_type<include_sentence>()) {
            for (const auto& grch : child->children) {
                if (grch->template is_type<path>()) {
                    ret.incls.emplace_back(grch->string());
                }
            }
        } else if (child->template is_type<program>()) {
            for (const auto& grch : child->children) {
                if (grch->template is_type<statement>()) {
                    ret.sts.emplace_back(ParseStatement(grch));
                }
            }
        }
    }
    return ret;
}
Program ParseOpenQASM2Impl(std::string_view qasm) {
    auto in = pegtl::string_input(qasm, "qasm");
    try {
        return ParseOpenQASM2Grammar(in);
    } catch (std::exception& ex) {
        throw std::runtime_error(fmt::format("parse error: {}", ex.what()));
    }
}
#endif  // USE_PEGTL

}  // namespace qret::openqasm2
