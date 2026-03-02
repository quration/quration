"""pyqret.algorithm.phase_estimation module."""

from __future__ import annotations

from typing import TYPE_CHECKING

from ..._qret_impl.algorithm import phase_estimation as _M  # pyright: ignore[reportMissingImports]

if TYPE_CHECKING:
    from ...frontend import CleanAncillae, Qubit, Qubits


def prepare(  # noqa:PLR0913,PLR0917
    alias_sampling: list[tuple[int, int]],
    index: Qubits,
    alternate_index: Qubits,
    random: Qubits,
    switch_weight: Qubits,
    cmp: Qubit,
    aux: CleanAncillae,
) -> None:
    r"""Perform PREPARE.

    See fig11 of https://arxiv.org/abs/1805.03662 for more information.
    Correspondence between fig11 and params:

    * n = :math:`\log L`
    * m = :math:`\mu`

    Args:
        alias_sampling (list[): pair of (alternated index, switch weight), keep weight = 2^m - switch weight, size = the number of pauli terms < =2^n
        index (Qubits): size = n
        alternate_index (Qubits): size = n
        random (Qubits): size = m, m is sub_bit_precision
        switch_weight (Qubits): size = m
        cmp (Qubit): size = 1
        aux (CleanAncillae): size = n+3
    """
    _M.prepare(
        alias_sampling,
        index._impl,
        alternate_index._impl,
        random._impl,
        switch_weight._impl,
        cmp._impl,
        aux._impl,
    )
