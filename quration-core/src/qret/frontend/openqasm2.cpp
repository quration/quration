/**
 * @file qret/frontend/openqasm2.cpp
 * @brief Build circuit from OpenQASM2 AST.
 */

#include "qret/frontend/openqasm2.h"

#include <cassert>
#include <cmath>
#include <filesystem>
#include <memory>
#include <numbers>
#include <stdexcept>
#include <unordered_set>

#include "qret/base/log.h"
#include "qret/base/type.h"
#include "qret/frontend/argument.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/intrinsic.h"
#include "qret/parser/openqasm2.h"

namespace qret::frontend {
using namespace qret::openqasm2;

namespace {
bool DoubleEq(double lhs, double rhs) {
    static constexpr auto Eps = 1e-9;
    return std::abs(lhs - rhs) < Eps;
}
}  // namespace

/**
 * @brief Evaluate the value of the expression.
 * @details Use this function to evaluate Exp in GateDecl.
 *
 * @param exp the expression
 * @param param_names the names of params
 * @param param_values the values of params
 * @return evaluated value
 */
double EvaluateExp(
        const std::unique_ptr<Exp>& exp,
        const std::vector<std::string>& param_names,
        const std::vector<double>& param_values
) {
    assert(param_names.size() == param_values.size());

    if (exp->IsReal()) {
        return exp->GetReal();
    } else if (exp->IsNNInteger()) {
        return static_cast<double>(exp->GetNNInteger());
    } else if (exp->IsPi()) {
        return std::numbers::pi;
    } else if (exp->IsId()) {
        // TODO: implement more efficiently
        const auto& name = exp->GetId();
        for (auto i = std::size_t{0}; i < param_names.size(); ++i) {
            if (name == param_names[i]) {
                return param_values[i];
            }
        }
        throw std::runtime_error("found unknown id (" + name + ") in the expression");
    } else if (exp->IsUnary()) {
        const auto val = EvaluateExp(exp->GetUnary(), param_names, param_values);
        switch (exp->t) {
            case Exp::Type::minus:
                return -val;
            default:
                assert(0 && "unreachable");
        }
    } else if (exp->IsBinary()) {
        const auto lhs = EvaluateExp(std::get<0>(exp->GetBinary()), param_names, param_values);
        const auto rhs = EvaluateExp(std::get<1>(exp->GetBinary()), param_names, param_values);
        switch (exp->t) {
            case Exp::Type::add:
                return lhs + rhs;
            case Exp::Type::sub:
                return lhs - rhs;
            case Exp::Type::mul:
                return lhs * rhs;
            case Exp::Type::div:
                return lhs / rhs;
            case Exp::Type::pow:
                return std::pow(lhs, rhs);
            default:
                assert(0 && "unreachable");
        }
    } else if (exp->IsFunc()) {
        const auto val = EvaluateExp(exp->GetUnary(), param_names, param_values);
        switch (exp->t) {
            case Exp::Type::sin:
                return std::sin(val);
            case Exp::Type::cos:
                return std::cos(val);
            case Exp::Type::tan:
                return std::tan(val);
            case Exp::Type::exp:
                return std::exp(val);
            case Exp::Type::ln:
                return std::log(val);
            case Exp::Type::sqrt:
                return std::sqrt(val);
            default:
                assert(0 && "unreachable");
        }
    }
    assert(0 && "unreachable");
    return 0.0;
}

class OpenQASM2Context {
public:
    OpenQASM2Context() = default;
    OpenQASM2Context(const OpenQASM2Context&) = delete;
    OpenQASM2Context(OpenQASM2Context&&) = delete;
    OpenQASM2Context& operator=(const OpenQASM2Context&) = delete;
    OpenQASM2Context& operator=(OpenQASM2Context&&) = delete;

    [[nodiscard]] bool AddDecl(const Decl& decl) {
        if (qubits_.contains(decl.symbol) || registers_.contains(decl.symbol)
            || gates_.contains(decl.symbol)) {
            return false;
        }
        if (decl.is_q) {
            qubits_.emplace(decl.symbol, static_cast<std::uint32_t>(decl.num));
        } else {
            registers_.emplace(decl.symbol, static_cast<std::uint32_t>(decl.num));
        }
        return true;
    }
    [[nodiscard]] bool AddGateDecl(const GateDecl& gate_decl) {
        if (gates_.contains(gate_decl.name) || qubits_.contains(gate_decl.name)
            || registers_.contains(gate_decl.name)) {
            return false;
        }
        gates_.emplace(gate_decl.name, gate_decl);
        return true;
    }
    void ImportIncludes(const std::vector<std::string>& includes) {
        for (const auto& include : includes) {
            ImportInclude(include);
        }
    }
    [[nodiscard]] bool Contains(const std::string& symbol) const {
        return gates_.contains(symbol);
    }
    [[nodiscard]] const GateDecl& GetGateDecl(const std::string& symbol) const {
        return gates_.at(symbol);
    }

private:
    static Program LoadQELIB1();
    static Program LoadStdGates();
    static Program LoadIncludeFile(const std::string& include);
    static std::string NormalizeIncludePath(std::string_view include);

    void ImportInclude(const std::string& include);
    void ImportProgramGates(Program&& program, std::string_view include_name);

    std::unordered_map<std::string, std::uint32_t> qubits_;
    std::unordered_map<std::string, std::uint32_t> registers_;
    std::unordered_map<std::string, const GateDecl&> gates_;
    std::list<Program> include_programs_;
    std::unordered_set<std::string> imported_includes_;
};
Program OpenQASM2Context::LoadQELIB1() {
    // Downloaded from
    // https://github.com/Qiskit/qiskit/blob/0a7cf4b5255463c063e5071eca16954e4398fb29/qiskit/qasm/libs/qelib1.inc
    return ParseOpenQASM2(
#include "qret/frontend/openqasm/qelib1.inc"
    );
}
Program OpenQASM2Context::LoadStdGates() {
    // Reuse qelib1 as a practical compatibility layer for stdgates.
    return LoadQELIB1();
}
Program OpenQASM2Context::LoadIncludeFile(const std::string& include) {
    return ParseOpenQASM2File(include);
}
std::string OpenQASM2Context::NormalizeIncludePath(std::string_view include) {
    if (include == "qelib1.inc" || include == "stdgates.inc") {
        return std::string(include);
    }
    return std::filesystem::path(include).lexically_normal().string();
}
void OpenQASM2Context::ImportInclude(const std::string& include) {
    const auto normalized = NormalizeIncludePath(include);
    if (imported_includes_.contains(normalized)) {
        return;
    }
    imported_includes_.emplace(normalized);

    auto program = Program();
    if (normalized == "qelib1.inc") {
        program = LoadQELIB1();
    } else if (normalized == "stdgates.inc") {
        program = LoadStdGates();
    } else {
        try {
            program = LoadIncludeFile(normalized);
        } catch (const std::exception& ex) {
            throw std::runtime_error(
                    fmt::format("failed to load include '{}': {}", normalized, ex.what())
            );
        }
    }

    for (const auto& child_include : program.incls) {
        ImportInclude(child_include);
    }

    ImportProgramGates(std::move(program), normalized);
}
void OpenQASM2Context::ImportProgramGates(Program&& program, std::string_view include_name) {
    include_programs_.emplace_back(std::move(program));
    const auto& include_program = include_programs_.back();
    for (const auto& st : include_program.sts) {
        if (st->t != Statement::Type::GateDecl) {
            continue;
        }
        const auto& gate_decl = *static_cast<const GateDecl*>(st.get());
        if (!AddGateDecl(gate_decl)) {
            throw std::runtime_error(
                    fmt::format(
                            "duplicate identifier '{}' while importing '{}'",
                            gate_decl.name,
                            include_name
                    )
            );
        }
    }
}

void InsertU1(double lambda, const Qubit& arg) {
    static constexpr auto pi = std::numbers::pi;
    if (DoubleEq(lambda, pi)) {
        frontend::gate::Z(arg);
    } else if (DoubleEq(lambda, pi / 2)) {
        frontend::gate::S(arg);
    } else if (DoubleEq(lambda, -pi / 2)) {
        frontend::gate::SDag(arg);
    } else if (DoubleEq(lambda, pi / 4)) {
        frontend::gate::T(arg);
    } else if (DoubleEq(lambda, -pi / 4)) {
        frontend::gate::TDag(arg);
    } else {
        frontend::gate::RZ(arg, lambda, 1e-9);
    }
}
void InsertU2(double phi, double lambda, const Qubit& arg) {
    static constexpr auto pi = std::numbers::pi;
    if (DoubleEq(phi, 0) && DoubleEq(lambda, pi)) {
        frontend::gate::H(arg);
    } else {
        frontend::gate::RZ(arg, lambda - (pi / 2), 1e-9);
        frontend::gate::RX(arg, pi / 2, 1e-9);
        frontend::gate::RZ(arg, phi + (pi / 2), 1e-9);
    }
}
void InsertU3(double theta, double phi, double lambda, const Qubit& arg) {
    static constexpr auto pi = std::numbers::pi;
    if (DoubleEq(theta, pi) && DoubleEq(phi, 0) && DoubleEq(lambda, pi)) {
        frontend::gate::X(arg);
    } else if (DoubleEq(theta, pi) && DoubleEq(phi, pi / 2) && DoubleEq(lambda, pi / 2)) {
        frontend::gate::Y(arg);
    } else if (DoubleEq(phi, -pi / 2) && DoubleEq(lambda, pi / 2)) {
        frontend::gate::RX(arg, theta, 1e-9);
    } else if (DoubleEq(phi, 0) && DoubleEq(lambda, 0)) {
        frontend::gate::RY(arg, theta, 1e-9);
    } else {
        frontend::gate::RZ(arg, lambda, 1e-9);
        frontend::gate::RX(arg, pi / 2, 1e-9);
        frontend::gate::RZ(arg, theta + pi, 1e-9);
        frontend::gate::RX(arg, pi / 2, 1e-9);
        frontend::gate::RZ(arg, phi + (3 * pi), 1e-9);
    }
}
void InsertU(const std::vector<double>& params, const Qubit& arg) {
    assert(params.size() == 3);

    const auto theta = params[0];
    const auto phi = params[1];
    const auto lambda = params[2];

    if (DoubleEq(theta, 0) && DoubleEq(phi, 0)) {
        InsertU1(lambda, arg);
    } else if (DoubleEq(theta, std::numbers::pi / 2)) {
        InsertU2(phi, lambda, arg);
    } else {
        InsertU3(theta, phi, lambda, arg);
    }
}

void RunUop(
        const OpenQASM2Context& context,
        const Uop& uop,
        const std::vector<double>& params,
        const Qubits& args
);

struct GateDeclGen : CircuitGenerator {
    const GateDecl& gate;
    const std::vector<double>& params;
    const OpenQASM2Context& context;

    static inline const char* Name = "__import_from_openqasm2__";
    explicit GateDeclGen(
            CircuitBuilder* builder,
            const GateDecl& gate,
            const std::vector<double>& params,
            const OpenQASM2Context& context
    )
        : CircuitGenerator(builder)
        , gate{gate}
        , params{params}
        , context{context} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        auto key = std::string(Name) + gate.name + "(";
        const char* sep = "";

        for (const auto& param : params) {
            key += std::exchange(sep, ",") + fmt::format("{:.10f}", param);
        }
        key += ")";
        return key;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("arg", Type::Qubit, gate.args.size(), Attribute::Operate);
    }
    Circuit* Generate() const override;
};

bool TryRunIntrinsicGate(const Uop& uop, const std::vector<double>& params, const Qubits& args) {
    if (uop.gate == "U" || uop.gate == "u" || uop.gate == "u3") {
        if (params.size() != 3 || uop.args.size() != 1) {
            throw std::runtime_error("u/U/u3 expects 3 params and 1 qubit");
        }
        InsertU(params, args[0]);
        return true;
    }
    if (uop.gate == "u2") {
        if (params.size() != 2 || uop.args.size() != 1) {
            throw std::runtime_error("u2 expects 2 params and 1 qubit");
        }
        InsertU2(params[0], params[1], args[0]);
        return true;
    }
    if (uop.gate == "u1" || uop.gate == "p" || uop.gate == "rz") {
        if (params.size() != 1 || uop.args.size() != 1) {
            throw std::runtime_error("u1/p/rz expects 1 param and 1 qubit");
        }
        InsertU1(params[0], args[0]);
        return true;
    }
    if (uop.gate == "u0") {
        if (params.size() != 1 || uop.args.size() != 1) {
            throw std::runtime_error("u0 expects 1 param and 1 qubit");
        }
        frontend::gate::I(args[0]);
        return true;
    }
    if (uop.gate == "id") {
        if (!params.empty() || uop.args.size() != 1) {
            throw std::runtime_error("id expects no params and 1 qubit");
        }
        frontend::gate::I(args[0]);
        return true;
    }
    if (uop.gate == "x") {
        if (!params.empty() || uop.args.size() != 1) {
            throw std::runtime_error("x expects no params and 1 qubit");
        }
        frontend::gate::X(args[0]);
        return true;
    }
    if (uop.gate == "y") {
        if (!params.empty() || uop.args.size() != 1) {
            throw std::runtime_error("y expects no params and 1 qubit");
        }
        frontend::gate::Y(args[0]);
        return true;
    }
    if (uop.gate == "z") {
        if (!params.empty() || uop.args.size() != 1) {
            throw std::runtime_error("z expects no params and 1 qubit");
        }
        frontend::gate::Z(args[0]);
        return true;
    }
    if (uop.gate == "h") {
        if (!params.empty() || uop.args.size() != 1) {
            throw std::runtime_error("h expects no params and 1 qubit");
        }
        frontend::gate::H(args[0]);
        return true;
    }
    if (uop.gate == "s") {
        if (!params.empty() || uop.args.size() != 1) {
            throw std::runtime_error("s expects no params and 1 qubit");
        }
        frontend::gate::S(args[0]);
        return true;
    }
    if (uop.gate == "sdg") {
        if (!params.empty() || uop.args.size() != 1) {
            throw std::runtime_error("sdg expects no params and 1 qubit");
        }
        frontend::gate::SDag(args[0]);
        return true;
    }
    if (uop.gate == "t") {
        if (!params.empty() || uop.args.size() != 1) {
            throw std::runtime_error("t expects no params and 1 qubit");
        }
        frontend::gate::T(args[0]);
        return true;
    }
    if (uop.gate == "tdg") {
        if (!params.empty() || uop.args.size() != 1) {
            throw std::runtime_error("tdg expects no params and 1 qubit");
        }
        frontend::gate::TDag(args[0]);
        return true;
    }
    if (uop.gate == "rx") {
        if (params.size() != 1 || uop.args.size() != 1) {
            throw std::runtime_error("rx expects 1 param and 1 qubit");
        }
        frontend::gate::RX(args[0], params[0], 1e-9);
        return true;
    }
    if (uop.gate == "ry") {
        if (params.size() != 1 || uop.args.size() != 1) {
            throw std::runtime_error("ry expects 1 param and 1 qubit");
        }
        frontend::gate::RY(args[0], params[0], 1e-9);
        return true;
    }
    if (uop.gate == "CX" || uop.gate == "cx") {
        if (!params.empty() || uop.args.size() != 2) {
            throw std::runtime_error("cx/CX expects no params and 2 qubits");
        }
        frontend::gate::CX(args[1], args[0]);
        return true;
    }
    if (uop.gate == "cy") {
        if (!params.empty() || uop.args.size() != 2) {
            throw std::runtime_error("cy expects no params and 2 qubits");
        }
        frontend::gate::CY(args[1], args[0]);
        return true;
    }
    if (uop.gate == "cz") {
        if (!params.empty() || uop.args.size() != 2) {
            throw std::runtime_error("cz expects no params and 2 qubits");
        }
        frontend::gate::CZ(args[1], args[0]);
        return true;
    }
    if (uop.gate == "ccx") {
        if (!params.empty() || uop.args.size() != 3) {
            throw std::runtime_error("ccx expects no params and 3 qubits");
        }
        frontend::gate::CCX(args[2], args[0], args[1]);
        return true;
    }
    return false;
}

/**
 * @brief Run uop.
 *
 * @param context OpenQASM2 context
 * @param uop uop
 * @param params parameters
 * @param args arguments
 */
void RunUop(
        const OpenQASM2Context& context,
        const Uop& uop,
        const std::vector<double>& params,
        const Qubits& args
) {
    if (TryRunIntrinsicGate(uop, params, args)) {
        return;
    }

    // user defined gate
    if (!context.Contains(uop.gate)) {
        throw std::runtime_error("unknown gate: " + uop.gate);
    }

    const auto& gate_decl = context.GetGateDecl(uop.gate);
    if (uop.params.size() != gate_decl.params.size()) {
        throw std::runtime_error("param size error");
    }
    if (uop.args.size() != gate_decl.args.size()) {
        throw std::runtime_error("arg size error");
    }
    (*GateDeclGen(args.GetBuilder(), gate_decl, params, context).Generate())(args);
}
Circuit* GateDeclGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }

    const auto get_q = [this](std::string_view arg_name) -> Qubit {
        for (auto i = std::size_t{0}; i < gate.args.size(); ++i) {
            if (arg_name == gate.args[i]) {
                return GetQubits(0)[i];
            }
        }
        throw std::runtime_error(fmt::format("unknown id: {}", arg_name));
    };

    BeginCircuitDefinition();
    for (const auto& gop : gate.gops) {
        const auto type = gop->t;
        if (type == Statement::Type::Uop) {
            const auto& uop = *static_cast<const Uop*>(gop.get());
            // get params
            auto local_params = std::vector<double>();
            for (const auto& exp : uop.params) {
                local_params.emplace_back(EvaluateExp(exp, gate.params, params));
            }
            // get args
            Qubits args = get_q(uop.args.front().symbol);
            for (auto i = std::size_t{1}; i < uop.args.size(); ++i) {
                args += get_q(uop.args[i].symbol);
            }

            RunUop(context, uop, local_params, args);
        } else if (type == Statement::Type::Barrier) {
            // Do nothing
        } else {
            throw std::runtime_error(
                    "Only uop and barrier statement is allowed in the definition of gate"
            );
        }
    }
    return EndCircuitDefinition();
}

struct OpenQASM2Gen : CircuitGenerator {
    std::string_view name;
    const Program& ast;
    OpenQASM2Context context;

    OpenQASM2Gen(CircuitBuilder* builder, std::string_view name, const Program& ast)
        : CircuitGenerator(builder)
        , name{name}
        , ast{ast}
        , context() {
        context.ImportIncludes(ast.incls);
        for (const auto& st : ast.sts) {
            if (st->t == Statement::Type::GateDecl) {
                // Add gate decl
                const auto& gate_decl = *static_cast<const GateDecl*>(st.get());
                const auto success = context.AddGateDecl(gate_decl);
                if (!success) {
                    std::cerr << "failed gate decl: ";
                    gate_decl.Print(std::cerr);
                    std::cerr << std::endl;
                    throw std::runtime_error("id must be unique");
                }
            } else if (st->t == Statement::Type::Decl) {
                // Set argument
                const auto& decl = *static_cast<const Decl*>(st.get());
                const auto success = context.AddDecl(decl);
                if (!success) {
                    std::cerr << "failed decl: ";
                    decl.Print(std::cerr);
                    std::cerr << std::endl;
                    throw std::runtime_error("id must be unique");
                }
            }
        }
    }
    std::string GetName() const override {
        return std::string(name);
    }
    std::string GetCacheKey() const override {
        return std::string(name);
    }
    void SetArgument(Argument& arg) const override {
        for (const auto& st : ast.sts) {
            if (st->t == Statement::Type::Decl) {
                const auto& decl = *static_cast<const Decl*>(st.get());
                if (decl.is_q) {
                    arg.Add(decl.symbol,
                            Circuit::Type::Qubit,
                            decl.num,
                            Circuit::Attribute::Operate);
                } else {
                    arg.Add(decl.symbol,
                            Circuit::Type::Register,
                            decl.num,
                            Circuit::Attribute::Output);
                }
            }
        }
    }
    Circuit* Generate() const override;

private:
    Qubit FromArgToQubit(const qret::openqasm2::Argument& arg, std::size_t index = 0) const {
        auto tmp = GetQubits(arg.symbol);
        return arg.index ? tmp[arg.index.value()] : tmp[index];
    }
    Register FromArgToRegister(const qret::openqasm2::Argument& arg, std::size_t index = 0) const {
        auto tmp = GetRegisters(arg.symbol);
        return arg.index ? tmp[arg.index.value()] : tmp[index];
    }

    void GenerateImpl(const std::unique_ptr<Statement>& st) const {
        if (st->t == Statement::Type::Uop) {
            const auto& uop = *static_cast<const Uop*>(st.get());
            GenerateUop(uop);
        } else if (st->t == Statement::Type::Measure) {
            const auto& measure = *static_cast<const Measure*>(st.get());
            GenerateMeasure(measure);
        } else if (st->t == Statement::Type::Reset) {
            const auto& reset = *static_cast<const Reset*>(st.get());
            GenerateReset(reset);
        } else if (st->t == Statement::Type::Branch) {
            const auto& branch = *static_cast<const Branch*>(st.get());
            GenerateBranch(branch);
        } else if (st->t == Statement::Type::Barrier) {
            const auto& barrier = *static_cast<const Barrier*>(st.get());
            GenerateBarrier(barrier);
        } else {
            // type of statement is Decl or GateDecl
            // do nothing
        }
    }
    void GenerateUop(const Uop& uop) const {
        // get params
        auto params = std::vector<double>();
        for (const auto& exp : uop.params) {
            params.emplace_back(EvaluateExp(exp, {}, {}));
        }
        // loop of args
        auto size = std::size_t{1};
        for (const auto& arg : uop.args) {
            if (!arg.index.has_value()) {
                const auto tmp_size = GetQubits(arg.symbol).Size();
                if (size != 1 && size != tmp_size) {
                    throw std::runtime_error("different arg size");
                }
                size = tmp_size;
            }
        }
        for (auto i = std::size_t{0}; i < size; ++i) {
            Qubits args = FromArgToQubit(uop.args[0], i);
            for (auto j = std::size_t{1}; j < uop.args.size(); ++j) {
                args += FromArgToQubit(uop.args[j], i);
            }

            RunUop(context, uop, params, args);
        }
    }
    void GenerateMeasure(const Measure& measure) const {
        if (measure.q.index && measure.c.index) {
            const auto q = FromArgToQubit(measure.q);
            const auto c = FromArgToRegister(measure.c);
            frontend::gate::Measure(q, c);
        } else if (!measure.q.index && !measure.c.index) {
            const auto qs = GetQubits(measure.q.symbol);
            const auto cs = GetRegisters(measure.c.symbol);
            if (qs.Size() != cs.Size()) {
                throw std::runtime_error("different size");
            }
            for (auto i = std::size_t{0}; i < qs.Size(); ++i) {
                frontend::gate::Measure(qs[i], cs[i]);
            }
        } else {
            throw std::runtime_error("type is different");
        }
    }
    void GenerateReset(const Reset& reset) const {
        const auto q = FromArgToQubit(reset.q);
        const auto c = GetTemporalRegister();
        ::qret::frontend::gate::Measure(q, c);
        control_flow::If(c);
        ::qret::frontend::gate::X(q);
        control_flow::EndIf(c);
    }
    void GenerateBranch(const Branch& branch) const {
        const auto regs = GetRegisters(branch.symbol);
        const auto vals = BigIntAsBoolArray(branch.value, regs.Size());
        if (BoolArrayAsBigInt(vals) != branch.value) {
            // it's not possible to satisfy <id> == <nninteger>
            return;
        }

        for (auto i = std::size_t{0}; i < regs.Size(); ++i) {
            control_flow::If(regs[i]);
            if (!vals[i]) {
                control_flow::Else(regs[i]);
            }
        }
        GenerateImpl(branch.op);
        for (auto j = std::size_t{0}; j < regs.Size(); ++j) {
            const auto i = regs.Size() - j - 1;
            control_flow::EndIf(regs[i]);
        }
    }
    void GenerateBarrier(const Barrier& /* barrier */) const {
        // TODO: Implement
    }
};
Circuit* OpenQASM2Gen::Generate() const {
    BeginCircuitDefinition();
    // Insert instructions to frontend::Circuit
    for (const auto& st : ast.sts) {
        GenerateImpl(st);
    }
    return EndCircuitDefinition();
}

Circuit*
BuildCircuitFromAST(const Program& ast, CircuitBuilder& builder, std::string_view entry_name) {
    if (!ast.IsQASM2() && !ast.IsQASM3()) {
        LOG_WARN("OpenQASM version is not specified");
    }

    auto gen = OpenQASM2Gen(&builder, entry_name, ast);
    return gen.Generate();
}
}  // namespace qret::frontend
