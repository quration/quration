# ruff: noqa: I001
import pytest
pytest.importorskip("pyqret.algorithm.util", exc_type=ImportError)
from pyqret.algorithm.util import *
from pyqret.frontend import ArgInfo
from pyqret.runtime import QuantumStateType, Simulator, SimulatorConfig

from pyqret import bool_array_as_int, int_as_bool_array

from frontend.common import generate_circuit_for_test


@pytest.mark.parametrize(
    ("size", "tgt"),
    [
        (5, 3),
        (5, 31),
        (10, 42),
        (1000, 2259781389479135703987517),
    ],
)
def test_apply_x_to_each(size: int, tgt: int):
    circuit = generate_circuit_for_test(
        f"apply_x_to_each_{size}",
        apply_x_to_each,
        [
            ArgInfo.operates("tgt", size),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    sim = Simulator(config, circuit)
    sim.set("tgt", tgt)
    result = sim.run()
    assert bool_array_as_int(result["m_tgt"]) == 2**size - 1 - tgt


@pytest.mark.parametrize(
    ("size", "tgt"),
    [
        (5, 3),
        (5, 31),
        (10, 42),
        (1000, 2259781389479135703987517),
    ],
)
def test_apply_x_to_each_if(size: int, tgt: int):
    circuit = generate_circuit_for_test(
        f"apply_x_to_each_if_{size}",
        apply_x_to_each_if,
        [
            ArgInfo.operates("tgt", size),
            ArgInfo.inputs("bools", size),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    sim = Simulator(config, circuit)
    sim.set("bools", tgt)
    result = sim.run()
    assert bool_array_as_int(result["m_tgt"]) == tgt


@pytest.mark.parametrize(
    ("size", "tgt"),
    [
        (5, 3),
        (5, 31),
        (10, 42),
        (1000, 2259781389479135703987517),
    ],
)
def test_apply_cx_to_each_if(size: int, tgt: int):
    circuit = generate_circuit_for_test(
        f"apply_cx_to_each_if_{size}",
        apply_cx_to_each_if,
        [
            ArgInfo.operate("c"),
            ArgInfo.operates("tgt", size),
            ArgInfo.inputs("bools", size),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    for v in [0, 1]:
        sim = Simulator(config, circuit)
        sim.set("c", v)
        sim.set("bools", tgt)
        result = sim.run()
        assert bool_array_as_int(result["m_tgt"]) == (0 if v == 0 else tgt)


@pytest.mark.parametrize(
    ("size", "tgt"),
    [
        (5, 3),
        (5, 31),
        (10, 42),
        (1000, 2259781389479135703987517),
    ],
)
def test_apply_x_to_each_if_imm(size: int, tgt: int):
    circuit = generate_circuit_for_test(
        f"apply_x_to_each_if_imm_{size}",
        apply_x_to_each_if_imm,
        [
            tgt,
            ArgInfo.operates("tgt", size),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    sim = Simulator(config, circuit)
    result = sim.run()
    assert bool_array_as_int(result["m_tgt"]) == tgt


@pytest.mark.parametrize(
    ("size", "tgt"),
    [
        (5, 3),
        (5, 31),
        (10, 42),
        (1000, 2259781389479135703987517),
    ],
)
def test_apply_cx_to_each_if_imm(size: int, tgt: int):
    circuit = generate_circuit_for_test(
        f"apply_cx_to_each_if_imm_{size}",
        apply_cx_to_each_if_imm,
        [
            tgt,
            ArgInfo.operate("c"),
            ArgInfo.operates("tgt", size),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    for v in [0, 1]:
        sim = Simulator(config, circuit)
        sim.set("c", v)
        result = sim.run()
        assert bool_array_as_int(result["m_tgt"]) == (0 if v == 0 else tgt)


@pytest.mark.parametrize(
    ("size", "l", "tgt"),
    [
        (5, 3, 3),
        (5, 3, 31),
        (10, 8, 42),
        (1000, 329, 2259781389479135703987517),
    ],
)
def test_multi_controlled_bit_order_rotation(size: int, l: int, tgt: int):
    csize = 3
    circuit = generate_circuit_for_test(
        f"multi_controlled_bit_order_rotation_{size}",
        multi_controlled_bit_order_rotation,
        [
            l,
            ArgInfo.operates("cs", csize),
            ArgInfo.operates("tgt", size),
            ArgInfo.dirty_ancillae("aux", 0),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    for v in [0, 2**csize - 1]:
        sim = Simulator(config, circuit)
        sim.set("cs", v)
        sim.set("tgt", tgt)
        result = sim.run()
        sol = int_as_bool_array(tgt)
        if len(sol) != size:
            sol += [False] * (size - len(sol))

        assert result["m_tgt"][: size - l] == (sol[: size - l] if v == 0 else sol[l:size])
        assert result["m_tgt"][size - l : size] == (sol[size - l : size] if v == 0 else sol[:l])


@pytest.mark.parametrize(
    ("size", "tgt"),
    [
        (5, 3),
        (5, 31),
        (10, 42),
        (1000, 2259781389479135703987517),
    ],
)
def test_multi_controlled_bit_order_reversal(size: int, tgt: int):
    csize = 3
    circuit = generate_circuit_for_test(
        f"multi_controlled_bit_order_reversal_{size}",
        multi_controlled_bit_order_reversal,
        [
            ArgInfo.operates("cs", csize),
            ArgInfo.operates("tgt", size),
            ArgInfo.dirty_ancillae("aux", 0),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    for v in [0, 2**csize - 1]:
        sim = Simulator(config, circuit)
        sim.set("cs", v)
        sim.set("tgt", tgt)
        result = sim.run()
        sol = int_as_bool_array(tgt)
        if len(sol) != size:
            sol += [False] * (size - len(sol))

        assert result["m_tgt"] == (sol if v == 0 else sol[::-1])


@pytest.mark.parametrize(
    ("size", "tgt"),
    [
        (5, 3),
        (5, 31),
        (10, 42),
        (1000, 2259781389479135703987517),
    ],
)
def test_mcmx(size: int, tgt: int):
    csize = 3
    circuit = generate_circuit_for_test(
        f"mcmx_{size}",
        mcmx,
        [
            ArgInfo.operates("cs", csize),
            ArgInfo.operates("tgt", size),
            ArgInfo.dirty_ancillae("aux", 2),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    for v in [0, 2**csize - 1]:
        sim = Simulator(config, circuit)
        sim.set("cs", v)
        sim.set("tgt", tgt)
        result = sim.run()

        assert bool_array_as_int(result["m_tgt"]) == (tgt if v == 0 else 2**size - 1 - tgt)
