# Generate Quration Core Test Circuits

This directory provides a helper to regenerate the JSON circuits stored under
`quration-core/tests/data/circuit`.
These files are consumed by quration-core tests when algorithm support is disabled.

Build the generator (requires algorithm enabled) and run the script:

```bash
cmake --build build --target generate_quration_core_test_circuits
quration-core/scripts/generate-quration-core-circuit/generate.sh
```

`generate.sh` accepts optional arguments: `build_dir` and `output_dir`.
