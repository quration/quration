/**
 * @file qret/target/sc_ls_fixed_v0/state.h
 * @brief Quantum state.
 */

#ifndef QRET_TARGET_SC_LS_FIXED_V0_STATE_H
#define QRET_TARGET_SC_LS_FIXED_V0_STATE_H

#include <fmt/format.h>
#include <pcg_random.hpp>

#include <cassert>
#include <limits>
#include <memory>
#include <ostream>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

#include "qret/qret_export.h"
#include "qret/target/sc_ls_fixed_v0/beat.h"
#include "qret/target/sc_ls_fixed_v0/sc_ls_fixed_v0_target_machine.h"
#include "qret/target/sc_ls_fixed_v0/symbol.h"
#include "qret/target/sc_ls_fixed_v0/topology.h"

namespace qret::sc_ls_fixed_v0 {
#pragma region magic state factory

/**
 * @brief Minimal state for the magic state factory.
 * @details
 * Invariants:
 * - 1 <= generation_step <= option.magic_generation_period
 * - magic_state_count <= option.maximum_magic_state_stock
 */
struct QRET_EXPORT MagicFactoryState {
    static MagicFactoryState Empty(std::uint64_t seed) {
        return MagicFactoryState{.engine = pcg64(seed)};
    }

    pcg64 engine;
    std::uint64_t generation_step = 0;
    std::uint64_t magic_state_count = 0;

    [[nodiscard]] bool IsAvailable() const {
        return magic_state_count > 0;
    }

    /**
     * @brief Consume one magic state.
     */
    void UseMagic() {
        if (magic_state_count == 0) {
            throw std::runtime_error(
                    "SC_LS_FIXED_V0 error: Cannot use magic state from empty magic state factory."
            );
        }
        magic_state_count--;
    }
    [[nodiscard]] bool TryUseMagic() {
        if (magic_state_count == 0) {
            return false;
        }
        magic_state_count--;
        return true;
    }
};

/**
 * @brief Advance a cultivation-based factory by one cloc. May create at most one magic state.
 * @details  A creation trial occurs when (generation_step == magic_generation_period) at the
 * beginning of this call. After the trial, generation_step is reset to 0, then incremented to 1
 * within the same call.
 * If @p option.maximum_magic_state_stock is already reached, no creation occurs.
 *
 * @return true if a magic state was created in this clock.
 */
inline bool StepProbabilisticMagicFactoryState(
        const ScLsFixedV0MachineOption& option,
        MagicFactoryState& state
) {
    auto magic_created = false;
    if ((state.generation_step == option.magic_generation_period)
        && (state.magic_state_count < option.maximum_magic_state_stock)) {
        // Get chance to create a magic state.
        auto dist = std::bernoulli_distribution(option.prob_magic_state_creation);
        if (dist(state.engine)) {
            magic_created = true;
            state.magic_state_count++;
        }
        state.generation_step = 0;
    }
    if (state.generation_step < option.magic_generation_period) {
        state.generation_step += 1;
    }
    return magic_created;
}

/**
 * @brief Advance a distillation-based factory by one clock (deterministic).
 * @return true if a magic state was created in this clock.
 */
inline bool StepDeterministicMagicFactoryState(
        const ScLsFixedV0MachineOption& option,
        MagicFactoryState& state
) {
    auto magic_created = false;
    if ((state.generation_step == option.magic_generation_period)
        && (state.magic_state_count < option.maximum_magic_state_stock)) {
        magic_created = true;
        state.magic_state_count++;
        state.generation_step = 0;
    }
    if (state.generation_step < option.magic_generation_period) {
        state.generation_step += 1;
    }
    return magic_created;
}

/**
 * @brief Advance the magic state factory by one clock according to @p option.
 * @return true if a magic state was created in this clock.
 */
inline bool
StepMagicFactoryState(const ScLsFixedV0MachineOption& option, MagicFactoryState& state) {
    return option.use_magic_state_cultivation ? StepProbabilisticMagicFactoryState(option, state)
                                              : StepDeterministicMagicFactoryState(option, state);
}

inline std::ostream& operator<<(std::ostream& out, const MagicFactoryState& state) {
    return out << "generation_step: " << state.generation_step
               << ", magic_state_count: " << state.magic_state_count;
}

#pragma endregion

#pragma region entanglement factory state
struct QRET_EXPORT EntanglementFactoryState {
    static EntanglementFactoryState Empty() {
        return {0, 0, {}};
    }

    std::uint64_t entangled_state_generation_elapsed;
    std::uint64_t entangled_state_stock_count_free;
    std::unordered_set<EHandle> entangled_state_handle_list;

    bool IsAvailable() const {
        return entangled_state_stock_count_free > 0;
    }
    std::uint64_t entangled_state_stock_count_reserve() const {  // NOLINT
        return entangled_state_handle_list.size();
    }
    bool IsReserved(EHandle handle) const {
        return entangled_state_handle_list.contains(handle);
    }
    void AddHandle(EHandle handle) {
        if (entangled_state_stock_count_free == 0) {
            throw std::runtime_error("empty entanglement factory");
        }
        entangled_state_stock_count_free--;
        entangled_state_handle_list.insert(handle);
    }
    void UseHandle(EHandle handle) {
        const auto itr = entangled_state_handle_list.find(handle);
        if (itr == entangled_state_handle_list.end()) {
            throw std::runtime_error(fmt::format("handle: {} not found", handle));
        }
        entangled_state_handle_list.erase(itr);
    }
};
inline bool StepEntanglementFactoryState(
        const ScLsFixedV0MachineOption& option,
        EntanglementFactoryState& state
) {
    auto entanglement_created = false;
    if ((state.entangled_state_generation_elapsed == option.entanglement_generation_period)
        && (state.entangled_state_stock_count_free + state.entangled_state_stock_count_reserve()
            < option.maximum_entangled_state_stock)) {
        entanglement_created = true;
        state.entangled_state_stock_count_free += 1;
        state.entangled_state_generation_elapsed = 0;
    }
    if (state.entangled_state_generation_elapsed < option.entanglement_generation_period) {
        state.entangled_state_generation_elapsed += 1;
    }
    return entanglement_created;
}
#pragma endregion

// Forward declarations
class QuantumPlane;
class QuantumGrid;
class QuantumState;
class QuantumStateBuffer;


namespace QuantumStateBufferInternal{
    enum class Type : std::uint8_t {
        Ancilla = 0,
        MagicFactory = 1,
        EntanglementFactory = 2,
        Qubit = 3,
    };
    struct QRET_EXPORT Node {
        // NOTE: member order is optimized to minimize the byte size of this class.
        bool is_available = true;
        Type type = Type::Ancilla;
        std::uint32_t reserved = 0;  //!< reserved for future use.
        std::uint64_t symbol =
                std::numeric_limits<std::uint64_t>::max();  // If 'type' is Qubit, MagicFactory, or
                                                            // EntanglementFactory, holds the symbol
                                                            // ID. If 'type' is Ancilla, temporarily
                                                            // stores the search cost.

        char ToChar() const;
        bool IsFreeAncilla() const {
            return is_available && type == Type::Ancilla;
        }
        std::uint64_t Cost() const {
            assert(type == Type::Ancilla);
            return symbol;
        }
        void SetCost(std::uint64_t cost) {
            assert(type == Type::Ancilla);
            symbol = cost;
        }
    };
};

class QRET_EXPORT QuantumPlane {
public:
    using Node = QuantumStateBufferInternal::Node;

    QuantumPlane(const QuantumPlane&) = delete;
    QuantumPlane(QuantumPlane&&) = default;

    const ScLsPlane& GetTopology() const {
        return topology_;
    }

    QuantumGrid& GetParent() {
        return *parent_;
    }

    Node& GetNode(const Coord2D& c) {
        const auto index = c.x + (c.y * topology_.GetMaxX());
        return nodes_[static_cast<std::size_t>(index)];
    }

    auto begin() {
        return nodes_.begin();
    }
    auto end() {
        return nodes_.end();
    }

private:
    friend class QuantumGrid;
    QuantumPlane(const ScLsPlane& topology, std::span<Node> nodes)
        : topology_(topology)
        , nodes_(nodes) {}
    void SetParent(QuantumGrid* parent) {
        parent_ = parent;
    }

    const ScLsPlane& topology_;
    QuantumGrid* parent_ = nullptr;
    std::span<Node> nodes_;
};

class QRET_EXPORT QuantumGrid {
public:
    using Node = QuantumStateBufferInternal::Node;

    QuantumGrid(const QuantumGrid&) = delete;
    QuantumGrid(QuantumGrid&&) noexcept;

    const ScLsGrid& GetTopology() const {
        return topology_;
    }

    QuantumState& GetParent() {
        return *parent_;
    }

    QuantumPlane& GetPlane(std::int32_t z);
    QuantumPlane& GetPlaneByIndex(std::size_t plane_index);

    Node& GetNode(const Coord3D& coord) {
        return nodes_[GetNodeIndex(coord)];
    }

    std::size_t GetNodeIndex(const Coord3D& coord) {
        const auto& plane = topology_.GetPlane(coord.z);
        return (plane.GetIndex() * static_cast<std::size_t>(plane.GetMaxX() * plane.GetMaxY()))
                + static_cast<std::size_t>((coord.y * plane.GetMaxX()) + coord.x);
    }

    auto begin() {
        return planes_.begin();
    }
    auto end() {
        return planes_.end();
    }

private:
    friend class QuantumState;
    QuantumGrid(const ScLsGrid& topology, std::vector<Node>& nodes);
    void SetParent(QuantumState* parent) {
        parent_ = parent;
    }

    const ScLsGrid& topology_;
    QuantumState* parent_ = nullptr;
    std::vector<Node>& nodes_;

    // Children
    std::vector<QuantumPlane> planes_;
};

class QRET_EXPORT QuantumState {
public:
    using Node = QuantumStateBufferInternal::Node;

    QuantumState(const QuantumState&) = delete;
    QuantumState(QuantumState&&) noexcept;

    const Topology& GetTopology() const {
        return topology_;
    }

    QuantumStateBuffer& GetParent() {
        return *parent_;
    }

    Beat GetBeat() const {
        return beat_;
    }

    QuantumGrid& GetGrid(std::int32_t z);
    QuantumGrid& GetGridByIndex(std::size_t grid_index);

    const std::vector<MSymbol>& GetMagicFactoryList(std::int32_t z) const;
    bool IsAvailable(MSymbol m) const;
    const Coord3D& GetPlace(MSymbol m) const;
    const MagicFactoryState& GetState(MSymbol m) const;

    const std::vector<ESymbol>& GetEntanglementFactoryList(std::int32_t z) const;
    bool IsAvailable(ESymbol e) const;
    bool IsReserved(EHandle handle) const;
    std::optional<ESymbol> LookupSymbol(EHandle handle) const;
    const Coord3D& GetPlace(ESymbol e) const;
    const EntanglementFactoryState& GetState(ESymbol e) const;

    const Coord3D& GetPlace(QSymbol q) const;
    std::uint32_t GetDir(QSymbol q) const;

    std::size_t GetNodeIndex(const Coord3D& coord) const {
        const auto& plane = topology_.GetPlane(coord.z);
        return (plane.GetIndex() * static_cast<std::size_t>(plane.GetMaxX() * plane.GetMaxY()))
                + static_cast<std::size_t>((coord.y * plane.GetMaxX()) + coord.x);
    }

    void Dump(std::ostream& out) const;

    auto begin() {
        return grids_.begin();
    }
    auto end() {
        return grids_.end();
    }

private:
    friend class QuantumStateBuffer;
    QuantumState(
            const Topology& topology,
            Beat beat,
            std::vector<std::vector<Node>>& nodes,
            std::unordered_map<MSymbol, MagicFactoryState>& mf_states,
            std::unordered_map<ESymbol, EntanglementFactoryState>& ef_states,
            std::unordered_map<QSymbol, std::pair<Coord3D, std::uint32_t>>& q2p
    );
    void SetParent(QuantumStateBuffer* parent) {
        parent_ = parent;
    }
    void SetBeat(Beat beat) {
        beat_ = beat;
    }

    const Topology& topology_;
    QuantumStateBuffer* parent_ = nullptr;
    Beat beat_;
    std::vector<std::vector<Node>>& nodes_;
    std::unordered_map<MSymbol, MagicFactoryState>& mf_states_;
    std::unordered_map<ESymbol, EntanglementFactoryState>& ef_states_;
    std::unordered_map<QSymbol, std::pair<Coord3D, std::uint32_t>>& q2p_;

    // Children
    std::vector<QuantumGrid> grids_;
};

class QRET_EXPORT QuantumStateBuffer {
public:
    using Node = QuantumStateBufferInternal::Node;

    static std::unique_ptr<QuantumStateBuffer> New(
            const Topology& topology,
            const ScLsFixedV0MachineOption& option,
            Beat size,
            Beat begin = 0
    ) {
        return std::unique_ptr<QuantumStateBuffer>(
                new QuantumStateBuffer(topology, option, size, begin)
        );
    }

    QuantumStateBuffer(const QuantumStateBuffer&) = delete;
    QuantumStateBuffer(QuantumStateBuffer&&) = delete;

    Beat GetInitialBeat() const {
        return begin_.first;
    }
    Beat GetFinalBeat() const {
        return GetInitialBeat() + size_ - 1;
    }
    Beat Size() const {
        return size_;
    }

    void SetBanned(Beat beat, const Coord3D& place);
    void PutQubit(Beat beat, QSymbol qubit, const Coord3D& place, std::uint32_t dir);
    void PutMagicFactory(
            Beat beat,
            MSymbol magic_factory,
            const Coord3D& place,
            const MagicFactoryState& mf_state
    );
    void PutEntanglementFactory(
            Beat beat,
            ESymbol entanglement_factory1,
            const Coord3D& place1,
            ESymbol entanglement_factory2,
            const Coord3D& place2,
            const EntanglementFactoryState& ef_state1 = EntanglementFactoryState::Empty(),
            const EntanglementFactoryState& ef_state2 = EntanglementFactoryState::Empty()
    );
    void RemoveQubit(Beat beat, QSymbol qubit);

    // Update states
    void RotateQubit(Beat beat, QSymbol qubit);
    void SetQubitDir(Beat beat, QSymbol qubit, std::uint32_t dir);
    void UseMagicState(Beat beat, MSymbol magic_factory);
    void UseEntanglement(Beat beat, ESymbol entanglement_factory, EHandle handle);

    QuantumState& GetQuantumState(Beat beat);

    void StepBeat();

private:
    friend class QuantumState;
    QuantumStateBuffer(
            const Topology& topology,
            const ScLsFixedV0MachineOption& option,
            Beat size,
            Beat begin
    );

    std::size_t GetBeatIndex(Beat beat) const {
        const auto [begin, index] = begin_;
        const auto new_index = (index + (beat - begin)) % size_;
        return new_index;
    }
    std::size_t GetNodeIndex(const Coord3D& coord) const {
        const auto& grid = topology_.GetGrid(coord.z);
        const auto& plane = grid.GetPlane(coord.z);
        return (plane.GetIndex() * static_cast<std::size_t>(plane.GetMaxX() * plane.GetMaxY()))
                + static_cast<std::size_t>((coord.y * plane.GetMaxX()) + coord.x);
    }

    const Topology& topology_;
    const ScLsFixedV0MachineOption& option_;
    const Beat size_;

    std::pair<Beat, std::size_t> begin_;  //!< (initial beat, index of initial beat)

    // Information that changes over beat.
    std::vector<std::vector<std::vector<Node>>> nodes_;
    std::vector<std::unordered_map<MSymbol, MagicFactoryState>> mf_states_;
    std::vector<std::unordered_map<ESymbol, EntanglementFactoryState>> ef_states_;
    std::vector<std::unordered_map<QSymbol, std::pair<Coord3D, std::uint32_t>>>
            q2p_;  //!< pair of (place, dir)

    // Information that does not change over beat.
    std::unordered_map<MSymbol, Coord3D> m2p_;
    std::unordered_map<std::int32_t, std::vector<MSymbol>> z2m_;
    std::unordered_map<ESymbol, Coord3D> e2p_;
    std::unordered_map<std::int32_t, std::vector<ESymbol>> z2e_;
    std::unordered_map<ESymbol, ESymbol> e_pair_;

    // Children
    std::vector<QuantumState> states_;
};


}  // namespace qret::sc_ls_fixed_v0

#endif  // QRET_TARGET_SC_LS_FIXED_V0_STATE_H
