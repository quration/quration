"""pyqret.backend module."""

from __future__ import annotations

import json
from typing import TYPE_CHECKING

from .. import IteratorWrapper
from .._qret_impl import backend as _M  # pyright: ignore[reportMissingImports]
from .._qret_impl.backend import OptLevel  # pyright: ignore[reportMissingImports]

if TYPE_CHECKING:
    from datetime import timedelta
    from pathlib import Path

    from ..frontend import Circuit


class MachineInstruction:
    """Represents instruction class of backend quantum computers."""

    def __str__(self) -> str:
        return str(self._impl)


class MachineBasicBlock:
    """Represents basic block class of backend quantum computers."""

    def num_instructions(self) -> int:
        """Get the number of instructions in this bb.

        Returns:
            int: the number of instructions.
        """
        return self._impl.num_instructions()

    def __iter__(self) -> IteratorWrapper:
        return IteratorWrapper(self._impl, MachineInstruction)


class MachineFunction:
    """Represents function class of backend quantum computers."""

    def num_bbs(self) -> int:
        """Get the number of basic blocks in this machine function.

        Returns:
            int: the number of basic blocks.
        """
        return self._impl.num_bbs()

    def __iter__(self) -> IteratorWrapper:
        return IteratorWrapper(self._impl, MachineBasicBlock)


class ScLsFixedV0Option:
    def __init__(  # noqa:PLR0913
        self,
        *,
        topology: str = "",
        use_magic_state_cultivation: bool = False,
        magic_factory_seed_offset: int = 0,
        magic_generation_period: int = 15,
        prob_magic_state_creation: float = 1.0,
        maximum_magic_state_stock: int = 10000,
        entanglement_generation_period: int = 100,
        maximum_entangled_state_stock: int = 10,
        reaction_time: int = 1,
        physical_error_rate: float = 0.0,
        drop_rate: float = 0.0,
        code_cycle_time_sec: float = 0.0,
        allowed_failure_prob: float = 0.0,
    ) -> None:
        """Compiling options for the SC_LS_FIXED_V0 Machine.

        Args:
            topology (str): path to the topology file.
            use_magic_state_cultivation (bool, optional): if true, magic state factories are simulated using the cultivation method. Defaults to False.
            magic_factory_seed_offset (int, optional): base seed offset added to each magic state factory's ID. Defaults to 0.
            magic_generation_period (int, optional): beats to generate a magic state. Defaults to 15.
            prob_magic_state_creation (float, optional): success probability of creating a magic state. Defaults to 1.0.
            maximum_magic_state_stock (int, optional): the number of magic states that can be stored in a magic state factory. Defaults to 10000.
            entanglement_generation_period (int, optional): beats to generate an entanglement pair. Defaults to 100.
            maximum_entangled_state_stock (int, optional): the number of entanglement pairs that can be stored in an entanglement factory. Defaults to 10.
            reaction_time (int, optional): beats it takes for the measured value to be error-corrected. Defaults to 1.
            physical_error_rate (float, optional): physical error rate p for logical error estimation. Defaults to 0.0.
            drop_rate (float, optional): drop rate Lambda for logical error estimation. Defaults to 0.0.
            code_cycle_time_sec (float, optional): code cycle time in seconds (t_cycle). Defaults to 0.0.
            allowed_failure_prob (float, optional): allowed failure probability eps. Defaults to 0.0.
        """
        self._impl = _M.ScLsFixedV0Option(
            topology,
            use_magic_state_cultivation,
            magic_factory_seed_offset,
            magic_generation_period,
            prob_magic_state_creation,
            maximum_magic_state_stock,
            entanglement_generation_period,
            maximum_entangled_state_stock,
            reaction_time,
            physical_error_rate,
            drop_rate,
            code_cycle_time_sec,
            allowed_failure_prob,
        )

    @property
    def topology(self) -> int:
        """Path to the topology file."""
        return self._impl.topology

    @property
    def magic_generation_period(self) -> int:
        """Beats to generate a magic state."""
        return self._impl.magic_generation_period

    @property
    def maximum_magic_state_stock(self) -> int:
        """The number of magic states that can be stored in a magic state factory."""
        return self._impl.maximum_magic_state_stock

    @property
    def entanglement_generation_period(self) -> int:
        """Beats to generate an entanglement pair."""
        return self._impl.entanglement_generation_period

    @property
    def maximum_entangled_state_stock(self) -> int:
        """The number of entanglement pairs that can be stored in an entanglement factory."""
        return self._impl.maximum_entangled_state_stock

    @property
    def reaction_time(self) -> int:
        """Beats it takes for the measured value to be error-corrected."""
        return self._impl.reaction_time

    @property
    def physical_error_rate(self) -> float:
        """Physical error rate p for logical error estimation."""
        return self._impl.physical_error_rate

    @property
    def drop_rate(self) -> float:
        """Drop rate Lambda for logical error estimation."""
        return self._impl.drop_rate

    @property
    def code_cycle_time_sec(self) -> float:
        """Code cycle time in seconds (t_cycle)."""
        return self._impl.code_cycle_time_sec

    @property
    def allowed_failure_prob(self) -> float:
        """Allowed failure probability eps."""
        return self._impl.allowed_failure_prob

    def __str__(self) -> str:
        return str(self._impl)


class CompileOption:
    def __init__(
        self,
        opt_level: OptLevel = OptLevel.O0,
        *,
        verbose: bool = False,
        sc_ls_fixed_v0_option: ScLsFixedV0Option | None = None,
    ) -> None:
        """Compiling options.

        Args:
            opt_level (OptLevel, optional): optimization level. Defaults to :obj:`OptLevel.O0`.
            verbose (bool, optional): if true, dump the process of compiling. Defaults to False.
            sc_ls_fixed_v0_option (ScLsFixedV0Option | None, optional): compiling options for the SC_LS_FIXED_V0 Machine. If not None, compile a circuit for SC_LS_FIXED_V0 Machine. Defaults to None.
        """
        self._impl = _M.CompileOption(opt_level, verbose)

        # Set compiling options for target machines
        self._compile_to_sc_ls_fixed_v0 = sc_ls_fixed_v0_option is not None
        if sc_ls_fixed_v0_option is not None:
            self._impl.sc_ls_fixed_v0_option = sc_ls_fixed_v0_option._impl

    @property
    def opt_level(self) -> bool:
        """Optimization level."""
        return self._impl.opt_level

    @property
    def verbose(self) -> bool:
        """If true, dump the process of compiling."""
        return self._impl.verbose

    @property
    def sc_ls_fixed_v0_option(self) -> ScLsFixedV0Option:
        """Compiling options for the SC_LS_FIXED_V0 Machine.

        Raises:
            RuntimeError: sc_ls_fixed_v0_option is None at the time of construction

        Returns:
            ScLsFixedV0Option: compiling options for the SC_LS_FIXED_V0 Machine
        """
        if not self._compile_to_sc_ls_fixed_v0:
            msg = "sc_ls_fixed_v0_option is None at the time of construction"
            raise RuntimeError(msg)

        ret = ScLsFixedV0Option()
        ret._impl = self._impl.sc_ls_fixed_v0_option
        return ret

    def __str__(self) -> str:
        return str(self._impl)


class CompileResult:
    """A result of compiling a circuit."""

    def get_run_order(self) -> list[str]:
        """Get the run order of the passes.

        Returns:
            list[str]: the run order.
        """
        return self._impl.get_run_order()

    def get_elapsed_time(self) -> list[timedelta]:
        """Get the elapsed time for each pass.

        The order of the elements in the list is the same as the run order of the passes.

        Returns:
            list[timedelta]: the elapsed time.
        """
        return self._impl.get_elapsed_time()


class Compiler:
    def __init__(self, option: CompileOption) -> None:
        """Compiler from IR to machine language.

        Args:
            option (CompileOption): compiling option.
        """
        self._impl = _M.Compiler(option._impl)
        self._option = option

    @staticmethod
    def get_available_passes() -> list[str]:
        """Get the list of passes available for use in the compiler.

        Returns:
            list[str]: the list of passes.
        """
        return _M.Compiler.get_available_passes()

    @property
    def option(self) -> CompileOption:
        """Compiling option."""
        return self._option

    def add_pass(self, name: str) -> None:
        """Add a pass to the compiler.

        Args:
            name (str): the name of the pass.
        """
        self._impl.add_pass(name)

    def add_external_pass(self, name: str, cmd: str, input_path: str | Path, output_path: str | Path) -> None:
        """Add an external pass to the compiler.

        Args:
            name (str): the name of the external pass.
            cmd (str): command to run
            input_path (str | Path): input file path of the command
            output_path (str | Path): output file path of the command
        """
        self._impl.add_external_pass(name, cmd, str(input_path), str(output_path))

    def get_pass_list(self) -> list[str]:
        """Get the list of passes to be run.

        Returns:
            list[str]: the list of passes.
        """
        return self._impl.get_pass_list()

    def compile(self, circuit: Circuit) -> CompileResult:
        """Compile a circuit.

        Args:
            circuit (Circuit): circuit.

        Returns:
            CompileResult: the result of compiling.
        """
        ret = CompileResult()
        ret._impl = self._impl.compile(circuit._impl)
        return ret

    def get_compile_info(self) -> ScLsFixedV0CompileInfo:
        """Get compile information.

        Returns:
            ScLsFixedV0CompileInfo: compile information
        """
        ret = ScLsFixedV0CompileInfo()
        ret._impl = self._impl.get_compile_info()
        return ret


class ScLsFixedV0CompileInfo:  # noqa:PLR0904
    """Information generated when compiling to SC_LS_FIXED_V0."""

    @property
    def magic_generation_period(self) -> int:  # noqa:D102
        return self._impl.magic_generation_period

    @property
    def maximum_magic_state_stock(self) -> int:  # noqa:D102
        return self._impl.maximum_magic_state_stock

    @property
    def reaction_time(self) -> int:  # noqa:D102
        return self._impl.reaction_time

    @property
    def runtime(self) -> int:  # noqa:D102
        return self._impl.runtime

    @property
    def runtime_without_topology(self) -> int:  # noqa:D102
        return self._impl.runtime_without_topology

    @property
    def gate_count(self) -> int:  # noqa:D102
        return self._impl.gate_count

    @property
    def count_per_gate(self) -> dict[str, int]:  # noqa:D102
        return self._impl.count_per_gate()

    @property
    def gate_depth(self) -> int:  # noqa:D102
        return self._impl.gate_depth

    @property
    def gate_throughput(self) -> list[int]:  # noqa:D102
        return self._impl.gate_throughput

    @property
    def measurement_feedback_count(self) -> int:  # noqa:D102
        return self._impl.measurement_feedback_count

    @property
    def measurement_feedback_depth(self) -> int:  # noqa:D102
        return self._impl.measurement_feedback_depth

    @property
    def measurement_feedback_rate(self) -> list[int]:  # noqa:D102
        return self._impl.measurement_feedback_rate

    @property
    def runtime_estimation_measurement_feedback_count(self) -> int:  # noqa:D102
        return self._impl.runtime_estimation_measurement_feedback_count

    @property
    def runtime_estimation_measurement_feedback_depth(self) -> int:  # noqa:D102
        return self._impl.runtime_estimation_measurement_feedback_depth

    @property
    def magic_state_consumption_count(self) -> int:  # noqa:D102
        return self._impl.magic_state_consumption_count

    @property
    def magic_state_consumption_depth(self) -> int:  # noqa:D102
        return self._impl.magic_state_consumption_depth

    @property
    def magic_state_consumption_rate(self) -> list[int]:  # noqa:D102
        return self._impl.magic_state_consumption_rate

    @property
    def runtime_estimation_magic_state_consumption_count(self) -> int:  # noqa:D102
        return self._impl.runtime_estimation_magic_state_consumption_count

    @property
    def runtime_estimation_magic_state_consumption_depth(self) -> int:  # noqa:D102
        return self._impl.runtime_estimation_magic_state_consumption_depth

    @property
    def magic_factory_count(self) -> int:  # noqa:D102
        return self._impl.magic_factory_count

    @property
    def chip_cell_count(self) -> int:  # noqa:D102
        return self._impl.chip_cell_count

    @property
    def chip_cell_algorithmic_qubit(self) -> list[int]:  # noqa:D102
        return self._impl.chip_cell_algorithmic_qubit

    @property
    def chip_cell_algorithmic_qubit_ratio(self) -> list[float]:  # noqa:D102
        return self._impl.chip_cell_algorithmic_qubit_ratio

    @property
    def chip_cell_active_qubit_area(self) -> list[int]:  # noqa:D102
        return self._impl.chip_cell_active_qubit_area

    @property
    def chip_cell_active_qubit_area_ratio(self) -> list[float]:  # noqa:D102
        return self._impl.chip_cell_active_qubit_area_ratio

    @property
    def qubit_volume(self) -> int:  # noqa:D102
        return self._impl.qubit_volume

    @property
    def code_distance(self) -> int:  # noqa:D102
        return self._impl.code_distance

    @property
    def execution_time_sec(self) -> float:  # noqa:D102
        return self._impl.execution_time_sec

    @property
    def num_physical_qubits(self) -> int:  # noqa:D102
        return self._impl.num_physical_qubits

    def to_json(self) -> dict:  # noqa:D102
        j = self._impl.to_json()
        return json.loads(j)

    def to_markdown(self) -> str:  # noqa:D102
        return self._impl.to_markdown()
