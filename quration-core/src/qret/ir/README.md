\page qret_ir IR module

QRET ライブラリにおける、量子回路の中間表現を実装する。
中間表現の設計や命令セットについては「量子回路の中間表現」」を参照すること。

## コードマップ

* `qret/ir/basic_block.h`
  * `qret::ir::BasicBlock` : 基本ブロッククラス
    * 基本ブロックは命令リストで function に属する
    * 基本ブロックに含まれる命令は分岐せず連続して実行する
    * well formed な基本ブロックの命令リストの末尾は terminator 命令である
    * well formed な基本ブロックの命令リストの途中に terminator 命令は含まれない
* `qret/ir/function.h`
  * `qret::ir::Function` : function クラス
* `qret/ir/context.h`
  * `qret::ir::IRContext` : 中間表現のコンテキスト
* `qret/ir/function_pass.h`
  * `qret::ir::FunctionPass` : function パスの親クラス
    * function を最適化するパスはこのクラスを継承する
* `qret/ir/instruction.h`
  * `qret::ir::Instruction` : 命令クラスの親クラス
* `qret/ir/instructions.h` : 様々な命令クラス
  * `qret::ir::MeasurementInst`
  * `qret::ir::UnaryInst`
  * `qret::ir::ParametrizedRotationInst`
  * `qret::ir::BinaryInst`
  * `qret::ir::TernaryInst`
  * `qret::ir::MultiControlInst`
  * `qret::ir::GlobalPhaseInst`
  * `qret::ir::CallInst`
  * `qret::ir::CallCFInst`
  * `qret::ir::DiscreteDistInst`
  * `qret::ir::BranchInst`
  * `qret::ir::SwitchInst`
  * `qret::ir::ReturnInst`
  * `qret::ir::CleanInst`
  * `qret::ir::CleanProbInst`
  * `qret::ir::DirtyInst`
* `qret/ir/json.h` : 中間表現の JSON シリアライザ・デシリアライザ
* `qret/ir/metadata.h` : 命令に付与するメタデータ
* `qret/ir/module.h`
  * `qret::ir::Module` : モジュール
* `qret/ir/value.h` : 中間表現における値を定義する
