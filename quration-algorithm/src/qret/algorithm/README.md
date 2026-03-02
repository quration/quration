\page qret_gate gate module

## 概要

### `qret/frontend/intrinsic.h`

| Name                 | Summary                                                                                                      |
| -------------------- | ------------------------------------------------------------------------------------------------------------ |
| Measure              | Measures a qubit                                                                                             |
| I                    | Performs the identity operation on a single qubit.                                                           |
| X                    | Applies the Pauli X gate.                                                                                    |
| Y                    | Applies the Pauli Y gate.                                                                                    |
| Z                    | Applies the Pauli Z gate.                                                                                    |
| H                    | Applies the Hadamard transformation to a single qubit.                                                       |
| S                    | Applies the pi/4 phase gate to a single qubit.                                                               |
| SDag                 | Applies the adjoint of S gate.                                                                               |
| T                    | Applies the pi/8 phase gate to a single qubit.                                                               |
| TDag                 | Applies the adjoint of T gate.                                                                               |
| RX                   | Applies a rotation about the x-axis by a given angle.                                                        |
| RY                   | Applies a rotation about the y-axis by a given angle.                                                        |
| RZ                   | Applies a rotation about the z-axis by a given angle.                                                        |
| CX                   | Applies the controlled Pauli X gate to a pair of qubits.                                                     |
| CY                   | Applies the controlled Pauli Y gate to a pair of qubits.                                                     |
| CZ                   | Applies the controlled Pauli Z gate to a pair of qubits.                                                     |
| CCX                  | Applies the doubly controlled Pauli X gate to three qubits.                                                  |
| CCY                  | Applies the doubly controlled Pauli Y gate to three qubits.                                                  |
| CCZ                  | Applies the doubly controlled Pauli Z gate to three qubits.                                                  |
| MCX                  | (Deprecated) Applies multi controlled Pauli X gate to a qubit.                                               |
| GlobalPhase          | Performs global phase rotation.                                                                              |
| CallCFFunction       | Calls a classical function.                                                                                  |
| DiscreteDistribution | Generates a classical random integer according to an arbitrary distribution defined by non-negative weights. |

### `qret/algorithm/arithmetic/`

#### ブーリアン演算

* `qret/algorithm/arithmetic/boolean.h`

| Name                 | Summary                                                           |
| -------------------- | ----------------------------------------------------------------- |
| TemporalAnd          | Performs temporal logical AND.                                    |
| UncomputeTemporalAnd | Performs measurement based uncomputation of temporal logical AND. |
| RT3                  | Performs a toffoli-like operation, while adding a relative phase. |
| IRT3                 | Performs a toffoli-like operation, while adding a relative phase. |
| RT4                  | Performs a toffoli-like operation, while adding a relative phase. |
| IRT4                 | Performs a toffoli-like operation, while adding a relative phase. |
| MAJ                  | Performs a in-place majorization.                                 |
| UMA                  | Performs a in-place unmajorization.                               |

#### 整数演算

* `qret/algorithm/arithmetic/integer.h`

| Name                       | Summary                                                                                    |
| -------------------------- | ------------------------------------------------------------------------------------------ |
| Inc                        | Performs increment.                                                                        |
| MultiControlledInc         | Performs controlled increment.                                                             |
| Add                        | Performs addition without using auxiliary qubits.                                          |
| AddCuccaro                 | Performs addition using Cuccaro's Ripple-Carry Adder (RCA).                                |
| AddCraig                   | Performs addition using Craig's RCA.                                                       |
| ControlledAddCraig         | Performs controlled addition using Craig's RCA.                                            |
| AddImm                     | Performs immediate value addition.                                                         |
| MultiControlledAddImm      | Performs controlled immediate value addition.                                              |
| AddGeneral                 | Performs general addition without using auxiliary qubits.                                  |
| MultiControlledAddGeneral  | Performs controlled general addition.                                                      |
| FinalCarryOfAddImm         | Computes the final carry of immediate addition.                                            |
| AddPiecewise               | Performs approximate piecewise addition.                                                   |
| Sub                        | Performs subtraction without using auxiliary qubits (adjoint of Add).                      |
| SubCuccaro                 | Performs subtraction using adjoint of Cuccaro's RCA.                                       |
| SubCraig                   | Performs subtraction using adjoint of Craig's RCA.                                         |
| ControlledSubCraig         | Performs controlled subtraction using adjoint of Craig's RCA.                              |
| SubImm                     | Performs immediate value subtraction (adjoint of AddImm).                                  |
| MultiControlledSubImm      | Performs controlled immediate value subtraction (adjoint of MultiControlledAddImm).        |
| SubGeneral                 | Performs general subtraction without using auxiliary qubits (adjoint of AddGeneral).       |
| MultiControlledSubGeneral  | Performs controlled general subtraction (adjoint of MultiControlledSubGeneral).            |
| MulW                       | Performs multiplication optimized with windowing.                                          |
| ScaledAdd                  | Performs scaled addition.                                                                  |
| ScaledAddW                 | Performs scaled addition optimized with windowing.                                         |
| EqualTo                    | Performs comparison `equal to` (==).                                                       |
| EqualToImm                 | Performs comparison `equal to` (==) with immediate value.                                  |
| LessThan                   | Performs comparison `less than` (<).                                                       |
| LessThanOrEqualTo          | Performs comparison `less than or equal to` (<=).                                          |
| LessThanImm                | Performs comparison `less than` (<) with immediate value.                                  |
| MultiControlledLessThanImm | Performs comparison `less than` (<) with immediate value controlled by qubits.             |
| BiFlip                     | Flips both [0 .. src-1] and [src .. max] sections at the same time.                        |
| MultiControlledBiFlip      | Performs controlled flips of both [0 .. src-1] and [src .. max] sections at the same time. |
| BiFlipImm                  | Flips both [0 .. imm-1] and [imm .. max] sections at the same time.                        |
| MultiControlledBiFlipImm   | Performs controlled flip of both [0 .. imm-1] and [imm .. max] sections at the same time.  |
| PivotFlip                  | Flips [0 .. src-1].                                                                        |
| MultiControlledPivotFlip   | Performs a controlled flip of [0 .. src-1].                                                |
| PivotFlipImm               | Flips [0 .. imm-1].                                                                        |
| MultiControlledBiFlipImm   | Performs a controlled flip of [0 .. imm-1].                                                |

#### modular 演算

* `qret/algorithm/arithmetic/modular.h`

| Name                        | Summary                                                    |
| --------------------------- | ---------------------------------------------------------- |
| ModNeg                      | Performs modular negation.                                 |
| MultiControlledModNeg       | Performs controlled modular negation.                      |
| ModAdd                      | Performs modular addition.                                 |
| MultiControlledModAdd       | Performs controlled modular addition.                      |
| ModAddImm                   | Performs modular immediate addition.                       |
| MultiControlledModAddImm    | Performs controlled modular immediate addition.            |
| ModSub                      | Performs modular subtraction.                              |
| MultiControlledModSub       | Performs controlled modular subtraction.                   |
| ModSubImm                   | Performs modular immediate subtraction.                    |
| MultiControlledModSubImm    | Performs controlled modular immediate subtraction.         |
| ModDoubling                 | Performs modular doubling.                                 |
| MultiControlledModDoubling  | Performs controlled modular doubling.                      |
| ModBiMul                    | Performs modular bi-multiplication.                        |
| MultiControlledModBiMul     | Performs controlled modular bi-multiplication.             |
| ModScaledAdd                | Performs modular scaled addition.                          |
| ModScaledAddW               | Performs modular scaled addition optimized with windowing. |
| MultiControlledModScaledAdd | Performs controlled modular scaled addition.               |
| ModExpW                     | Performs modular exponentiation optimized with windowing.  |

#### 素因数分解演算

* `qret/algorithm/arithmetic/shor.h`

| Name                        | Summary                                                      |
| --------------------------- | ----------------------------------------------------------   |
| PeriodFinding               | Performs period finding of a modular exponentiation function |

### `qret/algorithm/data/`

#### 量子辞書

* `qret/algorithm/data/qdict.h`

| Name             | Summary                                                                   |
| ---------------- | ------------------------------------------------------------------------- |
| InitializeQDict  | Initializes quantum dictionaries (q-dict) without QRAM.                   |
| InjectIntoQDict  | Injects addressed value into quantum dictionaries (q-dict) without QRAM.  |
| ExtractFromQDict | Extracts addressed value from quantum dictionaries (q-dict) without QRAM. |

#### 量子読み取り専用メモリー

* `qret/algorithm/data/qrom.h`

| Name                          | Summary                                                                                |
| ----------------------------- | -------------------------------------------------------------------------------------- |
| UnaryIterBegin                | Begins unary iteration.                                                                |
| UnaryIterStep                 | Steps unary iteration.                                                                 |
| UnaryIterEnd                  | Ends unary iteration.                                                                  |
| MultiControlledUnaryIterBegin | Performs controlled UnaryIterBegin.                                                    |
| MultiControlledUnaryIterStep  | Performs controlled UnaryIterStep.                                                     |
| MultiControlledUnaryIterEnd   | Performs controlled UnaryIterEnd.                                                      |
| QROM                          | Performs Quantum Read-Only Memory (QROM).                                              |
| MultiControlledQROM           | Performs controlled QROM.                                                              |
| QROMImm                       | Performs QROM where the written value is immediate.                                    |
| MultiControlledQROMImm        | Performs controlled QROM where the written value is immediate.                         |
| UncomputeQROM                 | Performs measurement-based uncomputation of QROM.                                      |
| UncomputeQROMImm              | Performs measurement-based uncomputation of QROM where the written value is immediate. |
| QROAMClean                    | Performs advanced QROM (QROAM) assisted by clean ancillae.                             |
| UncomputeQROAMClean           | Performs measurement-based uncomputation of QROAM assisted by clean ancillae.          |
| QROAMDirty                    | Performs QROAM assisted by dirty ancillae.                                             |
| UncomputeQROAMDirty           | Performs measurement-based uncomputation of QROAM assisted by dirty ancillae.          |
| QROMParallel                  | Parallel implementation of QROM.                                                       |

### `qret/algorithm/phase_estimation/`

#### 量子位相推定

* `qret/algorithm/phase_estimation/qpe.h`

| Name             | Summary                    |
| ---------------- | -------------------------- |
| PREPARE          | Performs PREPARE.          |
| SELECT           | Performs SELECT.           |
| PhaseEstimation  | Performs phase estimation. |

#### トロッター展開

* `qret/algorithm/phase_estimation/trotter.h`

| Name             | Summary                                             |
| ---------------- | --------------------------------------------------- |
| PauliRotation    | Performs phase rotation with a Pauli string.        |
| TrotterStep      | Performs a single Trotter step with a Hamiltonian.  |
| Trotter          | Performs full Trotter expansion with a Hamiltonian. |


### `qret/algorithm/preparation/`

* `qret/algorithm/preparation/arbitrary.h`
* `qret/algorithm/preparation/uniform.h`

| Name                                  | Summary                                                                                            | File                                |
| ------------------------------------- | -------------------------------------------------------------------------------------------------- | ----------------------------------- |
| PrepareArbitraryState                 | Prepares an arbitrary state where all coefficients are real numbers.                               | `qret/algorithm/preparation/arbitrary.h` |
| PrepareUniformSuperposition           | Creates a uniform superposition over states that encode 0 through `N` - 1.                         | `qret/algorithm/preparation/uniform.h`   |
| ControlledPrepareUniformSuperposition | Performs controlled creation of a uniform superposition over states that encode 0 through `N` - 1. | `qret/algorithm/preparation/uniform.h`   |

### `qret/algorithm/transform/`

* `qret/algorithm/transform/qft.h`

| Name             | Summary           |
| ---------------- | ----------------- |
| FourierTransform | Performs QFT.     |

### `qret/algorithm/util/`

* `qret/algorithm/util/array.h`
* `qret/algorithm/util/bitwise.h`
* `qret/algorithm/util/multi_control.h`
* `qret/algorithm/util/reset.h`
* `qret/algorithm/util/rotation.h`
* `qret/algorithm/util/util.h`
* `qret/algorithm/util/write_register.h`

| Name                            | Summary                                                                      | File                              |
| ------------------------------- | ---------------------------------------------------------------------------- | --------------------------------- |
| ApplyXToEach                    | Applies Pauli-X to all qubits.                                               | `qret/algorithm/util/array.h`          |
| ApplyXToEachIf                  | Applies Pauli-X to qubits if the corresponding register is 1.                | `qret/algorithm/util/array.h`          |
| ApplyCXToEachIf                 | Applies CNOT to qubits if the corresponding bit is 1.                        | `qret/algorithm/util/array.h`          |
| ApplyXToEachIfImm               | Applies Pauli-X to qubits if the corresponding bit is 1.                     | `qret/algorithm/util/array.h`          |
| ApplyCXToEachIfImm              | Applies CNOT to qubits if the corresponding bit is 1.                        | `qret/algorithm/util/array.h`          |
| MultiControlledBitOrderRotation | Performs controlled shift of the lower `len` qubits to the higher positions. | `qret/algorithm/util/bitwise.h`        |
| MultiControlledBitOrderReversal | Performs controlled reversal of the order of qubits.                         | `qret/algorithm/util/bitwise.h`        |
| MCMX                            | Performs multi-controlled multi-not.                                         | `qret/algorithm/util/multi_control.h`  |
| Reset                           | Resets a qubit.                                                              | `qret/algorithm/util/reset.h`          |
| R1                              | Performs R1 gate.                                                            | `qret/algorithm/util/rotation.h`       |
| ControlledR1                    | Performs controlled R1 gate.                                                 | `qret/algorithm/util/rotation.h`       |
| ControlledRX                    | Performs controlled RX gate.                                                 | `qret/algorithm/util/rotation.h`       |
| ControlledRY                    | Performs controlled RY gate.                                                 | `qret/algorithm/util/rotation.h`       |
| ControlledRZ                    | Performs controlled RZ gate.                                                 | `qret/algorithm/util/rotation.h`       |
| RandomizedMultiplexedRotation   | Performs multiplexed rotation using random.                                  | `qret/algorithm/util/rotation.h`       |
| ControlledH                     | Performs controlled H gate using catalysis `\|T>` .                          | `qret/algorithm/util/util.h`           |
| Swap                            | Performs swap gate using three CNOT.                                         | `qret/algorithm/util/util.h`           |
| ControlledSwap                  | Performs controlled swap gate.                                               | `qret/algorithm/util/util.h`           |
| WriteRegisters                  | Write a value to registers.                                                  | `qret/algorithm/util/write_register.h` |
