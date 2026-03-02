/**
 * @file qret/target/target_registry.h
 * @brief Target registration.
 * @details This file defines Target and TargetRegistry classes which are tools to
 * access target specific classes (TargetMachine, AsmPrinter, etc.)
 * which have been registered.
 */

#ifndef QRET_TARGET_TARGET_REGISTRY_H
#define QRET_TARGET_TARGET_REGISTRY_H

#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "qret/codegen/asm_printer.h"
#include "qret/codegen/asm_streamer.h"
#include "qret/qret_export.h"
#include "qret/target/target_enum.h"

namespace qret {
// Forward declarations
class TargetMachine;
class TargetCompileBackend;
class TargetRegistry;

/**
 * @brief Wrapper for Target specific information.
 */
class QRET_EXPORT Target {
public:
    friend class TargetRegistry;

    using TargetMachineCtorType = std::function<std::unique_ptr<TargetMachine>(const Target&)>;
    using AsmPrinterCtorType =
            std::function<std::unique_ptr<AsmPrinter>(std::unique_ptr<AsmStreamer>&&)>;

    [[nodiscard]] std::string_view GetName() const {
        return name_;
    }
    [[nodiscard]] std::string_view GetShortDescription() const {
        return short_desc_;
    }

    [[nodiscard]] bool HasAsmPrinter() const {
        return static_cast<bool>(asm_printer_ctor_);
    }
    [[nodiscard]] std::unique_ptr<AsmPrinter> TryCreateAsmPrinter(
            std::unique_ptr<AsmStreamer>&& streamer
    ) const {
        if (!HasAsmPrinter()) {
            return nullptr;
        }
        return asm_printer_ctor_(std::move(streamer));
    }
    [[nodiscard]] std::unique_ptr<AsmPrinter> CreateAsmPrinter(
            std::unique_ptr<AsmStreamer>&& streamer
    ) const {
        auto printer = TryCreateAsmPrinter(std::move(streamer));
        if (printer == nullptr) {
            throw std::runtime_error("asm printer is not registered for target '" + name_ + "'");
        }
        return printer;
    }
    [[nodiscard]] std::unique_ptr<AsmStreamer> CreateAsmStreamer() const {
        return std::unique_ptr<AsmStreamer>(new AsmStreamer());
    }

private:
    std::string name_;
    std::string short_desc_;
    AsmPrinterCtorType asm_printer_ctor_;
};
/**
 * @brief This class manages the registration and initialization of the target specific features as
 * application startup.
 */
class QRET_EXPORT TargetRegistry {
public:
    static TargetRegistry* GetTargetRegistry();
    TargetRegistry() = default;
    ~TargetRegistry() = default;

    [[nodiscard]] const Target* GetTarget(std::string_view name) const;
    [[nodiscard]] const Target* GetTarget(TargetEnum name) const;
    [[nodiscard]] const TargetCompileBackend* GetCompileBackend(std::string_view target_name) const;
    [[nodiscard]] const std::map<std::string, const TargetCompileBackend*>&
    GetCompileBackends() const {
        return compile_backend_map_;
    }

    void RegisterTarget(Target* target, std::string_view name, std::string_view short_desc);
    void RegisterCompileBackend(const TargetCompileBackend* backend);
    void RegisterAsmPrinter(Target* target, Target::AsmPrinterCtorType ctor) {
        target->asm_printer_ctor_ = std::move(ctor);
    }

private:
    std::map<std::string, const Target*> target_map_;
    std::map<std::string, const TargetCompileBackend*> compile_backend_map_;
};
/**
 * @brief This class is used to notify the system that a Target is available for use, and
 * registers it into the internal database maintained by the TargetRegistry.
 */
struct QRET_EXPORT RegisterTarget {
    RegisterTarget(Target* target, std::string_view name, std::string_view short_desc) {
        TargetRegistry::GetTargetRegistry()->RegisterTarget(target, name, short_desc);
    }
};
/**
 * @brief This class is used to notify the system that a compile backend is available for use,
 * and registers it into the internal database maintained by the TargetRegistry.
 */
struct QRET_EXPORT RegisterTargetCompileBackend {
    explicit RegisterTargetCompileBackend(const TargetCompileBackend* backend) {
        TargetRegistry::GetTargetRegistry()->RegisterCompileBackend(backend);
    }
};
/**
 * @brief This class is used to notify the system that a AsmPrinter is available for use, and
 * registers it into the internal database maintained by the TargetRegistry.
 *
 * @tparam AsmPrinter asm printer to register
 */
template <class AsmPrinterImpl>
struct RegisterAsmPrinter {
    explicit RegisterAsmPrinter(Target* target) {
        TargetRegistry::GetTargetRegistry()->RegisterAsmPrinter(
                target,
                Target::AsmPrinterCtorType{Allocator}
        );
    }

private:
    static std::unique_ptr<AsmPrinter> Allocator(std::unique_ptr<AsmStreamer>&& streamer) {
        return std::unique_ptr<AsmPrinter>(new AsmPrinterImpl(std::move(streamer)));
    }
};
}  // namespace qret

#endif  // QRET_TARGET_TARGET_REGISTRY_H
