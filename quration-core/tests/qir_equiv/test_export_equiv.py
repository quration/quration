"""Export equivalence: qret's simulation of an IR must match QDK's simulation of the QIR exported
from it. The .ll fixture is only a source of an IR here; the comparison is single-direction on the
export side. Deterministic circuits are compared exactly (per-register bits); superposition circuits
are compared as outcome distributions over many shots.
"""

from __future__ import annotations

import json
from pathlib import Path

import pytest
from qir_sim import (
    distributions_close,
    qdk_distribution,
    qdk_register_bits,
    quration_register_bits,
    quration_sample_summary,
    run_qir,
)

DATA = Path(__file__).resolve().parents[1] / "data" / "qir"

# Deterministic circuits Quration can import; each is simulated by Quration and by QDK-on-export.
EXPORT_CIRCUITS = [
    "qsc_bool_record_output",
    "qsc_if_no_else_bool_record_output",
    "qsc_multiway_if_elif_else",
    "qsc_int_computation_interleaved_with_gate",
    "qdk_adaptive_gate_wrapper",
    "classical_branch_condition",
    "record_multi_qubit_gates",
]

# Superposition circuits, compared statistically (pure Clifford, so no gridsynth approximation).
SUPERPOSITION_CIRCUITS = ["bell_pair"]  # H + CX: entangled, 00/11 each ~0.5

SHOTS = 10000
SEED = 1
TOLERANCE = 0.05


@pytest.mark.parametrize("name", EXPORT_CIRCUITS)
def test_export_matches_quration_simulation(qret, tmp_path, name):
    src = DATA / f"{name}.ll"
    ir = tmp_path / "ir.json"
    qret("parse", "-i", str(src), "-f", "qir", "-o", str(ir))
    entry = json.loads(ir.read_text())["entry_function"]

    sim_out = qret(
        "simulate",
        "-i",
        str(ir),
        "-f",
        entry,
        "-s",
        "Toffoli",
        "--max_superpositions",
        "16",
        "-n",
        "1",
    )
    quration = quration_register_bits(sim_out)

    exported = tmp_path / "exported.ll"
    qret("export", "-i", str(ir), "-o", str(exported))
    qdk = qdk_register_bits(
        run_qir(exported.read_text()), [len(bits) for bits in quration]
    )

    assert quration == qdk, f"{name}: quration={quration} qdk={qdk}"


@pytest.mark.parametrize("name", SUPERPOSITION_CIRCUITS)
def test_export_distribution_matches_quration(qret, tmp_path, name):
    src = DATA / f"{name}.ll"
    ir = tmp_path / "ir.json"
    qret("parse", "-i", str(src), "-f", "qir", "-o", str(ir))
    entry = json.loads(ir.read_text())["entry_function"]

    sim_out = qret(
        "simulate",
        "-i",
        str(ir),
        "-f",
        entry,
        "-s",
        "FullQuantum",
        "-n",
        str(SHOTS),
        "--sample_summary",
        "--seed",
        str(SEED),
    )
    quration = quration_sample_summary(sim_out)

    exported = tmp_path / "exported.ll"
    qret("export", "-i", str(ir), "-o", str(exported))
    qdk = qdk_distribution(run_qir(exported.read_text(), shots=SHOTS, seed=SEED))

    assert distributions_close(quration, qdk, TOLERANCE), (
        f"{name}: quration={quration} qdk={qdk}"
    )
