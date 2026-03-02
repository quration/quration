#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
build_dir="${1:-$root_dir/build}"
out_dir="${2:-$root_dir/quration-core/tests/data/circuit}"

"${build_dir}/examples/generate_quration_core_test_circuits" "${out_dir}"
