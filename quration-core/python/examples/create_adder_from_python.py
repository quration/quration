"""Examples of pyqret: create adder."""

from pathlib import Path

from pyqret import bool_array_as_int
from pyqret.algorithm.arithmetic import add_craig
from pyqret.backend import CompileOption, Compiler, OptLevel, ScLsFixedV0Option
from pyqret.frontend import Argument, CircuitBuilder, CircuitGenerator, Context, Module, Qubits, Registers
from pyqret.frontend.gate.intrinsic import measure
from pyqret.runtime import QuantumStateType, Simulator, SimulatorConfig


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
        add_craig(dst, src, aux)
        for q, r in zip(dst, m_dst):
            measure(q, r)
        for q, r in zip(src, m_src):
            measure(q, r)


# Define context, module, and builder
context = Context()
module = Module("example", context)
builder = CircuitBuilder(module)

adder = Adder(builder, 5)
print("==============================")
print("Generate circuit")
circuit = adder.generate()

print("==============================")
print("Dump circuit")
for i, bb in enumerate(circuit.get_ir()):
    print(f"bb{i}", bb.name())
    for inst in bb:
        print("  ", inst, f"({inst.get_opcode()}, {type(inst)}, {inst.get_metadata()})")

print("Dump control flow graph of adder")
filename = "cfg_adder.dot"
cfg = circuit.get_ir().gen_cfg()
with Path.open(Path(filename), "w") as ofile:
    ofile.write(cfg)

print(builder.get_circuit_list())
print("Dump control flow graph of UncomputeTemporalAnd")
filename = "cfg_uncompute_temporal_and.dot"
cfg = builder.get_circuit("UncomputeTemporalAnd").get_ir().gen_cfg()
with Path.open(Path(filename), "w") as ofile:
    ofile.write(cfg)

print("Dump call graph")
filename = "call_graph.dot"
cg = circuit.get_ir().gen_call_graph(display_num_calls=True)
with Path.open(Path(filename), "w") as ofile:
    ofile.write(cg)

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

print("==============================")
print("Compile circuit")
print("Available passes:", Compiler.get_available_passes())
option = CompileOption(
    opt_level=OptLevel.O0,
    sc_ls_fixed_v0_option=ScLsFixedV0Option(topology=str(Path(__file__).parent / "data/plane.yaml")),
)
compiler = Compiler(option)
print("Pass:", compiler.get_pass_list())
result = compiler.compile(circuit)
assert circuit.has_mf()

print("==============================")
print("Compiled circuit")
print("num_bbs =", circuit.get_mf().num_bbs())
for i, bb in enumerate(circuit.get_mf()):
    print(f"bb{i}")
    for inst in bb:
        print(" ", inst)
for pas, time in zip(result.get_run_order(), result.get_elapsed_time()):
    print(pas, ":", time)

print("Compile information")
compile_info = compiler.get_compile_info()
print(compile_info.to_markdown())
