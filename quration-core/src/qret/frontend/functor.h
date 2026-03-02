/**
 * @file qret/frontend/functor.h
 * @brief Functor: adjoint and control.
 * @details This file implements a functor, which is an operation that maps one quantum circuit to
 * another circuit.
 */

#ifndef QRET_FRONTEND_FUNCTOR_H
#define QRET_FRONTEND_FUNCTOR_H

#include <type_traits>

#include "qret/frontend/argument.h"
#include "qret/ir/instructions.h"
#include "qret/qret_export.h"

namespace qret::frontend {
namespace impl {
template <typename... Args>
inline constexpr bool HasQTargetV = std::disjunction_v<tag::IsQTarget<std::remove_cv_t<Args>>...>;
inline CircuitBuilder* GetBuilder() {
    return nullptr;
}
template <typename Head, typename... Args>
CircuitBuilder* GetBuilder(const Head& head, const Args&... args) {
    CircuitBuilder* builder = nullptr;
    if constexpr (tag::IsQTargetV<Head>) {
        builder = head.GetBuilder();
    } else {
        builder = GetBuilder(args...);
    }
    return builder;
}
QRET_EXPORT void ReplaceWithAdjoint(CircuitBuilder* builder, ir::CallInst* inst);
template <typename Op, typename... Args>
ir::CallInst* AdjointImpl(Op op, const Args&... args) {
    auto* builder = impl::GetBuilder(args...);
    auto* inst = op(args...);
    ReplaceWithAdjoint(builder, inst);
    return inst;
}
QRET_EXPORT void ReplaceWithControlled(const Qubit& c, ir::CallInst* inst);
template <typename Op, typename... Args>
ir::CallInst* ControlledImpl(const Qubit& c, Op op, const Args&... args) {
    auto* inst = op(args...);
    ReplaceWithControlled(c, inst);
    return inst;
}
}  // namespace impl

//--------------------------------------------------//
// Functor: adjoint
//--------------------------------------------------//

/**
 * @brief Adjointed operation.
 *
 * @tparam Op Operation type
 */
template <typename Op>
class AdjointOp {
public:
    explicit AdjointOp(Op op)
        : op_{op} {}

    /**
     * @brief Insert an adjointed operation.
     * @details Arguments must meet three conditions:
     * 1. `Op` can be invoked with `Args...`
     * 2. Return type of invoking `Op` with `Args...` is `ir::CallInst`
     * 3. The argument contains either Qubit, Qubits, Register, or Registers
     *
     * @param args arguments of an operation
     * @return ir::CallInst* the inserted instruction
     */
    template <
            typename... Args,
            // `Op` can be invoked with `Args...`
            std::enable_if_t<std::is_invocable_v<Op, Args...>, bool> = false,
            // Return type of invoking `Op` with `Args...` is `ir::CallInst`
            std::enable_if_t<std::is_invocable_r_v<ir::CallInst*, Op, Args...>, bool> = false,
            // The argument contains either Qubit, Qubits, Register, or Registers
            std::enable_if_t<impl::HasQTargetV<Args...>, bool> = false>
    ir::CallInst* operator()(const Args&... args) {
        return impl::AdjointImpl(op_, args...);
    }

private:
    Op op_;
};
/**
 * @brief Creates an adjoint operation.
 * @details Definition of an operation:
 * An operation is defined as something that satisfies the following three conditions:
 * 1. Callable as a function
 * 2. Returns an ir::CallInst* when called as a function
 * 3. Upon function call, inserts only one ir::CallInst into the circuit currently being defined
 *
 * This feature is implemented with reference to
 * https://learn.microsoft.com/en-us/azure/quantum/user-guide/language/expressions/functorapplication
 *
 * How to use:
 * Suppose `SomeGoodCircuit` is callable as `SomeGoodCircuit(p, q, r)` and it returns an
 * ir::CallInst* , then the function `Adjoint` can be used as follows:
 * * Adjoint(SomeGoodCircuit)(p, q, r);
 * * Adjoint(Adjoint(SomeGoodCircuit))(p, q, r);
 *
 * Note: Is isn't possible to create the adjoint of a circuit containing the following instructions:
 * * ir::MeasurementInst
 * * ir::CallCFInst
 * * ir::DiscreteDistInst
 * * ir::BranchInst
 * * ir::SwitchInst
 * * ir::CallInst
 *
 * How it works:
 * 1. Creates an circuit to be adjointed by calling `op(args...)`
 *     * By calling `op(args...)`, you can get ir::CallInst* and its callee ir::Function*
 * 2. Creates a new adjointed circuit in IR
 *     * Reverses the instructions in the circuit and replaces each instruction with its adjoint
 * counterpart
 *     * Examples:
 *         * X    --> X
 *         * S    --> SDag
 *         * TDag --> T
 * 3. Replace the callee of ir::CallInst* with the new adjointed circuit
 *
 * @tparam Op Operation type
 * @param op Input operation
 * @return impl::AdjointOp<Op> Adjoint operation
 */
template <typename Op>
AdjointOp<Op> Adjoint(Op op) {
    return AdjointOp<Op>(op);
}

//--------------------------------------------------//
// Functor: controlled
//--------------------------------------------------//

template <typename Op>
class ControlledOp {
public:
    explicit ControlledOp(Op op)
        : op_{op} {}

    /**
     * @brief Insert a controlled operation.
     * @details Arguments must meet three conditions:
     * 1. `Op` can be invoked with `Args...`
     * 2. Return type of invoking `Op` with `Args...` is `ir::CallInst`
     * 3. The argument contains either Qubit, Qubits, Register, or Registers
     *
     * @param c control qubit
     * @param args arguments of an operation
     * @return ir::CallInst* the inserted instruction
     */
    template <
            typename... Args,
            // `Op` can be invoked with `Args...`
            std::enable_if_t<std::is_invocable_v<Op, Args...>, bool> = false,
            // Return type of invoking `Op` with `Args...` is `ir::CallInst`
            std::enable_if_t<std::is_invocable_r_v<ir::CallInst*, Op, Args...>, bool> = false,
            // The argument contains either Qubit, Qubits, Register, or Registers
            std::enable_if_t<impl::HasQTargetV<Args...>, bool> = false>
    ir::CallInst* operator()(const Qubit& c, const Args&... args) {
        return impl::ControlledImpl(c, op_, args...);
    }

private:
    Op op_;
};
/**
 * @brief Creates a controlled operation.
 * @details For the definition of `operation`, refer to the documentation of `Adjoint`.
 *
 * This feature is implemented with reference to
 * https://learn.microsoft.com/en-us/azure/quantum/user-guide/language/expressions/functorapplication
 *
 * How to use:
 * Suppose `SomeGoodCircuit` is callable as `SomeGoodCircuit(p, q, r)` and it returns an
 * ir::CallInst* , then the function `Controlled` can be used as follows:
 * * Controlled(SomeGoodCircuit)(control, p, q, r);
 * * Controlled(Controlled(SomeGoodCircuit))(c0, c1, p, q, r);
 *
 * Note: Only circuits composed of the following instructions can be controlled
 * * X, Y, Z, CX, CY, CZ, CCZ, MCX
 *
 * How it works:
 * 1. Creates an circuit to be controlled by calling `op(args...)`
 *     * By calling `op(args...)`, you can get ir::CallInst* and its callee ir::Function*
 * 2. Creates a new controlled circuit in IR
 * 3. Replace the callee of ir::CallInst* with the new controlled circuit
 *
 * @tparam Op Operation type
 * @param op Input operation
 * @return impl::ControlledOp<Op> Controlled operation
 */
template <typename Op>
ControlledOp<Op> Controlled(Op op) {
    return ControlledOp<Op>(op);
}
}  // namespace qret::frontend

#endif  // QRET_FRONTEND_FUNCTOR_H
