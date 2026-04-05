#include <fmt/core.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <numbers>
#include <ostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "qret/algorithm/arithmetic/modular.h"
#include "qret/analysis/counter.h"
#include "qret/base/json.h"
#include "qret/base/type.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/control_flow.h"
#include "qret/frontend/intrinsic.h"
#include "qret/ir/json.h"
#include "qret/math/fraction.h"
#include "qret/math/integer.h"
#include "qret/runtime/simulator.h"

using namespace qret;
using frontend::CircuitBuilder, frontend::CircuitGenerator, runtime::SimulatorConfig,
        runtime::Simulator;

template <typename T>
std::string ToString(const T& x) {
    auto ss = std::stringstream();
    ss << x;
    return ss.str();
}

struct PeriodFinderGen : CircuitGenerator {
    BigInt mod;
    BigInt coprime;
    std::size_t depth;
    std::size_t n;  // n = BitSizeL(mod)

    static inline const char* Name = "PeriodFinder";
    explicit PeriodFinderGen(
            CircuitBuilder* builder,
            const BigInt& mod,
            const BigInt& coprime,
            std::size_t depth
    )
        : CircuitGenerator(builder)
        , mod{mod}
        , coprime{coprime}
        , depth{depth}
        , n{math::BitSizeL(mod)} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;  // Do not cache
    void SetArgument(Argument& arg) const override {
        arg.Add("caux", Type::Qubit, n + 2, Attribute::Operate);
        arg.Add("daux", Type::Qubit, n - 1, Attribute::Operate);
        arg.Add("out", Type::Register, depth, Attribute::Output);
    }
    frontend::Circuit* Generate() const override {
        using frontend::control_flow::If, frontend::control_flow::EndIf;
        using frontend::gate::Measure, frontend::gate::X, frontend::gate::H, frontend::gate::RX,
                frontend::gate::MultiControlledModBiMulImm;

        // Implements fig4 of https://arxiv.org/abs/1706.07884

        BeginCircuitDefinition();
        auto caux = GetQubits(0);
        auto daux = GetQubits(1);
        auto out = GetRegisters(2);

        auto lhs = caux.Range(0, n);
        auto rhs = daux + caux[n];
        auto ctrl = caux[n + 1];

        auto multipliers = std::vector<BigInt>(depth);
        multipliers.back() = coprime;
        for (auto i = static_cast<std::int32_t>(depth) - 2; i >= 0; --i) {
            multipliers[i] = (multipliers[i + 1] * multipliers[i + 1]) % mod;
        }

        X(lhs[0]);

        for (auto i = std::size_t{0}; i < depth; ++i) {
            const auto& mul = multipliers[i];

            H(ctrl);
            MultiControlledModBiMulImm(mod, mul, ctrl, lhs, rhs);
            H(ctrl);

            for (auto j = std::size_t{0}; j < i; ++j) {
                const auto theta = -std::pow(0.5, i - j) * std::numbers::pi;
                If(out[j]);
                RX(ctrl, theta);
                EndIf(out[j]);
            }

            // Measurement
            Measure(ctrl, out[i]);
            If(out[i]);
            X(ctrl);
            EndIf(out[i]);
        }

        return EndCircuitDefinition();
    }
};

std::set<BigInt> FindPeriod(const BigInt& mod, const BigInt& coprime, const ir::Function* func) {
    if (mod % 2 == BigInt{0}) {
        throw std::runtime_error("mod should be even.");
    }

    const auto num_samples = std::size_t{100};
    const auto size = math::BitSizeL(mod);
    const auto depth = 2 * size;
    const auto denom = BigInt{1} << depth;
    auto result_map = std::map<BigInt, std::size_t>();
    auto period_map = std::map<BigInt, std::size_t>();

    // Generate circuit
    ir::IRContext context;
    auto* module = ir::Module::Create("shor", context);
    auto builder = CircuitBuilder(module);
    auto gen = PeriodFinderGen(&builder, mod, coprime, depth);

    for (auto i = std::size_t{0}; i < num_samples; ++i) {
        // Run circuit and get measurement results
        const auto config = SimulatorConfig{
                .state_type = SimulatorConfig::StateType::FullQuantum,
                .seed = i,
                .use_qulacs = true
        };
        auto simulator = Simulator(config, func);
        simulator.RunAll();

        const auto& state = simulator.GetState();
        const auto result_out = state.ReadRegisters(0, depth);
        const auto val = BoolArrayAsBigInt(result_out);
        const auto fraction = math::ContinuedFractionConvergentL(BigFraction(val, denom), mod);
        fmt::print(
                fmt::runtime("Measurement: {:>10}, Approximate fraction: {:>10}\n"),
                static_cast<std::int32_t>(val),
                ToString(fraction)
        );

        if (result_map.contains(val)) {
            result_map[val]++;
        } else {
            result_map[val] = 1;
        }
        const auto& period = fraction.denominator();
        if (period_map.contains(period)) {
            period_map[period]++;
        } else {
            period_map[period] = 1;
        }
    }

    // Print results
    std::cout << "Frequency map" << std::endl;
    for (const auto& [key, num] : result_map) {
        std::cout << key << " : " << num << std::endl;
    }
    std::cout << "Period frequency map" << std::endl;
    auto candidates = std::set<BigInt>();
    for (const auto& [key, num] : period_map) {
        /// \todo (FIXME) If the number of divisors is large, the following conditional
        /// expression may not be met
        if (num >= num_samples / 10) {
            candidates.insert(key);
        }
        std::cout << key << " : " << num << std::endl;
    }
    return candidates;
}

int main() {
    const auto mod = BigInt{25};
    const auto coprime = BigInt{12};

    auto period = std::uint32_t{0};
    std::cout << "=============================" << std::endl;
    std::cout << "Search period:" << std::endl;
    for (auto exp = 1u; exp <= 30u; ++exp) {
        const auto modpow = math::ModPow(coprime, exp, mod);
        if (modpow == BigInt{1}) {
            // Found period!
            if (period == std::uint32_t{0}) {
                period = exp;
            }
            std::cout << "    ";
        }
        std::cout << coprime << "^" << exp << " mod " << mod << " = " << modpow << std::endl;
    }

    if (period != std::uint32_t{0}) {
        std::cout << "=============================" << std::endl;
        std::cout << "Period: " << period << std::endl;
        const auto width = math::BitSizeL(mod);
        const auto pow = std::size_t{1} << (2 * width);
        std::cout << "Expected measurement results:" << std::endl;
        for (auto i = 0u; i < period; ++i) {
            std::cout << std::round(static_cast<double>(i * pow) / static_cast<double>(period))
                      << std::endl;
        }
    }

    std::cout << "=============================" << std::endl;
    std::cout << "Generate find period circuit:" << std::endl;
    ir::IRContext context;
    auto* module = ir::Module::Create("shor", context);
    auto builder = CircuitBuilder(module);
    const auto size = math::BitSizeL(mod);
    const auto depth = 2 * size;
    auto gen = PeriodFinderGen(&builder, mod, coprime, depth);
    auto* circuit = gen.Generate();
    std::cout << "Count the number of instructions:" << std::endl;
    CountInst(*circuit->GetIR()).Print(std::cout);

    std::cout << "Save circuit to shor.json" << std::endl;
    auto ofs = std::ofstream("shor.json");
    if (!ofs.good()) {
        std::cerr << "Failed to write to shor.json" << std::endl;
    } else {
        auto j = Json();
        to_json(j, *module);
        ofs << j << std::endl;
        ofs.close();
    }

    std::cout << "=============================" << std::endl;
    std::cout << "Run find period circuit:" << std::endl;
    const auto candidates = FindPeriod(mod, coprime, circuit->GetIR());
    std::cout << "Candidates of period: " << std::endl;
    for (const auto& x : candidates) {
        std::cout << x << std::endl;
    }

    return 0;
}
