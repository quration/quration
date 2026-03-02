\page qret_runtime runtime module

中間表現のシミュレータを実装する。
シミュレータの使用方法の詳細については「量子回路のシミュレータによる検証」を参照すること。

## コードマップ

* `qret/runtime/clifford_state.h`
* `qret/runtime/dd.h`
* `qret/runtime/full_quantum_state.h`
  * `qret::runtime::FullQuantumState` : 任意のゲートを実行可能な量子状態クラス
* `qret/runtime/quantum_state.h`
  * `qret::runtime::QuantumState` : 量子状態の親クラス
    * 具体的な量子状態を実装する場合は、このクラスを継承して実装する
    * 例: `qret::runtime::ToffoliState`, `qret::runtime::FullQuantumState`
* `qret/runtime/simulator.h`
  * `qret::runtime::Simulator` : シミュレータクラス
    * `qret::runtime::QuantumState` を操作して、量子回路のシミュレーションを行う
  * `qret::runtime::SimulatorConfig` : シミュレータのコンフィグ
* `qret/runtime/toffoli_state.h`
  * `qret::runtime::ToffoliState` : 計算基底の重ね合わせ数が小さい場合に効率的な量子状態クラス
