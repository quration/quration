/**
 * @file qret/frontend/intrinsic.h
 * @brief Intrinsic gates.
 * @details This file defines intrinsic gates of quantum circuits.
 */

#ifndef QRET_FRONTEND_INTRINSIC_H
#define QRET_FRONTEND_INTRINSIC_H

#include "qret/base/portable_function.h"
#include "qret/frontend/argument.h"
#include "qret/ir/instructions.h"
#include "qret/qret_export.h"

namespace qret::frontend {
// Forward declarations
class CircuitBuilder;

namespace gate {
//--------------------------------------------------//
// Measurement
//--------------------------------------------------//

/**
 * @brief Inserts a measurement instruction into the circuit currently being defined in
 * CircuitBuilder.
 *
 * @param q qubit to measure
 * @param r register to save the measurement result
 * @return ir::MeasurementInst* the inserted instruction
 */
QRET_EXPORT ir::MeasurementInst* Measure(const Qubit& q, const Register& r);

//--------------------------------------------------//
// Unary qubit gate
//--------------------------------------------------//

/**
 * @brief Inserts an identity instruction into the circuit currently being defined in
 * CircuitBuilder.
 *
 * @param q qubit to apply gate
 * @return ir::UnaryInst* the inserted instruction
 */
QRET_EXPORT ir::UnaryInst* I(const Qubit& q);
/**
 * @brief Inserts a Pauli-X instruction into the circuit currently being defined in
 * CircuitBuilder.
 *
 * @param q qubit to apply gate
 * @return ir::UnaryInst* the inserted instruction
 */
QRET_EXPORT ir::UnaryInst* X(const Qubit& q);
/**
 * @brief Inserts a Pauli-Y instruction into the circuit currently being defined in
 * CircuitBuilder.
 *
 * @param q qubit to apply gate
 * @return ir::UnaryInst* the inserted instruction
 */
QRET_EXPORT ir::UnaryInst* Y(const Qubit& q);
/**
 * @brief Inserts a Pauli-Z instruction into the circuit currently being defined in
 * CircuitBuilder.
 *
 * @param q qubit to apply gate
 * @return ir::UnaryInst* the inserted instruction
 */
QRET_EXPORT ir::UnaryInst* Z(const Qubit& q);
/**
 * @brief Inserts an hadamard instruction into the circuit currently being defined in
 * CircuitBuilder.
 *
 * @param q qubit to apply gate
 * @return ir::UnaryInst* the inserted instruction
 */
QRET_EXPORT ir::UnaryInst* H(const Qubit& q);
/**
 * @brief Inserts a phase instruction into the circuit currently being defined in
 * CircuitBuilder.
 *
 * @param q qubit to apply gate
 * @return ir::UnaryInst* the inserted instruction
 */
QRET_EXPORT ir::UnaryInst* S(const Qubit& q);
/**
 * @brief Inserts an adjoint of phase instruction into the circuit currently being defined in
 * CircuitBuilder.
 *
 * @param q qubit to apply gate
 * @return ir::UnaryInst* the inserted instruction
 */
QRET_EXPORT ir::UnaryInst* SDag(const Qubit& q);
/**
 * @brief Inserts a T instruction into the circuit currently being defined in
 * CircuitBuilder.
 *
 * @param q qubit to apply gate
 * @return ir::UnaryInst* the inserted instruction
 */
QRET_EXPORT ir::UnaryInst* T(const Qubit& q);
/**
 * @brief Inserts an adjoint of T instruction into the circuit currently being defined in
 * CircuitBuilder.
 *
 * @param q qubit to apply gate
 * @return ir::UnaryInst* the inserted instruction
 */
QRET_EXPORT ir::UnaryInst* TDag(const Qubit& q);
/**
 * @brief Inserts a Pauli-X rotation instruction into the circuit currently being defined in
 * CircuitBuilder.
 * @details
 * \f[
 * R_x(\theta)=
 * \begin{bmatrix}
 * \cos\frac{\theta}{2} & -\,\mathrm{i}\sin\frac{\theta}{2} \\
 * -\,\mathrm{i}\sin\frac{\theta}{2} & \cos\frac{\theta}{2}
 * \end{bmatrix}
 * \f]
 *
 * The gate is the same as the gate defined in:
 * https://learn.microsoft.com/en-us/qsharp/api/qsharp/microsoft.quantum.intrinsic.rx
 *
 * @param q qubit
 * @param theta rotation angle
 * @param precision precision of theta
 * @return ir::ParametrizedRotationInst* the inserted instruction
 */
QRET_EXPORT ir::ParametrizedRotationInst*
RX(const Qubit& q, double theta, double precision = 1e-10);
/**
 * @brief Inserts a Pauli-Y rotation instruction into the circuit currently being defined in
 * CircuitBuilder.
 * @details
 * \f[
 * R_x(\theta)=
 * \begin{bmatrix}
 * \cos\frac{\theta}{2} & -\sin\frac{\theta}{2} \\
 * \sin\frac{\theta}{2} & \cos\frac{\theta}{2}
 * \end{bmatrix}
 * \f]
 *
 * The gate is the same as the gate defined in:
 * https://learn.microsoft.com/en-us/qsharp/api/qsharp/microsoft.quantum.intrinsic.ry
 *
 * @param q qubit
 * @param theta rotation angle
 * @param precision precision of theta
 * @return ir::ParametrizedRotationInst* the inserted instruction
 */
QRET_EXPORT ir::ParametrizedRotationInst*
RY(const Qubit& q, double theta, double precision = 1e-10);
/**
 * @brief Inserts a Pauli-Z rotation instruction into the circuit currently being defined in
 * CircuitBuilder.
 * @details
 * \f[
 * R_z(\theta)=
 * \begin{bmatrix}
 * e^{-\,\mathrm{i}\theta/2} & 0 \\
 * 0 & e^{\,\mathrm{i}\theta/2}
 * \end{bmatrix}.
 * \f]
 *
 * The gate is the same as the gate defined in:
 * https://learn.microsoft.com/en-us/qsharp/api/qsharp/microsoft.quantum.intrinsic.rz
 *
 * @param q qubit
 * @param theta rotation angle
 * @param precision precision of theta
 * @return ir::ParametrizedRotationInst* the inserted instruction
 */
QRET_EXPORT ir::ParametrizedRotationInst*
RZ(const Qubit& q, double theta, double precision = 1e-10);

//--------------------------------------------------//
// Binary qubit gate
//--------------------------------------------------//

/**
 * @brief Inserts a controlled Pauli-X instruction into the circuit currently being defined in
 * CircuitBuilder.
 *
 * @param t qubit to apply gate
 * @param c control qubit
 * @return ir::BinaryInst* the inserted instruction
 */
QRET_EXPORT ir::BinaryInst* CX(const Qubit& t, const Qubit& c);
/**
 * @brief Inserts a controlled Pauli-Y instruction into the circuit currently being defined in
 * CircuitBuilder.
 *
 * @param t qubit to apply gate
 * @param c control qubit
 * @return ir::BinaryInst* the inserted instruction
 */
QRET_EXPORT ir::BinaryInst* CY(const Qubit& t, const Qubit& c);
/**
 * @brief Inserts a controlled Pauli-Z instruction into the circuit currently being defined in
 * CircuitBuilder.
 *
 * @param t qubit to apply gate
 * @param c control qubit
 * @return ir::BinaryInst* the inserted instruction
 */
QRET_EXPORT ir::BinaryInst* CZ(const Qubit& t, const Qubit& c);

//--------------------------------------------------//
// Ternary qubit gate
//--------------------------------------------------//

/**
 * @brief Inserts a doubly controlled Pauli-X instruction into the circuit currently being defined
 * in CircuitBuilder.
 *
 * @param t qubit to apply gate
 * @param c0 control qubit
 * @param c1 control qubit
 * @return ir::TernaryInst* the inserted instruction
 */
QRET_EXPORT ir::TernaryInst* CCX(const Qubit& t, const Qubit& c0, const Qubit& c1);
/**
 * @brief Inserts a doubly controlled Pauli-Y instruction into the circuit currently being defined
 * in CircuitBuilder.
 *
 * @param t qubit to apply gate
 * @param c0 control qubit
 * @param c1 control qubit
 * @return ir::TernaryInst* the inserted instruction
 */
QRET_EXPORT ir::TernaryInst* CCY(const Qubit& t, const Qubit& c0, const Qubit& c1);
/**
 * @brief Inserts a doubly controlled Pauli-Z instruction into the circuit currently being defined
 * in CircuitBuilder.
 *
 * @param t qubit to apply gate
 * @param c0 control qubit
 * @param c1 control qubit
 * @return ir::TernaryInst* the inserted instruction
 */
QRET_EXPORT ir::TernaryInst* CCZ(const Qubit& t, const Qubit& c0, const Qubit& c1);

//--------------------------------------------------//
// Multi control qubit gate
//--------------------------------------------------//

/**
 * @brief Inserts a multi controlled Pauli-X instruction into the circuit currently being defined
 * in CircuitBuilder.
 *
 * @param t qubit to apply gate
 * @param c control qubits
 * @return ir::MultiControlInst* the inserted instruction
 */
QRET_EXPORT ir::MultiControlInst* MCX(const Qubit& t, const Qubits& c);

//--------------------------------------------------//
// Others
//--------------------------------------------------//

/**
 * @brief Inserts a global phase rotation instruction into the circuit currently being defined
 * in CircuitBuilder.
 *
 * @param builder
 * @param theta rotation angle
 * @param precision precision of theta
 * @return ir::GlobalPhaseInst* the inserted instruction
 */
QRET_EXPORT ir::GlobalPhaseInst*
GlobalPhase(CircuitBuilder* builder, double theta, double precision = 1e-10);

//--------------------------------------------------//
// Classical
//--------------------------------------------------//

/**
 * @brief Inserts a instruction to call a classical function into the circuit currently being
 * defined in CircuitBuilder.
 *
 * @param function classical function
 * @param in input registers
 * @param out output registers
 * @return ir::CallCFInst* the inserted instruction
 */
QRET_EXPORT ir::CallCFInst*
CallCF(const PortableAnnotatedFunction& function, const Registers& in, const Registers& out);
/**
 * @brief Inserts a classical random-index generator into the circuit being built by CircuitBuilder.
 *
 * Draws an integer index @c i according to the discrete distribution defined by @p weights and
 * writes the result into @p registers (as the binary representation of @c i).
 *
 * Definition:
 *  - Let S = sum(weights). With probability weights[i] / S, the value i is chosen.
 *
 * @pre All elements of @p weights are non-negative.
 * @pre sum(weights) > 0.
 * @pre The bit width of @p registers is at least ceil(log2(weights.size())) so that any index i can
 * be represented.
 * @note The value written is the **index** :math:`i` (not one-hot). Its binary representation is
 * mapped to the target registers with bit 0 at the least significant position (LSB0).
 *
 * @param weights Non-negative weights defining the (unnormalized) discrete distribution.
 * @param registers Destination classical registers to store the chosen integer index.
 * @return ir::DiscreteDistInst* Pointer to the inserted ir::DiscreteDistInst.
 */
QRET_EXPORT ir::DiscreteDistInst*
DiscreteDistribution(const std::vector<double>& weights, const Registers& registers);
}  // namespace gate
}  // namespace qret::frontend

#endif  // QRET_FRONTEND_INTRINSIC_H
