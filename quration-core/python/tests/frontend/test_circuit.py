from pyqret.frontend import (
    Argument,
    CircuitBuilder,
    CircuitGenerator,
    Context,
    Module,
    Qubit,
)
from pyqret.frontend.gate import control_flow, intrinsic
from pyqret.runtime import QuantumStateType, Simulator, SimulatorConfig


class TemporalAnd(CircuitGenerator):
    def __init__(self, builder: CircuitBuilder) -> None:
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


def temporal_and(builder: CircuitBuilder, t: Qubit, c0: Qubit, c1: Qubit):
    TemporalAnd(builder).generate()(t, c0, c1)


class UncomputeTemporalAnd(CircuitGenerator):
    def __init__(self, builder: CircuitBuilder) -> None:
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


def uncompute_temporal_and(builder: CircuitBuilder, t: Qubit, c0: Qubit, c1: Qubit):
    UncomputeTemporalAnd(builder).generate()(t, c0, c1)


class Adder(CircuitGenerator):
    def __init__(self, builder: CircuitBuilder, N: int) -> None:
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

        def t_and(t: Qubit, c0: Qubit, c1: Qubit) -> None:
            temporal_and(self.builder, t, c0, c1)

        def ut_and(t: Qubit, c0: Qubit, c1: Qubit) -> None:
            uncompute_temporal_and(self.builder, t, c0, c1)

        # Define circuit
        t_and(aux[0], src[0], dst[0])
        for i in range(1, self.N - 1):
            intrinsic.cx(src[i], aux[i - 1])
            intrinsic.cx(dst[i], aux[i - 1])
            t_and(aux[i], src[i], dst[i])
            intrinsic.cx(aux[i], aux[i - 1])
        intrinsic.cx(dst[self.N - 1], aux[self.N - 2])
        for i in range(self.N - 2, 0, -1):
            intrinsic.cx(aux[i], aux[i - 1])
            ut_and(aux[i], src[i], dst[i])
            intrinsic.cx(src[i], aux[i - 1])
        ut_and(aux[0], src[0], dst[0])
        for i in range(self.N):
            intrinsic.cx(dst[i], src[i])

        # Measure all operate qubits
        for q, r in zip(dst, m_dst):
            intrinsic.measure(q, r)
        for q, r in zip(src, m_src):
            intrinsic.measure(q, r)


def test_circuit_call_overload():
    context = Context()
    module = Module("example", context)
    builder = CircuitBuilder(module)
    gen = Adder(builder, 10)
    gen.generate()

    assert {"add_10", "temporal_and", "uncompute_temporal_and"} == set(builder.get_circuit_list())

    tma = builder.get_circuit("temporal_and")
    assert tma.has_frontend()
    assert tma.has_ir()
    assert not tma.has_mf()

    for i in range(2):
        for j in range(2):
            config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
            sim = Simulator(config, tma)
            sim.set("c0", i)
            sim.set("c1", j)
            sim.run()
            state = sim.get_state()
            assert state.get_value("t") == i & j
            assert state.get_value("c0") == i
            assert state.get_value("c1") == j

    add = builder.get_circuit("add_10")
    assert add.has_frontend()
    assert add.has_ir()
    assert not add.has_mf()

    for dst in range(1, 1024, 314):
        for src in range(2, 1024, 413):
            sim = Simulator(config, add)
            sim.set("dst", dst)
            sim.set("src", src)
            sim.run()
            state = sim.get_state()
            assert state.get_value("dst") == (dst + src) % 1024
            assert state.get_value("src") == src
