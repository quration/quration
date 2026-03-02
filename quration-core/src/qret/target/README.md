\page qret_target target module

バックエンドの量子コンピュータに固有の命令やパスを実装する。

## コードマップ

量子コンピュータ固有の情報やパスを登録する機能を以下のファイルで実装する。

* `qret/target/target_enum.h`
* `qret/target/target_machine.h`
  * `qret::TargetMachine` : ターゲットのマシンに関する情報を保持するクラス
* `qret/target/target_registry.h`
  * `qret::Target` : ターゲットに固有の情報を保持するクラス
    * `name` : ターゲット識別子（例: `sc_ls_fixed_v0`, `tutorial_nisq_v0`）
    * `short_desc` : ターゲットの短い説明
    * `asm_printer_ctor` : `qret::AsmPrinter` 生成関数（optional capability）
    * `CreateAsmStreamer()` : asm 出力時に使う `qret::AsmStreamer` を生成する
    * `HasAsmPrinter()` / `TryCreateAsmPrinter()` で asm 機能の有無を安全に判定できる
  * `qret::TargetRegistry` : ターゲットのレジストリクラス
    * `target_map` : target 名 -> `qret::Target`
    * `compile_backend_map` : target 名 -> `qret::TargetCompileBackend`
    * `RegisterTarget` / `RegisterCompileBackend` / `RegisterAsmPrinter` を提供する
    * compile backend と optional な asm printer capability を一元管理する

## バックエンド

* SC_LS_FIXED_V0: `qret/target/sc_ls_fixed_v0/` で実装する
* TUTORIAL_NISQ_V0: `qret/target/tutorial_nisq_v0/` で実装する
