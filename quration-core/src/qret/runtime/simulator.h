/**
 * @file qret/runtime/simulator.h
 * @brief Simulator.
 */

#ifndef QRET_RUNTIME_SIMULATOR_H
#define QRET_RUNTIME_SIMULATOR_H

#include <memory>
#include <ostream>
#include <string_view>
#include <vector>

#include "qret/base/type.h"
#include "qret/ir/function.h"
#include "qret/ir/instruction.h"
#include "qret/ir/instructions.h"
#include "qret/ir/metadata.h"
#include "qret/qret_export.h"
#include "qret/runtime/clifford_state.h"
#include "qret/runtime/full_quantum_state.h"
#include "qret/runtime/quantum_state.h"
#include "qret/runtime/toffoli_state.h"

namespace qret::runtime {
/**
 * @brief Configuration of simulator.
 */
struct QRET_EXPORT SimulatorConfig {
    enum class StateType : std::uint8_t {
        Toffoli,
        FullQuantum,
        Clifford,
        MPS,  // Unimplemented
    };

    bool IsToffoli() const {
        return state_type == StateType::Toffoli;
    }
    bool IsFullQuantum() const {
        return state_type == StateType::FullQuantum;
    }
    bool IsClifford() const {
        return state_type == StateType::Clifford;
    }
    bool IsMPS() const {
        return state_type == StateType::MPS;
    }

    StateType state_type;  //!< type of quantum state
    std::uint64_t seed = 3141592;

    // Settings for ToffoliState

    std::uint64_t max_superpositions = 1;  //!< maximum value of the superposition count

    // Settings for FullQuantumState

    bool use_qulacs = false;  //!< if true, simulate using Qulacs
    bool save_operation_matrix = false;  //!< if true, save operation matrix of quantum circuit.
};
/**
 * @brief Simulator of ir::Function.
 */
class QRET_EXPORT Simulator {
public:
    using iterator = ir::BasicBlock::const_iterator;

    explicit Simulator(const SimulatorConfig& config, const ir::Function* func);

    const SimulatorConfig& GetConfig() const {
        return config_;
    }

    // Get state
    const QuantumState& GetState() const {
        return *state_;
    }
    const ToffoliState& GetToffoliState() const;
    ToffoliState& GetToffoliState() {
        return const_cast<ToffoliState&>(static_cast<const Simulator*>(this)->GetToffoliState());
    }
    const FullQuantumState& GetFullQuantumState() const;
    FullQuantumState& GetFullQuantumState() {
        return const_cast<FullQuantumState&>(
                static_cast<const Simulator*>(this)->GetFullQuantumState()
        );
    }
    const CliffordState& GetCliffordState() const;
    CliffordState& GetCliffordState() {
        return const_cast<CliffordState&>(static_cast<const Simulator*>(this)->GetCliffordState());
    }

    // Get call state
    bool Finished() const;
    std::size_t GetCallDepth() const {
        return call_stack_.size();
    }
    iterator PeekInst() const {
        return lc_.pc;
    }
    bool PointsToCallInst() const {
        return PeekInst()->IsCall();
    }
    bool PointsToBreakPoint() const {
        return PeekInst()->HasMetadata(ir::MDKind::BreakPoint);
    }

    /**
     * @brief Initialize the \p q -th qubit to |1>
     */
    void SetAs1(std::uint64_t q);
    /**
     * @brief Initialize the qubits from \p start to \p start + \p size with \p value (little
     * endian).
     */
    void SetValue(std::uint64_t start, std::uint64_t size, const BigInt& value);
    /**
     * @brief Initialize the qubits of \p name with \p value (little endian).
     */
    void SetValueByName(std::string_view name, const BigInt& value);

    // Movement
    iterator Step();
    iterator StepInto();
    iterator StepOut();
    iterator JumpToNextBreakPoint();
    iterator JumpTo(std::string_view break_point_name);
    void RunAll();

    // Printer
    void PrintCallStack(std::ostream& out) const;

    // Util
    const ir::Function* GetFunction() const {
        return func_;
    }
    const ir::Function* GetCircuit() const {
        return GetFunction();
    }

private:
    /**
     * @brief Moves pc to the next instruction.
     * @details This method does not run a instruction.
     */
    void ToNextInstruction();
    /**
     * @brief Pushes current local context into `call_stack_` and reset local context for a new
     * callee.
     *
     * @param inst callee
     */
    void StepIntoImpl(const ir::CallInst* inst);
    /**
     * @brief Pops out the local context from `call_stack_`.
     */
    void StepOutImpl();
    /**
     * @brief Get the unique register id.
     *
     * @return std::size_t unique register id
     */
    std::size_t GetUniqueRegisterId() {
        return unique_register_id_++;
    }

    struct LocalContext {
        iterator pc;  //!< program counter
        std::vector<std::uint64_t> qmap;  //!< from local to global index of qubit
        std::vector<std::uint64_t> rmap;  //!< from local to global index of register
    };

    const SimulatorConfig config_;  //!< config
    const ir::Function* const func_;  //!< function
    std::unique_ptr<QuantumState> state_;  //!< quantum state
    std::vector<LocalContext> call_stack_;  //!< call stack
    LocalContext lc_;  //!< local context
    std::size_t unique_register_id_;  //!< the next unique register id
};
}  // namespace qret::runtime

#endif  // QRET_RUNTIME_SIMULATOR_H
