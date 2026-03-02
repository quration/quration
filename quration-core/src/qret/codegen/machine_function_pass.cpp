/**
 * @file qret/codegen/machine_function_pass.cpp
 * @brief Pass for MachineFunctions.
 */

#include "qret/codegen/machine_function_pass.h"

#include <chrono>  // NOLINT
#include <string>

#include "qret/base/log.h"
#include "qret/codegen/machine_function.h"

namespace qret {
void MFPassManager::Run(MachineFunction& mf) {
    for (auto i = analysis_.run_order.size(); i < passes_.size(); ++i) {
        auto& pass = passes_[i];
        const auto start = std::chrono::high_resolution_clock::now();

        auto* ptr = static_cast<MachineFunctionPass*>(pass.get());
        LOG_INFO("Run {}", std::string(ptr->GetPassName()));
        ptr->RunOnMachineFunction(mf);

        const auto finish = std::chrono::high_resolution_clock::now();
        const auto elapsed_ms = std::chrono::duration_cast<MFAnalysis::Time>(finish - start);
        analysis_.run_order.emplace_back(ptr);
        analysis_.elapsed_ms[ptr] = elapsed_ms;
    }
}
}  // namespace qret
