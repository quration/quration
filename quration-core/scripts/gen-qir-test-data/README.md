# Generate QIR Test Data

This directory contains a script that generates QIR (`.ll`) test data files for the QIR importer
and equivalence tests. The generated files are written to `quration-core/tests/data/qir/` and loaded
by the `QirFrontend` / `QirExporter` / `qir_equiv` tests.

Each fixture is one `build_*` function that constructs a module through the **pyqir builder API**:
declarations, entry-point attributes, and module flags come from pyqir (never string templating),
and every module is emitted at the adaptive profile, so the files are valid QIR that QDK and
qir-runner both accept — not fragments only Quration's lenient importer would tolerate. Each module
is validated with pyqir before it is written. Adding a fixture is adding one `build_*` function and
one entry in the `FIXTURES` table.

Negative fixtures are complete in every other respect, so the only reason to reject them is the one
unsupported construct they contain — never a missing declaration.

## Generated Files

| File                            | Description                                                                                |
| ------------------------------- | ------------------------------------------------------------------------------------------ |
| `classical_branch_condition.ll` | A branch conditioned on `icmp sgt (zext read_result), 0`, recording a 3-element tuple      |
| `classical_int_chain.ll`        | Two int-op subtrees sharing a measured input, interleaved to pin classical-synthesis order |
| `reset_preset.ll`               | X then `reset` then measure (reset drives the qubit back to \|0>)                          |
| `swap_preset.ll`                | X, `swap`, then measure both qubits                                                        |
| `qsc_mresetz.ll`                | `mresetz` + qis `read_result__body` + a conditional flip, then measures                    |
| `pyqir_style_read_result.ll`    | `mz` + qis `read_result__body` (pyqir spelling) + a conditional flip                       |
| `rzz_rxx_ryy.ll`                | Two-qubit rotations rzz/rxx/ryy (the importer decomposes these into cnot·rz·cnot)          |
| `clifford_gates.ll`             | Every single-qubit named gate: id/x/y/z/h/s/s†/t/t†, each mapping to its own IR opcode     |
| `rotations_and_sx.ll`           | Single-qubit rx/ry/rz with angles (preserved on import) plus `sx` (decomposes to RX+phase) |
| `record_multi_qubit_gates.ll`   | Bit-flip multi-qubit gates X/Y/CX/CY/CCX with recorded results (deterministic equivalence) |
| `barrier.ll`                    | X, `barrier`, measure — the barrier is a scheduling hint and is dropped on import          |
| `qsc_alias_gate_names.ll`       | The `cx`/`m` gate-name aliases the importer accepts as cnot/mz                             |
| `pyqir_multi_function.ll`       | The entry point calls a user-defined function (valid QIR both QDK and qir-runner run)      |
| `backward_branch.ll`            | An unbounded loop with a back-edge; must be rejected for the backward branch               |
| `cyclic_sub_circuits.ll`        | sub_a → sub_b → sub_a; must be rejected for the recursion cycle                            |
| `duplicate_record_output.ll`    | The same result recorded twice; must be rejected for the duplicate record                  |
| `unknown_function_call.ll`      | A call to an unsupported runtime function; must be rejected for the unknown call           |
| `unsupported_qis_gate.ll`       | A qis gate name outside the intrinsic catalog; must be rejected as an unknown gate         |
| `empty_circuit.ll`              | A valid entry point with no gates; imports to an empty circuit                             |

Not generated here: `switch`-based fixtures (neither pyqir nor QDK can emit a QIR `switch`; only
Quration's exporter does, so switch behaviour is covered by round-trip tests). The one exception,
`switch_join_phi.ll`, also contains an unsupported `phi` no generator can emit and is kept
hand-written. Authentic compiler output (`qsc_*`, `qdk_*`, `bell_pair`) is left as-is.

## Requirements

- Python 3.8+
- [`pyqir`](https://pypi.org/project/pyqir/) (used to build and validate every module)

Optional, only to additionally confirm the generated QIR simulates:

- The Microsoft `qdk` package (`pip install qdk`), which exposes `qdk.simulation.run_qir`
- A built `qir-runner` (from the qir-alliance/qir-runner repository)

## Usage

Run from the repository root, in an environment with the requirements above installed:

```bash
python quration-core/scripts/gen-qir-test-data/gen_qir_test_data.py
```

Output directory: `quration-core/tests/data/qir/`

The script is idempotent — running it again reproduces the same files.

## When to Regenerate

- When adding or changing a `build_*` fixture
- When the importer's or exporter's supported gate set or QIR shape changes
