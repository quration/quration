/**
 * @file qret/target/sc_ls_fixed_v0/simulator.cpp
 * @brief Simulator for SC_LS_FIXED_V0 chip.
 */

#include "qret/target/sc_ls_fixed_v0/simulator.h"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>

#include "qret/base/cast.h"
#include "qret/base/log.h"
#include "qret/codegen/machine_function.h"
#include "qret/target/sc_ls_fixed_v0/beat.h"
#include "qret/target/sc_ls_fixed_v0/constants.h"
#include "qret/target/sc_ls_fixed_v0/inst_queue.h"
#include "qret/target/sc_ls_fixed_v0/instruction.h"
#include "qret/target/sc_ls_fixed_v0/pauli.h"
#include "qret/target/sc_ls_fixed_v0/search_grid.h"
#include "qret/target/sc_ls_fixed_v0/state.h"
#include "qret/target/sc_ls_fixed_v0/symbol.h"
#include "qret/target/sc_ls_fixed_v0/topology.h"

namespace qret::sc_ls_fixed_v0 {
namespace {
bool IsAdjacent(const Coord3D& lhs, const Coord3D& rhs) {
    if (lhs.z != rhs.z) {
        return false;
    }
    const auto dx = std::abs(lhs.x - rhs.x);
    const auto dy = std::abs(lhs.y - rhs.y);
    return (dx + dy) == 1;
}

bool IsConnectedPathBetween(
        const Coord3D& src,
        const std::list<Coord3D>& path,
        const Coord3D& dst
) {
    auto prev = src;
    if (path.empty()) {
        return IsAdjacent(src, dst);
    }
    for (const auto& p : path) {
        if (!IsAdjacent(prev, p)) {
            return false;
        }
        prev = p;
    }
    return IsAdjacent(prev, dst);
}

bool HasOnlyBoundaryPauli(const std::list<Pauli>& basis_list) {
    return std::all_of(basis_list.begin(), basis_list.end(), [](Pauli pauli) {
        return pauli.IsX() || pauli.IsZ() || pauli.IsAny();
    });
}
}  // namespace

#pragma region simulator
ScLsSimulator::ScLsSimulator(
        const Topology& topology,
        const ScLsFixedV0MachineOption& option,
        Beat buffer_size,
        std::shared_ptr<SymbolGenerator> symbol_generator,
        std::unique_ptr<RouteSearcher> route_searcher
)
    : option_(option)
    , buffer_(QuantumStateBuffer::New(topology, option, buffer_size))
    , symbol_generator_(std::move(symbol_generator))
    , route_searcher_(std::move(route_searcher)) {
    if (!route_searcher_) {
        route_searcher_ = std::make_unique<DefaultRouteSearcher>();
    }

    // Initialize available places.
    for (const auto& grid : topology) {
        for (const auto& plane : grid) {
            auto z = plane.GetZ();
            for (auto x = 0; x < plane.GetMaxX(); ++x) {
                for (auto y = 0; y < plane.GetMaxY(); ++y) {
                    if (plane.IsBanned({x, y})) {
                        avail_.p[{x, y, z}] = InfBeat;
                        GetStateBuffer().SetBanned(0, {x, y, z});
                    } else {
                        avail_.p[{x, y, z}] = 0;
                    }
                }
            }
        }
    }
}
void ScLsSimulator::StepBeat() {
    // TODO: Implement debugging code.

    // Update state buffer.
    GetStateBuffer().StepBeat();
}
bool ScLsSimulator::SymbolsAreOnTheSamePlane(
        Beat beat,
        const std::list<QSymbol>& qs,
        const std::list<MSymbol>& ms,
        const std::list<ESymbol>& es
) const {
    auto& state = buffer_->GetQuantumState(beat);
    std::optional<std::int32_t> z = std::nullopt;
    for (const auto& q : qs) {
        const auto& q_z = state.GetPlace(q).z;
        if (z) {
            if (*z != q_z) {
                return false;
            }
        } else {
            z = q_z;
        }
    }
    for (const auto& m : ms) {
        const auto& m_z = state.GetPlace(m).z;
        if (z) {
            if (*z != m_z) {
                return false;
            }
        } else {
            z = m_z;
        }
    }
    for (const auto& e : es) {
        const auto& e_z = state.GetPlace(e).z;
        if (z) {
            if (*z != e_z) {
                return false;
            }
        } else {
            z = e_z;
        }
    }
    return true;
}
bool ScLsSimulator::Run(
        Beat beat,
        InstQueue& queue,
        MachineFunction& mf,
        ScLsInstructionBase* base_inst
) {
    LOG_DEBUG("[BEAT {}] Try: {}", beat, base_inst->ToString());

    const auto type = base_inst->Type();

    // Check CDepend.
    for (const auto& c : base_inst->CDepend()) {
        const auto itr = avail_.c.find(c);
        if (c.Id() >= NumReservedCSymbols && (itr == avail_.c.end() || beat < itr->second)) {
            LOG_DEBUG("Not corrected: {}", c);
            return false;
        }
    }

    // Check condition.
    for (const auto& c : base_inst->Condition()) {
        const auto itr = avail_.c.find(c);
        if (itr == avail_.c.end() || beat < itr->second) {
            LOG_DEBUG("Not corrected: {}", c);
            return false;
        }
    }

    // Run instruction if possible.
    switch (type) {
        case ScLsInstructionType::ALLOCATE: {
            auto* inst = Cast<Allocate>(base_inst);
            if (!IsAllocateRunnable(beat, inst)) {
                return false;
            }
            RunAllocate(beat, inst);
            RunInstructionInQueue(queue, inst);
            return true;
        }
        case ScLsInstructionType::ALLOCATE_MAGIC_FACTORY: {
            auto* inst = Cast<AllocateMagicFactory>(base_inst);
            if (!IsAllocateMagicFactoryRunnable(beat, inst)) {
                return false;
            }
            RunAllocateMagicFactory(beat, inst);
            RunInstructionInQueue(queue, inst);
            return true;
        }
        case ScLsInstructionType::ALLOCATE_ENTANGLEMENT_FACTORY: {
            auto* inst = Cast<AllocateEntanglementFactory>(base_inst);
            if (!IsAllocateEntanglementFactoryRunnable(beat, inst)) {
                return false;
            }
            RunAllocateEntanglementFactory(beat, inst);
            RunInstructionInQueue(queue, inst);
            return true;
        }
        case ScLsInstructionType::DEALLOCATE: {
            auto* inst = Cast<DeAllocate>(base_inst);
            if (!IsDeAllocateRunnable(beat, inst)) {
                return false;
            }
            RunDeAllocate(beat, inst);
            RunInstructionInQueue(queue, inst);
            return true;
        }
        case ScLsInstructionType::INIT_ZX: {
            auto* inst = Cast<InitZX>(base_inst);
            if (!IsInitZXRunnable(beat, inst)) {
                return false;
            }
            RunInitZX(beat, inst);
            RunInstructionInQueue(queue, inst);
            return true;
        }
        case ScLsInstructionType::MEAS_ZX: {
            auto* inst = Cast<MeasZX>(base_inst);
            if (!IsMeasZXRunnable(beat, inst)) {
                return false;
            }
            RunMeasZX(beat, inst);
            RunInstructionInQueue(queue, inst);
            return true;
        }
        case ScLsInstructionType::MEAS_Y: {
            auto* inst = Cast<MeasY>(base_inst);
            if (!SearchMeasYDir(beat, inst)) {
                return false;
            }
            RunMeasY(beat, inst);
            RunInstructionInQueue(queue, inst);
            return true;
        }
        case ScLsInstructionType::TWIST: {
            auto* inst = Cast<Twist>(base_inst);
            if (!SearchTwistDir(beat, inst)) {
                return false;
            }
            RunTwist(beat, inst);
            RunInstructionInQueue(queue, inst);
            return true;
        }
        case ScLsInstructionType::HADAMARD: {
            auto* inst = Cast<Hadamard>(base_inst);
            if (!IsHadamardRunnable(beat, inst)) {
                return false;
            }
            RunHadamard(beat, inst);
            RunInstructionInQueue(queue, inst);
            return true;
        }
        case ScLsInstructionType::ROTATE: {
            auto* inst = Cast<Rotate>(base_inst);
            if (!SearchRotateDir(beat, inst)) {
                return false;
            }
            RunRotate(beat, inst);
            RunInstructionInQueue(queue, inst);
            return true;
        }
        case ScLsInstructionType::LATTICE_SURGERY: {
            auto* inst = Cast<LatticeSurgery>(base_inst);
            return SearchLatticeSurgeryPathAndRun(beat, queue, mf, inst);
        }
        case ScLsInstructionType::LATTICE_SURGERY_MAGIC: {
            auto* inst = Cast<LatticeSurgeryMagic>(base_inst);
            return SearchLatticeSurgeryMagicPathAndRun(beat, queue, mf, inst);
        }
        case ScLsInstructionType::LATTICE_SURGERY_MULTINODE: {
            auto* inst = Cast<LatticeSurgeryMultinode>(base_inst);
            return SearchLatticeSurgeryMultinodePathAndRun(beat, queue, mf, inst);
        }
        case ScLsInstructionType::MOVE: {
            auto* inst = Cast<Move>(base_inst);
            return SearchMovePathAndRun(beat, queue, mf, inst);
        }
        case ScLsInstructionType::MOVE_MAGIC: {
            auto* inst = Cast<MoveMagic>(base_inst);
            return SearchMoveMagicPathAndRun(beat, queue, mf, inst);
        }
        case ScLsInstructionType::MOVE_ENTANGLEMENT: {
            auto* inst = Cast<MoveEntanglement>(base_inst);
            return SearchMoveEntanglementPathAndRun(beat, queue, mf, inst);
        }
        case ScLsInstructionType::CNOT: {
            auto* inst = Cast<Cnot>(base_inst);
            return SearchCnotPathAndRun(beat, queue, mf, inst);
        }
        case ScLsInstructionType::CNOT_TRANS: {
            auto* inst = Cast<CnotTrans>(base_inst);
            if (!IsCnotTransRunnable(beat, inst)) {
                return false;
            }
            RunCnotTrans(beat, inst);
            RunInstructionInQueue(queue, inst);
            return true;
        }
        case ScLsInstructionType::SWAP_TRANS: {
            auto* inst = Cast<SwapTrans>(base_inst);
            if (!IsSwapTransRunnable(beat, inst)) {
                return false;
            }
            RunSwapTrans(beat, inst);
            RunInstructionInQueue(queue, inst);
            return true;
        }
        case ScLsInstructionType::MOVE_TRANS: {
            auto* inst = Cast<MoveTrans>(base_inst);
            if (!IsMoveTransRunnable(beat, inst)) {
                return false;
            }
            RunMoveTrans(beat, inst);
            RunInstructionInQueue(queue, inst);
            return true;
        }
        case ScLsInstructionType::XOR:
        case ScLsInstructionType::AND:
        case ScLsInstructionType::OR: {
            // Do nothing in simulator.
            auto* inst = Cast<ClassicalOperation>(base_inst);

            for (const auto& c : inst->RegisterList()) {
                const auto itr = avail_.c.find(c);
                if (c.Id() >= NumReservedCSymbols
                    && (itr == avail_.c.end() || beat < itr->second)) {
                    LOG_DEBUG("Target of classical operation is not corrected: {}", c);
                    return false;
                }
            }

            inst->MetadataMut().beat = beat;
            avail_.c[inst->CDest()] = beat;

            RunInstructionInQueue(queue, base_inst);
            return true;
        }
        case ScLsInstructionType::PROBABILITY_HINT: {
            // ProbabilityHint is pseudo operation.
            // Do nothing in simulator.
            auto* inst = Cast<ProbabilityHint>(base_inst);

            inst->MetadataMut().beat = beat;

            RunInstructionInQueue(queue, base_inst);
            return true;
        }
        case ScLsInstructionType::AWAIT_CORRECTION: {
            // AwaitCorrection is pseudo operation.
            // Do nothing in simulator.
            auto* inst = Cast<AwaitCorrection>(base_inst);

            inst->MetadataMut().beat = beat;
            for (const auto& q : inst->QTarget()) {
                avail_.q[q] = beat;
            }

            RunInstructionInQueue(queue, base_inst);
            return true;
        }
        default:
            break;
    }

    assert(0 && "unreachable");
    return false;
}

#pragma region insert instructions
void ScLsSimulator::InsertBeforeAndRunTransMove(
        const Beat beat,
        MachineBasicBlock* mbb,
        ScLsInstructionBase* insert_before,
        const std::list<CSymbol>& condition,
        const QSymbol src_q,
        const Coord3D& src,
        const std::int32_t dst_z
) {
    const auto increase_z = src.z <= dst_z;
    for (auto diff = 0; diff < std::abs(dst_z - src.z); ++diff) {
        // Move transversely from 'src_qubit' to 'dst_qubit'.
        const auto src_qubit = diff == 0 ? src_q : symbol_generator_->LatestQ();
        const auto dst_qubit = symbol_generator_->GenerateQ();
        const auto tmp_dst_z = increase_z ? src.z + diff + 1 : src.z - diff - 1;

        // Allocate 'dst_qubit'.
        auto new_allocate_inst = Allocate::New(dst_qubit, {src.x, src.y, tmp_dst_z}, 0, condition);
        RunAllocate(beat, new_allocate_inst.get());
        mbb->InsertBefore(insert_before, std::move(new_allocate_inst));

        // Move transversely from 'src_qubit' to 'dst_qubit'.
        auto new_move_inst = MoveTrans::New(src_qubit, dst_qubit, condition);
        RunMoveTrans(beat, new_move_inst.get());
        mbb->InsertBefore(insert_before, std::move(new_move_inst));
    }
}
void ScLsSimulator::InsertBeforeAndRunRoute3D(
        const Beat beat,
        MachineBasicBlock* mbb,
        ScLsInstructionBase* insert_before,
        const std::list<CSymbol>& condition,
        const SearchRoute::Route3D& route
) {
    stats_.num_2beat_trans_move++;

    const auto qubit = route.q_src;
    // Move transversely.
    InsertBeforeAndRunTransMove(
            beat,
            mbb,
            insert_before,
            condition,
            qubit,
            route.src,
            route.move_path.front().z
    );

    if (route.move_path.front() == route.logical_path.front()) {
        // Do not need plane move.
        return;
    }

    if (route.move_path.size() <= 1 || route.move_path.front().XY() == route.move_path.back().XY()) {
        // No in-plane move in MOVE path.
        return;
    }

    // Move in plane.
    const auto src_qubit = route.src.z == route.move_path.front().z ? qubit  // If no trans move.
                                                                    : symbol_generator_->LatestQ();
    const auto dst_qubit = symbol_generator_->GenerateQ();

    // Allocate 'dst_qubit'.
    auto new_allocate_inst = Allocate::New(dst_qubit, route.move_path.back(), 0, condition);
    RunAllocate(beat, new_allocate_inst.get());
    mbb->InsertBefore(insert_before, std::move(new_allocate_inst));

    // Move in plane.
    auto path = route.move_path;
    path.pop_back();  // Delete coord of 'src_qubit'.
    path.pop_front();  // Delete coord of 'dst_qubit'.
    auto new_move_inst = Move::New(src_qubit, dst_qubit, path, condition);
    RunMove(beat, new_move_inst.get());
    mbb->InsertBefore(insert_before, std::move(new_move_inst));

    // Step beat: 'beat' -> 'beat+1'.

    // Move transversely.
    InsertBeforeAndRunTransMove(
            beat + 1,
            mbb,
            insert_before,
            condition,
            dst_qubit,
            route.move_path.back(),
            route.logical_path.front().z
    );
}
void ScLsSimulator::InsertBeforeAndRunRoute3DM(
        const Beat beat,
        MachineBasicBlock* mbb,
        ScLsInstructionBase* insert_before,
        const std::list<CSymbol>& condition,
        const SearchRoute::Route3DM& route
) {
    stats_.num_2beat_trans_move++;

    const auto magic_factory = route.magic_factory;

    // Move magic in plane.
    const auto dst_qubit = symbol_generator_->GenerateQ();
    {
        // Allocate 'dst_qubit'.
        auto new_allocate_inst = Allocate::New(dst_qubit, route.move_path.back(), 0, condition);
        RunAllocate(beat, new_allocate_inst.get());
        mbb->InsertBefore(insert_before, std::move(new_allocate_inst));

        // Move magic in plane.
        auto path = route.move_path;
        path.pop_back();  // Delete coord of 'src_qubit'.
        path.pop_front();  // Delete coord of 'dst_qubit'.
        auto new_mm_inst = MoveMagic::New(magic_factory, dst_qubit, path, condition);
        RunMoveMagic(beat, new_mm_inst.get(), route.moved_state_dir);
        mbb->InsertBefore(insert_before, std::move(new_mm_inst));
    }

    // Step beat: 'beat' -> 'beat+1'.

    // Move transversely.
    InsertBeforeAndRunTransMove(
            beat + 1,
            mbb,
            insert_before,
            condition,
            dst_qubit,
            route.move_path.back(),
            route.logical_path.front().z
    );
}
#pragma endregion

#pragma region Allocate
bool ScLsSimulator::IsAllocateRunnable(Beat beat, Allocate* inst) {
    const auto& dest = inst->Dest();
    const auto qubit = inst->Qubit();

    if (!PlaceIsAvailable(beat, dest)) {
        return false;
    }
    if (avail_.q.contains(qubit)) {
        LOG_WARN("Cannot allocate the same QSymbol {} more than once.", qubit);
        return false;
    }
    if (avail_.q_del.contains(qubit) && beat < avail_.q_del.at(qubit)) {
        return false;
    }
    return true;
}
void ScLsSimulator::RunAllocate(Beat beat, Allocate* inst) {
    const auto& dest = inst->Dest();
    const auto qubit = inst->Qubit();
    const auto dir = inst->Dir();

    // Update buffer.
    GetStateBuffer().PutQubit(beat, qubit, dest, dir);

    // Update avail_.
    avail_.q[qubit] = beat;
    avail_.p[dest] = InfBeat;

    // Update metadata.
    inst->MetadataMut() = {beat, dest.z};
}
#pragma endregion

#pragma region AllocateMagicFactory
bool ScLsSimulator::IsAllocateMagicFactoryRunnable(Beat beat, AllocateMagicFactory* inst) {
    const auto& dest = inst->Dest();
    const auto magic_factory = inst->MagicFactory();

    if (!PlaceIsAvailable(beat, dest)) {
        LOG_WARN("Failed to allocate magic factory: {}", inst->ToString());
        return false;
    }
    if (avail_.m.contains(magic_factory)) {
        LOG_WARN("Cannot allocate the same MSymbol: {} more than once.", magic_factory);
        return false;
    }
    return true;
}
void ScLsSimulator::RunAllocateMagicFactory(Beat beat, AllocateMagicFactory* inst) {
    const auto& dest = inst->Dest();
    const auto magic_factory = inst->MagicFactory();

    // Update buffer.
    GetStateBuffer().PutMagicFactory(
            beat,
            magic_factory,
            dest,
            MagicFactoryState::Empty(option_.magic_factory_seed_offset + magic_factory.Id())
    );

    // Update avail_.
    avail_.m[magic_factory] = beat;
    avail_.p[dest] = InfBeat;

    // Update metadata.
    inst->MetadataMut() = {beat, dest.z};
}
#pragma endregion

#pragma region AllocateEntanglementFactory
bool ScLsSimulator::IsAllocateEntanglementFactoryRunnable(
        Beat beat,
        AllocateEntanglementFactory* inst
) {
    const auto& dest1 = inst->Dest1();
    const auto& dest2 = inst->Dest2();
    const auto entanglement_factory1 = inst->EntanglementFactory1();
    const auto entanglement_factory2 = inst->EntanglementFactory2();

    if (!PlaceIsAvailable(beat, dest1) || !PlaceIsAvailable(beat, dest2)) {
        LOG_WARN("Failed to allocate entanglement factory");
        return false;
    }
    if (avail_.e.contains(entanglement_factory1) || avail_.e.contains(entanglement_factory2)) {
        LOG_WARN("Cannot allocate the same ESymbol more than once.");
        return false;
    }
    return true;
}
void ScLsSimulator::RunAllocateEntanglementFactory(Beat beat, AllocateEntanglementFactory* inst) {
    const auto& dest1 = inst->Dest1();
    const auto& dest2 = inst->Dest2();
    const auto entanglement_factory1 = inst->EntanglementFactory1();
    const auto entanglement_factory2 = inst->EntanglementFactory2();

    // Update buffer.
    GetStateBuffer().PutEntanglementFactory(
            beat,
            entanglement_factory1,
            dest1,
            entanglement_factory2,
            dest2
    );

    // Update avail_.
    avail_.e[entanglement_factory1] = avail_.e[entanglement_factory2] = beat;
    avail_.p[dest1] = avail_.p[dest2] = InfBeat;

    // Update metadata.
    inst->MetadataMut() = {beat, dest1.z};
}
#pragma endregion

#pragma region DeAllocate
bool ScLsSimulator::IsDeAllocateRunnable(Beat beat, DeAllocate* inst) {
    const auto qubit = inst->Qubit();

    return QubitIsAvailable(beat, qubit);
}
void ScLsSimulator::RunDeAllocate(Beat beat, DeAllocate* inst) {
    const auto qubit = inst->Qubit();
    // NOTE: COPY Coord3D to access place.z after its removal. (DO NOT const auto&)
    const auto place = GetStateBuffer().GetQuantumState(beat).GetPlace(qubit);

    // Update buffer.
    GetStateBuffer().RemoveQubit(beat, qubit);

    // Update avail_.
    avail_.q.erase(qubit);
    avail_.q_del[qubit] = beat;
    avail_.p[place] = beat;

    // Update metadata.
    inst->MetadataMut() = {beat, place.z};
}
#pragma endregion

#pragma region InitZX
bool ScLsSimulator::IsInitZXRunnable(Beat beat, InitZX* inst) {
    const auto qubit = inst->Qubit();

    return QubitIsAvailable(beat, qubit);
}
void ScLsSimulator::RunInitZX(Beat beat, InitZX* inst) {
    const auto qubit = inst->Qubit();
    const auto& place = GetStateBuffer().GetQuantumState(beat).GetPlace(qubit);

    // Update buffer.
    // Update avail_.
    // Update metadata.
    inst->MetadataMut() = {beat, place.z};
}
#pragma endregion

#pragma region MeasZX
bool ScLsSimulator::IsMeasZXRunnable(Beat beat, MeasZX* inst) {
    const auto qubit = inst->Qubit();

    return QubitIsAvailable(beat, qubit);
}
void ScLsSimulator::RunMeasZX(Beat beat, MeasZX* inst) {
    const auto qubit = inst->Qubit();
    const auto cdest = inst->CDest();
    const auto& place = GetStateBuffer().GetQuantumState(beat).GetPlace(qubit);

    // Update buffer.
    // Update avail_.
    avail_.c[cdest] = beat + inst->StartCorrecting() + option_.reaction_time;

    // Update metadata.
    inst->MetadataMut() = {beat, place.z};
}
#pragma endregion

#pragma region MeasY
bool ScLsSimulator::SearchMeasYDir(Beat beat, MeasY* inst) {
    const auto qubit = inst->Qubit();
    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto& place = state.GetPlace(qubit);

    if (!QubitIsAvailable(beat, qubit)) {
        return false;
    }
    inst->SetDir(4);
    for (auto dir = std::uint32_t{0}; dir < 4; ++dir) {
        const auto ancillae = MeasY::Ancillae(place, dir);
        auto runnable = true;
        for (auto b = beat; b < beat + inst->Latency(); ++b) {
            auto& plane = GetStateBuffer().GetQuantumState(b).GetGrid(place.z).GetPlane(place.z);
            for (const auto& coord : ancillae) {
                if (plane.GetTopology().OutOfPlane(coord.XY())
                    || !plane.GetNode(coord.XY()).is_available) {
                    runnable = false;
                    break;
                }
            }
        }
        if (runnable) {
            inst->SetDir(dir);
            break;
        }
    }
    return inst->Dir() != 4;
}
bool ScLsSimulator::IsMeasYRunnable(Beat beat, MeasY* inst) {
    const auto qubit = inst->Qubit();
    const auto dir = inst->Dir();
    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto& place = state.GetPlace(qubit);

    if (!QubitIsAvailable(beat, qubit)) {
        return false;
    }
    if (dir >= 4) {
        return false;
    }
    for (auto b = beat; b < beat + inst->Latency(); ++b) {
        auto& plane = GetStateBuffer().GetQuantumState(b).GetGrid(place.z).GetPlane(place.z);
        const auto& topology = plane.GetTopology();
        const auto ancillae = MeasY::Ancillae(place, dir);

        if (topology.OutOfPlane(place.XY())) {
            return false;
        }
        if (!plane.GetNode(place.XY()).is_available) {
            return false;
        }
        for (const auto& coord : ancillae) {
            if (topology.OutOfPlane(coord.XY())) {
                return false;
            }
            if (!plane.GetNode(coord.XY()).IsFreeAncilla()) {
                return false;
            }
        }
    }

    return true;
}
void ScLsSimulator::RunMeasY(Beat beat, MeasY* inst) {
    const auto qubit = inst->Qubit();
    const auto dir = inst->Dir();
    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto& place = state.GetPlace(qubit);

    // Update buffer.
    const auto ancillae = MeasY::Ancillae(place, dir);
    for (auto b = beat; b < beat + inst->Latency(); ++b) {
        auto& plane = GetStateBuffer().GetQuantumState(b).GetGrid(place.z).GetPlane(place.z);
        plane.GetNode(place.XY()).is_available = false;
        for (const auto& coord : ancillae) {
            plane.GetNode(coord.XY()).is_available = false;
        }
    }

    // Update avail_.
    for (const auto& coord : ancillae) {
        avail_.p[coord] = beat + inst->Latency();
    }
    avail_.q[qubit] = beat + inst->Latency();
    const auto cdest = inst->CDest();
    avail_.c[cdest] = beat + inst->StartCorrecting() + option_.reaction_time;

    // Update metadata.
    inst->MetadataMut() = {beat, place.z};
}
#pragma endregion

#pragma region Twist
bool ScLsSimulator::SearchTwistDir(Beat beat, Twist* inst) {
    const auto qubit = inst->Qubit();
    const auto& place = GetStateBuffer().GetQuantumState(beat).GetPlace(qubit);
    const auto& qubit_dir = GetStateBuffer().GetQuantumState(beat).GetDir(qubit);

    if (!QubitIsAvailable(beat, qubit)) {
        return false;
    }
    inst->SetDir(4);
    const auto directions = [inst, qubit_dir]() {
        const auto pauli = inst->GetTargetPauli();
        if (!pauli) {
            return std::vector<std::uint32_t>{0, 1, 2, 3};
        }
        return Twist::PossibleDirections(qubit_dir, *pauli);
    }();
    for (const auto dir : directions) {
        const auto ancilla = Twist::GetAncilla(place, dir);
        auto runnable = true;
        for (auto b = beat; b < beat + inst->Latency(); ++b) {
            auto& plane = GetStateBuffer().GetQuantumState(b).GetGrid(place.z).GetPlane(place.z);
            if (plane.GetTopology().OutOfPlane(ancilla.XY())
                || !plane.GetNode(ancilla.XY()).is_available) {
                runnable = false;
                break;
            }
        }
        if (runnable) {
            inst->SetDir(dir);
            break;
        }
    }
    return inst->Dir() != 4;
}
bool ScLsSimulator::IsTwistRunnable(Beat beat, Twist* inst) {
    const auto qubit = inst->Qubit();
    const auto dir = inst->Dir();
    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto& place = state.GetPlace(qubit);

    if (!QubitIsAvailable(beat, qubit)) {
        return false;
    }
    if (dir >= 4) {
        return false;
    }
    for (auto b = beat; b < beat + inst->Latency(); ++b) {
        auto& plane = GetStateBuffer().GetQuantumState(b).GetGrid(place.z).GetPlane(place.z);
        const auto& topology = plane.GetTopology();
        const auto ancilla = Twist::GetAncilla(place, dir);

        if (topology.OutOfPlane(place.XY())) {
            return false;
        }
        if (!plane.GetNode(place.XY()).is_available) {
            return false;
        }
        if (topology.OutOfPlane(ancilla.XY())) {
            return false;
        }
        if (!plane.GetNode(ancilla.XY()).IsFreeAncilla()) {
            return false;
        }
    }
    return true;
}
void ScLsSimulator::RunTwist(Beat beat, Twist* inst) {
    const auto qubit = inst->Qubit();
    const auto dir = inst->Dir();
    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto& place = state.GetPlace(qubit);

    // Update buffer.
    const auto ancilla = Twist::GetAncilla(place, dir);
    for (auto b = beat; b < beat + inst->Latency(); ++b) {
        auto& plane = GetStateBuffer().GetQuantumState(b).GetGrid(place.z).GetPlane(place.z);
        plane.GetNode(place.XY()).is_available = false;
        plane.GetNode(ancilla.XY()).is_available = false;
    }

    // Update avail_;
    avail_.p[ancilla] = beat + inst->Latency();
    avail_.q[qubit] = beat + inst->Latency();

    // Update metadata.
    inst->MetadataMut() = {beat, place.z};
}
#pragma endregion

#pragma region Hadamard
bool ScLsSimulator::IsHadamardRunnable(Beat beat, Hadamard* inst) {
    const auto qubit = inst->Qubit();

    return QubitIsAvailable(beat, qubit);
}
void ScLsSimulator::RunHadamard(Beat beat, Hadamard* inst) {
    const auto qubit = inst->Qubit();
    const auto& place = GetStateBuffer().GetQuantumState(beat).GetPlace(qubit);

    // Update buffer.
    GetStateBuffer().RotateQubit(beat, qubit);

    // Update avail_;
    // Update metadata.
    inst->MetadataMut() = {beat, place.z};
}
#pragma endregion

#pragma region Rotate
bool ScLsSimulator::SearchRotateDir(Beat beat, Rotate* inst) {
    const auto qubit = inst->Qubit();
    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto& place = state.GetPlace(qubit);

    if (!QubitIsAvailable(beat, qubit)) {
        return false;
    }
    inst->SetDir(4);
    for (auto dir = std::uint32_t{0}; dir < 4; ++dir) {
        const auto ancilla = Rotate::Ancilla(place, dir);
        auto runnable = true;
        for (auto b = beat; b < beat + inst->Latency(); ++b) {
            auto& plane = GetStateBuffer().GetQuantumState(b).GetGrid(place.z).GetPlane(place.z);
            if (plane.GetTopology().OutOfPlane(ancilla.XY())
                || !plane.GetNode(ancilla.XY()).is_available) {
                runnable = false;
                break;
            }
        }
        if (runnable) {
            inst->SetDir(dir);
            break;
        }
    }
    return inst->Dir() != 4;
}
bool ScLsSimulator::IsRotateRunnable(Beat beat, Rotate* inst) {
    const auto qubit = inst->Qubit();
    const auto dir = inst->Dir();
    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto& place = state.GetPlace(qubit);

    if (!QubitIsAvailable(beat, qubit)) {
        return false;
    }
    if (dir >= 4) {
        return false;
    }
    for (auto b = beat; b < beat + inst->Latency(); ++b) {
        auto& plane = GetStateBuffer().GetQuantumState(b).GetGrid(place.z).GetPlane(place.z);
        const auto& topology = plane.GetTopology();
        const auto ancilla = Rotate::Ancilla(place, dir);

        if (topology.OutOfPlane(place.XY())) {
            return false;
        }
        if (!plane.GetNode(place.XY()).is_available) {
            return false;
        }
        if (topology.OutOfPlane(ancilla.XY())) {
            return false;
        }
        if (!plane.GetNode(ancilla.XY()).IsFreeAncilla()) {
            return false;
        }
    }

    return true;
}
void ScLsSimulator::RunRotate(Beat beat, Rotate* inst) {
    const auto qubit = inst->Qubit();
    const auto dir = inst->Dir();
    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto& place = state.GetPlace(qubit);

    // Update buffer.
    const auto ancilla = Rotate::Ancilla(place, dir);
    for (auto b = beat; b < beat + inst->Latency(); ++b) {
        auto& plane = GetStateBuffer().GetQuantumState(b).GetGrid(place.z).GetPlane(place.z);
        plane.GetNode(place.XY()).is_available = false;
        plane.GetNode(ancilla.XY()).is_available = false;
    }
    GetStateBuffer().RotateQubit(beat + inst->Latency(), qubit);

    // Update avail_.
    avail_.p[ancilla] = beat + inst->Latency();
    avail_.q[qubit] = beat + inst->Latency();

    // Update metadata.
    inst->MetadataMut() = {beat, place.z};
}
#pragma endregion

#pragma region search helpers
std::optional<SearchRoute::Route2D> DefaultRouteSearcher::SearchRoute2D(
        QuantumStateBuffer& buffer,
        Beat beat,
        QSymbol q_src,
        QSymbol q_dst,
        std::uint32_t boundaries_src,
        std::uint32_t boundaries_dst
) {
    auto& state = buffer.GetQuantumState(beat);
    const auto& place_src = state.GetPlace(q_src);
    auto& plane = state.GetGrid(place_src.z).GetPlane(place_src.z);
    SearchHelper::SetCostsOfFreeAncillae(plane, std::numeric_limits<std::uint64_t>::max());
    return SearchRoute::FindRoute2D(plane, q_src, q_dst, boundaries_src, boundaries_dst);
}
std::optional<SearchRoute::Route2DM> DefaultRouteSearcher::SearchRoute2DM(
        QuantumStateBuffer& buffer,
        Beat beat,
        QSymbol q_dst,
        std::uint32_t boundaries_dst
) {
    auto& state = buffer.GetQuantumState(beat);
    const auto& place_dst = state.GetPlace(q_dst);
    auto& plane = state.GetGrid(place_dst.z).GetPlane(place_dst.z);
    SearchHelper::SetCostsOfFreeAncillae(plane, std::numeric_limits<std::uint64_t>::max());
    return SearchRoute::FindRoute2DM(plane, q_dst, boundaries_dst);
}
std::optional<SearchRoute::Route2DE> DefaultRouteSearcher::SearchRoute2DE(
        QuantumStateBuffer& buffer,
        Beat beat,
        ESymbol e_factory,
        QSymbol q_dst,
        std::uint32_t boundaries_dst
) {
    auto& state = buffer.GetQuantumState(beat);
    const auto& place_dst = state.GetPlace(q_dst);
    auto& plane = state.GetGrid(place_dst.z).GetPlane(place_dst.z);
    SearchHelper::SetCostsOfFreeAncillae(plane, std::numeric_limits<std::uint64_t>::max());
    return SearchRoute::FindRoute2DE(plane, e_factory, q_dst, boundaries_dst);
}
std::optional<SearchRoute::Route3D> DefaultRouteSearcher::SearchRoute3D(
        QuantumStateBuffer& buffer,
        Beat beat,
        QSymbol q_src,
        QSymbol q_dst,
        std::uint32_t boundaries_src,
        std::uint32_t boundaries_dst
) {
    auto& state_0 = buffer.GetQuantumState(beat);
    auto& state_1 = buffer.GetQuantumState(beat + 1);
    auto& state_2 = buffer.GetQuantumState(beat + 2);
    const auto& place_src = state_0.GetPlace(q_src);
    const auto& place_dst = state_0.GetPlace(q_dst);
    auto& grid_0 = state_0.GetGrid(place_src.z);
    auto& grid_1 = state_1.GetGrid(place_src.z);
    auto& grid_2 = state_2.GetGrid(place_src.z);

    SearchHelper::SetCostsOfFreeAncillae(grid_0, std::numeric_limits<std::uint64_t>::max());
    SearchHelper::SetCostsOfFreeAncillae(grid_2, std::numeric_limits<std::uint64_t>::max());

    if (!SearchRoute::LS(grid_2.GetPlane(place_dst.z), q_dst, boundaries_dst)) {
        return std::nullopt;
    }
    return SearchRoute::FindRoute3D(
            grid_0,
            grid_1,
            grid_2.GetPlane(place_dst.z),
            q_src,
            q_dst,
            boundaries_src
    );
}
std::optional<SearchRoute::Route3DM> DefaultRouteSearcher::SearchRoute3DM(
        QuantumStateBuffer& buffer,
        Beat beat,
        QSymbol q_dst,
        std::uint32_t boundaries_dst
) {
    auto& state_0 = buffer.GetQuantumState(beat);
    auto& state_1 = buffer.GetQuantumState(beat + 1);
    auto& state_2 = buffer.GetQuantumState(beat + 2);
    const auto& place_dst = state_0.GetPlace(q_dst);
    auto& grid_0 = state_0.GetGrid(place_dst.z);
    auto& grid_1 = state_1.GetGrid(place_dst.z);
    auto& grid_2 = state_2.GetGrid(place_dst.z);

    SearchHelper::SetCostsOfFreeAncillae(grid_0, std::numeric_limits<std::uint64_t>::max());
    SearchHelper::SetCostsOfFreeAncillae(grid_2, std::numeric_limits<std::uint64_t>::max());

    if (!SearchRoute::LS(grid_2.GetPlane(place_dst.z), q_dst, boundaries_dst)) {
        return std::nullopt;
    }
    return SearchRoute::FindRoute3DM(grid_0, grid_1, grid_2.GetPlane(place_dst.z), q_dst);
}

std::optional<SearchRoute::Route2D> ScLsSimulator::SearchRoute2D(
        Beat beat,
        QSymbol q_src,
        QSymbol q_dst,
        std::uint32_t boundaries_src,
        std::uint32_t boundaries_dst
) {
    try {
        return route_searcher_->SearchRoute2D(
                GetStateBuffer(),
                beat,
                q_src,
                q_dst,
                boundaries_src,
                boundaries_dst
        );
    } catch (const std::logic_error& err) {
        LOG_ERROR(
                "[BEAT {}] SearchRoute2D({}, {}) failed: {}",
                beat,
                q_src,
                q_dst,
                err.what()
        );
        throw;
    }
}
std::optional<SearchRoute::Route2DM>
ScLsSimulator::SearchRoute2DM(Beat beat, QSymbol q_dst, std::uint32_t boundaries_dst) {
    try {
        return route_searcher_->SearchRoute2DM(GetStateBuffer(), beat, q_dst, boundaries_dst);
    } catch (const std::logic_error& err) {
        LOG_ERROR(
                "[BEAT {}] SearchRoute2DM({}) failed: {}",
                beat,
                q_dst,
                err.what()
        );
        throw;
    }
}
std::optional<SearchRoute::Route2DE> ScLsSimulator::SearchRoute2DE(
        Beat beat,
        ESymbol e_factory,
        QSymbol q_dst,
        std::uint32_t boundaries_dst
) {
    try {
        return route_searcher_->SearchRoute2DE(
                GetStateBuffer(),
                beat,
                e_factory,
                q_dst,
                boundaries_dst
        );
    } catch (const std::logic_error& err) {
        LOG_ERROR(
                "[BEAT {}] SearchRoute2DE({}, {}) failed: {}",
                beat,
                e_factory,
                q_dst,
                err.what()
        );
        throw;
    }
}
std::optional<SearchRoute::Route3D> ScLsSimulator::SearchRoute3D(
        Beat beat,
        QSymbol q_src,
        QSymbol q_dst,
        std::uint32_t boundaries_src,
        std::uint32_t boundaries_dst
) {
    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto& place_src = state.GetPlace(q_src);
    const auto& place_dst = state.GetPlace(q_dst);
    LOG_DEBUG(
            "[BEAT {}] SearchRoute3D: {}@{} -> {}@{}",
            beat,
            q_src,
            place_src,
            q_dst,
            place_dst
    );
    try {
        return route_searcher_->SearchRoute3D(
                GetStateBuffer(),
                beat,
                q_src,
                q_dst,
                boundaries_src,
                boundaries_dst
        );
    } catch (const std::logic_error& err) {
        LOG_ERROR(
                "[BEAT {}] SearchRoute3D({}, {}) failed: {}",
                beat,
                q_src,
                q_dst,
                err.what()
        );
        throw;
    }
}
std::optional<SearchRoute::Route3DM>
ScLsSimulator::SearchRoute3DM(Beat beat, QSymbol q_dst, std::uint32_t boundaries_dst) {
    try {
        return route_searcher_->SearchRoute3DM(GetStateBuffer(), beat, q_dst, boundaries_dst);
    } catch (const std::logic_error& err) {
        LOG_ERROR(
                "[BEAT {}] SearchRoute3DM({}) failed: {}",
                beat,
                q_dst,
                err.what()
        );
        throw;
    }
}
#pragma endregion

#pragma region LatticeSurgery
bool ScLsSimulator::SearchLatticeSurgeryPathAndRun(
        Beat beat,
        InstQueue& queue,
        MachineFunction& mf,
        LatticeSurgery* inst
) {
    if (!QubitsAreAvailable(beat, inst->QubitList())) {
        return false;
    }
    if (inst->QubitList().size() <= 1) {
        RunInstructionInQueue(queue, inst);
        return true;
    }

    if (SymbolsAreOnTheSamePlane(beat, inst->QTarget(), {}, {})) {
        if (inst->QubitList().size() == 2) {
            return SearchLatticeSurgeryPath2DBFSAndRun(beat, queue, mf, inst);
        } else {
            return SearchLatticeSurgeryPath2DSteinerAndRun(beat, queue, mf, inst);
        }
    } else {
        return SearchLatticeSurgeryPath3DAndRun(beat, queue, mf, inst);
    }
}
bool ScLsSimulator::SearchLatticeSurgeryPath2DBFSAndRun(
        Beat beat,
        InstQueue& queue,
        MachineFunction&,
        LatticeSurgery* inst
) {
    if (inst->QubitList().size() >= 3) {
        throw std::logic_error("Use SearchLatticeSurgeryPath2DGeneralAndRun.");
    }

    const auto q_src = inst->QubitList().front();
    const auto q_dst = inst->QubitList().back();
    const auto pauli_src = inst->BasisList().front();
    const auto pauli_dst = inst->BasisList().back();

    const auto tmp_route = SearchRoute2D(
            beat,
            q_src,
            q_dst,
            SearchRoute::GetBoundaryCode(pauli_src),
            SearchRoute::GetBoundaryCode(pauli_dst)
    );
    if (!tmp_route) {
        return false;
    }

    // Found route.
    const auto& route = tmp_route.value();
    // Update LATTICE_SURGERY.
    auto path = route.logical_path;
    path.pop_front();  // Delete coord of 'q_src'.
    path.pop_back();  // Delete coord of 'q_dst'.
    inst->SetPath(std::move(path));
    // Run LATTICE_SURGERY.
    RunLatticeSurgery(beat, inst);
    RunInstructionInQueue(queue, inst);

    return true;
}
bool ScLsSimulator::SearchLatticeSurgeryPath2DSteinerAndRun(
        Beat beat,
        InstQueue& queue,
        MachineFunction&,
        LatticeSurgery* inst
) {
    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto q0 = inst->QubitList().front();
    const auto q0_z = state.GetPlace(q0).z;
    const auto tmp_ancilla = SearchRoute::FindAncilla(
            state.GetGrid(q0_z).GetPlane(q0_z),
            inst->QubitList(),
            inst->BasisList(),
            {},
            false
    );
    if (!tmp_ancilla) {
        return false;
    }

    // Found ancilla.
    const auto& ancilla = tmp_ancilla.value();
    // Update LATTICE_SURGERY.
    inst->SetPath(ancilla.ancilla);
    // Run LATTICE_SURGERY.
    RunLatticeSurgery(beat, inst);
    RunInstructionInQueue(queue, inst);

    return true;
}
bool ScLsSimulator::SearchLatticeSurgeryPath3DAndRun(
        Beat beat,
        InstQueue& queue,
        MachineFunction& mf,
        LatticeSurgery* inst
) {
    if (inst->QubitList().size() >= 3) {
        throw std::runtime_error("3D LATTICE_SURGERY with more than two qubits is unimplemented.");
    }

    // Move 'q_src' to the plane of 'q_dst'.
    const auto q_src = inst->QubitList().front();
    const auto q_dst = inst->QubitList().back();
    const auto pauli_src = inst->BasisList().front();
    const auto pauli_dst = inst->BasisList().back();

    // 1: Check if LATTICE_SURGERY is realizable.
    // Search 3D route from 'q_src' to the plane of 'q_dst'.
    const auto tmp_route = SearchRoute3D(
            beat,
            q_src,
            q_dst,
            SearchRoute::GetBoundaryCode(pauli_src),
            SearchRoute::GetBoundaryCode(pauli_dst)
    );
    if (!tmp_route) {
        return false;
    }

    // Found route.
    const auto& route = tmp_route.value();

    // 2: Run route instructions and LATTICE_SURGERY.
    auto* mbb = FindMBB(mf, inst);

    // Insert: trans move + plain move + trans move.
    InsertBeforeAndRunRoute3D(beat, mbb, inst, inst->Condition(), route);

    // Update LATTICE_SURGERY.
    const auto moved_q_src = symbol_generator_->LatestQ();
    auto qubit_list = inst->QubitList();
    qubit_list.front() = moved_q_src;  // Replace 'q_src' with 'moved_q_src'.
    inst->SetQubitList(std::move(qubit_list));
    auto path = route.logical_path;
    path.pop_front();  // Delete coord of 'moved_q_src'.
    path.pop_back();  // Delete coord of 'q_dst'.
    inst->SetPath(std::move(path));

    // Run LATTICE_SURGERY.
    RunLatticeSurgery(beat + 2, inst);
    RunFutureInstructionInQueue(beat + 2, queue, inst);

    // 3: Insert inverse move instructions. (For now, these will be only inserted and not run.)

    // Allocate q_src.
    auto new_allocate_inst = Allocate::New(q_src, route.src, 0, inst->Condition());
    auto* ptr = new_allocate_inst.get();
    queue.InsertAfter(inst, ptr);
    mbb->InsertAfter(inst, std::move(new_allocate_inst));

    // Inverse move from 'move_q_src' to q_src.
    auto new_move_inst = Move::New(moved_q_src, q_src, {}, inst->Condition());
    queue.InsertAfter(ptr, new_move_inst.get());
    mbb->InsertAfter(ptr, std::move(new_move_inst));

    return true;
}
bool ScLsSimulator::IsLatticeSurgeryRunnable(Beat beat, LatticeSurgery* inst) {
    const auto& qubits = inst->QubitList();
    if (qubits.size() <= 1) {
        return true;
    }

    const auto z = GetStateBuffer().GetQuantumState(beat).GetPlace(qubits.front()).z;
    const auto& topology =
            GetStateBuffer().GetQuantumState(beat).GetGrid(z).GetPlane(z).GetTopology();

    if (!QubitsAreAvailable(beat, qubits)) {
        return false;
    }
    for (auto b = beat; b < beat + inst->Latency(); ++b) {
        auto& state = GetStateBuffer().GetQuantumState(b);
        auto& plane = state.GetGrid(z).GetPlane(z);

        for (const auto& q : inst->QubitList()) {
            const auto& place = state.GetPlace(q);
            if (z != place.z) {
                return false;
            }
            if (!plane.GetNode(place.XY()).is_available) {
                return false;
            }
        }
        for (const auto& coord : inst->Path()) {
            if (z != coord.z) {
                return false;
            }
            if (topology.OutOfPlane(coord.XY())) {
                return false;
            }
            if (!plane.GetNode(coord.XY()).IsFreeAncilla()) {
                return false;
            }
        }
    }
    return HasOnlyBoundaryPauli(inst->BasisList());
}
void ScLsSimulator::RunLatticeSurgery(Beat beat, LatticeSurgery* inst) {
    const auto& qubits = inst->QubitList();
    if (qubits.size() <= 1) {
        return;
    }

    const auto z = GetStateBuffer().GetQuantumState(beat).GetPlace(qubits.front()).z;

    // Update buffer.
    for (auto b = beat; b < beat + inst->Latency(); ++b) {
        auto& state = GetStateBuffer().GetQuantumState(b);
        auto& plane = state.GetGrid(z).GetPlane(z);

        for (const auto& q : inst->QubitList()) {
            const auto& place = state.GetPlace(q);
            plane.GetNode(place.XY()).is_available = false;
        }
        for (const auto& coord : inst->Path()) {
            plane.GetNode(coord.XY()).is_available = false;
        }
    }

    // Update avail_.
    for (const auto q : inst->QubitList()) {
        avail_.q[q] = beat + inst->Latency();
    }
    for (const auto& coord : inst->Path()) {
        avail_.p[coord] = std::max(avail_.p[coord], beat + inst->Latency());
    }
    const auto cdest = inst->CDest();
    avail_.c[cdest] = beat + inst->StartCorrecting() + option_.reaction_time;

    // Update metadata.
    inst->MetadataMut() = {beat, z};
}
#pragma endregion

#pragma region LatticeSurgeryMagic
bool ScLsSimulator::SearchLatticeSurgeryMagicPathAndRun(
        Beat beat,
        InstQueue& queue,
        MachineFunction& mf,
        LatticeSurgeryMagic* inst
) {
    if (!QubitsAreAvailable(beat, inst->QubitList())) {
        return false;
    }
    if (inst->QubitList().size() <= 0) {
        RunInstructionInQueue(queue, inst);
        return true;
    }

    if (!SymbolsAreOnTheSamePlane(beat, inst->QTarget(), {}, {})) {
        return SearchLatticeSurgeryMagicPath3DAndRun(beat, queue, mf, inst);
    }

    // All qubits are on the same plane.

    // Find available magic factory in the same plane.
    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto qubit = inst->QubitList().front();
    const auto& place = state.GetPlace(qubit);
    for (const auto& magic_factory : state.GetMagicFactoryList(place.z)) {
        if (state.IsAvailable(magic_factory)) {
            if (inst->QubitList().size() == 1) {
                return SearchLatticeSurgeryMagicPath2DBFSAndRun(beat, queue, mf, inst);
            } else {
                return SearchLatticeSurgeryMagicPath2DSteinerAndRun(beat, queue, mf, inst);
            }
        }
    }

    // Find available magic factory in the same grid.
    auto& grid = state.GetGrid(place.z);
    for (auto& plane : grid) {
        const auto z = plane.GetTopology().GetZ();
        for (const auto& magic_factory : state.GetMagicFactoryList(z)) {
            if (state.IsAvailable(magic_factory)) {
                return SearchLatticeSurgeryMagicPath3DAndRun(beat, queue, mf, inst);
            }
        }
    }

    // No available magic factory in this grid.
    LOG_DEBUG("No available magic factory in this grid: {} (beat: {})", place.z, beat);

    return false;
}
bool ScLsSimulator::SearchLatticeSurgeryMagicPath2DBFSAndRun(
        Beat beat,
        InstQueue& queue,
        MachineFunction&,
        LatticeSurgeryMagic* inst
) {
    if (inst->QubitList().size() >= 2) {
        throw std::logic_error("Use SearchLatticeSurgeryMagicPath2DBFSAndRun.");
    }

    const auto q_dst = inst->QubitList().front();
    const auto pauli_dst = inst->BasisList().front();

    const auto tmp_route = SearchRoute2DM(beat, q_dst, SearchRoute::GetBoundaryCode(pauli_dst));
    if (!tmp_route) {
        return false;
    }

    // Found route.
    const auto& route = tmp_route.value();
    // Update LATTICE_SURGERY_MAGIC.
    inst->SetMagicFactory(route.magic_factory);
    auto path = route.logical_path;
    path.pop_front();  // Delete coord of magic factory.
    path.pop_back();  // Delete coord of 'q_dst'.
    inst->SetPath(std::move(path));
    // Run LATTICE_SURGERY_MAGIC.
    RunLatticeSurgeryMagic(beat, inst);
    RunInstructionInQueue(queue, inst);

    return true;
}
bool ScLsSimulator::SearchLatticeSurgeryMagicPath2DSteinerAndRun(
        Beat beat,
        InstQueue& queue,
        MachineFunction&,
        LatticeSurgeryMagic* inst
) {
    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto q0 = inst->QubitList().front();
    const auto q0_z = state.GetPlace(q0).z;
    const auto tmp_ancilla = SearchRoute::FindAncilla(
            state.GetGrid(q0_z).GetPlane(q0_z),
            inst->QubitList(),
            inst->BasisList(),
            {},
            true
    );
    if (!tmp_ancilla) {
        return false;
    }

    // Found ancilla.
    const auto& ancilla = tmp_ancilla.value();
    // Update LATTICE_SURGERY_MAGIC.
    if (!ancilla.m.has_value()) {
        throw std::logic_error("Ancilla does not have magic factory.");
    }
    inst->SetMagicFactory(ancilla.m.value());
    inst->SetPath(ancilla.ancilla);
    // Run LATTICE_SURGERY_MAGIC.
    RunLatticeSurgeryMagic(beat, inst);
    RunInstructionInQueue(queue, inst);

    return true;
}
bool ScLsSimulator::SearchLatticeSurgeryMagicPath3DAndRun(
        Beat beat,
        InstQueue& queue,
        MachineFunction& mf,
        LatticeSurgeryMagic* inst
) {
    if (inst->QubitList().size() >= 2) {
        throw std::runtime_error(
                "3D LATTICE_SURGERY_MAGIC with more than one qubit is unimplemented"
        );
    }

    // Move magic state to the plane of qubit.
    // LATTICE_SURGERY_MAGIC = MOVE_MAGIC + LATTICE_SURGERY.

    const auto q_dst = inst->QubitList().front();
    const auto pauli_dst = inst->BasisList().front();

    // 1: Check if LATTICE_SURGERY_MAGIC is realizable.
    // Search 3D route from magic factory to the plane of 'q_dst'.
    const auto tmp_route = SearchRoute3DM(beat, q_dst, SearchRoute::GetBoundaryCode(pauli_dst));
    if (!tmp_route) {
        return false;
    }

    // Found route.
    const auto& route = tmp_route.value();

    // 2: Run 'route' instructions and LATTICE_SURGERY_MAGIC.
    auto* mbb = FindMBB(mf, inst);

    // Insert: move magic + trans move.
    InsertBeforeAndRunRoute3DM(beat, mbb, inst, inst->Condition(), route);

    // Replace LATTICE_SURGERY_MAGIC with LATTICE_SURGERY + DEALLOCATE.
    // => Insert LATTICE_SURGERY + DEALLOCATE and erase LATTICE_SURGERY_MAGIC.

    // Insert LATTICE_SURGERY.
    const auto moved_magic_state = symbol_generator_->LatestQ();
    auto qubit_list = inst->QubitList();
    qubit_list.push_front(moved_magic_state);
    auto basis_list = inst->BasisList();
    basis_list.push_front(pauli_dst.IsZ() ? Pauli::Z() : Pauli::X());
    auto path = route.logical_path;
    path.pop_front();  // Delete coord of 'moved_magic_state'.
    path.pop_back();  // Delete coord of 'q_dst'.
    auto new_ls_inst =
            LatticeSurgery::New(qubit_list, basis_list, path, inst->CDest(), inst->Condition());
    auto* new_ls_inst_ptr = new_ls_inst.get();
    const auto latency = new_ls_inst->Latency();
    RunLatticeSurgery(beat + 2, new_ls_inst_ptr);
    mbb->InsertBefore(inst, std::move(new_ls_inst));

    // Insert DEALLOCATE of 'moved_magic_state'.
    auto new_da_inst = DeAllocate::New(moved_magic_state, inst->Condition());
    RunDeAllocate(beat + 2 + latency, new_da_inst.get());
    mbb->InsertBefore(inst, std::move(new_da_inst));

    // Erase LATTICE_SURGERY_MAGIC.
    // Keep only the executable LS node in the queue; DEALLOCATE is already scheduled in the block.
    queue.Replace(inst, {new_ls_inst_ptr}, latency);
    RunFutureInstructionInQueue(beat + 2, queue, new_ls_inst_ptr);
    mbb->Erase(inst);

    return true;
}
bool ScLsSimulator::IsLatticeSurgeryMagicRunnable(Beat beat, LatticeSurgeryMagic* inst) {
    const auto& qubits = inst->QubitList();
    const auto magic_factory = inst->MagicFactory();
    if (!QubitsAreAvailable(beat, qubits)) {
        return false;
    }
    if (!MagicFactoryIsAvailable(beat, magic_factory)) {
        return false;
    }

    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto& place = state.GetPlace(qubits.front());
    const auto& magic_factory_place = state.GetPlace(magic_factory);
    auto& plane = state.GetGrid(place.z).GetPlane(place.z);
    const auto& topology = plane.GetTopology();

    if (place.z != magic_factory_place.z) {
        return false;
    }
    if (plane.GetNode(magic_factory_place.XY()).is_available) {
        return false;
    }
    if (!state.IsAvailable(magic_factory)) {
        return false;
    }
    for (auto b = beat; b < beat + inst->Latency(); ++b) {
        auto& tmp_state = GetStateBuffer().GetQuantumState(b);
        auto& tmp_plane = tmp_state.GetGrid(place.z).GetPlane(place.z);
        for (const auto& [x, y, z] : inst->Path()) {
            if (place.z != z) {
                return false;
            }
            if (topology.OutOfPlane({x, y})) {
                return false;
            }
            if (!tmp_plane.GetNode({x, y}).IsFreeAncilla()) {
                return false;
            }
        }
        for (const auto& q : inst->QubitList()) {
            const auto& place = tmp_state.GetPlace(q);
            if (!tmp_plane.GetNode(place.XY()).is_available) {
                return false;
            }
        }
    }
    return HasOnlyBoundaryPauli(inst->BasisList());
}
void ScLsSimulator::RunLatticeSurgeryMagic(Beat beat, LatticeSurgeryMagic* inst) {
    const auto& qubits = inst->QubitList();
    const auto magic_factory = inst->MagicFactory();
    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto& place = state.GetPlace(qubits.front());
    // const auto& magic_factory_place = state.GetPlace(magic_factory);
    // auto& plane = state.GetGrid(place.z).GetPlane(place.z);
    // const auto& topology = plane.GetTopology();

    // Update buffer.
    for (auto b = beat; b < beat + inst->Latency(); ++b) {
        auto& tmp_state = GetStateBuffer().GetQuantumState(b);
        auto& tmp_plane = tmp_state.GetGrid(place.z).GetPlane(place.z);
        for (const auto& [x, y, _] : inst->Path()) {
            tmp_plane.GetNode({x, y}).is_available = false;
        }
        for (const auto& q : inst->QubitList()) {
            const auto& place = tmp_state.GetPlace(q);
            tmp_plane.GetNode(place.XY()).is_available = false;
        }
    }
    // Use magic factory.
    GetStateBuffer().UseMagicState(beat, magic_factory);

    // Update avail_.
    for (const auto& coord : inst->Path()) {
        avail_.p[coord] = beat + inst->Latency();
    }
    for (const auto& q : inst->QubitList()) {
        avail_.q[q] = beat + inst->Latency();
    }
    avail_.m[magic_factory] = beat + inst->Latency();
    const auto cdest = inst->CDest();
    avail_.c[cdest] = beat + inst->StartCorrecting() + option_.reaction_time;

    // Update metadata.
    inst->MetadataMut() = {beat, place.z};
}
#pragma endregion

#pragma region LatticeSurgeryMultinode
bool ScLsSimulator::SearchLatticeSurgeryMultinodePathAndRun(
        Beat beat,
        InstQueue& queue,
        MachineFunction&,
        LatticeSurgeryMultinode* inst
) {
    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto can_use_entanglement = [&state](ESymbol e, EHandle h) {
        const auto pair = state.GetTopology().GetPair(e);
        const auto& e_state = state.GetState(e);
        if (e_state.IsReserved(h)) {
            return true;
        }
        return e_state.IsAvailable() && state.GetState(pair).IsAvailable();
    };

    // Trivial cases.
    if (inst->UseMagicFactory() && inst->QubitList().size() + inst->EFactoryList().size() <= 0) {
        RunInstructionInQueue(queue, inst);
        return true;
    }
    if (!inst->UseMagicFactory() && inst->QubitList().size() + inst->EFactoryList().size() <= 1) {
        RunInstructionInQueue(queue, inst);
        return true;
    }

    // Qubits and entanglement factories are available.
    if (!QubitsAreAvailable(beat, inst->QubitList())) {
        return false;
    }
    auto e_itr = inst->EFactoryList().begin();
    auto h_itr = inst->EHandleList().begin();
    while (e_itr != inst->EFactoryList().end() && h_itr != inst->EHandleList().end()) {
        if (!EntanglementFactoryIsAvailable(beat, *e_itr) || !can_use_entanglement(*e_itr, *h_itr)) {
            return false;
        }
        e_itr++;
        h_itr++;
    }

    // Search.
    const auto& place = inst->QubitList().empty() ? state.GetPlace(inst->EFactoryList().front())
                                                  : state.GetPlace(inst->QubitList().front());
    auto& plane = state.GetGrid(place.z).GetPlane(place.z);
    const auto tmp_ancilla = SearchRoute::FindAncilla(
            plane,
            inst->QubitList(),
            inst->BasisList(),
            inst->EFactoryList(),
            inst->UseMagicFactory()
    );
    if (!tmp_ancilla.has_value()) {
        return false;
    }

    // Found ancilla.
    const auto& ancilla = tmp_ancilla.value();
    // Update LATTICE_SURGERY_MULTINODE.
    if (inst->UseMagicFactory()) {
        if (!ancilla.m.has_value()) {
            throw std::logic_error("Magic factory is not set in search function.");
        }
        inst->SetMagicFactory(*ancilla.m);
    }
    inst->SetPath(ancilla.ancilla);
    // Run LATTICE_SURGERY_MULTINODE.
    RunLatticeSurgeryMultinode(beat, inst);
    RunInstructionInQueue(queue, inst);

    return true;
}
bool ScLsSimulator::IsLatticeSurgeryMultinodeRunnable(Beat beat, LatticeSurgeryMultinode* inst) {
    const auto& qubits = inst->QubitList();
    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto can_use_entanglement = [&state](ESymbol e, EHandle h) {
        const auto pair = state.GetTopology().GetPair(e);
        const auto& e_state = state.GetState(e);
        if (e_state.IsReserved(h)) {
            return true;
        }
        return e_state.IsAvailable() && state.GetState(pair).IsAvailable();
    };

    if (!QubitsAreAvailable(beat, qubits)) {
        return false;
    }
    auto e_itr = inst->EFactoryList().begin();
    auto h_itr = inst->EHandleList().begin();
    while (e_itr != inst->EFactoryList().end() && h_itr != inst->EHandleList().end()) {
        if (!EntanglementFactoryIsAvailable(beat, *e_itr) || !can_use_entanglement(*e_itr, *h_itr)) {
            return false;
        }
        e_itr++;
        h_itr++;
    }
    if (inst->UseMagicFactory() && !MagicFactoryIsAvailable(beat, inst->MagicFactory())) {
        return false;
    }

    const auto& place = qubits.empty() ? state.GetPlace(inst->EFactoryList().front())
                                       : state.GetPlace(qubits.front());
    auto& plane = state.GetGrid(place.z).GetPlane(place.z);
    const auto& topology = plane.GetTopology();

    if (inst->UseMagicFactory()) {
        const auto magic_factory = inst->MagicFactory();
        const auto& magic_factory_place = state.GetPlace(magic_factory);
        if (place.z != magic_factory_place.z) {
            return false;
        }
        if (plane.GetNode(magic_factory_place.XY()).is_available) {
            return false;
        }
        if (!state.IsAvailable(magic_factory)) {
            return false;
        }
    }
    for (const auto e : inst->EFactoryList()) {
        const auto& e_place = state.GetPlace(e);
        if (place.z != e_place.z) {
            return false;
        }
        if (plane.GetNode(e_place.XY()).is_available) {
            return false;
        }
        if (!state.IsAvailable(e)) {
            return false;
        }
    }
    for (auto b = beat; b < beat + inst->Latency(); ++b) {
        auto& tmp_state = GetStateBuffer().GetQuantumState(b);
        auto& tmp_plane = tmp_state.GetGrid(place.z).GetPlane(place.z);
        for (const auto& [x, y, z] : inst->Path()) {
            if (place.z != z) {
                return false;
            }
            if (topology.OutOfPlane({x, y})) {
                return false;
            }
            if (!tmp_plane.GetNode({x, y}).IsFreeAncilla()) {
                return false;
            }
        }
        for (const auto& q : inst->QubitList()) {
            const auto& place = tmp_state.GetPlace(q);
            if (!tmp_plane.GetNode(place.XY()).is_available) {
                return false;
            }
        }
    }
    return HasOnlyBoundaryPauli(inst->BasisList());
}
void ScLsSimulator::RunLatticeSurgeryMultinode(Beat beat, LatticeSurgeryMultinode* inst) {
    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto& place = inst->QubitList().empty() ? state.GetPlace(inst->EFactoryList().front())
                                                  : state.GetPlace(inst->QubitList().front());
    // Update buffer.
    for (auto b = beat; b < beat + inst->Latency(); ++b) {
        auto& tmp_state = GetStateBuffer().GetQuantumState(b);
        auto& tmp_plane = tmp_state.GetGrid(place.z).GetPlane(place.z);
        for (const auto& [x, y, _] : inst->Path()) {
            tmp_plane.GetNode({x, y}).is_available = false;
        }
        for (const auto& q : inst->QubitList()) {
            const auto& place = tmp_state.GetPlace(q);
            tmp_plane.GetNode(place.XY()).is_available = false;
        }
    }
    // Use magic factory.
    if (inst->UseMagicFactory()) {
        GetStateBuffer().UseMagicState(beat, inst->MagicFactory());
    }
    // Use e factory.
    {
        auto e_itr = inst->EFactoryList().begin();
        auto h_itr = inst->EHandleList().begin();
        while (e_itr != inst->EFactoryList().end() && h_itr != inst->EHandleList().end()) {
            const auto e = *e_itr;
            const auto h = *h_itr;
            GetStateBuffer().UseEntanglement(beat, e, h);
            e_itr++;
            h_itr++;
        }
    }

    // Update avail_.
    for (const auto& coord : inst->Path()) {
        avail_.p[coord] = beat + inst->Latency();
    }
    for (const auto& q : inst->QubitList()) {
        avail_.q[q] = beat + inst->Latency();
    }
    if (inst->UseMagicFactory()) {
        avail_.m[inst->MagicFactory()] = beat + inst->Latency();
    }
    for (const auto e : inst->EFactoryList()) {
        avail_.e[e] = beat + inst->Latency();
    }
    const auto cdest = inst->CDest();
    avail_.c[cdest] = beat + inst->StartCorrecting() + option_.reaction_time;

    // Update metadata.
    inst->MetadataMut() = {beat, place.z};
}
#pragma endregion

#pragma region Move
bool ScLsSimulator::SearchMovePathAndRun(
        Beat beat,
        InstQueue& queue,
        MachineFunction& mf,
        Move* inst
) {
    if (!QubitsAreAvailable(beat, inst->QTarget())) {
        return false;
    }

    if (SymbolsAreOnTheSamePlane(beat, inst->QTarget(), {}, {})) {
        return SearchMovePath2DAndRun(beat, queue, mf, inst);
    } else {
        const auto& state = GetStateBuffer().GetQuantumState(beat);
        const auto q_src = inst->Qubit();
        const auto q_dst = inst->QDest();
        const auto& place_src = state.GetPlace(q_src);
        const auto& place_dst = state.GetPlace(q_dst);

        if (place_src.XY() == place_dst.XY() && std::abs(place_src.z - place_dst.z) == 1) {
            // Replace MOVE with MOVE_TRANS.
            auto* move_trans = ReplaceMoveWithMoveTrans(beat, queue, mf, inst);

            // Run MOVE_TRANS.
            if (!IsMoveTransRunnable(beat, move_trans)) {
                return false;
            }
            RunMoveTrans(beat, move_trans);
            RunInstructionInQueue(queue, move_trans);
            return true;
        } else {
            return SearchMovePath3DAndRun(beat, queue, mf, inst);
        }
    }
}
bool ScLsSimulator::SearchMovePath2DAndRun(
        Beat beat,
        InstQueue& queue,
        MachineFunction&,
        Move* inst
) {
    const auto q_src = inst->Qubit();
    const auto q_dst = inst->QDest();

    const auto tmp_route = SearchRoute2D(
            beat,
            q_src,
            q_dst,
            SearchRoute::LS_ANY_BOUNDARY,
            SearchRoute::LS_ANY_BOUNDARY
    );
    if (!tmp_route) {
        return false;
    }

    // Found route.
    const auto& route = tmp_route.value();
    // Update MOVE.
    auto path = route.logical_path;
    path.pop_front();  // Delete coord of 'q_src'.
    path.pop_back();  // Delete coord of 'q_dst'.
    inst->SetPath(std::move(path));
    // Run MOVE.
    RunMove(beat, inst);
    RunInstructionInQueue(queue, inst);

    return true;
}
MoveTrans*
ScLsSimulator::ReplaceMoveWithMoveTrans(Beat, InstQueue& queue, MachineFunction& mf, Move* inst) {
    const auto q_src = inst->Qubit();
    const auto q_dst = inst->QDest();

    // Replace Move with MOVE_TRANS.
    auto* mbb = FindMBB(mf, inst);
    auto new_inst = MoveTrans::New(q_src, q_dst, inst->Condition());
    auto* ptr = new_inst.get();

    // Update MachineBasicBlock.
    mbb->InsertBefore(inst, std::move(new_inst));
    mbb->Erase(inst);

    // Update InstQueue.
    // Replace MOVE with MOVE_TRANS.
    queue.Replace(inst, {ptr}, ptr->Latency());

    return ptr;
}
bool ScLsSimulator::SearchMovePath3DAndRun(
        Beat beat,
        InstQueue& queue,
        MachineFunction& mf,
        Move* inst
) {
    const auto q_src = inst->Qubit();
    const auto q_dst = inst->QDest();

    // 1: Check if MOVE is realizable.
    // Search 3D route from 'q_src' to the plane of 'q_dst'.
    const auto tmp_route = SearchRoute3D(
            beat,
            q_src,
            q_dst,
            SearchRoute::LS_ANY_BOUNDARY,
            SearchRoute::LS_ANY_BOUNDARY
    );
    if (!tmp_route) {
        return false;
    }

    // Found route.
    const auto& route = tmp_route.value();

    // 2: Run 'route' instructions and MOVE.
    auto* mbb = FindMBB(mf, inst);

    // Insert: trans move + plain move + trans move.
    InsertBeforeAndRunRoute3D(beat, mbb, inst, inst->Condition(), route);

    // Update MOVE.
    const auto moved_q_src = symbol_generator_->LatestQ();
    inst->QubitMut() = moved_q_src;
    auto path = route.logical_path;
    path.pop_front();  // Delete coord of 'moved_q_src'.
    path.pop_back();  // Delete coord of 'q_dst'.
    inst->SetPath(std::move(path));

    // Run MOVE.
    RunMove(beat + 2, inst);
    RunFutureInstructionInQueue(beat + 2, queue, inst);

    return true;
}
bool ScLsSimulator::IsMoveRunnable(Beat beat, Move* inst) {
    const auto& qubits = inst->QTarget();
    if (!QubitsAreAvailable(beat, qubits)) {
        return false;
    }

    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto place =
            state.GetPlace(qubits.front());  // NOTE: NEED COPY BECAUSE 'qubit' will be deallocated.
    auto& plane = state.GetGrid(place.z).GetPlane(place.z);
    const auto& topology = plane.GetTopology();

    for (auto b = beat; b < beat + inst->Latency(); ++b) {
        auto& tmp_state = GetStateBuffer().GetQuantumState(b);
        auto& tmp_plane = tmp_state.GetGrid(place.z).GetPlane(place.z);
        for (const auto& [x, y, z] : inst->Path()) {
            if (place.z != z) {
                return false;
            }
            if (topology.OutOfPlane({x, y})) {
                return false;
            }
            if (!tmp_plane.GetNode({x, y}).IsFreeAncilla()) {
                return false;
            }
        }
        for (const auto& q : inst->QTarget()) {
            const auto& place = tmp_state.GetPlace(q);
            if (!tmp_plane.GetNode(place.XY()).is_available) {
                return false;
            }
        }
    }
    return IsConnectedPathBetween(
            state.GetPlace(inst->Qubit()),
            inst->Path(),
            state.GetPlace(inst->QDest())
    );
}
void ScLsSimulator::RunMove(Beat beat, Move* inst) {
    const auto& qubit = inst->Qubit();
    const auto& qdest = inst->QDest();
    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto place =
            state.GetPlace(qubit);  // NOTE: NEED COPY BECAUSE 'qubit' will be deallocated.
    
    // auto& plane = state.GetGrid(place.z).GetPlane(place.z);
    // const auto& topology = plane.GetTopology();

    // Remove qubit
    RunDeAllocate(beat + inst->Latency(), DeAllocate::New(qubit, {}).get());

    // Update buffer.
    for (auto b = beat; b < beat + inst->Latency(); ++b) {
        auto& tmp_state = GetStateBuffer().GetQuantumState(b);
        auto& tmp_plane = tmp_state.GetGrid(place.z).GetPlane(place.z);
        for (const auto& [x, y, _] : inst->Path()) {
            tmp_plane.GetNode({x, y}).is_available = false;
        }
        for (const auto& q : inst->QTarget()) {
            const auto& place = tmp_state.GetPlace(q);
            tmp_plane.GetNode(place.XY()).is_available = false;
        }
    }

    const auto& src_place = place.XY();
    const auto& dst_place = state.GetPlace(qdest).XY();
    const auto src_dir = state.GetDir(qubit);
    if (inst->Path().empty()) {
        // 'qubit' and 'qdest' is neighbor.
        GetStateBuffer().SetQubitDir(beat + inst->Latency(), qdest, src_dir);
    } else {
        const auto& src_diff = src_place - inst->Path().front().XY();
        const auto& dst_diff = dst_place - inst->Path().back().XY();

        if (src_diff.Abs() != 1 || dst_diff.Abs() != 1) {
            throw std::logic_error("Path of MOVE must be aligned from source to destination.");
        }

        if ((std::abs(src_diff.x) == 1 && std::abs(dst_diff.x) == 1)
            || (std::abs(src_diff.y) == 1 && std::abs(dst_diff.y) == 1)) {
            GetStateBuffer().SetQubitDir(beat + inst->Latency(), qdest, src_dir);
        } else {
            GetStateBuffer().SetQubitDir(beat + inst->Latency(), qdest, 1 - src_dir);
        }
    }

    // Update avail_.
    for (const auto& coord : inst->Path()) {
        avail_.p[coord] = beat + inst->Latency();
    }
    avail_.q[qdest] = beat + inst->Latency();

    // Update metadata.
    inst->MetadataMut() = {beat, place.z};
}
#pragma endregion

#pragma region MoveMagic
bool ScLsSimulator::SearchMoveMagicPathAndRun(
        Beat beat,
        InstQueue& queue,
        MachineFunction& mf,
        MoveMagic* inst
) {
    if (!QubitIsAvailable(beat, inst->QDest())) {
        return false;
    }

    // Find available magic factory in the same plane.
    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto qubit = inst->QDest();
    const auto& place = state.GetPlace(qubit);
    for (const auto& magic_factory : state.GetMagicFactoryList(place.z)) {
        if (state.IsAvailable(magic_factory)) {
            return SearchMoveMagicPath2DAndRun(beat, queue, mf, inst);
        }
    }

    // Find available magic factory in the same grid.
    auto& grid = state.GetGrid(place.z);
    for (auto& plane : grid) {
        const auto z = plane.GetTopology().GetZ();
        for (const auto& magic_factory : state.GetMagicFactoryList(z)) {
            if (state.IsAvailable(magic_factory)) {
                return SearchMoveMagicPath3DAndRun(beat, queue, mf, inst);
            }
        }
    }

    // No available magic factory in this grid.

    return false;
}
bool ScLsSimulator::SearchMoveMagicPath2DAndRun(
        Beat beat,
        InstQueue& queue,
        MachineFunction&,
        MoveMagic* inst
) {
    const auto q_dst = inst->QDest();

    const auto tmp_route = SearchRoute2DM(beat, q_dst, SearchRoute::LS_ANY_BOUNDARY);
    if (!tmp_route) {
        return false;
    }

    // Found route.
    const auto& route = tmp_route.value();
    // Update MOVE_MAGIC.
    inst->SetMagicFactory(route.magic_factory);
    auto path = route.logical_path;
    path.pop_front();  // Delete coord of magic factory.
    path.pop_back();  // Delete coord of 'q_dst'.
    inst->SetPath(std::move(path));
    // Run MOVE_MAGIC.
    RunMoveMagic(beat, inst);
    RunInstructionInQueue(queue, inst);

    return true;
}
bool ScLsSimulator::SearchMoveMagicPath3DAndRun(
        Beat beat,
        InstQueue& queue,
        MachineFunction& mf,
        MoveMagic* inst
) {
    const auto q_dst = inst->QDest();

    // 1: Check if MOVE_MAGIC is realizable.
    // Search 3D route from magic state to z of qubit.
    const auto tmp_route = SearchRoute3DM(beat, q_dst, SearchRoute::LS_ANY_BOUNDARY);
    if (!tmp_route) {
        return false;
    }

    // Found route.
    const auto& route = tmp_route.value();

    // 2: Run 'route' instructions and MOVE_MAGIC.
    auto* mbb = FindMBB(mf, inst);

    // Insert: move magic + trans move.
    InsertBeforeAndRunRoute3DM(beat, mbb, inst, inst->Condition(), route);

    // Replace MOVE_MAGIC with MOVE.
    // => Insert MOVE and erase MOVE_MAGIC.

    // Insert MOVE.
    const auto moved_magic_state = symbol_generator_->LatestQ();
    auto path = route.logical_path;
    path.pop_front();  // Delete coord of 'moved_magic_state'.
    path.pop_back();  // Delete coord of 'q_dst'.
    auto new_move_inst = Move::New(moved_magic_state, inst->QDest(), path, inst->Condition());
    auto* new_move_inst_ptr = new_move_inst.get();
    const auto latency = new_move_inst->Latency();
    RunMove(beat + 2, new_move_inst_ptr);
    mbb->InsertBefore(inst, std::move(new_move_inst));

    // Erase MOVE_MAGIC.
    queue.Replace(inst, {new_move_inst_ptr}, latency);
    RunFutureInstructionInQueue(beat + 2, queue, new_move_inst_ptr);
    mbb->Erase(inst);

    return true;
}
bool ScLsSimulator::IsMoveMagicRunnable(Beat beat, MoveMagic* inst) {
    const auto& qubit = inst->QDest();
    const auto magic_factory = inst->MagicFactory();
    if (!QubitIsAvailable(beat, qubit)) {
        return false;
    }
    if (!MagicFactoryIsAvailable(beat, magic_factory)) {
        return false;
    }

    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto& place = state.GetPlace(qubit);
    const auto& magic_factory_place = state.GetPlace(magic_factory);
    auto& plane = state.GetGrid(place.z).GetPlane(place.z);
    const auto& topology = plane.GetTopology();

    if (place.z != magic_factory_place.z) {
        return false;
    }
    if (plane.GetNode(magic_factory_place.XY()).is_available) {
        return false;
    }
    if (!state.IsAvailable(magic_factory)) {
        return false;
    }
    for (auto b = beat; b < beat + inst->Latency(); ++b) {
        auto& tmp_state = GetStateBuffer().GetQuantumState(b);
        auto& tmp_plane = tmp_state.GetGrid(place.z).GetPlane(place.z);
        for (const auto& [x, y, z] : inst->Path()) {
            if (place.z != z) {
                return false;
            }
            if (topology.OutOfPlane({x, y})) {
                return false;
            }
            if (!tmp_plane.GetNode({x, y}).IsFreeAncilla()) {
                return false;
            }
        }
        const auto& place = tmp_state.GetPlace(qubit);
        if (!tmp_plane.GetNode(place.XY()).is_available) {
            return false;
        }
    }
    return IsConnectedPathBetween(
            state.GetPlace(inst->MagicFactory()),
            inst->Path(),
            state.GetPlace(inst->QDest())
    );
}
void ScLsSimulator::RunMoveMagic(Beat beat, MoveMagic* inst, std::uint32_t moved_state_dir) {
    const auto& qubit = inst->QDest();
    const auto magic_factory = inst->MagicFactory();
    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto& place = state.GetPlace(qubit);
    // const auto& magic_factory_place = state.GetPlace(magic_factory);
    // auto& plane = state.GetGrid(place.z).GetPlane(place.z);
    // const auto& topology = plane.GetTopology();

    // Update buffer.
    for (auto b = beat; b < beat + inst->Latency(); ++b) {
        auto& tmp_state = GetStateBuffer().GetQuantumState(b);
        auto& tmp_plane = tmp_state.GetGrid(place.z).GetPlane(place.z);
        for (const auto& [x, y, _] : inst->Path()) {
            tmp_plane.GetNode({x, y}).is_available = false;
        }
        const auto& place = tmp_state.GetPlace(qubit);
        tmp_plane.GetNode(place.XY()).is_available = false;
    }
    // Use magic factory
    GetStateBuffer().UseMagicState(beat, magic_factory);
    // Set qubit direction
    GetStateBuffer().SetQubitDir(beat, qubit, moved_state_dir);

    // Update avail_.
    for (const auto& coord : inst->Path()) {
        avail_.p[coord] = beat + inst->Latency();
    }
    avail_.q[qubit] = beat + inst->Latency();
    avail_.m[magic_factory] = beat + inst->Latency();

    // Update metadata.
    inst->MetadataMut() = {beat, place.z};
}
#pragma endregion

#pragma region MoveEntanglement
bool ScLsSimulator::SearchMoveEntanglementPathAndRun(
        Beat beat,
        InstQueue& queue,
        MachineFunction& mf,
        MoveEntanglement* inst
) {
    if (!QubitIsAvailable(beat, inst->QDest())) {
        return false;
    }
    if (!EntanglementFactoryIsAvailable(beat, inst->EFactory())) {
        return false;
    }

    // Find available entanglement factory in the same plane.
    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto e_factory = inst->EFactory();
    const auto e_handle = inst->GetEHandle();
    const auto pair_factory = state.GetTopology().GetPair(e_factory);
    const auto& e_state = state.GetState(e_factory);
    const auto can_use_entanglement = e_state.IsReserved(e_handle)
            || (e_state.IsAvailable() && state.GetState(pair_factory).IsAvailable());
    if (can_use_entanglement) {
        return SearchMoveEntanglementPath2DAndRun(beat, queue, mf, inst);
    }

    return false;
}
bool ScLsSimulator::SearchMoveEntanglementPath2DAndRun(
        Beat beat,
        InstQueue& queue,
        MachineFunction&,
        MoveEntanglement* inst
) {
    const auto q_dst = inst->QDest();
    const auto e_factory = inst->EFactory();

    const auto tmp_route = SearchRoute2DE(beat, e_factory, q_dst, SearchRoute::LS_ANY_BOUNDARY);
    if (!tmp_route) {
        return false;
    }

    // Found route.
    const auto& route = tmp_route.value();
    // Update MOVE_ENTANGLEMENT.
    auto path = route.logical_path;
    path.pop_front();  // Delete coord of 'e_factory'.
    path.pop_back();  // Delete coord of 'q_dst'.
    inst->SetPath(std::move(path));
    // Run MOVE_ENTANGLEMENT.
    RunMoveEntanglement(beat, inst);
    RunInstructionInQueue(queue, inst);

    return true;
}
bool ScLsSimulator::IsMoveEntanglementRunnable(Beat beat, MoveEntanglement* inst) {
    const auto& qubit = inst->QDest();
    const auto e_factory = inst->EFactory();
    const auto e_handle = inst->GetEHandle();
    if (!QubitIsAvailable(beat, qubit)) {
        return false;
    }
    if (!EntanglementFactoryIsAvailable(beat, e_factory)) {
        return false;
    }

    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto& place = state.GetPlace(qubit);
    const auto& e_factory_place = state.GetPlace(e_factory);
    auto& plane = state.GetGrid(place.z).GetPlane(place.z);
    const auto& topology = plane.GetTopology();

    if (place.z != e_factory_place.z) {
        return false;
    }
    if (plane.GetNode(e_factory_place.XY()).is_available) {
        return false;
    }
    const auto pair_factory = state.GetTopology().GetPair(e_factory);
    const auto& e_state = state.GetState(e_factory);
    const auto can_use_entanglement = e_state.IsReserved(e_handle)
            || (e_state.IsAvailable() && state.GetState(pair_factory).IsAvailable());
    if (!can_use_entanglement) {
        return false;
    }
    for (auto b = beat; b < beat + inst->Latency(); ++b) {
        auto& tmp_state = GetStateBuffer().GetQuantumState(b);
        auto& tmp_plane = tmp_state.GetGrid(place.z).GetPlane(place.z);
        for (const auto& [x, y, z] : inst->Path()) {
            if (place.z != z) {
                return false;
            }
            if (topology.OutOfPlane({x, y})) {
                return false;
            }
            if (!tmp_plane.GetNode({x, y}).IsFreeAncilla()) {
                return false;
            }
        }
        const auto& place = tmp_state.GetPlace(qubit);
        if (!tmp_plane.GetNode(place.XY()).is_available) {
            return false;
        }
    }

    return true;
}
void ScLsSimulator::RunMoveEntanglement(
        Beat beat,
        MoveEntanglement* inst,
        std::uint32_t moved_state_dir
) {
    const auto& qubit = inst->QDest();
    const auto e_factory = inst->EFactory();
    const auto e_handle = inst->GetEHandle();
    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto& place = state.GetPlace(qubit);

    // Update buffer.
    for (auto b = beat; b < beat + inst->Latency(); ++b) {
        auto& tmp_state = GetStateBuffer().GetQuantumState(b);
        auto& tmp_plane = tmp_state.GetGrid(place.z).GetPlane(place.z);
        for (const auto& [x, y, _] : inst->Path()) {
            tmp_plane.GetNode({x, y}).is_available = false;
        }
        const auto& place = tmp_state.GetPlace(qubit);
        tmp_plane.GetNode(place.XY()).is_available = false;
    }
    // Use entanglement.
    GetStateBuffer().UseEntanglement(beat, e_factory, e_handle);
    // Set qubit direction
    GetStateBuffer().SetQubitDir(beat, qubit, moved_state_dir);

    // Update avail_.
    for (const auto& coord : inst->Path()) {
        avail_.p[coord] = beat + inst->Latency();
    }
    avail_.q[qubit] = beat + inst->Latency();
    avail_.e[e_factory] = beat + inst->Latency();

    // Update metadata.
    inst->MetadataMut() = {beat, place.z};
}
#pragma endregion

#pragma region Cnot
bool ScLsSimulator::SearchCnotPathAndRun(
        Beat beat,
        InstQueue& queue,
        MachineFunction& mf,
        Cnot* inst
) {
    if (!QubitsAreAvailable(beat, inst->QTarget())) {
        return false;
    }

    if (SymbolsAreOnTheSamePlane(beat, inst->QTarget(), {}, {})) {
        // return SearchCnotPath2DAndRun(beat, queue, mf, inst);
        ReplaceCnotWithLS(beat, queue, mf, inst);
        return true;
    } else {
        const auto& state = GetStateBuffer().GetQuantumState(beat);
        const auto q_src = inst->Qubit1();
        const auto q_dst = inst->Qubit2();
        const auto& place_src = state.GetPlace(q_src);
        const auto& place_dst = state.GetPlace(q_dst);

        if (place_src.XY() == place_dst.XY() && std::abs(place_src.z - place_dst.z) == 1) {
            // Replace CNOT with CNOT_TRANS.
            auto* cnot_trans = ReplaceCnotWithCnotTrans(beat, queue, mf, inst);

            // Run CNOT_TRANS.
            if (!IsCnotTransRunnable(beat, cnot_trans)) {
                return false;
            }
            RunCnotTrans(beat, cnot_trans);
            RunInstructionInQueue(queue, cnot_trans);
            return true;
        } else {
            return SearchCnotPath3DAndRun(beat, queue, mf, inst);
        }
    }
}
bool ScLsSimulator::SearchCnotPath2DAndRun(
        Beat beat,
        InstQueue& queue,
        MachineFunction&,
        Cnot* inst
) {
    const auto q_src = inst->Qubit1();  // Z-boundary
    const auto q_dst = inst->Qubit2();  // X-boundary

    // TODO: search two code beats
    // TODO: implement kink constraint
    const auto tmp_route = SearchRoute2D(
            beat,
            q_src,
            q_dst,
            SearchRoute::LS_Z_BOUNDARY,
            SearchRoute::LS_X_BOUNDARY
    );
    if (!tmp_route) {
        return false;
    }

    // Found route.
    const auto& route = tmp_route.value();
    // Update CNOT.
    auto path = route.logical_path;
    path.pop_front();  // Delete coord of 'q_src'.
    path.pop_back();  // Delete coord of 'q_dst'.
    inst->SetPath(std::move(path));
    // Run CNOT.
    RunCnot(beat, inst);
    RunInstructionInQueue(queue, inst);

    return true;
}
void ScLsSimulator::ReplaceCnotWithLS(Beat, InstQueue& queue, MachineFunction& mf, Cnot* inst) {
    // Replace CNOT with LS Z + LS X.
    auto* mbb = FindMBB(mf, inst);
    auto lsz = LatticeSurgery::New(
            inst->QTarget(),
            {Pauli::Z(), Pauli::Z()},
            {},
            symbol_generator_->GenerateC(),
            inst->Condition()
    );
    auto lsx = LatticeSurgery::New(
            inst->QTarget(),
            {Pauli::X(), Pauli::X()},
            {},
            symbol_generator_->GenerateC(),
            inst->Condition()
    );
    auto* ptr_lsz = lsz.get();
    auto* ptr_lsx = lsx.get();

    // Update MachineBasicBlock.
    mbb->InsertAfter(inst, std::move(lsx));
    mbb->InsertAfter(inst, std::move(lsz));
    mbb->Erase(inst);

    // Update InstQueue.
    // Replace CNOT with LS Z + LS X.
    queue.Replace(inst, {ptr_lsz}, ptr_lsz->Latency());
    queue.InsertAfter(ptr_lsz, ptr_lsx);
}
CnotTrans*
ScLsSimulator::ReplaceCnotWithCnotTrans(Beat, InstQueue& queue, MachineFunction& mf, Cnot* inst) {
    const auto q_src = inst->Qubit1();  // Z-boundary
    const auto q_dst = inst->Qubit2();  // X-boundary

    // Replace CNOT with CNOT_TRANS.
    auto* mbb = FindMBB(mf, inst);
    auto new_inst = CnotTrans::New(q_src, q_dst, inst->Condition());
    auto* ptr = new_inst.get();

    // Update MachineBasicBlock.
    mbb->InsertBefore(inst, std::move(new_inst));
    mbb->Erase(inst);

    // Update InstQueue.
    // Replace CNOT with CNOT_TRANS.
    queue.Replace(inst, {ptr}, ptr->Latency());

    return ptr;
}
bool ScLsSimulator::SearchCnotPath3DAndRun(
        Beat beat,
        InstQueue& queue,
        MachineFunction& mf,
        Cnot* inst
) {
    const auto q_src = inst->Qubit1();  // Z-boundary
    const auto q_dst = inst->Qubit2();  // X-boundary

    // 1: Check if CNOT is realizable.
    // Search 3D route from 'q_src' to the plane of 'q_dst'.
    const auto tmp_route = SearchRoute3D(
            beat,
            q_src,
            q_dst,
            SearchRoute::LS_Z_BOUNDARY,
            SearchRoute::LS_X_BOUNDARY
    );
    if (!tmp_route) {
        return false;
    }

    // Found route.
    const auto& route = tmp_route.value();

    // 2: Run 'route' instructions and CNOT.
    auto* mbb = FindMBB(mf, inst);

    // Insert: trans move + plain move + trans move.
    InsertBeforeAndRunRoute3D(beat, mbb, inst, inst->Condition(), route);

    // Update CNOT.
    const auto moved_q_src = symbol_generator_->LatestQ();
    inst->Qubit1Mut() = moved_q_src;
    auto path = route.logical_path;
    path.pop_front();  // Delete coord of 'moved_q_src'.
    path.pop_back();  // Delete coord of 'q_dst'.
    inst->SetPath(std::move(path));

    // Run CNOT.
    RunCnot(beat + 2, inst);
    RunFutureInstructionInQueue(beat + 2, queue, inst);

    // 3: Insert inverse move instructions. (For now, these will be only inserted and not run.)

    // Allocate q_src.
    auto new_allocate_inst = Allocate::New(q_src, route.src, 0, inst->Condition());
    auto* ptr = new_allocate_inst.get();
    queue.InsertAfter(inst, ptr);
    mbb->InsertAfter(inst, std::move(new_allocate_inst));

    // Inverse move from 'move_q_src' to q_src.
    // Restore the original symbol after consuming the transported control qubit.
    auto new_move_inst = Move::New(moved_q_src, q_src, {}, inst->Condition());
    queue.InsertAfter(ptr, new_move_inst.get());
    mbb->InsertAfter(ptr, std::move(new_move_inst));

    return true;
}
bool ScLsSimulator::IsCnotRunnable(Beat beat, Cnot* inst) {
    const auto& qubits = inst->QTarget();
    if (!QubitsAreAvailable(beat, qubits)) {
        return false;
    }

    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto& place = state.GetPlace(qubits.front());
    auto& plane = state.GetGrid(place.z).GetPlane(place.z);
    const auto& topology = plane.GetTopology();

    for (auto b = beat; b < beat + inst->Latency(); ++b) {
        auto& tmp_state = GetStateBuffer().GetQuantumState(b);
        auto& tmp_plane = tmp_state.GetGrid(place.z).GetPlane(place.z);
        for (const auto& [x, y, z] : inst->Path()) {
            if (place.z != z) {
                return false;
            }
            if (topology.OutOfPlane({x, y})) {
                return false;
            }
            if (!tmp_plane.GetNode({x, y}).IsFreeAncilla()) {
                return false;
            }
        }
        for (const auto& q : inst->QTarget()) {
            const auto& place = tmp_state.GetPlace(q);
            if (!tmp_plane.GetNode(place.XY()).is_available) {
                return false;
            }
        }
    }
    return IsConnectedPathBetween(
            state.GetPlace(inst->Qubit1()),
            inst->Path(),
            state.GetPlace(inst->Qubit2())
    );
}
void ScLsSimulator::RunCnot(Beat beat, Cnot* inst) {
    const auto& qubits = inst->QTarget();
    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto& place = state.GetPlace(qubits.front());
    // auto& plane = state.GetGrid(place.z).GetPlane(place.z);
    // const auto& topology = plane.GetTopology();

    // Update buffer.
    for (auto b = beat; b < beat + inst->Latency(); ++b) {
        auto& tmp_state = GetStateBuffer().GetQuantumState(b);
        auto& tmp_plane = tmp_state.GetGrid(place.z).GetPlane(place.z);
        for (const auto& [x, y, _] : inst->Path()) {
            tmp_plane.GetNode({x, y}).is_available = false;
        }
        for (const auto& q : inst->QTarget()) {
            const auto& place = tmp_state.GetPlace(q);
            tmp_plane.GetNode(place.XY()).is_available = false;
        }
    }

    // Update avail_.
    for (const auto& coord : inst->Path()) {
        avail_.p[coord] = beat + inst->Latency();
    }
    for (const auto& q : inst->QTarget()) {
        avail_.q[q] = beat + inst->Latency();
    }
    inst->MetadataMut() = {beat, place.z};
}
#pragma endregion

#pragma region CnotTrans
bool ScLsSimulator::IsCnotTransRunnable(Beat beat, CnotTrans* inst) {
    const auto qubit1 = inst->Qubit1();
    const auto qubit2 = inst->Qubit2();
    if (!QubitIsAvailable(beat, qubit1) || !QubitIsAvailable(beat, qubit2)) {
        return false;
    }

    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto& place1 = state.GetPlace(qubit1);
    const auto& place2 = state.GetPlace(qubit2);
    auto& grid = state.GetGrid(place1.z);
    if (place1.x != place2.x) {
        return false;
    }
    if (place1.y != place2.y) {
        return false;
    }
    if (std::abs(place1.z - place2.z) != 1) {
        return false;
    }
    if (!grid.GetNode(place1).is_available) {
        return false;
    }
    if (!grid.GetNode(place2).is_available) {
        return false;
    }

    return true;
}
void ScLsSimulator::RunCnotTrans(Beat beat, CnotTrans* inst) {
    const auto qubit1 = inst->Qubit1();
    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto& place1 = state.GetPlace(qubit1);
    inst->MetadataMut() = {beat, place1.z};
}
#pragma endregion

#pragma region SwapTrans
bool ScLsSimulator::IsSwapTransRunnable(Beat beat, SwapTrans* inst) {
    const auto qubit1 = inst->Qubit1();
    const auto qubit2 = inst->Qubit2();
    if (!QubitIsAvailable(beat, qubit1) || !QubitIsAvailable(beat, qubit2)) {
        return false;
    }

    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto& place1 = state.GetPlace(qubit1);
    const auto& place2 = state.GetPlace(qubit2);
    auto& grid = state.GetGrid(place1.z);
    if (place1.x != place2.x) {
        return false;
    }
    if (place1.y != place2.y) {
        return false;
    }
    if (std::abs(place1.z - place2.z) != 1) {
        return false;
    }
    if (!grid.GetNode(place1).is_available) {
        return false;
    }
    if (!grid.GetNode(place2).is_available) {
        return false;
    }
    return true;
}
void ScLsSimulator::RunSwapTrans(Beat beat, SwapTrans* inst) {
    const auto qubit1 = inst->Qubit1();
    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto& place1 = state.GetPlace(qubit1);
    inst->MetadataMut() = {beat, place1.z};
}
#pragma endregion

#pragma region MoveTrans
bool ScLsSimulator::IsMoveTransRunnable(Beat beat, MoveTrans* inst) {
    const auto qubit = inst->Qubit();
    const auto qdest = inst->QDest();
    if (!QubitIsAvailable(beat, qubit) || !QubitIsAvailable(beat, qdest)) {
        return false;
    }

    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto place1 =
            state.GetPlace(qubit);  // NOTE: NEED COPY BECAUSE 'qubit' will be deallocated.
    const auto& place2 = state.GetPlace(qdest);
    auto& grid = state.GetGrid(place1.z);
    if (place1.x != place2.x) {
        return false;
    }
    if (place1.y != place2.y) {
        return false;
    }
    if (std::abs(place1.z - place2.z) != 1) {
        return false;
    }
    if (!grid.GetNode(place1).is_available) {
        return false;
    }
    if (!grid.GetNode(place2).is_available) {
        return false;
    }

    return true;
}
void ScLsSimulator::RunMoveTrans(Beat beat, MoveTrans* inst) {
    const auto qubit = inst->Qubit();
    auto& state = GetStateBuffer().GetQuantumState(beat);
    const auto place =
            state.GetPlace(qubit);  // NOTE: NEED COPY BECAUSE 'qubit' will be deallocated.

    // Set direction of dest.
    GetStateBuffer().SetQubitDir(beat, inst->QDest(), state.GetDir(qubit));

    assert(inst->Latency() == 0);
    RunDeAllocate(beat + 1, DeAllocate::New(qubit, {}).get());  // Remove qubit
    inst->MetadataMut() = {beat, place.z};
}
#pragma endregion

#pragma endregion  // simulator
}  // namespace qret::sc_ls_fixed_v0
