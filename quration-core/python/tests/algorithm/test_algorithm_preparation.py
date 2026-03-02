# ruff: noqa: I001
import math
import random
from collections import Counter

import pytest
pytest.importorskip("pyqret.algorithm.preparation", exc_type=ImportError)
from pyqret.algorithm.preparation import (
    controlled_prepare_uniform_superposition,
    prepare_arbitrary_state,
    prepare_uniform_superposition,
)
from pyqret.frontend import ArgInfo
from pyqret.runtime import QuantumStateType, Simulator, SimulatorConfig

from frontend.common import generate_circuit_for_test


@pytest.mark.parametrize("size", [2, 3, 4, 5])
def test_prepare_arbitrary_state(size: int):
    num_samples = 100_000
    precision = 10

    probs = [random.uniform(0.1, 1.0) for _ in range(2**size)]
    norm = sum(probs)
    probs = [x / norm for x in probs]
    entropy = 0.0
    for p in probs:
        entropy += -p * math.log(p)

    circuit = generate_circuit_for_test(
        f"prepare_arbitrary_state_{size}",
        prepare_arbitrary_state,
        [
            probs,
            ArgInfo.operates("tgt", size),
            ArgInfo.operates("theta", precision),
            ArgInfo.clean_ancillae("aux", size - 1),
        ],
        measure_all_operates=False,
    )
    config = SimulatorConfig(state_type=QuantumStateType.FullQuantum, seed=0)
    sim = Simulator(config, circuit)
    _ = sim.run()
    state = sim.get_state()

    assert state.get_entropy() == pytest.approx(entropy, rel=0.1)

    samples = state.sampling(num_samples)
    count = Counter(samples)
    for k, num in count.items():
        mi = num_samples * probs[k] / 2
        ma = num_samples * probs[k] * 2
        assert 0 <= k
        assert k < 2**size
        assert mi <= num
        assert num <= ma


@pytest.mark.parametrize(
    ("size", "N"),
    [(5, i) for i in range(17, 32)],
)
def test_prepare_uniform_superposition(size: int, N: int):
    num_samples = 100_000
    circuit = generate_circuit_for_test(
        f"prepare_uniform_superposition_{size}",
        prepare_uniform_superposition,
        [
            N,
            ArgInfo.operates("tgt", size),
            ArgInfo.clean_ancillae("aux", 2 * size),
        ],
        measure_all_operates=False,
    )
    config = SimulatorConfig(state_type=QuantumStateType.FullQuantum, seed=0)
    sim = Simulator(config, circuit)
    _ = sim.run()
    state = sim.get_state()

    assert state.get_entropy() == pytest.approx(math.log(N))

    samples = state.sampling(num_samples)
    count = Counter(samples)
    for k, num in count.items():
        mi = num_samples / N / 2
        ma = num_samples / N * 2
        assert 0 <= k
        assert k < N
        assert mi <= num
        assert num <= ma


@pytest.mark.parametrize(
    ("size", "v", "N"),
    [(5, 0, i) for i in range(17, 32)] + [(5, 1, i) for i in range(17, 32)],
)
def test_controlled_prepare_uniform_superposition(size: int, v: int, N: int):
    num_samples = 100_000
    circuit = generate_circuit_for_test(
        f"controlled_prepare_uniform_superposition_{size}",
        controlled_prepare_uniform_superposition,
        [
            N,
            ArgInfo.operate("c"),
            ArgInfo.operates("tgt", size),
            ArgInfo.clean_ancillae("aux", 2 * size),
        ],
        measure_all_operates=False,
    )
    config = SimulatorConfig(state_type=QuantumStateType.FullQuantum, seed=0)

    sim = Simulator(config, circuit)
    sim.set("c", v)
    _ = sim.run()

    if v == 0:
        state = sim.get_state()
        assert state.get_entropy() == pytest.approx(0)
        samples = state.sampling(10)
        assert [0] * 10 == samples

    else:
        state = sim.get_state()
        assert state.get_entropy() == pytest.approx(math.log(N))

        samples = state.sampling(num_samples)
        count = Counter(samples)
        for k, num in count.items():
            mi = num_samples / N / 2
            ma = num_samples / N * 2

            # test the types of keys that are sampled
            assert 0 <= (k // 2)
            assert (k // 2) < N

            # test the number of times a key is sampled
            assert mi <= num
            assert num <= ma
