#include <fmt/core.h>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <tuple>
#include <vector>

#include "qret/algorithm/arithmetic/boolean.h"
#include "qret/algorithm/arithmetic/integer.h"
#include "qret/algorithm/data/qrom.h"
#include "qret/algorithm/preparation/uniform.h"
#include "qret/algorithm/util/rotation.h"
#include "qret/base/type.h"
#include "qret/frontend/argument.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/functor.h"
#include "qret/frontend/intrinsic.h"
#include "qret/ir/context.h"
#include "qret/ir/json.h"
#include "qret/ir/module.h"

using namespace qret;
namespace fs = std::filesystem;

std::string ToFilename(double value) {
    auto s = fmt::format("{:.12g}", value);
    for (auto& c : s) {
        if (c == '.') {
            c = 'p';
        } else if (c == '-') {
            c = 'm';
        } else if (c == '+') {
            c = 'p';
        }
    }
    return s;
}

void WriteModule(const frontend::CircuitBuilder& builder, const fs::path& path) {
    fs::create_directories(path.parent_path());
    auto j = Json();
    j = *builder.GetModule();
    auto ofs = std::ofstream(path);
    ofs << std::setw(4) << j;
}

struct TestRGen : public frontend::CircuitGenerator {
    char pauli;
    double theta;
    double precision;

    static inline const char* Name = "TestR";
    explicit TestRGen(frontend::CircuitBuilder* builder, char pauli, double theta, double precision)
        : CircuitGenerator(builder)
        , pauli{pauli}
        , theta{theta}
        , precision{precision} {}
    std::string GetName() const override {
        return Name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 1, Attribute::Operate);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto q = GetQubit(0);

        if (pauli == 'X') {
            frontend::gate::RX(q, theta, precision);
        } else if (pauli == 'Y') {
            frontend::gate::RY(q, theta, precision);
        } else if (pauli == 'Z') {
            frontend::gate::RZ(q, theta, precision);
        } else if (pauli == '1') {
            frontend::gate::R1(q, theta, precision);
        }

        return EndCircuitDefinition();
    }
};

struct ComputeGraphGen : frontend::CircuitGenerator {
    static inline const char* Name = "ApplyXToEach";
    explicit ComputeGraphGen(qret::frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return "CircGen";
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("tgt0", Type::Qubit, 5, Attribute::Operate);
        arg.Add("tgt1", Type::Qubit, 4, Attribute::Operate);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto tgt0 = GetQubits("tgt0");
        auto tgt1 = GetQubits("tgt1");
        frontend::gate::X(tgt0[0]);
        frontend::gate::CX(tgt0[0], tgt0[1]);
        frontend::gate::MCX(tgt1[2], tgt0.Range(3, 2) + tgt1.Range(0, 2) + tgt1[3]);
        frontend::gate::TemporalAnd(tgt0[0], tgt0[1], tgt0[2]);
        return EndCircuitDefinition();
    }
};

struct InlinerTestGen : frontend::CircuitGenerator {
    static inline const char* Name = "Test";
    explicit InlinerTestGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return Name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 10, Attribute::Operate);
        arg.Add("r", Type::Register, 2, Attribute::Output);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto q = GetQubits(0);
        auto r = GetRegisters(1);

        frontend::gate::TemporalAnd(q[0], q[1], q[2]);
        frontend::gate::Measure(q[0], r[0]);

        frontend::control_flow::If(r[0]);
        { frontend::gate::MAJ(q[0], q[1], q[2]); }
        frontend::control_flow::Else(r[0]);
        { frontend::gate::UMA(q[0], q[1], q[2]); }
        frontend::control_flow::EndIf(r[0]);

        frontend::gate::Measure(q[2], r[1]);

        frontend::control_flow::If(r[1]);
        { frontend::gate::AddCuccaro(q.Range(0, 4), q.Range(4, 4), q[8]); }
        frontend::control_flow::Else(r[1]);
        { frontend::gate::UncomputeTemporalAnd(q[0], q[1], q[2]); }
        frontend::control_flow::EndIf(r[1]);

        frontend::gate::UMA(q[2], q[0], q[1]);

        return EndCircuitDefinition();
    }
};

struct AdjointGen : frontend::CircuitGenerator {
    static inline const char* Name = "Adjoint";
    explicit AdjointGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return Name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("a", Type::Qubit, 1, Attribute::Operate);
        arg.Add("b", Type::Qubit, 1, Attribute::Operate);
        arg.Add("c", Type::Qubit, 1, Attribute::Operate);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto a = GetQubit(0);
        auto b = GetQubit(1);
        auto c = GetQubit(2);

        frontend::Adjoint(frontend::Adjoint(frontend::Adjoint(frontend::gate::MAJ)))(a, b, c);
        frontend::Adjoint(frontend::Adjoint(frontend::gate::MAJ))(a, b, c);
        frontend::Adjoint(frontend::gate::MAJ)(a, b, c);

        return EndCircuitDefinition();
    }
};

struct ControlledGen : frontend::CircuitGenerator {
    static inline const char* Name = "Controlled";
    explicit ControlledGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return Name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("a", Type::Qubit, 1, Attribute::Operate);
        arg.Add("b", Type::Qubit, 1, Attribute::Operate);
        arg.Add("c", Type::Qubit, 1, Attribute::Operate);
        arg.Add("d", Type::Qubit, 1, Attribute::Operate);
        arg.Add("e", Type::Qubit, 1, Attribute::Operate);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto a = GetQubit(0);
        auto b = GetQubit(1);
        auto c = GetQubit(2);
        auto d = GetQubit(3);
        auto e = GetQubit(4);

        frontend::Controlled(frontend::gate::MAJ)(d, a, b, c);
        frontend::Controlled(frontend::Controlled(frontend::gate::MAJ))(e, d, a, b, c);

        return EndCircuitDefinition();
    }
};

struct PivotFlipUsingControlledFunctorGen : frontend::CircuitGenerator {
    std::size_t n;
    std::size_t m;

    static inline const char* Name = "PivotFlipUsingControlledFunctor";
    explicit PivotFlipUsingControlledFunctorGen(
            frontend::CircuitBuilder* builder,
            std::size_t n,
            std::size_t m
    )
        : CircuitGenerator(builder)
        , n{n}
        , m{m} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{})", GetName(), n, m);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("dst", Type::Qubit, n, Attribute::Operate);
        arg.Add("src", Type::Qubit, m, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 2, Attribute::DirtyAncilla);
    }
    frontend::Circuit* Generate() const override {
        if (IsCached()) {
            return GetCachedCircuit();
        }
        BeginCircuitDefinition();
        auto dst = GetQubits(0);  // size = n >= m
        auto src = GetQubits(1);  // size = m
        auto aux = GetQubits(2);  // size = 2

        // less than
        {
            frontend::gate::SubGeneral(dst + aux[0], src);
            frontend::gate::AddGeneral(dst, src);
        }
        // controlled bi flip
        frontend::Controlled(frontend::gate::BiFlip)(aux[0], dst, src);
        // less than
        {
            frontend::gate::SubGeneral(dst + aux[0], src);
            frontend::gate::AddGeneral(dst, src);
        }
        // controlled bi flip
        frontend::Controlled(frontend::gate::BiFlip)(aux[0], dst, src);

        return EndCircuitDefinition();
    }
};

struct TestClassicalFunctionGen : frontend::CircuitGenerator {
    const std::vector<BigInt>& dict;
    std::uint64_t address_size;
    std::uint64_t value_size;

    static inline const char* Name = "TestClassicalFunction";
    explicit TestClassicalFunctionGen(
            frontend::CircuitBuilder* builder,
            std::uint64_t address_size,
            std::uint64_t value_size,
            const std::vector<BigInt>& dict
    )
        : CircuitGenerator(builder)
        , address_size{address_size}
        , value_size{value_size}
        , dict{dict} {}
    std::string GetName() const override {
        return Name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("address", Type::Qubit, address_size, Attribute::Operate);
        arg.Add("value", Type::Qubit, value_size, Attribute::Operate);
        arg.Add("aux", Type::Qubit, address_size - 1, Attribute::CleanAncilla);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto address = GetQubits(0);
        auto value = GetQubits(1);
        auto aux = GetQubits(2);

        qret::frontend::gate::QROMImm(address, value, aux, dict);
        qret::frontend::gate::UncomputeQROMImm(address, value, aux, dict);

        return EndCircuitDefinition();
    }
};

void GenerateAdders(const fs::path& out_dir) {
    const std::vector<std::size_t> cuccaro_sizes = {1, 2, 3, 5, 1000};
    for (const auto size : cuccaro_sizes) {
        {
            ir::IRContext context;
            auto* module = ir::Module::Create("add_cuccaro", context);
            auto builder = frontend::CircuitBuilder(module);
            frontend::gate::AddCuccaroGen(&builder, size).Generate();
            WriteModule(builder, out_dir / fmt::format("add_cuccaro_{}.json", size));
        }
    }
    const std::vector<std::size_t> craig_sizes = {1, 2, 5, 1000};
    for (const auto size : craig_sizes) {
        {
            ir::IRContext context;
            auto* module = ir::Module::Create("add_craig", context);
            auto builder = frontend::CircuitBuilder(module);
            frontend::gate::AddCraigGen(&builder, size).Generate();
            WriteModule(builder, out_dir / fmt::format("add_craig_{}.json", size));
        }
    }
}

void GeneratePrepareUniform(const fs::path& out_dir) {
    for (auto N = 2; N <= 32; ++N) {
        ir::IRContext context;
        auto* module = ir::Module::Create("prepare_uniform", context);
        auto builder = frontend::CircuitBuilder(module);
        frontend::gate::PrepareUniformSuperpositionGen(&builder, N).Generate();
        WriteModule(builder, out_dir / "uniform" / fmt::format("prepare_uniform_{}.json", N));
    }
}

void GenerateRotation(const fs::path& out_dir) {
    const auto params = std::vector<std::tuple<double, double>>{
            {0.01, 0.0001},
            {0.02, 0.0001},
            {0.03, 0.0001},
            {0.1, 0.0001},
            {0.2, 0.0001},
            {0.3, 0.0001},
            {1, 0.0001},
            {2, 0.0001},
            {3, 0.0001},
            {0.01, 0.0000000001},
            {0.02, 0.0000000001},
            {0.03, 0.0000000001},
            {0.1, 0.0000000001},
            {0.2, 0.0000000001},
            {0.3, 0.0000000001},
            {1, 0.0000000001},
            {2, 0.0000000001},
            {3, 0.0000000001},
    };
    for (const auto& [theta, precision] : params) {
        for (const auto pauli : {'X', 'Y', 'Z', '1'}) {
            ir::IRContext context;
            auto* module = ir::Module::Create("test_r", context);
            auto builder = frontend::CircuitBuilder(module);
            TestRGen(&builder, pauli, theta, precision).Generate();
            const auto filename = fmt::format(
                    "rotation/test_r_{}_theta_{}_precision_{}.json",
                    pauli,
                    ToFilename(theta),
                    ToFilename(precision)
            );
            WriteModule(builder, out_dir / filename);
        }
    }
}

void GenerateComputeGraph(const fs::path& out_dir) {
    ir::IRContext context;
    auto* module = ir::Module::Create("compute_graph", context);
    auto builder = frontend::CircuitBuilder(module);
    ComputeGraphGen(&builder).Generate();
    WriteModule(builder, out_dir / "compute_graph.json");
}

void GenerateInliner(const fs::path& out_dir) {
    ir::IRContext context;
    auto* module = ir::Module::Create("inliner", context);
    auto builder = frontend::CircuitBuilder(module);
    InlinerTestGen(&builder).Generate();
    WriteModule(builder, out_dir / "inliner_test.json");
}

void GenerateFunctor(const fs::path& out_dir) {
    {
        ir::IRContext context;
        auto* module = ir::Module::Create("functor_adjoint", context);
        auto builder = frontend::CircuitBuilder(module);
        AdjointGen(&builder).Generate();
        WriteModule(builder, out_dir / "functor_adjoint.json");
    }
    {
        ir::IRContext context;
        auto* module = ir::Module::Create("functor_controlled", context);
        auto builder = frontend::CircuitBuilder(module);
        ControlledGen(&builder).Generate();
        WriteModule(builder, out_dir / "functor_controlled.json");
    }
    {
        ir::IRContext context;
        auto* module = ir::Module::Create("functor_pivot_flip", context);
        auto builder = frontend::CircuitBuilder(module);
        PivotFlipUsingControlledFunctorGen(&builder, 6, 5).Generate();
        WriteModule(builder, out_dir / "functor_pivot_flip_6_5.json");
    }
}

void GenerateClassicalFunction(const fs::path& out_dir) {
    const auto address_size = 5;
    const auto value_size = 6;
    const auto max_value = BigInt{1} << value_size;
    const auto get_value = [max_value](const auto x) -> BigInt {
        return (x * BigInt{12323474} + 234956789) % max_value;
    };
    auto dict = std::vector<BigInt>();
    dict.reserve(std::size_t{1} << address_size);
    for (auto address_value = 0; address_value < (1 << address_size); ++address_value) {
        dict.emplace_back(get_value(address_value));
    }

    ir::IRContext context;
    auto* module = ir::Module::Create("classical_function", context);
    auto builder = frontend::CircuitBuilder(module);
    TestClassicalFunctionGen(&builder, address_size, value_size, dict).Generate();
    WriteModule(builder, out_dir / "test_classical_function.json");
}

int main(int argc, char** argv) {
    const auto out_dir =
            (argc > 1) ? fs::path(argv[1]) : fs::path("quration-core/tests/data/circuit");

    GenerateAdders(out_dir);
    GeneratePrepareUniform(out_dir);
    GenerateRotation(out_dir);
    GenerateComputeGraph(out_dir);
    GenerateInliner(out_dir);
    GenerateFunctor(out_dir);
    GenerateClassicalFunction(out_dir);

    return 0;
}
