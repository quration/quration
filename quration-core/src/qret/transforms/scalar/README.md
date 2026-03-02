\page qret_transforms_scalar scalar transforms module

scalar では、個々の量子ビットや局所的なゲートに対する最適化パスを実装する。

## コードマップ

- `qret/transforms/scalar/decomposition.cpp` : 中間表現をより単純な命令で構成される中間表現に変換する
  - RX, RY, RZ, CY, CZ, CCX, CCY, CCZ を {X,Y,Z,S,SDag,T,TDag,CX} の組合せに変換する
- `qret/transforms/scalar/delete_consecutive_same_pauli.cpp` : 同じパウリ演算子を連続して適用している場合に削除する
- `qret/transforms/scalar/delete_opt_hint.cpp` : 最適化ヒント命令を削除する
- `qret/transforms/scalar/ignore_global_phase.cpp` : `qret::ir::GlobalPhaseInst` 命令を削除する
  - 量子回路に本質的に関係のない、グローバル位相を回転する命令を削除する
- `qret/transforms/static_condition_pruning.cpp` : 静的に定まる分岐を削除する
