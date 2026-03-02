#include <boost/program_options.hpp>

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

#include "qret/algorithm/arithmetic/shor.h"
#include "qret/base/type.h"
#include "qret/frontend/builder.h"
#include "qret/ir/context.h"
#include "qret/ir/json.h"
#include "qret/transforms/ipo/inliner.h"

int main(std::int32_t argc, const char* const* const argv) {
    namespace po = boost::program_options;
    po::options_description desc("Create a PeriodFinding circuit.");
    desc.add_options()
        ("help", "Print usage instructions")
        ("mod", po::value<std::string>(), "The modulus")
        ("coprime", po::value<std::string>(), "Coprime integer")
        ("depth", po::value<std::size_t>(), "Depth of the circuit")
        ("out", po::value<std::string>()->default_value("out.json"), "Path to the output file")
        ("inline", "Option to enable inline expansion.");

    po::variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);
    notify(vm);

    if (vm.count("help") > 0) {
        std::cout << desc << std::endl;
        return 0;
    }

    const auto mod = qret::BigInt(vm["mod"].as<std::string>());
    const auto coprime = qret::BigInt(vm["coprime"].as<std::string>());
    const auto depth = vm["depth"].as<std::size_t>();

    qret::ir::IRContext context;
    auto* module = qret::ir::Module::Create("PeriodFindingModule", context);
    auto builder = qret::frontend::CircuitBuilder(module);
    auto gen = qret::frontend::gate::PeriodFinderGen(&builder, mod, coprime, depth);
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
