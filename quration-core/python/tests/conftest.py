"""Pytest configuration for pyqret tests."""

from __future__ import annotations

import importlib

_ALGORITHM_TEST_FILES = frozenset(
    {
        "backend/test_compiler.py",
        "frontend/test_circuit_builder.py",
        "algorithm/test_algorithm_phase_estimation.py",
        "algorithm/test_algorithm_transform.py",
        "algorithm/test_algorithm_arithmetic_boolean.py",
        "algorithm/test_algorithm_arithmetic_integer.py",
        "algorithm/test_algorithm_arithmetic_modular.py",
        "algorithm/test_algorithm_data.py",
        "algorithm/test_algorithm_preparation.py",
        "algorithm/test_algorithm_util.py",
        "runtime/test_simulator.py",
    },
)


def _has_algorithm_support() -> bool:
    required_modules = (
        "pyqret.algorithm.arithmetic",
        "pyqret.algorithm.data",
        "pyqret.algorithm.phase_estimation",
        "pyqret.algorithm.preparation",
        "pyqret.algorithm.transform",
        "pyqret.algorithm.util",
    )
    for module in required_modules:
        try:
            importlib.import_module(module)
        except ImportError:
            return False
    return True


_HAS_ALGORITHM_SUPPORT = _has_algorithm_support()
collect_ignore = [] if _HAS_ALGORITHM_SUPPORT else sorted(_ALGORITHM_TEST_FILES)
