# ruff: noqa: I001
import itertools
from pathlib import Path

import pytest
pytest.importorskip("pyqret.algorithm", exc_type=ImportError)
from pyqret.algorithm.arithmetic import temporal_and, uncompute_temporal_and
from pyqret.frontend import Argument, CircuitBuilder, CircuitGenerator, Context, Module
from pyqret.frontend.gate.intrinsic import cx, measure
from pyqret.runtime import QuantumStateType, Simulator, SimulatorConfig


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

        # Define circuit
        temporal_and(aux[0], src[0], dst[0])
        for i in range(1, self.N - 1):
            cx(src[i], aux[i - 1])
            cx(dst[i], aux[i - 1])
            temporal_and(aux[i], src[i], dst[i])
            cx(aux[i], aux[i - 1])
        cx(dst[self.N - 1], aux[self.N - 2])
        for i in range(self.N - 2, 0, -1):
            cx(aux[i], aux[i - 1])
            uncompute_temporal_and(aux[i], src[i], dst[i])
            cx(src[i], aux[i - 1])
        uncompute_temporal_and(aux[0], src[0], dst[0])
        for i in range(self.N):
            cx(dst[i], src[i])

        # Measure all operate qubits
        for q, r in zip(dst, m_dst):
            measure(q, r)
        for q, r in zip(src, m_src):
            measure(q, r)


def test_circuit_builder():
    context = Context()
    module = Module("example", context)
    builder = CircuitBuilder(module)
    gen = Adder(builder, 10)
    gen.generate()

    assert {"add_10", "TemporalAnd", "UncomputeTemporalAnd"} == set(builder.get_circuit_list())

    tma = builder.get_circuit("TemporalAnd")
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


@pytest.mark.parametrize(
    "filename",
    [
        # "OpenQASM2/inv_qft_v1.qasm", # nest of `if` is currently not supported
        "OpenQASM2/inv_qft_v2.qasm",
        "OpenQASM2/qft.qasm",
        "OpenQASM2/rca.qasm",
        "OpenQASM2/teleport.qasm",
    ],
)
def test_parse_openqasm2(filename: str):
    context = Context()
    module = Module("openqasm2", context)
    builder = CircuitBuilder(module)
    ipath = Path(__file__).parent / filename
    circuit = builder.parse_openqasm2(str(ipath), "main")

    assert circuit.has_frontend()
    assert circuit.has_ir()
    assert circuit.get_ir().name() == "main"
    assert not circuit.has_mf()


@pytest.mark.parametrize(
    ("object", "size"),
    itertools.product(
        [
            ("add_operate", "add_operates"),
            ("add_clean_ancilla", "add_clean_ancillae"),
            ("add_dirty_ancilla", "add_dirty_ancillae"),
            ("add_input", "add_inputs"),
            ("add_output", "add_outputs"),
        ],
        [0, 9, 100],
    ),
)
def test_quantum_object(object: str, size: int):
    single, array = object
    is_qubit = single in {"add_operate", "add_clean_ancilla", "add_dirty_ancilla"}
    N = size
    M = size // 3

    context = Context()
    module = Module("test_quantum_object_module", context)
    builder = CircuitBuilder(module)

    # Define args
    arg = Argument()
    getattr(arg, array)(array, N)
    getattr(arg, single)(single)
    circuit = builder._create_circuit("test_quantum_object_circuit", arg)
    builder._begin_circuit_definition(circuit)

    #####################################
    # Test array
    #####################################

    # ops = [q_0, ..., q_{N-1}]
    ps = arg[array]
    assert is_qubit == ps.is_qubit()
    assert (not is_qubit) == ps.is_register()
    assert N == ps.size()
    assert list(range(N)) == [i.get_id() for i in ps]
    # q_0, ..., q_{M-1}
    assert list(range(M)) == [i.get_id() for i in ps.range(0, M)]
    # q_M, ..., q_{2*M-1}
    assert list(range(M, 2 * M)) == [i.get_id() for i in ps.range(M, M)]
    if N != 0:
        assert 0 == ps[0].get_id()
        assert N - 1 == ps[N - 1].get_id()

    # Test p
    p = arg[single]
    assert is_qubit == p.is_qubit()
    assert (not is_qubit) == p.is_register()
    assert 1 == p.size()
    assert N == p.get_id()

    #####################################
    # Test operator overloads
    #####################################

    # __str__
    if is_qubit:
        assert f"q{N}" == str(p)
        if N > 2:
            assert "{q0,q1}" == str(ps.range(0, 2))
    else:
        # register
        assert f"r{N}" == str(p)
        if N > 2:
            assert "{r0,r1}" == str(ps.range(0, 2))

    # __add__ (array + single)
    # [q_0, ..., q_{N-1}] + [q_N]
    assert N + 1 == (ps + p).size()
    assert [*list(range(N)), p.get_id()] == [i.get_id() for i in (ps + p)]

    # __add__ (array + array)
    # [q_0, ..., q_{N/2-1}] + [q_{N/2}, ..., q_{N-1}]
    assert list(range(N)) == [i.get_id() for i in (ps.range(0, N // 2) + (ps.range(N // 2, N - N // 2)))]

    # __radd__ (single + array)
    # [q_N] + [q_0, ..., q_{N-1}]
    assert N + 1 == (p + ps).size()
    assert [p.get_id(), *list(range(N))] == [i.get_id() for i in (p + ps)]

    # __iadd__ (array += array)
    # [q_0, ..., q_{N/2-1}] + [q_0, ..., q_{N-1}]
    qs = ps.range(0, N // 2)
    qs += ps
    assert N // 2 + N == qs.size()
    assert list(range(N // 2)) + list(range(N)) == [i.get_id() for i in qs]

    # __iadd__ (array += single)
    # [q_0, ..., q_{N/2-1}] + [q_{N}]
    qs = ps.range(0, N // 2)
    qs += p
    assert N // 2 + 1 == qs.size()
    assert [*list(range(N // 2)), N] == [i.get_id() for i in qs]

    # FIXME: Is it correct to behave as follows when there is self-assignment?
    # __iadd__ (array += array), self assignment
    # [q_0, ..., q_{N-1}] + [q_0, ..., q_{N-1}]
    qs = ps
    qs += ps
    assert 2 * N == qs.size()
    assert 2 * N == ps.size()
    assert list(range(N)) + list(range(N)) == [i.get_id() for i in qs]

    builder._end_circuit_definition(circuit)
