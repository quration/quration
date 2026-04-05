/**
 * @file qret/target/sc_ls_fixed_v0/routing.cpp
 * @brief Routing.
 */

#include "qret/target/sc_ls_fixed_v0/routing.h"

#include <fmt/format.h>

#include <cassert>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>

#include "qret/base/log.h"
#include "qret/base/option.h"
#include "qret/codegen/machine_function.h"
#include "qret/pass.h"
#include "qret/target/sc_ls_fixed_v0/inst_queue.h"
#include "qret/target/sc_ls_fixed_v0/instruction.h"
#include "qret/target/sc_ls_fixed_v0/sc_ls_fixed_v0_target_machine.h"
#include "qret/target/sc_ls_fixed_v0/search_chip_comm.h"
#include "qret/target/sc_ls_fixed_v0/simulator.h"
#include "qret/target/sc_ls_fixed_v0/symbol.h"
#include "qret/target/sc_ls_fixed_v0/validation.h"

namespace qret::sc_ls_fixed_v0 {
namespace {
static auto X = RegisterPass<Routing>("Routing", "sc_ls_fixed_v0::routing");

static Opt<std::int32_t> InstQueueWeightAlgorithm(
        "sc_ls_fixed_v0-inst-queue-weight-algorithm",
        2,
        "Weight algorithm of instruction queue (0: index, 1: type, 2: InvDepth)",
        OptionHidden::Hidden
);
static Opt<Beat> InstQueuePeekSize(
        "sc_ls_fixed_v0-inst-queue-peek-size",
        1000,
        "Peek size of instruction queue",
        OptionHidden::Hidden
);
static Opt<Beat> StateBufferWidth(
        "sc_ls_fixed_v0-state-buffer-width",
        20,
        "Buffer width of quantum states",
        OptionHidden::Hidden
);
static Opt<std::int32_t> RouteSearcherType(
        "sc_ls_fixed_v0-route-searcher-type",
        0,
        "Route searcher strategy (0: default)",
        OptionHidden::Hidden
);
}  // namespace

bool SkipAllocate(
        const std::int64_t initial_weight,
        const std::int64_t allocate_weight,
        InstQueue::WeightAlgorithm algorithm
) {
    if (algorithm != InstQueue::WeightAlgorithm::InvDepth) {
        return false;
    }

    assert(algorithm == InstQueue::WeightAlgorithm::InvDepth);
    // Skip if the weight of allocate instruction is too large.
    // return initial_weight + static_cast<std::int64_t>(StateBufferWidth) < allocate_weight;
    return initial_weight + 1 < allocate_weight;
}
bool Routing::RunOnMachineFunction(MachineFunction& mf) {
    if (InstQueueWeightAlgorithm > 2) {
        throw std::runtime_error(
                "InstQueueWeightAlgorithm must be 0, 1, or 2 (0: index, 1: type, 2: InvDepth)."
        );
    }
    if (InstQueuePeekSize <= 1) {
        throw std::runtime_error("InstQueuePeekSize must be larger than 1.");
    }
    if (StateBufferWidth <= 5) {
        throw std::runtime_error("StateBufferWidth must be larger than 5.");
    }
    if (RouteSearcherType != 0) {
        throw std::runtime_error("RouteSearcherType must be 0 (default).");
    }

    const auto& machine = *static_cast<const ScLsFixedV0TargetMachine*>(mf.GetTarget());
    const auto machine_type = machine.machine_option.type;
    const auto& topology = machine.topology;
    const auto& option = machine.machine_option;
    const auto weight_algorithm = InstQueue::WeightAlgorithm(InstQueueWeightAlgorithm.Get());

    if (machine_type == ScLsFixedV0MachineType::DistributedDim3) {
        throw std::runtime_error(
                "SC_LS_FIXED_V0 machine type DistributedDim3 is currently not supported."
        );
    }
    if (GetMachineType(*topology) == ScLsFixedV0MachineType::DistributedDim3) {
        LOG_ERROR("topology: {}", Json(*topology).dump());
        throw std::runtime_error(
                "SC_LS_FIXED_V0 machine type DistributedDim3 is currently not supported."
        );
    }

    Validate(mf);

    // Initialize machine function.
    for (auto&& mbb : mf) {
        mbb.ConstructInverseMap();
    }

    auto symbol_generator = SymbolGenerator::New();
    {
        auto q_max = QSymbol::IdType{0};
        auto c_max = CSymbol::IdType{0};
        for (const auto& mbb : mf) {
            for (const auto& minst : mbb) {
                const auto& inst = *static_cast<const ScLsInstructionBase*>(minst.get());
                for (const auto q : inst.QTarget()) {
                    q_max = std::max(q_max, q.Id());
                }
                for (const auto c : inst.CCreate()) {
                    c_max = std::max(c_max, c.Id());
                }
            }
        }
        symbol_generator->SetQ(QSymbol{q_max + 1});
        symbol_generator->SetC(CSymbol{c_max + 1});
    }
    auto splitter = SplitMultinodeInst(*topology, symbol_generator);

    // Define states.
    auto queue = InstQueue(option, mf, weight_algorithm);
    auto route_searcher = std::unique_ptr<RouteSearcher>();
    if (RouteSearcherType == 0) {
        // Keep the simulator wired to the strategy interface, even with a single backend today.
        route_searcher = std::make_unique<DefaultRouteSearcher>();
    }
    auto simulator = ScLsSimulator(
            *topology,
            option,
            StateBufferWidth,
            symbol_generator,
            std::move(route_searcher)
    );
    auto lightest_weight_of_inst_at_beat = std::numeric_limits<std::int64_t>::max();
    auto current_beat = simulator.GetBeat();
    auto idle_beats = Beat{0};

    // Peek instructions.
    queue.Peek(2 * InstQueuePeekSize);
    if (!queue.Empty() && queue.NumRunnables() > 0) {
        lightest_weight_of_inst_at_beat = queue.GetNode(*queue.begin()).weight;
    }

    while (!queue.Empty()) {
        // DEBUG.
        if (queue.NumRunnables() == 0 && queue.NumReserved() == 0) {
            throw std::logic_error(
                    "No runnable or reserved instructions in queue, while queue is not empty."
            );
        }

        // Peek instructions if needed.
        if (!queue.IsPeekFinished() && queue.NumInsts() < InstQueuePeekSize) {
            LOG_DEBUG("Peek instruction at beat: {}", current_beat);
            queue.Peek(InstQueuePeekSize);
        }

        lightest_weight_of_inst_at_beat = queue.NumRunnables() == 0
                ? std::numeric_limits<std::int64_t>::max()
                : queue.GetNode(*queue.begin()).weight;

        // Run an instruction.
        auto update_inst_queue = false;
        auto success = false;
        for (auto* inst : queue) {
            // The allocate instruction delays its execution as much as possible.
            const auto type = inst->Type();
            if (type == ScLsInstructionType::ALLOCATE
                && SkipAllocate(
                        lightest_weight_of_inst_at_beat,
                        queue.GetNode(inst).weight,
                        weight_algorithm
                )) {
                continue;
            }
            if (machine_type == ScLsFixedV0MachineType::DistributedDim2
                && (type == ScLsInstructionType::LATTICE_SURGERY_MULTINODE
                    || type == ScLsInstructionType::MOVE || type == ScLsInstructionType::CNOT)
                && splitter.Split(
                        simulator.GetStateBuffer().GetQuantumState(current_beat),
                        mf,
                        queue,
                        inst
                )) {
                // Rebuild queue dependencies first; runnability checks below should see the split
                // form.
                update_inst_queue = true;
                break;
            }

            // Check if 'inst' is runnable.
            if (simulator.Run(current_beat, queue, mf, inst)) {
                success = true;
                idle_beats = 0;
                LOG_DEBUG("[BEAT {}] Run", current_beat);
                break;
            }
        }

        // If no instructions are runnable, step beat.
        if (!update_inst_queue && !success) {
            ++current_beat;
            ++idle_beats;
            if (queue.SetBeat(current_beat) > 0) {
                idle_beats = 0;
            }
            if (simulator.GetBeat() + StateBufferWidth / 2 < current_beat) {
                simulator.StepBeat();
            }
            lightest_weight_of_inst_at_beat = queue.NumRunnables() == 0
                    ? std::numeric_limits<std::int64_t>::max()
                    : queue.GetNode(*queue.begin()).weight;
        }

        // Throw error if some instruction is not runnable for a long beats.
        if (idle_beats >= AllowedMaxIdleBeats(option)) {
            auto ss = std::stringstream();
            ss << "Do not process any instructions for " << idle_beats << " beats\n";
            ss << "Routing pass failed to satisfy the following instructions:\n";
            for (const auto* inst : queue) {
                ss << "  * " << inst->ToString() << "\n";
            }
            throw std::runtime_error(ss.str());
        }
    }

    LOG_DEBUG("Simulator stats: {}", simulator.GetStats());

    auto changed = true;
    return changed;
}
}  // namespace qret::sc_ls_fixed_v0
