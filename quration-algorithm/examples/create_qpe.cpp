#include <boost/program_options.hpp>
#include <nlohmann/json.hpp>

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "qret/algorithm/phase_estimation/qpe.h"
#include "qret/frontend/builder.h"
#include "qret/ir/context.h"
#include "qret/ir/json.h"
#include "qret/math/integer.h"
#include "qret/math/lcu_helper.h"
#include "qret/math/pauli.h"
#include "qret/transforms/ipo/inliner.h"

struct QPEParams {
    std::vector<qret::frontend::gate::PauliArray> paulis;
    std::vector<double> lcu_coefficients;
    std::size_t hadamard_size;
    std::size_t system_size;
    std::size_t sub_bit_precision;
    QPEParams() = default;
};

void from_json(const nlohmann::json& j, QPEParams& p) {
    p.hadamard_size = j.at("hadamard_size").get<std::size_t>();
    p.system_size = j.at("system_size").get<std::size_t>();
    p.sub_bit_precision = j.at("sub_bit_precision").get<std::size_t>();
    p.lcu_coefficients = j.at("lcu_coefficients").get<std::vector<double>>();

    p.paulis.clear();
    const auto& paulis_json = j.at("paulis");
    p.paulis.reserve(paulis_json.size());

    for (const auto& pauli_array_json : paulis_json) {
        auto current_array = qret::frontend::gate::PauliArray();
        for (const auto& pauli_string_json : pauli_array_json) {
            auto pauli_string = std::vector<qret::math::Pauli>();
            for (const auto& pauli_json : pauli_string_json) {
                const auto s = pauli_json.get<std::string>();
                auto pauli = qret::math::PauliFromString(s);
                pauli_string.emplace_back(pauli);
            }
            current_array.emplace_back(pauli_string);
        }
        p.paulis.emplace_back(current_array);
    }
}

QPEParams LoadQPEJson(const std::string& path) {
    auto ifs = std::ifstream(path.data());
    if (!ifs.good()) {
        throw std::runtime_error("Could not open file: " + path);
    }
    nlohmann::json j;
    ifs >> j;
    return j.get<QPEParams>();
};

int main(std::int32_t argc, const char* const* const argv) {
    std::filesystem::path source_file_path = __FILE__;
    const auto default_input_file = source_file_path.parent_path() / "data/sample_qpe.json";
    namespace po = boost::program_options;
    po::options_description desc("Create QPE circuit from JSON file");
    desc.add_options()
        ("help", "Print usage instructions")
        ("file", po::value<std::string>()->default_value(default_input_file.string()), "Path to JSON file of input parameters")
        ("out", po::value<std::string>()->default_value("out.json"), "Path to the output file")
        ("inline", "Option to enable inline expansion");

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
    const auto params = LoadQPEJson(input_file);
    const auto size = qret::math::BitSizeI(params.lcu_coefficients.size() - 1);
    const auto alias_sampling = qret::math::PreprocessLCUCoefficientsForReversibleSampling(
            params.lcu_coefficients,
            params.sub_bit_precision
    );

    qret::ir::IRContext context;
    auto* module = qret::ir::Module::Create("QPEModule", context);
    auto builder = qret::frontend::CircuitBuilder(module);
    auto gen = qret::frontend::gate::PhaseEstimationGen(
            &builder,
            params.paulis,
            alias_sampling,
            params.hadamard_size,
            params.system_size,
            size,
            params.sub_bit_precision
    );
    auto* circuit = gen.Generate();
    auto* ir_circuit = circuit->GetIR();

    // Inline expansion
    if (vm.count("inline") > 0) {
        qret::ir::RecursiveInlinerPass().RunOnFunction(*ir_circuit);
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
