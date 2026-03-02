/**
 * @file qret/cmd/simulate.h
 * @brief Define 'simulate' sumcommand in qret.
 */

#ifndef QRET_CMD_SIMULATE_H
#define QRET_CMD_SIMULATE_H

#include <string>

#include "qret/cmd/common.h"

namespace qret::cmd {
class CommandSimulate : public SubCommand {
public:
    CommandSimulate() = default;

    ReturnStatus Main(int argc, const char** argv) override;

    std::string Name() const override {
        return "simulate";
    }
};
ReturnStatus Simulate(
        const std::string& input,
        const std::string& function_name,
        const std::string& state,
        const std::string& init_state,
        std::uint64_t seed,
        std::uint64_t num_samples,
        bool sample_summary,
        bool print_raw,
        bool use_qulacs,
        std::uint64_t max_superpositions
);
}  // namespace qret::cmd

#endif  // QRET_CMD_SIMULATE_H
