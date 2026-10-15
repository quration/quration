\page qret QRET

## Overview

The QRET library consists of the following modules.
Documentation for each module is located in the corresponding directory's `README.md`.

* `analysis/`
  * Implements tools for analyzing the intermediate representation.
* `base/`
  * Implements basic classes and functions used widely across all modules.
* `cmd/`
  * Implements commands used from the command line.
  * Built only when `QRET_BUILD_APPLICATION` is `ON` during CMake configuration.
* `codegen/`
  * Defines classes shared by target quantum computers.
  * Target-specific implementations are placed under `target/`.
* `exporter/`
  * Exports the intermediate representation to external file formats (e.g. QIR).
* `frontend/`
  * Provides an intuitive way to define quantum circuits in C++.
* `gate/`
  * Implements various quantum circuits.
* `ir/`
  * Defines the intermediate representation for quantum circuits.
* `math/`
  * Implements mathematical functions.
* `parser/`
  * Implements parsers for various files.
* `runtime/`
  * Provides quantum circuit simulators.
* `target/`
  * Implements instructions and optimization passes for each target.
    * Currently, only sc_ls_fixed_v0 is implemented.
* `transforms/`
  * Implements optimization passes for quantum circuits described in the intermediate representation.

### Module Dependency Graph

\dot
digraph G {
  rankdir=LR;
  node [shape=box];

  BA [label="base"];
  MA [label="math"];
  IR [label="ir"];
  PA [label="parser"];
  EX [label="exporter"];
  FR [label="frontend"];
  GA [label="gate"];
  RU [label="runtime"];
  AN [label="analysis"];
  TR [label="transforms"];
  CO [label="codegen"];
  TA [label="target"];

  BA -> MA;
  BA -> IR;
  MA -> IR;
  BA -> PA;
  IR -> FR;
  PA -> FR;
  IR -> EX;
  PA -> EX;
  FR -> GA;
  MA -> GA;
  IR -> RU;
  MA -> RU;
  IR -> AN;
  IR -> TR;
  BA -> CO;
  CO -> TA;
  IR -> TA;
  MA -> TA;
}
\enddot
