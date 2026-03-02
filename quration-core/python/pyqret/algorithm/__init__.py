"""pyqret.algorithm module."""

from __future__ import annotations

import importlib

try:
    _algorithm_impl = importlib.import_module("pyqret._qret_impl.algorithm")
except Exception as exc:  # pragma: no cover - import-time guard
    msg = (
        "pyqret.algorithm is not available. "
        "Reinstall with algorithm support (e.g., "
        '`SKBUILD_CMAKE_DEFINE="QRET_BUILD_PYTHON=ON;QRET_PYTHON_WITH_ALGORITHM=ON" '
        "pip install .[algorithm]`)."
    )
    raise ImportError(msg) from exc

_ = _algorithm_impl

arithmetic = importlib.import_module(".arithmetic", __name__)
data = importlib.import_module(".data", __name__)
phase_estimation = importlib.import_module(".phase_estimation", __name__)
preparation = importlib.import_module(".preparation", __name__)
transform = importlib.import_module(".transform", __name__)
util = importlib.import_module(".util", __name__)

QPE = phase_estimation.prepare
QFT = transform.fourier_transform

__all__ = [
    "QFT",
    "QPE",
    "arithmetic",
    "data",
    "phase_estimation",
    "preparation",
    "transform",
    "util",
]
