\page qret QRET

## 概要

QRET ライブラリは以下のモジュールから構成される。
各モジュールについてのドキュメントは、対応するディレクトリに含まれる `README.md` に配置している。

* `analysis/`
  * 中間表現を解析するツールを実装する
* `base/`
  * 全てのモジュールで広く使用される、基本的なクラスと関数を実装する
* `cmd/`
  * コマンドラインで使用する各種コマンドを実装する
  * CMake の configuration 時に `QRET_BUILD_APPLICATION` が `ON` の時にのみビルドされる
* `codegen/`
  * ターゲットとなる量子コンピュータで共通して使用されるクラスを定義する
  * ターゲットごとの実装は `target/` で行う
* `frontend/`
  * C++ で直観的に量子回路を定義する方法を提供する
* `gate/`
  * 様々な量子回路を実装する
* `ir/`
  * 量子回路の中間表現を定義する
* `math/`
  * 数学関数を実装する
* `parser/`
  * 各種ファイルのパーサを実装する
* `runtime/`
  * 量子回路のシミュレータ
* `target/`
  * ターゲットごとに、命令や最適化パスを実装する
    * 現在は sc_ls_fixed_v0 のみ実装している
* `transforms/`
  * 中間表現で記述された量子回路の最適化パスを実装する

### モジュールの依存グラフ

\dot
digraph G {
  rankdir=LR;
  node [shape=box];

  BA [label="base"];
  MA [label="math"];
  IR [label="ir"];
  PA [label="parser"];
  FR [label="frontend"];
  GA [label="gate"];
  RU [label="runtime"];
  AN [label="analysis"];
  TR [label="transforms"];
  CO [label="codegen"];
  TA [label="target"];

  BA -> MA;
  BA -> IR;
  MA -> IR;
  BA -> PA;
  IR -> FR;
  PA -> FR;
  FR -> GA;
  MA -> GA;
  IR -> RU;
  MA -> RU;
  IR -> AN;
  IR -> TR;
  BA -> CO;
  CO -> TA;
  IR -> TA;
  MA -> TA;
}
\enddot
