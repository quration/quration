# pyqret examples

pyqret ライブラリの使用例を実装する。

## 環境構築

スクリプトを実行するには以下のライブラリを必要とする。

- numpy
- qiskit

## 概要

- `create_adder_from_python.py`
  - Python ラッパーを使って加算回路を実装する例
- `create_adder_using_loaded_module.py`
  - `boolean.json` で定義された回路を使って加算回路を実装する例
- `get_operation_matrix.py`
  - 量子回路の操作行列を取得する例
  - `boolean.json` で定義された回路を使って加算回路を実装する例
- `trotter.py`
  - Python ラッパーを使ってトロッター分解を用いたハミルトニアンの時間発展をシミュレートする量子回路の実装例
