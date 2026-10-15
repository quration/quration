"""Fixtures and gating for the QIR<->Quration equivalence tests.

Requires a built ``qret`` CLI and a Python env with ``qdk``: missing ``qdk`` ignores the modules at
collection, a missing ``qret`` binary skips with a message. See README.md.
"""

from __future__ import annotations

import os
import subprocess
from pathlib import Path

import pytest

# .../quration-core/tests/qir_equiv/conftest.py -> repo root is 4 levels up.
REPO_ROOT = Path(__file__).resolve().parents[3]


def _qdk_available() -> bool:
    try:
        import qdk.simulation  # noqa: F401

        return True
    except Exception:
        return False


# Prerequisite 1: without qdk there is nothing to compare against, so skip the whole layer.
collect_ignore = [] if _qdk_available() else ["test_export_equiv.py", "test_import_equiv.py"]


def pytest_addoption(parser):
    parser.addoption(
        "--qret-bin",
        action="store",
        default=None,
        help="Path to the built qret CLI binary (default: auto-discover under build*/).",
    )


def _discover_qret() -> Path | None:
    for candidate in (REPO_ROOT / "build" / "main" / "qret", REPO_ROOT / "build" / "bin" / "qret"):
        if candidate.exists():
            return candidate
    hits = sorted(REPO_ROOT.glob("build*/main/qret")) + sorted(REPO_ROOT.glob("build*/bin/qret"))
    return hits[0] if hits else None


def _lib_dir(qret_bin: Path) -> Path | None:
    # libqret-core.so sits under <build>/quration-core/src; the binary's rpath may point at a
    # different absolute path (e.g. a devcontainer mount), so set LD_LIBRARY_PATH explicitly.
    candidate = qret_bin.parent.parent / "quration-core" / "src"
    return candidate if candidate.exists() else None


def _gridsynth() -> Path | None:
    hits = sorted(REPO_ROOT.glob("externals/bin/gridsynth"))
    return hits[0] if hits else None


@pytest.fixture(scope="session")
def qret(pytestconfig):
    """A callable ``qret(*args) -> stdout`` running the built CLI from the repo root."""
    option = pytestconfig.getoption("--qret-bin")
    qret_bin = Path(option).resolve() if option else _discover_qret()
    if qret_bin is None or not qret_bin.exists():
        pytest.skip("qret CLI not found: build an all-* preset or pass --qret-bin=PATH")

    env = os.environ.copy()
    lib = _lib_dir(qret_bin)
    if lib is not None:
        env["LD_LIBRARY_PATH"] = f"{lib}:{env.get('LD_LIBRARY_PATH', '')}"
    gridsynth = _gridsynth()
    if gridsynth is not None:
        env.setdefault("GRIDSYNTH_PATH", str(gridsynth))

    def run(*args: str) -> str:
        proc = subprocess.run(
            [str(qret_bin), *args],
            cwd=str(REPO_ROOT),
            env=env,
            capture_output=True,
            text=True,
        )
        if proc.returncode != 0:
            raise RuntimeError(
                f"qret {' '.join(args)} failed (exit {proc.returncode}):\n{proc.stderr}"
            )
        return proc.stdout

    return run
