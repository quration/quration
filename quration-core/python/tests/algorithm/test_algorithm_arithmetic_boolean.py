# ruff: noqa: I001
import itertools

import pytest
pytest.importorskip("pyqret.algorithm.arithmetic", exc_type=ImportError)
from pyqret.algorithm.arithmetic import *
from pyqret.frontend import ArgInfo
from pyqret.runtime import QuantumStateType, Simulator, SimulatorConfig

from pyqret import bool_array_as_int

from frontend.common import generate_circuit_for_test


@pytest.mark.parametrize(("t", "c0", "c1"), list(itertools.product([0, 1], [0, 1], [0, 1])))
def test_rt3(t: int, c0: int, c1: int):
    circuit = generate_circuit_for_test(
        "rt3",
        rt3,
        [
            ArgInfo.operate("t"),
            ArgInfo.operate("c0"),
            ArgInfo.operate("c1"),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    sim = Simulator(config, circuit)
    sim.set("t", t)
    sim.set("c0", c0)
    sim.set("c1", c1)
    result = sim.run()
    assert bool_array_as_int(result["m_t"]) == (t + (c0 + c1) // 2) % 2
    assert bool_array_as_int(result["m_c0"]) == c0
    assert bool_array_as_int(result["m_c1"]) == c1


@pytest.mark.parametrize(("t", "c0", "c1"), list(itertools.product([0, 1], [0, 1], [0, 1])))
def test_irt3(t: int, c0: int, c1: int):
    circuit = generate_circuit_for_test(
        "irt3",
        irt3,
        [
            ArgInfo.operate("t"),
            ArgInfo.operate("c0"),
            ArgInfo.operate("c1"),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    sim = Simulator(config, circuit)
    sim.set("t", t)
    sim.set("c0", c0)
    sim.set("c1", c1)
    result = sim.run()
    assert bool_array_as_int(result["m_t"]) == (t + (c0 + c1) // 2) % 2
    assert bool_array_as_int(result["m_c0"]) == c0
    assert bool_array_as_int(result["m_c1"]) == c1


@pytest.mark.parametrize(("t", "c0", "c1"), list(itertools.product([0, 1], [0, 1], [0, 1])))
def test_rt4(t: int, c0: int, c1: int):
    circuit = generate_circuit_for_test(
        "rt4",
        rt4,
        [
            ArgInfo.operate("t"),
            ArgInfo.operate("c0"),
            ArgInfo.operate("c1"),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    sim = Simulator(config, circuit)
    sim.set("t", t)
    sim.set("c0", c0)
    sim.set("c1", c1)
    result = sim.run()
    assert bool_array_as_int(result["m_t"]) == (t + (c0 + c1) // 2) % 2
    assert bool_array_as_int(result["m_c0"]) == c0
    assert bool_array_as_int(result["m_c1"]) == c1


@pytest.mark.parametrize(("t", "c0", "c1"), list(itertools.product([0, 1], [0, 1], [0, 1])))
def test_irt4(t: int, c0: int, c1: int):
    circuit = generate_circuit_for_test(
        "irt4",
        irt4,
        [
            ArgInfo.operate("t"),
            ArgInfo.operate("c0"),
            ArgInfo.operate("c1"),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    sim = Simulator(config, circuit)
    sim.set("t", t)
    sim.set("c0", c0)
    sim.set("c1", c1)
    result = sim.run()
    assert bool_array_as_int(result["m_t"]) == (t + (c0 + c1) // 2) % 2
    assert bool_array_as_int(result["m_c0"]) == c0
    assert bool_array_as_int(result["m_c1"]) == c1


@pytest.mark.parametrize(("c0", "c1"), list(itertools.product([0, 1], [0, 1])))
def test_temporal_and(c0: int, c1: int):
    circuit = generate_circuit_for_test(
        "temporal_and",
        temporal_and,
        [
            ArgInfo.operate("t"),
            ArgInfo.operate("c0"),
            ArgInfo.operate("c1"),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    sim = Simulator(config, circuit)
    sim.set("c0", c0)
    sim.set("c1", c1)
    result = sim.run()
    assert bool_array_as_int(result["m_t"]) == (c0 + c1) // 2
    assert bool_array_as_int(result["m_c0"]) == c0
    assert bool_array_as_int(result["m_c1"]) == c1


@pytest.mark.parametrize(("c0", "c1"), list(itertools.product([0, 1], [0, 1])))
def test_uncompute_temporal_and(c0: int, c1: int):
    circuit = generate_circuit_for_test(
        "uncompute_temporal_and",
        uncompute_temporal_and,
        [
            ArgInfo.operate("t"),
            ArgInfo.operate("c0"),
            ArgInfo.operate("c1"),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    sim = Simulator(config, circuit)
    sim.set("t", (c0 + c1) // 2)
    sim.set("c0", c0)
    sim.set("c1", c1)
    result = sim.run()
    assert bool_array_as_int(result["m_t"]) == 0
    assert bool_array_as_int(result["m_c0"]) == c0
    assert bool_array_as_int(result["m_c1"]) == c1


@pytest.mark.parametrize(("x", "y", "z"), list(itertools.product([0, 1], [0, 1], [0, 1])))
def test_maj(x: int, y: int, z: int):
    circuit = generate_circuit_for_test(
        "maj",
        maj,
        [
            ArgInfo.operate("x"),
            ArgInfo.operate("y"),
            ArgInfo.operate("z"),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    sim = Simulator(config, circuit)
    sim.set("x", x)
    sim.set("y", y)
    sim.set("z", z)
    result = sim.run()
    assert bool_array_as_int(result["m_x"]) == (x + y + z) // 2
    assert bool_array_as_int(result["m_y"]) == (x + y) % 2
    assert bool_array_as_int(result["m_z"]) == (x + z) % 2


@pytest.mark.parametrize(("x", "y", "z"), list(itertools.product([0, 1], [0, 1], [0, 1])))
def test_uma(x: int, y: int, z: int):
    circuit = generate_circuit_for_test(
        "uma",
        uma,
        [
            ArgInfo.operate("x"),
            ArgInfo.operate("y"),
            ArgInfo.operate("z"),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    sim = Simulator(config, circuit)
    sim.set("x", (x + y + z) // 2)
    sim.set("y", (x + y) % 2)
    sim.set("z", (x + z) % 2)
    result = sim.run()
    assert bool_array_as_int(result["m_x"]) == x
    assert bool_array_as_int(result["m_y"]) == y
    assert bool_array_as_int(result["m_z"]) == (x + y + z) % 2
