#include "qret/algorithm/phase_estimation/qpe.h"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "qret/algorithm/arithmetic/integer.h"
#include "qret/algorithm/data/qrom.h"
#include "qret/algorithm/transform/qft.h"
#include "qret/base/type.h"
#include "qret/frontend/attribute.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/functor.h"
#include "qret/frontend/intrinsic.h"
#include "qret/ir/json.h"
#include "qret/math/integer.h"
#include "qret/math/lcu_helper.h"
#include "qret/runtime/simulator.h"

using namespace qret;
using frontend::CircuitBuilder, frontend::CircuitGenerator, runtime::SimulatorConfig,
        runtime::Simulator;

enum class Pauli : std::uint8_t { I, X, Y, Z };
std::string ToString(Pauli p) {
    switch (p) {
        case Pauli::I:
            return "I";
        case Pauli::X:
            return "X";
        case Pauli::Y:
            return "Y";
        case Pauli::Z:
            return "Z";
        default:
            throw std::logic_error("unreachable");
    }
}
Pauli PauliFromString(const std::string& str) {
    if (str == "I") {
        return Pauli::I;
    } else if (str == "X") {
        return Pauli::X;
    } else if (str == "Y") {
        return Pauli::Y;
    } else if (str == "Z") {
        return Pauli::Z;
    } else {
        throw std::runtime_error("Unknown Pauli: " + str);
    }
}
using PauliArray = std::vector<std::vector<Pauli>>;

struct PhaseEstimationGen : CircuitGenerator {
    // size of paulis and alias sampling is the number of terms in lcu-hamiltonian
    std::span<const PauliArray> paulis;
    std::span<const std::pair<std::uint32_t, BigInt>> alias_sampling;
    std::size_t hadamard_size;
    std::size_t system_size;
    std::size_t n;  // BitSizeI(paulis.size() - 1)
    std::size_t m;  // precision of PREPARE

    static inline const char* Name = "PhaseEstimation";
    explicit PhaseEstimationGen(
            CircuitBuilder* builder,
            std::span<const PauliArray> paulis,
            std::span<const std::pair<std::uint32_t, BigInt>> alias_sampling,
            std::size_t hadamard_size,
            std::size_t system_size,
            std::size_t n,
            std::size_t m
    )
        : CircuitGenerator(builder)
        , paulis{paulis}
        , alias_sampling{alias_sampling}
        , hadamard_size{hadamard_size}
        , system_size{system_size}
        , n{n}
        , m{m} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;  // Do not cache
    void SetArgument(Argument& arg) const override {
        arg.Add("system", Type::Qubit, system_size, Attribute::Operate);
        arg.Add("hadamard", Type::Qubit, hadamard_size, Attribute::Operate);
        arg.Add("index", Type::Qubit, n, Attribute::Operate);
        arg.Add("alternate_index", Type::Qubit, n, Attribute::Operate);
        arg.Add("random", Type::Qubit, m, Attribute::Operate);
        arg.Add("switch_weight", Type::Qubit, m, Attribute::Operate);
        arg.Add("cmp", Type::Qubit, 1, Attribute::Operate);
        arg.Add("aux", Type::Qubit, n + 3, Attribute::CleanAncilla);
        arg.Add("measure_system", Type::Register, system_size, Attribute::Output);
        arg.Add("measure_hadamard", Type::Register, hadamard_size, Attribute::Output);
        arg.Add("measure_index", Type::Register, n, Attribute::Output);
    }
    frontend::Circuit* Generate() const override {
        using frontend::gate::Measure, frontend::gate::X, frontend::gate::H, frontend::gate::RX,
                frontend::gate::CX, frontend::gate::CY, frontend::gate::CZ,
                frontend::gate::EqualToImm;

        using frontend::gate::FourierTransform,  // QFT
                frontend::gate::PREPARE,  // PREPARE
                frontend::gate::MultiControlledUnaryIterBegin,  // SELECT
                frontend::gate::MultiControlledUnaryIterStep,  // SELECT
                frontend::gate::MultiControlledUnaryIterEnd;  // SELECT

        BeginCircuitDefinition();
        auto system = GetQubits(0);  // size = system_size
        auto hadamard = GetQubits(1);  // size = hadamard_size
        auto index = GetQubits(2);  // size = n
        auto alternate_index = GetQubits(3);  // size = n
        auto random = GetQubits(4);  // size = m
        auto switch_weight = GetQubits(5);  // size = m
        auto cmp = GetQubit(6);  // size = 1
        auto aux = GetQubits(7);  // size = n+3
        auto measure_system = GetRegisters(8);  // size = system_size
        auto measure_hadamard = GetRegisters(9);  // size = hadamard_size
        auto measure_index = GetRegisters(10);  // size = n

        const auto num_paulis = paulis.size();

        const auto apply_pauli = [](const auto& c, const auto& q, std::span<const Pauli> pauli) {
            for (const auto& p : pauli) {
                switch (p) {
                    case Pauli::X:
                        CX(q, c);
                        break;
                    case Pauli::Y:
                        CY(q, c);
                        break;
                    case Pauli::Z:
                        CZ(q, c);
                        break;
                    default:
                        break;
                }
            }
        };
        const auto apply_paulis = [apply_pauli](
                                          const auto& c,
                                          const auto& qs,
                                          std::span<const std::vector<Pauli>> paulis
                                  ) {
            assert(qs.Size() == paulis.size());
            for (auto i = std::size_t{0}; i < qs.Size(); ++i) {
                apply_pauli(c, qs[i], paulis[i]);
            }
        };

        for (const auto& q : hadamard) {
            H(q);
        }

        PREPARE(alias_sampling, index, alternate_index, random, switch_weight, cmp, aux);

        for (auto i = std::size_t{0}; i < hadamard_size; ++i) {
            // Apply SELECT-based unitary for 2^i times
            const auto& ctrl = hadamard[i];

            for (auto _ = std::size_t{0}; _ < std::size_t{1} << i; ++_) {
                // loop unitary for 2^i times
                // U^(2^i)
                auto aux_sel = aux.Range(0, n);
                MultiControlledUnaryIterBegin(0, ctrl, index, aux_sel);
                for (auto k = std::size_t{0}; k < num_paulis - 1; ++k) {
                    apply_paulis(aux_sel[0], system, paulis[k]);
                    MultiControlledUnaryIterStep(k, ctrl, index, aux_sel);
                }
                apply_paulis(aux_sel[0], system, paulis[num_paulis - 1]);
                MultiControlledUnaryIterEnd(num_paulis - 1, ctrl, index, aux_sel);

                for (const auto& q : aux) {
                    MARK_AS_CLEAN(q);
                }

                Adjoint(PREPARE)(
                        alias_sampling,
                        index,
                        alternate_index,
                        random,
                        switch_weight,
                        cmp,
                        aux
                );
                // multi controlled Z
                {
                    EqualToImm(0, index, aux[0], aux.Range(1, n - 1));
                    CZ(ctrl, aux[0]);
                    EqualToImm(0, index, aux[0], aux.Range(1, n - 1));  // uncomputation
                }
                PREPARE(alias_sampling, index, alternate_index, random, switch_weight, cmp, aux);
            }
        }

        Adjoint(FourierTransform)(hadamard);
        Adjoint(PREPARE)(alias_sampling, index, alternate_index, random, switch_weight, cmp, aux);

        for (auto i = std::size_t{0}; i < hadamard_size; ++i) {
            Measure(hadamard[i], measure_hadamard[i]);
        }
        for (auto i = std::size_t{0}; i < system_size; ++i) {
            Measure(system[i], measure_system[i]);
        }
        for (auto i = std::size_t{0}; i < n; ++i) {
            Measure(index[i], measure_index[i]);
        }

        return EndCircuitDefinition();
    }
};

void TestQPE() {
    // Settings of hamiltonian
    // 0.25(I \otimes Z + Z \otimes Z + i I \otimes Z - i Z \otimes Z)
    // iZ = XY
    // -Z = XYXYZ
    const auto system_size = std::size_t{2};
    const auto dict = std::vector<PauliArray>{
            {{Pauli::Z}, {Pauli::I}},
            {{Pauli::Z}, {Pauli::Z}},
            {{Pauli::Y, Pauli::X}, {Pauli::I}},
            {{Pauli::Y, Pauli::X}, {Pauli::Z, Pauli::Y, Pauli::X, Pauli::Y, Pauli::X}}
    };
    const auto lcu_coefficients = std::vector<double>{0.25, 0.25, 0.25, 0.25};
    const auto size = math::BitSizeI(lcu_coefficients.size() - 1);  // size = 2

    // Settings of PREPARE
    const auto sub_bit_precision = 3;
    const auto alias_sampling = math::PreprocessLCUCoefficientsForReversibleSampling(
            lcu_coefficients,
            sub_bit_precision
    );

    // Settings of QPE
    const auto hadamard_size = std::size_t{3};

    // Generate circuit
    ir::IRContext context;
    auto* module = ir::Module::Create("qpe", context);
    auto builder = CircuitBuilder(module);
    auto gen = PhaseEstimationGen(
            &builder,
            dict,
            alias_sampling,
            hadamard_size,
            system_size,
            size,
            sub_bit_precision
    );
    auto* circuit = gen.Generate();

    std::cout << "Save circuit to qpe.json" << std::endl;
    auto ofs = std::ofstream("qpe.json");
    if (!ofs.good()) {
        std::cerr << "Failed to write to qpe.json" << std::endl;
    } else {
        auto j = Json();
        to_json(j, *module);
        ofs << j << std::endl;
        ofs.close();
    }

    for (auto system_value = std::size_t{0}; system_value < std::size_t{1} << system_size;
         ++system_value) {
        std::cout << "Simulation start (system = " << system_value << ")" << std::endl;
        const auto config = SimulatorConfig{
                .state_type = SimulatorConfig::StateType::FullQuantum,
                .use_qulacs = true
        };
        auto simulator = Simulator(config, circuit->GetIR());
        simulator.SetValue(0, system_size, system_value);  // Set system
        simulator.RunAll();

        // Print results
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
        std::cout << "hadamard: " << print_state(state.ReadRegisters(system_size, hadamard_size))
                  << std::endl;
        std::cout << "system  : " << print_state(state.ReadRegisters(0, system_size)) << std::endl;
        std::cout << "index   : "
                  << print_state(state.ReadRegisters(system_size + hadamard_size, size))
                  << std::endl;
    }
}

int main() {
    TestQPE();
    return 0;
}
