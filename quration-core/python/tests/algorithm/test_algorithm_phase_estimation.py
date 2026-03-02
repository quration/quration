# ruff: noqa: I001
import pytest
pytest.importorskip("pyqret.algorithm.phase_estimation", exc_type=ImportError)
from pyqret.algorithm.phase_estimation import prepare
from pyqret.frontend import ArgInfo
from pyqret.runtime import QuantumStateType, Simulator, SimulatorConfig

from pyqret import preprocess_lcu_coefficients_for_reversible_sampling

from frontend.common import generate_circuit_for_test


def test_qpe():
    size = 3
    mask = 2**size - 1
    sub_bit_precision = 5
    lcu_coefficients = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6]
    alias_sampling = preprocess_lcu_coefficients_for_reversible_sampling(lcu_coefficients, sub_bit_precision)

    circuit = generate_circuit_for_test(
        "prepare",
        prepare,
        [
            alias_sampling,
            ArgInfo.operates("index", size),
            ArgInfo.operates("alternate_index", size),
            ArgInfo.operates("random", sub_bit_precision),
            ArgInfo.operates("switch_weight", sub_bit_precision),
            ArgInfo.operate("cmp"),
            ArgInfo.clean_ancillae("aux", size + 3),
        ],
        measure_all_operates=False,
    )

    config = SimulatorConfig(state_type=QuantumStateType.FullQuantum, seed=0)
    sim = Simulator(config, circuit)
    sim.run()

    state = sim.get_full_quantum_state()
    state_vec = state.get_state_vector()
    reduced_coef = [0.0] * (2**size)
    for i, x in enumerate(state_vec):
        reduced_coef[i & mask] += abs(x)

    print(sum(reduced_coef))
    print(reduced_coef)

    for i, coef in enumerate(lcu_coefficients):
        exp_rato = coef / lcu_coefficients[0]
        act_ratio = reduced_coef[i] / reduced_coef[0]
        assert act_ratio == pytest.approx(exp_rato, rel=0.05)
