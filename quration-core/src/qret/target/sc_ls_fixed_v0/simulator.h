/**
 * @file qret/target/sc_ls_fixed_v0/simulator.h
 * @brief Simulator for SC_LS_FIXED_V0 chip.
 */

#ifndef QRET_TARGET_SC_LS_FIXED_V0_SIMULATOR_H
#define QRET_TARGET_SC_LS_FIXED_V0_SIMULATOR_H

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <limits>
#include <memory>
#include <ostream>
#include <unordered_map>

#include "qret/codegen/machine_function.h"
#include "qret/qret_export.h"
#include "qret/target/sc_ls_fixed_v0/beat.h"
#include "qret/target/sc_ls_fixed_v0/inst_queue.h"
#include "qret/target/sc_ls_fixed_v0/instruction.h"
#include "qret/target/sc_ls_fixed_v0/sc_ls_fixed_v0_target_machine.h"
#include "qret/target/sc_ls_fixed_v0/search_grid.h"
#include "qret/target/sc_ls_fixed_v0/state.h"
#include "qret/target/sc_ls_fixed_v0/symbol.h"
#include "qret/target/sc_ls_fixed_v0/topology.h"

namespace qret::sc_ls_fixed_v0 {
struct QRET_EXPORT MinimumAvailableBeat {
    std::unordered_map<QSymbol, Beat> q;
    std::unordered_map<QSymbol, Beat> q_del;
    std::unordered_map<CSymbol, Beat> c;
    std::unordered_map<MSymbol, Beat> m;
    std::unordered_map<ESymbol, Beat> e;
    std::unordered_map<Coord3D, Beat> p;
};

class QRET_EXPORT RouteSearcher {
public:
    virtual ~RouteSearcher() = default;

    virtual std::optional<SearchRoute::Route2D> SearchRoute2D(
            QuantumStateBuffer& buffer,
            Beat beat,
            QSymbol q_src,
            QSymbol q_dst,
            std::uint32_t boundaries_src,
            std::uint32_t boundaries_dst
    ) = 0;
    virtual std::optional<SearchRoute::Route2D>
    SearchCnotRoute2D(QuantumStateBuffer& buffer, Beat beat, QSymbol q_src, QSymbol q_dst) = 0;
    virtual std::optional<SearchRoute::Route2DM> SearchRoute2DM(
            QuantumStateBuffer& buffer,
            Beat beat,
            QSymbol q_dst,
            std::uint32_t boundaries_dst
    ) = 0;
    virtual std::optional<SearchRoute::Route2DE> SearchRoute2DE(
            QuantumStateBuffer& buffer,
            Beat beat,
            ESymbol e_factory,
            QSymbol q_dst,
            std::uint32_t boundaries_dst
    ) = 0;
    virtual std::optional<SearchRoute::Route3D> SearchRoute3D(
            QuantumStateBuffer& buffer,
            Beat beat,
            QSymbol q_src,
            QSymbol q_dst,
            std::uint32_t boundaries_src,
            std::uint32_t boundaries_dst
    ) = 0;
    virtual std::optional<SearchRoute::Route3D>
    SearchCnotRoute3D(QuantumStateBuffer& buffer, Beat beat, QSymbol q_src, QSymbol q_dst) = 0;
    virtual std::optional<SearchRoute::Route3DM> SearchRoute3DM(
            QuantumStateBuffer& buffer,
            Beat beat,
            QSymbol q_dst,
            std::uint32_t boundaries_dst
    ) = 0;
};

class QRET_EXPORT DefaultRouteSearcher : public RouteSearcher {
public:
    std::optional<SearchRoute::Route2D> SearchRoute2D(
            QuantumStateBuffer& buffer,
            Beat beat,
            QSymbol q_src,
            QSymbol q_dst,
            std::uint32_t boundaries_src,
            std::uint32_t boundaries_dst
    ) override;
    std::optional<SearchRoute::Route2D>
    SearchCnotRoute2D(QuantumStateBuffer& buffer, Beat beat, QSymbol q_src, QSymbol q_dst) override;
    std::optional<SearchRoute::Route2DM> SearchRoute2DM(
            QuantumStateBuffer& buffer,
            Beat beat,
            QSymbol q_dst,
            std::uint32_t boundaries_dst
    ) override;
    std::optional<SearchRoute::Route2DE> SearchRoute2DE(
            QuantumStateBuffer& buffer,
            Beat beat,
            ESymbol e_factory,
            QSymbol q_dst,
            std::uint32_t boundaries_dst
    ) override;
    std::optional<SearchRoute::Route3D> SearchRoute3D(
            QuantumStateBuffer& buffer,
            Beat beat,
            QSymbol q_src,
            QSymbol q_dst,
            std::uint32_t boundaries_src,
            std::uint32_t boundaries_dst
    ) override;
    std::optional<SearchRoute::Route3D>
    SearchCnotRoute3D(QuantumStateBuffer& buffer, Beat beat, QSymbol q_src, QSymbol q_dst) override;
    std::optional<SearchRoute::Route3DM> SearchRoute3DM(
            QuantumStateBuffer& buffer,
            Beat beat,
            QSymbol q_dst,
            std::uint32_t boundaries_dst
    ) override;
};

class QRET_EXPORT ScLsSimulator {
public:
    struct Stats {
        std::uint64_t num_2beat_trans_move = 0;
    };

    ScLsSimulator(
            const Topology& topology,
            const ScLsFixedV0MachineOption& option,
            Beat buffer_size,
            std::shared_ptr<SymbolGenerator> symbol_generator,
            std::unique_ptr<RouteSearcher> route_searcher = {}
    );

    [[nodiscard]] Beat GetBeat() const {
        return buffer_->GetInitialBeat();
    }
    void StepBeat();

    QuantumStateBuffer& GetStateBuffer() {
        return *buffer_;
    }

    bool Run(Beat beat, InstQueue& queue, MachineFunction& mf, ScLsInstructionBase* inst);

    const Stats& GetStats() const {
        return stats_;
    }

private:
    static constexpr auto InfBeat = std::numeric_limits<Beat>::max();

    const ScLsFixedV0MachineOption& option_;

    std::unique_ptr<QuantumStateBuffer> buffer_;
    MinimumAvailableBeat avail_;
    std::shared_ptr<SymbolGenerator> symbol_generator_;
    std::unique_ptr<RouteSearcher> route_searcher_;

    Stats stats_;

#pragma region helper
    bool PlaceIsAvailable(Beat beat, const Coord3D& place) const {
        const auto itr = avail_.p.find(place);
        return itr != avail_.p.end() && itr->second <= beat;
    }
    bool QubitIsAvailable(Beat beat, QSymbol qubit) const {
        const auto itr = avail_.q.find(qubit);
        assert(itr != avail_.q.end() && "qubit is not allocated");
        return itr != avail_.q.end() && itr->second <= beat;
    }
    bool MagicFactoryIsAvailable(Beat beat, MSymbol magic_factory) const {
        const auto itr = avail_.m.find(magic_factory);
        assert(itr != avail_.m.end() && "magic_factory is not allocated");
        return itr != avail_.m.end() && itr->second <= beat;
    }
    bool EntanglementFactoryIsAvailable(Beat beat, ESymbol entanglement_factory) const {
        const auto itr = avail_.e.find(entanglement_factory);
        assert(itr != avail_.e.end() && "entanglement_factory is not allocated");
        return itr != avail_.e.end() && itr->second <= beat;
    }
    bool QubitsAreAvailable(Beat beat, const std::list<QSymbol>& qubits) const {
        return std::all_of(qubits.begin(), qubits.end(), [this, beat](QSymbol qubit) {
            return QubitIsAvailable(beat, qubit);
        });
    }
    bool SymbolsAreOnTheSamePlane(
            Beat beat,
            const std::list<QSymbol>& qs,
            const std::list<MSymbol>& ms,
            const std::list<ESymbol>& es
    ) const;
    void RunInstructionInQueue(InstQueue& queue, ScLsInstructionBase* inst) {
        if (!queue.Run(inst)) {
            throw std::logic_error(
                    fmt::format("Run a not runnable instruction: {}", inst->ToString())
            );
        }
    }
    void RunFutureInstructionInQueue(Beat beat, InstQueue& queue, ScLsInstructionBase* inst) {
        if (!queue.RunFuture(beat, inst)) {
            throw std::logic_error("Try to run a not runnable instruction in future");
        }
    }
    MachineBasicBlock* FindMBB(MachineFunction& mf, MachineInstruction* inst) {
        MachineBasicBlock* mbb = nullptr;
        for (auto&& tmp_mbb : mf) {
            if (tmp_mbb.Contain(inst)) {
                mbb = &tmp_mbb;
                break;
            }
        }
        if (mbb == nullptr) {
            throw std::runtime_error(
                    fmt::format(
                            "MachineFunction does not contain instruction: {}",
                            inst->ToString()
                    )
            );
        }
        return mbb;
    }
#pragma endregion

#pragma region insert and run instructions
    /**
     * @brief Insert and run instructins of MOVE_TRANS from src to dst_z.
     * @details In fact, insert and run two instructions: ALLOCATE and MOVE_TRANS.
     *
     * @param beat beat to run MOVE_TRANS.
     * @param mbb mbb to insert MOVE_TRANS.
     * @param insert_before insert MOVE_TRANS before this instruction.
     * @param condition condition of instruction.
     * @param src_q source qubit.
     * @param src place of source qubit.
     * @param dst_z destination z.
     */
    void InsertBeforeAndRunTransMove(
            Beat beat,
            MachineBasicBlock* mbb,
            ScLsInstructionBase* insert_before,
            const std::list<CSymbol>& condition,
            QSymbol src_q,
            const Coord3D& src,
            std::int32_t dst_z
    );
    /**
     * @brief Insert 3D move instructions to run logical operation.
     * @details
     * 3D move = MOVE_TRANS (at beat) + plane move (at beat) + MOVE_TRANS (at beat + 1)
     *
     * After performing a 3D move, the qubit will be in a position to run a logical operation at
     beat + 2.
     *
     * route.src --(MOVE_TRANS)--> route.move_path.front() --(plane move)--> route.move_path.back()
     * --(MOVE_TRANS)--> route.logical_path.front()
     *
     * @param beat beat to start 3D moving.
     * @param mbb mbb to insert instructions.
     * @param insert_before insert instructions before this instruction.
     * @param condition condition of instruction.
     * @param route 3D route.
     */
    void InsertBeforeAndRunRoute3D(
            Beat beat,
            MachineBasicBlock* mbb,
            ScLsInstructionBase* insert_before,
            const std::list<CSymbol>& condition,
            const SearchRoute::Route3D& route
    );
    void InsertBeforeAndRunRoute3DM(
            Beat beat,
            MachineBasicBlock* mbb,
            ScLsInstructionBase* insert_before,
            const std::list<CSymbol>& condition,
            const SearchRoute::Route3DM& route
    );
#pragma endregion

#pragma region Allocate
    [[nodiscard]] bool IsAllocateRunnable(Beat, Allocate*);
    void RunAllocate(Beat, Allocate*);
#pragma endregion

#pragma region AllocateMagicFactory
    [[nodiscard]] bool IsAllocateMagicFactoryRunnable(Beat, AllocateMagicFactory*);
    void RunAllocateMagicFactory(Beat, AllocateMagicFactory*);
#pragma endregion

#pragma region AllocateEntanglementFactory
    [[nodiscard]] bool IsAllocateEntanglementFactoryRunnable(Beat, AllocateEntanglementFactory*);
    void RunAllocateEntanglementFactory(Beat, AllocateEntanglementFactory*);
#pragma endregion

#pragma region DeAllocate
    [[nodiscard]] bool IsDeAllocateRunnable(Beat, DeAllocate*);
    void RunDeAllocate(Beat, DeAllocate*);
#pragma endregion

#pragma region InitZX
    [[nodiscard]] bool IsInitZXRunnable(Beat, InitZX*);
    void RunInitZX(Beat, InitZX*);
#pragma endregion

#pragma region MeasZX
    [[nodiscard]] bool IsMeasZXRunnable(Beat, MeasZX*);
    void RunMeasZX(Beat, MeasZX*);
#pragma endregion

#pragma region MeasY
    [[nodiscard]] bool SearchMeasYDir(Beat, MeasY*);
    [[nodiscard]] bool IsMeasYRunnable(Beat, MeasY*);
    void RunMeasY(Beat, MeasY*);
#pragma endregion

#pragma region Twist
    [[nodiscard]] bool SearchTwistDir(Beat, Twist*);
    [[nodiscard]] bool IsTwistRunnable(Beat, Twist*);
    void RunTwist(Beat, Twist*);
#pragma endregion

#pragma region Hadamard
    [[nodiscard]] bool IsHadamardRunnable(Beat, Hadamard*);
    void RunHadamard(Beat, Hadamard*);
#pragma endregion

#pragma region Rotate
    [[nodiscard]] bool SearchRotateDir(Beat, Rotate*);
    [[nodiscard]] bool IsRotateRunnable(Beat, Rotate*);
    void RunRotate(Beat, Rotate*);
#pragma endregion

#pragma region search helpers
    /**
     * @brief Search 2D route from q_src to q_dst.
     *
     * @param beat beat to search 2D route.
     * @param q_src source qubit.
     * @param q_dst destination qubit.
     * @param boundaries_src
     * @param boundaries_dst
     * @return std::optional<SearchRoute::Route2D>
     */
    std::optional<SearchRoute::Route2D> SearchRoute2D(
            Beat beat,
            QSymbol q_src,
            QSymbol q_dst,
            std::uint32_t boundaries_src,
            std::uint32_t boundaries_dst
    );
    std::optional<SearchRoute::Route2D> SearchCnotRoute2D(Beat beat, QSymbol q_src, QSymbol q_dst);
    /**
     * @brief Search 2D route from magic factory to q_dst.
     *
     * @param beat beat to search 2D route.
     * @param q_dst destination qubit.
     * @param boundaries_dst
     * @return std::optional<SearchRoute::Route2DM>
     */
    std::optional<SearchRoute::Route2DM>
    SearchRoute2DM(Beat beat, QSymbol q_dst, std::uint32_t boundaries_dst);
    /**
     * @brief Search 2D route from e_factory to q_dst.
     *
     * @param beat beat to search 2D route.
     * @param e_factory entanglement factory
     * @param q_dst destination qubit.
     * @param boundaries_dst
     * @return std::optional<SearchRoute::Route2DE>
     */
    std::optional<SearchRoute::Route2DE>
    SearchRoute2DE(Beat beat, ESymbol e_factory, QSymbol q_dst, std::uint32_t boundaries_dst);
    /**
     * @brief Search 3D route from q_src to q_dst.
     *
     * @param beat search 3D route in beat ~ beat+2.
     * @param q_src source qubit.
     * @param q_dst destination qubit.
     * @param boundaries_src
     * @param boundaries_dst
     * @return std::optional<SearchRoute::Route3D>
     */
    std::optional<SearchRoute::Route3D> SearchRoute3D(
            Beat beat,
            QSymbol q_src,
            QSymbol q_dst,
            std::uint32_t boundaries_src,
            std::uint32_t boundaries_dst
    );
    std::optional<SearchRoute::Route3D> SearchCnotRoute3D(Beat beat, QSymbol q_src, QSymbol q_dst);
    /**
     * @brief Search 3D route from magic factory to q_dst.
     *
     * @param beat search 3D route in beat ~ beat+2.
     * @param q_dst destination qubit.
     * @param boundaries_dst
     * @return std::optional<SearchRoute::Route3DM>
     */
    std::optional<SearchRoute::Route3DM>
    SearchRoute3DM(Beat beat, QSymbol q_dst, std::uint32_t boundaries_dst);
#pragma endregion

#pragma region LatticeSurgery
    [[nodiscard]] bool
    SearchLatticeSurgeryPathAndRun(Beat, InstQueue&, MachineFunction&, LatticeSurgery*);
    [[nodiscard]] bool
    SearchLatticeSurgeryPath2DBFSAndRun(Beat, InstQueue&, MachineFunction&, LatticeSurgery*);
    [[nodiscard]] bool
    SearchLatticeSurgeryPath2DSteinerAndRun(Beat, InstQueue&, MachineFunction&, LatticeSurgery*);
    [[nodiscard]] bool
    SearchLatticeSurgeryPath3DAndRun(Beat, InstQueue&, MachineFunction&, LatticeSurgery*);
    [[nodiscard]] bool IsLatticeSurgeryRunnable(Beat, LatticeSurgery*);
    void RunLatticeSurgery(Beat, LatticeSurgery*);
#pragma endregion

#pragma region LatticeSurgeryMagic
    [[nodiscard]] bool
    SearchLatticeSurgeryMagicPathAndRun(Beat, InstQueue&, MachineFunction&, LatticeSurgeryMagic*);
    [[nodiscard]] bool SearchLatticeSurgeryMagicPath2DBFSAndRun(
            Beat,
            InstQueue&,
            MachineFunction&,
            LatticeSurgeryMagic*
    );
    [[nodiscard]] bool SearchLatticeSurgeryMagicPath2DSteinerAndRun(
            Beat,
            InstQueue&,
            MachineFunction&,
            LatticeSurgeryMagic*
    );
    [[nodiscard]] bool
    SearchLatticeSurgeryMagicPath3DAndRun(Beat, InstQueue&, MachineFunction&, LatticeSurgeryMagic*);
    [[nodiscard]] bool IsLatticeSurgeryMagicRunnable(Beat, LatticeSurgeryMagic*);
    void RunLatticeSurgeryMagic(Beat, LatticeSurgeryMagic*);
#pragma endregion

#pragma region LatticeSurgeryMultinode
    [[nodiscard]] bool SearchLatticeSurgeryMultinodePathAndRun(
            Beat,
            InstQueue&,
            MachineFunction&,
            LatticeSurgeryMultinode*
    );
    [[nodiscard]] bool IsLatticeSurgeryMultinodeRunnable(Beat, LatticeSurgeryMultinode*);
    void RunLatticeSurgeryMultinode(Beat, LatticeSurgeryMultinode*);
#pragma endregion

#pragma region Move
    [[nodiscard]] bool SearchMovePathAndRun(Beat, InstQueue&, MachineFunction&, Move*);
    [[nodiscard]] bool SearchMovePath2DAndRun(Beat, InstQueue&, MachineFunction&, Move*);
    MoveTrans* ReplaceMoveWithMoveTrans(Beat, InstQueue&, MachineFunction&, Move*);
    [[nodiscard]] bool SearchMovePath3DAndRun(Beat, InstQueue&, MachineFunction&, Move*);
    [[nodiscard]] bool IsMoveRunnable(Beat, Move*);
    void RunMove(Beat, Move*);
#pragma endregion

#pragma region MoveMagic
    [[nodiscard]] bool SearchMoveMagicPathAndRun(Beat, InstQueue&, MachineFunction&, MoveMagic*);
    [[nodiscard]] bool SearchMoveMagicPath2DAndRun(Beat, InstQueue&, MachineFunction&, MoveMagic*);
    [[nodiscard]] bool SearchMoveMagicPath3DAndRun(Beat, InstQueue&, MachineFunction&, MoveMagic*);
    [[nodiscard]] bool IsMoveMagicRunnable(Beat, MoveMagic*);
    void RunMoveMagic(Beat, MoveMagic*, std::uint32_t moved_state_dir = 0);
#pragma endregion

#pragma region MoveEntanglement
    [[nodiscard]] bool
    SearchMoveEntanglementPathAndRun(Beat, InstQueue&, MachineFunction&, MoveEntanglement*);
    [[nodiscard]] bool
    SearchMoveEntanglementPath2DAndRun(Beat, InstQueue&, MachineFunction&, MoveEntanglement*);
    [[nodiscard]] bool IsMoveEntanglementRunnable(Beat, MoveEntanglement*);
    void RunMoveEntanglement(Beat, MoveEntanglement*, std::uint32_t moved_state_dir = 0);
#pragma endregion

#pragma region Cnot
    [[nodiscard]] bool SearchCnotPathAndRun(Beat, InstQueue&, MachineFunction&, Cnot*);
    [[nodiscard]] bool SearchCnotPath2DAndRun(Beat, InstQueue&, MachineFunction&, Cnot*);
    CnotTrans* ReplaceCnotWithCnotTrans(Beat, InstQueue&, MachineFunction&, Cnot*);
    [[nodiscard]] bool SearchCnotPath3DAndRun(Beat, InstQueue&, MachineFunction&, Cnot*);
    [[nodiscard]] bool IsCnotRunnable(Beat, Cnot*);
    void RunCnot(Beat, Cnot*);
#pragma endregion

#pragma region CnotTrans
    [[nodiscard]] bool IsCnotTransRunnable(Beat, CnotTrans*);
    void RunCnotTrans(Beat, CnotTrans*);
#pragma endregion

#pragma region SwapTrans
    [[nodiscard]] bool IsSwapTransRunnable(Beat, SwapTrans*);
    void RunSwapTrans(Beat, SwapTrans*);
#pragma endregion

#pragma region MoveTrans
    [[nodiscard]] bool IsMoveTransRunnable(Beat, MoveTrans*);
    void RunMoveTrans(Beat, MoveTrans*);
#pragma endregion
};

inline std::ostream& operator<<(std::ostream& out, const ScLsSimulator::Stats& stats) {
    return out << fmt::format("num_2beat_trans_move: {}", stats.num_2beat_trans_move);
}
}  // namespace qret::sc_ls_fixed_v0

template <>
struct fmt::formatter<::qret::sc_ls_fixed_v0::ScLsSimulator::Stats> : ostream_formatter {};

#endif  // QRET_TARGET_SC_LS_FIXED_V0_SIMULATOR_H
