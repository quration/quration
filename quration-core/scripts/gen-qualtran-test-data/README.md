# Generate Qualtran Test Data

This directory contains a script that generates JSON test data files for the Qualtran frontend.
The generated files are written to `quration-core/tests/data/qualtran/` and loaded by the `QualtranFrontend` tests.

## Generated Files

| File                              | Description                                                                                                                                                                                                                                                                 |
| --------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `direct_gates.json`               | Every directly-mapped gate, including TDag/SDag (is_adjoint=True, not Adjoint(subbloq))                                                                                                                                                                                     |
| `parametrized_gates.json`         | Rx/Ry/Rz, XPowGate/YPowGate/ZPowGate, CRz, SU2RotationGate (CZPowGate is covered separately, since it hides an And compute/uncompute pair)                                                                                                                                  |
| `preset_gate_calls.json`          | CHadamard/TwoBitSwap/TwoBitCSwap, each a preset CallInst (not inlined)                                                                                                                                                                                                      |
| `and_gates.json`                  | And(compute)→And(uncompute) round trip, plus a separate And(cv1=0, cv2=1) compute-only leaf                                                                                                                                                                                 |
| `czpow_multiple.json`             | Two atomic CZPowGate leaves (differing exponent), each drawing a non-overlapping slice of the shared anc_clean pool                                                                                                                                                         |
| `swap_cswap_decomposed.json`      | Swap(bitsize=3)→CSwap(bitsize=3), decomposed serialization                                                                                                                                                                                                                  |
| `swap_cswap_leaf.json`            | Same circuit as `swap_cswap_decomposed.json`, but at leaf (atomic) serialization; both must produce the same IR                                                                                                                                                             |
| `state_basis.json`                | ZeroState/OneState/PlusState/MinusState, each immediately consumed by its own allocated qubit                                                                                                                                                                               |
| `producer_boundary.json`          | Allocate/Free (clean and dirty), an unfreed Allocate escaping at the root boundary, and AllocatingSubBloq() escaping at a sub-circuit boundary (a different call site)                                                                                                      |
| `int_state_effect.json`           | IntState(val=5, bitsize=3) → IntEffect(val=5, bitsize=3); both are non-atomic in Qualtran                                                                                                                                                                                   |
| `consumer_variants.json`          | Allocate(1)→ZeroEffect (MarkAsClean fires, lands in anc_clean) alongside Allocate+X/H→One/Plus/MinusEffect, actually preparing the claimed state before consuming it (no consumer-side MarkAsClean, lands in anc_operate)                                                   |
| `alias_variants.json`             | Split2→Join2 (with an X sandwiched on one split bit), Cast, and Partition round trip -- none of these AliasOnly bloqs emit a gate of their own                                                                                                                              |
| `subcircuit_variants.json`        | Two named custom Bloq instances (must resolve to the same shared Circuit), an anonymous 3-level CompositeBloq nesting, and two sub-circuits each forwarding their own scratch ancilla to the root's anc_clean pool without colliding                                        |
| `adjoint_variants.json`           | Every AdjointWrapper scenario: custom composite reversal, self-adjoint preset, inlined Gate terminal, name resolution across differently-parameterized instances, even/odd nesting depth, AliasOnly, a composite with its own CleanAncilla, and Producer/Consumer role flip |
| `adjoint_decomposed.json`         | Adjoint(MyGate()) serialized at max_depth=2, so Qualtran expands the Adjoint wrapper itself into a Composite rather than leaving it an AdjointWrapper atom                                                                                                                  |
| `controlled_variants.json`        | Every ControlledWrapper scenario: N=1 direct dispatch, exact-gate-via-And synthesis, rotation and GlobalPhase controls, multi-bit and negative-control AND-ladders, nested Controlled, and a decomposable sub-circuit                                                       |
| `bloq_counts_variants.json`       | GlobalPhase bloq_counts entries materialized regardless of source-order interleaving with regular decomposition instructions, plus an ArbitraryClifford bloq_counts entry that only fires the LOG_WARN "not already emitted via decomposition" branch                       |
| `leaf_root_hadamard.json`         | A directly-mapped gate as root (no ancilla, no synthesis)                                                                                                                                                                                                                   |
| `leaf_root_preset.json`           | A preset CircuitGenerator gate as root (dispatches via a Call, not an inline sequence)                                                                                                                                                                                      |
| `leaf_root_czpow.json`            | CZPowGate as root: a Gate terminal needing its own scratch ancilla                                                                                                                                                                                                          |
| `leaf_root_and_compute.json`      | And() as root: a NewQubitProducer Gate terminal, its own ancilla produced fresh rather than forwarded from an existing pool                                                                                                                                                 |
| `leaf_root_adjoint.json`          | An AdjointWrapper as root, unwrapping to a Gate terminal (CZPowGate, exponent negated)                                                                                                                                                                                      |
| `leaf_root_controlled.json`       | A ControlledWrapper as root; TGate needs ancilla, exercising that branch                                                                                                                                                                                                    |
| `unsupported_bloq.json`           | An unknown bloq name; must throw `UnsupportedQualtranBloq`                                                                                                                                                                                                                  |
| `controlled_unsupported_int.json` | `Controlled` with a non-QBit (integer-equality) control register dtype; must throw `UnsupportedQualtranBloq`                                                                                                                                                                |

`MalformedJson` is not backed by a fixture file; it feeds an invalid JSON string directly.

LeafRoot fixtures cover every bloq kind that can plausibly appear standalone as a root. Bookkeeping
bloqs are not expected to appear standalone as a root and are already covered by regression tests
elsewhere.

## Requirements

- Python 3.10+
- `qualtran` 0.7.0
- `protobuf` (installed as a dependency of `qualtran`)

## Usage

Run from the repository root, in an environment with the requirements above installed:

```bash
python quration-core/scripts/gen-qualtran-test-data/gen_qualtran_test_data.py
```

Output directory: `quration-core/tests/data/qualtran/`

## When to Regenerate

- When adding or changing the supported gate set
- When modifying the test circuit design
