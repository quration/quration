\page qret_codegen codegen module

ターゲットの量子コンピュータに共通する親クラスを提供する。
各ターゲットごとの実装は `qret/target/` で行う。

## 概要

* `qret/codegen/asm_printer.h`
  * `qret::AsmPrinter` : アセンブリをストリームに出力するクラス
* `qret/codegen/asm_streamer.h`
  * `qret::AsmStreamer` : アセンブリ用のストリームクラス
  * 区切り文字/コメント記号/インデント幅などの出力書式も保持する
  * `ToString()` で生成済みテキストを取得し、ファイル I/O は呼び出し側で行う
* `qret/codegen/compile_info.h`
* `qret/codegen/dummy.h`
* `qret/codegen/machine_function_pass.h`
  * `qret::MachineFunctionPass` : ターゲットの関数についてのパスの親クラス
  * `qret::MFAnalysis` : `qret::MachineFunctionPass` の実行順や実行時間を記録する
  * `qret::MFPassManager`
    * 複数の `qret::MachineFunctionPass` を保持し、適切な順番でパスを実行する
    * 実行後に `qret::MFAnalysis` を返す
* `qret/codegen/machine_function.h`
  * `qret::MachineInstruction` : ターゲットの命令の親クラス
  * `qret::MachineBasicBlock` : ターゲットの基本ブロックの親クラス
  * `qret::MachineFunction` : ターゲットの関数の親クラス
