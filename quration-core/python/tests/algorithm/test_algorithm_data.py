# ruff: noqa: I001
import pytest
pytest.importorskip("pyqret.algorithm.data", exc_type=ImportError)
pytest.importorskip("pyqret.algorithm.util", exc_type=ImportError)
from pyqret.algorithm.data import (
    extract_from_q_dict,
    initialize_q_dict,
    inject_into_q_dict,
    multi_controlled_qrom,
    multi_controlled_qrom_imm,
    qrom,
    qrom_imm,
    uncompute_qrom,
    uncompute_qrom_imm,
)
from pyqret.algorithm.util import write_registers
from pyqret.frontend import (
    Argument,
    CircuitBuilder,
    CircuitGenerator,
    Context,
    Module,
)
from pyqret.frontend.gate.intrinsic import measure
from pyqret.runtime import QuantumStateType, Simulator, SimulatorConfig

from pyqret import bool_array_as_int


class QDict(CircuitGenerator):
    def __init__(self, builder: CircuitBuilder, N: int, M: int, L: int) -> None:
        super().__init__(builder)
        self.N = N
        self.M = M
        self.L = L

    def name(self):
        return f"qdict_{self.N}"

    def arg(self) -> Argument:
        ret = Argument()
        ret.add_operates("address", self.N * self.L)
        ret.add_operates("value", self.M * self.L)
        ret.add_operates("key", self.N)
        ret.add_operates("input", self.M)
        ret.add_operates("output", self.M)
        ret.add_clean_ancillae("aux", self.N + max(self.N, self.M, 3))
        ret.add_outputs("meas", self.M)

        return ret

    def logic(self, arg: Argument):
        address = arg["address"]
        value = arg["value"]
        key = arg["key"]
        input = arg["input"]
        output = arg["output"]
        aux = arg["aux"]
        meas = arg["meas"]

        # Define circuit
        initialize_q_dict(self.L, address, value)
        inject_into_q_dict(self.L, address, value, key, input, aux)
        extract_from_q_dict(self.L, address, value, key, output, aux)

        for q, r in zip(output, meas):
            measure(q, r)


@pytest.mark.parametrize(
    "val",
    [5, 6, 7, 8, 9],
)
def test_qdict(val: int):
    context = Context()
    module = Module("test", context)
    builder = CircuitBuilder(module)

    qdict_gen = QDict(builder, 5, 10, 4)
    circuit = qdict_gen.generate()
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    sim = Simulator(config, circuit)
    sim.set("input", val)
    result = sim.run()
    assert bool_array_as_int(result["meas"]) == val


class QROM(CircuitGenerator):
    def __init__(self, builder: CircuitBuilder, N: int, M: int, *, C: int = 0, uncompute: bool = False) -> None:
        if uncompute and C != 0:
            msg = "C must be 0 when uncomputing"
            raise RuntimeError(msg)

        super().__init__(builder)
        self.N = N
        self.M = M
        self.C = C
        self.uncompute = uncompute

    def name(self):
        return f"qrom_{self.N}"

    def arg(self) -> Argument:
        ret = Argument()
        if self.C != 0:
            ret.add_operates("cs", self.C)
        ret.add_operates("address", self.N)
        ret.add_operates("value", self.M)
        ret.add_clean_ancillae("aux", self.N + self.C - 1)
        ret.add_inputs("registers", self.M * 2**self.N)
        ret.add_outputs("meas", self.M)

        return ret

    def get_value(self, x: int):
        return (x * 1234567 + 89012345) % 2**self.M

    def logic(self, arg: Argument):
        if self.C != 0:
            cs = arg["cs"]
        address = arg["address"]
        value = arg["value"]
        aux = arg["aux"]
        registers = arg["registers"]
        meas = arg["meas"]

        # Define circuit
        for i in range(2**self.N):
            write_registers(self.get_value(i), registers.range(i * self.M, self.M))

        if self.C != 0:
            multi_controlled_qrom(cs, address, value, aux, registers)
        else:
            qrom(address, value, aux, registers)

        if self.uncompute:
            uncompute_qrom(address, value, aux, registers)

        for q, r in zip(value, meas):
            measure(q, r)


@pytest.mark.parametrize(("size", "ind"), [(5, i) for i in range(2**5)] + [(6, i) for i in range(2**6)])
def test_qrom(size: int, ind: int):
    context = Context()
    module = Module("test", context)
    builder = CircuitBuilder(module)

    qrom_gen = QROM(builder, size, 100)
    circuit = qrom_gen.generate()
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    sim = Simulator(config, circuit)
    sim.set("address", ind)
    result = sim.run()
    assert sim.get_state().get_value("address") == ind
    assert bool_array_as_int(result["meas"]) == qrom_gen.get_value(ind)


@pytest.mark.parametrize(("size", "cval", "ind"), [(5, 0, i) for i in range(2**5)] + [(5, 7, i) for i in range(2**5)])
def test_multi_controlled_qrom(size: int, cval: int, ind: int):
    context = Context()
    module = Module("test", context)
    builder = CircuitBuilder(module)

    qrom_gen = QROM(builder, size, 100, C=3)
    circuit = qrom_gen.generate()
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    sim = Simulator(config, circuit)
    sim.set("cs", cval)
    sim.set("address", ind)
    result = sim.run()
    assert sim.get_state().get_value("address") == ind
    assert bool_array_as_int(result["meas"]) == (0 if cval == 0 else qrom_gen.get_value(ind))


@pytest.mark.parametrize(("size", "ind"), [(5, i) for i in range(2**5)] + [(6, i) for i in range(2**6)])
def test_uncompute_qrom(size: int, ind: int):
    context = Context()
    module = Module("test", context)
    builder = CircuitBuilder(module)

    qrom_gen = QROM(builder, size, 100, uncompute=True)
    circuit = qrom_gen.generate()
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=8)
    sim = Simulator(config, circuit)
    sim.set("address", ind)
    result = sim.run()
    assert sim.get_state().get_value("address") == ind
    assert bool_array_as_int(result["meas"]) == 0


class QROMImm(CircuitGenerator):
    def __init__(self, builder: CircuitBuilder, N: int, M: int, *, C: int = 0, uncompute: bool = False) -> None:
        if uncompute and C != 0:
            msg = "C must be 0 when uncomputing"
            raise RuntimeError(msg)

        super().__init__(builder)
        self.N = N
        self.M = M
        self.C = C
        self.uncompute = uncompute

    def name(self):
        return f"qrom_im_{self.N}"

    def arg(self) -> Argument:
        ret = Argument()
        if self.C != 0:
            ret.add_operates("cs", self.C)
        ret.add_operates("address", self.N)
        ret.add_operates("value", self.M)
        ret.add_clean_ancillae("aux", self.N + self.C - 1)
        ret.add_outputs("meas", self.M)

        return ret

    def get_value(self, x: int):
        return (x * 1234567 + 89012345) % 2**self.M

    def logic(self, arg: Argument):
        if self.C != 0:
            cs = arg["cs"]

        address = arg["address"]
        value = arg["value"]
        aux = arg["aux"]
        meas = arg["meas"]

        # Define circuit
        if self.C != 0:
            multi_controlled_qrom_imm(cs, address, value, aux, [self.get_value(i) for i in range(2**self.N)])
        else:
            qrom_imm(address, value, aux, [self.get_value(i) for i in range(2**self.N)])

        if self.uncompute:
            uncompute_qrom_imm(address, value, aux, [self.get_value(i) for i in range(2**self.N)])

        for q, r in zip(value, meas):
            measure(q, r)


@pytest.mark.parametrize(("size", "ind"), [(5, i) for i in range(2**5)] + [(6, i) for i in range(2**6)])
def test_qrom_imm(size: int, ind: int):
    context = Context()
    module = Module("test", context)
    builder = CircuitBuilder(module)

    qrom_imm_gen = QROMImm(builder, size, 100)
    circuit = qrom_imm_gen.generate()
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    sim = Simulator(config, circuit)
    sim.set("address", ind)
    result = sim.run()
    assert sim.get_state().get_value("address") == ind
    assert bool_array_as_int(result["meas"]) == qrom_imm_gen.get_value(ind)


@pytest.mark.parametrize(
    ("size", "cval", "ind"),
    [(5, 0, i) for i in range(2**5)] + [(5, 7, i) for i in range(2**5)],
)
def test_multi_controlled_qrom_imm(size: int, cval: int, ind: int):
    context = Context()
    module = Module("test", context)
    builder = CircuitBuilder(module)

    qrom_imm_gen = QROMImm(builder, size, 100, C=3)
    circuit = qrom_imm_gen.generate()
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    sim = Simulator(config, circuit)
    sim.set("cs", cval)
    sim.set("address", ind)
    result = sim.run()
    assert sim.get_state().get_value("address") == ind
    assert bool_array_as_int(result["meas"]) == (0 if cval == 0 else qrom_imm_gen.get_value(ind))


@pytest.mark.parametrize(("size", "ind"), [(5, i) for i in range(2**5)] + [(6, i) for i in range(2**6)])
def test_uncompute_qrom_imm(size: int, ind: int):
    context = Context()
    module = Module("test", context)
    builder = CircuitBuilder(module)

    qrom_imm_gen = QROMImm(builder, size, 100, uncompute=True)
    circuit = qrom_imm_gen.generate()
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=8)
    sim = Simulator(config, circuit)
    sim.set("address", ind)
    result = sim.run()
    assert sim.get_state().get_value("address") == ind
    assert bool_array_as_int(result["meas"]) == 0
