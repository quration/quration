#!/usr/bin/env python3
"""Generate QIR (.ll) test data files for the QIR importer and equivalence tests.

Usage (run from the repository root, in an environment with pyqir installed):

    python quration-core/scripts/gen-qir-test-data/gen_qir_test_data.py

Output files are written to:
    quration-core/tests/data/qir/

See README.md in this directory for what is generated and why.
"""

from __future__ import annotations

import sys
from pathlib import Path

import pyqir
from pyqir import (
    BasicBlock,
    Builder,
    Context,
    Function,
    FunctionType,
    IntPredicate,
    IntType,
    Linkage,
    PointerType,
    Type,
    const,
    entry_point,
    global_byte_string,
    qir_module,
    qubit,
    result,
)
from pyqir._native import result_record_output, tuple_record_output

OUT_DIR = Path(__file__).resolve().parents[2] / "tests" / "data" / "qir"


class ModuleBuilder:
    """Thin wrapper over pyqir for building one adaptive-profile QIR module."""

    def __init__(self, name: str, num_qubits: int, num_results: int) -> None:
        self.ctx = Context()
        self.module = qir_module(self.ctx, name)
        self.entry = entry_point(
            self.module,
            "main",
            num_qubits,
            num_results,
            qir_profiles="adaptive_profile",
            output_labeling_schema="schema_id",
        )
        self.builder = Builder(self.ctx)
        self.builder.insert_at_end(BasicBlock(self.ctx, "entry", self.entry))
        self.void = Type.void(self.ctx)
        self.i1 = IntType(self.ctx, 1)
        self.i64 = IntType(self.ctx, 64)
        self.ptr = PointerType(Type.void(self.ctx))
        self._functions: dict[str, Function] = {}

    # --- structure -------------------------------------------------------------------------------
    def block(self, name: str, function: Function | None = None) -> BasicBlock:
        return BasicBlock(self.ctx, name, function or self.entry)

    def at(self, block: BasicBlock) -> None:
        self.builder.insert_at_end(block)

    def declare(self, name: str, ret: Type, args: list[Type]) -> Function:
        if name not in self._functions:
            self._functions[name] = Function(
                FunctionType(ret, args), Linkage.EXTERNAL, name, self.module
            )
        return self._functions[name]

    def define(self, name: str, ret: Type, args: list[Type]) -> Function:
        return Function(FunctionType(ret, args), Linkage.INTERNAL, name, self.module)

    # --- operands --------------------------------------------------------------------------------
    def qubit(self, index: int):
        return qubit(self.ctx, index)

    def result(self, index: int):
        return result(self.ctx, index)

    def label(self, text: str):
        return global_byte_string(self.module, text.encode())

    # --- quantum / runtime calls -----------------------------------------------------------------
    def gate(self, name: str, *qubit_operands) -> None:
        self.builder.call(
            self.declare(name, self.void, [self.ptr] * len(qubit_operands)),
            list(qubit_operands),
        )

    def rotation(self, name: str, angle: float, *qubit_operands) -> None:
        self.builder.call(
            self.declare(
                name,
                self.void,
                [Type.double(self.ctx)] + [self.ptr] * len(qubit_operands),
            ),
            [const(Type.double(self.ctx), angle), *qubit_operands],
        )

    def measure(self, name: str, qubit_index: int, result_index: int) -> None:
        self.builder.call(
            self.declare(name, self.void, [self.ptr, self.ptr]),
            [self.qubit(qubit_index), self.result(result_index)],
        )

    def read_result(self, name: str, result_index: int):
        return self.builder.call(
            self.declare(name, self.i1, [self.ptr]), [self.result(result_index)]
        )

    def record_result(self, result_index: int, label: str) -> None:
        result_record_output(self.builder, self.result(result_index), self.label(label))

    def record_tuple(self, num_elements: int, label: str) -> None:
        tuple_record_output(
            self.builder, const(self.i64, num_elements), self.label(label)
        )

    def record_int(self, value, label: str) -> None:
        self.builder.call(
            self.declare("__quantum__rt__int_record_output", self.void, [self.i64, self.ptr]),
            [value, self.label(label)],
        )

    def ir(self) -> str:
        text = str(self.module)
        pyqir.Module.from_ir(
            Context(), text
        )  # validate the generated module is real QIR
        return text


# --------------------------------------------------------------------------------------------------
# Positive fixtures.
# --------------------------------------------------------------------------------------------------


def build_classical_branch_condition() -> str:
    """A br whose condition is `icmp sgt (zext read_result), 0`, then a tuple of 3 results."""
    m = ModuleBuilder("classical_branch_condition", 3, 3)
    then, els, join = m.block("then"), m.block("else"), m.block("join")

    m.gate("__quantum__qis__x__body", m.qubit(0))
    m.measure("__quantum__qis__mz__body", 0, 0)
    cond = m.read_result("__quantum__rt__read_result", 0)
    widened = m.builder.zext(cond, m.i64)
    taken = m.builder.icmp(IntPredicate.SGT, widened, const(m.i64, 0))
    m.builder.condbr(taken, then, els)

    m.at(then)
    m.gate("__quantum__qis__x__body", m.qubit(1))
    m.builder.br(join)
    m.at(els)
    m.gate("__quantum__qis__x__body", m.qubit(2))
    m.builder.br(join)

    m.at(join)
    m.measure("__quantum__qis__mz__body", 1, 1)
    m.measure("__quantum__qis__mz__body", 2, 2)
    m.record_tuple(3, "t3")
    m.record_result(0, "r0")
    m.record_result(1, "r1")
    m.record_result(2, "r2")
    m.builder.ret(None)
    return m.ir()


def build_classical_int_chain() -> str:
    """Two independent int-op subtrees sharing one measured input, interleaved in QIR source order.
    Pins the importer's classical-synthesis order: a demand-driven (output-rooted) build would group
    each subtree and swap shl/mul and or/sub, so a re-export must keep the QIR source order."""
    m = ModuleBuilder("classical_int_chain", 1, 1)
    m.gate("__quantum__qis__x__body", m.qubit(0))
    m.measure("__quantum__qis__mz__body", 0, 0)
    cond = m.read_result("__quantum__rt__read_result", 0)
    v = m.builder.zext(cond, m.i64)
    a1 = m.builder.add(v, const(m.i64, 1))
    b1 = m.builder.shl(v, const(m.i64, 2))
    a2 = m.builder.mul(a1, const(m.i64, 3))
    b2 = m.builder.or_(b1, const(m.i64, 1))
    a3 = m.builder.sub(a2, const(m.i64, 2))
    b3 = m.builder.xor(b2, const(m.i64, 4))
    m.record_tuple(2, "t")  # QDK groups the two int outputs under a tuple; the importer skips it
    m.record_int(a3, "a")
    m.record_int(b3, "b")
    m.builder.ret(None)
    return m.ir()


def build_reset_preset() -> str:
    m = ModuleBuilder("reset_preset", 1, 1)
    m.gate("__quantum__qis__x__body", m.qubit(0))
    m.gate("__quantum__qis__reset__body", m.qubit(0))
    m.measure("__quantum__qis__mz__body", 0, 0)
    m.builder.ret(None)
    return m.ir()


def build_swap_preset() -> str:
    m = ModuleBuilder("swap_preset", 2, 2)
    m.gate("__quantum__qis__x__body", m.qubit(0))
    m.gate("__quantum__qis__swap__body", m.qubit(0), m.qubit(1))
    m.measure("__quantum__qis__mz__body", 0, 0)
    m.measure("__quantum__qis__mz__body", 1, 1)
    m.builder.ret(None)
    return m.ir()


def _mresetz_then_flip(name: str) -> str:
    """Shared shape for the mresetz / pyqir-style read_result fixtures."""
    m = ModuleBuilder(name, 2, 3 if name == "qsc_mresetz" else 2)
    then, cont = m.block("then"), m.block("cont")
    m.gate("__quantum__qis__x__body", m.qubit(0))
    if name == "qsc_mresetz":
        m.measure("__quantum__qis__mresetz__body", 0, 0)
    else:
        m.measure("__quantum__qis__mz__body", 0, 0)
    cond = m.read_result("__quantum__qis__read_result__body", 0)
    m.builder.condbr(cond, then, cont)

    m.at(then)
    m.gate("__quantum__qis__x__body", m.qubit(1))
    m.builder.br(cont)

    m.at(cont)
    if name == "qsc_mresetz":
        m.measure("__quantum__qis__mz__body", 0, 1)
        m.measure("__quantum__qis__mz__body", 1, 2)
    else:
        m.measure("__quantum__qis__mz__body", 1, 1)
    m.builder.ret(None)
    return m.ir()


def build_qsc_mresetz() -> str:
    return _mresetz_then_flip("qsc_mresetz")


def build_pyqir_style_read_result() -> str:
    return _mresetz_then_flip("pyqir_style_read_result")


def build_rzz_rxx_ryy() -> str:
    m = ModuleBuilder("rzz_rxx_ryy", 2, 0)
    m.rotation("__quantum__qis__rzz__body", 1.0, m.qubit(0), m.qubit(1))
    m.rotation("__quantum__qis__rxx__body", 2.0, m.qubit(0), m.qubit(1))
    m.rotation("__quantum__qis__ryy__body", 3.0, m.qubit(0), m.qubit(1))
    m.builder.ret(None)
    return m.ir()


def build_clifford_gates() -> str:
    """Every single-qubit Clifford/named gate the importer maps to a distinct IR opcode."""
    m = ModuleBuilder("clifford_gates", 1, 1)
    for name in (
        "__quantum__qis__id__body",
        "__quantum__qis__x__body",
        "__quantum__qis__y__body",
        "__quantum__qis__z__body",
        "__quantum__qis__h__body",
        "__quantum__qis__s__body",
        "__quantum__qis__s__adj",
        "__quantum__qis__t__body",
        "__quantum__qis__t__adj",
    ):
        m.gate(name, m.qubit(0))
    m.measure("__quantum__qis__mz__body", 0, 0)
    m.builder.ret(None)
    return m.ir()


def build_rotations_and_sx() -> str:
    """Single-qubit rotations import with their angle preserved; sx decomposes to RX(pi/2)+phase."""
    m = ModuleBuilder("rotations_and_sx", 1, 1)
    m.rotation("__quantum__qis__rx__body", 0.5, m.qubit(0))
    m.rotation("__quantum__qis__ry__body", 1.25, m.qubit(0))
    m.rotation("__quantum__qis__rz__body", 2.0, m.qubit(0))
    m.gate("__quantum__qis__sx__body", m.qubit(0))
    m.measure("__quantum__qis__mz__body", 0, 0)
    m.builder.ret(None)
    return m.ir()


def build_record_multi_qubit_gates() -> str:
    """Bit-flip multi-qubit gates (X/Y/CX/CY/CCX) with recorded results, for equivalence checking."""
    m = ModuleBuilder("record_multi_qubit_gates", 5, 5)
    m.gate("__quantum__qis__x__body", m.qubit(0))  # q0 = 1
    m.gate("__quantum__qis__y__body", m.qubit(1))  # q1 = 1
    m.gate("__quantum__qis__cnot__body", m.qubit(0), m.qubit(2))  # control q0 -> q2 = 1
    m.gate("__quantum__qis__cy__body", m.qubit(0), m.qubit(3))  # control q0 -> q3 = 1
    m.builder.call(
        m.declare("__quantum__qis__ccx__body", m.void, [m.ptr, m.ptr, m.ptr]),
        [m.qubit(0), m.qubit(1), m.qubit(4)],  # controls q0,q1 -> q4 = 1
    )
    for i in range(5):
        m.measure("__quantum__qis__mz__body", i, i)
    m.record_tuple(5, "t5")
    for i in range(5):
        m.record_result(i, f"r{i}")
    m.builder.ret(None)
    return m.ir()


def build_barrier() -> str:
    """barrier is a scheduling hint with no state effect: the importer drops it."""
    m = ModuleBuilder("barrier", 1, 1)
    m.gate("__quantum__qis__x__body", m.qubit(0))
    m.builder.call(m.declare("__quantum__qis__barrier__body", m.void, []), [])
    m.measure("__quantum__qis__mz__body", 0, 0)
    m.builder.ret(None)
    return m.ir()


def build_qsc_alias_gate_names() -> str:
    """Uses the `cx`/`m` gate-name aliases the importer accepts as cnot/mz."""
    m = ModuleBuilder("qsc_alias_gate_names", 2, 2)
    m.gate("__quantum__qis__x__body", m.qubit(0))
    m.gate("__quantum__qis__cx__body", m.qubit(0), m.qubit(1))
    m.measure("__quantum__qis__m__body", 0, 0)
    m.measure("__quantum__qis__m__body", 1, 1)
    m.builder.ret(None)
    return m.ir()


def build_pyqir_multi_function() -> str:
    """The entry point calls a user-defined function (valid QIR; both QDK and qir-runner run it)."""
    m = ModuleBuilder("pyqir_multi_function", 1, 1)
    flip = m.define("flip_and_measure", m.void, [m.ptr, m.ptr])
    m.builder.call(flip, [m.qubit(0), m.result(0)])
    m.builder.ret(None)

    m.at(BasicBlock(m.ctx, "entry", flip))
    param_qubit, param_result = flip.params
    m.builder.call(m.declare("__quantum__qis__x__body", m.void, [m.ptr]), [param_qubit])
    m.builder.call(
        m.declare("__quantum__qis__mz__body", m.void, [m.ptr, m.ptr]),
        [param_qubit, param_result],
    )
    m.builder.ret(None)
    return m.ir()


# --------------------------------------------------------------------------------------------------
# Negative fixtures: complete except for the one construct the importer is expected to reject.
# --------------------------------------------------------------------------------------------------


def build_backward_branch() -> str:
    """An unbounded loop with a back-edge: rejected for the backward branch."""
    m = ModuleBuilder("backward_branch", 1, 0)
    loop = m.block("loop")
    m.builder.br(loop)
    m.at(loop)
    m.gate("__quantum__qis__x__body", m.qubit(0))
    m.builder.br(loop)
    return m.ir()


def build_cyclic_sub_circuits() -> str:
    """sub_a -> sub_b -> sub_a: rejected for the recursion cycle."""
    m = ModuleBuilder("cyclic_sub_circuits", 1, 0)
    sub_a = m.define("sub_a", m.void, [m.ptr])
    sub_b = m.define("sub_b", m.void, [m.ptr])
    m.builder.call(sub_a, [m.qubit(0)])
    m.builder.ret(None)

    m.at(BasicBlock(m.ctx, "entry", sub_a))
    m.builder.call(sub_b, [sub_a.params[0]])
    m.builder.ret(None)

    m.at(BasicBlock(m.ctx, "entry", sub_b))
    m.builder.call(sub_a, [sub_b.params[0]])
    m.builder.ret(None)
    return m.ir()


def build_duplicate_record_output() -> str:
    """The same result recorded twice: rejected for the duplicate record."""
    m = ModuleBuilder("duplicate_record_output", 1, 1)
    m.measure("__quantum__qis__mz__body", 0, 0)
    m.record_tuple(2, "t2")
    m.record_result(0, "r0")
    m.record_result(0, "r1")
    m.builder.ret(None)
    return m.ir()


def build_unknown_function_call() -> str:
    """A call to a runtime function the importer does not support: rejected for the unknown call."""
    m = ModuleBuilder("unknown_function_call", 1, 0)
    m.builder.call(
        m.declare("__quantum__rt__qubit_allocate", m.void, [m.ptr]), [m.qubit(0)]
    )
    m.builder.ret(None)
    return m.ir()


def build_unsupported_qis_gate() -> str:
    """A qis gate name outside the intrinsic catalog: rejected as an unknown gate."""
    m = ModuleBuilder("unsupported_qis_gate", 1, 0)
    m.gate("__quantum__qis__unknown__body", m.qubit(0))
    m.builder.ret(None)
    return m.ir()


def build_empty_circuit() -> str:
    """A valid entry point with no gates: imports to an empty circuit."""
    m = ModuleBuilder("empty_circuit", 1, 0)
    m.builder.ret(None)
    return m.ir()


FIXTURES = {
    "classical_branch_condition": build_classical_branch_condition,
    "classical_int_chain": build_classical_int_chain,
    "reset_preset": build_reset_preset,
    "swap_preset": build_swap_preset,
    "qsc_mresetz": build_qsc_mresetz,
    "pyqir_style_read_result": build_pyqir_style_read_result,
    "rzz_rxx_ryy": build_rzz_rxx_ryy,
    "clifford_gates": build_clifford_gates,
    "rotations_and_sx": build_rotations_and_sx,
    "record_multi_qubit_gates": build_record_multi_qubit_gates,
    "barrier": build_barrier,
    "qsc_alias_gate_names": build_qsc_alias_gate_names,
    "pyqir_multi_function": build_pyqir_multi_function,
    "backward_branch": build_backward_branch,
    "cyclic_sub_circuits": build_cyclic_sub_circuits,
    "duplicate_record_output": build_duplicate_record_output,
    "unknown_function_call": build_unknown_function_call,
    "unsupported_qis_gate": build_unsupported_qis_gate,
    "empty_circuit": build_empty_circuit,
}


def main() -> int:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for name, builder in FIXTURES.items():
        try:
            text = builder()
        except Exception as error:  # noqa: BLE001 - report which fixture failed to build/validate
            print(f"error: {name}: {error}", file=sys.stderr)
            return 1
        (OUT_DIR / f"{name}.ll").write_text(text)
        print(f"wrote {name}.ll")
    print(f"generated {len(FIXTURES)} fixtures into {OUT_DIR}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
