#include <boost/program_options.hpp>
#include <nlohmann/json.hpp>

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "qret/frontend/argument.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/intrinsic.h"
#include "qret/ir/instruction.h"
#include "qret/ir/json.h"
#include "qret/transforms/ipo/inliner.h"

using json = nlohmann::json;
using namespace qret;
using frontend::CircuitBuilder, frontend::CircuitGenerator;

enum class PauliChars : char { X = 'X', Y = 'Y', Z = 'Z' };
std::string PauliOpToStr(const PauliChars op) {
    switch (op) {
        case PauliChars::X:
            return "X";
        case PauliChars::Y:
            return "Y";
        case PauliChars::Z:
            return "Z";
        default: {
            throw std::runtime_error("Invalid PauliOp");
        }
    }
}
struct PauliTerm {
    double coeff;
    std::map<std::size_t, PauliChars> pauli_string;

    PauliTerm() = default;
    explicit PauliTerm(double coeff, std::map<std::size_t, PauliChars>& pauli_string)
        : coeff(coeff)
        , pauli_string(pauli_string) {

        };
};
void from_json(const json& j, PauliTerm& pauli_term) {
    j.at("coeff").get_to(pauli_term.coeff);

    pauli_term.pauli_string.clear();
    for (const auto& item : j.at("pauli_string").items()) {
        const auto qubit_index = static_cast<std::size_t>(std::stoull(item.key()));

        const auto s = item.value().get_ref<const std::string&>();
        pauli_term.pauli_string[qubit_index] = static_cast<PauliChars>(s[0]);
    }
}
void to_json(json& j, const PauliTerm& pauli_term) {
    json pauli_string_json;
    for (const auto& [qubit_index, pauli_op] : pauli_term.pauli_string) {
        pauli_string_json[std::to_string(qubit_index)] = PauliOpToStr(pauli_op);
    }

    j["coeff"] = pauli_term.coeff;
    j["pauli_string"] = pauli_string_json;
}

struct Hamiltonian {
    std::size_t num_qubits;
    std::vector<PauliTerm> pauli_terms;

    Hamiltonian() = default;
    explicit Hamiltonian(std::size_t num_qubits, std::vector<PauliTerm>& pauli_terms)
        : num_qubits(num_qubits)
        , pauli_terms(pauli_terms) {};
};
void from_json(const json& j, Hamiltonian& h) {
    j.at("num_qubits").get_to(h.num_qubits);
    j.at("pauli_terms").get_to(h.pauli_terms);
}
void to_json(json& j, const Hamiltonian& h) {
    j["num_qubits"] = h.num_qubits;
    j["pauli_terms"] = h.pauli_terms;
}
Hamiltonian ReadHamiltonian(const std::string& path) {
    auto ifs = std::ifstream(path.data());
    if (!ifs.good()) {
        throw std::runtime_error("Could not open file: " + path);
    }
    json j;
    ifs >> j;
    return j.get<Hamiltonian>();
}
struct PauliRotationGen : CircuitGenerator {
    std::size_t num_qubits;
    std::map<std::size_t, PauliChars> pauli_string;
    double angle;

    explicit PauliRotationGen(
            CircuitBuilder* builder,
            const std::size_t num_qubits,
            const std::map<std::size_t, PauliChars>& pauli_string,
            const double angle
    )
        : CircuitGenerator(builder)
        , num_qubits(num_qubits)
        , pauli_string(pauli_string)
        , angle(angle) {}
    std::string GetName() const override {
        auto serialized_pauli_string = std::string();
        for (const auto& pair : pauli_string) {
            serialized_pauli_string += fmt::format("{}{}", pair.first, PauliOpToStr(pair.second));
        }
        return fmt::format("pauli_rotation({}, {:.4f})", serialized_pauli_string, angle);
    }
    std::string GetCacheKey() const override {
        return GetName();
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, num_qubits, Attribute::Operate);
    }
    frontend::Circuit* Generate() const override {
        if (IsCached()) {
            return GetCachedCircuit();
        }
        BeginCircuitDefinition();
        auto q = GetQubits(0);

        for (const auto& [idx, op] : pauli_string) {
            switch (op) {
                case PauliChars::X:
                    frontend::gate::H(q[idx]);
                    break;
                case PauliChars::Y:
                    frontend::gate::SDag(q[idx]);
                    frontend::gate::H(q[idx]);
                    break;
                case PauliChars::Z:
                    break;
                default:
                    throw std::runtime_error("Unsupported pauli character.");
            }
        }

        auto target_qubits = std::vector<std::size_t>();
        for (const auto& [idx, _] : pauli_string) {
            target_qubits.emplace_back(idx);
        }
        for (auto i = std::size_t{0}; i < target_qubits.size() - 1; ++i) {
            frontend::gate::CX(q[target_qubits[i]], q[target_qubits[i + 1]]);
        }
        frontend::gate::RZ(q[target_qubits.back()], angle);

        for (const auto& [idx, op] : pauli_string) {
            switch (op) {
                case PauliChars::X:
                    frontend::gate::H(q[idx]);
                    break;
                case PauliChars::Y:
                    frontend::gate::H(q[idx]);
                    frontend::gate::S(q[idx]);
                    break;
                case PauliChars::Z:
                    break;
                default:
                    throw std::runtime_error("Unsupported pauli character.");
            }
        }

        return EndCircuitDefinition();
    }
};
void AddPauliRotation(
        const frontend::Qubits& q,
        const std::map<std::size_t, PauliChars>& pauli_string,
        const double angle
) {
    auto gen = PauliRotationGen(q.GetBuilder(), q.Size(), pauli_string, angle);
    auto* circuit = gen.Generate();
    (*circuit)(q);
}
struct TrotterStepGen : CircuitGenerator {
    Hamiltonian hamiltonian;
    double step_factor;
    static inline const char* Name = "trotter_step";

    explicit TrotterStepGen(CircuitBuilder* builder, const Hamiltonian& h, const double step_factor)
        : CircuitGenerator(builder)
        , hamiltonian(h)
        , step_factor(step_factor) {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return GetName();
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, hamiltonian.num_qubits, Attribute::Operate);
    }
    frontend::Circuit* Generate() const override {
        if (IsCached()) {
            return GetCachedCircuit();
        }

        BeginCircuitDefinition();
        auto q = GetQubits(0);
        for (const auto& term : hamiltonian.pauli_terms) {
            const auto step_angle = 2 * term.coeff * step_factor;
            AddPauliRotation(q, term.pauli_string, step_angle);
        }
        return EndCircuitDefinition();
    }
};
void AddTrotterStep(
        const frontend::Qubits& q,
        const Hamiltonian& hamiltonian,
        const double step_factor
) {
    auto gen = TrotterStepGen(q.GetBuilder(), hamiltonian, step_factor);
    auto* circuit = gen.Generate();
    (*circuit)(q);
}
struct TrotterCircuitGen : CircuitGenerator {
    Hamiltonian hamiltonian;
    std::size_t num_trotter_steps;
    double time;

    TrotterCircuitGen(
            CircuitBuilder* builder,
            const Hamiltonian& h,
            const std::size_t num_trotter_steps,
            const double time
    )
        : CircuitGenerator(builder)
        , hamiltonian(h)
        , num_trotter_steps(num_trotter_steps)
        , time(time) {}
    std::string GetName() const override {
        return fmt::format("trotter_circuit({})", num_trotter_steps);
    }
    std::string GetCacheKey() const override {
        return GetName();
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, hamiltonian.num_qubits, Attribute::Operate);
    }
    frontend::Circuit* Generate() const override {
        if (IsCached()) {
            return GetCachedCircuit();
        }
        BeginCircuitDefinition();
        auto q = GetQubits(0);
        for (auto i = std::size_t{0}; i < num_trotter_steps; ++i) {
            AddTrotterStep(q, hamiltonian, time / num_trotter_steps);
        }
        return EndCircuitDefinition();
    }
};

std::int32_t main(std::int32_t argc, const char* const* const argv) {
    std::filesystem::path source_file_path = __FILE__;
    const auto default_input_file =
            source_file_path.parent_path() / "data/trotter/1d_ising_hamiltonian.json";
    namespace po = boost::program_options;
    po::options_description desc("Simulation of Trotter decomposition");
    desc.add_options()
        ("help", "Print usage instructions")
        ("file", po::value<std::string>()->default_value(default_input_file.string()), "Path to JSON file of input Hamiltonian")
        ("time", po::value<double>()->default_value(1.0), "Time for time evolution")
        ("num_trotter_steps", po::value<std::size_t>()->default_value(1), "Number of trotter steps")
        ("out", po::value<std::string>()->default_value("out.json"), "Path to the output file")
        ("inline", "Option to enable inline expansion for all modules in the Trotter circuit.");

    po::variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);
    notify(vm);

    if (vm.count("help") > 0) {
        std::cout << desc << std::endl;
        return 0;
    }

    std::string input_file;
    if (vm.count("file") > 0) {
        input_file = vm["file"].as<std::string>();
    }
    const auto h = ReadHamiltonian(input_file);

    const auto time = vm["time"].as<double>();
    const auto num_trotter_steps = vm["num_trotter_steps"].as<std::size_t>();
    ir::IRContext context;
    auto* module = ir::Module::Create("simulation_time_evolution_with_trotter", context);
    auto builder = CircuitBuilder(module);
    auto gen = TrotterCircuitGen(&builder, h, num_trotter_steps, time);
    auto* circuit = gen.Generate();
    auto* ir_circuit = circuit->GetIR();

    // Inline expansion
    if (vm.count("inline") > 0) {
        ir::RecursiveInlinerPass().RunOnFunction(*ir_circuit);
    }

    std::string output_file;
    if (vm.count("out") > 0) {
        output_file = vm["out"].as<std::string>();
    }
    std::ofstream ofs(output_file);
    if (!ofs) {
        std::cerr << "Failed to open output file: " << output_file << std::endl;
        return 1;
    }
    auto module_json = qret::Json(*module);
    ofs << module_json;
    ofs.close();

    return 0;
}
