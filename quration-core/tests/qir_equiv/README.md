# qir_equiv — QIR ⇄ Quration simulation-equivalence tests

A pytest suite that checks `qret`'s QIR export/import against the external oracle **QDK**
(`qdk.simulation.run_qir`). It catches the kind of semantic mistake the C++ (structural) gtest
suite cannot — a symmetric bug where export and import share the same wrong convention.

- **Export equivalence** ([test_export_equiv.py](test_export_equiv.py)):
  `qret simulate`(original IR) must equal `run_qir`(the QIR that `qret export` produces from that IR).
- **Import equivalence** ([test_import_equiv.py](test_import_equiv.py)):
  `run_qir`(input `.ll`) must equal `qret simulate`(the IR that `qret parse` imports from it).

Within each direction, deterministic circuits are compared exactly (per-register, at the bit level)
and superposition circuits are compared as outcome distributions over many shots (within a
tolerance). Both live in [test_export_equiv.py](test_export_equiv.py) /
[test_import_equiv.py](test_import_equiv.py).

Only circuits that record their outputs are compared here: QDK returns the recorded results and
`qret simulate` reports the matching Output registers. Circuits without `record_output` are valid
QIR and covered by the importer tests, but expose no Output-register observable to compare.

## Prerequisites

This suite is intentionally not part of the C++ ctest run; it needs two things that suite does not:

1. A built `qret` CLI binary (an `all-*` preset produces `build/main/qret`). If it cannot be found
   the tests skip with an actionable message.
2. A Python environment with the `qdk` package installed. Without it the test modules are ignored at
   collection (see `conftest.py`).

Run `pytest -rs` to see the reason for any skip.

## Usage

```sh
# Run with a Python that has qdk, pointing at the built qret binary.
<qdk-python> -m pytest quration-core/tests/qir_equiv -rs --qret-bin build/main/qret
```

`--qret-bin` may be omitted to auto-discover `build*/main/qret`. The `qret` shared library and
`GRIDSYNTH_PATH` are configured for the subprocess by `conftest.py`.
