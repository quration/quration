import pytest
from pyqret.frontend import Argument, CircuitBuilder, CircuitGenerator, Context, IRFunction, Module, Opcode
from pyqret.frontend.gate import control_flow, intrinsic
from pyqret.frontend.transforms import inline_call_inst, inline_circuit


class TemporalAnd(CircuitGenerator):
    def __init__(self, builder: CircuitBuilder):
        super().__init__(builder)

    def name(self):
        return "temporal_and"

    def arg(self) -> Argument:
        ret = Argument()
        ret.add_operate("t")
        ret.add_operate("c0")
        ret.add_operate("c1")
        return ret

    def logic(self, arg: Argument):
        t = arg["t"]
        c0 = arg["c0"]
        c1 = arg["c1"]

        # Define circuit
        intrinsic.h(t)
        intrinsic.t(t)
        intrinsic.cx(t, c0)
        intrinsic.cx(t, c1)
        intrinsic.cx(c0, t)
        intrinsic.cx(c1, t)
        intrinsic.tdag(c0)
        intrinsic.tdag(c1)
        intrinsic.t(t)
        intrinsic.cx(c0, t)
        intrinsic.cx(c1, t)
        intrinsic.h(t)
        intrinsic.s(t)


class UncomputeTemporalAnd(CircuitGenerator):
    def __init__(self, builder: CircuitBuilder):
        super().__init__(builder)

    def name(self):
        return "uncompute_temporal_and"

    def arg(self) -> Argument:
        ret = Argument()
        ret.add_operate("t")
        ret.add_operate("c0")
        ret.add_operate("c1")
        return ret

    def logic(self, arg: Argument):
        t = arg["t"]
        c0 = arg["c0"]
        c1 = arg["c1"]

        r = self.get_temporal_register()

        # Define circuit
        intrinsic.h(t)
        intrinsic.measure(t, r)
        control_flow.qu_if(r)
        intrinsic.x(t)
        intrinsic.cz(c0, c1)
        control_flow.qu_end_if(r)


class Adder(CircuitGenerator):
    def __init__(self, builder: CircuitBuilder, N: int):
        super().__init__(builder)
        self.N = N

    def name(self):
        return f"add_{self.N}"

    def arg(self) -> Argument:
        ret = Argument()
        ret.add_operates("dst", self.N)
        ret.add_operates("src", self.N)
        ret.add_clean_ancillae("aux", self.N - 1)
        ret.add_outputs("m_dst", self.N)
        ret.add_outputs("m_src", self.N)

        return ret

    def logic(self, arg: Argument):
        dst = arg["dst"]
        src = arg["src"]
        aux = arg["aux"]
        m_dst = arg["m_dst"]
        m_src = arg["m_src"]

        def temporal_and(t, c0, c1):
            gen = TemporalAnd(self.builder)
            circuit = gen.generate()
            circuit(t, c0, c1)

        def uncompute_temporal_and(t, c0, c1):
            gen = UncomputeTemporalAnd(self.builder)
            circuit = gen.generate()
            circuit(t, c0, c1)

        # Define circuit
        temporal_and(aux[0], src[0], dst[0])
        for i in range(1, self.N - 1):
            intrinsic.cx(src[i], aux[i - 1])
            intrinsic.cx(dst[i], aux[i - 1])
            temporal_and(aux[i], src[i], dst[i])
            intrinsic.cx(aux[i], aux[i - 1])
        intrinsic.cx(dst[self.N - 1], aux[self.N - 2])
        for i in range(self.N - 2, 0, -1):
            intrinsic.cx(aux[i], aux[i - 1])
            uncompute_temporal_and(aux[i], src[i], dst[i])
            intrinsic.cx(src[i], aux[i - 1])
        uncompute_temporal_and(aux[0], src[0], dst[0])
        for i in range(self.N):
            intrinsic.cx(dst[i], src[i])

        # Measure all operate qubits
        for q, r in zip(dst, m_dst):
            intrinsic.measure(q, r)
        for q, r in zip(src, m_src):
            intrinsic.measure(q, r)


def dump_ir_circuit(circuit: IRFunction):
    for bb in circuit.get_ir():
        print(f"{bb.name()}:")
        for inst in bb:
            print(f"  {inst}")


@pytest.fixture
def circuit():
    context = Context()
    module = Module("module", context)
    builder = CircuitBuilder(module)
    gen = Adder(builder, 5)

    return gen.generate()


def test_inline_call_inst_not_merge(circuit):
    print("======================")
    dump_ir_circuit(circuit)

    num_instructions_before_inlining = circuit.get_ir().num_instructions()

    print("======================")
    # Inline temporal_and
    for bb in circuit.get_ir():
        for inst in bb:
            if inst.get_opcode() == Opcode.Call:
                print("inlining: ", str(inst))
                inline_call_inst(inst, merge_trivial=False)
            break

    print("======================")
    dump_ir_circuit(circuit)

    num_instructions_after_inlining = circuit.get_ir().num_instructions()

    # #BB = 3
    assert 3 == sum(1 for _ in circuit.get_ir())
    assert num_instructions_before_inlining < num_instructions_after_inlining


def test_inline_call_inst_merge(circuit):
    print("======================")
    dump_ir_circuit(circuit)

    num_instructions_before_inlining = circuit.get_ir().num_instructions()

    print("======================")
    # Inline temporal_and
    for bb in circuit.get_ir():
        for inst in bb:
            if inst.get_opcode() == Opcode.Call:
                print("inlining: ", str(inst))
                inline_call_inst(inst, merge_trivial=True)
            break

    print("======================")
    dump_ir_circuit(circuit)

    num_instructions_after_inlining = circuit.get_ir().num_instructions()

    # #BB = 1
    assert 1 == sum(1 for _ in circuit.get_ir())
    assert num_instructions_before_inlining < num_instructions_after_inlining


def test_inline_call_inst_not_call(circuit):
    not_call_inst = None
    for bb in circuit.get_ir():
        for inst in bb:
            if inst.get_opcode() != Opcode.Call:
                not_call_inst = inst
                break
        if not_call_inst:
            break

    assert not_call_inst is not None

    with pytest.raises(ValueError):
        inline_call_inst(not_call_inst)


def test_inline_circuit_not_merge(circuit):
    print("======================")
    dump_ir_circuit(circuit)

    num_instructions_before_inlining = circuit.get_ir().num_instructions()

    inline_circuit(circuit.get_ir(), merge_trivial=False)

    print("======================")
    dump_ir_circuit(circuit)

    num_instructions_after_inlining = circuit.get_ir().num_instructions()

    assert 1 < sum(1 for _ in circuit.get_ir())
    assert num_instructions_before_inlining < num_instructions_after_inlining
    for bb in circuit.get_ir():
        for inst in bb:
            assert inst.get_opcode() != Opcode.Call


def test_inline_circuit_merge(circuit):
    print("======================")
    dump_ir_circuit(circuit)

    num_instructions_before_inlining = circuit.get_ir().num_instructions()

    inline_circuit(circuit.get_ir(), merge_trivial=True)

    print("======================")
    dump_ir_circuit(circuit)

    num_instructions_after_inlining = circuit.get_ir().num_instructions()

    assert 1 < sum(1 for _ in circuit.get_ir())
    assert num_instructions_before_inlining < num_instructions_after_inlining
    for bb in circuit.get_ir():
        for inst in bb:
            assert inst.get_opcode() != Opcode.Call
