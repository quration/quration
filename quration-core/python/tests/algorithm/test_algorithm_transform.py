# ruff: noqa: I001
import pytest
pytest.importorskip("pyqret.algorithm.transform", exc_type=ImportError)
from pyqret.algorithm.transform import fourier_transform
from pyqret.frontend import (
    Argument,
    CircuitBuilder,
    CircuitGenerator,
    Context,
    Module,
)
from pyqret.frontend.gate.intrinsic import h, measure
from pyqret.runtime import QuantumStateType, Simulator, SimulatorConfig

from pyqret import bool_array_as_int


class QFT(CircuitGenerator):
    def __init__(self, builder: CircuitBuilder, N: int) -> None:
        super().__init__(builder)
        self.N = N

    def name(self):
        return f"qft_{self.N}"

    def arg(self) -> Argument:
        ret = Argument()
        ret.add_operates("tgt", self.N)
        ret.add_outputs("m_tgt", self.N)

        return ret

    def logic(self, arg: Argument):
        tgt = arg["tgt"]
        m_tgt = arg["m_tgt"]

        # Define circuit
        for q in tgt:
            h(q)
        fourier_transform(tgt)
        for q, r in zip(tgt, m_tgt):
            measure(q, r)


def test_qft():
    context = Context()
    module = Module("test", context)
    builder = CircuitBuilder(module)

    qft = QFT(builder, 5)
    circuit = qft.generate()
    config = SimulatorConfig(state_type=QuantumStateType.FullQuantum, seed=0)
    sim = Simulator(config, circuit)
    result = sim.run()
    assert bool_array_as_int(result["m_tgt"]) == 0
