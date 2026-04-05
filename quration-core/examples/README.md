\page examples Examples

qret ライブラリの使用例を実装する。

## 概要

### 回路の記述 (core)

| ファイル                | 説明                                                                            |
| ----------------------- | ------------------------------------------------------------------------------- |
| `quration-core/examples/grover.cpp`   | グローバー探索を実装する                                                        |
| `quration-core/examples/trotter.cpp`  | トロッター分解を用いたハミルトニアンの時間発展をシミュレートする回路を実装する  |
| `quration-core/examples/portable_function.cpp` | PortableFunction の利用例を示す                                        |
| `quration-core/examples/external_decompose_pass.cpp` | 外部パスの利用例を示す                                             |
| `quration-core/examples/external_mapping_pass.cpp`   | 外部マッピングパスの利用例を示す                                   |
| `quration-core/examples/create_random.cpp`           | ランダム回路生成の例を示す                                         |


#### `quration-core/examples/trotter.cpp`

パウリ文字列から構成されるハミルトニアンの時間発展をシミュレートする回路をトロッター分解を用いて実装する。

### 回路の記述 (algorithm)

アルゴリズムライブラリのサンプルは `quration-algorithm/examples/` に配置している。

### 中間表現

| ファイル                               | 説明                                                       |
| -------------------------------------- | ---------------------------------------------------------- |
| `examples/create_{}.cpp`               | 量子回路の中間表現の JSON ファイルを作成する               |
| `examples/external_decompose_pass.cpp` | 外部パスを使って、 Toffoli ゲートを T ゲートなどに分解する |

### SC_LS_FIXED_V0 へのコンパイル

| ファイル                                         | 説明                                              |
| ------------------------------------------------ | ------------------------------------------------- |
| `examples/compile_adder_to_distributed_chip.cpp` | 加算回路を SC_LS_FIXED_V0 (分散命令セット) にコンパイルする |

### その他

| ファイル                         | 説明                                              |
| -------------------------------- | ------------------------------------------------- |
| `examples/portable_function.cpp` | `PortableFunction` を使ってコラッツ関数を実装する |

## 参考文献

- [1] : Encoding Electronic Spectra in Quantum Circuits with Linear T Complexity
  - <https://arxiv.org/abs/1805.03662>
- [2] : Factoring with n+2 clean qubits and n-1 dirty qubits
  - <https://arxiv.org/abs/1706.07884>
- [3] : Stern–Brocot tree
  - <https://en.wikipedia.org/wiki/Stern-Brocot_tree>
