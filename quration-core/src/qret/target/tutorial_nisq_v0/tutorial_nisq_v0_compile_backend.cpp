/**
 * @file qret/target/tutorial_nisq_v0/tutorial_nisq_v0_compile_backend.cpp
 * @brief Compile backend for TUTORIAL_NISQ_V0 target.
 */

#include "qret/target/tutorial_nisq_v0/tutorial_nisq_v0_compile_backend.h"

#include <exception>
#include <fstream>
#include <iostream>
#include <optional>

#include "qret/base/json.h"
#include "qret/codegen/asm_streamer.h"
#include "qret/codegen/machine_function.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/openqasm2.h"
#include "qret/ir/context.h"
#include "qret/ir/function.h"
#include "qret/ir/json.h"
#include "qret/ir/module.h"
#include "qret/parser/openqasm2.h"
#include "qret/target/target_registry.h"
#include "qret/target/tutorial_nisq_v0/lowering.h"
#include "qret/target/tutorial_nisq_v0/tutorial_nisq_v0_target_machine.h"
#include "qret/transforms/ipo/inliner.h"
#include "qret/transforms/scalar/decomposition.h"
#include "qret/transforms/scalar/ignore_global_phase.h"
#include "qret/transforms/scalar/static_condition_pruning.h"

namespace qret::tutorial_nisq_v0 {
namespace {
std::optional<qret::ir::Function*>
LoadFunctionFromIR(const qret::CompileRequest& request, qret::ir::IRContext& context) {
    auto ifs = std::ifstream(request.input);
    if (!ifs.good()) {
        std::cerr << "failed to open input file: " << request.input << std::endl;
        return std::nullopt;
    }

    qret::Json j;
    try {
        j = qret::Json::parse(ifs);
    } catch (const std::exception& ex) {
        std::cerr << "failed to parse IR json: " << ex.what() << std::endl;
        return std::nullopt;
    }

    try {
        qret::ir::LoadJson(j, context);
    } catch (const std::exception& ex) {
        std::cerr << "failed to load IR module: " << ex.what() << std::endl;
        return std::nullopt;
    }

    if (context.owned_module.empty() || context.owned_module.back() == nullptr) {
        std::cerr << "input does not contain a valid IR module" << std::endl;
        return std::nullopt;
    }

    auto* func = context.owned_module.back()->GetFunction(request.function_name);
    if (func == nullptr) {
        std::cerr << "function of name '" << request.function_name << "' not found" << std::endl;
        return std::nullopt;
    }
    return func;
}

std::optional<qret::ir::Function*>
LoadFunctionFromOpenQASM2(const qret::CompileRequest& request, qret::ir::IRContext& context) {
    try {
        const auto ast = qret::openqasm2::ParseOpenQASM2File(request.input);
        auto* module = qret::ir::Module::Create("OpenQASM2", context);
        auto builder = qret::frontend::CircuitBuilder(module);
        const auto entry_name = request.function_name.empty() ? "main" : request.function_name;
        auto* circuit = qret::frontend::BuildCircuitFromAST(ast, builder, entry_name);
        return circuit->GetIR();
    } catch (const std::exception& ex) {
        std::cerr << "failed to load OpenQASM2: " << ex.what() << std::endl;
        return std::nullopt;
    }
}

void PreprocessIRForTutorial(qret::ir::Function& func) {
    // This sequence intentionally mirrors the regular compile flow, but keeps
    // only the minimum steps needed for this tutorial target.
    qret::ir::RecursiveInlinerPass().RunOnFunction(func);
    qret::ir::StaticConditionPruningPass().RunOnFunction(func);
    qret::ir::DecomposeInst().RunOnFunction(func);
    qret::ir::IgnoreGlobalPhase().RunOnFunction(func);
}

// tutorial_nisq_v0 intentionally treats assembly as the compile output format.
// Therefore this backend invokes AsmPrinter inside Compile().
// The routine below is mostly target-agnostic, but kept local to preserve a
// self-contained tutorial flow (load IR -> lower -> emit asm).
bool EmitAsm(const qret::CompileRequest& request, qret::MachineFunction& mf) {
    const auto* registry = qret::TargetRegistry::GetTargetRegistry();
    const auto* target = registry->GetTarget(request.target_name);
    if (target == nullptr) {
        return false;
    }
    // For tutorial_nisq_v0, asm emission is required for successful compile output.
    if (!target->HasAsmPrinter()) {
        std::cerr << "target '" << request.target_name << "' does not provide asm printer"
                  << std::endl;
        return false;
    }

    auto streamer = target->CreateAsmStreamer();
    auto printer = target->TryCreateAsmPrinter(std::move(streamer));
    if (printer == nullptr) {
        std::cerr << "target '" << request.target_name << "' does not provide asm printer"
                  << std::endl;
        return false;
    }

    // AsmPrinter follows MachineFunctionPass convention and returns a "changed" flag.
    // Assembly emission itself is side-effectful and valid regardless of that flag.
    printer->RunOnMachineFunction(mf);

    try {
        auto ofs = std::ofstream(request.output);
        if (!ofs.good()) {
            std::cerr << "failed to open output file: " << request.output << std::endl;
            return false;
        }
        ofs << printer->GetStreamer()->ToString();
        ofs.close();
    } catch (const std::exception& ex) {
        std::cerr << "failed to write output file: " << ex.what() << std::endl;
        return false;
    }
    return true;
}
}  // namespace

std::string_view TutorialNisqV0CompileBackend::TargetName() const {
    return "tutorial_nisq_v0";
}

void TutorialNisqV0CompileBackend::AddCompileOptions(
        qret::CompileOptionRegistrar& registrar
) const {
    // No target-specific option in tutorial backend.
    (void)registrar;
}

bool TutorialNisqV0CompileBackend::Supports(const qret::CompileFormat source) const {
    return source == qret::CompileFormat::IR || source == qret::CompileFormat::OPENQASM2;
}

bool TutorialNisqV0CompileBackend::Compile(
        const qret::CompileRequest& request,
        const qret::CompileOptionReader& options
) const {
    (void)options;

    if (!Supports(request.source_format)) {
        std::cerr << "source format is not supported for target 'tutorial_nisq_v0'" << std::endl;
        return false;
    }
    if (request.source_format == qret::CompileFormat::IR && request.function_name.empty()) {
        std::cerr << "--function option is required" << std::endl;
        return false;
    }

    auto context = qret::ir::IRContext();
    const auto func = request.source_format == qret::CompileFormat::IR
            ? LoadFunctionFromIR(request, context)
            : LoadFunctionFromOpenQASM2(request, context);
    if (!func.has_value()) {
        return false;
    }

    PreprocessIRForTutorial(*func.value());

    auto target_machine = qret::tutorial_nisq_v0::TutorialNisqV0TargetMachine::New();
    auto mf = qret::MachineFunction(target_machine.get());
    mf.SetIR(*func);

    if (!qret::tutorial_nisq_v0::TutorialNisqV0Lowering().RunOnMachineFunction(mf)) {
        return false;
    }

    return EmitAsm(request, mf);
}
}  // namespace qret::tutorial_nisq_v0
