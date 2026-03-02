"""pyqret.algorithm.arithmetic module."""

from ... import BigInt
from ..._qret_impl.algorithm import arithmetic as _M  # pyright: ignore[reportMissingImports]
from ...frontend import (
    CleanAncilla,
    CleanAncillae,
    DirtyAncilla,
    DirtyAncillae,
    Qubit,
    Qubits,
    Registers,
)

######################################################
# arithmetic/boolean.h
######################################################


def temporal_and(t: Qubit, c0: Qubit, c1: Qubit) -> None:
    """Performs temporal logical AND.

    Calculate t = c0 & c1. t must be clean at first.
    See fig3 of https://arxiv.org/abs/1709.06648 for more information.

    Different implementation from the current one: fig4 of https://arxiv.org/abs/1805.03662

    Args:
        t (Qubit): target
        c0 (Qubit): control
        c1 (Qubit): control
    """
    _M.temporal_and(t._impl, c0._impl, c1._impl)


def uncompute_temporal_and(t: Qubit, c0: Qubit, c1: Qubit) -> None:
    """Performs measurement based uncomputation of temporal logical AND.

    Calculate t = c0 & c1, by measuring a qubit `t`.
    See fig3 of https://arxiv.org/abs/1709.06648 for more information.

    Args:
        t (Qubit): target
        c0 (Qubit): control
        c1 (Qubit): control
    """
    _M.uncompute_temporal_and(t._impl, c0._impl, c1._impl)


def rt3(t: Qubit, c0: Qubit, c1: Qubit) -> None:
    """Performs a toffoli-like operation, while adding a relative phase.

    Calculate t ^= (c0 & c1), while adding a relative phase.
    If you use IRT3 to uncompute RT3, the relative phase disappears.
    RT3 and IRT3 is called "paired toffoli".
    See fig3 of https://arxiv.org/abs/2101.04764 for more information.

    Args:
        t (Qubit): target
        c0 (Qubit): control
        c1 (Qubit): control
    """
    _M.rt3(t._impl, c0._impl, c1._impl)


def irt3(t: Qubit, c0: Qubit, c1: Qubit) -> None:
    """Performs a toffoli-like operation, while adding a relative phase.

    Calculate t ^= (c0 & c1), while adding a relative phase.
    If you use IRT3 to uncompute RT3, the relative phase disappears.
    RT3 and IRT3 is called "paired toffoli".
    See fig3 of https://arxiv.org/abs/2101.04764 for more information.

    Args:
        t (Qubit): target
        c0 (Qubit): control
        c1 (Qubit): control
    """
    _M.irt3(t._impl, c0._impl, c1._impl)


def rt4(t: Qubit, c0: Qubit, c1: Qubit) -> None:
    """Performs a toffoli-like operation, while adding a relative phase.

    Calculate t ^= (c0 & c1), while adding a relative phase.
    If you use IRT4 to uncompute RT4, the relative phase disappears.
    RT3 and IRT3 is called "paired toffoli".
    See fig4 of https://arxiv.org/abs/2101.04764 for more information.

    Args:
        t (Qubit): target
        c0 (Qubit): control
        c1 (Qubit): control
    """
    _M.rt4(t._impl, c0._impl, c1._impl)


def irt4(t: Qubit, c0: Qubit, c1: Qubit) -> None:
    """Performs a toffoli-like operation, while adding a relative phase.

    Calculate t ^= (c0 & c1), while adding a relative phase.
    If you use IRT4 to uncompute RT4, the relative phase disappears.
    RT3 and IRT3 is called "paired toffoli".
    See fig4 of https://arxiv.org/abs/2101.04764 for more information.

    Args:
        t (Qubit): target
        c0 (Qubit): control
        c1 (Qubit): control
    """
    _M.irt4(t._impl, c0._impl, c1._impl)


def maj(x: Qubit, y: Qubit, z: Qubit) -> None:
    """Performs a in-place majorization.

    Calculates the following transformation:

    `|xyz>` -> `|MAJ(x,y,z)>` `|x+y>` `|x+z>`

    See fig 3 of https://arxiv.org/abs/quant-ph/0410184 for more information.

    Args:
        x (Qubit): size=1
        y (Qubit): size=1
        z (Qubit): size=1
    """
    _M.maj(x._impl, y._impl, z._impl)


def uma(x: Qubit, y: Qubit, z: Qubit) -> None:
    r"""Performs a in-place unmajorization.

    Calculates the following transformation:

    `|MAJ(x,y,z)>` `|x+y>` `|x+z>` -> `|xyu>` where `u = x \oplus y \oplus z` .

    See fig 3 of https://arxiv.org/abs/quant-ph/0410184 for more information.

    Args:
        x (Qubit): size=1
        y (Qubit): size=1
        z (Qubit): size=1
    """
    _M.uma(x._impl, y._impl, z._impl)


######################################################
# arithmetic/integer.h
######################################################


def inc(tgt: Qubits, aux: DirtyAncillae) -> None:
    """Performs increment.

    See fig19 and fig20 of https://arxiv.org/abs/1706.07884 for more information.

    Args:
        tgt (Qubits): size = n
        aux (DirtyAncillae): size = m (If n >= 4, then m >= 1)
    """
    _M.inc(tgt._impl, aux._impl)


def multi_controlled_inc(cs: Qubits, tgt: Qubits, aux: DirtyAncillae) -> None:
    """Performs controlled increment.

    See fig20 of https://arxiv.org/abs/1706.07884 for more information.

    Args:
        cs (Qubits): size = c
        tgt (Qubits): size = n
        aux (DirtyAncillae): size = m >= 1
    """
    _M.multi_controlled_inc(cs._impl, tgt._impl, aux._impl)


def add(dst: Qubits, src: Qubits) -> None:
    """Performs addition without using auxiliary qubits.

    Calculate dst += src

    See fig15 of https://arxiv.org/abs/1706.07884 for more information.

    Args:
        dst (Qubits): size = n
        src (Qubits): size = n
    """
    _M.add(dst._impl, src._impl)


def add_cuccaro(dst: Qubits, src: Qubits, aux: CleanAncilla) -> None:
    """Performs addition using Cuccaro's Ripple-Carry Adder (RCA).

    Calculate dst += src

    See fig4 of https://arxiv.org/abs/quant-ph/0410184 for more information.

    Args:
        dst (Qubits): size = n
        src (Qubits): size = n
        aux (CleanAncilla): size = 1
    """
    _M.add_cuccaro(dst._impl, src._impl, aux._impl)


def add_craig(dst: Qubits, src: Qubits, aux: CleanAncillae) -> None:
    """Performs addition using Craig's RCA.

    Calculate dst += src

    See fig1 of https://arxiv.org/abs/1709.06648 for more information.

    Args:
        dst (Qubits): size = n
        src (Qubits): size = n
        aux (CleanAncillae): size = n-1
    """
    _M.add_craig(dst._impl, src._impl, aux._impl)


def controlled_add_craig(c: Qubit, dst: Qubits, src: Qubits, aux: CleanAncillae) -> None:
    """Performs controlled addition using Craig's RCA.

    Calculate dst += src if `c` is `|1>` .
    See fig4 of https://arxiv.org/abs/1709.06648 for more information.

    Args:
        c (Qubit): size = 1
        dst (Qubits): size = n
        src (Qubits): size = n
        aux (CleanAncillae): size = n
    """
    _M.controlled_add_craig(c._impl, dst._impl, src._impl, aux._impl)


def add_imm(imm: int, tgt: Qubits, aux: DirtyAncilla) -> None:
    """Performs immediate value addition.

    Calculate tgt += imm

    See fig16 of https://arxiv.org/abs/1706.07884 for more information.

    Args:
        imm (int): immediate value, smaller than 2^n
        tgt (Qubits): size = n
        aux (DirtyAncilla): size = 1
    """
    _M.add_imm(BigInt(imm)._impl, tgt._impl, aux._impl)


def multi_controlled_add_imm(imm: int, cs: Qubits, tgt: Qubits, aux: DirtyAncilla) -> None:
    """Performs controlled immediate value addition.

    Calculate tgt += imm controlled by cs

    See fig16 and fig18 of https://arxiv.org/abs/1706.07884 for more information.

    Args:
        imm (int): immediate value, smaller than 2^n
        cs (Qubits): size = c
        tgt (Qubits): size = n
        aux (DirtyAncilla): size = 1
    """
    _M.multi_controlled_add_imm(BigInt(imm)._impl, cs._impl, tgt._impl, aux._impl)


def add_general(dst: Qubits, src: Qubits) -> None:
    """Performs general addition without using auxiliary qubits.

    Calculate dst += src

    See fig17 of https://arxiv.org/abs/1706.07884 for more information.

    Args:
        dst (Qubits): size = n (n >= m)
        src (Qubits): size = m (m >= 2)
    """
    _M.add_general(dst._impl, src._impl)


def multi_controlled_add_general(cs: Qubits, dst: Qubits, src: Qubits, aux: DirtyAncilla) -> None:
    """Performs controlled general addition.

    Calculate dst += src controlled by cs

    See fig18 of https://arxiv.org/abs/1706.07884 for more information.

    Args:
        cs (Qubits): size = c
        dst (Qubits): size = n (n >= m)
        src (Qubits): size = m (m >= 2)
        aux (DirtyAncilla): size = 1
    """
    _M.multi_controlled_add_general(cs._impl, dst._impl, src._impl, aux._impl)


def add_piecewise(p: int, dst: Qubits, src: Qubits, aux: CleanAncillae, out: Registers) -> None:
    """Performs approximate piecewise addition.

    Calculate dst += src.

    This circuit can reduce the asymptotic depth of addition to O(lg lg n)
    (n: size of `dst` and `src`) by dividing qubits into lower and upper bits during addition.
    The carry from lower to upper bits uses additional auxiliary qubits
    which size is `m` and initialized to the `|+>` state.

    Note that if carry occurs, the addition will fail with probability 1/2^m.
    If all aux measurement results are `|0>`, the addition may have failed.

    See fig2 and fig3 of https://arxiv.org/abs/1905.08488 for more information.

    Args:
        p (int): size of lower bits, 0 < p < n
        dst (Qubits): size = n
        src (Qubits): size = n
        aux (CleanAncillae): size = m + 1 + max(n - p - m, m), 0 < m <= n-p
        out (Registers): size = m
    """
    _M.add_piecewise(p, dst._impl, src._impl, aux._impl, out._impl)


def sub(dst: Qubits, src: Qubits) -> None:
    """Performs subtraction without using auxiliary qubits (adjoint of :func:`add` ).

    Calculate dst -= src

    Args:
        dst (Qubits): size = n
        src (Qubits): size = n
    """
    _M.sub(dst._impl, src._impl)


def sub_cuccaro(dst: Qubits, src: Qubits, aux: CleanAncilla) -> None:
    """Performs subtraction using adjoint of Cuccaro's RCA.

    Calculate dst -= src

    Args:
        dst (Qubits): size = n
        src (Qubits): size = n
        aux (CleanAncilla): size = 1
    """
    _M.sub_cuccaro(dst._impl, src._impl, aux._impl)


def sub_craig(dst: Qubits, src: Qubits, aux: CleanAncillae) -> None:
    """Performs subtraction using adjoint of Craig's RCA.

    Calculate dst -= src

    Args:
        dst (Qubits): size = n
        src (Qubits): size = n
        aux (CleanAncillae): size = n-1
    """
    _M.sub_craig(dst._impl, src._impl, aux._impl)


def controlled_sub_craig(c: Qubit, dst: Qubits, src: Qubits, aux: CleanAncillae) -> None:
    """Performs controlled subtraction using adjoint of Craig's RCA.

    Calculate dst -= src if `c` is `|1>`.

    Args:
        c (Qubit): size = 1
        dst (Qubits): size = n
        src (Qubits): size = n
        aux (CleanAncillae): size = n
    """
    _M.controlled_sub_craig(c._impl, dst._impl, src._impl, aux._impl)


def sub_imm(imm: int, tgt: Qubits, aux: DirtyAncilla) -> None:
    """Performs immediate value subtraction (adjoint of :func:`add_imm`).

    Calculate tgt -= imm

    Args:
        imm (int): immediate value, smaller than 2^n
        tgt (Qubits): size = n
        aux (DirtyAncilla): size = 1
    """
    _M.sub_imm(BigInt(imm)._impl, tgt._impl, aux._impl)


def multi_controlled_sub_imm(imm: int, cs: Qubits, tgt: Qubits, aux: DirtyAncilla) -> None:
    """Performs controlled immediate value subtraction (adjoint of :func:`multi_controlled_add_imm`).

    Calculate tgt -= imm controlled by cs

    Args:
        imm (int): immediate value, smaller than 2^n
        cs (Qubits): size = c
        tgt (Qubits): size = n
        aux (DirtyAncilla): size = 1
    """
    _M.multi_controlled_sub_imm(BigInt(imm)._impl, cs._impl, tgt._impl, aux._impl)


def sub_general(dst: Qubits, src: Qubits) -> None:
    """Performs general subtraction without using auxiliary qubits (adjoint of :func:`add_general` ).

    Calculate dst -= src

    Args:
        dst (Qubits): size = n (n >= m)
        src (Qubits): size = m (m >= 2)
    """
    _M.sub_general(dst._impl, src._impl)


def multi_controlled_sub_general(cs: Qubits, dst: Qubits, src: Qubits, aux: DirtyAncilla) -> None:
    """Performs controlled general subtraction (adjoint of :func:`multi_controlled_sub_general`).

    Calculate dst -= src controlled by cs

    Args:
        cs (Qubits): size = c
        dst (Qubits): size = n (n >= m)
        src (Qubits): size = m (m >= 2)
        aux (DirtyAncilla): size = 1
    """
    _M.multi_controlled_sub_general(cs._impl, dst._impl, src._impl, aux._impl)


def mul_w(window_size: int, mul: int, tgt: Qubits, table: CleanAncillae, aux: CleanAncillae) -> None:
    """Performs multiplication optimized with windowing.

    Calculates `tgt *= mul`

    See `times_equal` of https://arxiv.org/abs/1905.07682 for more information.

    Args:
        window_size (int): window_size < n
        mul (int): odd number
        tgt (Qubits): size = n
        table (CleanAncillae): size = n
        aux (CleanAncillae): size = n - 1
    """
    _M.mul_w(window_size, BigInt(mul)._impl, tgt._impl, table._impl, aux._impl)


def scaled_add(scale: int, dst: Qubits, src: Qubits, aux: CleanAncillae) -> None:
    """Performs scaled addition.

    Calculates dst += scale * src

    See fig6 of https://arxiv.org/abs/1706.07884 for more information.

    Args:
        scale (int): positive integer
        dst (Qubits): size = n
        src (Qubits): size = n
        aux (CleanAncillae): size = n - 1
    """
    _M.scaled_add(BigInt(scale)._impl, dst._impl, src._impl, aux._impl)


def scaled_add_w(  # noqa:PLR0913,PLR0917
    window_size: int,
    scale: int,
    dst: Qubits,
    src: Qubits,
    table: CleanAncillae,
    aux: CleanAncillae,
) -> None:
    """Performs scaled addition optimized with windowing.

    Calculates dst += scale * src

    See `plus_equal_product` of https://arxiv.org/abs/1905.07682 for more information.

    Args:
        window_size (int): window_size < n
        scale (int): positive integer
        dst (Qubits): size = n
        src (Qubits): size = n
        table (CleanAncillae): size = n
        aux (CleanAncillae): size = n - 1
    """
    _M.scaled_add_w(window_size, BigInt(scale)._impl, dst._impl, src._impl, table._impl, aux._impl)


def equal_to(lhs: Qubits, rhs: Qubits, cmp: Qubit, aux: CleanAncillae) -> None:
    """Performs comparison `equal to` (==).

    Args:
        lhs (Qubits): size = n
        rhs (Qubits): size = n
        cmp (Qubit): size = 1
        aux (CleanAncillae): size = n-1
    """
    _M.equal_to(lhs._impl, rhs._impl, cmp._impl, aux._impl)


def equal_to_imm(imm: int, tgt: Qubits, cmp: Qubit, aux: CleanAncillae) -> None:
    """Performs comparison `equal to` (==) with immediate value.

    Args:
        imm (int): immediate value, smaller than 2^n
        tgt (Qubits): size = n
        cmp (Qubit): size = 1
        aux (CleanAncillae): size = n-1
    """
    _M.equal_to_imm(BigInt(imm)._impl, tgt._impl, cmp._impl, aux._impl)


def less_than(lhs: Qubits, rhs: Qubits, cmp: Qubit, aux: CleanAncillae) -> None:
    """Performs comparison `less than` (<).

    If lhs < rhs, flips `cmp`.

    Algorithm:

    1. Computes lhs -= rhs by adding an extra most significant bit
        * Let `cmp` be the most significant bit of `lhs`
    2. Computes lhs += rhs to uncompute the previous subtraction

    Args:
        lhs (Qubits): size = n
        rhs (Qubits): size = n
        cmp (Qubit): size = 1
        aux (CleanAncillae): size = 2
    """
    _M.less_than(lhs._impl, rhs._impl, cmp._impl, aux._impl)


def less_than_or_equal_to(lhs: Qubits, rhs: Qubits, cmp: Qubit, aux: CleanAncillae) -> None:
    """Performs comparison `less than or equal to` (<=).

    If lhs <= rhs, flips `cmp`.

    Algorithm:

    1. Computes lhs -= (rhs+1) by adding an extra most significant bit
        * Let `cmp` be the most significant bit of `lhs`
    2. Computes lhs += rhs and lhs += 1 to uncompute the previous subtraction
        * addition and increment

    Args:
        lhs (Qubits): size = n
        rhs (Qubits): size = n
        cmp (Qubit): size = 1
        aux (CleanAncillae): size = 2
    """
    _M.less_than_or_equal_to(lhs._impl, rhs._impl, cmp._impl, aux._impl)


def less_than_imm(imm: int, tgt: Qubits, cmp: Qubit, aux: DirtyAncilla) -> None:
    """Performs comparison `less than` (<) with immediate value.

    If tgt < imm, flips `cmp`

    See fig14 of https://arxiv.org/abs/1706.07884 for more information.

    Args:
        imm (int): immediate value
        tgt (Qubits): size = n
        cmp (Qubit): size = 1
        aux (DirtyAncilla): size = 1
    """
    _M.less_than_imm(BigInt(imm)._impl, tgt._impl, cmp._impl, aux._impl)


def multi_controlled_less_than_imm(imm: int, cs: Qubits, tgt: Qubits, cmp: Qubit, aux: DirtyAncilla) -> None:
    """Performs comparison `less than` (<) with immediate value controlled by qubits.

    If tgt < imm, flips `cmp` controlled by qubits `cs`.

    See fig14 of https://arxiv.org/abs/1706.07884 for more information.

    Args:
        imm (int): immediate value
        cs (Qubits): size = c
        tgt (Qubits): size = n
        cmp (Qubit): size = 1
        aux (DirtyAncilla): size = 1
    """
    _M.multi_controlled_less_than_imm(BigInt(imm)._impl, cs._impl, tgt._impl, cmp._impl, aux._impl)


def bi_flip(dst: Qubits, src: Qubits) -> None:
    """Flips both [0, src-1] and [src, max] sections at the same time.

    See section2.5 (fig8) of https://arxiv.org/abs/1706.07884 for more information.

    * max = 2^n - 1

    Args:
        dst (Qubits): size = n >= m
        src (Qubits): size = m
    """
    _M.bi_flip(dst._impl, src._impl)


def multi_controlled_bi_flip(cs: Qubits, dst: Qubits, src: Qubits, aux: DirtyAncilla) -> None:
    """Performs controlled flip of both [0, src-1] and [src, max] sections at the same time.

    See section2.5 (fig8) of https://arxiv.org/abs/1706.07884 for more information.

    * max = 2^n - 1

    Args:
        cs (Qubits): size = c
        dst (Qubits): size = n >= m
        src (Qubits): size = m
        aux (DirtyAncilla): size = 1
    """
    _M.multi_controlled_bi_flip(cs._impl, dst._impl, src._impl, aux._impl)


def bi_flip_imm(imm: int, tgt: Qubits, aux: DirtyAncilla) -> None:
    """Flips both [0, imm-1] and [imm, max] sections at the same time.

    See section2.5 (fig9) of https://arxiv.org/abs/1706.07884 for more information.

    * max = 2^n - 1

    Args:
        imm (int): immediate value
        tgt (Qubits): size = n
        aux (DirtyAncilla): size = 1
    """
    _M.bi_flip_imm(BigInt(imm)._impl, tgt._impl, aux._impl)


def multi_controlled_bi_flip_imm(imm: int, cs: Qubits, tgt: Qubits, aux: DirtyAncilla) -> None:
    """Performs controlled flip of both [0, imm-1] and [imm, max] sections at the same time.

    See section2.5 (fig9) of https://arxiv.org/abs/1706.07884 for more information.

    * max = 2^n - 1

    Args:
        imm (int): immediate value
        cs (Qubits): size = c
        tgt (Qubits): size = n
        aux (DirtyAncilla): size = 1
    """
    _M.multi_controlled_bi_flip_imm(BigInt(imm)._impl, cs._impl, tgt._impl, aux._impl)


def pivot_flip(dst: Qubits, src: Qubits, aux: DirtyAncillae) -> None:
    """Flips [0, src-1] .

    See fig8 of https://arxiv.org/abs/1706.07884 for more information.

    Uses "toggle-controlled" technique to implement pivot flip.

    Args:
        dst (Qubits): size = n >= m
        src (Qubits): size = m
        aux (DirtyAncillae): size = 2
    """
    _M.pivot_flip(dst._impl, src._impl, aux._impl)


def multi_controlled_pivot_flip(cs: Qubits, dst: Qubits, src: Qubits, aux: DirtyAncillae) -> None:
    """Performs a controlled flip of [0, src-1].

    See fig8 of https://arxiv.org/abs/1706.07884 for more information.

    Uses "toggle-controlled" technique to implement pivot flip.

    Args:
        cs (Qubits): size = c
        dst (Qubits): size = n >= m
        src (Qubits): size = m
        aux (DirtyAncillae): size = 2
    """
    _M.multi_controlled_pivot_flip(cs._impl, dst._impl, src._impl, aux._impl)


def pivot_flip_imm(imm: int, tgt: Qubits, aux: DirtyAncillae) -> None:
    """Flips [0, imm-1].

    See fig9 of https://arxiv.org/abs/1706.07884 for more information.

    Args:
        imm (int): immediate value
        tgt (Qubits): size = n
        aux (DirtyAncillae): size = 2
    """
    _M.pivot_flip_imm(BigInt(imm)._impl, tgt._impl, aux._impl)


def multi_controlled_pivot_flip_imm(imm: int, cs: Qubits, tgt: Qubits, aux: DirtyAncillae) -> None:
    """Performs a controlled flip of [0 .. imm-1].

    See fig9 of https://arxiv.org/abs/1706.07884 for more information.

    Args:
        imm (int): immediate value
        cs (Qubits): size = c
        tgt (Qubits): size = n
        aux (DirtyAncillae): size = 2
    """
    _M.multi_controlled_pivot_flip_imm(BigInt(imm)._impl, cs._impl, tgt._impl, aux._impl)


######################################################
# arithmetic/modular.h
######################################################


def mod_neg(mod: int, tgt: Qubits, aux: DirtyAncillae) -> None:
    """Performs modular negation.

    Calculates tgt = (mod - tgt) % mod

    See fig13 of https://arxiv.org/abs/1706.07884 for more information.

    Algorithm:

    1. Decrement `tgt`
    2. Pivot flip tgt under mod - 1
    3. Increment `tgt`

    Note:
        `tgt` must be smaller than mod.

    Args:
        mod (int): positive integer
        tgt (Qubits): size = n
        aux (DirtyAncillae): size = 2
    """
    _M.mod_neg(BigInt(mod)._impl, tgt._impl, aux._impl)


def multi_controlled_mod_neg(mod: int, cs: Qubits, tgt: Qubits, aux: DirtyAncillae) -> None:
    """Performs controlled modular negation.

    Calculates tgt = (mod - tgt) % mod controlled by `cs`

    See fig13 of https://arxiv.org/abs/1706.07884 for more information.

    Note:
        `tgt` must be smaller than mod.

    Args:
        mod (int): positive integer
        cs (Qubits): size = c
        tgt (Qubits): size = n
        aux (DirtyAncillae): size = 2
    """
    _M.multi_controlled_mod_neg(BigInt(mod)._impl, cs._impl, tgt._impl, aux._impl)


def mod_add(mod: int, dst: Qubits, src: Qubits, aux: DirtyAncillae) -> None:
    """Performs modular addition.

    Calculate dst = (dst + src) % mod controlled by `cs`

    See fig11 of https://arxiv.org/abs/1706.07884 for more information.

    Args:
        mod (int): positive integer
        dst (Qubits): size = n >= 2
        src (Qubits): size = n
        aux (DirtyAncillae): size = 2
    """
    _M.mod_add(BigInt(mod)._impl, dst._impl, src._impl, aux._impl)


def multi_controlled_mod_add(mod: int, cs: Qubits, dst: Qubits, src: Qubits, aux: DirtyAncillae) -> None:
    """Performs controlled modular addition.

    Calculate dst = (dst + src) % mod controlled by `cs`

    See fig11 of https://arxiv.org/abs/1706.07884 for more information.

    Args:
        mod (int): positive integer
        cs (Qubits): size = c
        dst (Qubits): size = n >= 2
        src (Qubits): size = n
        aux (DirtyAncillae): size = 2 - c (if c >= 2, size of aux is zero)
    """
    _M.multi_controlled_mod_add(BigInt(mod)._impl, cs._impl, dst._impl, src._impl, aux._impl)


def mod_add_imm(mod: int, imm: int, tgt: Qubits, aux: DirtyAncillae) -> None:
    """Performs modular immediate addition.

    Calculate tgt = (tgt + imm) % mod

    See fig9 of https://arxiv.org/abs/1706.07884 for more information.

    Args:
        mod (int): positive integer
        imm (int): positive integer
        tgt (Qubits): size = n >= 2
        aux (DirtyAncillae): size = 2
    """
    _M.mod_add_imm(BigInt(mod)._impl, BigInt(imm)._impl, tgt._impl, aux._impl)


def multi_controlled_mod_add_imm(mod: int, imm: int, cs: Qubits, tgt: Qubits, aux: DirtyAncillae) -> None:
    """Performs controlled modular immediate addition.

    Calculate tgt = (tgt + imm) % mod controlled by `cs`

    See fig9 of https://arxiv.org/abs/1706.07884 for more information.

    Args:
        mod (int): positive integer
        imm (int): positive integer
        cs (Qubits): size = c
        tgt (Qubits): size = n >= 2
        aux (DirtyAncillae): size = 2
    """
    _M.multi_controlled_mod_add_imm(BigInt(mod)._impl, BigInt(imm)._impl, cs._impl, tgt._impl, aux._impl)


def mod_sub(mod: int, dst: Qubits, src: Qubits, aux: DirtyAncillae) -> None:
    """Performs modular subtraction (adjoint of :func:`mod_add` ).

    Calculate dst = (dst - src) % mod

    Args:
        mod (int): positive integer
        dst (Qubits): size = n >= 2
        src (Qubits): size = n
        aux (DirtyAncillae): size = 2
    """
    _M.mod_sub(BigInt(mod)._impl, dst._impl, src._impl, aux._impl)


def multi_controlled_mod_sub(mod: int, cs: Qubits, dst: Qubits, src: Qubits, aux: DirtyAncillae) -> None:
    """Performs controlled modular subtraction (adjoint of :func:`multi_controlled_mod_add` ).

    Calculate dst = (dst - src) % mod controlled by `cs`

    Args:
        mod (int): positive integer
        cs (Qubits): size = c
        dst (Qubits): size = n >= 2
        src (Qubits): size = n
        aux (DirtyAncillae): size = 2 - c (if c >= 2, size of aux is zero)
    """
    _M.multi_controlled_mod_sub(BigInt(mod)._impl, cs._impl, dst._impl, src._impl, aux._impl)


def mod_sub_imm(mod: int, imm: int, tgt: Qubits, aux: DirtyAncillae) -> None:
    """Performs modular immediate subtraction (adjoint of :func:`mod_add_imm` ).

    Calculate tgt = (tgt - imm) % mod

    Args:
        mod (int): positive integer
        imm (int): positive integer
        tgt (Qubits): size = n >= 2
        aux (DirtyAncillae): size = 2
    """
    _M.mod_sub_imm(BigInt(mod)._impl, BigInt(imm)._impl, tgt._impl, aux._impl)


def multi_controlled_mod_sub_imm(mod: int, imm: int, cs: Qubits, tgt: Qubits, aux: DirtyAncillae) -> None:
    """Performs controlled modular immediate subtraction (adjoint of :func:`multi_controlled_mod_add_imm` ).

    Calculate tgt = (tgt - imm) % mod controlled by `cs`

    Args:
        mod (int): positive integer
        imm (int): positive integer
        cs (Qubits): size = c
        tgt (Qubits): size = n >= 2
        aux (DirtyAncillae): size = 2
    """
    _M.multi_controlled_mod_sub_imm(BigInt(mod)._impl, BigInt(imm)._impl, cs._impl, tgt._impl, aux._impl)


def mod_doubling(mod: int, tgt: Qubits, aux: DirtyAncilla) -> None:
    """Performs modular doubling.

    Calculates tgt = (2 * tgt) % mod

    See fig7 of https://arxiv.org/abs/1706.07884 for more information.

    Args:
        mod (int): odd number
        tgt (Qubits): size = n >= 3
        aux (DirtyAncilla): size = 1
    """
    _M.mod_doubling(BigInt(mod)._impl, tgt._impl, aux._impl)


def multi_controlled_mod_doubling(mod: int, cs: Qubits, tgt: Qubits, aux: DirtyAncilla) -> None:
    """Performs controlled modular doubling.

    Calculates tgt = (2 * tgt) % mod controlled by `cs`

    See fig7 of https://arxiv.org/abs/1706.07884 for more information.

    Args:
        mod (int): odd number
        cs (Qubits): size = c
        tgt (Qubits): size = n >= 3
        aux (DirtyAncilla): size = 1
    """
    _M.multi_controlled_mod_doubling(BigInt(mod)._impl, cs._impl, tgt._impl, aux._impl)


def mod_bi_mul_imm(mod: int, imm: int, lhs: Qubits, rhs: Qubits) -> None:
    """Performs immediate bi multiplication.

    Calculates lhs = (lhs * imm) % mod and rhs = (rhs * (multiplicative inverse of `imm` modulo `mod`)) % mod at the same time.

    See fig5 of https://arxiv.org/abs/1706.07884 for more information.

    Args:
        mod (int): odd number
        imm (int): `imm` must have multiplicative inverse modulo `mod`
        lhs (Qubits): size = n
        rhs (Qubits): size = n
    """
    _M.mod_bi_mul_imm(BigInt(mod)._impl, BigInt(imm)._impl, lhs._impl, rhs._impl)


def multi_controlled_mod_bi_mul_imm(mod: int, imm: int, cs: Qubits, lhs: Qubits, rhs: Qubits) -> None:
    """Performs controlled immediate bi multiplication.

    Calculates lhs = (lhs * imm) % mod and rhs = (rhs * (multiplicative inverse of `imm` modulo `mod`)) % mod at the same time controlled by `cs` .

    See fig5 of https://arxiv.org/abs/1706.07884 for more information.

    Args:
        mod (int): odd number
        imm (int): `imm` must have multiplicative inverse modulo `mod`
        cs (Qubits): size = c
        lhs (Qubits): size = n
        rhs (Qubits): size = n
    """
    _M.multi_controlled_mod_bi_mul_imm(BigInt(mod)._impl, BigInt(imm)._impl, cs._impl, lhs._impl, rhs._impl)


def mod_scaled_add(mod: int, scale: int, dst: Qubits, src: Qubits) -> None:
    """Performs modular scaled addition.

    Calculates dst = (dst + scale * src) % mod

    See fig6 of https://arxiv.org/abs/1706.07884 for more information.

    Args:
        mod (int): odd number
        scale (int): scale < mod, coprime to mod
        dst (Qubits): size = n >= 3
        src (Qubits): size = n
    """
    _M.mod_scaled_add(BigInt(mod)._impl, BigInt(scale)._impl, dst._impl, src._impl)


def mod_scaled_add_w(  # noqa:PLR0913,PLR0917
    window_size: int,
    mod: int,
    scale: int,
    dst: Qubits,
    src: Qubits,
    table: CleanAncillae,
    aux: CleanAncillae,
) -> None:
    """Performs modular scaled addition optimized with windowing.

    Calculates dst = (dst + scale * src) % mod

    See `plus_equal_product_mod` of https://arxiv.org/abs/1905.07682 for more information.

    Args:
        window_size (int): window_size < n
        mod (int): odd number
        scale (int): scale < mod, coprime to mod
        dst (Qubits): size=n
        src (Qubits): size=n
        table (CleanAncillae): size=n
        aux (CleanAncillae): size=max(n-1,2)
    """
    _M.mod_scaled_add_w(
        window_size,
        BigInt(mod)._impl,
        BigInt(scale)._impl,
        dst._impl,
        src._impl,
        table._impl,
        aux._impl,
    )


def multi_controlled_mod_scaled_add(mod: int, scale: int, cs: Qubits, dst: Qubits, src: Qubits) -> None:
    """Performs controlled modular scaled addition.

    Calculates dst = (dst + scale * src) % mod controlled by `cs`

    See fig6 of https://arxiv.org/abs/1706.07884 for more information.

    Args:
        mod (int): odd number
        scale (int): scale < mod, coprime to mod
        cs (Qubits): size = c
        dst (Qubits): size = n >= 3
        src (Qubits): size = n
    """
    _M.multi_controlled_mod_scaled_add(BigInt(mod)._impl, BigInt(scale)._impl, cs._impl, dst._impl, src._impl)


def mod_exp_w(  # noqa:PLR0913,PLR0917
    mul_window_size: int,
    exp_window_size: int,
    mod: int,
    base: int,
    tgt: Qubits,
    exp: Qubits,
    table: CleanAncillae,
    tmp: CleanAncillae,
    aux: CleanAncillae,
) -> None:
    """Performs modular exponentiation optimized with windowing.

    Calculates tgt = tgt * base^exp % mod

    See `times_equal_exp_mod` of https://arxiv.org/abs/1905.07682 for more information.

    Args:
        mul_window_size (int): mul_window_size < n
        exp_window_size (int): exp_window_size < m
        mod (int): odd number
        base (int): base must have multiplicative inverse modulo `mod`
        tgt (Qubits): size = n
        exp (Qubits): size = m
        table (CleanAncillae): size = n
        tmp (CleanAncillae): size = n
        aux (CleanAncillae): size = mul_window_size + exp_window_size - 1 (>=2)
    """
    _M.mod_exp_w(
        mul_window_size,
        exp_window_size,
        BigInt(mod)._impl,
        BigInt(base)._impl,
        tgt._impl,
        exp._impl,
        table._impl,
        tmp._impl,
        aux._impl,
    )
