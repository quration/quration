# Quration: Quantum Resource Estimation Toolchain

Quration is a toolchain for exploring design space of fault-tolerant quantum computer (FTQC) architectures and curate optimization strategies and co-design techniques to find a state-of-the-art system designs.  

This project is developed and maintained by RIKEN, Fixstars, The University of Osaka, and QunaSys.
The development of Quration is in progress, and their backward compatibility might be broken in future updates.  

## License

- External libraries (`./externals/`) are re-distributed under each library's license.
- Application generators (`./quration-algorithm/`) are distributed only for research purpose.
- The other quration libraries (`./quration-core/`, `./quration-visualize/`, `./quration-docs/`) are distributed under MIT-license.

## Features

Quration consists of three components; `quration-algorithm`, `quration-core`, and `quration-visualizer`.

See `./quration-docs/tutorial/` for tutorials of these programs.

- `quration-algorithm`: Generate popular quantum algorithms and their subroutines as Quration-IR
  - Quration-IR is a succinct LLVM-like language dedicated for evaluating large-scale FTQC applications.
  - Algorithm generators for the following applications are implemented.
    - Trotter-based quantum simulation
    - Qubitization-based quantum phase estimation by [R. Babbush et al.](https://arxiv.org/abs/1805.03662)
    - Shor's factoring by [C. Gidney](https://arxiv.org/pdf/1706.07884)

- `quration-core`: Manipulate Quration-IR and compile them into a target instruction set.
  - Quration-IR can be handled with an application named `qret`. Avaialble commands for `qret` are as follows (see manual for details).
    - `qret parse`: Parse existing formats (e.g. QASM) into Quration-IR
    - `qret print`: Print Quration-IR as an LLVM-like form
    - `qret simulate`: Simulate Quration-IR with several types of backend
    - `qret opt`: Optimize Quration-IR with pre-defined or user-defined workflow
    - `qret compile`: Compile Quration-IR into pre-defined or user-defined instruction sets of FTQCs
    - `qret profile`: Profile compiled programs and generate resource estimation profiles
    - `qret asm`: Dump compiled programs in a simple ascii form
  - Currently available target instructions:
    - We currently support FTQC based on surface codes, lattice surgery, code-deformation-based S-gate, and magic-state cultivation, and communication.
      - `SC_LS_FIXED_V0_Dim2`: Standard architecture, i.e., 2D array of surface codes
      - `SC_LS_FIXED_V0_Dim3`: Architecture with several layers of 2D surface-code arrays allowing transversal CNOTs between layers
      - `SC_LS_FIXED_V0_Dist`: Standard distributed FTQC, where each nodes consists of 2D surface code arrays

- `quration-visualizer`: Visualize execution traces and compare resource estimation profiles on browers


## Install prebuilt binaries

Each GitHub Release carries an archive per platform:

| Platform | Asset |
| --- | --- |
| Linux (x86_64) | `qret-ubuntu-latest.tar.gz` |
| macOS (Apple Silicon) | `qret-macos-latest.tar.gz` |
| Windows (x64) | `qret-windows-latest.zip` |

Download from the command line.

```sh
gh release download <tag> -R quration/quration -p 'qret-ubuntu-latest.tar.gz'
# or
curl -LO https://github.com/quration/quration/releases/download/<tag>/qret-ubuntu-latest.tar.gz
```

Extract it:

```sh
tar xzf qret-ubuntu-latest.tar.gz        # tar -xf qret-windows-latest.zip on Windows
cd qret-ubuntu-latest
```

On Linux and macOS the binaries carry no embedded library path, so point the
dynamic linker at the bundled `lib/` before running them. Windows needs no
library setup -- the DLLs are installed next to the executables in `bin/`.

```sh
export LD_LIBRARY_PATH="$PWD/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"        # Linux
export DYLD_LIBRARY_PATH="$PWD/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"  # macOS
```

`qret` shells out to `gridsynth` to decompose rotation gates, and looks it up
through `GRIDSYNTH_PATH`, the cabal install directories, the working directory
and `PATH` -- never `bin/`. Point it at the bundled copy, giving the full path
to the binary rather than the directory holding it:

```sh
export GRIDSYNTH_PATH="$PWD/bin/gridsynth"   # Linux, macOS
```

```bat
set GRIDSYNTH_PATH=%CD%\bin\gridsynth.exe
```

Without it, any pass that decomposes a rotation fails with
`cannot run gridsynth for this path: gridsynth`.

`qret` and the generators take every path as an argument, so they run from
anywhere. Three examples -- `compile_adder_to_distributed_chip`,
`external_mapping_pass` and `external_decompose_pass` -- instead read their
input through paths hard-coded relative to the working directory, so run those
**from the extracted root**.

```sh
./bin/qret --help
./bin/create_qpe \
  --input quration-algorithm/benchmark_generators/data/sample_qpe.json \
  --output qpe.json
```


## Build Quration-Core and Quration-Algorithm from source

### Environment

We expect our library will work on Windows, MacOS, and Linux with compilers GCC, Clang, and MSVS. This library is tested on the following environments.

- Windows 11 + Microsoft Visual Studio 2022
- Ubuntu 24.04 + Clang

### Install dependencies

- Install `vcpkg`: 
  - vcpkg is a package manager for C++ libraries, which is used for downloading dependent C++ libraries.
  - See [vcpkg installation page](https://learn.microsoft.com/ja-jp/vcpkg/get_started/get-started?pivots=shell-bash) for how to install vcpkg.
  - Make sure that an environment variable `VCPKG_ROOT` is set to the path to vcpkg folder.

- Install cmake:
  - Windows: Install cmake from official web sites
  - Ubuntu: Instlal with `apt install cmake`

- Optional
  - Python: required if you want to build python binding
  - gridsynth: required if you want to decompose Pauli-rotation gates into Clifford+T
    - Since official binaries in [gridsynth websites](https://www.mathstat.dal.ca/~selinger/newsynth/) do not work on recent Linux environment, our builds are re-distributed with GNU GPL.
      - Windows: `./externals/bin/gridsynth.exe`
      - Linux: `./externals/bin/gridsynth`
      - MacOS: `./externals/bin/gridsynth_macos`


### Build process

Please type the following commands at the root of this repository.
```
cmake --preset dist
cmake --build --preset build-dist
```

Then, you can find the following binaries in `./bin/main` and `./bin/benchmark_generators`.

- `qret`: main program
  - In the case of windows, it is dependent on `qret-core.dll`, `yaml-cpp.dll`, and `boost_program_options-*.dll`
- Quration-IR generators:
  - Shor's factoring and subroutines
    - `create_period_finding`: Generate the whole period finding programs.
    - `create_multi_controlled_mod_bi_mul_imm`: Generate circuits for Mod-bimultiplication.
    - `create_add_craig`: Generate craig-adder circuits.
    - `create_add_cuccaro`: Generate cuccaro-adder circuits.
    - `create_qrom`: Generate QROM circuits.
  - Qubitization-based quantum phase estimation and subroutines:
    - `create_qpe`: Generate the whole qunatum phase estimation programs
    - `create_select`: Generate SELECT circuits
    - `create_prepare`: Generate PREPARE circuits
  - Trotter-based Quantum-dynamics simulation
    - `create_trotter`: Generate the whole dynamics simulation


## quration-visualizer

`quration-visualizer` is a streamlit based web visualization of compiled FTQC programs.

### Install

Install dependent python libraries with
```
pip install numpy pandas plotly streamlit graphviz
```

### Usage
Then, you can run web applications as follows

* `streamlit run visualize_profile.py` : Visualizer for profile information
* `streamlit run visualize_computational_process.py` : Visualizer for traces of quantum programs
