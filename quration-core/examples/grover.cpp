#include <cassert>
#include <cstddef>
#include <iostream>
#include <numbers>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "qret/base/type.h"
#include "qret/frontend/argument.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/intrinsic.h"
#include "qret/runtime/simulator.h"

using namespace qret;

namespace {

/*
 * Reflects the state about the given marked bit.
 */
class ReflectAboutMarkedGen : frontend::CircuitGenerator {
public:
    std::size_t n;
    std::size_t marked_bit;

    static inline const char* Name = "ReflectAboutMarked";
    explicit ReflectAboutMarkedGen(
            frontend::CircuitBuilder* builder,
            std::size_t n,
            const std::size_t marked_bit
    )
        : CircuitGenerator{builder}
        , n{n}
        , marked_bit{marked_bit} {
        assert(marked_bit < (1 << n));
    }

    std::string GetName() const override {
        return fmt::format("{}_{}", Name, marked_bit);
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}_{}", Name, marked_bit);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("x", Type::Qubit, n, Attribute::Operate);
        arg.Add("y", Type::Qubit, 1, Attribute::Operate);
        arg.Add("aux", Type::Qubit, n > 2 ? n - 1 : 0, Attribute::CleanAncilla);
    }
    frontend::Circuit* Generate() const override {
        assert(n >= 2);

        if (IsCached()) {
            return GetCachedCircuit();
        }
        BeginCircuitDefinition();
        auto x = GetQubits(0);
        auto y = GetQubit(1);

        frontend::gate::X(y);
        frontend::gate::H(y);

        for (std::size_t i = 0; i < n; ++i) {
            // flip the bit if it is not marked
            if (((marked_bit >> i) & 1) == 0) {
                frontend::gate::X(x[i]);
            }
        }

        // multi-controlled X gate(y,x)
        if (n > 2) {
            auto aux = GetQubits(2);

            frontend::gate::CCX(aux[0], x[0], x[1]);
            for (std::size_t i = 2; i < n; ++i) {
                frontend::gate::CCX(aux[i - 1], x[i], aux[i - 2]);
            }

            frontend::gate::CX(y, aux[n - 2]);

            for (std::size_t i = n - 1; i >= 2; --i) {
                frontend::gate::CCX(aux[i - 1], x[i], aux[i - 2]);
            }

            frontend::gate::CCX(aux[0], x[0], x[1]);
        } else {
            frontend::gate::CCX(y, x[0], x[1]);
        }

        for (std::size_t i = 0; i < n; ++i) {
            // flip the bit if it is not marked
            if (((marked_bit >> i) & 1) == 0) {
                frontend::gate::X(x[i]);
            }
        }

        frontend::gate::H(y);
        frontend::gate::X(y);

        return EndCircuitDefinition();
    }
};

void ReflectAboutMarked(
        const frontend::Qubits& x,
        const frontend::Qubit& y,
        const frontend::Qubits& aux,
        const std::size_t& marked_bit
) {
    auto gen = ReflectAboutMarkedGen(x.GetBuilder(), x.Size(), marked_bit);
    auto* circuit = gen.Generate();
    (*circuit)(x, y, aux);
}

class ReflectAboutUniformGen : frontend::CircuitGenerator {
public:
    std::size_t n;

    static inline const char* Name = "ReflectAboutUniform";
    explicit ReflectAboutUniformGen(frontend::CircuitBuilder* builder, std::size_t n)
        : CircuitGenerator{builder}
        , n{n} {}

    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return Name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("x", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, n > 3 ? n - 2 : 0, Attribute::CleanAncilla);
    }
    frontend::Circuit* Generate() const override {
        if (IsCached()) {
            return GetCachedCircuit();
        }
        BeginCircuitDefinition();
        auto x = GetQubits(0);

        auto prepare_uniform = [](const frontend::Qubits& x) {
            for (const auto& q : x) {
                frontend::gate::H(q);
            }
        };

        // reflect
        prepare_uniform(x);
        for (const auto& q : x) {
            frontend::gate::X(q);
        }

        // negate phase about all ones
        if (n > 3) {
            auto aux = GetQubits(1);

            frontend::gate::CCX(aux[0], x[0], x[1]);
            for (std::size_t i = 2; i <= n - 2; ++i) {
                frontend::gate::CCX(aux[i - 1], x[i], aux[i - 2]);
            }

            frontend::gate::CZ(x[n - 1], aux[n - 3]);

            for (std::size_t i = n - 2; i >= 2; --i) {
                frontend::gate::CCX(aux[i - 1], x[i], aux[i - 2]);
            }
            frontend::gate::CCX(aux[0], x[0], x[1]);
        } else {
            frontend::gate::CCZ(x[2], x[0], x[1]);
        }

        // Adjoint reflect
        for (const auto& q : x) {
            frontend::gate::X(q);
        }
        prepare_uniform(x);

        return EndCircuitDefinition();
    }
};

void ReflectAboutUniform(const frontend::Qubits& x, const frontend::Qubits& aux) {
    auto gen = ReflectAboutUniformGen(x.GetBuilder(), x.Size());
    auto* circuit = gen.Generate();
    (*circuit)(x, aux);
}

class GroverSearchGen : frontend::CircuitGenerator {
public:
    std::size_t n;
    std::size_t marked_bit;

    static inline const char* Name = "GroverSearch";
    explicit GroverSearchGen(
            frontend::CircuitBuilder* builder,
            std::size_t n,
            const std::size_t marked_bit
    )
        : CircuitGenerator{builder}
        , n{n}
        , marked_bit{marked_bit} {
        assert(marked_bit < (1 << n));
    }

    std::string GetName() const override {
        return fmt::format("{}_{}", Name, marked_bit);
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}_{}", Name, marked_bit);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("x", Type::Qubit, n, Attribute::Operate);
        arg.Add("y", Type::Qubit, 1, Attribute::CleanAncilla);
        arg.Add("r", Type::Register, n, Attribute::Output);
        arg.Add("aux_marked", Type::Qubit, n > 2 ? n - 1 : 0, Attribute::CleanAncilla);
        arg.Add("aux_uniform", Type::Qubit, n > 3 ? n - 2 : 0, Attribute::CleanAncilla);
    }
    frontend::Circuit* Generate() const override {
        assert(n >= 3);

        if (IsCached()) {
            return GetCachedCircuit();
        }

        const int optimal_iteration = [](std::size_t n) {
            auto n_items = 1 << n;
            const double angle = std::asin(1.0 / std::sqrt(n_items));
            return static_cast<int>(std::round((std::numbers::pi * 0.25 / angle) - 0.5));
        }(n);

        BeginCircuitDefinition();
        auto x = GetQubits(0);
        auto y = GetQubit(1);
        auto r = GetRegisters(2);
        auto aux_marked = GetQubits(3);
        auto aux_uniform = GetQubits(4);

        for (const auto& q : x) {
            frontend::gate::H(q);
        }

        for (auto _ = 0; _ < optimal_iteration; ++_) {
            ReflectAboutMarked(x, y, aux_marked, marked_bit);
            ReflectAboutUniform(x, aux_uniform);
        }

        // measure each z
        for (std::size_t i = 0; i < n; ++i) {
            frontend::gate::Measure(x[i], r[i]);
        }

        return EndCircuitDefinition();
    }
};

void DumpInstructions(ir::Function* func) {
    for (const auto& bb : *func) {
        std::cout << bb.GetName() << std::endl;
        for (const auto& inst : bb) {
            std::cout << "  ";  // Indent
            inst.Print(std::cout);
            std::cout << std::endl;
        }
    }
}

void TestGroverSearch() {
    ir::IRContext context;
    auto* module = ir::Module::Create("GroverSearch ", context);
    auto builder = frontend::CircuitBuilder(module);

    constexpr std::size_t grover_search_size = 5;

    for (std::size_t marked_bit = 0; marked_bit < (1 << grover_search_size); ++marked_bit) {
        auto gen = GroverSearchGen(&builder, grover_search_size, marked_bit);

        auto* circuit = gen.Generate();
        auto* ir_circuit = circuit->GetIR();

        // avoid dumping the circuit multiple times
        if (marked_bit == 0) {
            std::cout << "GroverSearch circuit:" << std::endl;
            DumpInstructions(ir_circuit);
        }

        using runtime::SimulatorConfig, runtime::Simulator;
        const auto config = SimulatorConfig{
                .state_type = SimulatorConfig::StateType::FullQuantum,
        };

        auto simulator = Simulator(config, ir_circuit);
        simulator.SetValue(0, grover_search_size, 0);

        simulator.RunAll();

        const auto& state = simulator.GetState();
        const auto print_state = [](const auto& input) {
            auto ss = std::stringstream();
            ss << "|";
            for (auto i = std::size_t{0}; i < input.size(); ++i) {
                auto i_rev = input.size() - 1 - i;
                ss << input[i_rev];
            }
            ss << ">";
            return ss.str();
        };

        std::vector<bool> marked_bit_vector(grover_search_size, false);
        for (std::size_t b = 0; b < grover_search_size; ++b) {
            if (((marked_bit >> b) & 1) == 1)
                marked_bit_vector[b] = true;
        }

        std::cout << "marked_bit: " << print_state(marked_bit_vector)
                  << ", output: " << print_state(state.ReadRegisters(0, grover_search_size))
                  << std::endl;
    }
}
}  // namespace

int main() {
    TestGroverSearch();
    return 0;
}
