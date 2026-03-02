"""Examples of pyqret: create adder."""

from pathlib import Path

from pyqret import bool_array_as_int
from pyqret.frontend import Argument, CircuitBuilder, CircuitGenerator, Context, Module, Qubits, Registers
from pyqret.frontend.gate import intrinsic
from pyqret.runtime import QuantumStateType, Simulator, SimulatorConfig

# Define context and load module defining boolean circuits.
context = Context()
boolean_module = context.load(str(Path(__file__).parent / "boolean.json"))
print(f"boolean_module contains: {boolean_module.get_circuit_list()}")

# Define module and builder.
module = Module("craig", context)
builder = CircuitBuilder(module)
builder.load_module(boolean_module)

temporal_and = builder.get_circuit("TemporalAnd")
uncompute_temporal_and = builder.get_circuit("UncomputeTemporalAnd")


# Build CraigAdder using loaded modules.
class Adder(CircuitGenerator):
    def __init__(self, builder: CircuitBuilder, N: int) -> None:
        super().__init__(builder)
        self.N = N

    def name(self) -> str:
        return f"add_{self.N}"

    def arg(self) -> Argument:
        ret = Argument()
        ret.add_operates("dst", self.N)
        ret.add_operates("src", self.N)
        ret.add_clean_ancillae("aux", self.N - 1)
        ret.add_outputs("m_dst", self.N)
        ret.add_outputs("m_src", self.N)

        return ret

    def logic(self, arg: Argument) -> None:
        dst: Qubits = arg["dst"]  # pyright: ignore[reportAssignmentType]
        src: Qubits = arg["src"]  # pyright: ignore[reportAssignmentType]
        aux: Qubits = arg["aux"]  # pyright: ignore[reportAssignmentType]
        m_dst: Registers = arg["m_dst"]  # pyright: ignore[reportAssignmentType]
        m_src: Registers = arg["m_src"]  # pyright: ignore[reportAssignmentType]

        # Define circuit
        temporal_and(aux[0], src[0], dst[0])
        for i in range(1, self.N - 1):
            intrinsic.cx(src[i], aux[i - 1])
            intrinsic.cx(dst[i], aux[i - 1])
            temporal_and(aux[i], src[i], dst[i])
            intrinsic.cx(aux[i], aux[i - 1])
        intrinsic.cx(dst[self.N - 1], aux[self.N - 2])
        for i in range(self.N - 2, 0, -1):
            intrinsic.cx(aux[i], aux[i - 1])
            uncompute_temporal_and(aux[i], src[i], dst[i])
            intrinsic.cx(src[i], aux[i - 1])
        uncompute_temporal_and(aux[0], src[0], dst[0])
        for i in range(self.N):
            intrinsic.cx(dst[i], src[i])

        # Measure all operate qubits
        for q, r in zip(dst, m_dst):
            intrinsic.measure(q, r)
        for q, r in zip(src, m_src):
            intrinsic.measure(q, r)


adder = Adder(builder, 5)
print("Generate circuit")
circuit = adder.generate()

print("==============================")
print("Dump circuit")
for i, bb in enumerate(circuit.get_ir()):
    print(f"bb{i}", bb.name())
    for inst in bb:
        print("  ", inst, f"({inst.get_opcode()}, {type(inst)}, {inst.get_metadata()})")

print("==============================")
print("Run circuit")
config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)

sim = Simulator(config, circuit)
sim.set("dst", 18)
sim.set("src", 7)
result = sim.run()
print("18 + 7 ==", bool_array_as_int(result["m_dst"]))
print("7 ==", bool_array_as_int(result["m_src"]))

state = sim.get_state()
print(sim.get_config())
print(state.get_raw_vector())
