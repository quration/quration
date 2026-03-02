#include <fstream>
#include <random>
#include <stdexcept>
#include <string>

#include "qret/base/option.h"
#include "qret/base/string.h"
#include "qret/codegen/asm_printer.h"
#include "qret/codegen/machine_function.h"
#include "qret/frontend/argument.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/intrinsic.h"
#include "qret/target/sc_ls_fixed_v0/calc_compile_info.h"
#include "qret/target/sc_ls_fixed_v0/external_pass.h"
#include "qret/target/sc_ls_fixed_v0/lowering.h"
#include "qret/target/sc_ls_fixed_v0/mapping.h"
#include "qret/target/sc_ls_fixed_v0/routing.h"
#include "qret/target/sc_ls_fixed_v0/sc_ls_fixed_v0_target_machine.h"
#include "qret/target/sc_ls_fixed_v0/topology.h"
#include "qret/transforms/scalar/decomposition.h"

using namespace qret;

namespace {
class SampleGen : frontend::CircuitGenerator {
public:
    static inline const char* Name = "SampleCircuit";
    explicit SampleGen(frontend::CircuitBuilder* builder)
        : CircuitGenerator{builder} {}

    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return Name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("x", Type::Qubit, 64, Attribute::Operate);
        arg.Add("r", Type::Register, 1, Attribute::Output);
    }

    void ApplyRandomGate(
            const frontend::Qubits& x,
            std::mt19937_64& engine,
            const auto get_index
    ) const {
        const auto op = engine() % 5;
        const auto p = get_index(engine());
        const auto q = get_index(engine());
        const auto r = get_index(engine());
        switch (op) {
            case 0:
                if (p != q) {
                    frontend::gate::CX(x[p], x[q]);
                }
                break;
            case 1:
                if (p != q) {
                    frontend::gate::CY(x[p], x[q]);
                }
                break;
            case 2:
                if (p != q) {
                    frontend::gate::CZ(x[p], x[q]);
                }
                break;
            case 3:
                if (p != q && q != r && r != p) {
                    frontend::gate::CCX(x[p], x[q], x[r]);
                }
                break;
            case 4:
                if (p != q && q != r && r != p) {
                    frontend::gate::CCY(x[p], x[q], x[r]);
                }
                break;
            default:
                if (p != q && q != r && r != p) {
                    frontend::gate::CCZ(x[p], x[q], x[r]);
                }
                break;
        }
    }

    frontend::Circuit* Generate() const override {
        if (IsCached()) {
            return GetCachedCircuit();
        }
        BeginCircuitDefinition();

        const auto x = GetQubits(0);
        const auto num_clusters = std::uint64_t{4};
        const auto cluster_size = x.Size() / num_clusters;

        const auto seed = std::uint64_t{21983};
        auto engine = std::mt19937_64{seed};

        for (auto c = std::uint64_t{0}; c < num_clusters; ++c) {
            for (auto i = 0; i < 100; ++i) {
                ApplyRandomGate(x, engine, [num_clusters, cluster_size, c](const auto t) {
                    return (c * cluster_size) + (t % cluster_size);
                });
            }
        }
        for (auto i = 0; i < 10; ++i) {
            ApplyRandomGate(x, engine, [&x](const auto t) { return t % (x.Size()); });
        }

        return EndCircuitDefinition();
    }
};

void Mapping() {
    ir::IRContext context;
    auto* module = ir::Module::Create("SampleModule", context);
    auto builder = frontend::CircuitBuilder(module);
    auto gen = SampleGen(&builder);
    auto* circuit = gen.Generate()->GetIR();
    ir::DecomposeInst().RunOnFunction(*circuit);

    auto topology = sc_ls_fixed_v0::Topology::FromYAML(
            LoadFile("quration-core/examples/data/topology/dist_circle.yaml")
    );
    const auto target = sc_ls_fixed_v0::ScLsFixedV0TargetMachine(
            topology,
            sc_ls_fixed_v0::ScLsFixedV0MachineOption{
                    .type = sc_ls_fixed_v0::ScLsFixedV0MachineType::DistributedDim2,
                    .magic_generation_period = 15,
                    .maximum_magic_state_stock = 10,
                    .entanglement_generation_period = 100,
                    .maximum_entangled_state_stock = 20,
                    .reaction_time = 15,
            }
    );

    bool use_external_pass = true;

    auto mf = MachineFunction(&target);
    mf.SetIR(circuit);
    sc_ls_fixed_v0::InitCompileInfo().RunOnMachineFunction(mf);
    sc_ls_fixed_v0::Lowering().RunOnMachineFunction(mf);
    if (use_external_pass) {
        std::get<Option<std::string>*>(OptionStorage::GetOptionStorage()->At(
                                               "sc_ls_fixed_v0_dump_compile_info_to_markdown"
                                       ))
                ->SetValue("external_mapping.md");

        sc_ls_fixed_v0::ExternalPass(
                // pass name
                "external-mapping",
                // cmd
                "python quration-core/examples/external-pass/mapping.py in.json out.json",
                // input
                "in.json",
                // output
                "out.json",
                // runner (unused)
                ""
        )
                .RunOnMachineFunction(mf);
    } else {
        std::get<Option<std::string>*>(OptionStorage::GetOptionStorage()->At(
                                               "sc_ls_fixed_v0_dump_compile_info_to_markdown"
                                       ))
                ->SetValue("internal_mapping.md");

        // 0: Map based on topology file, 1: Auto
        std::get<Option<std::uint32_t>*>(
                OptionStorage::GetOptionStorage()->At("sc_ls_fixed_v0-mapping-algorithm")
        )
                ->SetValue(std::uint32_t{0});
        sc_ls_fixed_v0::Mapping().RunOnMachineFunction(mf);
    }
    sc_ls_fixed_v0::Routing().RunOnMachineFunction(mf);
    sc_ls_fixed_v0::CompileInfoWithoutTopology().RunOnMachineFunction(mf);
    sc_ls_fixed_v0::CompileInfoWithTopology().RunOnMachineFunction(mf);
    sc_ls_fixed_v0::DumpCompileInfo().RunOnMachineFunction(mf);

    const auto* target_obj = qret::TargetRegistry::GetTargetRegistry()->GetTarget("sc_ls_fixed_v0");
    if (!target_obj->HasAsmPrinter()) {
        throw std::runtime_error("sc_ls_fixed_v0 does not provide asm printer");
    }
    auto streamer = target_obj->CreateAsmStreamer();
    auto asm_printer = target_obj->CreateAsmPrinter(std::move(streamer));
    asm_printer->RunOnMachineFunction(mf);

    auto ofs = std::ofstream(use_external_pass ? "external_mapping.asm" : "internal_mapping.asm");
    if (!ofs.good()) {
        throw std::runtime_error("failed to open asm output file");
    }
    ofs << asm_printer->GetStreamer()->ToString();
    ofs.close();
}
}  // namespace

int main() {
    qret::Logger::EnableConsoleOutput();
    qret::Logger::SetLogLevel(qret::LogLevel::Info);
    Mapping();
    return 0;
}
