/**
 * @file qret/pass.h
 * @brief Base class for passes.
 * @details This file defines a base class for passes.
 */

#ifndef QRET_PASS_H
#define QRET_PASS_H

#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

#include "qret/qret_export.h"

namespace qret {
// Forward declarations
template <typename PassName>
struct RegisterPass;
class Pass;
class MFPassManager;

/**
 * @brief Use the PassInfo to identify a pass.
 */
using PassIDType = const char*;
/**
 * @brief Different types of passes.
 */
enum class PassKind : std::uint8_t {
    Region,
    Loop,
    Function,
    CallGraphSCC,
    Module,
    /// \todo (FIXME) LLVM doesn't have MachineFunction
    MachineFunction,
};
/**
 * @brief Different types of internal pass managers.
 */
enum class PassManagerType : std::uint8_t {
    RegionPassManager,
    LoopPassManager,
    FunctionPassManager,
    CallGraphPassManager,
    ModulePassManager,
    /// \todo (FIXME) LLVM doesn't have MFPassManager
    MFPassManager,
};
/**
 * @brief An instance of this class exists for every pass known by the system, and can be
 * obtained from a live Pass by calling its GetPassInfo() method. These objects are set up by
 * the RegisterPass<> template.
 */
class QRET_EXPORT PassInfo {
public:
    using NormalCtorType = std::function<std::unique_ptr<Pass>()>;
    template <typename PassName>
    friend struct RegisterPass;

    [[nodiscard]] std::string_view GetPassName() const {
        return pass_name_;
    }
    [[nodiscard]] std::string_view GetPassArgument() const {
        return pass_argument_;
    }
    [[nodiscard]] PassIDType GetPassID() const {
        return pass_id_;
    }
    [[nodiscard]] NormalCtorType GetNormalCtor() const {
        return ctor_;
    }
    void SetNormalCtor(NormalCtorType ctor) {
        ctor_ = std::move(ctor);
    }

private:
    PassInfo(
            std::string_view pass_name,
            std::string_view pass_argument,
            PassIDType pass_id,
            NormalCtorType ctor
    )
        : pass_name_{pass_name}
        , pass_argument_{pass_argument}
        , pass_id_{pass_id}
        , ctor_{std::move(ctor)} {}
    std::string_view pass_name_;  //!< nice name for pass
    std::string_view pass_argument_;  //!< command Line argument to run this pass
    PassIDType pass_id_;  //!< id object for the pass
    NormalCtorType ctor_;  //!< constructor of the pass
};
/**
 * @brief Pass interface.
 * @details Subclass this to implement optimization procedure.
 */
class QRET_EXPORT Pass {
public:
    explicit Pass(PassIDType pass_id, PassKind kind)
        : pass_id_{pass_id}
        , kind_{kind} {}
    virtual ~Pass() = default;
    Pass(const Pass&) = delete;
    Pass& operator=(const Pass&) = delete;

    [[nodiscard]] PassIDType GetPassId() const {
        return pass_id_;
    }
    [[nodiscard]] PassKind GetPassKind() const {
        return kind_;
    }
    [[nodiscard]] const PassInfo* GetPassInfo() const;
    [[nodiscard]] virtual std::string_view GetPassName() const;
    [[nodiscard]] virtual std::string_view GetPassArgument() const;
    [[nodiscard]] virtual PassManagerType GetPassManagerType() const = 0;

private:
    PassIDType pass_id_;
    PassKind kind_;
};
/**
 * @brief Manages a sequence of passes.
 */
class QRET_EXPORT PassManager {
public:
    PassManager() = default;
    PassManager(PassManager&&) = default;
    PassManager& operator=(PassManager&&) = default;
    ~PassManager() = default;

    Pass* AddPass(std::unique_ptr<Pass>&& pass) {
        passes_.emplace_back(std::move(pass));
        return passes_.back().get();
    };

    auto begin() const {
        return passes_.begin();
    }  // NOLINT
    auto cbegin() {
        return passes_.cbegin();
    }  // NOLINT
    auto end() const {
        return passes_.end();
    }  // NOLINT
    auto cend() {
        return passes_.cend();
    }  // NOLINT

protected:
    std::vector<std::unique_ptr<Pass>> passes_;
};
/**
 * @brief This class manages the registration and initialization of the pass subsystem as
 * application startup.
 */
class QRET_EXPORT PassRegistry {
public:
    static PassRegistry* GetPassRegistry();
    PassRegistry() = default;
    ~PassRegistry() = default;

    [[nodiscard]] const PassInfo* GetPassInfo(PassIDType pass_id) const;
    [[nodiscard]] const PassInfo* GetPassInfo(std::string_view arg) const;
    [[nodiscard]] bool Contains(PassIDType pass_id) const;
    [[nodiscard]] bool Contains(std::string_view arg) const;
    void RegisterPass(const PassInfo& pi);

    auto begin() const {
        return pass_info_string_map_.begin();
    }  // NOLINT
    auto cbegin() {
        return pass_info_string_map_.cbegin();
    }  // NOLINT
    auto end() const {
        return pass_info_string_map_.end();
    }  // NOLINT
    auto cend() {
        return pass_info_string_map_.cend();
    }  // NOLINT

private:
    std::map<PassIDType, const PassInfo*> pass_info_map_;
    std::map<std::string_view, const PassInfo*> pass_info_string_map_;
};
template <class PassName, std::enable_if_t<std::is_default_constructible<PassName>{}, bool> = true>
std::unique_ptr<PassName> CallDefaultCtor() {
    return std::unique_ptr<PassName>(new PassName());
}
template <class PassName, std::enable_if_t<!std::is_default_constructible<PassName>{}, bool> = true>
std::unique_ptr<PassName> CallDefaultCtor() {
    std::cerr << "pass " << typeid(PassName).name() << " doesn't have default ctor" << std::endl;
    return nullptr;
}
/**
 * @brief This template class is used to notify the system that a Pass is available for use, and
 * registers it into the internal database maintained by the PassRegistry.
 *
 * @tparam PassName pass to register
 */
template <typename PassName>
struct RegisterPass : public PassInfo {
    RegisterPass(std::string_view pass_name, std::string_view pass_argument)
        : PassInfo(
                  pass_name,
                  pass_argument,
                  &PassName::ID,
                  PassInfo::NormalCtorType(CallDefaultCtor<PassName>)
          ) {
        PassRegistry::GetPassRegistry()->RegisterPass(*this);
    }
};
class QRET_EXPORT OptimizationLevel final {
public:
    static const OptimizationLevel O0;
    static const OptimizationLevel O1;
    static const OptimizationLevel O2;
    static const OptimizationLevel O3;
    static const OptimizationLevel Os;
    static const OptimizationLevel Oz;

    OptimizationLevel() = default;
    OptimizationLevel(std::uint32_t speed_level, std::uint32_t size_level)
        : speed_level_{speed_level}
        , size_level_{size_level} {
        // Check that only valid combinations are passed
        assert(speed_level <= 3 && "optimization level for speed should be 0, 1, 2, or 3");
        assert(size_level <= 2 && "optimization level for size should be 0, 1, or 2");
        assert((size_level == 0 || speed_level == 2)
               && "optimize for size should be encoded with speedup level == 2");
    }

    [[nodiscard]] bool IsOptimizingForSpeed() const {
        return size_level_ == 0 && speed_level_ > 0;
    }
    [[nodiscard]] bool IsOptimizingForSize() const {
        return size_level_ > 0;
    }
    [[nodiscard]] std::uint32_t GetSpeedupLevel() const {
        return speed_level_;
    }
    [[nodiscard]] std::uint32_t GetSizeLevel() const {
        return size_level_;
    }

private:
    std::uint32_t speed_level_ = 2;
    std::uint32_t size_level_ = 0;
};
inline bool operator==(const OptimizationLevel& lhs, const OptimizationLevel& rhs) {
    return lhs.GetSizeLevel() == rhs.GetSizeLevel()
            && lhs.GetSpeedupLevel() == rhs.GetSpeedupLevel();
}
inline bool operator!=(const OptimizationLevel& lhs, const OptimizationLevel& rhs) {
    return !(lhs == rhs);
}
/**
 * @brief This class provides access to building qret's passes.
 * @todo Implement this class.
 */
class QRET_EXPORT PassBuilder {
public:
    [[nodiscard]] MFPassManager BuildMFDefaultPipeline(const OptimizationLevel& level) const;

private:
};
}  // namespace qret

#endif  // QRET_PASS_H
