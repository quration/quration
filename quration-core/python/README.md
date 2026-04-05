# pyqret

## About pyqret

pyqret は量子回路の開発と誤り耐性量子コンピュータ等の具体的な量子コンピュータの命令列へのコンパイルを行うためのライブラリである。
C++ で実装された qret の Python ラッパーのため、内部実装を知りたい人は qret のドキュメントを参照すること。

### Features

* 量子回路の簡潔な記述
* シミュレータによる量子回路の検証
* 誤り耐性量子計算機向けのコンパイル

### Requirements

Linux /macOS (ARM64) / Windows11 をサポートしている。

* Linux の場合
  * Python 3.10, 3.11, 3.12, 3.13, 3.14
* macOS (ARM64) の場合
  * Python 3.10, 3.11, 3.12, 3.13, 3.14
* Windows11 の場合
  * Python 3.10, 3.11, 3.12, 3.13, 3.14

## Installation

### Quick install

環境 (OS, Python version) にあった `.whl` ファイルを `pip` でインストールする。
`<version>` の部分はインストールする pyqret のバージョン (例: `0.7.2`) に置き換える。

#### Python 3.12 以降 (3.12, 3.13, 3.14)

Python 3.12 以降では、共通の Stable ABI (abi3) 対応ファイルを使用する。

```sh
# Python 3.12, 3.13, 3.14 共通
pip install pyqret-<version>-cp312-abi3-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl
```

#### Python 3.10, 3.11

Python のバージョンごとに専用のファイルを使用する。

```sh
# Python 3.10 の場合
pip install pyqret-<version>-cp310-cp310-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl
```

### Install Python library from source

#### Requirements

* CMake >= 3.18
* C++ compiler (clang++ or VisualStudio)
  * clang++ >= 16.0.0 (checked in Linux)
  * Microsoft VisualStudio C++ 2022
  * g++ is not checked.
* C++ library: see `vcpkg.json`
* Python
  * Linux: 3.10, 3.11, 3.12, 3.13, 3.14
  * macOS(ARM64): 3.10, 3.11, 3.12, 3.13, 3.14
  * Windows: 3.10, 3.11, 3.12, 3.13, 3.14
* Python library: see `Pipfile`

#### How to install

##### Linux/macOS(ARM64) の場合

`pyproject.toml` のあるディレクトリ (=このファイルの親ディレクトリ) で次のコマンドを実行する。

```sh
SKBUILD_CMAKE_DEFINE="QRET_BUILD_PYTHON=ON" pip install .
```

アルゴリズムモジュールもインストールする場合は次を実行する。

```sh
SKBUILD_CMAKE_DEFINE="QRET_BUILD_PYTHON=ON;QRET_PYTHON_WITH_ALGORITHM=ON" pip install .[algorithm]
```

Vcpkg で環境構築している場合は次のコマンドを実行する。

```sh
SKBUILD_CMAKE_DEFINE="CMAKE_TOOLCHAIN_FILE=<VCPKG_ROOT>/scripts/buildsystems/vcpkg.cmake;QRET_BUILD_PYTHON=ON" pip install .
```

##### Windows の場合

`pyproject.toml` のあるディレクトリ (=このファイルの親ディレクトリ) で次のコマンドを実行する。

```cmd
set SKBUILD_CMAKE_DEFINE=QRET_BUILD_PYTHON=ON
pip install .
```

##### Install options

* `tests` : テストする場合に使用する。
* `docs` : ドキュメントをビルドする場合に使用する。
* `algorithm` : アルゴリズムモジュールを含めてインストールする場合に使用する。
* `dev` : pyqret の開発時に使用する。
