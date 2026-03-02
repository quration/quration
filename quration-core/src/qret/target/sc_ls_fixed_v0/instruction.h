/**
 * @file qret/target/sc_ls_fixed_v0/instruction.h
 * @brief Instruction.
 */

#ifndef QRET_TARGET_SC_LS_FIXED_V0_INSTRUCTION_H
#define QRET_TARGET_SC_LS_FIXED_V0_INSTRUCTION_H

#include <fmt/format.h>
#include <nlohmann/adl_serializer.hpp>

#include <cstdint>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "qret/base/json.h"
#include "qret/base/tribool.h"
#include "qret/codegen/machine_function.h"
#include "qret/qret_export.h"
#include "qret/target/sc_ls_fixed_v0/beat.h"
#include "qret/target/sc_ls_fixed_v0/pauli.h"
#include "qret/target/sc_ls_fixed_v0/sc_ls_fixed_v0_target_machine.h"
#include "qret/target/sc_ls_fixed_v0/symbol.h"
#include "qret/target/sc_ls_fixed_v0/topology.h"

namespace qret::sc_ls_fixed_v0 {
template <class T>
concept DerivedFromScLsInstruction = std::derived_from<T, class ScLsInstructionBase>;

enum class QRET_EXPORT ScLsInstructionType : std::uint8_t {
    // Cell allocation and deallocation
    ALLOCATE = 0,
    ALLOCATE_MAGIC_FACTORY,
    ALLOCATE_ENTANGLEMENT_FACTORY,
    DEALLOCATE,

    // Initialization
    INIT_ZX,

    // Measurement
    MEAS_ZX,
    MEAS_Y,

    // Unary operation
    TWIST,
    HADAMARD,
    ROTATE,

    // Lattice surgery
    LATTICE_SURGERY,
    LATTICE_SURGERY_MAGIC,
    LATTICE_SURGERY_MULTINODE,

    // Movement
    MOVE,
    MOVE_MAGIC,
    MOVE_ENTANGLEMENT,

    // Special instructions
    CNOT,

    // 3D
    CNOT_TRANS,
    SWAP_TRANS,
    MOVE_TRANS,

    // Classical
    XOR,
    AND,
    OR,

    // Others
    PROBABILITY_HINT,
    AWAIT_CORRECTION,
};
std::string ToString(ScLsInstructionType);
std::ostream& operator<<(std::ostream&, ScLsInstructionType);
ScLsInstructionType ScLsInstructionTypeFromString(const std::string&);

static inline const std::
        unordered_map<ScLsFixedV0MachineType, std::unordered_set<ScLsInstructionType>>
                SupportedInstructionType = {
                        {ScLsFixedV0MachineType::Dim2,
                         {
                                 ScLsInstructionType::ALLOCATE,
                                 ScLsInstructionType::ALLOCATE_MAGIC_FACTORY,
                                 ScLsInstructionType::DEALLOCATE,
                                 ScLsInstructionType::INIT_ZX,
                                 ScLsInstructionType::MEAS_ZX,
                                 ScLsInstructionType::MEAS_Y,
                                 ScLsInstructionType::TWIST,
                                 ScLsInstructionType::HADAMARD,
                                 ScLsInstructionType::ROTATE,
                                 ScLsInstructionType::LATTICE_SURGERY,
                                 ScLsInstructionType::LATTICE_SURGERY_MAGIC,
                                 ScLsInstructionType::MOVE,
                                 ScLsInstructionType::MOVE_MAGIC,
                                 ScLsInstructionType::CNOT,
                                 ScLsInstructionType::XOR,
                                 ScLsInstructionType::AND,
                                 ScLsInstructionType::OR,
                                 ScLsInstructionType::PROBABILITY_HINT,
                                 ScLsInstructionType::AWAIT_CORRECTION,
                         }},
                        {ScLsFixedV0MachineType::Dim3,
                         {
                                 ScLsInstructionType::ALLOCATE,
                                 ScLsInstructionType::ALLOCATE_MAGIC_FACTORY,
                                 ScLsInstructionType::DEALLOCATE,
                                 ScLsInstructionType::INIT_ZX,
                                 ScLsInstructionType::MEAS_ZX,
                                 ScLsInstructionType::MEAS_Y,
                                 ScLsInstructionType::TWIST,
                                 ScLsInstructionType::HADAMARD,
                                 ScLsInstructionType::ROTATE,
                                 ScLsInstructionType::LATTICE_SURGERY,
                                 ScLsInstructionType::LATTICE_SURGERY_MAGIC,
                                 ScLsInstructionType::MOVE,
                                 ScLsInstructionType::MOVE_MAGIC,
                                 ScLsInstructionType::CNOT,
                                 ScLsInstructionType::CNOT_TRANS,
                                 ScLsInstructionType::SWAP_TRANS,
                                 ScLsInstructionType::MOVE_TRANS,
                                 ScLsInstructionType::XOR,
                                 ScLsInstructionType::AND,
                                 ScLsInstructionType::OR,
                                 ScLsInstructionType::PROBABILITY_HINT,
                                 ScLsInstructionType::AWAIT_CORRECTION,
                         }},
                        {ScLsFixedV0MachineType::DistributedDim2,
                         {
                                 ScLsInstructionType::ALLOCATE,
                                 ScLsInstructionType::ALLOCATE_MAGIC_FACTORY,
                                 ScLsInstructionType::ALLOCATE_ENTANGLEMENT_FACTORY,
                                 ScLsInstructionType::DEALLOCATE,
                                 ScLsInstructionType::INIT_ZX,
                                 ScLsInstructionType::MEAS_ZX,
                                 ScLsInstructionType::MEAS_Y,
                                 ScLsInstructionType::TWIST,
                                 ScLsInstructionType::HADAMARD,
                                 ScLsInstructionType::ROTATE,
                                 ScLsInstructionType::LATTICE_SURGERY_MULTINODE,
                                 ScLsInstructionType::MOVE,
                                 ScLsInstructionType::MOVE_MAGIC,
                                 ScLsInstructionType::MOVE_ENTANGLEMENT,
                                 ScLsInstructionType::CNOT,
                                 ScLsInstructionType::XOR,
                                 ScLsInstructionType::AND,
                                 ScLsInstructionType::OR,
                                 ScLsInstructionType::PROBABILITY_HINT,
                                 ScLsInstructionType::AWAIT_CORRECTION,
                         }},
};

struct QRET_EXPORT ScLsMetadata {
    Beat beat = 0;
    std::int32_t z = 0;
};
inline void to_json(Json& j, const ScLsMetadata& metadata) {
    j["beat"] = metadata.beat;
    j["z"] = metadata.z;
}
inline void from_json(const Json& j, ScLsMetadata& metadata) {
    if (j.contains("beat")) {
        j.at("beat").get_to(metadata.beat);
    }
    if (j.contains("z")) {
        j.at("z").get_to(metadata.z);
    }
}

/**
 * @brief Base class of SC_LS_FIXED_V0 instructions.
 */
class QRET_EXPORT ScLsInstructionBase : public MachineInstruction {
public:
    // virtual methods

    [[nodiscard]] virtual bool IsValidFormat() const = 0;
    [[nodiscard]] virtual ScLsInstructionType Type() const = 0;
    [[nodiscard]] virtual Beat Latency() const = 0;
    [[nodiscard]] virtual Beat StartCorrecting() const = 0;
    [[nodiscard]] std::string ToString() const override = 0;
    [[nodiscard]] virtual std::string ToStringWithMD() const;
    [[nodiscard]] virtual Json ToJson() const = 0;

    static std::unique_ptr<ScLsInstructionBase> FromJson(const Json& j);

    // access to operands

    [[nodiscard]] virtual const std::list<QSymbol>& QTarget() const {
        static const auto empty = std::list<QSymbol>();
        return empty;
    }
    [[nodiscard]] virtual const std::list<CSymbol>& CCreate() const {
        static const auto empty = std::list<CSymbol>();
        return empty;
    }
    [[nodiscard]] virtual const std::list<CSymbol>& CDepend() const {
        static const auto empty = std::list<CSymbol>();
        return empty;
    }
    [[nodiscard]] virtual const std::list<MSymbol>& MTarget() const {
        static const auto empty = std::list<MSymbol>();
        return empty;
    }
    [[nodiscard]] virtual const std::list<ESymbol>& ETarget() const {
        static const auto empty = std::list<ESymbol>();
        return empty;
    }
    [[nodiscard]] virtual const std::list<EHandle>& EHTarget() const {
        static const auto empty = std::list<EHandle>();
        return empty;
    }
    [[nodiscard]] virtual const std::list<Coord3D>& Ancilla() const {
        static const auto empty = std::list<Coord3D>();
        return empty;
    }
    [[nodiscard]] const std::list<CSymbol>& Condition() const {
        return condition_list_;
    }
    void SetCondition(const std::list<CSymbol>& new_condition_list) {
        condition_list_ = new_condition_list;
    }
    void SetCondition(std::list<CSymbol>&& new_condition_list) {
        condition_list_ = std::move(new_condition_list);
    }
    [[nodiscard]] const ScLsMetadata& Metadata() const {
        return metadata_;
    }
    [[nodiscard]] ScLsMetadata& MetadataMut() {
        return metadata_;
    }

    // access to properties

    [[nodiscard]] virtual bool IsMeasurement() const {
        return false;
    }
    [[nodiscard]] virtual bool IsClassical() const {
        return false;
    }
    [[nodiscard]] virtual bool IsOptHint() const {
        return false;
    }

    [[nodiscard]] virtual std::size_t CountAncillae() const {
        return Ancilla().size();
    }
    [[nodiscard]] virtual bool UseAncilla() const {
        return !Ancilla().empty();
    }
    [[nodiscard]] virtual bool UseMagicState() const {
        return !MTarget().empty();
    }
    [[nodiscard]] virtual std::size_t CountEntanglement() const {
        return ETarget().size();
    }
    [[nodiscard]] virtual bool UseEntanglement() const {
        return !ETarget().empty();
    }

    /**
     * @brief Return if the given object is the instance of this class.
     * @details this is used from LLVM-style cast functions defined in base/cast.h
     */
#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromScLsInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From*) {
        return true;
    }

private:
    std::list<CSymbol> condition_list_;
    ScLsMetadata metadata_;

protected:
    explicit ScLsInstructionBase(const std::list<CSymbol>& condition_list)
        : condition_list_{condition_list} {}

    // Print

    template <typename T>
    static void PrintList(std::ostream& out, const std::list<T>& l) {
        out << '[';
        for (auto itr = l.begin(); itr != l.end(); ++itr) {
            if (itr != l.begin()) {
                out << " ";
            }
            out << *itr;
        }
        out << ']';
    }
    void PrintConditionIfExists(std::ostream& out) const {
        if (!Condition().empty()) {
            out << ' ';
            PrintList(out, Condition());
        }
    }
    void PrintMetadata(std::ostream& out) const {
        out << " [beat: " << metadata_.beat << ", z: " << metadata_.z << "]";
    }

    // Json

    [[nodiscard]] Json DefaultJson() const {
        auto j = Json();
        j["type"] = ::qret::sc_ls_fixed_v0::ToString(Type());
        j["qtarget"] = QTarget();
        j["ccreate"] = CCreate();
        j["cdepend"] = CDepend();
        j["mtarget"] = MTarget();
        j["etarget"] = ETarget();
        j["ehtarget"] = EHTarget();
        j["condition"] = Condition();
        j["ancilla"] = Ancilla();
        j["metadata"] = Metadata();
        j["raw"] = ToString();
        return j;
    }

    template <typename T>
    [[nodiscard]] static std::list<T> JsonToT(const Json& j, const std::string& key) {
        auto ret = std::list<T>();
        if (j.contains(key)) {
            for (const auto& x : j[key]) {
                ret.emplace_back(x.template get<T>());
            }
        }
        return ret;
    }

    void SetMetadata(const Json& j) {
        if (j.contains("metadata")) {
            from_json(j.at("metadata"), MetadataMut());
        }
    }
};

//--------------------------------------------------//
// Implement individual instructions.
//--------------------------------------------------//

// TODO: write documentation

class QRET_EXPORT Allocate : public ScLsInstructionBase {
public:
    [[nodiscard]] static std::unique_ptr<Allocate>
    New(QSymbol qubit, const Coord3D& dest, std::uint32_t dir, const std::list<CSymbol>& condition
    ) {
        return std::unique_ptr<Allocate>(new Allocate(qubit, dest, dir, condition));
    }

    [[nodiscard]] QSymbol Qubit() const {
        return QTarget().front();
    }
    [[nodiscard]] const Coord3D& Dest() const {
        return dest_;
    }
    void SetDest(const Coord3D& dest) {
        dest_ = dest;
    }
    [[nodiscard]] std::uint32_t Dir() const {
        return dir_;
    }
    void SetDir(const std::uint32_t dir) {
        dir_ = dir;
    }

    // virtual methods

    [[nodiscard]] bool IsValidFormat() const override {
        return QTarget().size() == 1 && Dir() < 2;
    }
    [[nodiscard]] ScLsInstructionType Type() const override {
        return ScLsInstructionType::ALLOCATE;
    }
    [[nodiscard]] Beat Latency() const override {
        return 0;
    }
    [[nodiscard]] Beat StartCorrecting() const override {
        return 0;
    }
    [[nodiscard]] std::string ToString() const override;
    [[nodiscard]] Json ToJson() const override;
    [[nodiscard]] static std::unique_ptr<Allocate> FromJson(const Json&);

    // override access to operands

    [[nodiscard]] const std::list<QSymbol>& QTarget() const override {
        return q_;
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromScLsInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->Type() == ScLsInstructionType::ALLOCATE;
    }

private:
    Allocate(
            QSymbol qubit,
            const Coord3D& dest,
            std::uint32_t dir,
            const std::list<CSymbol>& condition
    )
        : ScLsInstructionBase(condition)
        , q_{qubit}
        , dest_{dest}
        , dir_{dir} {}
    std::list<QSymbol> q_;
    Coord3D dest_;
    std::uint32_t dir_;
};
class QRET_EXPORT AllocateMagicFactory : public ScLsInstructionBase {
public:
    [[nodiscard]] static std::unique_ptr<AllocateMagicFactory>
    New(MSymbol magic_factory, const Coord3D& dest, const std::list<CSymbol>& condition) {
        return std::unique_ptr<AllocateMagicFactory>(
                new AllocateMagicFactory(magic_factory, dest, condition)
        );
    }

    [[nodiscard]] MSymbol MagicFactory() const {
        return MTarget().front();
    }
    [[nodiscard]] const Coord3D& Dest() const {
        return dest_;
    }
    void SetDest(const Coord3D& dest) {
        dest_ = dest;
    }

    // virtual methods

    [[nodiscard]] bool IsValidFormat() const override {
        return MTarget().size() == 1;
    }
    [[nodiscard]] ScLsInstructionType Type() const override {
        return ScLsInstructionType::ALLOCATE_MAGIC_FACTORY;
    }
    [[nodiscard]] Beat Latency() const override {
        return 0;
    }
    [[nodiscard]] Beat StartCorrecting() const override {
        return 0;
    }
    [[nodiscard]] std::string ToString() const override;
    [[nodiscard]] Json ToJson() const override;
    [[nodiscard]] static std::unique_ptr<AllocateMagicFactory> FromJson(const Json&);

    // override access to operands

    [[nodiscard]] const std::list<MSymbol>& MTarget() const override {
        return m_;
    }

    // access to properties

    [[nodiscard]] bool UseMagicState() const override {
        return false;
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromScLsInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->Type() == ScLsInstructionType::ALLOCATE_MAGIC_FACTORY;
    }

private:
    AllocateMagicFactory(
            MSymbol magic_factory,
            const Coord3D& dest,
            const std::list<CSymbol>& condition
    )
        : ScLsInstructionBase(condition)
        , m_{magic_factory}
        , dest_{dest} {}
    std::list<MSymbol> m_;
    Coord3D dest_;
};
class QRET_EXPORT AllocateEntanglementFactory : public ScLsInstructionBase {
public:
    [[nodiscard]] static std::unique_ptr<AllocateEntanglementFactory> New(
            ESymbol entanglement_factory1,
            const Coord3D& dest1,
            ESymbol entanglement_factory2,
            const Coord3D& dest2,
            const std::list<CSymbol>& condition
    ) {
        return std::unique_ptr<AllocateEntanglementFactory>(new AllocateEntanglementFactory(
                entanglement_factory1,
                dest1,
                entanglement_factory2,
                dest2,
                condition
        ));
    }

    [[nodiscard]] ESymbol EntanglementFactory1() const {
        return ETarget().front();
    }
    [[nodiscard]] ESymbol EntanglementFactory2() const {
        return ETarget().back();
    }
    [[nodiscard]] const Coord3D& Dest1() const {
        return dest1_;
    }
    [[nodiscard]] const Coord3D& Dest2() const {
        return dest2_;
    }
    void SetDest1(const Coord3D& dest) {
        dest1_ = dest;
    }
    void SetDest2(const Coord3D& dest) {
        dest2_ = dest;
    }

    // virtual methods

    [[nodiscard]] bool IsValidFormat() const override {
        return ETarget().size() == 1;
    }
    [[nodiscard]] ScLsInstructionType Type() const override {
        return ScLsInstructionType::ALLOCATE_ENTANGLEMENT_FACTORY;
    }
    [[nodiscard]] Beat Latency() const override {
        return 0;
    }
    [[nodiscard]] Beat StartCorrecting() const override {
        return 0;
    }
    [[nodiscard]] std::string ToString() const override;
    [[nodiscard]] Json ToJson() const override;
    [[nodiscard]] static std::unique_ptr<AllocateEntanglementFactory> FromJson(const Json&);

    // override access to operands

    [[nodiscard]] const std::list<ESymbol>& ETarget() const override {
        return e_;
    }

    [[nodiscard]] std::size_t CountEntanglement() const override {
        return 0;
    }
    [[nodiscard]] bool UseEntanglement() const override {
        return false;
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromScLsInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->Type() == ScLsInstructionType::ALLOCATE_ENTANGLEMENT_FACTORY;
    }

private:
    AllocateEntanglementFactory(
            ESymbol entanglement_factory1,
            const Coord3D& dest1,
            ESymbol entanglement_factory2,
            const Coord3D& dest2,
            const std::list<CSymbol>& condition
    )
        : ScLsInstructionBase(condition)
        , e_{entanglement_factory1, entanglement_factory2}
        , dest1_{dest1}
        , dest2_{dest2} {}
    std::list<ESymbol> e_;
    Coord3D dest1_;
    Coord3D dest2_;
};
class QRET_EXPORT DeAllocate : public ScLsInstructionBase {
public:
    static std::unique_ptr<DeAllocate> New(QSymbol qubit, const std::list<CSymbol>& condition) {
        return std::unique_ptr<DeAllocate>(new DeAllocate(qubit, condition));
    }

    [[nodiscard]] QSymbol Qubit() const {
        return QTarget().front();
    }

    // virtual methods

    [[nodiscard]] bool IsValidFormat() const override {
        return QTarget().size() == 1;
    }
    [[nodiscard]] ScLsInstructionType Type() const override {
        return ScLsInstructionType::DEALLOCATE;
    }
    [[nodiscard]] Beat Latency() const override {
        return 0;
    }
    [[nodiscard]] Beat StartCorrecting() const override {
        return 0;
    }
    [[nodiscard]] std::string ToString() const override;
    [[nodiscard]] Json ToJson() const override;
    [[nodiscard]] static std::unique_ptr<DeAllocate> FromJson(const Json& j);

    // override access to operands

    [[nodiscard]] const std::list<QSymbol>& QTarget() const override {
        return q_;
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromScLsInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->Type() == ScLsInstructionType::DEALLOCATE;
    }

private:
    DeAllocate(QSymbol qubit, const std::list<CSymbol>& condition)
        : ScLsInstructionBase(condition)
        , q_{qubit} {}
    std::list<QSymbol> q_;
};
class QRET_EXPORT InitZX : public ScLsInstructionBase {
public:
    static std::unique_ptr<InitZX>
    New(QSymbol qubit, std::uint32_t zx, const std::list<CSymbol>& condition) {
        return std::unique_ptr<InitZX>(new InitZX(qubit, zx, condition));
    }

    [[nodiscard]] QSymbol Qubit() const {
        return QTarget().front();
    }
    [[nodiscard]] std::uint32_t ZX() const {
        return zx_;
    }
    void SetZX(std::uint32_t zx) {
        zx_ = zx;
    }
    [[nodiscard]] bool IsZ() const {
        return ZX() == 0;
    }
    [[nodiscard]] bool IsX() const {
        return ZX() == 1;
    }

    // virtual methods

    [[nodiscard]] bool IsValidFormat() const override {
        return QTarget().size() == 1 && ZX() < 2;
    }
    [[nodiscard]] ScLsInstructionType Type() const override {
        return ScLsInstructionType::INIT_ZX;
    }
    [[nodiscard]] Beat Latency() const override {
        return 0;
    }
    [[nodiscard]] Beat StartCorrecting() const override {
        return 0;
    }
    [[nodiscard]] std::string ToString() const override;
    [[nodiscard]] Json ToJson() const override;
    [[nodiscard]] static std::unique_ptr<InitZX> FromJson(const Json& j);

    // override access to operands

    [[nodiscard]] const std::list<QSymbol>& QTarget() const override {
        return q_;
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromScLsInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->Type() == ScLsInstructionType::INIT_ZX;
    }

private:
    InitZX(QSymbol qubit, std::uint32_t zx, const std::list<CSymbol>& condition)
        : ScLsInstructionBase(condition)
        , q_{qubit}
        , zx_{zx} {}
    std::list<QSymbol> q_;
    std::uint32_t zx_;
};
class QRET_EXPORT MeasZX : public ScLsInstructionBase {
public:
    static constexpr auto Z = std::uint32_t{0};
    static constexpr auto X = std::uint32_t{1};

    static std::unique_ptr<MeasZX>
    New(QSymbol qubit, std::uint32_t zx, CSymbol cdest, const std::list<CSymbol>& condition) {
        return std::unique_ptr<MeasZX>(new MeasZX(qubit, zx, cdest, condition));
    }

    [[nodiscard]] QSymbol Qubit() const {
        return QTarget().front();
    }
    [[nodiscard]] CSymbol CDest() const {
        return CCreate().front();
    }
    [[nodiscard]] std::uint32_t ZX() const {
        return zx_;
    }
    void SetZX(std::uint32_t zx) {
        zx_ = zx;
    }
    [[nodiscard]] bool IsZ() const {
        return zx_ == Z;
    }
    [[nodiscard]] bool IsX() const {
        return zx_ == X;
    }

    // virtual methods

    [[nodiscard]] bool IsValidFormat() const override {
        return QTarget().size() == 1 && CCreate().size() == 1 && ZX() < 2;
    }
    [[nodiscard]] ScLsInstructionType Type() const override {
        return ScLsInstructionType::MEAS_ZX;
    }
    [[nodiscard]] Beat Latency() const override {
        return 0;
    }
    [[nodiscard]] Beat StartCorrecting() const override {
        return 0;
    }
    [[nodiscard]] std::string ToString() const override;
    [[nodiscard]] Json ToJson() const override;
    [[nodiscard]] static std::unique_ptr<MeasZX> FromJson(const Json& j);

    // override access to operands

    [[nodiscard]] const std::list<QSymbol>& QTarget() const override {
        return q_;
    }
    [[nodiscard]] const std::list<CSymbol>& CCreate() const override {
        return c_;
    }

    // access to properties

    [[nodiscard]] bool IsMeasurement() const override {
        return true;
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromScLsInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->Type() == ScLsInstructionType::MEAS_ZX;
    }

private:
    MeasZX(QSymbol qubit, std::uint32_t zx, CSymbol cdest, const std::list<CSymbol>& condition)
        : ScLsInstructionBase(condition)
        , q_{qubit}
        , c_{cdest}
        , zx_{zx} {}
    std::list<QSymbol> q_;
    std::list<CSymbol> c_;
    std::uint32_t zx_;
};
class QRET_EXPORT MeasY : public ScLsInstructionBase {
public:
    static std::unique_ptr<MeasY>
    New(QSymbol qubit, std::uint32_t dir, CSymbol cdest, const std::list<CSymbol>& condition) {
        return std::unique_ptr<MeasY>(new MeasY(qubit, dir, cdest, condition));
    }
    static std::array<Coord3D, 3> Ancillae(const Coord3D& coord, std::uint32_t dir);

    [[nodiscard]] QSymbol Qubit() const {
        return QTarget().front();
    }
    [[nodiscard]] std::uint32_t Dir() const {
        return dir_;
    }
    void SetDir(std::uint32_t dir) {
        dir_ = dir;
    }
    [[nodiscard]] CSymbol CDest() const {
        return CCreate().front();
    }

    // virtual methods

    [[nodiscard]] bool IsValidFormat() const override {
        return QTarget().size() == 1 && CCreate().size() == 1 && Dir() < 4;
    }
    [[nodiscard]] ScLsInstructionType Type() const override {
        return ScLsInstructionType::MEAS_Y;
    }
    [[nodiscard]] Beat Latency() const override {
        return 2;
    }
    [[nodiscard]] Beat StartCorrecting() const override {
        return 1;
    }
    [[nodiscard]] std::string ToString() const override;
    [[nodiscard]] Json ToJson() const override;
    [[nodiscard]] static std::unique_ptr<MeasY> FromJson(const Json& j);

    // override access to operands

    [[nodiscard]] const std::list<QSymbol>& QTarget() const override {
        return q_;
    }
    [[nodiscard]] const std::list<CSymbol>& CCreate() const override {
        return c_;
    }

    // access to properties

    [[nodiscard]] bool IsMeasurement() const override {
        return true;
    }
    [[nodiscard]] std::size_t CountAncillae() const override {
        return 3;
    }
    [[nodiscard]] bool UseAncilla() const override {
        return true;
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromScLsInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->Type() == ScLsInstructionType::MEAS_Y;
    }

private:
    MeasY(QSymbol qubit, std::uint32_t dir, CSymbol cdest, const std::list<CSymbol>& condition)
        : ScLsInstructionBase(condition)
        , q_{qubit}
        , c_{cdest}
        , dir_{dir} {}
    std::list<QSymbol> q_;
    std::list<CSymbol> c_;
    std::uint32_t dir_;
};
class QRET_EXPORT Twist : public ScLsInstructionBase {
public:
    static std::unique_ptr<Twist>
    New(QSymbol qubit, std::uint32_t dir, const std::list<CSymbol>& condition) {
        return std::unique_ptr<Twist>(new Twist(qubit, dir, condition));
    }
    static std::vector<std::uint32_t> PossibleDirections(std::uint32_t qubit_dir, Pauli pauli);
    static Coord3D GetAncilla(const Coord3D& coord, std::uint32_t dir);

    [[nodiscard]] QSymbol Qubit() const {
        return QTarget().front();
    }
    [[nodiscard]] std::uint32_t Dir() const {
        return dir_;
    }
    void SetDir(std::uint32_t dir) {
        dir_ = dir;
    }

    // metadata
    std::optional<Pauli> GetTargetPauli() const {
        return pauli_;
    }
    void SetTargetPauli(std::optional<Pauli> pauli) {
        pauli_ = pauli;
    }

    // virtual methods

    [[nodiscard]] bool IsValidFormat() const override {
        return QTarget().size() == 1 && Dir() < 4;
    }
    [[nodiscard]] ScLsInstructionType Type() const override {
        return ScLsInstructionType::TWIST;
    }
    [[nodiscard]] Beat Latency() const override {
        return 2;
    }
    [[nodiscard]] Beat StartCorrecting() const override {
        return 0;
    }
    [[nodiscard]] std::string ToString() const override;
    [[nodiscard]] Json ToJson() const override;
    [[nodiscard]] static std::unique_ptr<Twist> FromJson(const Json& j);

    // override access to operands

    [[nodiscard]] const std::list<QSymbol>& QTarget() const override {
        return q_;
    }

    // access to properties

    [[nodiscard]] std::size_t CountAncillae() const override {
        return 1;
    }
    [[nodiscard]] bool UseAncilla() const override {
        return true;
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromScLsInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->Type() == ScLsInstructionType::TWIST;
    }

private:
    Twist(QSymbol qubit, std::uint32_t dir, const std::list<CSymbol>& condition)
        : ScLsInstructionBase(condition)
        , q_{qubit}
        , dir_{dir} {}
    std::list<QSymbol> q_;
    std::uint32_t dir_;
    std::optional<Pauli> pauli_ = std::nullopt;  // z or x
};
class QRET_EXPORT Hadamard : public ScLsInstructionBase {
public:
    static std::unique_ptr<Hadamard> New(QSymbol qubit, const std::list<CSymbol>& condition) {
        return std::unique_ptr<Hadamard>(new Hadamard(qubit, condition));
    }

    [[nodiscard]] QSymbol Qubit() const {
        return QTarget().front();
    }

    // virtual methods

    [[nodiscard]] bool IsValidFormat() const override {
        return QTarget().size() == 1;
    }
    [[nodiscard]] ScLsInstructionType Type() const override {
        return ScLsInstructionType::HADAMARD;
    }
    [[nodiscard]] Beat Latency() const override {
        return 0;
    }
    [[nodiscard]] Beat StartCorrecting() const override {
        return 0;
    }
    [[nodiscard]] std::string ToString() const override;
    [[nodiscard]] Json ToJson() const override;
    [[nodiscard]] static std::unique_ptr<Hadamard> FromJson(const Json& j);

    // override access to operands

    [[nodiscard]] const std::list<QSymbol>& QTarget() const override {
        return q_;
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromScLsInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->Type() == ScLsInstructionType::HADAMARD;
    }

private:
    Hadamard(QSymbol qubit, const std::list<CSymbol>& condition)
        : ScLsInstructionBase(condition)
        , q_{qubit} {}
    std::list<QSymbol> q_;
};
class QRET_EXPORT Rotate : public ScLsInstructionBase {
public:
    static std::unique_ptr<Rotate>
    New(QSymbol qubit, std::uint32_t dir, const std::list<CSymbol>& condition) {
        return std::unique_ptr<Rotate>(new Rotate(qubit, dir, condition));
    }
    static Coord3D Ancilla(const Coord3D& coord, std::uint32_t dir);

    [[nodiscard]] QSymbol Qubit() const {
        return QTarget().front();
    }
    [[nodiscard]] std::uint32_t Dir() const {
        return dir_;
    }
    void SetDir(std::uint32_t dir) {
        dir_ = dir;
    }

    // virtual methods

    [[nodiscard]] bool IsValidFormat() const override {
        return QTarget().size() == 1 && Dir() < 4;
    }
    [[nodiscard]] ScLsInstructionType Type() const override {
        return ScLsInstructionType::ROTATE;
    }
    [[nodiscard]] Beat Latency() const override {
        return 3;
    }
    [[nodiscard]] Beat StartCorrecting() const override {
        return 0;
    }
    [[nodiscard]] std::string ToString() const override;
    [[nodiscard]] Json ToJson() const override;
    [[nodiscard]] static std::unique_ptr<Rotate> FromJson(const Json& j);

    // override access to operands

    [[nodiscard]] const std::list<QSymbol>& QTarget() const override {
        return q_;
    }

    // access to properties

    [[nodiscard]] std::size_t CountAncillae() const override {
        return 1;
    }
    [[nodiscard]] bool UseAncilla() const override {
        return true;
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromScLsInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->Type() == ScLsInstructionType::ROTATE;
    }

private:
    Rotate(QSymbol qubit, std::uint32_t dir, const std::list<CSymbol>& condition)
        : ScLsInstructionBase(condition)
        , q_{qubit}
        , dir_{dir} {}
    std::list<QSymbol> q_;
    std::uint32_t dir_;
};
class QRET_EXPORT LatticeSurgery : public ScLsInstructionBase {
public:
    static std::unique_ptr<LatticeSurgery> New(
            const std::list<QSymbol>& qubit_list,
            const std::list<Pauli>& basis_list,
            const std::list<Coord3D>& path,
            CSymbol cdest,
            const std::list<CSymbol>& condition
    ) {
        return std::unique_ptr<LatticeSurgery>(
                new LatticeSurgery(qubit_list, basis_list, path, cdest, condition)
        );
    }

    [[nodiscard]] const std::list<QSymbol>& QubitList() const {
        return QTarget();
    }
    void SetQubitList(const std::list<QSymbol>& qubit_list) {
        q_ = qubit_list;
    }
    void SetQubitList(std::list<QSymbol>&& qubit_list) {
        q_ = std::move(qubit_list);
    }
    [[nodiscard]] const std::list<Pauli>& BasisList() const {
        return basis_list_;
    }
    [[nodiscard]] const std::list<Coord3D>& Path() const {
        return Ancilla();
    }
    void SetPath(const std::list<Coord3D>& path) {
        ancilla_ = path;
    }
    void SetPath(std::list<Coord3D>&& path) {
        ancilla_ = std::move(path);
    }
    [[nodiscard]] CSymbol CDest() const {
        return CCreate().front();
    }

    // virtual methods

    [[nodiscard]] bool IsValidFormat() const override {
        return QubitList().size() == BasisList().size() && CCreate().size() == 1;
    }
    [[nodiscard]] ScLsInstructionType Type() const override {
        return ScLsInstructionType::LATTICE_SURGERY;
    }
    [[nodiscard]] Beat Latency() const override {
        return 1;
    }
    [[nodiscard]] Beat StartCorrecting() const override {
        return 0;
    }
    [[nodiscard]] std::string ToString() const override;
    [[nodiscard]] Json ToJson() const override;
    [[nodiscard]] static std::unique_ptr<LatticeSurgery> FromJson(const Json& j);

    // override access to operands

    [[nodiscard]] const std::list<QSymbol>& QTarget() const override {
        return q_;
    }
    [[nodiscard]] const std::list<CSymbol>& CCreate() const override {
        return c_;
    }
    [[nodiscard]] const std::list<Coord3D>& Ancilla() const override {
        return ancilla_;
    }

    // access to properties

    [[nodiscard]] bool IsMeasurement() const override {
        return true;
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromScLsInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->Type() == ScLsInstructionType::LATTICE_SURGERY;
    }

private:
    LatticeSurgery(
            const std::list<QSymbol>& qubit_list,
            const std::list<Pauli>& basis_list,
            const std::list<Coord3D>& path,
            CSymbol cdest,
            const std::list<CSymbol>& condition
    )
        : ScLsInstructionBase(condition)
        , q_(qubit_list)
        , basis_list_{basis_list}
        , ancilla_(path)
        , c_{cdest} {}
    std::list<QSymbol> q_;
    std::list<Pauli> basis_list_;
    std::list<Coord3D> ancilla_;
    std::list<CSymbol> c_;
};
class QRET_EXPORT LatticeSurgeryMagic : public ScLsInstructionBase {
public:
    static std::unique_ptr<LatticeSurgeryMagic> New(
            const std::list<QSymbol>& qubit_list,
            const std::list<Pauli>& basis_list,
            const std::list<Coord3D>& path,
            CSymbol cdest,
            MSymbol magic_factory,
            const std::list<CSymbol>& condition
    ) {
        return std::unique_ptr<LatticeSurgeryMagic>(new LatticeSurgeryMagic(
                qubit_list,
                basis_list,
                path,
                cdest,
                magic_factory,
                condition
        ));
    }

    [[nodiscard]] const std::list<QSymbol>& QubitList() const {
        return QTarget();
    }
    void SetQubitList(const std::list<QSymbol>& qubit_list) {
        q_ = qubit_list;
    }
    void SetQubitList(std::list<QSymbol>&& qubit_list) {
        q_ = std::move(qubit_list);
    }
    [[nodiscard]] const std::list<Pauli>& BasisList() const {
        return basis_list_;
    }
    [[nodiscard]] const std::list<Coord3D>& Path() const {
        return Ancilla();
    }
    void SetPath(const std::list<Coord3D>& path) {
        ancilla_ = path;
    }
    void SetPath(std::list<Coord3D>&& path) {
        ancilla_ = std::move(path);
    }
    [[nodiscard]] MSymbol MagicFactory() const {
        return MTarget().front();
    }
    void SetMagicFactory(MSymbol magic_factory) {
        m_ = {magic_factory};
    }
    [[nodiscard]] CSymbol CDest() const {
        return CCreate().front();
    }

    // virtual methods

    [[nodiscard]] bool IsValidFormat() const override {
        return QubitList().size() == BasisList().size() && CCreate().size() == 1
                && MTarget().size() == 1;
    }
    [[nodiscard]] ScLsInstructionType Type() const override {
        return ScLsInstructionType::LATTICE_SURGERY_MAGIC;
    }
    [[nodiscard]] Beat Latency() const override {
        return 1;
    }
    [[nodiscard]] Beat StartCorrecting() const override {
        return 0;
    }
    [[nodiscard]] std::string ToString() const override;
    [[nodiscard]] Json ToJson() const override;
    [[nodiscard]] static std::unique_ptr<LatticeSurgeryMagic> FromJson(const Json& j);

    // override access to operands

    [[nodiscard]] const std::list<QSymbol>& QTarget() const override {
        return q_;
    }
    [[nodiscard]] const std::list<CSymbol>& CCreate() const override {
        return c_;
    }
    [[nodiscard]] const std::list<MSymbol>& MTarget() const override {
        return m_;
    }
    [[nodiscard]] const std::list<Coord3D>& Ancilla() const override {
        return ancilla_;
    }

    // access to properties

    [[nodiscard]] bool IsMeasurement() const override {
        return true;
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromScLsInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->Type() == ScLsInstructionType::LATTICE_SURGERY_MAGIC;
    }

private:
    LatticeSurgeryMagic(
            const std::list<QSymbol>& qubit_list,
            const std::list<Pauli>& basis_list,
            const std::list<Coord3D>& path,
            CSymbol cdest,
            MSymbol magic_factory,
            const std::list<CSymbol>& condition
    )
        : ScLsInstructionBase(condition)
        , q_(qubit_list)
        , basis_list_{basis_list}
        , ancilla_(path)
        , c_{cdest}
        , m_{magic_factory} {}
    std::list<QSymbol> q_;
    std::list<Pauli> basis_list_;
    std::list<Coord3D> ancilla_;
    std::list<CSymbol> c_;
    std::list<MSymbol> m_;
};
class QRET_EXPORT LatticeSurgeryMultinode : public ScLsInstructionBase {
public:
    static std::unique_ptr<LatticeSurgeryMultinode> New(
            const std::list<QSymbol>& qubit_list,
            const std::list<Pauli>& basis_list,
            const std::list<Coord3D>& path,
            CSymbol cdest,
            const std::list<MSymbol>& m_factory,
            const std::list<ESymbol>& e_factory_list,
            const std::list<EHandle>& e_handle_list,
            const std::list<CSymbol>& condition
    ) {
        return std::unique_ptr<LatticeSurgeryMultinode>(new LatticeSurgeryMultinode(
                qubit_list,
                basis_list,
                path,
                cdest,
                m_factory,
                e_factory_list,
                e_handle_list,
                condition
        ));
    }

    [[nodiscard]] const std::list<QSymbol>& QubitList() const {
        return QTarget();
    }
    void SetQubitList(const std::list<QSymbol>& qubit_list) {
        q_ = qubit_list;
    }
    void SetQubitList(std::list<QSymbol>&& qubit_list) {
        q_ = std::move(qubit_list);
    }
    [[nodiscard]] const std::list<Pauli>& BasisList() const {
        return basis_list_;
    }
    [[nodiscard]] const std::list<Coord3D>& Path() const {
        return Ancilla();
    }
    void SetPath(const std::list<Coord3D>& path) {
        ancilla_ = path;
    }
    void SetPath(std::list<Coord3D>&& path) {
        ancilla_ = std::move(path);
    }
    [[nodiscard]] CSymbol CDest() const {
        return CCreate().front();
    }
    bool UseMagicFactory() const {
        return !MTarget().empty();
    }
    [[nodiscard]] MSymbol MagicFactory() const {
        return MTarget().front();
    }
    void SetMagicFactory(MSymbol magic_factory) {
        m_ = {magic_factory};
    }
    bool UseEFactory() const {
        return !ETarget().empty();
    }
    [[nodiscard]] const std::list<ESymbol>& EFactoryList() const {
        return ETarget();
    }
    void SetEFactoryList(const std::list<ESymbol>& e_factory_list) {
        e_ = e_factory_list;
    }
    void SetEFactoryList(std::list<ESymbol>&& e_factory_list) {
        e_ = std::move(e_factory_list);
    }
    [[nodiscard]] const std::list<EHandle>& EHandleList() const {
        return EHTarget();
    }
    void SetEHandleList(const std::list<EHandle>& e_handle_list) {
        eh_ = e_handle_list;
    }
    void SetEHandleList(std::list<EHandle>&& e_handle_list) {
        eh_ = std::move(e_handle_list);
    }

    // virtual methods

    [[nodiscard]] bool IsValidFormat() const override {
        return QubitList().size() == BasisList().size() && CCreate().size() == 1
                && MTarget().size() <= 1 && ETarget().size() == EHTarget().size();
    }
    [[nodiscard]] ScLsInstructionType Type() const override {
        return ScLsInstructionType::LATTICE_SURGERY_MULTINODE;
    }
    [[nodiscard]] Beat Latency() const override {
        return 1;
    }
    [[nodiscard]] Beat StartCorrecting() const override {
        return 0;
    }
    [[nodiscard]] std::string ToString() const override;
    [[nodiscard]] Json ToJson() const override;
    [[nodiscard]] static std::unique_ptr<LatticeSurgeryMultinode> FromJson(const Json& j);

    // override access to operands

    [[nodiscard]] const std::list<QSymbol>& QTarget() const override {
        return q_;
    }
    [[nodiscard]] const std::list<CSymbol>& CCreate() const override {
        return c_;
    }
    [[nodiscard]] const std::list<MSymbol>& MTarget() const override {
        return m_;
    }
    [[nodiscard]] const std::list<ESymbol>& ETarget() const override {
        return e_;
    }
    [[nodiscard]] const std::list<EHandle>& EHTarget() const override {
        return eh_;
    }
    [[nodiscard]] const std::list<Coord3D>& Ancilla() const override {
        return ancilla_;
    }

    // access to properties

    [[nodiscard]] bool IsMeasurement() const override {
        return true;
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromScLsInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->Type() == ScLsInstructionType::LATTICE_SURGERY_MULTINODE;
    }

private:
    LatticeSurgeryMultinode(
            const std::list<QSymbol>& qubit_list,
            const std::list<Pauli>& basis_list,
            const std::list<Coord3D>& path,
            CSymbol cdest,
            const std::list<MSymbol>& m_factory,
            const std::list<ESymbol>& e_factory_list,
            const std::list<EHandle>& e_handle_list,
            const std::list<CSymbol>& condition
    )
        : ScLsInstructionBase(condition)
        , q_(qubit_list)
        , basis_list_{basis_list}
        , c_{cdest}
        , m_(m_factory)
        , e_(e_factory_list)
        , eh_(e_handle_list)
        , ancilla_(path) {}
    std::list<QSymbol> q_;
    std::list<Pauli> basis_list_;
    std::list<CSymbol> c_;
    std::list<MSymbol> m_;
    std::list<ESymbol> e_;
    std::list<EHandle> eh_;
    std::list<Coord3D> ancilla_;
};
class QRET_EXPORT Move : public ScLsInstructionBase {
public:
    static std::unique_ptr<Move> New(
            QSymbol qubit,
            QSymbol qdest,
            const std::list<Coord3D>& path,
            const std::list<CSymbol>& condition
    ) {
        return std::unique_ptr<Move>(new Move(qubit, qdest, path, condition));
    }

    [[nodiscard]] QSymbol Qubit() const {
        return QTarget().front();
    }
    [[nodiscard]] QSymbol& QubitMut() {
        return q_.front();
    }
    [[nodiscard]] QSymbol QDest() const {
        return QTarget().back();
    }
    [[nodiscard]] const std::list<Coord3D>& Path() const {
        return Ancilla();
    }
    void SetPath(const std::list<Coord3D>& path) {
        ancilla_ = path;
    }
    void SetPath(std::list<Coord3D>&& path) {
        ancilla_ = std::move(path);
    }

    // virtual methods

    [[nodiscard]] bool IsValidFormat() const override {
        return QTarget().size() == 2;
    }
    [[nodiscard]] ScLsInstructionType Type() const override {
        return ScLsInstructionType::MOVE;
    }
    [[nodiscard]] Beat Latency() const override {
        return 1;
    }
    [[nodiscard]] Beat StartCorrecting() const override {
        return 0;
    }
    [[nodiscard]] std::string ToString() const override;
    [[nodiscard]] Json ToJson() const override;
    [[nodiscard]] static std::unique_ptr<Move> FromJson(const Json& j);

    // override access to operands

    [[nodiscard]] const std::list<QSymbol>& QTarget() const override {
        return q_;
    }
    [[nodiscard]] const std::list<Coord3D>& Ancilla() const override {
        return ancilla_;
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromScLsInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->Type() == ScLsInstructionType::MOVE;
    }

private:
    Move(QSymbol qubit,
         QSymbol qdest,
         const std::list<Coord3D>& path,
         const std::list<CSymbol>& condition)
        : ScLsInstructionBase(condition)
        , q_{qubit, qdest}
        , ancilla_(path) {}
    std::list<QSymbol> q_;
    std::list<Coord3D> ancilla_;
};
class QRET_EXPORT MoveMagic : public ScLsInstructionBase {
public:
    static std::unique_ptr<MoveMagic> New(
            MSymbol magic_factory,
            QSymbol qdest,
            const std::list<Coord3D>& path,
            const std::list<CSymbol>& condition
    ) {
        return std::unique_ptr<MoveMagic>(new MoveMagic(magic_factory, qdest, path, condition));
    }

    [[nodiscard]] MSymbol MagicFactory() const {
        return MTarget().front();
    }
    void SetMagicFactory(MSymbol magic_factory) {
        m_ = {magic_factory};
    }
    [[nodiscard]] QSymbol QDest() const {
        return QTarget().front();
    }
    [[nodiscard]] const std::list<Coord3D>& Path() const {
        return Ancilla();
    }
    void SetPath(const std::list<Coord3D>& path) {
        ancilla_ = path;
    }
    void SetPath(std::list<Coord3D>&& path) {
        ancilla_ = std::move(path);
    }

    // virtual methods

    [[nodiscard]] bool IsValidFormat() const override {
        return QTarget().size() == 1 && MTarget().size() == 1;
    }
    [[nodiscard]] ScLsInstructionType Type() const override {
        return ScLsInstructionType::MOVE_MAGIC;
    }
    [[nodiscard]] Beat Latency() const override {
        return 1;
    }
    [[nodiscard]] Beat StartCorrecting() const override {
        return 0;
    }
    [[nodiscard]] std::string ToString() const override;
    [[nodiscard]] Json ToJson() const override;
    [[nodiscard]] static std::unique_ptr<MoveMagic> FromJson(const Json& j);

    // override access to operands

    [[nodiscard]] const std::list<QSymbol>& QTarget() const override {
        return q_;
    }
    [[nodiscard]] const std::list<MSymbol>& MTarget() const override {
        return m_;
    }
    [[nodiscard]] const std::list<Coord3D>& Ancilla() const override {
        return ancilla_;
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromScLsInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->Type() == ScLsInstructionType::MOVE_MAGIC;
    }

private:
    MoveMagic(
            MSymbol magic_factory,
            QSymbol qdest,
            const std::list<Coord3D>& path,
            const std::list<CSymbol>& condition
    )
        : ScLsInstructionBase(condition)
        , q_{qdest}
        , m_{magic_factory}
        , ancilla_(path) {}
    std::list<QSymbol> q_;
    std::list<MSymbol> m_;
    std::list<Coord3D> ancilla_;
};
class QRET_EXPORT MoveEntanglement : public ScLsInstructionBase {
public:
    static std::unique_ptr<MoveEntanglement> New(
            ESymbol e_factory,
            EHandle e_handle,
            QSymbol qdest,
            const std::list<Coord3D>& path,
            const std::list<CSymbol>& condition
    ) {
        return std::unique_ptr<MoveEntanglement>(
                new MoveEntanglement(e_factory, e_handle, qdest, path, condition)
        );
    }

    [[nodiscard]] ESymbol EFactory() const {
        return ETarget().front();
    }
    void SetEFactory(ESymbol e_factory) {
        e_ = {e_factory};
    }
    [[nodiscard]] EHandle GetEHandle() const {
        return EHTarget().front();
    }
    void SetEHandle(EHandle e_handle) {
        eh_ = {e_handle};
    }
    [[nodiscard]] QSymbol QDest() const {
        return QTarget().front();
    }
    [[nodiscard]] const std::list<Coord3D>& Path() const {
        return Ancilla();
    }
    void SetPath(const std::list<Coord3D>& path) {
        ancilla_ = path;
    }
    void SetPath(std::list<Coord3D>&& path) {
        ancilla_ = std::move(path);
    }

    // virtual methods

    [[nodiscard]] bool IsValidFormat() const override {
        return QTarget().size() == 1 && ETarget().size() == 1 && EHTarget().size() == 1;
    }
    [[nodiscard]] ScLsInstructionType Type() const override {
        return ScLsInstructionType::MOVE_ENTANGLEMENT;
    }
    [[nodiscard]] Beat Latency() const override {
        return 1;
    }
    [[nodiscard]] Beat StartCorrecting() const override {
        return 0;
    }
    [[nodiscard]] std::string ToString() const override;
    [[nodiscard]] Json ToJson() const override;
    [[nodiscard]] static std::unique_ptr<MoveEntanglement> FromJson(const Json& j);

    // override access to operands

    [[nodiscard]] const std::list<QSymbol>& QTarget() const override {
        return q_;
    }
    [[nodiscard]] const std::list<ESymbol>& ETarget() const override {
        return e_;
    }
    [[nodiscard]] const std::list<EHandle>& EHTarget() const override {
        return eh_;
    }
    [[nodiscard]] const std::list<Coord3D>& Ancilla() const override {
        return ancilla_;
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromScLsInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->Type() == ScLsInstructionType::MOVE_ENTANGLEMENT;
    }

private:
    MoveEntanglement(
            ESymbol e_factory,
            EHandle e_handle,
            QSymbol qdest,
            const std::list<Coord3D>& path,
            const std::list<CSymbol>& condition
    )
        : ScLsInstructionBase(condition)
        , q_{qdest}
        , e_{e_factory}
        , eh_{e_handle}
        , ancilla_(path) {}
    std::list<QSymbol> q_;
    std::list<ESymbol> e_;
    std::list<EHandle> eh_;
    std::list<Coord3D> ancilla_;
};
class QRET_EXPORT Cnot : public ScLsInstructionBase {
public:
    static std::unique_ptr<Cnot> New(
            QSymbol qubit1,
            QSymbol qubit2,
            const std::list<Coord3D>& path,
            const std::list<CSymbol>& condition
    ) {
        return std::unique_ptr<Cnot>(new Cnot(qubit1, qubit2, path, condition));
    }

    [[nodiscard]] QSymbol Qubit1() const {
        return QTarget().front();
    }
    [[nodiscard]] QSymbol Qubit2() const {
        return QTarget().back();
    }
    QSymbol& Qubit1Mut() {
        return q_.front();
    }
    QSymbol& Qubit2Mut() {
        return q_.back();
    }
    [[nodiscard]] const std::list<Coord3D>& Path() const {
        return Ancilla();
    }
    void SetPath(const std::list<Coord3D>& path) {
        ancilla_ = path;
    }
    void SetPath(std::list<Coord3D>&& path) {
        ancilla_ = std::move(path);
    }

    // virtual methods

    [[nodiscard]] bool IsValidFormat() const override {
        return QTarget().size() == 2;
    }
    [[nodiscard]] ScLsInstructionType Type() const override {
        return ScLsInstructionType::CNOT;
    }
    [[nodiscard]] Beat Latency() const override {
        return 2;
    }
    [[nodiscard]] Beat StartCorrecting() const override {
        return 0;
    }
    [[nodiscard]] std::string ToString() const override;
    [[nodiscard]] Json ToJson() const override;
    [[nodiscard]] static std::unique_ptr<Cnot> FromJson(const Json& j);

    // override access to operands

    [[nodiscard]] const std::list<QSymbol>& QTarget() const override {
        return q_;
    }
    [[nodiscard]] const std::list<Coord3D>& Ancilla() const override {
        return ancilla_;
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromScLsInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->Type() == ScLsInstructionType::CNOT;
    }

private:
    Cnot(QSymbol qubit1,
         QSymbol qubit2,
         const std::list<Coord3D>& path,
         const std::list<CSymbol>& condition)
        : ScLsInstructionBase(condition)
        , q_{qubit1, qubit2}
        , ancilla_(path) {}
    std::list<QSymbol> q_;
    std::list<Coord3D> ancilla_;
};
class QRET_EXPORT CnotTrans : public ScLsInstructionBase {
public:
    static std::unique_ptr<CnotTrans>
    New(QSymbol qubit1, QSymbol qubit2, const std::list<CSymbol>& condition) {
        return std::unique_ptr<CnotTrans>(new CnotTrans(qubit1, qubit2, condition));
    }

    [[nodiscard]] QSymbol Qubit1() const {
        return QTarget().front();
    }
    [[nodiscard]] QSymbol Qubit2() const {
        return QTarget().back();
    }

    // virtual methods

    [[nodiscard]] bool IsValidFormat() const override {
        return QTarget().size() == 2;
    }
    [[nodiscard]] ScLsInstructionType Type() const override {
        return ScLsInstructionType::CNOT_TRANS;
    }
    [[nodiscard]] Beat Latency() const override {
        return 0;
    }  // NOTE: 1/d
    [[nodiscard]] Beat StartCorrecting() const override {
        return 0;
    }
    [[nodiscard]] std::string ToString() const override;
    [[nodiscard]] Json ToJson() const override;
    [[nodiscard]] static std::unique_ptr<CnotTrans> FromJson(const Json& j);

    // override access to operands

    [[nodiscard]] const std::list<QSymbol>& QTarget() const override {
        return q_;
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromScLsInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->Type() == ScLsInstructionType::CNOT_TRANS;
    }

private:
    CnotTrans(QSymbol qubit1, QSymbol qubit2, const std::list<CSymbol>& condition)
        : ScLsInstructionBase(condition)
        , q_{qubit1, qubit2} {}
    std::list<QSymbol> q_;
};
class QRET_EXPORT SwapTrans : public ScLsInstructionBase {
public:
    static std::unique_ptr<SwapTrans>
    New(QSymbol qubit1, QSymbol qubit2, const std::list<CSymbol>& condition) {
        return std::unique_ptr<SwapTrans>(new SwapTrans(qubit1, qubit2, condition));
    }

    [[nodiscard]] QSymbol Qubit1() const {
        return QTarget().front();
    }
    [[nodiscard]] QSymbol Qubit2() const {
        return QTarget().back();
    }

    // virtual methods

    [[nodiscard]] bool IsValidFormat() const override {
        return QTarget().size() == 2;
    }
    [[nodiscard]] ScLsInstructionType Type() const override {
        return ScLsInstructionType::SWAP_TRANS;
    }
    [[nodiscard]] Beat Latency() const override {
        return 0;
    }  // NOTE: 1/d
    [[nodiscard]] Beat StartCorrecting() const override {
        return 0;
    }
    [[nodiscard]] std::string ToString() const override;
    [[nodiscard]] Json ToJson() const override;
    [[nodiscard]] static std::unique_ptr<SwapTrans> FromJson(const Json& j);

    // override access to operands

    [[nodiscard]] const std::list<QSymbol>& QTarget() const override {
        return q_;
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromScLsInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->Type() == ScLsInstructionType::SWAP_TRANS;
    }

private:
    SwapTrans(QSymbol qubit1, QSymbol qubit2, const std::list<CSymbol>& condition)
        : ScLsInstructionBase(condition)
        , q_{qubit1, qubit2} {}
    std::list<QSymbol> q_;
};
class QRET_EXPORT MoveTrans : public ScLsInstructionBase {
public:
    static std::unique_ptr<MoveTrans>
    New(QSymbol qubit, QSymbol qdest, const std::list<CSymbol>& condition) {
        return std::unique_ptr<MoveTrans>(new MoveTrans(qubit, qdest, condition));
    }

    [[nodiscard]] QSymbol Qubit() const {
        return QTarget().front();
    }
    [[nodiscard]] QSymbol QDest() const {
        return QTarget().back();
    }

    // virtual methods

    [[nodiscard]] bool IsValidFormat() const override {
        return QTarget().size() == 2;
    }
    [[nodiscard]] ScLsInstructionType Type() const override {
        return ScLsInstructionType::MOVE_TRANS;
    }
    [[nodiscard]] Beat Latency() const override {
        return 0;
    }  // NOTE: 1/d
    [[nodiscard]] Beat StartCorrecting() const override {
        return 0;
    }
    [[nodiscard]] std::string ToString() const override;
    [[nodiscard]] Json ToJson() const override;
    [[nodiscard]] static std::unique_ptr<MoveTrans> FromJson(const Json& j);

    // override access to operands

    [[nodiscard]] const std::list<QSymbol>& QTarget() const override {
        return q_;
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromScLsInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->Type() == ScLsInstructionType::MOVE_TRANS;
    }

private:
    MoveTrans(QSymbol qubit, QSymbol qdest, const std::list<CSymbol>& condition)
        : ScLsInstructionBase(condition)
        , q_{qubit, qdest} {}
    std::list<QSymbol> q_;
};

class QRET_EXPORT ClassicalOperation : public ScLsInstructionBase {
public:
    static std::unique_ptr<ClassicalOperation> New(
            ScLsInstructionType type,
            const std::list<CSymbol>& register_list,
            CSymbol cdest,
            const std::list<CSymbol>& condition
    ) {
        return std::unique_ptr<ClassicalOperation>(
                new ClassicalOperation(type, register_list, cdest, condition)
        );
    }
    static std::unique_ptr<ClassicalOperation>
    Xor(const std::list<CSymbol>& register_list, CSymbol cdest, const std::list<CSymbol>& condition
    ) {
        return std::unique_ptr<ClassicalOperation>(
                new ClassicalOperation(ScLsInstructionType::XOR, register_list, cdest, condition)
        );
    }
    static std::unique_ptr<ClassicalOperation>
    And(const std::list<CSymbol>& register_list, CSymbol cdest, const std::list<CSymbol>& condition
    ) {
        return std::unique_ptr<ClassicalOperation>(
                new ClassicalOperation(ScLsInstructionType::AND, register_list, cdest, condition)
        );
    }
    static std::unique_ptr<ClassicalOperation>
    Or(const std::list<CSymbol>& register_list, CSymbol cdest, const std::list<CSymbol>& condition
    ) {
        return std::unique_ptr<ClassicalOperation>(
                new ClassicalOperation(ScLsInstructionType::OR, register_list, cdest, condition)
        );
    }

    [[nodiscard]] const std::list<CSymbol>& RegisterList() const {
        return register_list_;
    }
    [[nodiscard]] CSymbol CDest() const {
        return dest_.back();
    }

    /**
     * @brief Executes a bitwise logic operation on two Tribool operands based on the instruction
     * type.
     *
     * @param type Instruction type.
     * @param lhs The left-hand side Tribool operand.
     * @param rhs The right-hand side Tribool operand.
     * @return Tribool The result of the logical operation.
     * @throws std::logic_error If the instruction type is unknown.
     */
    [[nodiscard]] static Tribool Op(ScLsInstructionType type, Tribool lhs, Tribool rhs) {
        switch (type) {
            case ScLsInstructionType::XOR:
                return lhs ^ rhs;
            case ScLsInstructionType::AND:
                return lhs & rhs;
            case ScLsInstructionType::OR:
                return lhs | rhs;
            default:
                throw std::runtime_error("Invalid operation type: not classical operation.");
        }
        return Tribool::Unknown;
    }

    // virtual methods

    [[nodiscard]] bool IsValidFormat() const override {
        return !CCreate().empty();
    }
    [[nodiscard]] ScLsInstructionType Type() const override {
        return type_;
    }
    [[nodiscard]] Beat Latency() const override {
        return 0;
    }
    [[nodiscard]] Beat StartCorrecting() const override {
        return 0;
    }
    [[nodiscard]] std::string ToString() const override;
    [[nodiscard]] Json ToJson() const override;
    [[nodiscard]] static std::unique_ptr<ClassicalOperation> FromJson(const Json& j);

    // override access to operands

    [[nodiscard]] const std::list<CSymbol>& CCreate() const override {
        return dest_;
    }
    [[nodiscard]] const std::list<CSymbol>& CDepend() const override {
        return register_list_;
    }

    // access to properties

    [[nodiscard]] bool IsClassical() const override {
        return true;
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromScLsInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        const auto type = from->Type();
        return type == ScLsInstructionType::XOR || type == ScLsInstructionType::AND
                || type == ScLsInstructionType::OR;
    }

private:
    ClassicalOperation(
            ScLsInstructionType type,
            const std::list<CSymbol>& register_list,
            CSymbol cdest,
            const std::list<CSymbol>& condition
    )
        : ScLsInstructionBase(condition)
        , type_{type}
        , register_list_{register_list}
        , dest_{cdest} {}

    ScLsInstructionType type_;
    std::list<CSymbol> register_list_;
    std::list<CSymbol> dest_;
};

class QRET_EXPORT ProbabilityHint : public ScLsInstructionBase {
public:
    static std::unique_ptr<ProbabilityHint>
    New(CSymbol cdest, double prob, const std::list<CSymbol>& condition) {
        return std::unique_ptr<ProbabilityHint>(new ProbabilityHint(cdest, prob, condition));
    }

    [[nodiscard]] CSymbol CDest() const {
        return c_.back();
    }
    [[nodiscard]] double Prob() const {
        return prob_;
    }

    void SetSampledValue(Tribool t) {
        sampled_value_ = t;
    }
    [[nodiscard]] Tribool GetSampledValue() const {
        return sampled_value_;
    }

    // virtual methods

    [[nodiscard]] bool IsValidFormat() const override {
        return CCreate().empty() && CDepend().size() == 1;
    }
    [[nodiscard]] ScLsInstructionType Type() const override {
        return ScLsInstructionType::PROBABILITY_HINT;
    }
    [[nodiscard]] Beat Latency() const override {
        return 0;
    }
    [[nodiscard]] Beat StartCorrecting() const override {
        return 0;
    }
    [[nodiscard]] std::string ToString() const override;
    [[nodiscard]] Json ToJson() const override;
    [[nodiscard]] static std::unique_ptr<ProbabilityHint> FromJson(const Json& j);

    // override access to operands

    [[nodiscard]] const std::list<CSymbol>& CDepend() const override {
        return c_;
    }

    // access to properties

    [[nodiscard]] bool IsOptHint() const override {
        return true;
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromScLsInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->Type() == ScLsInstructionType::PROBABILITY_HINT;
    }

private:
    ProbabilityHint(CSymbol cdest, double prob, const std::list<CSymbol>& condition)
        : ScLsInstructionBase(condition)
        , c_{cdest}
        , prob_{prob} {}
    std::list<CSymbol> c_;
    double prob_;
    Tribool sampled_value_ = Tribool::Unknown;
};

class QRET_EXPORT AwaitCorrection : public ScLsInstructionBase {
public:
    static std::unique_ptr<AwaitCorrection>
    New(const std::list<QSymbol>& qs, const std::list<CSymbol>& condition) {
        return std::unique_ptr<AwaitCorrection>(new AwaitCorrection(qs, condition));
    }

    // virtual methods

    [[nodiscard]] bool IsValidFormat() const override {
        return !QTarget().empty();
    }
    [[nodiscard]] ScLsInstructionType Type() const override {
        return ScLsInstructionType::AWAIT_CORRECTION;
    }
    [[nodiscard]] Beat Latency() const override {
        return 0;
    }
    [[nodiscard]] Beat StartCorrecting() const override {
        return 0;
    }
    [[nodiscard]] std::string ToString() const override;
    [[nodiscard]] Json ToJson() const override;
    [[nodiscard]] static std::unique_ptr<AwaitCorrection> FromJson(const Json& j);

    // override access to operands

    [[nodiscard]] const std::list<QSymbol>& QTarget() const override {
        return qs_;
    }

#if defined(_MSC_VER)
    template <typename From>
#else
    template <DerivedFromScLsInstruction From>
#endif
    [[nodiscard]] static bool ClassOf(const From* from) {
        return from->Type() == ScLsInstructionType::AWAIT_CORRECTION;
    }

private:
    AwaitCorrection(const std::list<QSymbol>& qs, const std::list<CSymbol>& condition)
        : ScLsInstructionBase(condition)
        , qs_{qs} {}
    std::list<QSymbol> qs_;
};

#pragma region machine function util
void QRET_EXPORT DumpMachineFunctionSortedByBeat(std::ostream& out, const MachineFunction& mf);
#pragma endregion
}  // namespace qret::sc_ls_fixed_v0

#endif  // QRET_TARGET_SC_LS_FIXED_V0_INSTRUCTION_H
