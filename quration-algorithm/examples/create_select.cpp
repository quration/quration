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
#include "qret/transforms/ipo/inliner.h"

struct SELECTParams {
    qret::frontend::gate::PauliArray pauli_strings;
};

void from_json(const nlohmann::json& j, SELECTParams& p) {
    p.pauli_strings.clear();
    const auto& pauli_strings_json = j.at("pauli_strings");
    p.pauli_strings.reserve(pauli_strings_json.size());

    for (const auto& pauli_string_json : pauli_strings_json) {
        auto pauli_string = std::vector<qret::math::Pauli>();
        for (const auto& pauli_json : pauli_string_json) {
            const auto s = pauli_json.get<std::string>();
            auto pauli = qret::math::PauliFromString(s);
            pauli_string.emplace_back(pauli);
        }
        p.pauli_strings.emplace_back(pauli_string);
    }
}

SELECTParams LoadSELECTJson(const std::string& path) {
    auto ifs = std::ifstream(path.data());
    if (!ifs.good()) {
        throw std::runtime_error("Could not open file: " + path);
    }
    nlohmann::json j;
    ifs >> j;
    return j.get<SELECTParams>();
};

int main(std::int32_t argc, const char* const* const argv) {
    std::filesystem::path source_file_path = __FILE__;
    const auto default_input_file = source_file_path.parent_path() / "data/sample_select.json";
    namespace po = boost::program_options;
    po::options_description desc("Create SELECT circuit from JSON file");
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
    const auto params = LoadSELECTJson(input_file);

    qret::ir::IRContext context;
    auto* module = qret::ir::Module::Create("SELECTModule", context);
    auto builder = qret::frontend::CircuitBuilder(module);
    auto gen = qret::frontend::gate::SELECTGen(&builder, params.pauli_strings);
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
