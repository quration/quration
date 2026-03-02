/**
 * @file qret/codegen/machine_function.h
 * @brief Machine function.
 * @details This file defines classes for target specific machine functions.
 */

#ifndef QRET_CODEGEN_MACHINE_FUNCTION_H
#define QRET_CODEGEN_MACHINE_FUNCTION_H

#include <cstddef>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "qret/codegen/compile_info.h"
#include "qret/ir/function.h"
#include "qret/qret_export.h"

namespace qret {
// Forward declaration
class TargetMachine;
class MachineFunction;

/**
 * @brief Basic representation for all target dependent machine instructions used by the backend.
 */
class QRET_EXPORT MachineInstruction {
public:
    virtual ~MachineInstruction() = default;
    [[nodiscard]] virtual std::string ToString() const = 0;
};
/**
 * @brief Collect the sequence of machine instructions for a basic block.
 */
class QRET_EXPORT MachineBasicBlock {
public:
    friend class MachineFunction;
    using Container = std::list<std::unique_ptr<MachineInstruction>>;
    using Iterator = Container::iterator;
    using ConstIterator = Container::const_iterator;

    MachineBasicBlock(const MachineBasicBlock&) = delete;
    MachineBasicBlock& operator=(const MachineBasicBlock&) = delete;
    MachineBasicBlock(MachineBasicBlock&&) noexcept = default;
    MachineBasicBlock& operator=(MachineBasicBlock&&) = default;

    MachineFunction* Parent() {
        return parent_;
    }
    [[nodiscard]] std::size_t PredSize() const {
        return predecessors_.size();
    }
    [[nodiscard]] std::size_t SuccSize() const {
        return successors_.size();
    }

    /**
     * @brief This should be called before using Contain() and Insert() methods.
     * @todo Implement a smarter method.
     */
    void ConstructInverseMap();
    [[nodiscard]] bool Contain(const MachineInstruction* inst) const;
    void
    InsertBefore(const MachineInstruction* inst, std::unique_ptr<MachineInstruction>&& new_inst);
    void
    InsertAfter(const MachineInstruction* inst, std::unique_ptr<MachineInstruction>&& new_inst);
    void Erase(MachineInstruction* inst);

    void EmplaceBack(std::unique_ptr<MachineInstruction>&& inst) {
        instructions_.emplace_back(std::move(inst));
    }

    std::size_t NumInstructions() const {
        return instructions_.size();
    }

    Iterator begin() {
        return instructions_.begin();
    }  // NOLINT
    Iterator end() {
        return instructions_.end();
    }  // NOLINT
    [[nodiscard]] ConstIterator begin() const {
        return instructions_.begin();
    }  // NOLINT
    [[nodiscard]] ConstIterator end() const {
        return instructions_.end();
    }  // NOLINT
    [[nodiscard]] ConstIterator cbegin() const {
        return instructions_.cbegin();
    }  // NOLINT
    [[nodiscard]] ConstIterator cend() const {
        return instructions_.cend();
    }  // NOLINT

private:
    MachineBasicBlock(
            MachineFunction* parent,
            std::list<std::unique_ptr<MachineInstruction>>&& instructions,
            const std::vector<MachineBasicBlock*>& predecessors,
            const std::vector<MachineBasicBlock*>& successors
    )
        : parent_{parent}
        , instructions_{std::move(instructions)}
        , predecessors_{predecessors}
        , successors_{successors} {}

    MachineFunction* parent_;
    std::list<std::unique_ptr<MachineInstruction>> instructions_;
    std::map<const MachineInstruction*, ConstIterator> mp_;
    std::vector<MachineBasicBlock*> predecessors_;  //!< currently not used field
    std::vector<MachineBasicBlock*> successors_;  //!< currently not used field
};
/**
 * @brief This class contains a list of MachineBasicBlock instances that make up the current
 * compiled function.
 */
class QRET_EXPORT MachineFunction {
public:
    using Container = std::list<MachineBasicBlock>;
    using Iterator = Container::iterator;
    using ConstIterator = Container::const_iterator;

    MachineFunction() = default;
    explicit MachineFunction(const TargetMachine* target)
        : target_{target} {}

    MachineFunction(const MachineFunction&) = delete;
    MachineFunction& operator=(const MachineFunction&) = delete;
    MachineFunction(MachineFunction&&) = default;
    MachineFunction& operator=(MachineFunction&&) = default;

    MachineBasicBlock& AddBlock() {
        blocks_.emplace_back(MachineBasicBlock{this, {}, {}, {}});
        return blocks_.back();
    };
    MachineBasicBlock& InsertBlock(ConstIterator itr) {
        return *blocks_.insert(itr, MachineBasicBlock{this, {}, {}, {}});
    }
    void Erase(ConstIterator itr) {
        blocks_.erase(itr);
    }
    void Clear() {
        blocks_.clear();
    }

    void SetTarget(const TargetMachine* target) {
        target_ = target;
    }
    [[nodiscard]] const TargetMachine* GetTarget() const {
        return target_;
    }

    std::size_t NumBBs() const {
        return blocks_.size();
    }

    Iterator begin() {
        return blocks_.begin();
    }  // NOLINT
    Iterator end() {
        return blocks_.end();
    }  // NOLINT
    [[nodiscard]] ConstIterator begin() const {
        return blocks_.begin();
    }  // NOLINT
    [[nodiscard]] ConstIterator end() const {
        return blocks_.end();
    }  // NOLINT
    [[nodiscard]] ConstIterator cbegin() const {
        return blocks_.cbegin();
    }  // NOLINT
    [[nodiscard]] ConstIterator cend() const {
        return blocks_.cend();
    }  // NOLINT

    void SetIR(const ir::Function* ir) {
        ir_ = ir;
    }
    bool HasIR() const {
        return ir_ != nullptr;
    }
    const ir::Function* GetIR() const {
        return ir_;
    }

    void InitializeCompileInfo(std::unique_ptr<CompileInfo>&& compile_info) {
        compile_info_ = std::move(compile_info);
    }
    [[nodiscard]] bool HasCompileInfo() const {
        return static_cast<bool>(compile_info_);
    }
    CompileInfo* GetMutCompileInfo() {
        return compile_info_.get();
    }
    const CompileInfo* GetCompileInfo() const {
        return compile_info_.get();
    }

private:
    const TargetMachine* target_ = nullptr;
    const ir::Function* ir_ = nullptr;
    std::unique_ptr<CompileInfo> compile_info_ = nullptr;
    std::list<MachineBasicBlock> blocks_ = {};
};
}  // namespace qret

#endif  // QRET_TARGET_MACHINE_FUNCTION_H
