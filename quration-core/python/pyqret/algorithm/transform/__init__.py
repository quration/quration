"""pyqret.algorithm.transform module."""

from __future__ import annotations

from typing import TYPE_CHECKING

from ..._qret_impl.algorithm import transform as _M  # pyright: ignore[reportMissingImports]

if TYPE_CHECKING:
    from ...frontend import Qubits


def fourier_transform(tgt: Qubits) -> None:
    """Perform quantum fourier transformation (QFT).

    Args:
        tgt (Qubits): target qubits.
    """
    _M.fourier_transform(tgt._impl)
