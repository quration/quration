\page qret_base base module

全てのモジュールで広く使われる基本的な機能を提供する。

## コードマップ

* `qret/base/cast.h` : 明示的なキャストを行う関数
* `qret/base/graph.h` : グラフライブラリ
  * `qret::DiGraph` : 無向グラフクラス
  * `qret::Graph` : 有向グラフクラス
* `qret/base/gridsynth.h` : `GridSynth` のラッパー
  * 環境変数 `GRIDSYNTH_PATH` を適切に設定している場合のみ、ラッパー関数を使用できる
* `qret/base/iterator.h` : `std::list<std::unique_ptr<T>>` 型に対するイテレータ
  * `std::list` のイテレータを参照すると `std::unique_ptr<T>&` や `const std::unique_ptr<T>&` を得るが、このイテレータを参照すると `T&` や `const T&` を得る
* `qret/base/json.h` : `qret::PortableFunction` クラスのシリアライザ、デシリアライザ
* `qret/base/list.h` : リストに関する便利関数
* `qret/base/log.h` : ロガー
* `qret/base/option.h` : コマンドライン引数でオプションの値をセットする方法を提供する
* `qret/base/portable_function.h` : JSON にシリアライズ・デシリアライザ可能な古典関数
  * `qret::PortableFunction` : 古典関数クラス
  * `qret::PortableFunctionBuilder` : `qret::PortableFunction` を作成するクラス
  * `qret::PortableAnnotatedFunction` : bool の配列を引数に取り、 bool の配列を出力する `qret::PortableFunction`
* `qret/base/string.h` : 文字列に関する関数
* `qret/base/system.h` : バイナリを実行するラッパー関数
* `qret/base/time.h` : 時間に関する関数
* `qret/base/type.h` : 基本的な型 `qret::Int`, `qret::BigInt`, `qret::BigFraction` とその変換関数
