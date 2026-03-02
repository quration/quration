\page qret_frontend frontend module

量子回路を高レベルで記述する方法を提供する。
詳細については「量子回路の定義」を参照すること。

## コードマップ

* `qret/frontend/argument.h` : 量子回路の引数クラス
  * `qret::frontend::QuantumBase<ObjectType, SizeType>` : 量子ビット、量子ビット列、レジスタ、レジスタ列のテンプレートクラス
    * `Qubit = QuantumBase<ObjectType::Qubit, SizeType::Single>` : 量子ビットクラス
    * `Qubits = QuantumBase<ObjectType::Qubit, SizeType::Array>` : 量子ビット列クラス
    * `Register = QuantumBase<ObjectType::Register, SizeType::Single>` : レジスタクラス
    * `Registers = QuantumBase<ObjectType::Register, SizeType::Array>` : レジスタ列クラス
  * 列クラスは `[idx]` のようにアクセスでき、量子ビット・レジスタを返す
  * フロントエンドの回路クラスは以上4つのクラスのみを引数に取ることができる
* `qret/frontend/attribute.h` : 量子ビットのアトリビュート
  * 現在実装しているアトリビュートは最適化ヒントのみである
  * 最適化ヒントは3種類ある
    * `|0>` である
      * clean な補助量子ビットとして使用することを示す
    * 確率 `p` で `|0>` である
      * 量子ビットを測定したときに、 `|0>` を観測する確率が `p` である
    * dirty な補助量子ビットとして使用する
* `qret/frontend/builder.h`
  * `qret::frontend::CircuitBuilder` : 回路のビルダー
    * 回路の作成を容易にする
    * 作成した回路のキャッシュ機能も持つ
* `qret/frontend/circuit_generator.h`
  * `qret::frontend::CircuitGenerator` : 回路のジェネレータ
    * ジェネレータはパラメータを引数に取り、回路を作成する
    * `qret::frontend::CircuitGenerator` は作成した回路をキャッシュするか否か制御できる
* `qret/frontend/circuit.h`
  * `qret::frontend::Circuit` : **フロントエンドの** 回路クラス
    * 中間表現の回路クラスへのポインタをメンバに持つ
    * 関数オブジェクトとして呼び出すと、現在ビルダーで定義中の回路に、この回路の呼び出し命令を挿入する
* `qret/frontend/control_flow.h` : 分岐命令
  * `qret::frontend::control_flow::If`, `qret::frontend::control_flow::Else`, `qret::frontend::control_flow::EndIf`
    * レジスタの値に応じて実行する命令を変更する場合に使用する
* `qret/frontend/functor.h` : ファンクタ
  * `qret::frontend::Adjoint` : 回路を adjoint な回路に変換するファンクタ
  * `qret::frontend::Controlled` : 回路を controlled な回路に変換するファンクタ
* `qret/frontend/intrinsic.h` : intrinsic命令
  * 中間表現で定義されている命令に対応するゲートを実装する
* `qret/frontend/openqasm2.h` : OpenQASM2 のパーサ
  * OpenQASM2 ファイルをパースし、回路を作成する
  * `qret/frontend/qelib1.inc` は OpenQASM2 言語で記述された OpenQASM2 の標準ライブラリ

## その他

* `qret/frontend/qelib1.inc` は [qiskit](https://github.com/Qiskit/qiskit/blob/4f7b54a6e7b9c2ea549cf55eeac5db21091b3802/qiskit/qasm/libs/qelib1.inc) より取得した
* OpenQASM2 のパーサは [Open Quantum Assembly Language](https://arxiv.org/abs/1707.03429) で定義されている文法をほぼ全てパース出来るように実装した
  * **TODO** validator を実装する
