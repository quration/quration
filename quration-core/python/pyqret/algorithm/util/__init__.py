"""pyqret.algorithm.util module."""

from __future__ import annotations

from typing import TYPE_CHECKING

from ... import BigInt
from ..._qret_impl.algorithm import util as _M  # pyright: ignore[reportMissingImports]

if TYPE_CHECKING:
    from ...frontend import CleanAncilla, CleanAncillae, DirtyAncillae, Qubit, Qubits, Registers


def apply_x_to_each(tgt: Qubits) -> None:
    """Applies Pauli-X to all qubits.

    Args:
        tgt (Qubits): size = n
    """
    _M.apply_x_to_each(tgt._impl)


def apply_x_to_each_if(tgt: Qubits, bools: Registers) -> None:
    """Applies Pauli-X to qubits if the corresponding register is 1.

    Applies Pauli-X to tgt[i] if bools[i] == 1

    Args:
        tgt (Qubits): size = n
        bools (Registers): size = n
    """
    _M.apply_x_to_each_if(tgt._impl, bools._impl)


def apply_cx_to_each_if(c: Qubit, tgt: Qubits, bools: Registers) -> None:
    """Applies CNOT to qubits if the corresponding bit is 1.

    Applies CNOT to (c, tgt[i]) if bools[i] == 1

    Args:
        c (Qubit): size = 1
        tgt (Qubits): size = n
        bools (Registers): size = n
    """
    _M.apply_cx_to_each_if(c._impl, tgt._impl, bools._impl)


def apply_x_to_each_if_imm(imm: int, tgt: Qubits) -> None:
    """Applies Pauli-X to qubits if the corresponding bit is 1.

    Applies Pauli-X to tgt[i] if (imm>>i) & 1 == 1

    Args:
        imm (int): condition
        tgt (Qubits): size = n
    """
    _M.apply_x_to_each_if_imm(BigInt(imm)._impl, tgt._impl)


def apply_cx_to_each_if_imm(imm: int, c: Qubit, tgt: Qubits) -> None:
    """Applies CNOT to qubits if the corresponding bit is 1.

    Applies CNOT to (c, tgt[i]) if (imm>>i) & 1 == 1

    Args:
        imm (int): condition
        c (Qubit): size = 1
        tgt (Qubits): size = n
    """
    _M.apply_cx_to_each_if_imm(BigInt(imm)._impl, c._impl, tgt._impl)


def multi_controlled_bit_order_rotation(l: int, cs: Qubits, tgt: Qubits, aux: DirtyAncillae) -> None:  # noqa:E741
    r"""Performs controlled shift of the lower `l` qubits to the higher positions.

    See fig22 of https://arxiv.org/abs/1706.07884 for more information.

    Diagram of bit order rotation::

        lower `l` part         ------\ /---------
                                      X
        higher `n` - `l` part  ------/ \---------

    Math of bit order rotation: new_tgt = tgt.Range(l, n - l) + tgt(0, l)

    We need dirty ancillae to run multi-controlled swap.

    Args:
        l (int): 0 < l < n
        cs (Qubits): size = c
        tgt (Qubits): size = n
        aux (DirtyAncillae): size = m, if n == 2 and c >= 2, we need aux: m = 1, otherwise m = 0
    """
    _M.multi_controlled_bit_order_rotation(l, cs._impl, tgt._impl, aux._impl)


def multi_controlled_bit_order_reversal(cs: Qubits, tgt: Qubits, aux: DirtyAncillae) -> None:
    """Performs controlled reversal of the order of qubits.

    See fig23 of https://arxiv.org/abs/1706.07884 for more information.

    We need dirty ancillae to run multi-controlled swap.

    Args:
        cs (Qubits): size = c
        tgt (Qubits): size = n
        aux (DirtyAncillae): size = m, if n = 2 and c >= 2, we need aux: m = 1; otherwise m = 0
    """
    _M.multi_controlled_bit_order_reversal(cs._impl, tgt._impl, aux._impl)


def mcmx(cs: Qubits, tgt: Qubits, aux: DirtyAncillae) -> None:
    """Performs multi-controlled multi-not.

    Calculate pauli X to all `t` if all qubits of `c` is `|1>`.

    See fig25 of https://arxiv.org/abs/1706.07884 for more information.

    Switching algorithms depending on the number of target, control, aux qubits:

    1. case1: c = 0                       : just do X to all t
    2. case2: c = 1                       : just do CX to all t, c[0]
    3. case3: c = 2                       : fig24
    4. case4: c >= 3, (n-1)+m >= c-2      : fig25
    5. case5: c >= 3, c-2 > (n-1)+m >= 1  : See https://algassert.com/circuits/2015/06/05/Constructing-Large-Controlled-Nots.html

    Args:
        cs (Qubits): size = c
        tgt (Qubits): size = n
        aux (DirtyAncillae): size = m (if n=1 and c>=3 then m>=1)
    """
    _M.mcmx(cs._impl, tgt._impl, aux._impl)


def reset(q: Qubit) -> None:
    """Reset a qubit to `|0>`.

    Args:
        q (Qubit): qubit to reset
    """
    _M.reset(q._impl)


def r1(q: Qubit, theta: float, precision: float = 1e-10) -> None:
    r"""Performs R1 gate.

    ``precision`` is used when compiling for backend quantum computers that do not have an arbitrary angle rotation instruction such as SC_LS_FIXED_V0.

    .. math::
        \begin{bmatrix}
                1 & 0 \\
                0 & e^{i \theta}
        \end{bmatrix}

    The name "R1" is referencing Q#:
    https://learn.microsoft.com/en-us/qsharp/api/qsharp/microsoft.quantum.intrinsic.r1

    Args:
        q (Qubit): qubit
        theta (float): angle
        precision (float, optional): precision. Defaults to 1e-10.
    """
    _M.r1(q._impl, theta, precision)


def controlled_r1(c: Qubit, q: Qubit, theta: float, precision: float = 1e-10) -> None:
    """Performs controlled R1 gate.

    ``precision`` is used when compiling for backend quantum computers that do not have an arbitrary angle rotation instruction such as SC_LS_FIXED_V0.

    See https://quantumcomputing.stackexchange.com/questions/2143/how-can-a-controlledry-be-made-from-cnots-and-rotations for more information.

    Args:
        c (Qubit): control
        q (Qubit): qubit
        theta (float): angle
        precision (float, optional): precision. Defaults to 1e-10.
    """
    _M.controlled_r1(c._impl, q._impl, theta, precision)


def controlled_rx(c: Qubit, q: Qubit, theta: float, precision: float = 1e-10) -> None:
    """Performs controlled RX gate.

    ``precision`` is used when compiling for backend quantum computers that do not have an arbitrary angle rotation instruction such as SC_LS_FIXED_V0.

    Args:
        c (Qubit): control
        q (Qubit): qubit
        theta (float): angle
        precision (float, optional): precision. Defaults to 1e-10.
    """
    _M.controlled_rx(c._impl, q._impl, theta, precision)


def controlled_ry(c: Qubit, q: Qubit, theta: float, precision: float = 1e-10) -> None:
    """Performs controlled RY gate.

    ``precision`` is used when compiling for backend quantum computers that do not have an arbitrary angle rotation instruction such as SC_LS_FIXED_V0.

    Args:
        c (Qubit): control
        q (Qubit): qubit
        theta (float): angle
        precision (float, optional): precision. Defaults to 1e-10.
    """
    _M.controlled_ry(c._impl, q._impl, theta, precision)


def controlled_rz(c: Qubit, q: Qubit, theta: float, precision: float = 1e-10) -> None:
    """Performs controlled RZ gate.

    ``precision`` is used when compiling for backend quantum computers that do not have an arbitrary angle rotation instruction such as SC_LS_FIXED_V0.

    Args:
        c (Qubit): control
        q (Qubit): qubit
        theta (float): angle
        precision (float, optional): precision. Defaults to 1e-10.
    """
    _M.controlled_rz(c._impl, q._impl, theta, precision)


def randomized_multiplexed_rotation(  # noqa:PLR0913,PLR0917
    lookup_par: int,
    del_lookup_par: int,
    sub_bit_precision: int,
    seed: int,
    theta: list[float],
    q: Qubit,
    address: Qubits,
    aux: CleanAncillae,
) -> None:
    """Performs multiplexed rotation using random.

    Implements eq(1) of https://arxiv.org/abs/2110.13439 using QROAM technique.

    Args:
        lookup_par (int): the number of threads when looking up is 2^lookup_par
        del_lookup_par (int): the number of threads when deleting look up is 2^lookup_par
        sub_bit_precision (int): precision of rotation
        seed (int): seed
        theta (list[float]): rotation angle
        q (Qubit): size = 1
        address (Qubits): size = n
        aux (CleanAncillae): size = 1 + sub_bit_precision + max(n - 1 - lookup_par, n - 1 - del_lookup_par) + max((2^lookup_par - 1)*sub_bit_precision, 2^del_lookup_par)
    """
    _M.randomized_multiplexed_rotation(
        lookup_par,
        del_lookup_par,
        sub_bit_precision,
        seed,
        theta,
        q._impl,
        address._impl,
        aux._impl,
    )


def controlled_h(c: Qubit, q: Qubit, catalyst: Qubit, aux: CleanAncilla) -> None:
    """Performs controlled H gate using catalysis `|T>`.

    See fig17 of https://arxiv.org/abs/2011.03494 for more information

    Args:
        c (Qubit): control
        q (Qubit): qubit
        catalyst (Qubit): `|T>`
        aux (CleanAncilla): size=1
    """
    _M.controlled_h(c._impl, q._impl, catalyst._impl, aux._impl)


def swap(lhs: Qubit, rhs: Qubit) -> None:
    """Performs swap gate using three CNOT.

    Args:
        lhs (Qubit): size=1
        rhs (Qubit): size=1
    """
    _M.swap(lhs._impl, rhs._impl)


def controlled_swap(c: Qubit, lhs: Qubit, rhs: Qubit, aux: CleanAncilla) -> None:
    """Performs controlled swap gate.

    See https://quantumcomputing.stackexchange.com/questions/33774/whats-a-good-cliffordt-circuit-for-a-controlled controlled swap-gate for more information

    Args:
        c (Qubit): control
        lhs (Qubit): size=1
        rhs (Qubit): size=1
        aux (CleanAncilla): size=1
    """
    _M.controlled_swap(c._impl, lhs._impl, rhs._impl, aux._impl)


def write_registers(imm: int, out: Registers) -> None:
    """Write a value to registers.

    Set register values as follows:

    1. out[0] = 0-th bit of imm
    2. out[1] = 1-st bit of imm
    3. ... (repeat)

    out[i] = (imm >> i) & 1

    Args:
        imm (int): value
        out (Registers): size = n
    """
    _M.write_registers(BigInt(imm)._impl, out._impl)
