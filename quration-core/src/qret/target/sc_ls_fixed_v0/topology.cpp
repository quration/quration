/**
 * @file qret/target/sc_ls_fixed_v0/topology.cpp
 * @brief Topology.
 */

#include "qret/target/sc_ls_fixed_v0/topology.h"

#include <fmt/format.h>
#include <yaml-cpp/node/detail/node_iterator.h>
#include <yaml-cpp/node/iterator.h>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include "qret/base/log.h"
#include "qret/target/sc_ls_fixed_v0/symbol.h"

namespace qret::sc_ls_fixed_v0 {
Layout2D Layout2D::From2DString(std::string_view layout) {
    if (layout.empty()) {
        throw std::runtime_error("layout must not be empty");
    }
    if (layout[0] == '\n') {
        throw std::runtime_error("the first character of layout must not be '\\n'");
    }
    if (layout.back() != '\n') {
        throw std::runtime_error("the last character of layout must be '\\n'");
    }

    auto str1d = std::string();
    str1d.reserve(layout.size());
    auto max_x = std::int32_t{0};
    auto max_y = std::int32_t{0};
    auto count = std::int32_t{0};
    for (auto i = std::size_t{0}; i < layout.size(); ++i) {
        if (layout[i] == '\n') {
            if (max_x == 0) {
                max_x = count;
            } else {
                if (max_x != count) {
                    throw std::runtime_error("the number of characters in each line must be equal");
                }
            }
            max_y++;
            count = 0;
        } else {
            str1d.push_back(layout[i]);
            count++;
        }
    }
    return {str1d, max_x, max_y};
}
std::string Layout2D::ToString(bool y_axis_downwards) const {
    auto ss = std::stringstream{};
    for (auto tmp_y = std::int32_t{0}; tmp_y < max_.y; ++tmp_y) {
        const auto y = y_axis_downwards ? tmp_y : max_.y - 1 - tmp_y;
        for (auto x = std::int32_t{0}; x < max_.x; ++x) {
            ss << GetChar(x, y);
        }
        if (tmp_y + 1 != max_.y) {
            ss << '\n';
        }
    }
    return ss.str();
}
[[nodiscard]] std::size_t Layout2D::GetIndex(std::int32_t x, std::int32_t y) const {
#ifndef NDEBUG
    if (x < 0) {
        throw std::runtime_error(
                fmt::format(
                        "'x' must be non-negative. x={}, y={}, max_x={}, max_y={}",
                        x,
                        y,
                        max_.x,
                        max_.y
                )
        );
    }
    if (max_.x <= x) {
        throw std::runtime_error(
                fmt::format(
                        "'x' must be smaller than max_x. x={}, y={}, max_x={}, max_y={}",
                        x,
                        y,
                        max_.x,
                        max_.y
                )
        );
    }
    if (y < 0) {
        throw std::runtime_error(
                fmt::format(
                        "'y' must be non-negative. x={}, y={}, max_x={}, max_y={}",
                        x,
                        y,
                        max_.x,
                        max_.y
                )
        );
    }
    if (max_.y <= y) {
        throw std::runtime_error(
                fmt::format(
                        "'y' must be smaller than max_y. x={}, y={}, max_x={}, max_y={}",
                        x,
                        y,
                        max_.x,
                        max_.y
                )
        );
    }
#endif
    return (static_cast<std::size_t>(max_.x) * static_cast<std::size_t>(y))
            + static_cast<std::size_t>(x);
}
std::ostream& operator<<(std::ostream& out, const Layout2D& layout) {
    return out << layout.ToString();
}
[[nodiscard]] bool ScLsPlane::Contains(QSymbol q) const {
    if (!t_->Contains(q)) {
        return false;
    }
    return t_->GetPlace(q).z == z_;
}
[[nodiscard]] Coord2D ScLsPlane::GetPlace(QSymbol q) const {
    if (!t_->Contains(q)) {
        throw std::runtime_error(fmt::format("No QSymbol {} found", q));
    }
    return t_->GetPlace(q).XY();
}
[[nodiscard]] bool ScLsPlane::Contains(MSymbol m) const {
    if (!t_->Contains(m)) {
        return false;
    }
    return t_->GetPlace(m).z == z_;
}
[[nodiscard]] Coord2D ScLsPlane::GetPlace(MSymbol m) const {
    if (!t_->Contains(m)) {
        throw std::runtime_error(fmt::format("No MSymbol {} found", m));
    }
    return t_->GetPlace(m).XY();
}
[[nodiscard]] bool ScLsPlane::Contains(ESymbol e) const {
    if (!t_->Contains(e)) {
        return false;
    }
    return t_->GetPlace(e).z == z_;
}
[[nodiscard]] ESymbol ScLsPlane::GetPair(ESymbol e) const {
    if (!t_->Contains(e)) {
        throw std::runtime_error(fmt::format("No ESymbol {} found", e));
    }
    return t_->GetPair(e);
}
[[nodiscard]] Coord2D ScLsPlane::GetPlace(ESymbol e) const {
    if (!t_->Contains(e)) {
        throw std::runtime_error(fmt::format("No ESymbol {} found", e));
    }
    return t_->GetPlace(e).XY();
}
void ScLsPlane::SetFree(const Coord2D& p) {
    if (OutOfPlane(p)) {
        throw std::out_of_range(fmt::format("coordinate: {} is out of plane", p));
    }
    const auto old = GetChar(p);
    if (old == Free) {
        return;
    }

    num_free_++;
    Set(p, Free);
    if (old == Banned) {
        num_banned_--;
        bans_.erase(p);
    } else if (old == Qubit) {
        num_qubits_--;
        qubits_.erase(p);
        t_->EraseQ({p.x, p.y, z_});
    } else if (old == MagicFactory) {
        num_magic_factories_--;
        magic_factories_.erase(p);
        t_->EraseM({p.x, p.y, z_});
    } else if (old == EntanglementFactory) {
        num_entanglement_factories_--;
        entanglement_factories_.erase(p);
        t_->EraseE({p.x, p.y, z_});
    }
}
void ScLsPlane::SetBanned(const Coord2D& p) {
    if (OutOfPlane(p)) {
        throw std::out_of_range(fmt::format("coordinate: {} is out of plane", p));
    }
    SetFree(p);
    num_free_--;
    Set(p, Banned);
    num_banned_++;
    bans_.insert(p);
}
void ScLsPlane::SetQubit(const Coord2D& p, QSymbol q) {
    if (OutOfPlane(p)) {
        throw std::out_of_range(fmt::format("coordinate: {} is out of plane", p));
    }
    SetFree(p);
    num_free_--;
    Set(p, Qubit);
    num_qubits_++;
    qubits_[p] = q;
    t_->SetQ({p.x, p.y, z_}, q);
}
void ScLsPlane::SetMagicFactory(const Coord2D& p, MSymbol m) {
    if (OutOfPlane(p)) {
        throw std::out_of_range(fmt::format("coordinate: {} is out of plane", p));
    }
    SetFree(p);
    num_free_--;
    Set(p, MagicFactory);
    num_magic_factories_++;
    magic_factories_[p] = m;
    t_->SetM({p.x, p.y, z_}, m);
}
void ScLsPlane::SetEntanglementFactory(const Coord2D& p, ESymbol e, ESymbol pair) {
    if (OutOfPlane(p)) {
        throw std::out_of_range(fmt::format("coordinate: {} is out of plane", p));
    }
    SetFree(p);
    num_free_--;
    Set(p, EntanglementFactory);
    num_entanglement_factories_++;
    entanglement_factories_[p] = e;
    t_->SetE({p.x, p.y, z_}, e, pair);
}
void ParsePlane(std::shared_ptr<Topology>& t, const YAML::detail::iterator_value& yaml) {
    if (yaml["coord"].size() != 3) {
        throw std::runtime_error("size of 'coord' must be 3");
    }

    auto& plane = [&t, &yaml]() -> ScLsPlane& {
        const auto x = yaml["coord"][0].as<std::int32_t>();
        const auto y = yaml["coord"][1].as<std::int32_t>();
        const auto z = yaml["coord"][2].as<std::int32_t>();

        return t->AddPlane({x, y}, z);
    }();

    // ban
    for (const auto& ban : yaml["ban"]) {
        const auto x = ban[0].as<std::int32_t>();
        const auto y = ban[1].as<std::int32_t>();
        plane.SetBanned({x, y});
    }

    // qubit
    for (const auto& q : yaml["qubit"]) {
        if (q["coord"].size() != 2) {
            throw std::runtime_error("size of 'qubit.coord' must be 2");
        }
        const auto x = q["coord"][0].as<std::int32_t>();
        const auto y = q["coord"][1].as<std::int32_t>();
        const auto symbol = q["symbol"].as<QSymbol::IdType>();
        plane.SetQubit({x, y}, QSymbol{symbol});
    }

    // magic factory
    for (const auto& mf : yaml["magic_factory"]) {
        if (mf["coord"].size() != 2) {
            throw std::runtime_error("size of 'magic_factory.coord' must be 2");
        }
        const auto x = mf["coord"][0].as<std::int32_t>();
        const auto y = mf["coord"][1].as<std::int32_t>();
        const auto symbol = mf["symbol"].as<MSymbol::IdType>();
        plane.SetMagicFactory({x, y}, MSymbol{symbol});
    }

    // entanglement factory
    for (const auto& ef : yaml["entanglement_factory"]) {
        if (ef["coord"].size() != 2) {
            throw std::runtime_error("size of 'entanglement_factory.coord' must be 2");
        }
        const auto x = ef["coord"][0].as<std::int32_t>();
        const auto y = ef["coord"][1].as<std::int32_t>();
        const auto symbol = ef["symbol"].as<ESymbol::IdType>();
        const auto pair = ef["pair"].as<ESymbol::IdType>();
        plane.SetEntanglementFactory({x, y}, ESymbol{symbol}, ESymbol{pair});
    }
}
void ParseGrid(std::shared_ptr<Topology>& t, const YAML::detail::iterator_value& yaml) {
    if (yaml["coord"].size() != 4) {
        throw std::runtime_error("size of 'coord' must be 4");
    }

    auto& grid = [&t, &yaml]() -> ScLsGrid& {
        const auto x = yaml["coord"][0].as<std::int32_t>();
        const auto y = yaml["coord"][1].as<std::int32_t>();
        const auto z_min = yaml["coord"][2].as<std::int32_t>();
        const auto z_max = yaml["coord"][3].as<std::int32_t>();
        return t->AddGrid({x, y}, z_min, z_max);
    }();

    // ban
    for (const auto& ban : yaml["ban"]) {
        const auto x = ban[0].as<std::int32_t>();
        const auto y = ban[1].as<std::int32_t>();
        const auto z = ban[2].as<std::int32_t>();
        grid.GetMutPlane(z).SetBanned({x, y});
    }

    // qubit
    for (const auto& q : yaml["qubit"]) {
        if (q["coord"].size() != 3) {
            throw std::runtime_error("size of 'qubit.coord' must be 3");
        }
        const auto x = q["coord"][0].as<std::int32_t>();
        const auto y = q["coord"][1].as<std::int32_t>();
        const auto z = q["coord"][2].as<std::int32_t>();
        const auto symbol = q["symbol"].as<QSymbol::IdType>();
        grid.GetMutPlane(z).SetQubit({x, y}, QSymbol{symbol});
    }

    // magic factory
    for (const auto& mf : yaml["magic_factory"]) {
        if (mf["coord"].size() != 3) {
            throw std::runtime_error("size of 'magic_factory.coord' must be 3");
        }
        const auto x = mf["coord"][0].as<std::int32_t>();
        const auto y = mf["coord"][1].as<std::int32_t>();
        const auto z = mf["coord"][2].as<std::int32_t>();
        const auto symbol = mf["symbol"].as<MSymbol::IdType>();
        grid.GetMutPlane(z).SetMagicFactory({x, y}, MSymbol{symbol});
    }

    // entanglement factory
    for (const auto& ef : yaml["entanglement_factory"]) {
        if (ef["coord"].size() != 3) {
            throw std::runtime_error("size of 'entanglement_factory.coord' must be 3");
        }
        const auto x = ef["coord"][0].as<std::int32_t>();
        const auto y = ef["coord"][1].as<std::int32_t>();
        const auto z = ef["coord"][2].as<std::int32_t>();
        const auto symbol = ef["symbol"].as<ESymbol::IdType>();
        const auto pair = ef["pair"].as<ESymbol::IdType>();
        grid.GetMutPlane(z).SetEntanglementFactory({x, y}, ESymbol{symbol}, ESymbol{pair});
    }
}
std::shared_ptr<Topology> Topology::FromYAML(const std::string& str) {
    auto ret = std::shared_ptr<Topology>(new Topology());

    auto yaml = YAML::Load(str.data());

    for (const auto& grid : yaml["grids"]) {
        if (!grid["type"].IsDefined()) {
            throw std::runtime_error("'type' is not defined");
        }
        const auto type = grid["type"].Scalar();
        if (type == "plane") {
            ParsePlane(ret, grid);
        } else if (type == "grid") {
            ParseGrid(ret, grid);
        }
    }

    if (!ret->Validate()) {
        throw std::runtime_error(fmt::format("YAML topology file '{}' is not valid", str));
    }

    return ret;
}
void ParsePlane(std::shared_ptr<Topology>& t, const Json& j) {
    if (j["coord"].size() != 3) {
        throw std::runtime_error("size of 'coord' must be 3");
    }

    auto& plane = [&t, &j]() -> ScLsPlane& {
        const auto x = j["coord"][0].get<std::int32_t>();
        const auto y = j["coord"][1].get<std::int32_t>();
        const auto z = j["coord"][2].get<std::int32_t>();

        return t->AddPlane({x, y}, z);
    }();

    // ban
    if (j.contains("ban")) {
        for (const auto& ban : j["ban"]) {
            const auto x = ban[0].get<std::int32_t>();
            const auto y = ban[1].get<std::int32_t>();
            plane.SetBanned({x, y});
        }
    }

    // qubit
    if (j.contains("qubit")) {
        for (const auto& q : j["qubit"]) {
            if (!q["coord"].is_array() || q["coord"].size() != 2) {
                throw std::runtime_error("size of 'qubit.coord' must be 2");
            }
            const auto x = q["coord"][0].get<std::int32_t>();
            const auto y = q["coord"][1].get<std::int32_t>();
            const auto symbol = q["symbol"].get<QSymbol::IdType>();
            plane.SetQubit({x, y}, QSymbol{symbol});
        }
    }

    // magic factory
    if (j.contains("magic_factory")) {
        for (const auto& mf : j["magic_factory"]) {
            if (!mf["coord"].is_array() || mf["coord"].size() != 2) {
                throw std::runtime_error("size of 'magic_factory.coord' must be 2");
            }
            const auto x = mf["coord"][0].get<std::int32_t>();
            const auto y = mf["coord"][1].get<std::int32_t>();
            const auto symbol = mf["symbol"].get<MSymbol::IdType>();
            plane.SetMagicFactory({x, y}, MSymbol{symbol});
        }
    }

    // entanglement factory
    if (j.contains("entanglement_factory")) {
        for (const auto& ef : j["entanglement_factory"]) {
            if (!ef["coord"].is_array() || ef["coord"].size() != 2) {
                throw std::runtime_error("size of 'entanglement_factory.coord' must be 2");
            }
            const auto x = ef["coord"][0].get<std::int32_t>();
            const auto y = ef["coord"][1].get<std::int32_t>();
            const auto symbol = ef["symbol"].get<ESymbol::IdType>();
            const auto pair = ef["pair"].get<ESymbol::IdType>();
            plane.SetEntanglementFactory({x, y}, ESymbol{symbol}, ESymbol{pair});
        }
    }
}
void ParseGrid(std::shared_ptr<Topology>& t, const Json& j) {
    if (j["coord"].size() != 4) {
        throw std::runtime_error("size of 'coord' must be 4");
    }

    auto& grid = [&t, &j]() -> ScLsGrid& {
        const auto x = j["coord"][0].get<std::int32_t>();
        const auto y = j["coord"][1].get<std::int32_t>();
        const auto z_min = j["coord"][2].get<std::int32_t>();
        const auto z_max = j["coord"][3].get<std::int32_t>();
        return t->AddGrid({x, y}, z_min, z_max);
    }();

    // ban
    if (j.contains("ban")) {
        for (const auto& ban : j["ban"]) {
            const auto x = ban[0].get<std::int32_t>();
            const auto y = ban[1].get<std::int32_t>();
            const auto z = ban[2].get<std::int32_t>();
            grid.GetMutPlane(z).SetBanned({x, y});
        }
    }

    // qubit
    if (j.contains("qubit")) {
        for (const auto& q : j["qubit"]) {
            if (!q["coord"].is_array() || q["coord"].size() != 3) {
                throw std::runtime_error("size of 'qubit.coord' must be 3");
            }
            const auto x = q["coord"][0].get<std::int32_t>();
            const auto y = q["coord"][1].get<std::int32_t>();
            const auto z = q["coord"][2].get<std::int32_t>();
            const auto symbol = q["symbol"].get<QSymbol::IdType>();
            grid.GetMutPlane(z).SetQubit({x, y}, QSymbol{symbol});
        }
    }

    // magic factory
    if (j.contains("magic_factory")) {
        for (const auto& mf : j["magic_factory"]) {
            if (!mf["coord"].is_array() || mf["coord"].size() != 3) {
                throw std::runtime_error("size of 'magic_factory.coord' must be 3");
            }
            const auto x = mf["coord"][0].get<std::int32_t>();
            const auto y = mf["coord"][1].get<std::int32_t>();
            const auto z = mf["coord"][2].get<std::int32_t>();
            const auto symbol = mf["symbol"].get<MSymbol::IdType>();
            grid.GetMutPlane(z).SetMagicFactory({x, y}, MSymbol{symbol});
        }
    }

    // entanglement factory
    if (j.contains("entanglement_factory")) {
        for (const auto& ef : j["entanglement_factory"]) {
            if (!ef["coord"].is_array() || ef["coord"].size() != 3) {
                throw std::runtime_error("size of 'entanglement_factory.coord' must be 3");
            }
            const auto x = ef["coord"][0].get<std::int32_t>();
            const auto y = ef["coord"][1].get<std::int32_t>();
            const auto z = ef["coord"][2].get<std::int32_t>();
            const auto symbol = ef["symbol"].get<ESymbol::IdType>();
            const auto pair = ef["pair"].get<ESymbol::IdType>();
            grid.GetMutPlane(z).SetEntanglementFactory({x, y}, ESymbol{symbol}, ESymbol{pair});
        }
    }
}
std::shared_ptr<Topology> Topology::FromJSON(const Json& j) {
    auto ret = std::shared_ptr<Topology>(new Topology());

    if (!j.is_array()) {
        throw std::runtime_error("Topology file must be array.");
    }

    for (const auto& grid : j) {
        if (!grid.contains("type")) {
            throw std::runtime_error("'type' is not defined");
        }

        const auto type = grid["type"].get<std::string>();
        if (type == "plane") {
            ParsePlane(ret, grid);
        } else if (type == "grid") {
            ParseGrid(ret, grid);
        } else {
            throw std::runtime_error(fmt::format("Invalid grid type: {}", type));
        }
    }

    if (!ret->Validate()) {
        throw std::runtime_error(fmt::format("JSON topology string is not valid: {}", j.dump()));
    }

    return ret;
}
std::vector<std::int32_t> Topology::GetAllZs() const {
    auto zs = std::vector<std::int32_t>{};
    zs.reserve(z2idx_.size());
    for (const auto& [z, _] : z2idx_) {
        zs.emplace_back(z);
    }
    std::sort(zs.begin(), zs.end());
    return zs;
}
const ScLsGrid* Topology::FindGrid(std::int32_t z) const {
    if (!ContainsZ(z)) {
        return nullptr;
    }
    return &grids_[z2idx_.at(z)];
}
const ScLsPlane* Topology::FindPlane(std::int32_t z) const {
    const auto* grid = FindGrid(z);
    if (grid == nullptr) {
        return nullptr;
    }
    return &grid->GetPlane(z);
}
const Coord3D* Topology::FindPlace(QSymbol q) const {
    const auto itr = qs_.find(q);
    if (itr == qs_.end()) {
        return nullptr;
    }
    return &itr->second;
}
const Coord3D* Topology::FindPlace(MSymbol m) const {
    const auto itr = ms_.find(m);
    if (itr == ms_.end()) {
        return nullptr;
    }
    return &itr->second;
}
std::optional<ESymbol> Topology::FindPair(ESymbol e) const {
    const auto itr = es_.find(e);
    if (itr == es_.end()) {
        return std::nullopt;
    }
    return itr->second.first;
}
const Coord3D* Topology::FindPlace(ESymbol e) const {
    const auto itr = es_.find(e);
    if (itr == es_.end()) {
        return nullptr;
    }
    return &itr->second.second;
}
std::vector<std::int32_t> Topology::GetLinkedZs(std::int32_t z) const {
    auto zs = std::vector<std::int32_t>();
    if (!ContainsZ(z)) {
        return zs;
    }

    auto seen = std::unordered_set<std::int32_t>();
    for (const auto& [e, pair_coord] : es_) {
        const auto& [e_pair, c] = pair_coord;
        if (c.z != z) {
            continue;
        }
        const auto pair_itr = es_.find(e_pair);
        if (pair_itr == es_.end()) {
            continue;
        }
        const auto z_pair = pair_itr->second.second.z;
        if (seen.contains(z_pair)) {
            continue;
        }
        seen.emplace(z_pair);
        zs.emplace_back(z_pair);
    }

    std::sort(zs.begin(), zs.end());
    return zs;
}
std::vector<std::pair<ESymbol, ESymbol>>
Topology::GetEntanglementPairsBetween(std::int32_t src_z, std::int32_t dst_z) const {
    auto pairs = std::vector<std::pair<ESymbol, ESymbol>>{};
    if (!ContainsZ(src_z) || !ContainsZ(dst_z) || src_z == dst_z) {
        return pairs;
    }

    for (const auto& [e, pair_coord] : es_) {
        const auto& [e_pair, c] = pair_coord;
        if (c.z != src_z) {
            continue;
        }
        const auto pair_itr = es_.find(e_pair);
        if (pair_itr == es_.end()) {
            continue;
        }
        if (pair_itr->second.second.z != dst_z) {
            continue;
        }
        pairs.emplace_back(e, e_pair);
    }

    std::sort(
            pairs.begin(),
            pairs.end(),
            [](const std::pair<ESymbol, ESymbol>& x, const std::pair<ESymbol, ESymbol>& y) {
                if (x.first.Id() != y.first.Id()) {
                    return x.first.Id() < y.first.Id();
                }
                return x.second.Id() < y.second.Id();
            }
    );
    return pairs;
}
bool Topology::Validate() const {
    auto is_valid = true;

    // X and Y must be positive.
    // Min Z < Max Z.
    {
        for (const auto& grid : *this) {
            if (grid.GetMaxX() <= 0) {
                LOG_ERROR("X is not positive (grid index: {})", grid.GetIndex());
                is_valid = false;
            }
            if (grid.GetMaxY() <= 0) {
                LOG_ERROR("Y is not positive (grid index: {})", grid.GetIndex());
                is_valid = false;
            }
            if (grid.GetMinZ() >= grid.GetMaxZ()) {
                LOG_ERROR(
                        "Minimum of Z is equal to or greater than maximum of Z (grid index: {})",
                        grid.GetIndex()
                );
                is_valid = false;
            }
        }
    }

    // Z must be unique.
    {
        auto zs = std::unordered_set<std::int32_t>();
        for (const auto& grid : *this) {
            for (auto z = grid.GetMinZ(); z < grid.GetMaxZ(); ++z) {
                if (zs.contains(z)) {
                    LOG_DEBUG("Z coordinate: {} is overlapped", z);
                    is_valid = false;
                }
                zs.emplace(z);
            }
        }
    }

    // Symbol must not be conflict.
    auto qs = std::unordered_set<QSymbol>{};
    auto ms = std::unordered_set<MSymbol>{};
    auto es = std::unordered_set<ESymbol>{};
    for (const auto& grid : *this) {
        for (const auto& plane : grid) {
            for (auto itr = plane.qubits_begin(); itr != plane.qubits_end(); ++itr) {
                const auto& q = itr->second;
                if (qs.contains(q)) {
                    LOG_ERROR("Qubit overlap: {}", q);
                    is_valid = false;
                }
                qs.emplace(q);
            }
            for (auto itr = plane.magic_factories_begin(); itr != plane.magic_factories_end();
                 ++itr) {
                const auto& m = itr->second;
                if (ms.contains(m)) {
                    LOG_ERROR("Magic state factory overlap: {}", m);
                    is_valid = false;
                }
                ms.emplace(m);
            }
            for (auto itr = plane.entanglement_factories_begin();
                 itr != plane.entanglement_factories_end();
                 ++itr) {
                const auto& e = itr->second;
                if (es.contains(e)) {
                    LOG_ERROR("Entanglement factory overlap: {}", e);
                    is_valid = false;
                }
                es.emplace(e);
            }
        }
    }

    // Check coordinate.
    const auto check_x = [&is_valid](std::int32_t x, std::int32_t max_x) {
        if (x < 0 || max_x <= x) {
            LOG_ERROR("X must be [{}, {}) (actual: {})", 0, max_x, x);
            is_valid = false;
        }
    };
    const auto check_y = [&is_valid](std::int32_t y, std::int32_t max_y) {
        if (y < 0 || max_y <= y) {
            LOG_ERROR("Y must be [{}, {}) (actual: {})", 0, max_y, y);
            is_valid = false;
        }
    };

    // Place must not be conflict.
    {
        for (const auto& grid : *this) {
            for (const auto& plane : grid) {
                auto cs = std::unordered_set<Coord3D>();

                for (auto itr = plane.bans_begin(); itr != plane.bans_end(); ++itr) {
                    const auto& c = Coord3D(itr->x, itr->y, plane.GetZ());
                    check_x(c.x, plane.GetMaxX());
                    check_y(c.y, plane.GetMaxY());
                    if (cs.contains(c)) {
                        LOG_ERROR("Ban coordinate overlap: {}", c);
                        is_valid = false;
                    }
                    cs.emplace(c);
                }
                for (auto itr = plane.qubits_begin(); itr != plane.qubits_end(); ++itr) {
                    const auto& coord = itr->first;
                    const auto& c = Coord3D(coord.x, coord.y, plane.GetZ());
                    check_x(c.x, plane.GetMaxX());
                    check_y(c.y, plane.GetMaxY());
                    if (cs.contains(c)) {
                        LOG_ERROR("Qubit coordinate overlap: {}", c);
                        is_valid = false;
                    }
                    cs.emplace(c);
                }
                for (auto itr = plane.magic_factories_begin(); itr != plane.magic_factories_end();
                     ++itr) {
                    const auto& coord = itr->first;
                    const auto& c = Coord3D(coord.x, coord.y, plane.GetZ());
                    check_x(c.x, plane.GetMaxX());
                    check_y(c.y, plane.GetMaxY());
                    if (cs.contains(c)) {
                        LOG_ERROR("Magic state factory coordinate overlap: {}", c);
                        is_valid = false;
                    }
                    cs.emplace(c);
                }
                for (auto itr = plane.entanglement_factories_begin();
                     itr != plane.entanglement_factories_end();
                     ++itr) {
                    const auto& coord = itr->first;
                    const auto& c = Coord3D(coord.x, coord.y, plane.GetZ());
                    check_x(c.x, plane.GetMaxX());
                    check_y(c.y, plane.GetMaxY());
                    if (cs.contains(c)) {
                        LOG_ERROR("Entanglement factory coordinate overlap : {}", c);
                        is_valid = false;
                    }
                    cs.emplace(c);
                }
            }
        }
    }

    // Check entanglement factory pair.
    {
        // Check symbols are paired (bidirectional and existing).
        for (const auto e : es) {
            const auto p = GetPair(e);
            if (!es.contains(p)) {
                LOG_ERROR("Entanglement factory pair target does not exist: {} -> {}", e, p);
                is_valid = false;
            }
            if (e == p) {
                LOG_ERROR("Entanglement factory cannot be paired to itself: {}", e);
                is_valid = false;
                continue;
            }

            const auto p_pair = GetPair(p);
            if (p_pair != e) {
                LOG_ERROR(
                        "Entanglement factory pair must be bidirectional: {} -> {}, but {} -> {}",
                        e,
                        p,
                        p,
                        p_pair
                );
                is_valid = false;
            }
        }

        // Check coordinates are on the different grid.
        for (const auto& grid : *this) {
            for (const auto& plane : grid) {
                for (auto itr = plane.entanglement_factories_begin();
                     itr != plane.entanglement_factories_end();
                     ++itr) {
                    const auto& e = itr->second;
                    const auto p = GetPair(e);

                    const auto z_pair = GetPlace(p).z;
                    if (grid.GetMinZ() <= z_pair && z_pair < grid.GetMaxZ()) {
                        LOG_ERROR(
                                "Entanglement factory is paired in the same grid: {} and {}",
                                e,
                                p
                        );
                        is_valid = false;
                    }
                }
            }
        }
    }

    return is_valid;
}
void to_json(Json& j, const ScLsGrid& grid) {
    if (grid.IsPlane()) {
        // Plane
        j["type"] = "plane";
        j["coord"] = {grid.GetMaxX(), grid.GetMaxY(), grid.GetMinZ()};
        const auto& plane = grid.GetPlaneByIndex(0);

        const auto to_json_symbol = [](Json& j,
                                       const std::string& key,
                                       const ScLsPlane& plane,
                                       const auto begin,
                                       const auto end) {
            auto array = Json::array();
            for (auto itr = begin; itr != end; ++itr) {
                const auto& [coord, symbol] = *itr;
                auto tmp = Json();
                tmp["symbol"] = symbol.Id();
                if constexpr (std::is_same_v<ESymbol, std::remove_cvref_t<decltype(symbol)>>) {
                    tmp["pair"] = plane.GetPair(symbol).Id();
                }
                tmp["coord"] = Json::array({coord.x, coord.y});
                array.emplace_back(tmp);
            }
            std::sort(array.begin(), array.end());
            if (!array.empty()) {
                j[key] = std::move(array);
            }
        };

        if (plane.NumBanned() != 0) {
            auto array = Json::array();
            for (auto itr = plane.bans_begin(); itr != plane.bans_end(); ++itr) {
                const auto& coord = *itr;
                array.emplace_back(Json::array({coord.x, coord.y}));
            }
            std::sort(array.begin(), array.end());
            j["ban"] = std::move(array);
        }
        if (plane.NumQubits() != 0) {
            to_json_symbol(j, "qubit", plane, plane.qubits_begin(), plane.qubits_end());
        }
        if (plane.NumMagicFactories() != 0) {
            to_json_symbol(
                    j,
                    "magic_factory",
                    plane,
                    plane.magic_factories_begin(),
                    plane.magic_factories_end()
            );
        }
        if (plane.NumEntanglementFactories() != 0) {
            to_json_symbol(
                    j,
                    "entanglement_factory",
                    plane,
                    plane.entanglement_factories_begin(),
                    plane.entanglement_factories_end()
            );
        }
    } else {
        // Grid
        j["type"] = "grid";
        j["coord"] = {grid.GetMaxX(), grid.GetMaxY(), grid.GetMinZ(), grid.GetMaxZ()};

        auto ban_array = Json::array();
        auto qubit_array = Json::array();
        auto magic_factory_array = Json::array();
        auto entanglement_factory_array = Json::array();

        const auto append_json_symbol = [](Json& array,
                                           const ScLsPlane& plane,
                                           const auto begin,
                                           const auto end) {
            for (auto itr = begin; itr != end; ++itr) {
                const auto& [coord, symbol] = *itr;
                auto tmp = Json();
                tmp["symbol"] = symbol.Id();
                if constexpr (std::is_same_v<ESymbol, std::remove_cvref_t<decltype(symbol)>>) {
                    tmp["pair"] = plane.GetPair(symbol).Id();
                }
                tmp["coord"] = Json::array({coord.x, coord.y, plane.GetZ()});
                array.emplace_back(tmp);
            }
        };
        for (const auto& plane : grid) {
            for (auto itr = plane.bans_begin(); itr != plane.bans_end(); ++itr) {
                const auto& coord = *itr;
                ban_array.emplace_back(Json::array({coord.x, coord.y, plane.GetZ()}));
            }
            append_json_symbol(qubit_array, plane, plane.qubits_begin(), plane.qubits_end());
            append_json_symbol(
                    magic_factory_array,
                    plane,
                    plane.magic_factories_begin(),
                    plane.magic_factories_end()
            );
            append_json_symbol(
                    entanglement_factory_array,
                    plane,
                    plane.entanglement_factories_begin(),
                    plane.entanglement_factories_end()
            );
        }
        std::sort(ban_array.begin(), ban_array.end());
        std::sort(qubit_array.begin(), qubit_array.end());
        std::sort(magic_factory_array.begin(), magic_factory_array.end());
        std::sort(entanglement_factory_array.begin(), entanglement_factory_array.end());

        if (ban_array.empty()) {
            j.erase("ban");
        } else {
            j["ban"] = std::move(ban_array);
        }
        if (qubit_array.empty()) {
            j.erase("qubit");
        } else {
            j["qubit"] = std::move(qubit_array);
        }
        if (magic_factory_array.empty()) {
            j.erase("magic_factory");
        } else {
            j["magic_factory"] = std::move(magic_factory_array);
        }
        if (entanglement_factory_array.empty()) {
            j.erase("entanglement_factory");
        } else {
            j["entanglement_factory"] = std::move(entanglement_factory_array);
        }
    }
}
void to_json(Json& j, const Topology& topology) {
    j = Json::array();
    for (const auto& grid : topology) {
        j.emplace_back(grid);
    }
}
}  // namespace qret::sc_ls_fixed_v0
