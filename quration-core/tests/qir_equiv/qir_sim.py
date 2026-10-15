"""Helpers for the equivalence tests.

Deterministic circuits are compared per-register at the bit level: Quration exports a multi-bit
register as individual per-bit result records (it can't tell a bool array from an integer, and it
may exceed 64 bits), so both sides are reduced to per-register bit lists before comparing.
Superposition circuits are compared as outcome distributions over many shots (see below).
"""

from __future__ import annotations

import re
from collections import Counter

_BIT_TRUE = ("One", "True")
_BIT_FALSE = ("Zero", "False")


def run_qir(qir_text: str, *, shots: int = 1, seed: int = 1):
    import qdk.simulation as simulation

    return simulation.run_qir(qir_text, shots=shots, seed=seed, type="cpu")


def _to_bit(value) -> int:
    text = str(value)
    if text in _BIT_TRUE:
        return 1
    if text in _BIT_FALSE:
        return 0
    return int(value) & 1


def _shot_items(result) -> list:
    if not result:
        raise AssertionError("run_qir returned no shots")
    shot = result[0]
    if isinstance(shot, str):
        # Circuits without record_output yield a raw measurement bit-string, one char per result.
        return list(shot)
    return list(shot) if isinstance(shot, (tuple, list)) else [shot]


def _item_to_bits(item, size: int) -> list[int]:
    if isinstance(item, (tuple, list)):
        return [_to_bit(bit) for bit in item]
    if isinstance(item, bool) or str(item) in _BIT_TRUE + _BIT_FALSE:
        return [_to_bit(item)]
    value = int(item)  # integer record output -> size LSB-first bits
    return [(value >> i) & 1 for i in range(size)]


def qdk_register_bits(result, sizes: list[int]) -> list[list[int]]:
    """Per-register bit lists from a single deterministic shot, one entry per register size."""
    items = _shot_items(result)
    if len(items) != len(sizes):
        raise AssertionError(
            f"QDK returned {len(items)} outputs, expected {len(sizes)} registers"
        )
    return [_item_to_bits(item, size) for item, size in zip(items, sizes)]


def quration_register_bits(stdout: str) -> list[list[int]]:
    """Per-register bit lists (LSB-first) from qret simulate's [Register Final State] table."""
    lines = stdout.splitlines()
    start = next(
        (
            i
            for i, line in enumerate(lines)
            if line.strip().startswith("[Register Final State]")
        ),
        None,
    )
    if start is None:
        raise AssertionError(
            "no [Register Final State] section in qret simulate output"
        )

    registers: list[list[int]] = []
    for line in lines[start + 1 :]:
        stripped = line.strip()
        if not stripped:
            continue
        if stripped.startswith("note:") or (
            stripped.startswith("[") and not stripped.startswith("|")
        ):
            break  # end of the table
        if not stripped.startswith("|"):
            continue
        columns = [cell.strip() for cell in stripped.strip("|").split("|")]
        if not columns or columns[0] == "name" or columns[0].startswith("---"):
            continue  # header or separator row
        bits = columns[3]  # columns: name, size, bit_range, bits, value
        registers.append([int(char) for char in bits])
    return registers


# --- statistical comparison (superposition circuits) ---------------------------------------------


def _shot_to_bitstring(shot) -> str:
    if isinstance(shot, str):
        return shot  # raw measurement bit-string
    items = list(shot) if isinstance(shot, (tuple, list)) else [shot]
    return "".join(str(_to_bit(item)) for item in items)


def qdk_distribution(result) -> dict[str, float]:
    """Outcome-string -> frequency over all shots of a run_qir result."""
    counts = Counter(_shot_to_bitstring(shot) for shot in result)
    total = sum(counts.values())
    return {outcome: count / total for outcome, count in counts.items()}


def quration_sample_summary(stdout: str) -> dict[str, float]:
    """Outcome-string -> frequency from qret simulate --sample_summary's [Summary] section."""
    lines = stdout.splitlines()
    start = next(
        (i for i, line in enumerate(lines) if line.strip() == "[Summary]"), None
    )
    if start is None:
        raise AssertionError("no [Summary] section in qret simulate output")

    counts: dict[str, int] = {}
    total = 0
    for line in lines[start + 1 :]:
        stripped = line.strip()
        if stripped.startswith("count:"):
            total = int(stripped.split(":", 1)[1])
            continue
        if stripped.startswith("["):
            break
        match = re.match(r"^(\S+):\s*(\d+)\s*\(", stripped)  # e.g. "11: 1001 (50.05%)"
        if match:
            counts[match.group(1)] = int(match.group(2))
    if total == 0:
        raise AssertionError("qret simulate summary reported no samples")
    return {outcome: count / total for outcome, count in counts.items()}


def distributions_close(
    lhs: dict[str, float], rhs: dict[str, float], tolerance: float
) -> bool:
    """True if every outcome's frequency agrees within ``tolerance`` across both distributions."""
    return all(
        abs(lhs.get(outcome, 0.0) - rhs.get(outcome, 0.0)) <= tolerance
        for outcome in set(lhs) | set(rhs)
    )
