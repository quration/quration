/**
 * @file qret/target/sc_ls_fixed_v0/state.cpp
 * @brief Quantum state.
 */

#include "qret/target/sc_ls_fixed_v0/state.h"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <stdexcept>

#include "qret/target/sc_ls_fixed_v0/symbol.h"

namespace qret::sc_ls_fixed_v0 {
namespace {
template <typename SymbolT>
void InsertSortedUniqueSymbol(std::vector<SymbolT>& symbols, SymbolT symbol) {
    if (std::find(symbols.begin(), symbols.end(), symbol) != symbols.end()) {
        return;
    }
    symbols.emplace_back(symbol);
    std::sort(symbols.begin(), symbols.end(), [](SymbolT lhs, SymbolT rhs) {
        return lhs.Id() < rhs.Id();
    });
}

void StepEntanglementFactoryPairState(
        const ScLsFixedV0MachineOption& option,
        EntanglementFactoryState& lhs,
        EntanglementFactoryState& rhs
) {
    const auto lhs_stock =
            lhs.entangled_state_stock_count_free + lhs.entangled_state_stock_count_reserve();
    const auto rhs_stock =
            rhs.entangled_state_stock_count_free + rhs.entangled_state_stock_count_reserve();
    if ((lhs.entangled_state_generation_elapsed == option.entanglement_generation_period)
        && (rhs.entangled_state_generation_elapsed == option.entanglement_generation_period)
        && (lhs_stock < option.maximum_entangled_state_stock)
        && (rhs_stock < option.maximum_entangled_state_stock)) {
        lhs.entangled_state_stock_count_free += 1;
        rhs.entangled_state_stock_count_free += 1;
        lhs.entangled_state_generation_elapsed = 0;
        rhs.entangled_state_generation_elapsed = 0;
    }
    if (lhs.entangled_state_generation_elapsed < option.entanglement_generation_period) {
        lhs.entangled_state_generation_elapsed += 1;
    }
    if (rhs.entangled_state_generation_elapsed < option.entanglement_generation_period) {
        rhs.entangled_state_generation_elapsed += 1;
    }
}
}  // namespace

#pragma region QuantumStateBuffer
char QuantumStateBuffer::Node::ToChar() const {
    switch (type) {
        case Type::Ancilla: {
            return is_available ? '.' : 'x';
        }
        case Type::Qubit: {
            return is_available ? 'Q' : 'q';
        }
        case Type::MagicFactory: {
            return is_available ? 'M' : 'm';
        }
        case Type::EntanglementFactory: {
            return is_available ? 'E' : 'e';
        }
        default:
            break;
    }
    return '#';
}
QuantumStateBuffer::QuantumStateBuffer(
        const Topology& topology,
        const ScLsFixedV0MachineOption& option,
        Beat size,
        Beat begin
)
    : topology_(topology)
    , option_(option)
    , size_(size)
    , begin_(begin, 0) {
    // Initialize buffer
    ef_states_.resize(size);
    mf_states_.resize(size);
    q2p_.resize(size);

    auto nodes = std::vector<std::vector<Node>>();
    for (const auto& grid : topology) {
        const auto num_nodes = static_cast<std::size_t>(grid.GetMaxX())
                * static_cast<std::size_t>(grid.GetMaxY())
                * static_cast<std::size_t>(grid.GetZSize());
        nodes.emplace_back(static_cast<std::size_t>(num_nodes), Node{});
    }
    nodes_.resize(size, nodes);

    for (const auto& grid : topology) {
        for (const auto& plane : grid) {
            z2m_[plane.GetZ()] = {};
            z2e_[plane.GetZ()] = {};
        }
    }

    // Initialize children
    states_.reserve(size);
    for (auto i = Beat{0}; i < size; ++i) {
        states_.emplace_back(
                QuantumState(topology_, begin + i, nodes_[i], mf_states_[i], ef_states_[i], q2p_[i])
        );
        states_.back().SetParent(this);
    }
}
void QuantumStateBuffer::SetBanned(Beat beat, const Coord3D& place) {
    const auto& grid = topology_.GetGrid(place.z);
    const auto node_index = GetNodeIndex(place);
    for (auto b = beat; b <= GetFinalBeat(); ++b) {
        const auto index = GetBeatIndex(b);

        // Update nodes_.
        auto& node = nodes_[index][grid.GetIndex()][node_index];
        assert(node.type == Type::Ancilla && node.is_available);
        node.is_available = false;
    }
}
void QuantumStateBuffer::PutQubit(
        Beat beat,
        QSymbol qubit,
        const Coord3D& place,
        std::uint32_t dir
) {
    const auto& grid = topology_.GetGrid(place.z);
    const auto node_index = GetNodeIndex(place);
    for (auto b = beat; b <= GetFinalBeat(); ++b) {
        const auto index = GetBeatIndex(b);

        // Update nodes_.
        nodes_[index][grid.GetIndex()][node_index] =
                Node{.type = Type::Qubit, .symbol = qubit.Id()};

        // Update q2p_.
        q2p_[index][qubit] = {place, dir};
    }
}
void QuantumStateBuffer::PutMagicFactory(
        Beat beat,
        MSymbol magic_factory,
        const Coord3D& place,
        const MagicFactoryState& initial_mf_state
) {
    m2p_[magic_factory] = place;
    InsertSortedUniqueSymbol(z2m_[place.z], magic_factory);

    const auto& grid = topology_.GetGrid(place.z);
    const auto node_index = GetNodeIndex(place);

    auto index = GetBeatIndex(beat);
    nodes_[index][grid.GetIndex()][node_index] =
            Node{.type = Type::MagicFactory, .symbol = magic_factory.Id()};
    auto mf_state = initial_mf_state;
    mf_states_[index][magic_factory] = mf_state;
    index = (index + 1) % size_;
    for (auto b = beat + 1; b <= GetFinalBeat(); ++b) {
        // Update nodes_.
        nodes_[index][grid.GetIndex()][node_index] =
                Node{.type = Type::MagicFactory, .symbol = magic_factory.Id()};

        // Update mf_states_.
        StepMagicFactoryState(option_, mf_state);
        mf_states_[index][magic_factory] = mf_state;

        index = (index + 1) % size_;
    }
}
void QuantumStateBuffer::PutEntanglementFactory(
        Beat beat,
        ESymbol entanglement_factory1,
        const Coord3D& place1,
        ESymbol entanglement_factory2,
        const Coord3D& place2,
        const EntanglementFactoryState& ef_state1,
        const EntanglementFactoryState& ef_state2
) {
    e2p_[entanglement_factory1] = place1;
    e2p_[entanglement_factory2] = place2;
    e_pair_[entanglement_factory1] = entanglement_factory2;
    e_pair_[entanglement_factory2] = entanglement_factory1;
    InsertSortedUniqueSymbol(z2e_[place1.z], entanglement_factory1);
    InsertSortedUniqueSymbol(z2e_[place2.z], entanglement_factory2);

    const auto& grid1 = topology_.GetGrid(place1.z);
    const auto node_index1 = GetNodeIndex(place1);

    const auto& grid2 = topology_.GetGrid(place2.z);
    const auto node_index2 = GetNodeIndex(place2);

    auto index = GetBeatIndex(beat);
    auto ef_state_curr1 = ef_state1;
    auto ef_state_curr2 = ef_state2;
    nodes_[index][grid1.GetIndex()][node_index1] =
            Node{.type = Type::EntanglementFactory, .symbol = entanglement_factory1.Id()};
    nodes_[index][grid2.GetIndex()][node_index2] =
            Node{.type = Type::EntanglementFactory, .symbol = entanglement_factory2.Id()};
    ef_states_[index][entanglement_factory1] = ef_state_curr1;
    ef_states_[index][entanglement_factory2] = ef_state_curr2;
    for (auto b = beat + 1; b <= GetFinalBeat(); ++b) {
        StepEntanglementFactoryPairState(option_, ef_state_curr1, ef_state_curr2);
        index = (index + 1) % size_;

        nodes_[index][grid1.GetIndex()][node_index1] =
                Node{.type = Type::EntanglementFactory, .symbol = entanglement_factory1.Id()};
        nodes_[index][grid2.GetIndex()][node_index2] =
                Node{.type = Type::EntanglementFactory, .symbol = entanglement_factory2.Id()};
        ef_states_[index][entanglement_factory1] = ef_state_curr1;
        ef_states_[index][entanglement_factory2] = ef_state_curr2;
    }
}
void QuantumStateBuffer::RemoveQubit(Beat beat, QSymbol qubit) {
    const auto place = q2p_[GetBeatIndex(beat)].at(qubit).first;
    const auto& grid = topology_.GetGrid(place.z);
    const auto node_index = GetNodeIndex(place);
    for (auto b = beat; b <= GetFinalBeat(); ++b) {
        const auto index = GetBeatIndex(b);

        // Initialize nodes_.
        auto& node = nodes_[index][grid.GetIndex()][node_index];
        assert(node.is_available && node.type == Type::Qubit);
        node = Node{};

        // Remove qubit from q2p_.
        q2p_[index].erase(qubit);
    }
}
void QuantumStateBuffer::RotateQubit(Beat beat, QSymbol qubit) {
    for (auto b = beat; b <= GetFinalBeat(); ++b) {
        const auto index = GetBeatIndex(b);
        q2p_[index].at(qubit).second ^= 1u;
    }
}
void QuantumStateBuffer::SetQubitDir(Beat beat, QSymbol qubit, std::uint32_t dir) {
    for (auto b = beat; b <= GetFinalBeat(); ++b) {
        const auto index = GetBeatIndex(b);
        q2p_[index].at(qubit).second = dir;
    }
}
void QuantumStateBuffer::UseMagicState(Beat beat, MSymbol magic_factory) {
    for (auto b = beat; b <= GetFinalBeat(); ++b) {
        const auto index = GetBeatIndex(b);
        mf_states_[index].at(magic_factory).UseMagic();
    }
}
void QuantumStateBuffer::UseEntanglement(Beat beat, ESymbol entanglement_factory, EHandle handle) {
    const auto pair_factory = topology_.GetPair(entanglement_factory);
    auto index = GetBeatIndex(beat);
    auto ef_state = ef_states_[index].at(entanglement_factory);
    auto pa_state = ef_states_[index].at(pair_factory);
    if (ef_state.IsReserved(handle)) {
        ef_state.UseHandle(handle);
    } else {
        if (ef_state.entangled_state_stock_count_free == 0
            || pa_state.entangled_state_stock_count_free == 0) {
            throw std::runtime_error(
                    fmt::format(
                            "Insufficient free entanglement stock for handle reservation: "
                            "e_factory={}, pair_factory={}, handle={}",
                            entanglement_factory,
                            pair_factory,
                            handle
                    )
            );
        }
        // Consume one free pair from the requested factory side and reserve the counterpart.
        ef_state.entangled_state_stock_count_free -= 1;
        pa_state.AddHandle(handle);
    }
    ef_states_[index][entanglement_factory] = ef_state;
    ef_states_[index][pair_factory] = pa_state;

    for (auto b = beat + 1; b <= GetFinalBeat(); ++b) {
        StepEntanglementFactoryPairState(option_, ef_state, pa_state);
        index = GetBeatIndex(b);
        ef_states_[index][entanglement_factory] = ef_state;
        ef_states_[index][pair_factory] = pa_state;
    }
}
QuantumState& QuantumStateBuffer::GetQuantumState(Beat beat) {
    return states_[GetBeatIndex(beat)];
}
void QuantumStateBuffer::StepBeat() {
    const auto [curr_beat, curr_index] = begin_;
    const auto prev_index = (curr_index + size_ - 1) % size_;

    // prev_index is the latest plane.
    // [curr_index] initial beat
    // [prev_index] final beat
    // If beat steps, state of curr_index becomes the next state of prev_index

    // Copy latest plane to 'curr_index'.
    for (auto i = std::size_t{0}; i < topology_.NumGrids(); ++i) {
        const auto& grid = topology_.GetGridByIndex(i);
        const auto num_nodes = static_cast<std::size_t>(grid.GetMaxX())
                * static_cast<std::size_t>(grid.GetMaxY())
                * static_cast<std::size_t>(grid.GetZSize());
        for (auto j = std::size_t{0}; j < num_nodes; ++j) {
            nodes_[curr_index][i][j] = nodes_[prev_index][i][j];
        }
    }
    mf_states_[curr_index] = mf_states_[prev_index];
    ef_states_[curr_index] = ef_states_[prev_index];
    q2p_[curr_index] = q2p_[prev_index];

    // Update factory states.
    for (auto&& [_, mf_state] : mf_states_[curr_index]) {
        StepMagicFactoryState(option_, mf_state);
    }
    auto stepped = std::unordered_set<ESymbol>{};
    for (const auto& [e, pair_e] : e_pair_) {
        if (stepped.contains(e) || stepped.contains(pair_e)) {
            continue;
        }
        if (!ef_states_[curr_index].contains(e) || !ef_states_[curr_index].contains(pair_e)) {
            continue;
        }

        auto& ef_state = ef_states_[curr_index].at(e);
        auto& pair_state = ef_states_[curr_index].at(pair_e);
        StepEntanglementFactoryPairState(option_, ef_state, pair_state);
        stepped.insert(e);
        stepped.insert(pair_e);
    }

    // Update children
    states_[curr_index].SetBeat(curr_beat + size_);

    // Update 'begin_'.
    begin_ = {curr_beat + 1, (curr_index + 1) % size_};
}
#pragma endregion

#pragma region QuantumState
QuantumState::QuantumState(
        const Topology& topology,
        Beat beat,
        std::vector<std::vector<Node>>& nodes,
        std::unordered_map<MSymbol, MagicFactoryState>& mf_states,
        std::unordered_map<ESymbol, EntanglementFactoryState>& ef_states,
        std::unordered_map<QSymbol, std::pair<Coord3D, std::uint32_t>>& q2p
)
    : topology_(topology)
    , beat_(beat)
    , nodes_(nodes)
    , mf_states_(mf_states)
    , ef_states_(ef_states)
    , q2p_(q2p) {
    // Initialize children
    grids_.reserve(topology.NumGrids());
    for (const auto& grid : topology) {
        grids_.emplace_back(QuantumGrid(grid, nodes_[grid.GetIndex()]));
        grids_.back().SetParent(this);
    }
}
QuantumState::QuantumState(QuantumState&& from) noexcept
    : topology_(from.topology_)
    , parent_(from.parent_)
    , beat_(from.beat_)
    , nodes_(from.nodes_)
    , mf_states_(from.mf_states_)
    , ef_states_(from.ef_states_)
    , q2p_(from.q2p_) {
    // Initialize children
    grids_.reserve(from.topology_.NumGrids());
    for (const auto& grid : from.topology_) {
        grids_.emplace_back(QuantumGrid(grid, nodes_[grid.GetIndex()]));
        grids_.back().SetParent(this);
    }
}
QuantumGrid& QuantumState::GetGrid(std::int32_t z) {
    return grids_[topology_.GetGrid(z).GetIndex()];
}
QuantumGrid& QuantumState::GetGridByIndex(std::size_t grid_index) {
    return grids_[grid_index];
}
const std::vector<MSymbol>& QuantumState::GetMagicFactoryList(std::int32_t z) const {
    return parent_->z2m_.at(z);
}
bool QuantumState::IsAvailable(MSymbol m) const {
    return mf_states_.at(m).IsAvailable();
}
const Coord3D& QuantumState::GetPlace(MSymbol m) const {
    return parent_->m2p_.at(m);
}
const MagicFactoryState& QuantumState::GetState(MSymbol m) const {
    return mf_states_.at(m);
}
const std::vector<ESymbol>& QuantumState::GetEntanglementFactoryList(std::int32_t z) const {
    return parent_->z2e_.at(z);
}
bool QuantumState::IsAvailable(ESymbol e) const {
    return ef_states_.at(e).IsAvailable();
}
const Coord3D& QuantumState::GetPlace(ESymbol e) const {
    return parent_->e2p_.at(e);
}
const EntanglementFactoryState& QuantumState::GetState(ESymbol e) const {
    return ef_states_.at(e);
}
bool QuantumState::IsReserved(EHandle handle) const {
    for (const auto& [_, ef_state] : ef_states_) {
        if (ef_state.IsReserved(handle)) {
            return true;
        }
    }
    return false;
}
std::optional<ESymbol> QuantumState::LookupSymbol(EHandle handle) const {
    for (const auto& [e, ef_state] : ef_states_) {
        if (ef_state.IsReserved(handle)) {
            return e;
        }
    }
    return std::nullopt;
}
const Coord3D& QuantumState::GetPlace(QSymbol q) const {
    return q2p_.at(q).first;
}
std::uint32_t QuantumState::GetDir(QSymbol q) const {
    return q2p_.at(q).second;
}
void QuantumState::Dump(std::ostream& out) const {
    out << "----------------------------------------------" << std::endl;
    out << fmt::format("[BEAT {}] ", beat_)
        << fmt::format(
                   "#Q: {}, #MF: {}, #EF: {}, #Grid: {}",
                   q2p_.size(),
                   mf_states_.size(),
                   ef_states_.size(),
                   nodes_.size()
           )
        << std::endl;

    // Dump nodes.
    for (const auto& grid : topology_) {
        const auto& grid_node = nodes_[grid.GetIndex()];
        out << "-------------------" << std::endl;
        out << fmt::format("Grid {} (z: {} ~ {})", grid.GetIndex(), grid.GetMinZ(), grid.GetMaxZ())
            << std::endl;

        for (const auto& plane : grid) {
            out << std::endl;
            out << fmt::format(
                    "  Plane {} (x: {}, y: {}, z: {})",
                    plane.GetIndex(),
                    plane.GetMaxX(),
                    plane.GetMaxY(),
                    plane.GetZ()
            ) << std::endl;

            for (auto y = plane.GetMaxY() - 1; y >= 0; --y) {
                out << "  ";
                for (auto x = 0; x < plane.GetMaxX(); ++x) {
                    const auto& node = grid_node[GetNodeIndex({x, y, plane.GetZ()})];
                    out << node.ToChar();
                }
                out << std::endl;
            }
        }
    }

    out << "-------------------" << std::endl;

    // Dump maps.
    if (!q2p_.empty()) {
        out << "Qubits:" << std::endl;
        auto sorted = std::map<QSymbol, std::pair<Coord3D, std::uint32_t>>();
        for (const auto& [q, state] : q2p_) {
            sorted[q] = state;
        }
        for (const auto& [q, state] : sorted) {
            out << fmt::format("  [{}] place: {}, direction: {}", q, state.first, state.second)
                << std::endl;
        }
    }
    if (!mf_states_.empty()) {
        out << "Magic factory states:" << std::endl;
        auto sorted = std::map<MSymbol, MagicFactoryState>();
        for (const auto& [m, state] : mf_states_) {
            sorted[m] = state;
        }
        for (const auto& [m, state] : sorted) {
            out << fmt::format(
                    "  [{}] generation_step: {}, magic_state_count: {}",
                    m,
                    state.generation_step,
                    state.magic_state_count
            ) << std::endl;
        }
    }
    if (!ef_states_.empty()) {
        out << "Entanglement factory states:" << std::endl;
        auto sorted = std::map<ESymbol, EntanglementFactoryState>();
        for (const auto& [e, state] : ef_states_) {
            sorted[e] = state;
        }
        for (const auto& [e, state] : sorted) {
            out << fmt::format(
                    "  [{}] entangled_state_generation_elapsed: {}, "
                    "entangled_state_stock_count_free: {}, entangled_state_handle_list: {}",
                    e,
                    state.entangled_state_generation_elapsed,
                    state.entangled_state_stock_count_free,
                    state.entangled_state_handle_list
            ) << std::endl;
        }
    }
}
#pragma endregion

#pragma region QuantumGrid
QuantumGrid::QuantumGrid(const ScLsGrid& topology, std::vector<Node>& nodes)
    : topology_(topology)
    , nodes_(nodes) {
    // Initialize children
    planes_.reserve(topology.NumPlanes());
    const auto plane_size = static_cast<std::size_t>(topology.GetMaxX())
            * static_cast<std::size_t>(topology.GetMaxY());
    for (const auto& plane : topology) {
        auto span = std::span(nodes_);
        planes_.emplace_back(
                QuantumPlane(plane, span.subspan(plane.GetIndex() * plane_size, plane_size))
        );
        planes_.back().SetParent(this);
    }
}
QuantumGrid::QuantumGrid(QuantumGrid&& from) noexcept
    : topology_(from.topology_)
    , parent_(from.parent_)
    , nodes_(from.nodes_) {
    // Initialize children
    planes_.reserve(from.topology_.NumPlanes());
    const auto plane_size = static_cast<std::size_t>(from.topology_.GetMaxX())
            * static_cast<std::size_t>(from.topology_.GetMaxY());
    for (const auto& plane : from.topology_) {
        auto span = std::span(nodes_);
        planes_.emplace_back(
                QuantumPlane(plane, span.subspan(plane.GetIndex() * plane_size, plane_size))
        );
        planes_.back().SetParent(this);
    }
}
QuantumPlane& QuantumGrid::GetPlane(std::int32_t z) {
    return planes_[topology_.GetPlane(z).GetIndex()];
}
QuantumPlane& QuantumGrid::GetPlaneByIndex(std::size_t plane_index) {
    return planes_[plane_index];
}
#pragma endregion

#pragma region QuantumPlane
#pragma endregion
}  // namespace qret::sc_ls_fixed_v0
