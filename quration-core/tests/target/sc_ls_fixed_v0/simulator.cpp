#include "qret/target/sc_ls_fixed_v0/simulator.h"

#include <gtest/gtest.h>

#include <memory>

#include "qret/base/cast.h"
#include "qret/base/string.h"
#include "qret/target/sc_ls_fixed_v0/simulator_runnability.h"

using namespace qret;
using namespace qret::sc_ls_fixed_v0;

namespace {
ScLsFixedV0MachineOption MakeOption() {
    return ScLsFixedV0MachineOption{
            .magic_generation_period = 15,
            .maximum_magic_state_stock = 100,
            .entanglement_generation_period = 15,
            .maximum_entangled_state_stock = 100,
            .reaction_time = 1,
    };
}

void PutQubit(
        ScLsSimulator& simulator,
        MinimumAvailableBeat& avail,
        QSymbol q,
        const Coord3D& place,
        std::uint32_t dir
) {
    simulator.GetStateBuffer().PutQubit(0, q, place, dir);
    avail.q[q] = 0;
}

void PutMagicFactory(
        ScLsSimulator& simulator,
        MinimumAvailableBeat& avail,
        MSymbol m,
        const Coord3D& place
) {
    auto state = MagicFactoryState::Empty(0);
    state.magic_state_count = 1;
    simulator.GetStateBuffer().PutMagicFactory(0, m, place, state);
    avail.m[m] = 0;
}

void PutEntanglementFactoryPair(
        ScLsSimulator& simulator,
        MinimumAvailableBeat& avail,
        ESymbol e1,
        const Coord3D& place1,
        ESymbol e2,
        const Coord3D& place2
) {
    simulator.GetStateBuffer().PutEntanglementFactory(
            0,
            e1,
            place1,
            e2,
            place2,
            EntanglementFactoryState{
                    .entangled_state_generation_elapsed = 0,
                    .entangled_state_stock_count_free = 1,
                    .entangled_state_handle_list = {}
            },
            EntanglementFactoryState{
                    .entangled_state_generation_elapsed = 0,
                    .entangled_state_stock_count_free = 1,
                    .entangled_state_handle_list = {}
            }
    );
    avail.e[e1] = 0;
    avail.e[e2] = 0;
}

class DirectMagicRouteSearcher final : public RouteSearcher {
public:
    std::size_t search_route_3dm_calls = 0;

    std::optional<SearchRoute::Route2D> SearchRoute2D(
            QuantumStateBuffer&,
            Beat,
            QSymbol,
            QSymbol,
            std::uint32_t,
            std::uint32_t
    ) override {
        ADD_FAILURE() << "SearchRoute2D should not be used for 3D LATTICE_SURGERY_MAGIC.";
        return std::nullopt;
    }

    std::optional<SearchRoute::Route2D>
    SearchCnotRoute2D(QuantumStateBuffer&, Beat, QSymbol, QSymbol) override {
        return std::nullopt;
    }

    std::optional<SearchRoute::Route2DM>
    SearchRoute2DM(QuantumStateBuffer&, Beat, QSymbol, std::uint32_t) override {
        return std::nullopt;
    }

    std::optional<SearchRoute::Route2DE>
    SearchRoute2DE(QuantumStateBuffer&, Beat, ESymbol, QSymbol, std::uint32_t) override {
        return std::nullopt;
    }

    std::optional<SearchRoute::Route3D> SearchRoute3D(
            QuantumStateBuffer&,
            Beat,
            QSymbol,
            QSymbol,
            std::uint32_t,
            std::uint32_t
    ) override {
        return std::nullopt;
    }

    std::optional<SearchRoute::Route3D>
    SearchCnotRoute3D(QuantumStateBuffer&, Beat, QSymbol, QSymbol) override {
        return std::nullopt;
    }

    std::optional<SearchRoute::Route3DM> SearchRoute3DM(
            QuantumStateBuffer& buffer,
            Beat beat,
            QSymbol q_dst,
            std::uint32_t boundaries_dst
    ) override {
        ++search_route_3dm_calls;
        EXPECT_EQ(beat, Beat{1});
        EXPECT_EQ(boundaries_dst, SearchRoute::LS_Z_BOUNDARY);

        const auto dst = buffer.GetQuantumState(beat).GetPlace(q_dst);
        auto route = SearchRoute::Route3DM{
                .magic_factory = MSymbol{0},
                .q_dst = q_dst,
                .moved_state_dir = 0,
                .src = Coord3D{0, 0, 2},
                .move_path = {Coord3D{0, 0, 2}, Coord3D{1, 0, 2}},
                .logical_path =
                        {
                                Coord3D{1, 0, 4},
                                Coord3D{1, 1, 4},
                                Coord3D{2, 1, 4},
                                dst,
                        },
                .dst = dst,
        };
        route.Audit();
        return route;
    }
};

}  // namespace

TEST(SimulatorValidation, LatticeSurgeryRejectsWrongBoundaryPath) {
    auto topology = Topology::FromYAML(LoadFile("quration-core/tests/data/topology/plane.yaml"));
    auto option = MakeOption();
    auto simulator = ScLsSimulator(*topology, option, 8, SymbolGenerator::New());
    auto avail = MinimumAvailableBeat{};

    PutQubit(simulator, avail, QSymbol{0}, Coord3D{1, 1, 3}, 0);
    PutQubit(simulator, avail, QSymbol{1}, Coord3D{4, 1, 3}, 0);

    auto inst = LatticeSurgery::New(
            {QSymbol{0}, QSymbol{1}},
            {Pauli::Z(), Pauli::Z()},
            {
                    Coord3D{1, 2, 3},
                    Coord3D{2, 2, 3},
                    Coord3D{3, 2, 3},
                    Coord3D{4, 2, 3},
            },
            CSymbol{10},
            {}
    );

    EXPECT_FALSE(detail::IsLatticeSurgeryRunnable(simulator.GetStateBuffer(), avail, 0, *inst));
}

TEST(SimulatorValidation, LatticeSurgeryMagicRejectsPathThatMissesFactory) {
    auto topology = Topology::FromYAML(LoadFile("quration-core/tests/data/topology/plane.yaml"));
    auto option = MakeOption();
    auto simulator = ScLsSimulator(*topology, option, 8, SymbolGenerator::New());
    auto avail = MinimumAvailableBeat{};

    PutQubit(simulator, avail, QSymbol{0}, Coord3D{4, 3, 3}, 0);
    PutMagicFactory(simulator, avail, MSymbol{0}, Coord3D{0, 0, 3});

    auto inst = LatticeSurgeryMagic::New(
            {QSymbol{0}},
            {Pauli::Z()},
            {
                    Coord3D{3, 3, 3},
                    Coord3D{2, 3, 3},
            },
            CSymbol{10},
            MSymbol{0},
            {}
    );

    EXPECT_FALSE(
            detail::IsLatticeSurgeryMagicRunnable(simulator.GetStateBuffer(), avail, 0, *inst)
    );
}

TEST(SimulatorValidation, LatticeSurgeryMultinodeRejectsPathThatMissesEntanglementFactory) {
    auto topology =
            Topology::FromYAML(LoadFile("quration-core/tests/data/topology/distribute.yaml"));
    auto option = MakeOption();
    auto simulator = ScLsSimulator(*topology, option, 8, SymbolGenerator::New());
    auto avail = MinimumAvailableBeat{};

    PutQubit(simulator, avail, QSymbol{0}, Coord3D{4, 1, 2}, 0);
    PutEntanglementFactoryPair(
            simulator,
            avail,
            ESymbol{1},
            Coord3D{0, 0, 2},
            ESymbol{0},
            Coord3D{0, 0, 0}
    );

    auto inst = LatticeSurgeryMultinode::New(
            {QSymbol{0}},
            {Pauli::Z()},
            {
                    Coord3D{3, 1, 2},
                    Coord3D{2, 1, 2},
            },
            CSymbol{10},
            {},
            {ESymbol{1}},
            {EHandle{0}},
            {}
    );

    EXPECT_FALSE(
            detail::IsLatticeSurgeryMultinodeRunnable(simulator.GetStateBuffer(), avail, 0, *inst)
    );
}

TEST(SimulatorValidation, MoveEntanglementRejectsPathThatDoesNotReachDestination) {
    auto topology =
            Topology::FromYAML(LoadFile("quration-core/tests/data/topology/distribute.yaml"));
    auto option = MakeOption();
    auto simulator = ScLsSimulator(*topology, option, 8, SymbolGenerator::New());
    auto avail = MinimumAvailableBeat{};

    PutQubit(simulator, avail, QSymbol{0}, Coord3D{4, 1, 2}, 0);
    PutEntanglementFactoryPair(
            simulator,
            avail,
            ESymbol{1},
            Coord3D{0, 0, 2},
            ESymbol{0},
            Coord3D{0, 0, 0}
    );

    auto inst = MoveEntanglement::New(
            ESymbol{1},
            EHandle{0},
            QSymbol{0},
            {
                    Coord3D{1, 0, 2},
                    Coord3D{2, 0, 2},
            },
            {}
    );

    EXPECT_FALSE(detail::IsMoveEntanglementRunnable(simulator.GetStateBuffer(), avail, 0, *inst));
}

TEST(SimulatorValidation, LatticeSurgeryMagic3DUsesSearchRoutePathDirectly) {
    auto topology = Topology::FromYAML(LoadFile("quration-core/tests/data/topology/grid.yaml"));
    auto option = MakeOption();
    option.magic_generation_period = 0;
    auto route_searcher = std::make_unique<DirectMagicRouteSearcher>();
    auto* route_searcher_ptr = route_searcher.get();
    auto simulator =
            ScLsSimulator(*topology, option, 8, SymbolGenerator::New(), std::move(route_searcher));

    const auto target = ScLsFixedV0TargetMachine(topology, option);
    auto mf = MachineFunction(&target);
    auto& block = mf.AddBlock();
    auto allocate_magic = AllocateMagicFactory::New(MSymbol{0}, Coord3D{0, 0, 2}, {});
    auto* allocate_magic_ptr = allocate_magic.get();
    block.EmplaceBack(std::move(allocate_magic));
    auto allocate_qubit = Allocate::New(QSymbol{0}, Coord3D{3, 1, 4}, 0, {});
    auto* allocate_qubit_ptr = allocate_qubit.get();
    block.EmplaceBack(std::move(allocate_qubit));
    auto inst =
            LatticeSurgeryMagic::New({QSymbol{0}}, {Pauli::Z()}, {}, CSymbol{10}, MSymbol{0}, {});
    auto* inst_ptr = inst.get();
    block.EmplaceBack(std::move(inst));
    block.ConstructInverseMap();

    auto queue = InstQueue(option, mf, InstQueue::WeightAlgorithm::Index);
    queue.Peek(8);

    ASSERT_TRUE(simulator.Run(0, queue, mf, allocate_magic_ptr));
    ASSERT_TRUE(simulator.Run(0, queue, mf, allocate_qubit_ptr));
    simulator.StepBeat();
    ASSERT_TRUE(simulator.Run(1, queue, mf, inst_ptr));
    EXPECT_EQ(route_searcher_ptr->search_route_3dm_calls, std::size_t{1});

    const auto expected_path = std::list<Coord3D>{Coord3D{1, 1, 4}, Coord3D{2, 1, 4}};
    auto* inserted_ls = static_cast<const LatticeSurgery*>(nullptr);
    auto num_magic = std::size_t{0};
    for (const auto& minst : block) {
        const auto* base = static_cast<const ScLsInstructionBase*>(minst.get());
        if (const auto* ls = DynCast<LatticeSurgery>(base); ls != nullptr) {
            inserted_ls = ls;
        }
        if (DynCast<LatticeSurgeryMagic>(base) != nullptr) {
            ++num_magic;
        }
    }

    ASSERT_NE(inserted_ls, nullptr);
    EXPECT_EQ(num_magic, std::size_t{0});
    EXPECT_EQ(inserted_ls->Path(), expected_path);
    EXPECT_EQ(inserted_ls->BasisList().front(), Pauli::X());
}
