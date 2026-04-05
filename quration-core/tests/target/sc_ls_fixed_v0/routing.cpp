#include "qret/target/sc_ls_fixed_v0/routing.h"

#include <gtest/gtest.h>

#include <fmt/format.h>

#include <iostream>

#include "qret/base/cast.h"
#include "qret/base/log.h"
#include "qret/base/option.h"
#include "qret/base/string.h"
#include "qret/codegen/machine_function.h"
#include "qret/target/sc_ls_fixed_v0/instruction.h"
#include "qret/target/sc_ls_fixed_v0/lowering.h"
#include "qret/target/sc_ls_fixed_v0/mapping.h"
#include "qret/target/sc_ls_fixed_v0/sc_ls_fixed_v0_target_machine.h"
#include "qret/target/sc_ls_fixed_v0/topology.h"
#include "qret/transforms/ipo/inliner.h"
#include "qret/transforms/scalar/decomposition.h"

#include "test_utils.h"

using namespace qret;
using namespace qret::sc_ls_fixed_v0;

namespace {
ir::Function* LoadAddCuccaroCircuit(std::size_t size, ir::IRContext& context) {
    const auto path = fmt::format("quration-core/tests/data/circuit/add_cuccaro_{}.json", size);
    const auto name = fmt::format("AddCuccaro({})", size);
    return tests::LoadCircuitFromJsonFile(path, name, context);
}

void SetRandomMappingOptions(std::uint64_t seed) {
    std::get<Option<std::uint32_t>*>(
            OptionStorage::GetOptionStorage()->At("sc_ls_fixed_v0-partition-algorithm")
    )
            ->SetValue(std::uint32_t{1});
    std::get<Option<std::uint64_t>*>(
            OptionStorage::GetOptionStorage()->At("sc_ls_fixed_v0-partition-seed")
    )
            ->SetValue(seed);
    std::get<Option<std::uint32_t>*>(
            OptionStorage::GetOptionStorage()->At("sc_ls_fixed_v0-find-place-algorithm")
    )
            ->SetValue(std::uint32_t{1});
}

bool PathTurns(const std::list<Coord3D>& path) {
    if (path.size() < 3) {
        return false;
    }

    auto prev = path.begin();
    auto curr = std::next(prev);
    auto next = std::next(curr);
    auto prev_axis = (prev->x != curr->x) ? 'X' : 'Y';
    for (; next != path.end(); ++prev, ++curr, ++next) {
        const auto axis = (curr->x != next->x) ? 'X' : 'Y';
        if (axis != prev_axis) {
            return true;
        }
        prev_axis = axis;
    }
    return false;
}
}  // namespace

TEST(Routing, Plane) {
    for (const auto seed : tests::LoadScLsFixedV0PartitionSeeds()) {
        SCOPED_TRACE(fmt::format("partition_seed={}", seed));

        const auto size = std::size_t{3};

        Logger::EnableConsoleOutput();
        Logger::SetLogLevel(LogLevel::Debug);
        SetRandomMappingOptions(seed);

        ir::IRContext context;
        auto* circuit = LoadAddCuccaroCircuit(size, context);
        ASSERT_NE(nullptr, circuit);
        ir::DecomposeInst().RunOnFunction(*circuit);
        ir::InlinerPass().RunOnFunction(*circuit);

        auto topology = Topology::FromYAML(LoadFile("quration-core/tests/data/topology/plane.yaml"));
        const auto target = ScLsFixedV0TargetMachine(
                topology,
                ScLsFixedV0MachineOption{
                        .magic_generation_period = 15,
                        .maximum_magic_state_stock = 100,
                        .entanglement_generation_period = 15,
                        .maximum_entangled_state_stock = 100,
                        .reaction_time = 1,
                }
        );
        auto mf = MachineFunction(&target);
        mf.SetIR(circuit);
        Lowering().RunOnMachineFunction(mf);
        Mapping().RunOnMachineFunction(mf);
        Routing().RunOnMachineFunction(mf);

        std::cout << "=======================" << std::endl;
        std::cout << "MachineFunction" << std::endl;
        for (const auto& mbb : mf) {
            for (const auto& minst : mbb) {
                std::cout << static_cast<const ScLsInstructionBase*>(minst.get())->ToStringWithMD()
                          << std::endl;
            }
        }
        std::cout << "=======================" << std::endl;
        std::cout << "MachineFunction sorted by beat" << std::endl;
        DumpMachineFunctionSortedByBeat(std::cout, mf);
    }
}
TEST(Routing, LatticeSurgeryConnectManyQubits) {
    Logger::EnableConsoleOutput();
    Logger::SetLogLevel(LogLevel::Debug);

    // Define machine function
    auto topology = Topology::FromYAML(LoadFile("quration-core/tests/data/topology/plane_ls.yaml"));
    const auto target = ScLsFixedV0TargetMachine(
            topology,
            ScLsFixedV0MachineOption{
                    .magic_generation_period = 15,
                    .maximum_magic_state_stock = 100,
                    .entanglement_generation_period = 15,
                    .maximum_entangled_state_stock = 100,
                    .reaction_time = 1,
            }
    );
    auto mf = MachineFunction(&target);

    // Add instructions
    const auto x_size = 10;
    const auto y_size = 20;
    const auto qsym = [&x_size](std::int32_t x, std::int32_t y) {
        return QSymbol(static_cast<std::uint64_t>((y * x_size) + x));
    };
    auto& alloc = mf.AddBlock();
    auto& block = mf.AddBlock();
    for (auto x = 0; x < x_size; ++x) {
        for (auto y = 0; y < y_size; ++y) {
            alloc.EmplaceBack(
                    Allocate::New(
                            qsym(x, y),
                            Coord3D((2 * x) + 1, (2 * y) + 1),
                            std::uint8_t{0},
                            {}
                    )
            );
        }
    }
    block.EmplaceBack(
            LatticeSurgery::New(
                    {qsym(4, 4),
                     qsym(4, 6),
                     qsym(4, 8),
                     qsym(6, 8),
                     qsym(8, 8),
                     qsym(6, 6),
                     qsym(6, 4)},
                    {Pauli::Z(),
                     Pauli::X(),
                     Pauli::X(),
                     Pauli::Z(),
                     Pauli::X(),
                     Pauli::Z(),
                     Pauli::X()},
                    {},
                    CSymbol{10},
                    {}
            )
    );
    block.EmplaceBack(
            LatticeSurgery::New(
                    {qsym(3, 3),
                     qsym(x_size - 1, 0),
                     qsym(0, y_size - 1),
                     qsym(x_size - 1, y_size - 1)},
                    {Pauli::Z(), Pauli::X(), Pauli::X(), Pauli::Z()},
                    {},
                    CSymbol{11},
                    {}
            )
    );

    Routing().RunOnMachineFunction(mf);

    std::cout << "=======================" << std::endl;
    std::cout << "MachineFunction" << std::endl;
    for (const auto& mbb : mf) {
        for (const auto& minst : mbb) {
            std::cout << static_cast<const ScLsInstructionBase*>(minst.get())->ToStringWithMD()
                      << std::endl;
        }
    }
    std::cout << "=======================" << std::endl;
    std::cout << "MachineFunction sorted by beat" << std::endl;
    DumpMachineFunctionSortedByBeat(std::cout, mf);
}
TEST(Routing, CnotRemainsCnotAndUsesBentPath) {
    auto topology = Topology::FromYAML(LoadFile("quration-core/tests/data/topology/plane.yaml"));
    const auto target = ScLsFixedV0TargetMachine(
            topology,
            ScLsFixedV0MachineOption{
                    .magic_generation_period = 15,
                    .maximum_magic_state_stock = 100,
                    .entanglement_generation_period = 15,
                    .maximum_entangled_state_stock = 100,
                    .reaction_time = 1,
            }
    );
    auto mf = MachineFunction(&target);

    auto& alloc = mf.AddBlock();
    auto& block = mf.AddBlock();
    alloc.EmplaceBack(Allocate::New(QSymbol{0}, Coord3D{1, 1, 3}, 0, {}));
    alloc.EmplaceBack(Allocate::New(QSymbol{1}, Coord3D{4, 3, 3}, 0, {}));
    block.EmplaceBack(Cnot::New(QSymbol{0}, QSymbol{1}, {}, {}));

    Routing().RunOnMachineFunction(mf);

    auto* routed_cnot = static_cast<const Cnot*>(nullptr);
    auto num_cnot = std::size_t{0};
    auto num_lattice_surgery = std::size_t{0};
    for (const auto& mbb : mf) {
        for (const auto& minst : mbb) {
            const auto* inst = static_cast<const ScLsInstructionBase*>(minst.get());
            if (const auto* cnot = DynCast<Cnot>(inst); cnot != nullptr) {
                routed_cnot = cnot;
                ++num_cnot;
            }
            if (DynCast<LatticeSurgery>(inst) != nullptr) {
                ++num_lattice_surgery;
            }
        }
    }

    ASSERT_NE(nullptr, routed_cnot);
    EXPECT_EQ(num_cnot, 1);
    EXPECT_EQ(num_lattice_surgery, 0);
    EXPECT_FALSE(routed_cnot->Path().empty());

    auto full_path = routed_cnot->Path();
    full_path.emplace_front(Coord3D{1, 1, 3});
    full_path.emplace_back(Coord3D{4, 3, 3});
    EXPECT_TRUE(PathTurns(full_path));
}
TEST(Routing, LatticeSurgeryMagicConnectManyQubits) {
    Logger::EnableConsoleOutput();
    Logger::SetLogLevel(LogLevel::Debug);

    // Define machine function
    auto topology = Topology::FromYAML(LoadFile("quration-core/tests/data/topology/plane_ls.yaml"));
    const auto target = ScLsFixedV0TargetMachine(
            topology,
            ScLsFixedV0MachineOption{
                    .magic_generation_period = 15,
                    .maximum_magic_state_stock = 100,
                    .entanglement_generation_period = 15,
                    .maximum_entangled_state_stock = 100,
                    .reaction_time = 1,
            }
    );
    auto mf = MachineFunction(&target);

    // Add instructions
    const auto x_size = 10;
    const auto y_size = 20;
    const auto qsym = [&x_size](std::int32_t x, std::int32_t y) {
        return QSymbol(static_cast<std::uint64_t>(y * x_size + x));
    };
    auto& alloc = mf.AddBlock();
    auto& block = mf.AddBlock();
    alloc.EmplaceBack(AllocateMagicFactory::New(MSymbol{0}, Coord3D(0, 0), {}));
    alloc.EmplaceBack(AllocateMagicFactory::New(MSymbol{1}, Coord3D(1, 0), {}));
    alloc.EmplaceBack(AllocateMagicFactory::New(MSymbol{2}, Coord3D(2, 0), {}));
    for (auto x = 0; x < x_size; ++x) {
        for (auto y = 0; y < y_size; ++y) {
            alloc.EmplaceBack(
                    Allocate::New(qsym(x, y), Coord3D(2 * x + 1, 2 * y + 1), std::uint8_t{0}, {})
            );
        }
    }
    block.EmplaceBack(
            LatticeSurgeryMagic::New(
                    {qsym(4, 4),
                     qsym(4, 6),
                     qsym(4, 8),
                     qsym(6, 8),
                     qsym(8, 8),
                     qsym(6, 6),
                     qsym(6, 4)},
                    {Pauli::Z(),
                     Pauli::X(),
                     Pauli::X(),
                     Pauli::Z(),
                     Pauli::X(),
                     Pauli::Z(),
                     Pauli::X()},
                    {},
                    CSymbol{10},
                    MSymbol{0},
                    {}
            )
    );
    block.EmplaceBack(
            LatticeSurgeryMagic::New(
                    {qsym(3, 3),
                     qsym(x_size - 1, 0),
                     qsym(0, y_size - 1),
                     qsym(x_size - 1, y_size - 1)},
                    {Pauli::Z(), Pauli::X(), Pauli::X(), Pauli::Z()},
                    {},
                    CSymbol{11},
                    MSymbol{1},
                    {}
            )
    );

    Routing().RunOnMachineFunction(mf);

    std::cout << "=======================" << std::endl;
    std::cout << "MachineFunction" << std::endl;
    for (const auto& mbb : mf) {
        for (const auto& minst : mbb) {
            std::cout << static_cast<const ScLsInstructionBase*>(minst.get())->ToStringWithMD()
                      << std::endl;
        }
    }
    std::cout << "=======================" << std::endl;
    std::cout << "MachineFunction sorted by beat" << std::endl;
    DumpMachineFunctionSortedByBeat(std::cout, mf);
}
TEST(Routing, Grid) {
    for (const auto seed : tests::LoadScLsFixedV0PartitionSeeds()) {
        SCOPED_TRACE(fmt::format("partition_seed={}", seed));

        const auto size = std::size_t{3};

        Logger::EnableConsoleOutput();
        Logger::SetLogLevel(LogLevel::Debug);
        SetRandomMappingOptions(seed);

        ir::IRContext context;
        auto* circuit = LoadAddCuccaroCircuit(size, context);
        ASSERT_NE(nullptr, circuit);
        ir::DecomposeInst().RunOnFunction(*circuit);
        ir::InlinerPass().RunOnFunction(*circuit);

        auto topology = Topology::FromYAML(LoadFile("quration-core/tests/data/topology/grid.yaml"));
        const auto target = ScLsFixedV0TargetMachine(
                topology,
                ScLsFixedV0MachineOption{
                        .magic_generation_period = 15,
                        .maximum_magic_state_stock = 100,
                        .entanglement_generation_period = 15,
                        .maximum_entangled_state_stock = 100,
                        .reaction_time = 1,
                }
        );
        auto mf = MachineFunction(&target);
        mf.SetIR(circuit);
        Lowering().RunOnMachineFunction(mf);
        Mapping().RunOnMachineFunction(mf);
        Routing().RunOnMachineFunction(mf);

        std::cout << "=======================" << std::endl;
        std::cout << "MachineFunction" << std::endl;
        for (const auto& mbb : mf) {
            for (const auto& minst : mbb) {
                std::cout << static_cast<const ScLsInstructionBase*>(minst.get())->ToStringWithMD()
                          << std::endl;
            }
        }
        std::cout << "=======================" << std::endl;
        std::cout << "MachineFunction sorted by beat" << std::endl;
        DumpMachineFunctionSortedByBeat(std::cout, mf);
    }
}
TEST(Routing, Distribute) {
    for (const auto seed : tests::LoadScLsFixedV0PartitionSeeds()) {
        SCOPED_TRACE(fmt::format("partition_seed={}", seed));

        const auto size = std::size_t{3};

        Logger::EnableConsoleOutput();
        Logger::EnableColorfulOutput();
        Logger::SetLogLevel(LogLevel::Debug);
        SetRandomMappingOptions(seed);

        ir::IRContext context;
        auto* circuit = LoadAddCuccaroCircuit(size, context);
        ASSERT_NE(nullptr, circuit);
        ir::DecomposeInst().RunOnFunction(*circuit);
        ir::InlinerPass().RunOnFunction(*circuit);

        auto topology =
                Topology::FromYAML(LoadFile("quration-core/tests/data/topology/distribute.yaml"));
        const auto target = ScLsFixedV0TargetMachine(
                topology,
                ScLsFixedV0MachineOption{
                        .type = ScLsFixedV0MachineType::DistributedDim2,
                        .magic_generation_period = 15,
                        .maximum_magic_state_stock = 100,
                        .entanglement_generation_period = 15,
                        .maximum_entangled_state_stock = 100,
                        .reaction_time = 15,
                }
        );
        auto mf = MachineFunction(&target);
        mf.SetIR(circuit);
        Lowering().RunOnMachineFunction(mf);
        Mapping().RunOnMachineFunction(mf);
        Routing().RunOnMachineFunction(mf);

        // std::cout << "=======================" << std::endl;
        // std::cout << "MachineFunction" << std::endl;
        // for (const auto& mbb : mf) {
        //     for (const auto& minst : mbb) { std::cout << minst->ToString() << std::endl; }
        // }
    }
}
