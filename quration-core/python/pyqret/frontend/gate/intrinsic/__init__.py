"""pyqret.frontend.gate.intrinsic module."""

from ...._qret_impl.frontend.gate import intrinsic as _M  # pyright: ignore[reportMissingImports]
from ... import CircuitBuilder, Qubit, Qubits, Register, Registers

# TODO: Return Instruction class


def measure(q: Qubit, r: Register) -> None:
    """Insert a measurement instruction to the quantum circuit.

    Measure the qubit ``q`` on the Z basis and save the result to the register ``r``.

    Args:
        q (Qubit): qubit.
        r (Register): register to save the measurement result.
    """
    _M.measure(q._impl, r._impl)


def i(q: Qubit) -> None:
    r"""Insert an identity instruction to the quantum circuit.

    .. math::
        \begin{bmatrix}
                1 & 0 \\
                0 & 1
        \end{bmatrix}

    Args:
        q (Qubit): qubit.
    """
    _M.i(q._impl)


def x(q: Qubit) -> None:
    r"""Insert a pauli-X instruction to the quantum circuit.

    .. math::
        \begin{bmatrix}
                0 & 1 \\
                1 & 0
        \end{bmatrix}

    Args:
        q (Qubit): qubit.
    """
    _M.x(q._impl)


def y(q: Qubit) -> None:
    r"""Insert a pauli-Y instruction to the quantum circuit.

    .. math::
        \begin{bmatrix}
                0 & -i \\
                i & 0
        \end{bmatrix}

    Args:
        q (Qubit): qubit.
    """
    _M.y(q._impl)


def z(q: Qubit) -> None:
    r"""Insert a pauli-Z instruction to the quantum circuit.

    .. math::
        \begin{bmatrix}
                1 & 0 \\
                0 & -1
        \end{bmatrix}

    Args:
        q (Qubit): qubit.
    """
    _M.z(q._impl)


def h(q: Qubit) -> None:
    r"""Insert an hadamard instruction to the quantum circuit.

    .. math::
        \begin{bmatrix}
                \frac{1}{\sqrt{2}} & \frac{1}{\sqrt{2}} \\
                \frac{1}{\sqrt{2}} & -\frac{1}{\sqrt{2}}
        \end{bmatrix}

    Args:
        q (Qubit): qubit.
    """
    _M.h(q._impl)


def s(q: Qubit) -> None:
    r"""Insert a pi/4-phase instruction to the quantum circuit.

    .. math::
        \begin{bmatrix}
                1 & 0 \\
                0 & i
        \end{bmatrix}

    Args:
        q (Qubit): qubit.
    """
    _M.s(q._impl)


def sdag(q: Qubit) -> None:
    r"""Insert an adjoint of pi/4-phase instruction to the quantum circuit.

    .. math::
        \begin{bmatrix}
                1 & 0 \\
                0 & -i
        \end{bmatrix}

    Args:
        q (Qubit): qubit.
    """
    _M.sdag(q._impl)


def t(q: Qubit) -> None:
    r"""Insert a pi/8-phase instruction to the quantum circuit.

    .. math::
        \begin{bmatrix}
                1 & 0 \\
                0 & e^{i\pi/4}
        \end{bmatrix}

    Args:
        q (Qubit): qubit.
    """
    _M.t(q._impl)


def tdag(q: Qubit) -> None:
    r"""Insert an adjoint of pi/8-phase instruction to the quantum circuit.

    .. math::
        \begin{bmatrix}
                1 & 0 \\
                0 & e^{-i\pi/4}
        \end{bmatrix}

    Args:
        q (Qubit): qubit.
    """
    _M.tdag(q._impl)


def rx(q: Qubit, theta: float, precision: float = 1e-10) -> None:
    r"""Insert a pauli-X rotation instruction to the quantum circuit.

    ``precision`` is used when compiling for backend quantum computers that do not have an arbitrary angle rotation instruction such as SC_LS_FIXED_V0.

    .. math::
        \begin{bmatrix}
                \cos(\theta/2)    & -i \sin(\theta/2) \\
                -i \sin(\theta/2) & \cos(\theta/2)
        \end{bmatrix}

    Args:
        q (Qubit): qubit.
        theta (float): angle.
        precision (float, optional): precision. Defaults to 1e-10.
    """
    _M.rx(q._impl, theta, precision)


def ry(q: Qubit, theta: float, precision: float = 1e-10) -> None:
    r"""Insert a pauli-Y rotation instruction to the quantum circuit.

    ``precision`` is used when compiling for backend quantum computers that do not have an arbitrary angle rotation instruction such as SC_LS_FIXED_V0.

    .. math::
        \begin{bmatrix}
                \cos(\theta/2) & - \sin(\theta/2) \\
                \sin(\theta/2) & \cos(\theta/2)
        \end{bmatrix}

    Args:
        q (Qubit): qubit.
        theta (float): angle.
        precision (float, optional): precision. Defaults to 1e-10.
    """
    _M.ry(q._impl, theta, precision)


def rz(q: Qubit, theta: float, precision: float = 1e-10) -> None:
    r"""Insert a pauli-Z rotation instruction to the quantum circuit.

    ``precision`` is used when compiling for backend quantum computers that do not have an arbitrary angle rotation instruction such as SC_LS_FIXED_V0.

    .. math::
        \begin{bmatrix}
                e^{-i \theta/2} & 0 \\
                0               & e^{i \theta/2}
        \end{bmatrix}

    Args:
        q (Qubit): qubit.
        theta (float): angle.
        precision (float, optional): precision. Defaults to 1e-10.
    """
    _M.rz(q._impl, theta, precision)


def cx(t: Qubit, c: Qubit) -> None:
    """Insert a controlled-X instruction to the quantum circuit.

    Args:
        t (Qubit): target qubit.
        c (Qubit): control qubit.
    """
    _M.cx(t._impl, c._impl)


def cy(t: Qubit, c: Qubit) -> None:
    """Insert a controlled-Y instruction to the quantum circuit.

    Args:
        t (Qubit): target qubit.
        c (Qubit): control qubit.
    """
    _M.cy(t._impl, c._impl)


def cz(t: Qubit, c: Qubit) -> None:
    """Insert a controlled-Z instruction to the quantum circuit.

    Args:
        t (Qubit): target qubit.
        c (Qubit): control qubit.
    """
    _M.cz(t._impl, c._impl)


def ccx(t: Qubit, c0: Qubit, c1: Qubit) -> None:
    """Insert a doubly controlled-X, also known as a Toffoli, instruction to the quantum circuit.

    Args:
        t (Qubit): target qubit.
        c0 (Qubit): control qubit.
        c1 (Qubit): control qubit.
    """
    _M.ccx(t._impl, c0._impl, c1._impl)


def ccy(t: Qubit, c0: Qubit, c1: Qubit) -> None:
    """Insert a doubly controlled-Y instruction to the quantum circuit.

    Args:
        t (Qubit): target qubit.
        c0 (Qubit): control qubit.
        c1 (Qubit): control qubit.
    """
    _M.ccy(t._impl, c0._impl, c1._impl)


def ccz(t: Qubit, c0: Qubit, c1: Qubit) -> None:
    """Insert a doubly controlled-Z instruction to the quantum circuit.

    Args:
        t (Qubit): target qubit.
        c0 (Qubit): control qubit.
        c1 (Qubit): control qubit.
    """
    _M.ccz(t._impl, c0._impl, c1._impl)


def mcx(t: Qubit, c: Qubits) -> None:
    """Insert a multi controlled-X instruction to the quantum circuit.

    Args:
        t (Qubit): target qubit.
        c (Qubits): control qubits.
    """
    _M.mcx(t._impl, c._impl)


def global_phase(builder: CircuitBuilder, theta: float, precision: float = 1e-10) -> None:
    r"""Insert a global phase rotation instruction to the quantum circuit.

    ``precision`` is used when compiling for backend quantum computers that do not have an arbitrary angle rotation instruction such as SC_LS_FIXED_V0.

    Note:
        Since the rotation of the global phase cannot be measured, this function is not used for the implementation of quantum circuits.
        This function exists to implement gates that cannot be constructed with combinations of intrinsic gates.
        For example, consider implementing a U(1) gate.
        Both the U(1) gate and the RZ gate are pauli-Z rotations;
        however, the U(1) gate is non-unitary, whereas the RZ gate is unitary.
        Therefore the U(1) gate cannot be implemented using only the RZ gate.
        By using thins function, you can implement the U(1) gate as follows:

        .. math::
            U(1)(\lambda)
            = \begin{bmatrix}
                1 & 0 \\
                0 & e^{i\lambda}
            \end{bmatrix}
            = e^{i\lambda/2} \begin{bmatrix}
                e^{-i\lambda/2} & 0 \\
                0               & e^{i\lambda/2}
            \end{bmatrix}
            = \text{RotateGlobalPhase}(\lambda/2) RZ(\lambda)

    Args:
        builder (CircuitBuilder): the builder currently inserting instructions.
        theta (float): angle.
        precision (float, optional): precision. Defaults to 1e-10.
    """
    _M.global_phase(builder._impl, theta, precision)


def discrete_distribution(weights: list[float], registers: Registers) -> None:
    r"""Insert a DiscreteDistribution instruction into the quantum circuit.

    This instruction samples an outcome :math:`k \in \{0,\dots,N-1\}` with probability
    proportional to the provided non-negative weights and writes :math:`k` into the given
    classical ``registers`` as an unsigned integer in **LSB0** (little-endian bit order).

    Notes:
        * The value written is the **index** :math:`k` (not one-hot). Its binary
          representation is mapped to the target registers with bit 0 at the least
          significant position (LSB0).
        * This instruction may be unsupported by some fault-tolerant backends (e.g. SC_LS_FIXED_V0).
          Compilation to such targets can fail if control-flow depends on the resulting
          registers. Use only when the backend explicitly supports discrete distributions.
        * ``weights`` need not be normalized; they are internally normalized.

    Args:
        weights (list[float]):  Non-negative weights :math:`[w_0,\dots,w_{N-1}]`.
            At least one weight must be strictly positive. ``NaN``/``inf`` are not allowed.
        registers (Registers): Classical registers that will receive the sampled
            index :math:`k` in binary (LSB0). The bit-width must satisfy
            :math:`\lceil \log_2 N \rceil \le \text{width(registers)}`.
    """
    _M.discrete_distribution(weights, registers._impl)
