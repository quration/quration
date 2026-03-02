# ruff: noqa: I001
from pathlib import Path

import pytest

pytest.importorskip("pyqret.algorithm", exc_type=ImportError)
from pyqret.algorithm.arithmetic import add_craig, add_cuccaro, sub_craig
from pyqret import bool_array_as_int
from pyqret.backend import CompileOption, Compiler, ScLsFixedV0Option, OptLevel
from pyqret.frontend import Argument, CircuitBuilder, CircuitGenerator, Context, Module
from pyqret.frontend.gate.intrinsic import measure
from pyqret.runtime import QuantumStateType, Simulator, SimulatorConfig


topology_path = str(Path(__file__).parent / "plane.yaml")


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


def test_option():
    option = CompileOption(
        opt_level=OptLevel.O0,
        verbose=True,
        sc_ls_fixed_v0_option=ScLsFixedV0Option(
            topology=topology_path,
            magic_generation_period=20,
            maximum_magic_state_stock=30,
            entanglement_generation_period=40,
            maximum_entangled_state_stock=50,
            reaction_time=2,
        ),
    )

    assert OptLevel.O0 == option.opt_level
    assert option.verbose
    assert topology_path == option.sc_ls_fixed_v0_option.topology
    assert 20 == option.sc_ls_fixed_v0_option.magic_generation_period
    assert 30 == option.sc_ls_fixed_v0_option.maximum_magic_state_stock
    assert 40 == option.sc_ls_fixed_v0_option.entanglement_generation_period
    assert 50 == option.sc_ls_fixed_v0_option.maximum_entangled_state_stock
    assert 2 == option.sc_ls_fixed_v0_option.reaction_time

    print("option:", option)
    print("sc_ls_fixed_v0_option:", option.sc_ls_fixed_v0_option)

    compiler = Compiler(option)
    option2 = compiler.option

    assert option.opt_level == option2.opt_level
    assert option.verbose == option2.verbose
    assert option.sc_ls_fixed_v0_option.topology == option2.sc_ls_fixed_v0_option.topology
    assert (
        option.sc_ls_fixed_v0_option.magic_generation_period == option2.sc_ls_fixed_v0_option.magic_generation_period
    )
    assert (
        option.sc_ls_fixed_v0_option.maximum_magic_state_stock
        == option2.sc_ls_fixed_v0_option.maximum_magic_state_stock
    )
    assert (
        option.sc_ls_fixed_v0_option.entanglement_generation_period
        == option2.sc_ls_fixed_v0_option.entanglement_generation_period
    )
    assert (
        option.sc_ls_fixed_v0_option.maximum_entangled_state_stock
        == option2.sc_ls_fixed_v0_option.maximum_entangled_state_stock
    )
    assert option.sc_ls_fixed_v0_option.reaction_time == option2.sc_ls_fixed_v0_option.reaction_time


def test_option_with_no_target():
    option = CompileOption(opt_level=OptLevel.O0, verbose=True)
    with pytest.raises(RuntimeError) as _:
        option.sc_ls_fixed_v0_option
    with pytest.raises(RuntimeError) as _:
        Compiler(option)


def test_simple_compile():
    # Define context, module, and builder
    context = Context()
    module = Module("example", context)
    builder = CircuitBuilder(module)

    # Generate circuit
    adder = AdderTest(builder, 5)
    circuit = adder.generate()
    assert circuit.has_frontend()
    assert circuit.has_ir()
    assert not circuit.has_mf()

    # Compile circuit
    option = CompileOption(
        opt_level=OptLevel.O0,
        sc_ls_fixed_v0_option=ScLsFixedV0Option(topology=topology_path),
    )
    compiler = Compiler(option)
    result = compiler.compile(circuit)
    assert [
        "InitCompileInfo",
        "Mapping",
        "Routing",
        "CompileInfoWithoutTopology",
        "CompileInfoWithTopology",
    ] == result.get_run_order()
    assert circuit.has_frontend()
    assert circuit.has_ir()
    assert circuit.has_mf()

    compile_info = compiler.get_compile_info()
    assert compile_info.magic_factory_count == 4
    assert compile_info.chip_cell_count == 20 * 20 - 4

    assert isinstance(compile_info.magic_generation_period, int)
    assert isinstance(compile_info.maximum_magic_state_stock, int)
    assert isinstance(compile_info.reaction_time, int)
    assert isinstance(compile_info.runtime, int)
    assert isinstance(compile_info.runtime_without_topology, int)
    assert isinstance(compile_info.gate_count, int)
    for k, v in compile_info.count_per_gate.items():
        assert isinstance(k, str)
        assert isinstance(v, int)
    assert isinstance(compile_info.gate_depth, int)
    for x in compile_info.gate_throughput:
        assert isinstance(x, int)
    assert isinstance(compile_info.measurement_feedback_count, int)
    assert isinstance(compile_info.measurement_feedback_depth, int)
    for x in compile_info.measurement_feedback_rate:
        assert isinstance(x, int)
    assert isinstance(compile_info.runtime_estimation_measurement_feedback_count, int)
    assert isinstance(compile_info.runtime_estimation_measurement_feedback_depth, int)
    assert isinstance(compile_info.magic_state_consumption_count, int)
    assert isinstance(compile_info.magic_state_consumption_depth, int)
    for x in compile_info.magic_state_consumption_rate:
        assert isinstance(x, int)
    assert isinstance(compile_info.runtime_estimation_magic_state_consumption_count, int)
    assert isinstance(compile_info.runtime_estimation_magic_state_consumption_depth, int)
    assert isinstance(compile_info.magic_factory_count, int)
    assert isinstance(compile_info.chip_cell_count, int)
    for x in compile_info.chip_cell_algorithmic_qubit:
        assert isinstance(x, int)
    for x in compile_info.chip_cell_algorithmic_qubit_ratio:
        assert isinstance(x, float)
    for x in compile_info.chip_cell_active_qubit_area:
        assert isinstance(x, int)
    for x in compile_info.chip_cell_active_qubit_area_ratio:
        assert isinstance(x, float)
    assert isinstance(compile_info.qubit_volume, int)
    assert isinstance(compile_info.code_distance, int)
    assert isinstance(compile_info.execution_time_sec, float)
    assert isinstance(compile_info.num_physical_qubits, int)

    print(compile_info.to_json())
    print(compile_info.to_markdown())


def test_pass_list():
    assert "sc_ls_fixed_v0::mapping" in Compiler.get_available_passes()
    assert "sc_ls_fixed_v0::routing" in Compiler.get_available_passes()


def test_manual_compile():
    # Define context, module, and builder
    context = Context()
    module = Module("example", context)
    builder = CircuitBuilder(module)

    # Generate circuit
    adder = AdderTest(builder, 5)
    circuit = adder.generate()
    assert circuit.has_frontend()
    assert circuit.has_ir()
    assert not circuit.has_mf()

    # Run circuit
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    sim = Simulator(config, circuit)
    sim.set("dst", 18)
    sim.set("src", 7)
    result = sim.run()
    assert 18 + 7 == bool_array_as_int(result["m_dst"])
    assert 7 == bool_array_as_int(result["m_src"])

    # Compile circuit
    option = CompileOption(
        opt_level=OptLevel.Manual,
        sc_ls_fixed_v0_option=ScLsFixedV0Option(topology=topology_path),
    )
    compiler = Compiler(option)
    compiler.add_pass("sc_ls_fixed_v0::mapping")
    compiler.add_pass("sc_ls_fixed_v0::routing")

    assert [
        "Mapping",
        "Routing",
    ] == compiler.get_pass_list()

    result = compiler.compile(circuit)
    assert [
        "Mapping",
        "Routing",
    ] == result.get_run_order()
    assert 2 == len(result.get_elapsed_time())

    assert circuit.has_frontend()
    assert circuit.has_ir()
    assert circuit.has_mf()

    assert 3 == circuit.get_mf().num_bbs()
    for bb in circuit.get_mf():
        print(bb.num_instructions())
        for inst in bb:
            print(inst)
