"""pyqret Example: Operation Matrix Verification.

This script demonstrates how to:
1. Define a quantum circuit using the pyqret frontend.
2. Run the circuit on the simulator with operation matrix tracking enabled.
3. Compare the resulting unitary matrix against a Qiskit reference.
"""

import numpy as np
import pyqret.frontend.gate.intrinsic as gate
from pyqret.frontend import (
    Argument,
    CircuitBuilder,
    CircuitGenerator,
    Context,
    Module,
    Qubits,
)
from pyqret.runtime import QuantumStateType, Simulator, SimulatorConfig
from qiskit import QuantumCircuit
from qiskit.quantum_info import Operator

# Tolerance for floating point comparisons
EPS = 1e-9


class TestCircuit(CircuitGenerator):
    """A simple test circuit generator to verify gate operations."""

    def __init__(self, builder: CircuitBuilder) -> None:
        super().__init__(builder)

    def name(self) -> str:
        return "TestCircuit"

    def arg(self) -> Argument:
        args = Argument()
        # Allocate a quantum register 'q' with 4 qubits
        args.add_operates("q", 4)
        return args

    def logic(self, arg: Argument) -> None:
        """Define the quantum gate logic."""
        q: Qubits = arg["q"]  # pyright: ignore[reportAssignmentType]

        # Apply various gates
        # Note: Ensure arguments match the C++ definition (e.g., Target, Control)
        gate.x(q[0])
        gate.h(q[1])
        gate.s(q[1])
        gate.h(q[2])
        gate.cy(q[2], q[1])
        gate.t(q[2])
        gate.cx(q[3], q[0])  # Target: q[3], Control: q[0] (assuming C++ signature)
        gate.ccx(q[1], q[2], q[0])  # Target: q[1], Controls: q[2], q[0]
        gate.ccz(q[2], q[3], q[1])

    def get_qiskit_reference(self) -> tuple[QuantumCircuit, np.ndarray]:
        """Generates the reference unitary matrix using Qiskit.

        Returns:
            tuple[QuantumCircuit, np.ndarray]:
                - The Qiskit circuit object (for visualization).
                - The Operation matrix (transposed to match pyqret).
        """
        qc = QuantumCircuit(4)

        # Apply corresponding Qiskit gates
        # Note: Qiskit syntax is usually (Control, Target)
        qc.x(0)
        qc.h(1)
        qc.s(1)
        qc.h(2)
        qc.cy(1, 2)
        qc.t(2)
        qc.cx(0, 3)  # Control: 0, Target: 3
        qc.ccx(2, 0, 1)  # Control: 2, 0, Target: 1
        qc.ccz(3, 1, 2)

        # Get the unitary matrix from Qiskit
        matrix = Operator(qc).data

        # Transpose the matrix to match memory layout.
        # - Qiskit/NumPy uses Row-Major order.
        # - pyqret (Eigen backend) uses Column-Major order.
        return qc, matrix.T


def normalize_phase(matrix: np.ndarray) -> np.ndarray:
    """Normalize the global phase of a matrix to make comparison possible.

    Divides the matrix by the phase of its first non-zero element.

    Returns:
        np.ndarray: normalized matrix.
    """
    flat = matrix.flatten()
    # Find the index of the first element with magnitude > EPS
    idx = np.argmax(np.abs(flat) > EPS)

    val = flat[idx]
    if np.abs(val) < EPS:
        return matrix

    # Calculate phase factor: val / |val|
    phase = val / np.abs(val)
    return matrix / phase


def print_matrix_preview(label: str, matrix: np.ndarray, size: int = 4) -> None:
    """Print the top-left corner of a matrix for inspection."""
    print(f"\n--- {label} (Top-Left {size}x{size}) ---")
    # Clean up output format
    with np.printoptions(precision=3, suppress=True, linewidth=120):
        print(matrix[:size, :size])
    print(f"(Full shape: {matrix.shape})")


def compare_matrices(mat_a: np.ndarray, mat_b: np.ndarray) -> None:
    """Compare two matrices and print the result, ignoring global phase."""
    print("\n--- Comparison Result ---")
    if mat_a.shape != mat_b.shape:
        print(f"❌ SHAPE MISMATCH: {mat_a.shape} vs {mat_b.shape}")
        return

    # Normalize both matrices to remove global phase differences
    m1_norm = normalize_phase(mat_a)
    m2_norm = normalize_phase(mat_b)

    # Check if they are close enough
    is_same = np.allclose(m1_norm, m2_norm, atol=1e-10)

    if is_same:
        print("✅ MATCH (ignoring global phase & floating point noise)")
        print("   The operation matrix from pyqret matches the Qiskit reference.")
    else:
        diff = np.max(np.abs(m1_norm - m2_norm))
        print(f"❌ MISMATCH (Max diff: {diff:.2e})")
        print("   See the raw matrices above to debug.")


def main() -> None:
    """Main execution flow."""
    # Setup numpy print options for better readability
    np.set_printoptions(precision=3, suppress=True)

    # 1. Setup pyqret environment
    context = Context()
    module = Module("example", context)
    builder = CircuitBuilder(module)

    # 2. Generate the circuit
    test_circuit_gen = TestCircuit(builder)
    ir_circuit = test_circuit_gen.generate()

    print("==============================")
    print(" 1. Reference Circuit (Qiskit)")
    print("==============================")
    # Retrieve Qiskit circuit for drawing and matrix for comparison
    qc_ref, qiskit_matrix = test_circuit_gen.get_qiskit_reference()
    print(qc_ref.draw())

    print("\n==============================")
    print(" 2. Running pyqret simulation")
    print("==============================")

    # 3. Configure and run simulator
    # Enable 'save_operation_matrix' to track the unitary evolution
    config = SimulatorConfig(state_type=QuantumStateType.FullQuantum, seed=0, save_operation_matrix=True)

    sim = Simulator(config, ir_circuit)
    sim.run()

    # 4. Retrieve the operation matrix from the simulator state
    state = sim.get_full_quantum_state()
    pyqret_matrix = state.get_operation_matrix()

    # Show matrix preview
    print_matrix_preview("pyqret Matrix", pyqret_matrix)
    print_matrix_preview("Qiskit Matrix (Transposed)", qiskit_matrix)

    print("\n==============================")
    print(" 3. Verification")
    print("==============================")
    # Debug info
    print(f"Determinant (pyqret): {np.linalg.det(pyqret_matrix):.4f}")

    compare_matrices(pyqret_matrix, qiskit_matrix)


if __name__ == "__main__":
    main()
