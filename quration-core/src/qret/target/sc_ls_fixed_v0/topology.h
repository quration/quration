/**
 * @file qret/target/sc_ls_fixed_v0/topology.h
 * @brief Topology.
 */

#ifndef QRET_TARGET_SC_LS_FIXED_V0_TOPOLOGY_H
#define QRET_TARGET_SC_LS_FIXED_V0_TOPOLOGY_H

#include <fmt/format.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "qret/qret_export.h"
#include "qret/target/sc_ls_fixed_v0/geometry.h"
#include "qret/target/sc_ls_fixed_v0/symbol.h"

namespace qret::sc_ls_fixed_v0 {
using Coord2D = Vec2D<std::int32_t>;
using Coord3D = Vec3D<std::int32_t>;

/**
 * @brief 2D layout class.
 * @details You can place char on each square.
 */
class QRET_EXPORT Layout2D {
public:
    Layout2D(std::string_view layout, std::int32_t max_x, std::int32_t max_y)
        : layout_{layout}
        , max_{max_x, max_y} {
        if (max_x <= 0) {
            throw std::runtime_error("'max_x' must be positive.");
        }
        if (max_y <= 0) {
            throw std::runtime_error("'max_y' must be positive.");
        }
        if (layout.size() != static_cast<std::size_t>(max_x) * static_cast<std::size_t>(max_y)) {
            throw std::runtime_error(
                    "the size of 'layout' must be equal to the product of 'max_x' and 'max_y'."
            );
        }
    }
    /**
     * @brief Create layout from a 2D string that is delimited by '\n'.
     *
     * @exception Throw std::runtime_error if any of the following three rules are broken:
     * 1. the first character must not be '\n'
     * 2. the last character of layout must be '\n'
     * 3. the number of characters in each line must be equal
     *
     * @param layout 2D string
     * @return Layout2D layout
     */
    static Layout2D From2DString(std::string_view layout);

    [[nodiscard]] auto begin() {
        return layout_.begin();
    }
    [[nodiscard]] auto begin() const {
        return layout_.begin();
    }
    [[nodiscard]] auto cbegin() const {
        return layout_.cbegin();
    }
    [[nodiscard]] auto end() {
        return layout_.end();
    }
    [[nodiscard]] auto end() const {
        return layout_.end();
    }
    [[nodiscard]] auto cend() const {
        return layout_.cend();
    }

    [[nodiscard]] char GetChar(std::int32_t x, std::int32_t y) const {
        return layout_[GetIndex(x, y)];
    }
    [[nodiscard]] char GetChar(const Coord2D& v) const {
        return GetChar(v.x, v.y);
    }
    [[nodiscard]] std::string_view GetLine(std::int32_t y) const {
        return {layout_.begin() + static_cast<std::int64_t>(GetIndex(0, y)),
                layout_.begin() + static_cast<std::int64_t>(GetIndex(GetMaxX() - 1, y) + 1)};
    }
    [[nodiscard]] const std::string& GetRaw() const {
        return layout_;
    }

    [[nodiscard]] std::size_t Count(char c) const {
        return static_cast<std::size_t>(std::count(begin(), end(), c));
    }

    [[nodiscard]] constexpr std::int32_t GetMaxX() const {
        return max_.x;
    }
    [[nodiscard]] constexpr std::int32_t GetMaxY() const {
        return max_.y;
    }
    [[nodiscard]] std::size_t Size() const {
        return layout_.size();
    }

    [[nodiscard]] std::string ToString(bool y_axis_downwards = true) const;

    constexpr auto operator<=>(const Layout2D&) const = default;

protected:
    void Set(std::int32_t x, std::int32_t y, char c) {
        layout_[GetIndex(x, y)] = c;
    }
    void Set(const Coord2D& v, char c) {
        Set(v.x, v.y, c);
    }
    [[nodiscard]] std::size_t GetIndex(std::int32_t x, std::int32_t y) const;

private:
    std::string layout_;  //!< size = x * y, [[y=0 string], [y=1 string], ...]
    Coord2D max_;
};
std::ostream& operator<<(std::ostream& out, const Layout2D& layout);

// Forward declarations
class Topology;
class ScLsGrid;

/**
 * @brief Define topology of sc_ls_fixed_v0 qubits.
 */
class ScLsPlane : public Layout2D {
public:
    static constexpr char Free = '.';
    static constexpr char Banned = 'x';
    static constexpr char Qubit = 'Q';
    static constexpr char MagicFactory = 'M';
    static constexpr char EntanglementFactory = 'E';

    ScLsPlane(
            Topology* t,
            std::size_t index,
            std::int32_t z,
            std::int32_t max_x,
            std::int32_t max_y
    )
        : Layout2D(
                  std::string(
                          static_cast<std::size_t>(max_x) * static_cast<std::size_t>(max_y),
                          Free
                  ),
                  max_x,
                  max_y
          )
        , t_{t}
        , index_{index}
        , z_{z}
        , num_free_{static_cast<std::size_t>(max_x) * static_cast<std::size_t>(max_y)} {}

    [[nodiscard]] std::size_t GetIndex() const {
        return index_;
    }
    [[nodiscard]] std::int32_t GetZ() const {
        return z_;
    }

    [[nodiscard]] bool OutOfPlane(const Coord2D& p) const {
        return p.x < 0 || p.y < 0 || GetMaxX() <= p.x || GetMaxY() <= p.y;
    }
    [[nodiscard]] std::size_t NumFree() const {
        return num_free_;
    }
    [[nodiscard]] std::size_t NumBanned() const {
        return num_banned_;
    }
    [[nodiscard]] std::size_t NumQubits() const {
        return num_qubits_;
    }
    [[nodiscard]] std::size_t NumMagicFactories() const {
        return num_magic_factories_;
    }
    [[nodiscard]] std::size_t NumEntanglementFactories() const {
        return num_entanglement_factories_;
    }
    [[nodiscard]] bool IsFree(const Coord2D& p) const {
        return OutOfPlane(p) ? false : GetChar(p) == Free;
    }
    [[nodiscard]] bool IsBanned(const Coord2D& p) const {
        return OutOfPlane(p) ? false : GetChar(p) == Banned;
    }
    [[nodiscard]] bool IsQubit(const Coord2D& p) const {
        return OutOfPlane(p) ? false : GetChar(p) == Qubit;
    }
    [[nodiscard]] bool IsMagicFactory(const Coord2D& p) const {
        return OutOfPlane(p) ? false : GetChar(p) == MagicFactory;
    }
    [[nodiscard]] bool IsEntanglementFactory(const Coord2D& p) const {
        return OutOfPlane(p) ? false : GetChar(p) == EntanglementFactory;
    }
    [[nodiscard]] bool Contains(QSymbol q) const;
    [[nodiscard]] Coord2D GetPlace(QSymbol q) const;
    [[nodiscard]] bool Contains(MSymbol m) const;
    [[nodiscard]] Coord2D GetPlace(MSymbol m) const;
    [[nodiscard]] bool Contains(ESymbol e) const;
    [[nodiscard]] ESymbol GetPair(ESymbol e) const;
    [[nodiscard]] Coord2D GetPlace(ESymbol e) const;

    void SetFree(const Coord2D& p);
    void SetBanned(const Coord2D& p);
    void SetQubit(const Coord2D& p, QSymbol q);
    void SetMagicFactory(const Coord2D& p, MSymbol m);
    void SetEntanglementFactory(const Coord2D& p, ESymbol e, ESymbol pair);

    [[nodiscard]] auto bans_begin() {
        return bans_.begin();
    }
    [[nodiscard]] auto bans_begin() const {
        return bans_.begin();
    }
    [[nodiscard]] auto bans_end() {
        return bans_.end();
    }
    [[nodiscard]] auto bans_end() const {
        return bans_.end();
    }

    [[nodiscard]] auto qubits_begin() {
        return qubits_.begin();
    }
    [[nodiscard]] auto qubits_begin() const {
        return qubits_.begin();
    }
    [[nodiscard]] auto qubits_end() {
        return qubits_.end();
    }
    [[nodiscard]] auto qubits_end() const {
        return qubits_.end();
    }

    [[nodiscard]] auto magic_factories_begin() {
        return magic_factories_.begin();
    }
    [[nodiscard]] auto magic_factories_begin() const {
        return magic_factories_.begin();
    }
    [[nodiscard]] auto magic_factories_end() {
        return magic_factories_.end();
    }
    [[nodiscard]] auto magic_factories_end() const {
        return magic_factories_.end();
    }

    [[nodiscard]] auto entanglement_factories_begin() {
        return entanglement_factories_.begin();
    }
    [[nodiscard]] auto entanglement_factories_begin() const {
        return entanglement_factories_.begin();
    }
    [[nodiscard]] auto entanglement_factories_end() {
        return entanglement_factories_.end();
    }
    [[nodiscard]] auto entanglement_factories_end() const {
        return entanglement_factories_.end();
    }

private:
    Topology* t_;
    std::size_t index_;
    std::int32_t z_;

    std::size_t num_free_ = 0;
    std::size_t num_banned_ = 0;
    std::size_t num_qubits_ = 0;
    std::size_t num_magic_factories_ = 0;
    std::size_t num_entanglement_factories_ = 0;

    std::unordered_set<Coord2D> bans_ = {};
    std::unordered_map<Coord2D, QSymbol> qubits_ = {};
    std::unordered_map<Coord2D, MSymbol> magic_factories_ = {};
    std::unordered_map<Coord2D, ESymbol> entanglement_factories_ = {};
};
class QRET_EXPORT ScLsGrid {
public:
    ScLsGrid(Topology* t, std::size_t index, Coord2D max, std::int32_t min_z, std::int32_t max_z)
        : t_{t}
        , index_{index}
        , max_(max)
        , min_z_(min_z)
        , max_z_(max_z) {
        auto plane_index = std::size_t{0};
        for (auto z = min_z; z < max_z; ++z) {
            planes_.emplace_back(t, plane_index++, z, max.x, max.y);
        }
    }

    [[nodiscard]] const Topology& GetTopology() const {
        return *t_;
    }
    [[nodiscard]] std::size_t GetIndex() const {
        return index_;
    }
    [[nodiscard]] constexpr std::int32_t GetMaxX() const {
        return max_.x;
    }
    [[nodiscard]] constexpr std::int32_t GetMaxY() const {
        return max_.y;
    }
    [[nodiscard]] constexpr std::int32_t GetMinZ() const {
        return min_z_;
    }
    [[nodiscard]] constexpr std::int32_t GetMaxZ() const {
        return max_z_;
    }
    [[nodiscard]] constexpr std::int32_t GetZSize() const {
        return max_z_ - min_z_;
    }
    [[nodiscard]] bool IsPlane() const {
        return GetZSize() == 1;
    }

    [[nodiscard]] std::size_t NumPlanes() const {
        return planes_.size();
    }
    [[nodiscard]] const ScLsPlane& GetPlaneByIndex(std::size_t plane_index) const {
        return planes_[plane_index];
    }
    [[nodiscard]] ScLsPlane& GetMutPlaneByIndex(std::size_t plane_index) {
        return planes_[plane_index];
    }
    const ScLsPlane& GetPlane(std::int32_t z) const {
        if (z < min_z_ || max_z_ <= z) {
            throw std::out_of_range(fmt::format("z: {} is out of grid", z));
        }
        const auto index = z - min_z_;
        return planes_[static_cast<std::size_t>(index)];
    }
    ScLsPlane& GetMutPlane(std::int32_t z) {
        if (z < min_z_ || max_z_ <= z) {
            throw std::out_of_range(fmt::format("z: {} is out of grid", z));
        }
        const auto index = z - min_z_;
        return planes_[static_cast<std::size_t>(index)];
    }

    [[nodiscard]] auto begin() {
        return planes_.begin();
    }
    [[nodiscard]] auto begin() const {
        return planes_.begin();
    }
    [[nodiscard]] auto end() {
        return planes_.end();
    }
    [[nodiscard]] auto end() const {
        return planes_.end();
    }

private:
    Topology* t_;
    std::size_t index_;
    Coord2D max_;
    std::int32_t min_z_;
    std::int32_t max_z_;

    std::vector<ScLsPlane> planes_;
};
class QRET_EXPORT Topology {
public:
    static std::shared_ptr<Topology> FromYAML(const std::string& str);
    static std::shared_ptr<Topology> FromJSON(const Json& j);

    [[nodiscard]] bool ContainsZ(std::int32_t z) const {
        return z2idx_.contains(z);
    }
    [[nodiscard]] std::size_t NumGrids() const {
        return grids_.size();
    }
    [[nodiscard]] std::vector<std::int32_t> GetAllZs() const;
    [[nodiscard]] const ScLsGrid& GetGridByIndex(std::size_t grid_index) const {
        return grids_[grid_index];
    }
    [[nodiscard]] const ScLsGrid& GetGrid(std::int32_t z) const {
        if (!z2idx_.contains(z)) {
            throw std::runtime_error(fmt::format("Grid with z coordinate {} does not exist", z));
        }
        return grids_[z2idx_.at(z)];
    }
    [[nodiscard]] const ScLsGrid* FindGrid(std::int32_t z) const;
    [[nodiscard]] const ScLsPlane& GetPlane(std::int32_t z) const {
        return GetGrid(z).GetPlane(z);
    }
    [[nodiscard]] const ScLsPlane* FindPlane(std::int32_t z) const;

    [[nodiscard]] bool Contains(QSymbol q) const {
        return qs_.contains(q);
    }
    [[nodiscard]] const Coord3D& GetPlace(QSymbol q) const {
        return qs_.at(q);
    }
    [[nodiscard]] const Coord3D* FindPlace(QSymbol q) const;
    [[nodiscard]] bool Contains(MSymbol m) const {
        return ms_.contains(m);
    }
    [[nodiscard]] const Coord3D& GetPlace(MSymbol m) const {
        return ms_.at(m);
    }
    [[nodiscard]] const Coord3D* FindPlace(MSymbol m) const;
    [[nodiscard]] bool Contains(ESymbol e) const {
        return es_.contains(e);
    }
    [[nodiscard]] ESymbol GetPair(ESymbol e) const {
        return es_.at(e).first;
    }
    [[nodiscard]] std::optional<ESymbol> FindPair(ESymbol e) const;
    [[nodiscard]] const Coord3D& GetPlace(ESymbol e) const {
        return es_.at(e).second;
    }
    [[nodiscard]] const Coord3D* FindPlace(ESymbol e) const;

    /**
     * @brief Get linked z coordinates via entanglement factory pairs.
     */
    [[nodiscard]] std::vector<std::int32_t> GetLinkedZs(std::int32_t z) const;

    /**
     * @brief Get all entanglement factory pairs from src_z to dst_z.
     * @details Returned pairs are sorted by symbol IDs.
     */
    [[nodiscard]] std::vector<std::pair<ESymbol, ESymbol>>
    GetEntanglementPairsBetween(std::int32_t src_z, std::int32_t dst_z) const;

    [[nodiscard]] auto begin() {
        return grids_.begin();
    }
    [[nodiscard]] auto begin() const {
        return grids_.begin();
    }
    [[nodiscard]] auto end() {
        return grids_.end();
    }
    [[nodiscard]] auto end() const {
        return grids_.end();
    }

    /**
     * @brief Validate topology data format.
     * @details Topology is invalid if one of the following conditions is met.
     *
     * 1. Either x, y, or z coordinates of grid is invalid.
     * 2. Symbol must be unique.
     * 3. Place of symbol must be unique.
     * 4. Entanglement factories must be properly paired.
     *
     * @return true if topology is valid.
     * @return false if topology is invalid.
     */
    bool Validate() const;

    ScLsPlane& AddPlane(Coord2D max, std::int32_t z) {
        const auto grid_index = grids_.size();
        z2idx_[z] = grid_index;
        grids_.emplace_back(this, grid_index, max, z, z + 1);
        return grids_.back().GetMutPlaneByIndex(0);
    }
    ScLsGrid& AddGrid(Coord2D max, std::int32_t min_z, std::int32_t max_z) {
        const auto grid_index = grids_.size();
        for (auto z = min_z; z < max_z; ++z) {
            z2idx_[z] = grid_index;
        }
        return grids_.emplace_back(this, grid_index, max, min_z, max_z);
    }

private:
    friend ScLsPlane;

    void EraseQ(const Coord3D& c) {
        auto itr =
                std::find_if(qs_.begin(), qs_.end(), [c](const auto& x) { return x.second == c; });
        qs_.erase(itr);
    }
    void EraseM(const Coord3D& c) {
        auto itr =
                std::find_if(ms_.begin(), ms_.end(), [c](const auto& x) { return x.second == c; });
        ms_.erase(itr);
    }
    void EraseE(const Coord3D& c) {
        auto itr = std::find_if(es_.begin(), es_.end(), [c](const auto& x) {
            return x.second.second == c;
        });
        es_.erase(itr);
    }
    void SetQ(const Coord3D& c, QSymbol q) {
        qs_[q] = c;
    }
    void SetM(const Coord3D& c, MSymbol m) {
        ms_[m] = c;
    }
    void SetE(const Coord3D& c, ESymbol e, ESymbol pair) {
        es_[e] = {pair, c};
    }

    Topology() = default;

    std::vector<ScLsGrid> grids_;

    std::unordered_map<std::int32_t, std::size_t> z2idx_;

    std::unordered_map<QSymbol, Coord3D> qs_;
    std::unordered_map<MSymbol, Coord3D> ms_;
    std::unordered_map<ESymbol, std::pair<ESymbol, Coord3D>> es_;
};

void QRET_EXPORT to_json(Json& j, const ScLsGrid& grid);
void QRET_EXPORT to_json(Json& j, const Topology& topology);
}  // namespace qret::sc_ls_fixed_v0

#endif  // QRET_TARGET_SC_LS_FIXED_V0_TOPOLOGY_H
