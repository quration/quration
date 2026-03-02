# ruff: noqa: I001
import pytest
pytest.importorskip("pyqret.algorithm", exc_type=ImportError)
from pyqret.algorithm.arithmetic import add_craig, add_cuccaro, sub_craig
from pyqret.frontend import Argument, CircuitBuilder, CircuitGenerator, Context, Module
from pyqret.frontend.gate.intrinsic import measure
from pyqret.runtime import QuantumStateType, Simulator, SimulatorConfig

from pyqret import bool_array_as_int


class AdderTest(CircuitGenerator):
    def __init__(self, builder: CircuitBuilder, N: int) -> None:
        super().__init__(builder)
        self.N = N

    def name(self):
        return f"add_{self.N}"

    def arg(self) -> Argument:
        ret = Argument()
        ret.add_operates("dst", self.N)
        ret.add_operates("src", self.N)
        ret.add_clean_ancillae("aux", self.N)
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
        sub_craig(dst, src, aux.range(0, self.N - 1))
        add_cuccaro(dst, src, aux[0])
        add_craig(dst, src, aux.range(0, self.N - 1))

        for q, r in zip(dst, m_dst):
            measure(q, r)
        for q, r in zip(src, m_src):
            measure(q, r)


def test_quantum_state_type():
    assert QuantumStateType.Toffoli == QuantumStateType.Toffoli
    assert QuantumStateType.FullQuantum == QuantumStateType.FullQuantum
    assert QuantumStateType.Toffoli != QuantumStateType.FullQuantum
    assert QuantumStateType.FullQuantum != QuantumStateType.Toffoli
    assert str(QuantumStateType.Toffoli) == "QuantumStateType.Toffoli"
    assert str(QuantumStateType.FullQuantum) == "QuantumStateType.FullQuantum"


def test_toffoli_simulator():
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=314, max_superpositions=23984)

    assert QuantumStateType.Toffoli == config.state_type
    assert 314 == config.seed
    assert 23984 == config.max_superpositions

    # Define context, module, and builder
    context = Context()
    module = Module("example", context)
    builder = CircuitBuilder(module)

    # Generate circuit
    adder = AdderTest(builder, 5)
    circuit = adder.generate()

    # Test config
    sim = Simulator(config, circuit)
    config2 = sim.get_config()

    assert config.state_type == config2.state_type
    assert config.seed == config2.seed
    assert config.max_superpositions == config2.max_superpositions

    sim.set("dst", 18)
    sim.set("src", 7)
    result = sim.run()

    assert 25 == bool_array_as_int(result["m_dst"])
    assert 7 == bool_array_as_int(result["m_src"])

    state = sim.get_state()

    assert 1 == state.get_num_superpositions()
    assert state.is_computational_basis()
    assert not state.is_superposed(0)
    # 25 = 10011
    for i, prob in enumerate([1, 0, 0, 1, 1]):
        assert float(1 - prob) == pytest.approx(state.calc_0_prob(i))
        assert float(prob) == pytest.approx(state.calc_1_prob(i))

    assert 1.0 == state.get_global_phase()

    sol = [1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0]
    assert 1 == len(state.get_raw_vector())
    assert 1.0 == state.get_raw_vector()[0][0]
    assert sol == state.get_raw_vector()[0][1]


def test_full_quantum_simulator():
    config = SimulatorConfig(state_type=QuantumStateType.FullQuantum, seed=314, max_superpositions=23984)

    assert QuantumStateType.FullQuantum == config.state_type
    assert 314 == config.seed
    assert 23984 == config.max_superpositions

    # Define context, module, and builder
    context = Context()
    module = Module("example", context)
    builder = CircuitBuilder(module)

    # Generate circuit
    adder = AdderTest(builder, 5)
    circuit = adder.generate()

    # Test config
    sim = Simulator(config, circuit)
    config2 = sim.get_config()

    assert config.state_type == config2.state_type
    assert config.seed == config2.seed
    assert config.max_superpositions == config2.max_superpositions

    sim.set("dst", 18)
    sim.set("src", 7)
    result = sim.run()

    assert 25 == bool_array_as_int(result["m_dst"])
    assert 7 == bool_array_as_int(result["m_src"])

    state = sim.get_state()

    assert 1 == state.get_num_superpositions()
    assert state.is_computational_basis()
    assert not state.is_superposed(0)
    # 25 = 10011
    for i, prob in enumerate([1, 0, 0, 1, 1]):
        assert float(1 - prob) == pytest.approx(state.calc_0_prob(i))
        assert float(prob) == pytest.approx(state.calc_1_prob(i))

    assert 0.0 == state.get_entropy()

    sol = [1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0]

    assert bool_array_as_int([x == 1 for x in sol]) == state.sampling(2)[0]
    assert bool_array_as_int([x == 1 for x in sol]) == state.sampling(3)[2]

    print(state.get_state_vector())
