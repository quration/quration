"""pyqret.algorithm.data module."""

from __future__ import annotations

from typing import TYPE_CHECKING

from ... import BigInt
from ..._qret_impl.algorithm import data as _M  # pyright: ignore[reportMissingImports]

if TYPE_CHECKING:
    from ...frontend import CleanAncilla, CleanAncillae, DirtyAncillae, Qubits, Registers


def initialize_q_dict(capacity: int, address: Qubits, value: Qubits) -> None:
    """Initializes quantum dictionaries (q-dict) without QRAM.

    Implements https://arxiv.org/abs/2204.13835.
    This q-dict holds fixed-length list of sorted address-value pairs.
    All addresses are initialized to the maximum address and the value is initialized to 0.
    Note that if you want to use :func:`inject_into_q_dict` or :func:`extract_from_q_dict`, run this function in
    advance.

    Args:
        capacity (int): capacity of q-dict
        address (Qubits): size = n * capacity
        value (Qubits): size = m * capacity
    """
    _M.initialize_q_dict(capacity, address._impl, value._impl)


def inject_into_q_dict(  # noqa:PLR0913,PLR0917
    capacity: int,
    address: Qubits,
    value: Qubits,
    key: Qubits,
    input_value: Qubits,
    aux: CleanAncillae,
) -> None:
    """Injects addressed value into quantum dictionaries (q-dict) without QRAM.

    Implements https://arxiv.org/abs/2204.13835.
    Injects `input` value to address corresponding to `key`.
    Note that if you want to inject into an address that already has a value,
    extract that address first and then inject into it.

    Args:
        capacity (int): capacity of q-dict
        address (Qubits): size = n * capacity
        value (Qubits): size = m * capacity
        key (Qubits): size = n
        input_value (Qubits): size = m
        aux (CleanAncillae): size = n + max(n, m, 3)
    """
    _M.inject_into_q_dict(capacity, address._impl, value._impl, key._impl, input_value._impl, aux._impl)


def extract_from_q_dict(  # noqa:PLR0913,PLR0917
    capacity: int,
    address: Qubits,
    value: Qubits,
    key: Qubits,
    out: Qubits,
    aux: CleanAncillae,
) -> None:
    """Extracts addressed value from quantum dictionaries (q-dict) without QRAM.

    Implements https://arxiv.org/abs/2204.13835.
    If `key` exists in `address`, save its value in `out` and remove it from q-dict.
    Does nothing if `key` does not exist.

    Args:
        capacity (int): capacity of q-dict
        address (Qubits): size = n * capacity
        value (Qubits): size = m * capacity
        key (Qubits): size = n
        out (Qubits): size = m
        aux (CleanAncillae): size = n + max(n, m, 3)
    """
    _M.extract_from_q_dict(capacity, address._impl, value._impl, key._impl, out._impl, aux._impl)


def unary_iter_begin(begin: int, address: Qubits, aux: Qubits) -> None:
    """Begins unary iteration.

    If the value of `address` is equal to `begin`, set `aux[0] = 1`.
    See fig5 and fig7 of https://arxiv.org/abs/1805.03662 for more information.

    Unary iteration from `|L>` to `|L + M>`

    1. :obj:`unary_iter_begin` (L, address, aux);
    2. :obj:`unary_iter_step` (L, address, aux);
    3. :obj:`unary_iter_step` (L + 1, address, aux);
    4. ... (repeat) ...
    5. :obj:`unary_iter_step` (L + M - 1, address, aux);
    6. :obj:`unary_iter_end` (L + M, address, aux);

    Args:
        begin (int): begin < 2^n
        address (Qubits): size = n >= 2
        aux (Qubits): size = n-1, must be clean at first
    """
    _M.unary_iter_begin(BigInt(begin)._impl, address._impl, aux._impl)


def unary_iter_step(step: int, address: Qubits, aux: Qubits) -> None:
    """Steps unary iteration.

    If the value of `address` is equal to `step+1`, set `aux[0] = 1`.
    Steps unary iteration from `step` to `step + 1`

    :obj:`unary_iter_step` (step, address, aux) can be called after :obj:`unary_iter_begin` (step, address, aux) or :obj:`unary_iter_step` (step-1, address, aux).

    Args:
        step (int): step < 2^n - 1
        address (Qubits): size = n >= 2
        aux (Qubits): size = n-1
    """
    _M.unary_iter_step(BigInt(step)._impl, address._impl, aux._impl)


def unary_iter_end(end: int, address: Qubits, aux: Qubits) -> None:
    """Ends unary iteration.

    Ends unary iteration at `end`.
    :obj:`unary_iter_end` (step, address, aux) can be called after :obj:`unary_iter_begin` (end, address, aux) or :obj:`unary_iter_step` (end-1, address, aux).

    Args:
        end (int): step < 2^n - 1
        address (Qubits): size = n >= 2
        aux (Qubits): size = n-1
    """
    _M.unary_iter_end(BigInt(end)._impl, address._impl, aux._impl)


def multi_controlled_unary_iter_begin(begin: int, cs: Qubits, address: Qubits, aux: Qubits) -> None:
    """Performs controlled :func:`unary_iter_begin`.

    Args:
        begin (int): begin < 2^n
        cs (Qubits): size = c
        address (Qubits): size = n,
        aux (Qubits): size = n+c-1, must be clean at first
    """
    _M.multi_controlled_unary_iter_begin(BigInt(begin)._impl, cs._impl, address._impl, aux._impl)


def multi_controlled_unary_iter_step(step: int, cs: Qubits, address: Qubits, aux: Qubits) -> None:
    """Performs controlled :func:`unary_iter_step`.

    Args:
        step (int): step < 2^n - 1
        cs (Qubits): size = c
        address (Qubits): size = n >= 2
        aux (Qubits): size = n-1
    """
    _M.multi_controlled_unary_iter_step(BigInt(step)._impl, cs._impl, address._impl, aux._impl)


def multi_controlled_unary_iter_end(end: int, cs: Qubits, address: Qubits, aux: Qubits) -> None:
    """Performs controlled :func:`unary_iter_end`.

    Args:
        end (int): size = c
        cs (Qubits): size = c
        address (Qubits): size = n >= 2
        aux (Qubits): size = n-1, must be clean at end
    """
    _M.multi_controlled_unary_iter_end(BigInt(end)._impl, cs._impl, address._impl, aux._impl)


def qrom(address: Qubits, value: Qubits, aux: CleanAncillae, d: Registers) -> None:
    """Performs Quantum Read-Only Memory (QROM).

    Implements fig7 of https://arxiv.org/abs/1805.03662

    Args:
        address (Qubits): size = n >= 1
        value (Qubits): size = m
        aux (CleanAncillae): size = n - 1
        d (Registers): size = m * dict_size, dict_size must be smaller than 2^n
    """
    _M.qrom(address._impl, value._impl, aux._impl, d._impl)


def multi_controlled_qrom(cs: Qubits, address: Qubits, value: Qubits, aux: CleanAncillae, d: Registers) -> None:
    """Performs controlled QROM.

    Implements fig7 of https://arxiv.org/abs/1805.03662

    Args:
        cs (Qubits): size = c
        address (Qubits): size = n >= 1
        value (Qubits): size = m
        aux (CleanAncillae): size = n + c - 1
        d (Registers): size = m * dict_size, dict_size must be smaller than 2^n
    """
    _M.multi_controlled_qrom(cs._impl, address._impl, value._impl, aux._impl, d._impl)


def qrom_imm(address: Qubits, value: Qubits, aux: Qubits, d: list[int]) -> None:
    """Performs QROM where the written value is immediate.

    Implements fig7 of https://arxiv.org/abs/1805.03662

    Args:
        address (Qubits): size = n >= 1
        value (Qubits): size = m
        aux (Qubits): size = n - 1
        d (list[int]): size of span must be smaller than 2^n, bit width of each element is m
    """
    d = [BigInt(x)._impl for x in d]
    _M.qrom_imm(address._impl, value._impl, aux._impl, d)


def multi_controlled_qrom_imm(cs: Qubits, address: Qubits, value: Qubits, aux: CleanAncillae, d: list[int]) -> None:
    """Performs controlled QROM where the written value is immediate.

    Implements fig7 of https://arxiv.org/abs/1805.03662.

    Args:
        cs (Qubits): size = c
        address (Qubits): size = n >= 1
        value (Qubits): size = m
        aux (CleanAncillae): size = n + c - 1
        d (list[int]): size of span must be smaller than 2^n, bit width of each element is m
    """
    d = [BigInt(x)._impl for x in d]
    _M.multi_controlled_qrom_imm(cs._impl, address._impl, value._impl, aux._impl, d)


def uncompute_qrom(address: Qubits, value: Qubits, aux: CleanAncillae, d: Registers) -> None:
    """Performs measurement-based uncomputation of QROM.

    See fig5 of https://arxiv.org/abs/1902.02134 for more information.

    Args:
        address (Qubits): size = n >= 1
        value (Qubits): size = m
        aux (CleanAncillae): size = n - 1
        d (Registers): size = m * dict_size, dict_size must be smaller than 2^n
    """
    _M.uncompute_qrom(address._impl, value._impl, aux._impl, d._impl)


def uncompute_qrom_imm(address: Qubits, value: Qubits, aux: CleanAncillae, d: list[int]) -> None:
    """Performs measurement-based uncomputation of QROM where the written value is immediate.

    See fig5 of https://arxiv.org/abs/1902.02134 for more information.

    Args:
        address (Qubits): size = n >= 1
        value (Qubits): size = m
        aux (CleanAncillae): size = n - 1
        d (list[int]): size of span must be smaller than 2^n, bit width of each element is m
    """
    d = [BigInt(x)._impl for x in d]
    _M.uncompute_qrom_imm(address._impl, value._impl, aux._impl, d)


def qroam_clean(  # noqa:PLR0913,PLR0917
    num_par: int,
    address: Qubits,
    value: Qubits,
    duplicate: Qubits,
    aux: CleanAncillae,
    d: Registers,
) -> None:
    """Performs advanced QROM (QROAM) assisted by clean ancillae.

    See fig4 of https://arxiv.org/abs/1902.02134 for more information.

    Args:
        num_par (int): num_par < n-1, the number of threads = 2^num_par
        address (Qubits): size = n
        value (Qubits): size = m
        duplicate (Qubits): size = (2^num_par - 1) * m, must be clean at first
        aux (CleanAncillae): size = n - num_par - 1
        d (Registers): size = m * dict_size, dict_size must be smaller than 2^n
    """
    _M.qroam_clean(num_par, address._impl, value._impl, duplicate._impl, aux._impl, d._impl)


def uncompute_qroam_clean(  # noqa:PLR0913,PLR0917
    num_par: int,
    address: Qubits,
    value: Qubits,
    duplicate: CleanAncillae,
    aux: CleanAncillae,
    d: Registers,
) -> None:
    """Performs measurement-based uncomputation of QROAM assisted by clean ancillae.

    See fig6 of https://arxiv.org/abs/1902.02134 for more information.

    Args:
        num_par (int): num_par < n-1, the number of threads = 2^num_par
        address (Qubits): size = n
        value (Qubits): size = m
        duplicate (CleanAncillae): size = 2^num_par, must be clean at first
        aux (CleanAncillae): size = n - num_par - 1
        d (Registers): size = m * dict_size, dict_size must be smaller than 2^n
    """
    _M.uncompute_qroam_clean(num_par, address._impl, value._impl, duplicate._impl, aux._impl, d._impl)


def qroam_dirty(  # noqa:PLR0913,PLR0917
    num_par: int,
    address: Qubits,
    value: Qubits,
    duplicate: DirtyAncillae,
    aux: CleanAncillae,
    d: Registers,
) -> None:
    """Performs QROAM assisted by dirty ancillae.

    See fig4 of https://arxiv.org/abs/1902.02134 for more information.

    Args:
        num_par (int): num_par < n-1, the number of threads = 2^num_par
        address (Qubits): size = n
        value (Qubits): size = m
        duplicate (DirtyAncillae): size = (2^num_par - 1) * m
        aux (CleanAncillae): size = n - num_par - 1
        d (Registers): size = m * dict_size, dict_size must be smaller than 2^n
    """
    _M.qroam_dirty(num_par, address._impl, value._impl, duplicate._impl, aux._impl, d._impl)


def uncompute_qroam_dirty(  # noqa:PLR0913,PLR0917
    num_par: int,
    address: Qubits,
    value: Qubits,
    duplicate: DirtyAncillae,
    src: CleanAncilla,
    aux: CleanAncillae,
    d: Registers,
) -> None:
    """Performs measurement-based uncomputation of QROAM assisted by dirty ancillae.

    See fig7 of https://arxiv.org/abs/1902.02134 for more information.

    Args:
        num_par (int): num_par < n-1, the number of threads = 2^num_par
        address (Qubits): size = n
        value (Qubits): size = m
        duplicate (DirtyAncillae): size = 2^num_par - 1
        src (CleanAncilla): size = 1
        aux (CleanAncillae): size = n - num_par - 1
        d (Registers): size = m * dict_size, dict_size must be smaller than 2^n
    """
    _M.uncompute_qroam_dirty(
        num_par,
        address._impl,
        value._impl,
        duplicate._impl,
        src._impl,
        aux._impl,
        d._impl,
    )


def qrom_parallel(  # noqa:PLR0913,PLR0917
    num_par: int,
    address: Qubits,
    value: Qubits,
    duplicate_address: CleanAncillae,
    duplicate_aux: CleanAncillae,
    d: Registers,
) -> None:
    """Parallel implementation of QROM.

    Implements http://id.nii.ac.jp/1001/00218669/

    The difference between QROAM and QROMParallel:

    * QROAM duplicates values
    * QROMParallel duplicates addresses

    Args:
        num_par (int): num_par < n-1, the number of threads = 2^num_par
        address (Qubits): size = n >= 1
        value (Qubits): size = m
        duplicate_address (CleanAncillae): size = (2^num_par-1) * n
        duplicate_aux (CleanAncillae): size = 2^num_par * (n-1)
        d (Registers): size = m * dict_size, dict_size must be smaller than 2^n
    """
    _M.qrom_parallel(
        num_par,
        address._impl,
        value._impl,
        duplicate_address._impl,
        duplicate_aux._impl,
        d._impl,
    )
