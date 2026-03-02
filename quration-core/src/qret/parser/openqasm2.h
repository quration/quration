/**
 * @file qret/parser/openqasm2.h
 * @brief OpenQASM2 parser.
 * @details This file defines a parser of OpenQASM2.
 */

#ifndef QRET_PARSER_OPENQASM2_H
#define QRET_PARSER_OPENQASM2_H

#include <algorithm>
#include <iostream>
#include <list>
#include <memory>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "qret/base/type.h"
#include "qret/qret_export.h"

namespace qret::openqasm2 {
// Forward declarations
struct QRET_EXPORT Program;

/**
 * @brief Determine if OpenQASM2 files can be parsed.
 * @details When the USE_PEGTL macro is defined, the OpenQASM2 file can be parsed.
 *
 * @return true can parse
 * @return false cannot parse
 */
QRET_EXPORT bool CanParseOpenQASM2();

/**
 * @brief Parse OpenQASM2 file and construct AST.
 *
 * @param file path to OpenQASM2 file
 * @return Program AST
 */
QRET_EXPORT Program ParseOpenQASM2File(std::string_view file);

/**
 * @brief Parse OpenQASM2 string and construct AST.
 *
 * @param qasm OpenQASM2 string
 * @return Program AST
 */
QRET_EXPORT Program ParseOpenQASM2(const std::string& qasm);

//--------------------------------------------------//
// AST
//--------------------------------------------------//

/**
 * @brief Argument class.
 * @details
 * @verbatim
 * <argument> |= <id> | <id> [<nninteger>]
 * @endverbatim
 */
struct QRET_EXPORT Argument {
    std::string symbol;
    std::optional<std::size_t> index;

    void Print(std::ostream& out) const;
};

/**
 * @brief Expression class.
 * @details
 * @verbatim
 * <exp> |= <real> | <nninteger> | pi | <id>
 *        | <exp> + <exp> | <exp> - <exp> | <exp> * <exp>
 *        | <exp> / <exp> | -<exp> | <exp> ^ <exp>
 *        | (<exp>) | <unaryop> (<exp>)
 *
 * <unaryop> = sin | cos | tan | exp | ln | sqrt
 * @endverbatim
 */
struct QRET_EXPORT Exp {
    enum class Type : std::uint8_t {
        // atomic
        atomic_begin,
        real,
        nninteger,
        pi,
        id,
        atomic_end,
        // unary
        unary_begin,
        minus,
        unary_end,
        // binary
        binary_begin,
        add,
        sub,
        mul,
        div,
        pow,
        binary_end,
        // func
        func_begin,
        sin,
        cos,
        tan,
        exp,
        ln,
        sqrt,
        func_end,
    };
    using Data = std::variant<
            // 0: real, pi
            double,
            // 1: nninteger
            BigInt,
            // 2: id
            std::string,
            // 3: unary, func
            std::unique_ptr<Exp>,
            // 4: binary
            std::pair<std::unique_ptr<Exp>, std::unique_ptr<Exp>>
            // end
            >;
    Type t;
    Data data;

    Exp(Type t, Data&& data)
        : t{t}
        , data{std::move(data)} {}
    Exp(const Exp&) = delete;
    Exp& operator=(const Exp&) = delete;

    static std::unique_ptr<Exp> Real(double x) {
        return std::unique_ptr<Exp>(new Exp{Type::real, x});
    }
    static std::unique_ptr<Exp> NNInteger(const BigInt& x) {
        return std::unique_ptr<Exp>(new Exp{Type::nninteger, x});
    }
    static std::unique_ptr<Exp> Pi() {
        return std::unique_ptr<Exp>(new Exp{Type::pi, std::numbers::pi});
    }
    static std::unique_ptr<Exp> Id(const std::string& x) {
        return std::unique_ptr<Exp>(new Exp{Type::id, x});
    }
    static std::unique_ptr<Exp> Minus(std::unique_ptr<Exp>&& x) {
        return std::unique_ptr<Exp>(new Exp{Type::minus, std::move(x)});
    }
    static std::unique_ptr<Exp>
    Binary(Type t, std::unique_ptr<Exp>&& lhs, std::unique_ptr<Exp>&& rhs) {
        return std::unique_ptr<Exp>(new Exp{t, std::make_pair(std::move(lhs), std::move(rhs))});
    }
    static std::unique_ptr<Exp> Add(std::unique_ptr<Exp>&& lhs, std::unique_ptr<Exp>&& rhs) {
        return Binary(Type::add, std::move(lhs), std::move(rhs));
    }
    static std::unique_ptr<Exp> Sub(std::unique_ptr<Exp>&& lhs, std::unique_ptr<Exp>&& rhs) {
        return Binary(Type::sub, std::move(lhs), std::move(rhs));
    }
    static std::unique_ptr<Exp> Mul(std::unique_ptr<Exp>&& lhs, std::unique_ptr<Exp>&& rhs) {
        return Binary(Type::mul, std::move(lhs), std::move(rhs));
    }
    static std::unique_ptr<Exp> Div(std::unique_ptr<Exp>&& lhs, std::unique_ptr<Exp>&& rhs) {
        return Binary(Type::div, std::move(lhs), std::move(rhs));
    }
    static std::unique_ptr<Exp> Pow(std::unique_ptr<Exp>&& lhs, std::unique_ptr<Exp>&& rhs) {
        return Binary(Type::pow, std::move(lhs), std::move(rhs));
    }
    static std::unique_ptr<Exp> Func(Type t, std::unique_ptr<Exp>&& x) {
        return std::unique_ptr<Exp>(new Exp{t, std::move(x)});
    }
    static std::unique_ptr<Exp> Sin(std::unique_ptr<Exp>&& x) {
        return Func(Type::sin, std::move(x));
    }
    static std::unique_ptr<Exp> Cos(std::unique_ptr<Exp>&& x) {
        return Func(Type::cos, std::move(x));
    }
    static std::unique_ptr<Exp> Tan(std::unique_ptr<Exp>&& x) {
        return Func(Type::tan, std::move(x));
    }
    static std::unique_ptr<Exp> Ex(std::unique_ptr<Exp>&& x) {
        return Func(Type::exp, std::move(x));
    }
    static std::unique_ptr<Exp> Ln(std::unique_ptr<Exp>&& x) {
        return Func(Type::ln, std::move(x));
    }
    static std::unique_ptr<Exp> Sqrt(std::unique_ptr<Exp>&& x) {
        return Func(Type::sqrt, std::move(x));
    }

    [[nodiscard]] bool IsReal() const {
        return t == Type::real;
    }
    [[nodiscard]] bool IsNNInteger() const {
        return t == Type::nninteger;
    }
    [[nodiscard]] bool IsPi() const {
        return t == Type::pi;
    }
    [[nodiscard]] bool IsId() const {
        return t == Type::id;
    }
    [[nodiscard]] bool IsUnary() const {
        return Type::unary_begin <= t && t <= Type::unary_end;
    }
    [[nodiscard]] bool IsBinary() const {
        return Type::binary_begin <= t && t <= Type::binary_end;
    }
    [[nodiscard]] bool IsFunc() const {
        return Type::func_begin <= t && t <= Type::func_end;
    }

    double& GetReal() {
        return std::get<0>(data);
    }
    const double& GetReal() const {
        return std::get<0>(data);
    }
    BigInt& GetNNInteger() {
        return std::get<1>(data);
    }
    const BigInt& GetNNInteger() const {
        return std::get<1>(data);
    }
    std::string& GetId() {
        return std::get<2>(data);
    }
    const std::string& GetId() const {
        return std::get<2>(data);
    }
    std::unique_ptr<Exp>& GetUnary() {
        return std::get<3>(data);
    }
    const std::unique_ptr<Exp>& GetUnary() const {
        return std::get<3>(data);
    }
    std::pair<std::unique_ptr<Exp>, std::unique_ptr<Exp>>& GetBinary() {
        return std::get<4>(data);
    }
    const std::pair<std::unique_ptr<Exp>, std::unique_ptr<Exp>>& GetBinary() const {
        return std::get<4>(data);
    }
    std::unique_ptr<Exp>& GetFunc() {
        return std::get<3>(data);
    }
    const std::unique_ptr<Exp>& GetFunc() const {
        return std::get<3>(data);
    }

    void Print(std::ostream& out) const;
};

/**
 * @brief Statement class.
 * @todo Support opaque syntax.
 * @details
 * @verbatim
 * <statement> |= <decl>
 *              | <gatedecl> <goplist> }
 *              | <gatedecl> }
 *              | <qop>
 *              | if (<id> == <nninteger>) <qop>
 *              | barrier <anylist> ;
 *
 * <qop> |= <uop>
 *        | measure <argument> -> <argument> ;
 *        | reset <argument> ;
 * @endverbatim
 */
struct QRET_EXPORT Statement {
    enum class Type : std::uint8_t {
        // <decl>
        Decl = 0,
        // <qop>
        Uop,
        Measure,
        Reset,
        // <gatedecl> <goplist> } && <gatedecl> }
        GateDecl,
        // if (<id> == <nninteger>) <qop>
        Branch,
        // barrier <anylist> ;
        Barrier
    };
    Type t;

    explicit Statement(Type t)
        : t{t} {}
    virtual ~Statement() = default;
    virtual void Print(std::ostream& out) const = 0;
};

/**
 * @brief Declaration statement class.
 * @details
 * @verbatim
 * <decl> |= qreg <id> [nninteger] ; | creg <id> [nninteger] ;
 * @endverbatim
 */
struct QRET_EXPORT Decl : Statement {
    bool is_q;
    std::string symbol;
    std::size_t num;

    Decl(bool is_q, const std::string& symbol, std::size_t num)
        : Statement(Type::Decl)
        , is_q{is_q}
        , symbol{symbol}
        , num{num} {}
    void Print(std::ostream& out) const override;
};

/**
 * @brief Applying an unary operation statement class.
 * @details
 * @verbatim
 * <uop> |= U (<explist>) <argument> ;
 *        | CX <argument>, <argument> ;
 *        | <id> <anylist> ; | <id> () <anylist> ;
 *        | <id> (<explist>) <anylist> ;
 * @endverbatim
 */
struct QRET_EXPORT Uop : Statement {
    std::string gate;
    std::list<std::unique_ptr<Exp>> params;  // classical value
    std::vector<Argument> args;  // quantum value

    Uop(const std::string& gate,
        std::list<std::unique_ptr<Exp>>&& params,
        const std::vector<Argument>& args)
        : Statement(Type::Uop)
        , gate{gate}
        , params{std::move(params)}
        , args{args} {}
    Uop(const Uop&) = delete;
    Uop& operator=(const Uop&) = delete;
    void Print(std::ostream& out) const override;
};

/**
 * @brief Measurement statement class.
 * @details
 * @verbatim
 * measure <argument> -> <argument> ;
 * @endverbatim
 */
struct QRET_EXPORT Measure : Statement {
    Argument q;
    Argument c;

    Measure(const Argument& q, const Argument& c)
        : Statement(Type::Measure)
        , q{q}
        , c{c} {}
    void Print(std::ostream& out) const override;
};

/**
 * @brief Reset statement class.
 * @details
 * @verbatim
 * reset <argument> ;
 * @endverbatim
 */
struct QRET_EXPORT Reset : Statement {
    Argument q;

    explicit Reset(const Argument& q)
        : Statement(Type::Reset)
        , q{q} {}
    void Print(std::ostream& out) const override;
};

/**
 * @brief Statement of gate declaration class.
 * @details
 * @verbatim
 * <gatedecl> |= gate <id> <idlist> {
 *             | gate <id> () <idlist> {
 *             | gate <id> (<idlist>) <idlist> {
 *
 * <gatedecl> <goplist> } && <gatedecl> }
 *
 * <goplist> |= <uop>
 *            | barrier <idlist> ;
 *            | <goplist> <uop> ;
 *            | <goplist> barrier <idlist> ;
 * @endverbatim
 */
struct QRET_EXPORT GateDecl : Statement {
    std::string name;
    std::vector<std::string> params;
    std::vector<std::string> args;
    std::list<std::unique_ptr<Statement>> gops;

    GateDecl(
            const std::string& name,
            const std::vector<std::string>& params,
            const std::vector<std::string>& args,
            std::list<std::unique_ptr<Statement>>&& gops
    )
        : Statement(Type::GateDecl)
        , name{name}
        , params{params}
        , args{args}
        , gops{std::move(gops)} {}
    GateDecl(const GateDecl&) = delete;
    GateDecl& operator=(const GateDecl&) = delete;
    void Print(std::ostream& out) const override;
};

/**
 * @brief Branch statement class.
 * @details
 * @verbatim
 * if (<id> == <nninteger>) <qop>
 * @endverbatim
 */
struct QRET_EXPORT Branch : Statement {
    std::string symbol;
    BigInt value;
    std::unique_ptr<Statement> op;

    Branch(const std::string& symbol, const BigInt& value, std::unique_ptr<Statement>&& op)
        : Statement(Type::Branch)
        , symbol{symbol}
        , value{value}
        , op{std::move(op)} {}
    Branch(const Branch&) = delete;
    Branch& operator=(const Branch&) = delete;
    void Print(std::ostream& out) const override;
};

/**
 * @brief Barrier statement class.
 * @verbatim
 * barrier <anylist> ;
 * @endverbatim
 */
struct QRET_EXPORT Barrier : Statement {
    std::vector<Argument> args;

    explicit Barrier(const std::vector<Argument>& args)
        : Statement(Type::Barrier)
        , args{args} {}
    void Print(std::ostream& out) const override;
};

/**
 * @brief Program class of OpenQASM2.
 * @details
 * @verbatim
 * <mainprogram> |= OPENQASM <real> ; <program>
 *
 * <program> = <statement> | <program> <statement>
 * @endverbatim
 */
struct QRET_EXPORT Program {
    double version = -1.0;
    std::vector<std::string> incls;  //!< list of includes
    std::list<std::unique_ptr<Statement>> sts;  //!< list of statements

    Program() = default;
    Program(const Program&) = delete;
    Program& operator=(const Program&) = delete;
    Program(Program&&) = default;
    Program& operator=(Program&& program) = default;

    [[nodiscard]] bool NoVersion() const {
        return version < 0;
    }
    [[nodiscard]] bool IsQASM2() const {
        return 2.0 <= version && version < 3.0;
    }
    [[nodiscard]] bool IsQASM3() const {
        return 3.0 <= version;
    }
    [[nodiscard]] bool UseQELIB1() const {
        return incls.end() != std::find(incls.begin(), incls.end(), "qelib1.inc");
    }
    void Print(std::ostream& out) const;
};
}  // namespace qret::openqasm2

#endif  // QRET_PARSER_OPENQASM2_H
