from pyqret.frontend import (
    Argument,
    CircuitBuilder,
    CircuitGenerator,
    Context,
    Module,
    Opcode,
)
from pyqret.frontend.gate import control_flow, intrinsic


class IntrinsicGateTestGen(CircuitGenerator):
    def __init__(self, builder: CircuitBuilder) -> None:
        super().__init__(builder)

    def name(self) -> str:
        return "test_intrinsic_gate"

    def arg(self) -> Argument:
        arg = Argument()
        arg.add_operates("foo", 3)
        arg.add_outputs("bar", 3)
        return arg

    def logic(self, arg: Argument):
        foo = arg["foo"]
        bar = arg["bar"]

        _ = self.get_temporal_register()
        _ = self.get_temporal_registers(3)

        intrinsic.i(foo[0])
        intrinsic.x(foo[0])
        intrinsic.y(foo[0])
        intrinsic.z(foo[0])
        intrinsic.h(foo[0])
        intrinsic.s(foo[0])
        intrinsic.sdag(foo[0])
        intrinsic.t(foo[0])
        intrinsic.tdag(foo[0])
        intrinsic.rx(foo[0], 0.1)
        intrinsic.ry(foo[0], 0.2)
        intrinsic.rz(foo[0], 0.3)
        intrinsic.cx(foo[0], foo[1])
        intrinsic.cy(foo[0], foo[1])
        intrinsic.cz(foo[0], foo[1])
        intrinsic.ccx(foo[0], foo[1], foo[2])
        intrinsic.ccy(foo[0], foo[1], foo[2])
        intrinsic.ccz(foo[0], foo[1], foo[2])
        intrinsic.mcx(foo[0], foo.range(1, 2))
        intrinsic.global_phase(self.builder, 0.4)

        intrinsic.discrete_distribution([0.1, 0.2, 6.0], bar.range(1, 2))

        for q, r in zip(foo, bar):
            intrinsic.measure(q, r)

        control_flow.qu_if(bar[0])
        intrinsic.y(foo[0])
        intrinsic.z(foo[0])
        control_flow.qu_else(bar[0])
        intrinsic.z(foo[0])
        intrinsic.y(foo[0])
        control_flow.qu_end_if(bar[0])

        control_flow.qu_switch(bar.range(0, 2))
        control_flow.qu_case(bar.range(0, 2), 0)
        intrinsic.y(foo[0])
        intrinsic.z(foo[0])
        control_flow.qu_case(bar.range(0, 2), 2)
        intrinsic.z(foo[0])
        control_flow.qu_case(bar.range(0, 2), 4)
        intrinsic.y(foo[0])
        control_flow.qu_default(bar.range(0, 2))
        intrinsic.h(foo[0])
        control_flow.qu_end_switch(bar.range(0, 2))


def test_intrinsic_gate():
    context = Context()
    module = Module("test", context)
    builder = CircuitBuilder(module)
    gen = IntrinsicGateTestGen(builder)
    circuit = gen.generate()

    for bb in circuit.get_ir():
        print(f"bb: {bb.name()}")
        for inst in bb:
            print("  ", inst)

    ops = [
        Opcode.I,
        Opcode.X,
        Opcode.Y,
        Opcode.Z,
        Opcode.H,
        Opcode.S,
        Opcode.SDag,
        Opcode.T,
        Opcode.TDag,
        Opcode.RX,
        Opcode.RY,
        Opcode.RZ,
        Opcode.CX,
        Opcode.CY,
        Opcode.CZ,
        Opcode.CCX,
        Opcode.CCY,
        Opcode.CCZ,
        Opcode.MCX,
        Opcode.GlobalPhase,
        Opcode.DiscreteDistribution,
        Opcode.Measurement,
        Opcode.Measurement,
        Opcode.Measurement,
        Opcode.Branch,
    ]
    for bb in circuit.get_ir():
        if bb.name() != "entry":
            continue
        assert bb.num_instructions() == len(ops)
        for op, inst in zip(ops, bb):
            assert op == inst.get_opcode()

    num_insts = {
        "entry": len(ops),
        "then_0": 3,
        "else_0": 3,
        "if_cont_0": 1,
        "case_0_0": 3,
        "case_2_0": 2,
        "case_4_0": 2,
        "default_0": 2,
        "switch_cont_0": 1,
    }
    for bb in circuit.get_ir():
        assert bb.name() in num_insts
        assert num_insts[bb.name()] == bb.num_instructions()

    for is_entry, bb in zip([True, False, False, False], circuit.get_ir()):
        assert is_entry == bb.is_entry_block()

    for is_unitary, bb in zip([False, True, True, True], circuit.get_ir()):
        assert is_unitary == bb.is_unitary()
        assert is_unitary != bb.does_measurement()

    assert "test_intrinsic_gate" == circuit.get_ir().name()
    entry_bb = circuit.get_ir().get_entry_bb()
    assert "entry" == entry_bb.name()
    assert entry_bb.is_entry_block()
    assert sum(num_insts.values()) == circuit.get_ir().num_instructions()
    assert 3 == circuit.get_ir().num_qubits()
    assert 3 == circuit.get_ir().num_registers()
    assert 4 == circuit.get_ir().num_tmp_registers()
    assert not circuit.get_ir().is_unitary()
    assert circuit.get_ir().does_measurement()

    print(circuit.get_ir().gen_cfg())
