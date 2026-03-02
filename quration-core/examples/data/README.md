# Data {#examples_data}

- `circuit/` : 中間表現やSC_LS_FIXED_V0中間ファイル
- `OpenQASM2/` : OpenQASM2 の論文 [1] よりコピーした量子回路の例
- `topology/` : トポロジーファイル
- `pipeline/` : パイプラインファイル

## `circuit/`

- `circuit/add_craig_5.json` : `create_add_craig.cpp` を実行したときに出力される JSON ファイル
  - 幅 5 の Craig の加算回路の中間表現を定義する
- `circuit/add_cuccaro_5.json` : `create_add_cuccaro.cpp` を実行したときに出力される JSON ファイル
  - 幅 5 の Cuccaro の加算回路の中間表現を定義する
- `circuit/add_cuccaro_5_pbc.json` : `circuit/add_cuccaro_5.json` のすべての量子ビットを測定したもの
  - `sc_ls_fixed_v0_enable_pbc_mode=true` でコンパイルする際はすべての量子ビットを測定する必要がある
- `circuit/branch.json` : 測定値に依存した分岐を行う回路
- `circuit/decompose_using_external_pass.json`
- `circuit/mapping_using_external_pass.json`
- `circuit/random.json` : `create_random.cpp` を実行したときに出力される JSON ファイル

## `pipeline/`

- `pipeline/decompose_using_external_pass.yaml`
- `pipeline/mapping_based_on_topology_file.yaml`
- `pipeline/mapping_using_external_pass.yaml`
- `pipeline/qret_compile_branch.yaml` : 測定値に依存した分岐をコンパイルするパスの例
- `pipeline/qret_compile_cultivation.yaml`
- `pipeline/qret_compile_pbc.yaml` : PBC モード (`sc_ls_fixed_v0_enable_pbc_mode=true`) にコンパイルするパスの例
- `pipeline/qret_compile_random.yaml`
- `pipeline/qret_compile.yaml` : SC_LS_FIXED_V0にコンパイルするパスの例
- `pipeline/qret_opt.yaml` : 中間表現の最適化パスの例
- `pipeline/runtime_simulation_pruning.yaml` :`sc_ls_fixed_v0_runtime_simulation_pruning_seed` に応じて `PROBABILITY_HINT` の値を決定し、この分岐に基づくリソース推定を行うパスの例

## `topology/`

- `topology/dist_all.yaml`
- `topology/dist_circle.yaml`
- `topology/dist_line.yaml`
- `topology/grid.yaml`
- `topology/plane.yaml`
- `topology/tutorial.yaml`

## 参考文献

- [1] : Open Quantum Assembly Language
  - <https://arxiv.org/abs/1707.03429>
