/**
 * @file qret/target/sc_ls_fixed_v0/inst_queue.h
 * @brief Instruction queue.
 */

#ifndef QRET_TARGET_SC_LS_FIXED_V0_INST_QUEUE_H
#define QRET_TARGET_SC_LS_FIXED_V0_INST_QUEUE_H

#include <set>
#include <tuple>
#include <unordered_map>

#include "qret/codegen/machine_function.h"
#include "qret/qret_export.h"
#include "qret/target/sc_ls_fixed_v0/instruction.h"
#include "qret/target/sc_ls_fixed_v0/sc_ls_fixed_v0_target_machine.h"

namespace qret::sc_ls_fixed_v0 {
/**
 * @brief Forward iterator class SC_LS_FIXED_V0's machine functions.
 */
class ScLsInstIterator {
public:
    explicit ScLsInstIterator(MachineFunction& mf);

    ScLsInstructionBase* Next();
    ScLsInstructionBase* Peek() const;
    bool IsEnd() const;

private:
    void ForwardTillValidInstruction();

    MachineFunction& mf_;
    MachineFunction::Iterator mbb_itr_;
    MachineBasicBlock::Iterator minst_itr_;
};

/**
 * @brief Instruction queue.
 * @details
 *
 * The InstdQueue class pre-fetches instructions up to N steps ahead and manages dependencies.
 * The value of N can be dynamically adjusted at runtime.
 */
class QRET_EXPORT InstQueue {
public:
    struct Node {
        std::size_t index;
        std::unordered_map<ScLsInstructionBase*, Beat> parents;
        std::unordered_map<ScLsInstructionBase*, Beat> children;
        std::uint32_t num_remaining_parents = 0;
        std::int64_t weight = 0;
    };
    enum class WeightAlgorithm : std::uint8_t {
        Index = 0,  //!< weight of node is the index of its instruction in MachineFunction.
        Type = 1,  //!< weight of node is the determined by its instruction type.
        InvDepth = 2,  //!< inverse depth of instruction DAG.
    };
    static void CalculateWeight(
            WeightAlgorithm algorithm,
            std::unordered_map<ScLsInstructionBase*, Node>& nodes
    );
    static void CalculateWeightByIndex(std::unordered_map<ScLsInstructionBase*, Node>& nodes);
    static void CalculateWeightByType(std::unordered_map<ScLsInstructionBase*, Node>& nodes);
    static void CalculateWeightByInvDepth(std::unordered_map<ScLsInstructionBase*, Node>& nodes);

    explicit InstQueue(
            const ScLsFixedV0MachineOption& option,
            MachineFunction& mf,
            WeightAlgorithm algorithm = WeightAlgorithm::InvDepth
    );

    /**
     * @brief Peek instructions and construct queue.
     * @note When peeked, recalculate weights of all nodes.
     *
     * @param size peek size.
     * @return true if peek has finished.
     * @return false otherwise.
     */
    bool Peek(std::size_t size);

    /**
     * @brief Mark instruction as done.
     * @details Delete inst from nodes_ and runnable_.
     *
     * @param inst instruction to run.
     * @return true if inst is runnable.
     * @return false otherwise.
     */
    bool Run(ScLsInstructionBase* inst);

    /**
     * @brief Mark instruction as reserved.
     * @details The InstQueue class has two methods: Run and RunFuture.
     *
     * - Run executes the instruction at the current beat.
     * - RunFuture executes the instruction at a specified future beat.
     *
     * @param beat beat to run the instruction.
     * @param inst instruction to run at beat 'beat'.
     * @return true if inst is runnable.
     * @return false otherwise.
     */
    bool RunFuture(Beat beat, ScLsInstructionBase* inst);

    /**
     * @brief Set beat.
     *
     * @param beat
     * @return std::size_t the number of instructions that are in the reserved state and run after
     * updating beat.
     */
    std::size_t SetBeat(Beat beat);

    [[nodiscard]] std::size_t NumInsts() const {
        return nodes_.size();
    }
    [[nodiscard]] std::size_t NumRunnables() const {
        return runnable_.size();
    }
    [[nodiscard]] std::size_t NumReserved() const {
        return reserved_.size();
    }

    [[nodiscard]] bool Contains(ScLsInstructionBase* inst) const {
        return nodes_.contains(inst);
    }
    [[nodiscard]] const Node& GetNode(ScLsInstructionBase* inst) const {
        return nodes_.at(inst);
    }
    [[nodiscard]] bool IsRunnable(ScLsInstructionBase* inst) const {
        return runnable_.contains(inst);
    }
    [[nodiscard]] bool IsReserved(ScLsInstructionBase* inst) const {
        for (const auto& [beat, tmp_inst] : reserved_) {
            if (tmp_inst == inst) {
                return true;
            }
        }
        return false;
    }
    [[nodiscard]] bool IsPeekFinished() const {
        return iter_.IsEnd();
    }
    /**
     * @brief Check if queue is empty.
     *
     * @return true if queue has no instruction.
     * @return false otherwise.
     */
    [[nodiscard]] bool Empty() const {
        return nodes_.empty();
    }

    /**
     * @brief Insert new instruction before 'inst'.
     *
     * @param inst
     * @param new_inst new instruction to insert in queue
     * @param len length between inst and new_inst
     */
    void InsertBefore(ScLsInstructionBase* inst, ScLsInstructionBase* new_inst, Beat len = 0);
    /**
     * @brief Insert new instruction after 'inst'.
     *
     * @param inst
     * @param new_inst new instruction to insert in queue
     * @param len length between inst and new_inst
     */
    void InsertAfter(ScLsInstructionBase* inst, ScLsInstructionBase* new_inst, Beat len = 0);
    /**
     * @brief Replace inst with new instructions.
     *
     * @param inst
     * @param new_insts
     * @param len length between new_insts and children of inst
     */
    void Replace(
            ScLsInstructionBase* inst,
            const std::vector<ScLsInstructionBase*>& new_insts,
            Beat len
    );

    auto begin() {
        return runnable_.begin();
    }
    auto begin() const {
        return runnable_.begin();
    }
    auto end() {
        return runnable_.end();
    }
    auto end() const {
        return runnable_.end();
    }

private:
    const ScLsFixedV0MachineOption& option_;
    ScLsInstIterator iter_;  //!< iterator for peek instructions.
    WeightAlgorithm algorithm_;

    std::size_t index_ = 0;

    std::unordered_map<QSymbol, ScLsInstructionBase*> qmap_ = {};
    std::unordered_map<CSymbol, ScLsInstructionBase*> cmap_ = {};
    std::unordered_map<ScLsInstructionBase*, Node> nodes_ = {};

    struct CompareWeight {
        const std::unordered_map<ScLsInstructionBase*, Node>* node = nullptr;

        [[nodiscard]] bool operator()(ScLsInstructionBase* lhs, ScLsInstructionBase* rhs) const {
            using T = std::tuple<std::int64_t, std::size_t, ScLsInstructionBase*>;
            const auto& lhs_node = node->at(lhs);
            const auto& rhs_node = node->at(rhs);
            return T(lhs_node.weight, lhs_node.index, lhs)
                    < T(rhs_node.weight, rhs_node.index, rhs);
        }
    };

    std::set<ScLsInstructionBase*, CompareWeight> runnable_ =
            {};  //!< runnable instructions sorted by weight.

    std::set<std::pair<Beat, ScLsInstructionBase*>> reserved_ = {};
};
}  // namespace qret::sc_ls_fixed_v0

#endif  // QRET_TARGET_SC_LS_FIXED_V0_INST_QUEUE_H
