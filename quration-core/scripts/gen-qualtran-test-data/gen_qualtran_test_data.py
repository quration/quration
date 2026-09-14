#!/usr/bin/env python3
"""Generate Qualtran BloqLibrary JSON test data files for the Qualtran frontend tests.

Usage (run from the repository root, in an environment with qualtran installed):

    python quration-core/scripts/gen-qualtran-test-data/gen_qualtran_test_data.py

Output files are written to:
    quration-core/tests/data/qualtran/
"""

from __future__ import annotations

from pathlib import Path

import attrs
import numpy as np
from qualtran import (
    Adjoint,
    Bloq,
    BloqBuilder,
    Controlled,
    CtrlSpec,
    QAny,
    QBit,
    QUInt,
    Register,
    Side,
    Signature,
)
from qualtran.bloqs.basic_gates import (
    CNOT,
    CZ,
    CYGate,
    GlobalPhase,
    Hadamard,
    Identity,
    IntEffect,
    IntState,
    MinusEffect,
    MinusState,
    OneEffect,
    OneState,
    PlusEffect,
    PlusState,
    SGate,
    TGate,
    Toffoli,
    XGate,
    YGate,
    ZeroEffect,
    ZeroState,
    ZGate,
)
from qualtran.bloqs.basic_gates.hadamard import CHadamard
from qualtran.bloqs.basic_gates.rotation import CRz, CZPowGate, Rx, Ry, Rz, XPowGate, YPowGate, ZPowGate
from qualtran.bloqs.basic_gates.su2_rotation import SU2RotationGate
from qualtran.bloqs.basic_gates.swap import CSwap, Swap, TwoBitCSwap, TwoBitSwap
from qualtran.bloqs.bookkeeping.arbitrary_clifford import ArbitraryClifford
from qualtran.bloqs.bookkeeping.cast import Cast
from qualtran.bloqs.bookkeeping.partition import Join2, Partition, Split2
from qualtran.bloqs.bookkeeping.split import Split
from qualtran.bloqs.mcmt.and_bloq import And
from qualtran.serialization import bloq as bloq_to_proto

OUT_DIR = Path(__file__).parent.parent.parent / "tests" / "data" / "qualtran"


def _to_json(bloq_library) -> str:
    from google.protobuf import json_format

    return json_format.MessageToJson(bloq_library)


def _save(name: str, bloq_library) -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    path = OUT_DIR / f"{name}.json"
    path.write_text(_to_json(bloq_library), encoding="utf-8")
    print(f"  wrote {path}")


@attrs.frozen
class MyGate(Bloq):
    @property
    def signature(self) -> Signature:
        return Signature.build(q=1)

    def build_composite_bloq(self, bb: BloqBuilder, q):
        (q,) = bb.add_t(XGate(), q=q)
        (q,) = bb.add_t(Hadamard(), q=q)
        return {"q": q}


@attrs.frozen
class AndWithGatesGate(Bloq):
    @property
    def signature(self) -> Signature:
        return Signature([Register("ctrl", QBit(), shape=(2,))])

    def build_composite_bloq(self, bb: BloqBuilder, ctrl):
        ctrl, target = bb.add_t(And(), ctrl=ctrl)
        (ctrl[0],) = bb.add_t(ZGate(), q=ctrl[0])
        (ctrl,) = bb.add_t(And(uncompute=True), ctrl=ctrl, target=target)
        return {"ctrl": ctrl}


# Parameterized so two differently-angled instances share bloq.name() but are distinct bloqs.
@attrs.frozen
class MyRzGate(Bloq):
    angle: float

    @property
    def signature(self) -> Signature:
        return Signature.build(q=1)

    def build_composite_bloq(self, bb: BloqBuilder, q):
        (q,) = bb.add_t(Rz(angle=self.angle, eps=1e-8), q=q)
        return {"q": q}


@attrs.frozen
class MyGateWithCZPow(Bloq):
    exponent: float

    @property
    def signature(self) -> Signature:
        return Signature([Register("q", QBit(), shape=(2,))])

    def build_composite_bloq(self, bb: BloqBuilder, q):
        (q,) = bb.add_t(CZPowGate(exponent=self.exponent, eps=1e-8), q=q)
        return {"q": q}


@attrs.frozen
class InnerNamedGate(Bloq):
    @property
    def signature(self) -> Signature:
        return Signature.build(q=1)

    def build_composite_bloq(self, bb: BloqBuilder, q):
        (q,) = bb.add_t(TGate(), q=q)
        (q,) = bb.add_t(SGate(), q=q)
        return {"q": q}


@attrs.frozen
class OuterNamedGate(Bloq):
    @property
    def signature(self) -> Signature:
        return Signature.build(p=1, q=1)

    def build_composite_bloq(self, bb: BloqBuilder, p, q):
        (p,) = bb.add_t(XGate(), q=p)
        (q,) = bb.add_t(InnerNamedGate(), q=q)
        return {"p": p, "q": q}


@attrs.frozen
class AllocatingSubBloq(Bloq):
    @property
    def signature(self) -> Signature:
        return Signature([Register("anc", QAny(2), side=Side.RIGHT)])

    def build_composite_bloq(self, bb: BloqBuilder):
        anc = bb.allocate(2)
        return {"anc": anc}


def gen_direct_gates() -> None:
    bb = BloqBuilder()
    a = bb.add_register("a", QBit())
    b = bb.add_register("b", QBit())
    c = bb.add_register("c", QBit())
    d = bb.add_register("d", QBit())
    tc = bb.add_register(Register("tc", QBit(), shape=(2,)))
    e = bb.add_register("e", QBit())

    (a,) = bb.add_t(XGate(), q=a)
    (c,) = bb.add_t(YGate(), q=c)
    (d,) = bb.add_t(ZGate(), q=d)
    a, b = bb.add_t(CNOT(), ctrl=a, target=b)
    (b,) = bb.add_t(Hadamard(), q=b)
    b, c = bb.add_t(CZ(), q1=b, q2=c)
    c, d = bb.add_t(CYGate(), ctrl=c, target=d)
    (d,) = bb.add_t(SGate(), q=d)
    (d,) = bb.add_t(TGate(), q=d)
    (d,) = bb.add_t(SGate(is_adjoint=True), q=d)
    (d,) = bb.add_t(TGate(is_adjoint=True), q=d)
    (d,) = bb.add_t(Identity(), q=d)
    tc, e = bb.add_t(Toffoli(), ctrl=tc, target=e)

    cbloq = bb.finalize(a=a, b=b, c=c, d=d, tc=tc, e=e)
    lib = bloq_to_proto.bloqs_to_proto(cbloq, name="direct_gates_test")
    _save("direct_gates", lib)


def gen_parametrized_gates() -> None:
    bb = BloqBuilder()
    qx = bb.add_register("qx", QBit())
    qy = bb.add_register("qy", QBit())
    qz = bb.add_register("qz", QBit())
    qxp = bb.add_register("qxp", QBit())
    qyp = bb.add_register("qyp", QBit())
    qzp = bb.add_register("qzp", QBit())
    crz_ctrl = bb.add_register("crz_ctrl", QBit())
    crz_q = bb.add_register("crz_q", QBit())
    su2_q = bb.add_register("su2_q", QBit())

    angle = np.pi / 4
    eps = 1e-8

    (qx,) = bb.add_t(Rx(angle=angle, eps=eps), q=qx)
    (qy,) = bb.add_t(Ry(angle=angle, eps=eps), q=qy)
    (qz,) = bb.add_t(Rz(angle=angle, eps=eps), q=qz)
    (qxp,) = bb.add_t(XPowGate(exponent=0.5, eps=eps), q=qxp)
    (qyp,) = bb.add_t(YPowGate(exponent=0.5, eps=eps), q=qyp)
    (qzp,) = bb.add_t(ZPowGate(exponent=0.5, eps=eps), q=qzp)
    crz_ctrl, crz_q = bb.add_t(CRz(angle=0.3, eps=eps), ctrl=crz_ctrl, q=crz_q)
    (su2_q,) = bb.add_t(
        SU2RotationGate(theta=0.1, phi=0.2, lambd=0.3, global_shift=0.4, eps=eps), q=su2_q
    )

    cbloq = bb.finalize(
        qx=qx,
        qy=qy,
        qz=qz,
        qxp=qxp,
        qyp=qyp,
        qzp=qzp,
        crz_ctrl=crz_ctrl,
        crz_q=crz_q,
        su2_q=su2_q,
    )
    lib = bloq_to_proto.bloqs_to_proto(cbloq, name="parametrized_gates_test")
    _save("parametrized_gates", lib)


def gen_preset_gate_calls() -> None:
    bb = BloqBuilder()
    ctrl = bb.add_register("ctrl", QBit())
    tgt = bb.add_register("tgt", QBit())
    x = bb.add_register("x", QBit())
    y = bb.add_register("y", QBit())
    cswap_ctrl = bb.add_register("cswap_ctrl", QBit())
    cx = bb.add_register("cx", QBit())
    cy = bb.add_register("cy", QBit())

    ctrl, tgt = bb.add_t(CHadamard(), ctrl=ctrl, target=tgt)
    x, y = bb.add_t(TwoBitSwap(), x=x, y=y)
    cswap_ctrl, cx, cy = bb.add_t(TwoBitCSwap(), ctrl=cswap_ctrl, x=cx, y=cy)

    cbloq = bb.finalize(ctrl=ctrl, tgt=tgt, x=x, y=y, cswap_ctrl=cswap_ctrl, cx=cx, cy=cy)
    lib = bloq_to_proto.bloqs_to_proto(cbloq, name="preset_gate_calls_test")
    _save("preset_gate_calls", lib)


def gen_and_gates() -> None:
    bb = BloqBuilder()
    ctrl = bb.add_register(Register("ctrl", QBit(), shape=(2,)))
    ctrl_cv0 = bb.add_register(Register("ctrl_cv0", QBit(), shape=(2,)))

    ctrl, target = bb.add_t(And(), ctrl=ctrl)
    ctrl = bb.add_t(And(uncompute=True), ctrl=ctrl, target=target)[0]
    ctrl_cv0, target_cv0 = bb.add_t(And(cv1=0, cv2=1), ctrl=ctrl_cv0)

    cbloq = bb.finalize(ctrl=ctrl, ctrl_cv0=ctrl_cv0, target_cv0=target_cv0)
    lib = bloq_to_proto.bloqs_to_proto(cbloq, name="and_gates_test", max_depth=2)
    _save("and_gates", lib)


def gen_czpow_multiple() -> None:
    bb = BloqBuilder()
    q1 = bb.add_register(Register("q1", QBit(), shape=(2,)))
    q2 = bb.add_register(Register("q2", QBit(), shape=(2,)))
    (q1,) = bb.add_t(CZPowGate(exponent=0.5, eps=1e-8), q=q1)
    (q2,) = bb.add_t(CZPowGate(exponent=0.25, eps=1e-8), q=q2)
    cbloq = bb.finalize(q1=q1, q2=q2)
    lib = bloq_to_proto.bloqs_to_proto(cbloq, name="czpow_multiple_test", max_depth=1)
    _save("czpow_multiple", lib)


def _build_swap_cswap_composite():
    bb = BloqBuilder()
    ctrl = bb.add_register("ctrl", QBit())
    x = bb.add_register(Register("x", QUInt(3)))
    y = bb.add_register(Register("y", QUInt(3)))
    x, y = bb.add_t(Swap(bitsize=3), x=x, y=y)
    ctrl, x, y = bb.add_t(CSwap(bitsize=3), ctrl=ctrl, x=x, y=y)
    return bb.finalize(ctrl=ctrl, x=x, y=y)


# Same composite serialized at two depths; both must produce the same IR.
def gen_swap_cswap_decomposed() -> None:
    cbloq = _build_swap_cswap_composite()
    lib = bloq_to_proto.bloqs_to_proto(cbloq, name="swap_cswap_decomposed_test", max_depth=3)
    _save("swap_cswap_decomposed", lib)


def gen_swap_cswap_leaf() -> None:
    cbloq = _build_swap_cswap_composite()
    lib = bloq_to_proto.bloqs_to_proto(cbloq, name="swap_cswap_leaf_test", max_depth=1)
    _save("swap_cswap_leaf", lib)


def gen_state_basis() -> None:
    bb = BloqBuilder()
    zero = bb.add(ZeroState())
    one = bb.add(OneState())
    plus = bb.add(PlusState())
    minus = bb.add(MinusState())
    cbloq = bb.finalize(zero=zero, one=one, plus=plus, minus=minus)
    lib = bloq_to_proto.bloqs_to_proto(cbloq, name="state_basis_test")
    _save("state_basis", lib)


def gen_producer_boundary() -> None:
    bb = BloqBuilder()
    x_thru = bb.add_register("x_thru", 1)
    anc_clean = bb.allocate(1)
    (anc_clean,) = bb.add_t(XGate(), q=anc_clean)
    bb.free(anc_clean)
    anc_dirty = bb.allocate(1, dirty=True)
    (anc_dirty,) = bb.add_t(XGate(), q=anc_dirty)
    bb.free(anc_dirty, dirty=True)
    anc_escape = bb.allocate(2)
    sub_escape = bb.add(AllocatingSubBloq())
    cbloq = bb.finalize(x_thru=x_thru, anc_escape=anc_escape, sub_escape=sub_escape)
    lib = bloq_to_proto.bloqs_to_proto(cbloq, name="producer_boundary_test", max_depth=2)
    _save("producer_boundary", lib)


def gen_int_state_effect() -> None:
    bb = BloqBuilder()
    val = bb.add(IntState(val=5, bitsize=3))
    bb.add(IntEffect(val=5, bitsize=3), val=val)
    cbloq = bb.finalize()
    lib = bloq_to_proto.bloqs_to_proto(cbloq, name="int_state_effect_test", max_depth=2)
    _save("int_state_effect", lib)


def gen_consumer_variants() -> None:
    bb = BloqBuilder()
    zero = bb.allocate(1)
    bb.add(ZeroEffect(), q=zero)
    one = bb.allocate(1)
    (one,) = bb.add_t(XGate(), q=one)  # |0> -> |1>
    bb.add(OneEffect(), q=one)
    plus = bb.allocate(1)
    (plus,) = bb.add_t(Hadamard(), q=plus)  # |0> -> |+>
    bb.add(PlusEffect(), q=plus)
    minus = bb.allocate(1)
    (minus,) = bb.add_t(XGate(), q=minus)  # |0> -> |1> -> |->
    (minus,) = bb.add_t(Hadamard(), q=minus)
    bb.add(MinusEffect(), q=minus)
    cbloq = bb.finalize()
    lib = bloq_to_proto.bloqs_to_proto(cbloq, name="consumer_variants_test")
    _save("consumer_variants", lib)


def gen_alias_variants() -> None:
    bb = BloqBuilder()
    tag = bb.add_register("tag", QBit())
    xs = bb.add_register(Register("xs", QAny(4)))
    p = bb.add_register("p", QBit())
    q_cast = bb.add_register(Register("q_cast", QUInt(3)))
    xp = bb.add_register(Register("xp", QAny(4)))

    (tag,) = bb.add_t(XGate(), q=tag)
    y1, y2 = bb.add_t(Split2(2, 2), x=xs)
    y1_bits = bb.split(y1)
    (y1_bits[0],) = bb.add_t(XGate(), q=y1_bits[0])
    y1 = bb.join(y1_bits)
    (xs_out,) = bb.add_t(Join2(2, 2), y1=y1, y2=y2)

    (p,) = bb.add_t(XGate(), q=p)
    q_cast = bb.add(Cast(inp_dtype=QUInt(3), out_dtype=QAny(3)), reg=q_cast)

    regs = (Register("pa", QAny(2)), Register("pb", QAny(2)))
    pa, pb = bb.add_t(Partition(4, regs, partition=True), x=xp)
    (xp_out,) = bb.add_t(Partition(4, regs, partition=False), pa=pa, pb=pb)

    cbloq = bb.finalize(tag=tag, xs=xs_out, p=p, q_cast=q_cast, xp=xp_out)
    lib = bloq_to_proto.bloqs_to_proto(cbloq, name="alias_variants_test")
    _save("alias_variants", lib)


def gen_subcircuit_variants() -> None:
    inner_bb = BloqBuilder()
    q_inner = inner_bb.add_register("q", QBit())
    (q_inner,) = inner_bb.add_t(TGate(), q=q_inner)
    (q_inner,) = inner_bb.add_t(SGate(), q=q_inner)
    inner_cbloq = inner_bb.finalize(q=q_inner)

    middle_bb = BloqBuilder()
    a_mid = middle_bb.add_register("a", QBit())
    b_mid = middle_bb.add_register("b", QBit())
    (a_mid,) = middle_bb.add_t(Hadamard(), q=a_mid)
    (b_mid,) = middle_bb.add_t(inner_cbloq, q=b_mid)
    middle_cbloq = middle_bb.finalize(a=a_mid, b=b_mid)

    bb = BloqBuilder()
    p1 = bb.add_register("p1", QBit())
    q1 = bb.add_register("q1", QBit())
    p2 = bb.add_register("p2", QBit())
    q2 = bb.add_register("q2", QBit())
    nested_a = bb.add_register("nested_a", QBit())
    nested_b = bb.add_register("nested_b", QBit())
    sub_a = bb.add_register(Register("sub_a", QBit(), shape=(2,)))
    sub_b = bb.add_register(Register("sub_b", QBit(), shape=(2,)))

    p1, q1 = bb.add_t(OuterNamedGate(), p=p1, q=q1)
    p2, q2 = bb.add_t(OuterNamedGate(), p=p2, q=q2)
    nested_a, nested_b = bb.add_t(middle_cbloq, a=nested_a, b=nested_b)
    (sub_a,) = bb.add_t(MyGateWithCZPow(exponent=0.5), q=sub_a)
    (sub_b,) = bb.add_t(MyGateWithCZPow(exponent=0.25), q=sub_b)

    cbloq = bb.finalize(
        p1=p1,
        q1=q1,
        p2=p2,
        q2=q2,
        nested_a=nested_a,
        nested_b=nested_b,
        sub_a=sub_a,
        sub_b=sub_b,
    )
    lib = bloq_to_proto.bloqs_to_proto(cbloq, name="subcircuit_variants_test", max_depth=3)
    _save("subcircuit_variants", lib)


def gen_adjoint_variants() -> None:
    bb = BloqBuilder()
    q_custom = bb.add_register("q_custom", QBit())
    ctrl_ch = bb.add_register("ctrl_ch", QBit())
    target_ch = bb.add_register("target_ch", QBit())
    ctrl_crz = bb.add_register("ctrl_crz", QBit())
    q_crz = bb.add_register("q_crz", QBit())
    q_rz_a = bb.add_register("q_rz_a", QBit())
    q_rz_b = bb.add_register("q_rz_b", QBit())
    q_su2 = bb.add_register("q_su2", QBit())
    q_rz_sign = bb.add_register("q_rz_sign", QBit())
    ctrl_even = bb.add_register(Register("ctrl_even", QBit(), shape=(2,)))
    ctrl_odd = bb.add_register(Register("ctrl_odd", QBit(), shape=(2,)))
    x_split = bb.add_register(Register("x_split", QAny(2)))
    ctrl_and_gates = bb.add_register(Register("ctrl_and_gates", QBit(), shape=(2,)))
    q_state = bb.add_register(Register("q_state", QBit(), side=Side.LEFT))

    (q_custom,) = bb.add_t(Adjoint(MyGate()), q=q_custom)
    ctrl_ch, target_ch = bb.add_t(Adjoint(CHadamard()), ctrl=ctrl_ch, target=target_ch)
    ctrl_crz, q_crz = bb.add_t(Adjoint(CRz(angle=0.3, eps=1e-8)), ctrl=ctrl_crz, q=q_crz)
    (q_rz_a,) = bb.add_t(Adjoint(MyRzGate(angle=0.3)), q=q_rz_a)
    (q_rz_b,) = bb.add_t(Adjoint(MyRzGate(angle=0.7)), q=q_rz_b)
    # Same params as parametrized_gates' non-adjoint SU2RotationGate, for direct comparison.
    (q_su2,) = bb.add_t(
        Adjoint(SU2RotationGate(theta=0.1, phi=0.2, lambd=0.3, global_shift=0.4, eps=1e-8)),
        q=q_su2,
    )
    (q_rz_sign,) = bb.add_t(Adjoint(Rz(angle=np.pi / 4, eps=1e-8)), q=q_rz_sign)

    ctrl_even, target_even = bb.add_t(Adjoint(Adjoint(And())), ctrl=ctrl_even)
    (ctrl_even,) = bb.add_t(And(uncompute=True), ctrl=ctrl_even, target=target_even)

    ctrl_odd, target_odd = bb.add_t(And(), ctrl=ctrl_odd)
    (ctrl_odd,) = bb.add_t(Adjoint(Adjoint(Adjoint(And()))), ctrl=ctrl_odd, target=target_odd)

    bits = bb.split(x_split)
    (merged,) = bb.add_t(Adjoint(Split(QAny(2))), reg=bits)

    (ctrl_and_gates,) = bb.add_t(Adjoint(AndWithGatesGate()), ctrl=ctrl_and_gates)

    bb.add(Adjoint(ZeroState()), q=q_state)
    q_effect = bb.add(Adjoint(ZeroEffect()))

    cbloq = bb.finalize(
        q_custom=q_custom,
        ctrl_ch=ctrl_ch,
        target_ch=target_ch,
        ctrl_crz=ctrl_crz,
        q_crz=q_crz,
        q_rz_a=q_rz_a,
        q_rz_b=q_rz_b,
        q_su2=q_su2,
        q_rz_sign=q_rz_sign,
        ctrl_even=ctrl_even,
        ctrl_odd=ctrl_odd,
        x_split=merged,
        ctrl_and_gates=ctrl_and_gates,
        q_effect=q_effect,
    )
    lib = bloq_to_proto.bloqs_to_proto(
        cbloq,
        MyGate(),
        MyRzGate(angle=0.3),
        MyRzGate(angle=0.7),
        AndWithGatesGate(),
        name="adjoint_variants_test",
        max_depth=1,
    )
    _save("adjoint_variants", lib)


# max_depth=2 (vs. adjoint_variants' 1) forces Qualtran to expand the Adjoint wrapper itself.
def gen_adjoint_decomposed() -> None:
    bb = BloqBuilder()
    q = bb.add_register("q", QBit())
    (q,) = bb.add_t(Adjoint(MyGate()), q=q)
    cbloq = bb.finalize(q=q)
    lib = bloq_to_proto.bloqs_to_proto(
        cbloq, MyGate(), name="adjoint_decomposed_test", max_depth=2
    )
    _save("adjoint_decomposed", lib)


def gen_controlled_variants() -> None:
    bb = BloqBuilder()
    # Single-control, single-target variants share one (ctrl, q) pair (cf. gen_direct_gates).
    ctrl = bb.add_register("ctrl", QBit())
    q = bb.add_register("q", QBit())
    ctrl2_cnot = bb.add_register("ctrl2_cnot", QBit())
    ctrl_cnot = bb.add_register("ctrl_cnot", QBit())
    target_cnot = bb.add_register("target_cnot", QBit())
    ctrl_multi = bb.add_register(Register("ctrl_multi", QBit(), shape=(3,)))
    q_multi = bb.add_register("q_multi", QBit())
    ctrl2_tof = bb.add_register("ctrl2_tof", QBit())
    ctrl_tof = bb.add_register(Register("ctrl_tof", QBit(), shape=(2,)))
    target_tof = bb.add_register("target_tof", QBit())
    ctrl2_nest = bb.add_register("ctrl2_nest", QBit())
    ctrl_nest = bb.add_register("ctrl_nest", QBit())
    q_nest = bb.add_register("q_nest", QBit())

    ctrl, q = bb.add_t(Controlled(XGate(), ctrl_spec=CtrlSpec()), ctrl=ctrl, q=q)
    ctrl, q = bb.add_t(Controlled(Hadamard(), ctrl_spec=CtrlSpec()), ctrl=ctrl, q=q)
    ctrl, q = bb.add_t(Controlled(TGate(), ctrl_spec=CtrlSpec()), ctrl=ctrl, q=q)
    ctrl, q = bb.add_t(Controlled(TGate(is_adjoint=True), ctrl_spec=CtrlSpec()), ctrl=ctrl, q=q)
    # Controlled(Adjoint(subbloq)): the wrapper form (vs. TGate's own is_adjoint arg above),
    # dispatched via EmitAdjointedControlledSubbloqGate's throwaway-Circuit + ReplaceWithAdjoint.
    ctrl, q = bb.add_t(Controlled(Adjoint(TGate()), ctrl_spec=CtrlSpec()), ctrl=ctrl, q=q)
    ctrl, q = bb.add_t(
        Controlled(XPowGate(exponent=0.5, eps=1e-8), ctrl_spec=CtrlSpec()), ctrl=ctrl, q=q
    )

    angle = np.pi / 4
    eps = 1e-8
    ctrl, q = bb.add_t(
        Controlled(Rz(angle=angle, eps=eps), ctrl_spec=CtrlSpec()), ctrl=ctrl, q=q
    )
    (ctrl,) = bb.add_t(
        Controlled(GlobalPhase(exponent=0.3, eps=1e-8), ctrl_spec=CtrlSpec()), ctrl=ctrl
    )
    ctrl, q = bb.add_t(Controlled(XGate(), ctrl_spec=CtrlSpec(cvs=0)), ctrl=ctrl, q=q)
    ctrl, q = bb.add_t(Controlled(MyGate(), ctrl_spec=CtrlSpec()), ctrl=ctrl, q=q)

    ctrl2_cnot, ctrl_cnot, target_cnot = bb.add_t(
        Controlled(CNOT(), ctrl_spec=CtrlSpec()),
        ctrl2=ctrl2_cnot,
        ctrl=ctrl_cnot,
        target=target_cnot,
    )
    ctrl_multi, q_multi = bb.add_t(
        Controlled(XGate(), ctrl_spec=CtrlSpec(cvs=[1, 0, 1])), ctrl=ctrl_multi, q=q_multi
    )
    ctrl2_tof, ctrl_tof, target_tof = bb.add_t(
        Controlled(subbloq=Toffoli(), ctrl_spec=CtrlSpec()),
        ctrl2=ctrl2_tof,
        ctrl=ctrl_tof,
        target=target_tof,
    )
    ctrl2_nest, ctrl_nest, q_nest = bb.add_t(
        Controlled(Controlled(TGate(), ctrl_spec=CtrlSpec()), ctrl_spec=CtrlSpec()),
        ctrl2=ctrl2_nest,
        ctrl=ctrl_nest,
        q=q_nest,
    )

    cbloq = bb.finalize(
        ctrl=ctrl,
        q=q,
        ctrl2_cnot=ctrl2_cnot,
        ctrl_cnot=ctrl_cnot,
        target_cnot=target_cnot,
        ctrl_multi=ctrl_multi,
        q_multi=q_multi,
        ctrl2_tof=ctrl2_tof,
        ctrl_tof=ctrl_tof,
        target_tof=target_tof,
        ctrl2_nest=ctrl2_nest,
        ctrl_nest=ctrl_nest,
        q_nest=q_nest,
    )
    lib = bloq_to_proto.bloqs_to_proto(cbloq, name="controlled_variants_test", max_depth=2)
    _save("controlled_variants", lib)


# Reports ArbitraryClifford via build_call_graph independent of any other bloq's own decomposition
# (deliberately not IntState, to avoid depending on IntState's own decomposition staying correct).
@attrs.frozen
class ArbitraryCliffordSource(Bloq):
    @property
    def signature(self) -> Signature:
        return Signature.build(q=1)

    def build_composite_bloq(self, bb: BloqBuilder, q):
        (q,) = bb.add_t(XGate(), q=q)
        return {"q": q}

    def build_call_graph(self, ssa):
        return {ArbitraryClifford(1): 1}


def gen_bloq_counts_variants() -> None:
    bb = BloqBuilder()
    q = bb.add_register("q", QBit())
    (q,) = bb.add_t(XGate(), q=q)
    bb.add(GlobalPhase(exponent=0.5, eps=1e-8))
    (q,) = bb.add_t(XGate(), q=q)
    bb.add(GlobalPhase(exponent=0.25, eps=1e-8))
    bb.add(GlobalPhase(exponent=0.5, eps=1e-8))
    (q,) = bb.add_t(ArbitraryCliffordSource(), q=q)
    cbloq = bb.finalize(q=q)
    lib = bloq_to_proto.bloqs_to_proto(
        cbloq, ArbitraryCliffordSource(), name="bloq_counts_variants_test", max_depth=2
    )
    _save("bloq_counts_variants", lib)


def gen_leaf_root_hadamard() -> None:
    bloq = Hadamard()
    lib = bloq_to_proto.bloqs_to_proto(bloq, name="leaf_root_hadamard_test")
    _save("leaf_root_hadamard", lib)


def gen_leaf_root_preset() -> None:
    bloq = CHadamard()
    lib = bloq_to_proto.bloqs_to_proto(bloq, name="leaf_root_preset_test")
    _save("leaf_root_preset", lib)


def gen_leaf_root_czpow() -> None:
    bloq = CZPowGate(exponent=0.5, eps=1e-8)
    lib = bloq_to_proto.bloqs_to_proto(bloq, name="leaf_root_czpow_test")
    _save("leaf_root_czpow", lib)


def gen_leaf_root_and_compute() -> None:
    bloq = And()
    lib = bloq_to_proto.bloqs_to_proto(bloq, name="leaf_root_and_compute_test")
    _save("leaf_root_and_compute", lib)


def gen_leaf_root_adjoint() -> None:
    bloq = Adjoint(CZPowGate(exponent=0.5, eps=1e-8))
    lib = bloq_to_proto.bloqs_to_proto(bloq, name="leaf_root_adjoint_test", max_depth=0)
    _save("leaf_root_adjoint", lib)


def gen_leaf_root_controlled() -> None:
    bloq = Controlled(TGate(), ctrl_spec=CtrlSpec())
    lib = bloq_to_proto.bloqs_to_proto(bloq, name="leaf_root_controlled_test", max_depth=0)
    _save("leaf_root_controlled", lib)


def gen_unsupported_bloq() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    path = OUT_DIR / "unsupported_bloq.json"
    path.write_text(
        """{
  "name": "unsupported_bloq_test",
  "table": [
    {
      "bloq": {
        "name": "qualtran.bloqs.some.UnsupportedBloq",
        "registers": {
          "registers": [
            {"name": "q", "dtype": {"qbit": {}}, "side": "THRU"}
          ]
        }
      }
    }
  ]
}
""",
        encoding="utf-8",
    )
    print(f"  wrote {path}")


def gen_controlled_unsupported_int() -> None:
    bb = BloqBuilder()
    ctrl = bb.add_register("ctrl", QUInt(4))
    q = bb.add_register("q", QBit())
    ctrl, q = bb.add_t(
        Controlled(TGate(), ctrl_spec=CtrlSpec(qdtypes=QUInt(4), cvs=6)), ctrl=ctrl, q=q
    )
    cbloq = bb.finalize(ctrl=ctrl, q=q)
    lib = bloq_to_proto.bloqs_to_proto(
        cbloq, name="controlled_unsupported_int_test", max_depth=1
    )
    _save("controlled_unsupported_int", lib)


if __name__ == "__main__":
    print(f"Output directory: {OUT_DIR}")
    gen_direct_gates()
    gen_parametrized_gates()
    gen_preset_gate_calls()
    gen_and_gates()
    gen_czpow_multiple()
    gen_swap_cswap_decomposed()
    gen_swap_cswap_leaf()
    gen_state_basis()
    gen_producer_boundary()
    gen_int_state_effect()
    gen_consumer_variants()
    gen_alias_variants()
    gen_subcircuit_variants()
    gen_adjoint_variants()
    gen_adjoint_decomposed()
    gen_controlled_variants()
    gen_bloq_counts_variants()
    gen_leaf_root_hadamard()
    gen_leaf_root_preset()
    gen_leaf_root_czpow()
    gen_leaf_root_and_compute()
    gen_leaf_root_adjoint()
    gen_leaf_root_controlled()
    gen_unsupported_bloq()
    gen_controlled_unsupported_int()
    print("Done.")
