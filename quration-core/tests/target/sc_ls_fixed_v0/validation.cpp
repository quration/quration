#include "qret/target/sc_ls_fixed_v0/validation.h"

#include <gtest/gtest.h>

#include "qret/base/string.h"
#include "qret/codegen/machine_function.h"
#include "qret/error.h"
#include "qret/target/sc_ls_fixed_v0/exception.h"
#include "qret/target/sc_ls_fixed_v0/instruction.h"
#include "qret/target/sc_ls_fixed_v0/pauli.h"
#include "qret/target/sc_ls_fixed_v0/sc_ls_fixed_v0_target_machine.h"
#include "qret/target/sc_ls_fixed_v0/symbol.h"
#include "qret/target/sc_ls_fixed_v0/topology.h"

using namespace qret;
using namespace qret::sc_ls_fixed_v0;

TEST(Validation, UseUnallocatedQ) {
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
    auto& mb = mf.AddBlock();
    mb.EmplaceBack(Allocate::New(QSymbol{0}, Coord3D{}, 0, {}));
    mb.EmplaceBack(DeAllocate::New(QSymbol{0}, {}));

    // OK
    Validate(mf);

    // NG
    mb.EmplaceBack(DeAllocate::New(QSymbol{0}, {}));

    EXPECT_THROW(Validate(mf), ScLsError);
    try {
        Validate(mf);
    } catch (const ScLsError& e) {
        EXPECT_EQ(qret::error::UseUnallocatedSymbol, e.id());
        std::cout << e.what() << std::endl;
    }
}

TEST(Validation, UseUnallocatedC) {
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
    auto& mb = mf.AddBlock();
    mb.EmplaceBack(Allocate::New(QSymbol{0}, Coord3D{}, 0, {}));
    mb.EmplaceBack(Allocate::New(QSymbol{1}, Coord3D{}, 0, {}));
    mb.EmplaceBack(MeasY::New(QSymbol{0}, 0, CSymbol{10}, {}));
    mb.EmplaceBack(
            LatticeSurgery::New(
                    {QSymbol{0}, QSymbol{1}},
                    {Pauli::X(), Pauli::Z()},
                    {},
                    CSymbol{11},
                    {CSymbol{10}}
            )
    );

    // OK
    Validate(mf);

    // NG
    mb.EmplaceBack(
            LatticeSurgery::New(
                    {QSymbol{0}, QSymbol{1}},
                    {Pauli::X(), Pauli::Z()},
                    {},
                    CSymbol{12},
                    {CSymbol{1000}}
            )
    );

    EXPECT_THROW(Validate(mf), ScLsError);
    try {
        Validate(mf);
    } catch (const ScLsError& e) {
        EXPECT_EQ(qret::error::UseUnallocatedSymbol, e.id());
        std::cout << e.what() << std::endl;
    }
}

TEST(Validation, UseUnallocatedM) {
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
    auto& mb = mf.AddBlock();
    mb.EmplaceBack(Allocate::New(QSymbol{0}, Coord3D{}, 0, {}));
    mb.EmplaceBack(AllocateMagicFactory::New(MSymbol{0}, Coord3D{}, {}));
    mb.EmplaceBack(
            LatticeSurgeryMagic::New({QSymbol{0}}, {Pauli::X()}, {}, CSymbol{11}, MSymbol{0}, {})
    );

    // OK
    Validate(mf);

    // NG
    mb.EmplaceBack(
            LatticeSurgeryMagic::New({QSymbol{0}}, {Pauli::X()}, {}, CSymbol{12}, MSymbol{1}, {})
    );

    EXPECT_THROW(Validate(mf), ScLsError);
    try {
        Validate(mf);
    } catch (const ScLsError& e) {
        EXPECT_EQ(qret::error::UseUnallocatedSymbol, e.id());
        std::cout << e.what() << std::endl;
    }
}

TEST(Validation, UseUnallocatedE) {
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
    auto& mb = mf.AddBlock();
    mb.EmplaceBack(Allocate::New(QSymbol{0}, Coord3D{}, 0, {}));
    mb.EmplaceBack(AllocateEntanglementFactory::New(ESymbol{0}, Coord3D{}, ESymbol{1}, {}, {}));
    mb.EmplaceBack(AllocateEntanglementFactory::New(ESymbol{2}, Coord3D{}, ESymbol{3}, {}, {}));
    mb.EmplaceBack(
            LatticeSurgeryMultinode::New(
                    {QSymbol{0}},
                    {Pauli::X()},
                    {},
                    CSymbol{11},
                    {},
                    {ESymbol{0}},
                    {EHandle{0}},
                    {}
            )
    );

    // OK
    Validate(mf);

    // NG
    mb.EmplaceBack(
            LatticeSurgeryMultinode::New(
                    {QSymbol{0}},
                    {Pauli::X()},
                    {},
                    CSymbol{12},
                    {},
                    {ESymbol{5}},
                    {EHandle{1}},
                    {}
            )
    );

    EXPECT_THROW(Validate(mf), ScLsError);
    try {
        Validate(mf);
    } catch (const ScLsError& e) {
        EXPECT_EQ(qret::error::UseUnallocatedSymbol, e.id());
        std::cout << e.what() << std::endl;
    }
}

TEST(Validation, ReAllocateQ) {
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
    auto& mb = mf.AddBlock();
    mb.EmplaceBack(Allocate::New(QSymbol{0}, Coord3D{}, 0, {}));

    // OK
    Validate(mf);

    // NG
    mb.EmplaceBack(Allocate::New(QSymbol{0}, Coord3D{}, 0, {}));

    EXPECT_THROW(Validate(mf), ScLsError);
    try {
        Validate(mf);
    } catch (const ScLsError& e) {
        EXPECT_EQ(qret::error::ReAllocateSymbol, e.id());
        std::cout << e.what() << std::endl;
    }
}

TEST(Validation, ReAllocateQAfterMove) {
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
    auto& mb = mf.AddBlock();
    mb.EmplaceBack(Allocate::New(QSymbol{0}, Coord3D{}, 0, {}));
    mb.EmplaceBack(Allocate::New(QSymbol{1}, Coord3D{}, 0, {}));
    mb.EmplaceBack(Move::New(QSymbol{0}, QSymbol{1}, {}, {}));
    mb.EmplaceBack(Allocate::New(QSymbol{0}, Coord3D{}, 0, {}));

    EXPECT_NO_THROW(Validate(mf));
}

TEST(Validation, ReAllocateQAfterMoveTrans) {
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
    auto& mb = mf.AddBlock();
    mb.EmplaceBack(Allocate::New(QSymbol{0}, Coord3D{}, 0, {}));
    mb.EmplaceBack(Allocate::New(QSymbol{1}, Coord3D{}, 0, {}));
    mb.EmplaceBack(MoveTrans::New(QSymbol{0}, QSymbol{1}, {}));
    mb.EmplaceBack(Allocate::New(QSymbol{0}, Coord3D{}, 0, {}));

    EXPECT_NO_THROW(Validate(mf));
}

TEST(Validation, ReAllocateC) {
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
    auto& mb = mf.AddBlock();
    mb.EmplaceBack(Allocate::New(QSymbol{0}, Coord3D{}, 0, {}));
    mb.EmplaceBack(Allocate::New(QSymbol{1}, Coord3D{}, 0, {}));
    mb.EmplaceBack(MeasY::New(QSymbol{0}, 0, CSymbol{10}, {}));
    mb.EmplaceBack(
            LatticeSurgery::New(
                    {QSymbol{0}, QSymbol{1}},
                    {Pauli::X(), Pauli::Z()},
                    {},
                    CSymbol{11},
                    {CSymbol{10}}
            )
    );

    // OK
    Validate(mf);

    // NG
    mb.EmplaceBack(
            LatticeSurgery::New(
                    {QSymbol{0}, QSymbol{1}},
                    {Pauli::X(), Pauli::Z()},
                    {},
                    CSymbol{11},
                    {CSymbol{10}}
            )
    );

    EXPECT_THROW(Validate(mf), ScLsError);
    try {
        Validate(mf);
    } catch (const ScLsError& e) {
        EXPECT_EQ(qret::error::ReAllocateSymbol, e.id());
        std::cout << e.what() << std::endl;
    }
}

TEST(Validation, ReAllocateReservedC) {
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
    auto& mb = mf.AddBlock();
    mb.EmplaceBack(Allocate::New(QSymbol{0}, Coord3D{}, 0, {}));
    mb.EmplaceBack(MeasY::New(QSymbol{0}, 0, CSymbol{0}, {}));

    EXPECT_THROW(Validate(mf), ScLsError);
    try {
        Validate(mf);
    } catch (const ScLsError& e) {
        EXPECT_EQ(qret::error::ReAllocateSymbol, e.id());
        std::cout << e.what() << std::endl;
    }
}

TEST(Validation, ReAllocateM) {
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
    auto& mb = mf.AddBlock();
    mb.EmplaceBack(AllocateMagicFactory::New(MSymbol{0}, Coord3D{}, {}));
    mb.EmplaceBack(AllocateMagicFactory::New(MSymbol{1}, Coord3D{}, {}));

    // OK
    Validate(mf);

    // NG
    mb.EmplaceBack(AllocateMagicFactory::New(MSymbol{1}, Coord3D{}, {}));

    EXPECT_THROW(Validate(mf), ScLsError);
    try {
        Validate(mf);
    } catch (const ScLsError& e) {
        EXPECT_EQ(qret::error::ReAllocateSymbol, e.id());
        std::cout << e.what() << std::endl;
    }
}

TEST(Validation, ReAllocateE) {
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
    auto& mb = mf.AddBlock();
    mb.EmplaceBack(AllocateEntanglementFactory::New(ESymbol{0}, Coord3D{}, ESymbol{1}, {}, {}));
    mb.EmplaceBack(AllocateEntanglementFactory::New(ESymbol{2}, Coord3D{}, ESymbol{3}, {}, {}));

    // OK
    Validate(mf);

    // NG
    mb.EmplaceBack(AllocateEntanglementFactory::New(ESymbol{1}, Coord3D{}, ESymbol{4}, {}, {}));

    EXPECT_THROW(Validate(mf), ScLsError);
    try {
        Validate(mf);
    } catch (const ScLsError& e) {
        EXPECT_EQ(qret::error::ReAllocateSymbol, e.id());
        std::cout << e.what() << std::endl;
    }
}
