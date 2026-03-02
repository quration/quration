/**
 * @file qret/target/sc_ls_fixed_v0/calc_compile_info.cpp
 * @brief Calculate compile information.
 */

#include "qret/target/sc_ls_fixed_v0/calc_compile_info.h"

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <cmath>
#include <fstream>
#include <limits>
#include <numeric>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

#include "qret/base/cast.h"
#include "qret/base/graph.h"
#include "qret/base/log.h"
#include "qret/base/option.h"
#include "qret/codegen/machine_function.h"
#include "qret/target/sc_ls_fixed_v0/compile_info.h"
#include "qret/target/sc_ls_fixed_v0/constants.h"
#include "qret/target/sc_ls_fixed_v0/inst_queue.h"
#include "qret/target/sc_ls_fixed_v0/instruction.h"
#include "qret/target/sc_ls_fixed_v0/routing.h"
#include "qret/target/sc_ls_fixed_v0/sc_ls_fixed_v0_target_machine.h"
#include "qret/target/sc_ls_fixed_v0/state.h"
#include "qret/target/sc_ls_fixed_v0/symbol.h"
#include "qret/target/sc_ls_fixed_v0/validation.h"

namespace qret::sc_ls_fixed_v0 {
namespace {
// Register pass to registry
static auto PassWithoutTopology = RegisterPass<CompileInfoWithoutTopology>(
        "CompileInfoWithoutTopology",
        "sc_ls_fixed_v0::calc_info_without_topology"
);
static auto PassWithTopology = RegisterPass<CompileInfoWithTopology>(
        "CompileInfoWithTopology",
        "sc_ls_fixed_v0::calc_info_with_topology"
);
static auto PassWithQEC = RegisterPass<CompileInfoWithQecResourceEstimation>(
        "CompileInfoWithQECResourceEstimation",
        "sc_ls_fixed_v0::calc_info_with_qec_resource_estimation"
);
static auto PassInit =
        RegisterPass<InitCompileInfo>("InitCompileInfo", "sc_ls_fixed_v0::init_compile_info");
static auto PassDump =
        RegisterPass<DumpCompileInfo>("DumpCompileInfo", "sc_ls_fixed_v0::dump_compile_info");

static Opt<std::string> DumpCompileInfoToJson(
        "sc_ls_fixed_v0_dump_compile_info_to_json",
        "",
        "Dump compile information to json",
        OptionHidden::NotHidden
);
static Opt<std::string> DumpCompileInfoToMarkdown(
        "sc_ls_fixed_v0_dump_compile_info_to_markdown",
        "",
        "Dump compile information to markdown",
        OptionHidden::NotHidden
);
}  // namespace

DepGraph::DepGraph(const MachineFunction& mf) {
    const auto& target = *static_cast<const ScLsFixedV0TargetMachine*>(mf.GetTarget());

    // Construct graph (DAG)
    graph_ = DiGraph();
    auto q2id = std::map<QSymbol, DiGraph::IdType>();
    auto c2id = std::map<CSymbol, DiGraph::IdType>();
    for (CSymbol::IdType i = 0; i < NumReservedCSymbols; ++i) {
        c2id[CSymbol{i}] = std::numeric_limits<DiGraph::IdType>::max();
    }
    auto measurement_c = std::unordered_set<CSymbol>{};

    auto add_edge = [&q2id, &c2id, &measurement_c, &target, this](
                            const DiGraph::IdType id,
                            const ScLsInstructionBase& inst
                    ) {
        // Check qtarget.
        for (const auto& q : inst.QTarget()) {
            if (q2id.contains(q)) {
                const auto old = q2id.at(q);
                graph_.AddEdge(old, id);
            }
            q2id[q] = id;
        }
        if (const auto* i = DynCast<Move>(&inst)) {
            const auto src = i->Qubit();
            const auto dst = i->QDest();
            if (src != dst) {
                q2id.erase(src);
                q2id[dst] = id;
            }
        } else if (const auto* i = DynCast<MoveTrans>(&inst)) {
            const auto src = i->Qubit();
            const auto dst = i->QDest();
            if (src != dst) {
                q2id.erase(src);
                q2id[dst] = id;
            }
        }

        // Check condition.
        for (const auto& c : inst.Condition()) {
            const auto old = c2id.at(c);
            if (measurement_c.contains(c)) {
                graph_.AddEdge(old, id, target.machine_option.reaction_time);
            } else if (c.Id() >= NumReservedCSymbols) {
                graph_.AddEdge(old, id, 0);
            }
        }

        // Check CDepend.
        for (const auto& c : inst.CDepend()) {
            if (!c2id.contains(c)) {
                throw std::runtime_error(
                        fmt::format(
                                "Compile info calculation error: Dependant classical symbol {} is "
                                "not allocated ({}) ",
                                c.ToString(),
                                inst.ToString()
                        )
                );
            }
            const auto old = c2id.at(c);
            if (measurement_c.contains(c)) {
                graph_.AddEdge(old, id, target.machine_option.reaction_time);
            } else if (c.Id() >= NumReservedCSymbols) {
                graph_.AddEdge(old, id, 0);
            }
        }

        // Check CCreate.
        for (const auto& c : inst.CCreate()) {
            if (c2id.contains(c)) {
                throw std::runtime_error(
                        fmt::format(
                                "Compile info calculation error: Cannot store values "
                                "into the same "
                                "classical symbol ({}) ({}).",
                                c.ToString(),
                                inst.ToString()
                        )
                );
            }
            c2id[c] = id;

            if (inst.IsMeasurement()) {
                measurement_c.emplace(c);
            }
        }
    };

    for (const auto& bb : mf) {
        for (const auto& minst : bb) {
            const auto& inst = *static_cast<const ScLsInstructionBase*>(minst.get());
            const auto id = static_cast<DiGraph::IdType>(ptr2id_.size());
            ptr2id_[&inst] = id;
            id2ptr_[id] = &inst;
            graph_.AddNode(id, inst.Latency());
            add_edge(id, inst);
        }
    }

    // Sort topologically
    const auto is_dag = graph_.Topsort();
    if (!is_dag) {
        throw std::logic_error("instruction graph is not DAG");
    }
}

class StateWithoutTopology {
public:
    StateWithoutTopology() {
        // Add reserved symbols.
        for (CSymbol::IdType i = 0; i < NumReservedCSymbols; ++i) {
            c_[CSymbol{i}] = Beat{0};
        }
    }

    void Step(const ScLsFixedV0MachineOption& option) {
        for (auto&& [_, state] : mf_state_) {
            StepMagicFactoryState(option, state);
        }
        for (auto&& [_, state] : ef_state_) {
            StepEntanglementFactoryState(option, state);
        }
    }

    void AddQubit(Beat b, QSymbol q) {
        if (q_.contains(q)) {
            throw std::runtime_error(
                    fmt::format(
                            "Compile info calculation error: Cannot allocate already allocated "
                            "qubit symbol ({}).",
                            q.ToString()
                    )
            );
        }
        q_[q] = b;
    }
    void DelQubit(QSymbol q) {
        if (!q_.contains(q)) {
            throw std::runtime_error(
                    fmt::format(
                            "Compile info calculation error: Cannot use not allocated qubit symbol "
                            "({}).",
                            q
                    )
            );
        }
        q_.erase(q);
    }
    /**
     * @brief CSymbol @p c becomes available at beat @p available.
     *
     * @param available
     * @param c
     */
    void AddRegister(Beat available, CSymbol c) {
        if (c_.contains(c)) {
            throw std::runtime_error(
                    fmt::format(
                            "Compile info calculation error: Cannot store values into the same "
                            "classical symbol ({}).",
                            c.ToString()
                    )
            );
        }
        c_[c] = available;
    }
    void AddMagicFactory(const ScLsFixedV0MachineOption& option, MSymbol m) {
        if (mf_state_.contains(m)) {
            throw std::runtime_error(
                    fmt::format(
                            "Compile info calculation error: Cannot allocate already allocated "
                            "magic state factory ({}).",
                            m.ToString()
                    )
            );
        }
        mf_state_[m] = MagicFactoryState::Empty(option.magic_factory_seed_offset + m.Id());
    }
    void AddEntanglementFactory(ESymbol e1, ESymbol e2) {
        for (const auto e : {e1, e2}) {
            if (ef_state_.contains(e)) {
                throw std::runtime_error(
                        fmt::format(
                                "Compile info calculation error: Cannot allocate already allocated "
                                "entanglement factory ({}).",
                                e.ToString()
                        )
                );
            }
        }
        ef_state_[e1] = EntanglementFactoryState::Empty();
        ef_state_[e2] = EntanglementFactoryState::Empty();
        ef_pair_[e1] = e2;
        ef_pair_[e2] = e1;
    }

    bool IsConditionSatisfied(Beat b, const std::list<CSymbol>& condition) const {
        for (const auto c : condition) {
            if (!IsCAvailable(b, c)) {
                return false;
            }
        }
        return true;
    }

    bool IsQAvailable(Beat b, QSymbol q) const {
        if (!q_.contains(q)) {
            throw std::runtime_error(
                    fmt::format(
                            "Compile info calculation error: Cannot use not allocated qubit symbol "
                            "({}).",
                            q.ToString()
                    )
            );
        }
        return q_.at(q) <= b;
    }
    bool IsCAvailable(Beat b, CSymbol c) const {
        if (!c_.contains(c)) {
            throw std::runtime_error(
                    fmt::format(
                            "Compile info calculation error: Cannot use not allocated classical "
                            "symbol ({}).",
                            c.ToString()
                    )
            );
        }
        return c_.at(c) <= b;
    }
    bool IsMagicAvailable() const {
        for (const auto& [_, state] : mf_state_) {
            if (state.IsAvailable()) {
                return true;
            }
        }
        return false;
    }
    bool IsEntanglementAvailable(EHandle eh) const {
        if (eh_es_.contains(eh)) {
            return true;
        }
        for (const auto& [_, state] : ef_state_) {
            if (state.IsAvailable()) {
                return true;
            }
        }
        return false;
    }

    bool TryUseTarget(Beat b, const ScLsInstructionBase* inst) {
        for (const auto q : inst->QTarget()) {
            if (!IsQAvailable(b, q)) {
                return false;
            }
        }
        if (inst->UseMagicState() && !IsMagicAvailable()) {
            return false;
        }
        if (inst->UseEntanglement()) {
            const auto eh = inst->EHTarget().front();
            if (!IsEntanglementAvailable(eh)) {
                return false;
            }
        }

        // Use.
        if (const auto* i = DynCast<Move>(inst)) {
            const auto src = i->Qubit();
            const auto dst = i->QDest();
            UseQ(b + inst->Latency(), dst);
            if (src != dst) {
                DelQubit(src);
            }
        } else if (const auto* i = DynCast<MoveTrans>(inst)) {
            const auto src = i->Qubit();
            const auto dst = i->QDest();
            UseQ(b + inst->Latency(), dst);
            if (src != dst) {
                DelQubit(src);
            }
        } else {
            for (const auto q : inst->QTarget()) {
                UseQ(b + inst->Latency(), q);
            }
        }
        if (inst->UseMagicState()) {
            UseMagic();
        }
        if (inst->UseEntanglement()) {
            const auto eh = inst->EHTarget().front();
            UseEntanglementAvailable(eh);
        }

        return true;
    }

private:
    void UseQ(Beat use_until, QSymbol q) {
        q_[q] = use_until;
    }
    void UseMagic() {
        for (auto&& [m, state] : mf_state_) {
            if (state.TryUseMagic()) {
                return;
            }
        }

        throw std::runtime_error("Compile info calculation error: Cannot use empty magic factory.");
    }
    void UseEntanglementAvailable(EHandle eh) {
        // Handle is already created.
        if (eh_es_.contains(eh)) {
            const auto [e1, e2] = eh_es_.at(eh);
            ef_state_.at(e1).UseHandle(eh);
            ef_state_.at(e2).UseHandle(eh);
            return;
        }

        // Use entanglement pair and create entanglement handle.
        for (auto&& [e1, state] : ef_state_) {
            if (state.IsAvailable()) {
                state.AddHandle(eh);
                const auto e2 = ef_pair_.at(e1);
                ef_state_.at(e2).AddHandle(eh);
                eh_es_[eh] = {e1, e2};
                return;
            }
        }

        throw std::runtime_error(
                "Compile info calculation error: Cannot use empty entanglement factory."
        );
    }

    std::unordered_map<QSymbol, Beat> q_;
    std::unordered_map<CSymbol, Beat> c_;
    std::unordered_map<MSymbol, MagicFactoryState> mf_state_;
    std::unordered_map<ESymbol, EntanglementFactoryState> ef_state_;
    std::unordered_map<ESymbol, ESymbol> ef_pair_;
    std::unordered_map<EHandle, std::pair<ESymbol, ESymbol>> eh_es_;
};

Beat CalcRuntimeWithoutTopology(MachineFunction& mf) {
    static constexpr auto InstQueuePeekSize = 2000;

    const auto& machine = *static_cast<const ScLsFixedV0TargetMachine*>(mf.GetTarget());
    const auto& option = machine.machine_option;

    auto state = StateWithoutTopology{};
    auto queue = InstQueue(option, mf, InstQueue::WeightAlgorithm::InvDepth);
    queue.Peek(InstQueuePeekSize);
    auto current_beat = Beat{0};
    auto runtime = Beat{0};
    auto idle_beats = Beat{0};
    while (!queue.Empty()) {
        // Peek instructions if needed.
        if (!queue.IsPeekFinished() && queue.NumInsts() < InstQueuePeekSize) {
            queue.Peek(InstQueuePeekSize);
        }

        ScLsInstructionBase* run_instruction = nullptr;
        for (auto* base_inst : queue) {
            // Check if base_inst is runnable or note.
            // If runnable, update state, set run_instruction and break this loop.

            if (!state.IsConditionSatisfied(current_beat, base_inst->Condition())) {
                continue;
            }

            if (auto* inst = DynCast<Allocate>(base_inst)) {
                const auto q = inst->Qubit();
                state.AddQubit(current_beat, q);
                run_instruction = base_inst;
            } else if (auto* inst = DynCast<DeAllocate>(base_inst)) {
                const auto q = inst->Qubit();
                state.DelQubit(q);
                run_instruction = base_inst;
            } else if (auto* inst = DynCast<AllocateMagicFactory>(base_inst)) {
                const auto m = inst->MagicFactory();
                state.AddMagicFactory(option, m);
                run_instruction = base_inst;
            } else if (auto* inst = DynCast<AllocateEntanglementFactory>(base_inst)) {
                const auto e1 = inst->EntanglementFactory1();
                const auto e2 = inst->EntanglementFactory2();
                state.AddEntanglementFactory(e1, e2);
                run_instruction = base_inst;
            } else if (auto* inst = DynCast<ClassicalOperation>(base_inst)) {
                auto runnable = true;
                for (const auto c : inst->RegisterList()) {
                    if (!state.IsCAvailable(current_beat, c)) {
                        runnable = false;
                        break;
                    }
                }
                if (runnable) {
                    state.AddRegister(current_beat, inst->CDest());
                    run_instruction = base_inst;
                }
            } else {
                if (state.TryUseTarget(current_beat, base_inst)) {
                    run_instruction = base_inst;
                    if (!base_inst->CCreate().empty()) {
                        state.AddRegister(
                                current_beat + option.reaction_time + base_inst->StartCorrecting(),
                                base_inst->CCreate().front()
                        );
                    }
                }
            }

            if (run_instruction != nullptr) {
                break;
            }
        }

        if (run_instruction != nullptr) {
            queue.Run(run_instruction);
            idle_beats = 0;
            runtime = std::max(runtime, current_beat + run_instruction->Latency());
        } else {
            // If no instructions are runnable, step beat.
            ++current_beat;
            ++idle_beats;
            queue.SetBeat(current_beat);
            state.Step(option);
        }

        // Throw error if some instruction is not runnable for a long beats.
        if (idle_beats >= AllowedMaxIdleBeats(option)) {
            auto ss = std::stringstream();
            ss << "Compile info calculation error: Do not process any instructions for "
               << idle_beats << " beats\n";
            ss << "Routing pass failed to satisfy the following instructions:\n";
            for (const auto* inst : queue) {
                ss << "  * " << inst->ToString() << "\n";
            }
            throw std::runtime_error(ss.str());
        }
    }

    return runtime;
}

TimeSeries::TimeSeries(const MachineFunction& mf) {
    const auto& target = *static_cast<const ScLsFixedV0TargetMachine*>(mf.GetTarget());

    const auto runtime = [&mf]() {
        auto runtime = std::uint64_t{0};
        for (const auto& bb : mf) {
            for (const auto& minst : bb) {
                const auto& inst = *static_cast<const ScLsInstructionBase*>(minst.get());
                runtime = std::max(runtime, inst.Metadata().beat + inst.Latency());
            }
        }
        return runtime;
    }();

    // Initialize beat2chip_ and beat2inst_
    beat2inst_.resize(runtime + 1);
    beat2chip_.resize(runtime + 1);

    // Set beat2inst_
    for (const auto& bb : mf) {
        for (const auto& minst : bb) {
            const auto& inst = *static_cast<const ScLsInstructionBase*>(minst.get());
            const auto& metadata = inst.Metadata();

            const auto latency = inst.Latency() == 0 ? 1 : inst.Latency();
            for (auto beat = metadata.beat; beat < metadata.beat + latency; ++beat) {
                beat2inst_[beat].emplace_back(&inst);
            }
        }
    }

    // Calculate beat2chip_
    for (auto beat = Beat{0}; beat < beat2inst_.size(); ++beat) {
        const auto& insts = beat2inst_[beat];

        auto& chip_info = beat2chip_[beat];
        if (beat == 0) {
            auto space = std::int32_t{0};
            for (const auto& grid : *target.topology) {
                space += grid.GetMaxX() * grid.GetMaxY() * grid.GetZSize();
                for (const auto& plane : grid) {
                    space -= plane.NumBanned();
                }
            }
            chip_info.space = static_cast<std::uint32_t>(space);
        } else {
            chip_info = beat2chip_[beat - 1];
            chip_info.used_ancilla_count = 0;
        }

        for (const auto& inst : insts) {
            if (inst->Type() == ScLsInstructionType::ALLOCATE) {
                chip_info.q_symb++;
            } else if (inst->Type() == ScLsInstructionType::ALLOCATE_MAGIC_FACTORY) {
                chip_info.m_symb++;
            } else if (inst->Type() == ScLsInstructionType::ALLOCATE_ENTANGLEMENT_FACTORY) {
                chip_info.e_symb += 2;
            } else if (inst->Type() == ScLsInstructionType::DEALLOCATE) {
                chip_info.q_symb--;
            }
            chip_info.used_ancilla_count += inst->CountAncillae();
        }
    }
}

bool CompileInfoWithoutTopology::RunOnMachineFunction(MachineFunction& mf) {
    LOG_INFO("Calculate compile information without topology.");
    if (!mf.HasCompileInfo()) {
        LOG_INFO("Initialize compile information.");
        InitCompileInfo().RunOnMachineFunction(mf);
    }

    Validate(mf);

    const auto& target = *static_cast<const ScLsFixedV0TargetMachine*>(mf.GetTarget());
    auto& compile_info = *static_cast<ScLsFixedV0CompileInfo*>(mf.GetMutCompileInfo());

    // gate_count, gate_count_dict, magic_state_consumption_count, magic_factory_count
    compile_info.gate_count = 0;
    compile_info.gate_count_dict.clear();
    compile_info.magic_state_consumption_count = 0;
    compile_info.magic_factory_count = 0;
    compile_info.entanglement_consumption_count = 0;
    compile_info.entanglement_factory_count = 0;
    for (const auto& bb : mf) {
        for (const auto& minst : bb) {
            compile_info.gate_count++;

            const auto& inst = *static_cast<const ScLsInstructionBase*>(minst.get());
            const auto type = inst.Type();
            if (compile_info.gate_count_dict.contains(type)) {
                compile_info.gate_count_dict[inst.Type()]++;
            } else {
                compile_info.gate_count_dict.insert({type, 1});
            }

            if (inst.UseMagicState()) {
                compile_info.magic_state_consumption_count++;
            }
            if (inst.UseEntanglement()) {
                compile_info.entanglement_consumption_count += inst.CountEntanglement();
            }

            if (type == ScLsInstructionType::ALLOCATE_MAGIC_FACTORY) {
                compile_info.magic_factory_count++;
            }
            if (type == ScLsInstructionType::ALLOCATE_ENTANGLEMENT_FACTORY) {
                compile_info.entanglement_factory_count++;
            }
        }
    }

    // runtime_estimation_magic_state_consumption_count
    compile_info.runtime_estimation_magic_state_consumption_count =
            compile_info.magic_state_consumption_count
            * target.machine_option.magic_generation_period;

    if (compile_info.entanglement_consumption_count % 2 != 0) {
        LOG_ERROR(
                "Entanglement consumption count: {}",
                compile_info.entanglement_consumption_count
        );
        throw std::logic_error("Entanglement consumption count must be even.");
    }
    compile_info.entanglement_consumption_count /= 2;
    // runtime_estimation_entanglement_consumption_count
    compile_info.runtime_estimation_entanglement_consumption_count =
            compile_info.entanglement_consumption_count
            * target.machine_option.entanglement_generation_period;

    // dependency graph of instruction
    auto graph = DepGraph(mf);

    // runtime_without_topology
    compile_info.runtime_without_topology = CalcRuntimeWithoutTopology(mf);

    // gate_depth
    for (const auto& bb : mf) {
        for (const auto& minst : bb) {
            const auto& inst = *static_cast<const ScLsInstructionBase*>(minst.get());
            graph.SetInstWeight(inst, inst.Latency() == 0 ? 0 : 1);
        }
    }
    compile_info.gate_depth = graph.CalcHeaviest();

    // magic_state_consumption_depth
    for (const auto& bb : mf) {
        for (const auto& minst : bb) {
            const auto& inst = *static_cast<const ScLsInstructionBase*>(minst.get());
            graph.SetInstWeight(inst, inst.UseMagicState() ? 1 : 0);
        }
    }
    compile_info.magic_state_consumption_depth = graph.CalcHeaviest();

    // runtime_estimation_magic_state_consumption_depth
    compile_info.runtime_estimation_magic_state_consumption_depth =
            compile_info.magic_state_consumption_depth
            * target.machine_option.magic_generation_period;

    // entanglement_consumption_depth
    for (const auto& bb : mf) {
        for (const auto& minst : bb) {
            const auto& inst = *static_cast<const ScLsInstructionBase*>(minst.get());
            graph.SetInstWeight(inst, inst.UseEntanglement() ? 1 : 0);
        }
    }
    compile_info.entanglement_consumption_depth = graph.CalcHeaviest();

    // runtime_estimation_entanglement_consumption_depth
    compile_info.runtime_estimation_entanglement_consumption_depth =
            compile_info.entanglement_consumption_depth
            * target.machine_option.entanglement_generation_period;

    // measurement_feedback_count, runtime_estimation_measurement_feedback_count
    compile_info.measurement_feedback_count = [&mf]() {
        auto feedback = std::set<CSymbol>();
        for (const auto& bb : mf) {
            for (const auto& minst : bb) {
                const auto& inst = *static_cast<const ScLsInstructionBase*>(minst.get());
                for (const auto& c : inst.Condition()) {
                    feedback.insert(c);
                }
            }
        }
        return feedback.size();
    }();
    compile_info.runtime_estimation_measurement_feedback_count =
            compile_info.measurement_feedback_count * target.machine_option.reaction_time;

    // measurement_feedback_depth
    {
        graph.SetAllLength(0);

        auto c2inst = std::map<CSymbol, const ScLsInstructionBase*>();
        for (CSymbol::IdType i = 0; i < NumReservedCSymbols; ++i) {
            c2inst[CSymbol{i}] = nullptr;
        }
        for (const auto& bb : mf) {
            for (const auto& minst : bb) {
                const auto& inst = *static_cast<const ScLsInstructionBase*>(minst.get());

                // If condition is measurement result, set length to 1.
                for (const auto& c : inst.Condition()) {
                    const auto* from = c2inst.at(c);
                    if (from != nullptr && from->IsMeasurement()) {
                        // Measurement.
                        graph.SetLength(*from, inst, 1);
                    }
                }

                // Check CDepend.
                for (const auto& c : inst.CDepend()) {
                    const auto* from = c2inst.at(c);
                    if (from != nullptr && from->IsMeasurement()) {
                        // c is measurement result.
                        graph.SetLength(*from, inst, 1);
                    }
                }

                // Check CCreate.
                for (const auto& c : inst.CCreate()) {
                    c2inst[c] = &inst;
                }
            }
        }
    }
    compile_info.measurement_feedback_depth = graph.CalcLongest();

    // runtime_estimation_measurement_feedback_depth
    compile_info.runtime_estimation_measurement_feedback_depth =
            compile_info.measurement_feedback_depth * target.machine_option.reaction_time;

    return false;
}

bool CompileInfoWithTopology::RunOnMachineFunction(MachineFunction& mf) {
    LOG_INFO("Calculate compile information with topology.");
    if (!mf.HasCompileInfo()) {
        LOG_INFO("Initialize compile information.");
        InitCompileInfo().RunOnMachineFunction(mf);
    }

    Validate(mf);

    auto& compile_info = *static_cast<ScLsFixedV0CompileInfo*>(mf.GetMutCompileInfo());

    const auto time_series = TimeSeries(mf);

    // runtime
    compile_info.runtime = time_series.GetRuntime();

    if (compile_info.runtime == 0) {
        return false;
    }

    // gate_throughput, measurement_feedback_rate, magic_state_consumption_rate,
    // entanglement_consumption_rate
    struct FeedbackInfo {
        Beat beat;
        bool counted;
    };
    auto feedback_info = std::map<CSymbol, FeedbackInfo>();
    for (std::uint64_t i = 0; i < NumReservedCSymbols; ++i) {
        feedback_info.emplace(CSymbol{i}, FeedbackInfo{.beat = 0, .counted = false});
    }
    compile_info.gate_throughput.resize(compile_info.runtime, 0);
    compile_info.measurement_feedback_rate.resize(compile_info.runtime, 0);
    compile_info.magic_state_consumption_rate.resize(compile_info.runtime, 0);
    compile_info.entanglement_consumption_rate.resize(compile_info.runtime, 0);
    for (auto beat = Beat{0}; beat < compile_info.runtime; ++beat) {
        const auto& insts = time_series.GetInstructions(beat);

        // gate_throughput
        compile_info.gate_throughput[beat] = insts.size();

        // measurement_feedback_rate
        for (const auto& inst : insts) {
            for (const auto& c : inst->CCreate()) {
                if (feedback_info.contains(c)) {
                    throw std::runtime_error(
                            fmt::format(
                                    "Store the measurement results to the same c-symbol ({}) "
                                    "more than once",
                                    c.ToString()
                            )
                    );
                }
                feedback_info.emplace(c, FeedbackInfo{beat + inst->StartCorrecting(), false});
            }
        }
        for (const auto& inst : insts) {
            for (const auto& c : inst->Condition()) {
                if (!feedback_info.contains(c)) {
                    throw std::runtime_error(
                            fmt::format(
                                    "Instruction ({}) is conditioned by unknown c-symbol ({})",
                                    inst->ToString(),
                                    c.ToString()
                            )
                    );
                }

                auto& info = feedback_info.at(c);
                if (!info.counted) {
                    compile_info.measurement_feedback_rate[info.beat]++;
                    info.counted = true;
                }
            }
        }

        // magic_state_consumption_rate, entanglement_consumption_rate
        for (const auto& inst : insts) {
            if (inst->UseMagicState()) {
                compile_info.magic_state_consumption_rate[beat]++;
            }
            if (inst->UseEntanglement()) {
                compile_info.entanglement_consumption_rate[beat] += inst->CountEntanglement();
            }
        }
    }

    // chip_cell_count
    compile_info.chip_cell_count = time_series.GetChipInfo(0).ChipCellCount();

    // chip_cell_algorithmic_qubit, chip_cell_algorithmic_qubit_ratio, chip_cell_active_qubit_area,
    // chip_cell_active_qubit_area_ratio
    compile_info.chip_cell_algorithmic_qubit.resize(compile_info.runtime);
    compile_info.chip_cell_algorithmic_qubit_ratio.resize(compile_info.runtime);
    compile_info.chip_cell_active_qubit_area.resize(compile_info.runtime);
    compile_info.chip_cell_active_qubit_area_ratio.resize(compile_info.runtime);
    for (auto beat = Beat{0}; beat < compile_info.runtime; ++beat) {
        compile_info.chip_cell_algorithmic_qubit[beat] =
                time_series.GetChipInfo(beat).ChipCellAlgorithmicQubit();
        compile_info.chip_cell_algorithmic_qubit_ratio[beat] =
                time_series.GetChipInfo(beat).ChipCellAlgorithmicQubitRatio();
        compile_info.chip_cell_active_qubit_area[beat] =
                time_series.GetChipInfo(beat).ChipCellActiveQubitArea();
        compile_info.chip_cell_active_qubit_area_ratio[beat] =
                time_series.GetChipInfo(beat).ChipCellActiveQubitAreaRatio();
    }

    // qubit_volume
    compile_info.qubit_volume = std::accumulate(
            compile_info.chip_cell_active_qubit_area.begin(),
            compile_info.chip_cell_active_qubit_area.end(),
            std::uint64_t{0}
    );

    return false;
}

std::uint64_t CompileInfoWithQecResourceEstimation::EstimateMinimumCodeDistance(
        double p,
        double lambda,
        double eps,
        std::uint64_t active_volume
) {
    const auto valid = p > 0.0 && eps > 0.0 && active_volume > 0 && lambda > 0.0 && lambda < 1.0;
    if (!valid) {
        const auto msg = fmt::format(
                "Invalid parameters for estimating code distance: "
                "p={}, lambda={}, eps={}, active_volume={}",
                p,
                lambda,
                eps,
                active_volume
        );
        throw std::runtime_error(msg);
    }

    const auto ratio = eps / (p * static_cast<double>(active_volume));
    if (!std::isfinite(ratio) || ratio <= 0.0) {
        const auto msg = fmt::format(
                "Invalid ratio for estimating code distance: ratio={}, p={}, lambda={}, "
                "eps={}, "
                "active_volume={}",
                ratio,
                p,
                lambda,
                eps,
                active_volume
        );
        throw std::runtime_error(msg);
    }
    if (ratio >= 1.0) {
        return 1;
    }

    const auto log_lambda = std::log(lambda);
    const auto log_ratio = std::log(ratio);
    if (!std::isfinite(log_lambda) || !std::isfinite(log_ratio) || log_lambda >= 0.0) {
        const auto msg = fmt::format(
                "Invalid logs for estimating code distance: log_lambda={}, log_ratio={}",
                log_lambda,
                log_ratio
        );
        throw std::runtime_error(msg);
    }

    const auto exponent = log_ratio / log_lambda;
    const auto ceil_exp = static_cast<std::uint64_t>(std::ceil(exponent));
    return (2 * ceil_exp) + 1;
}
double CompileInfoWithQecResourceEstimation::EstimateExecutionTimeSec(
        std::uint64_t d,
        std::uint64_t runtime,
        double t_cycle
) {
    return static_cast<double>(runtime) * static_cast<double>(d) * t_cycle;
}
std::uint64_t CompileInfoWithQecResourceEstimation::EstimatePhysicalQubitCount(
        std::uint64_t d,
        std::uint64_t chip_cell_count
) {
    return d * d * chip_cell_count * 2;
}

bool CompileInfoWithQecResourceEstimation::RunOnMachineFunction(MachineFunction& mf) {
    auto& compile_info = *static_cast<ScLsFixedV0CompileInfo*>(mf.GetMutCompileInfo());
    const auto& target = *static_cast<const ScLsFixedV0TargetMachine*>(mf.GetTarget());
    const auto& option = target.machine_option;

    try {
        compile_info.code_distance = EstimateMinimumCodeDistance(
                option.physical_error_rate,
                option.drop_rate,
                option.allowed_failure_prob,
                compile_info.qubit_volume
        );
        compile_info.execution_time_sec = EstimateExecutionTimeSec(
                compile_info.code_distance,
                compile_info.runtime,
                option.code_cycle_time_sec
        );
        compile_info.num_physical_qubits = EstimatePhysicalQubitCount(
                compile_info.code_distance,
                compile_info.chip_cell_count
        );
    } catch (const std::runtime_error& error) {
        LOG_ERROR("{}", error.what());
    }

    return false;
}

bool InitCompileInfo::RunOnMachineFunction(MachineFunction& mf) {
    LOG_INFO("Initialize compile information");
    mf.InitializeCompileInfo(std::unique_ptr<ScLsFixedV0CompileInfo>(new ScLsFixedV0CompileInfo()));

    const auto& target = *static_cast<const ScLsFixedV0TargetMachine*>(mf.GetTarget());
    auto& compile_info = *static_cast<ScLsFixedV0CompileInfo*>(mf.GetMutCompileInfo());

    // Initialize constants.
    compile_info.use_magic_state_cultivation = target.machine_option.use_magic_state_cultivation;
    compile_info.magic_factory_seed_offset = target.machine_option.magic_factory_seed_offset;
    compile_info.magic_generation_period = target.machine_option.magic_generation_period;
    compile_info.prob_magic_state_creation = target.machine_option.prob_magic_state_creation;
    compile_info.maximum_magic_state_stock = target.machine_option.maximum_magic_state_stock;
    compile_info.entanglement_generation_period =
            target.machine_option.entanglement_generation_period;
    compile_info.maximum_entangled_state_stock =
            target.machine_option.maximum_entangled_state_stock;
    compile_info.reaction_time = target.machine_option.reaction_time;
    compile_info.topology = target.topology;

    return false;
}

bool DumpCompileInfo::RunOnMachineFunction(MachineFunction& mf) {
    LOG_INFO("Dump compile information");
    if (!mf.HasCompileInfo()) {
        LOG_ERROR(
                "MachineFunction does not have compile information. Run "
                "sc_ls_fixed_v0::calc_info_without_topology and/or "
                "sc_ls_fixed_v0::calc_info_with_topology before "
                "running sc_ls_fixed_v0::dump_compile_info."
        );
        return false;
    }

    const auto& compile_info = *static_cast<const ScLsFixedV0CompileInfo*>(mf.GetCompileInfo());
    std::cout << compile_info << std::endl;

    if (!DumpCompileInfoToJson.Get().empty()) {
        const auto& path = DumpCompileInfoToJson.Get();
        auto fs = std::ofstream(path);
        if (fs.good()) {
            fs << compile_info.Json() << std::endl;
        } else {
            LOG_ERROR("Failed to open: {}", path);
        }
    }
    if (!DumpCompileInfoToMarkdown.Get().empty()) {
        const auto& path = DumpCompileInfoToMarkdown.Get();
        auto fs = std::ofstream(DumpCompileInfoToMarkdown.Get());
        if (fs.good()) {
            fs << compile_info.Markdown() << std::endl;
        } else {
            LOG_ERROR("Failed to open: {}", path);
        }
    }
    return false;
}
}  // namespace qret::sc_ls_fixed_v0
