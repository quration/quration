#include <boost/program_options.hpp>

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

#include "qret/algorithm/phase_estimation/trotter.h"
#include "qret/frontend/builder.h"
#include "qret/ir/context.h"
#include "qret/ir/json.h"
#include "qret/transforms/ipo/inliner.h"

int main(std::int32_t argc, const char* const* const argv) {
    namespace po = boost::program_options;
    po::options_description desc(
            "Create a time-evolution circuit from a given Hamiltonian using Trotter expansion"
    );
    desc.add_options()
        ("help", "Print usage instructions")
        ("file", po::value<std::string>()->required(), "Path to JSON file of input Hamiltonian")
        ("time", po::value<double>()->default_value(1.0), "Time for time evolution")
        ("num_trotter_steps", po::value<std::size_t>()->default_value(1), "Number of trotter steps")
        ("out", po::value<std::string>()->default_value("out.json"), "Path to the output file")
        ("inline", "Option to enable inline expansion");

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
    const auto h = qret::frontend::gate::ReadHamiltonian(input_file);

    const auto time = vm["time"].as<double>();
    const auto num_trotter_steps = vm["num_trotter_steps"].as<std::size_t>();
    qret::ir::IRContext context;
    auto* module = qret::ir::Module::Create("TrotterModule", context);
    auto builder = qret::frontend::CircuitBuilder(module);
    auto gen = qret::frontend::gate::TrotterGen(&builder, h, num_trotter_steps, time);
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
