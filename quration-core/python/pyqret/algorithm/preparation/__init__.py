"""pyqret.algorithm.preparation module."""

from __future__ import annotations

from typing import TYPE_CHECKING

from ..._qret_impl.algorithm import preparation as _M  # pyright: ignore[reportMissingImports]

if TYPE_CHECKING:
    from ...frontend import CleanAncillae, Qubit, Qubits


def prepare_arbitrary_state(probs: list[float], tgt: Qubits, theta: Qubits, aux: CleanAncillae) -> None:
    """Prepares arbitrary state.

    See https://arxiv.org/abs/1812.00954 for more information.

    Args:
        probs (list[float]): size of span must be smaller than 2^n, sum is 1.0
        tgt (Qubits): size = n
        theta (Qubits): size = m
        aux (CleanAncillae): size = n-1
    """
    _M.prepare_arbitrary_state(probs, tgt._impl, theta._impl, aux._impl)


def prepare_uniform_superposition(N: int, tgt: Qubits, aux: CleanAncillae) -> None:  # noqa:N803
    """Creates a uniform superposition over states that encode 0 through `N` - 1.

    Use amplitude amplification technique.
    See fig12 of https://arxiv.org/abs/1805.03662 for more information.

    Args:
        N (int): upper bound, N = 2^k * L (L is odd number)
        tgt (Qubits): size = n >= BitSizeL(N-1)
        aux (CleanAncillae): size = m >= 3 + BitSizeL(L)
    """
    _M.prepare_uniform_superposition(N, tgt._impl, aux._impl)


def controlled_prepare_uniform_superposition(N: int, c: Qubit, tgt: Qubits, aux: CleanAncillae) -> None:  # noqa:N803
    """Performs controlled creation of a uniform superposition over states that encode 0 through `N` - 1.

    Use amplitude amplification technique.
    See fig12 of https://arxiv.org/abs/1805.03662 for more information.

    Args:
        N (int): upper bound, N = 2^k * L (L is odd number)
        c (Qubit): size = 1
        tgt (Qubits): size = n >= BitSizeL(N-1)
        aux (CleanAncillae): size = m >= 3 + BitSizeL(L)
    """
    _M.controlled_prepare_uniform_superposition(N, c._impl, tgt._impl, aux._impl)
