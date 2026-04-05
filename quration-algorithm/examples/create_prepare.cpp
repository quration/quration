#include <boost/program_options.hpp>
#include <nlohmann/json.hpp>

#include <cerrno>
#include <cstddef>
#include <cstdint>
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

struct PREPAREParams {
    std::vector<double> lcu_coefficients;
    std::size_t sub_bit_precision;
    PREPAREParams() = default;
};

void from_json(const nlohmann::json& j, PREPAREParams& p) {
    p.sub_bit_precision = j.at("sub_bit_precision").get<std::size_t>();
    p.lcu_coefficients = j.at("lcu_coefficients").get<std::vector<double>>();
}

PREPAREParams LoadPREPAREJson(const std::string& path) {
    auto ifs = std::ifstream(path.data());
    if (!ifs.good()) {
        throw std::runtime_error("Could not open file: " + path);
    }
    nlohmann::json j;
    ifs >> j;
    return j.get<PREPAREParams>();
};

int main(std::int32_t argc, const char* const* const argv) {
    namespace po = boost::program_options;
    po::options_description desc("Create PREPARE circuit from JSON file");
    desc.add_options()
        ("help", "Print usage instructions")
        ("file", po::value<std::string>()->required(), "Path to JSON file of input parameters")
        ("out", po::value<std::string>()->default_value("out.json"), "Path to the output file")
        ("inline", "Option to enable inline expansion.");

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        if (vm.count("help") > 0) {
            std::cout << desc << std::endl;
            return 0;
        }
        po::notify(vm);
    } catch (const po::error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << desc << std::endl;
        return 1;
    }

    std::string input_file;
    if (vm.count("file") > 0) {
        input_file = vm["file"].as<std::string>();
    }
    const auto params = LoadPREPAREJson(input_file);
    const auto size = qret::math::BitSizeI(params.lcu_coefficients.size() - 1);
    const auto alias_sampling = qret::math::PreprocessLCUCoefficientsForReversibleSampling(
            params.lcu_coefficients,
            params.sub_bit_precision
    );

    qret::ir::IRContext context;
    auto* module = qret::ir::Module::Create("PREPAREModule", context);
    auto builder = qret::frontend::CircuitBuilder(module);
    auto gen = qret::frontend::gate::PREPAREGen(
            &builder,
            alias_sampling,
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
