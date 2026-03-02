"""pyqret.runtime module provides simulators to run IR circuits."""

from __future__ import annotations

from abc import ABC, abstractmethod
from typing import TYPE_CHECKING

from .. import BigInt
from .._qret_impl import runtime as _M  # pyright: ignore[reportMissingImports]
from .._qret_impl.runtime import QuantumStateType  # pyright: ignore[reportMissingImports]

if TYPE_CHECKING:
    import numpy as np
    from numpy.typing import NDArray

    from ..frontend import Circuit


class SimulatorConfig:
    def __init__(
        self,
        state_type: QuantumStateType = QuantumStateType.FullQuantum,
        seed: int = 0,
        max_superpositions: int = 1,
        *,
        save_operation_matrix: bool = False,
    ) -> None:
        """Config of simulator.

        Args:
            state_type (QuantumStateType): type of a quantum state which simulator uses.
            seed (int): seed.
            max_superpositions (int): the maximum number of superpositions.
                This parameter is only used when `state_type` is Toffoli.
            save_operation_matrix (bool): Save operation matrix of circuit.
                This parameter is only used when `state_type` is FullQuantum.
        """
        self._impl = _M.SimulatorConfig(state_type, seed, max_superpositions, save_operation_matrix)

    @property
    def state_type(self) -> QuantumStateType:
        """Type of a quantum state which simulator uses."""
        return self._impl.state_type

    @property
    def seed(self) -> int:
        """Seed."""
        return self._impl.seed

    @property
    def max_superpositions(self) -> int:
        """The maximum number of superpositions."""
        return self._impl.max_superpositions

    @property
    def save_operation_matrix(self) -> bool:
        """Flag of saving operation matrix."""
        return self._impl.save_operation_matrix


class QuantumState(ABC):
    """Base class of quantum states."""

    @abstractmethod
    def get_num_superpositions(self) -> int:
        """Get the number of superpositions in Z basis.

        Returns:
            int: the number of superpositions.
        """

    @abstractmethod
    def is_computational_basis(self) -> bool:
        """Return true if the quantum state is in the computational basis.

        self.is_computational_basis() == not self.is_superposed()

        Returns:
            bool: true if the quantum state is in the computational basis.
        """

    @abstractmethod
    def is_superposed(self, q: int) -> bool:
        """Return true if the qubit is in superposition.

        If the quantum state is `|0011> + |1010>`, the followings are satisfied:

        * state.is_superposed(0) == True
        * state.is_superposed(1) == False
        * state.is_superposed(2) == False
        * state.is_superposed(3) == True

        Args:
            q (int): the index of qubit.

        Returns:
            bool: true if the qubit is in superposition.
        """

    @abstractmethod
    def calc_0_prob(self, q: int) -> float:
        """Calculate the probability of the qubit being ``|0>``.

        Args:
            q (int): the index of qubit.

        Returns:
            float: the probability of the qubit being ``|0>``
        """

    @abstractmethod
    def calc_1_prob(self, q: int) -> float:
        """Calculate the probability of the qubit being ``|1>``.

        Args:
            q (int): the index of qubit.

        Returns:
            float: the probability of the qubit being ``|1>``
        """


class ToffoliState(QuantumState):
    """A quantum state class that efficiently simulates X, CX, CCX instructions.

    ToffoliState is a quantum state class that efficiently simulates instructions that can be
    calculated over the computational basis.
    However, instructions that create superpositions like arbitrary rotations except Hadamard cannot be simulated.
    If you use Hadamard instructions, you need to set the :obj:`SimulatorConfig` 's max_superpositions value to 2 or
    more.

    The instructions that can be simulated by the ToffoliState class are as follows:

    +-------------------------------+------------------------------+
    | Measurement instructions      | Measurement                  |
    +-------------------------------+------------------------------+
    | Unary instructions            | X, Y, Z, H, S, SDag, T, TDag |
    +-------------------------------+------------------------------+
    | Binary instructions           | CX, CY, CZ                   |
    +-------------------------------+------------------------------+
    | Ternary instructions          | CCX, CCY, CCZ                |
    +-------------------------------+------------------------------+
    | Multi-controlled instructions | MCX                          |
    +-------------------------------+------------------------------+
    """

    def get_num_superpositions(self) -> int:  # noqa:D102
        return self._impl.get_num_superpositions()

    def is_computational_basis(self) -> bool:  # noqa:D102
        return self._impl.is_computational_basis()

    def is_superposed(self, q: int) -> bool:  # noqa:D102
        return self._impl.is_superposed(q)

    def calc_0_prob(self, q: int) -> float:  # noqa:D102
        return self._impl.calc_0_prob(q)

    def calc_1_prob(self, q: int) -> float:  # noqa:D102
        return self._impl.calc_1_prob(q)

    def get_global_phase(self) -> complex:
        """Get the global phase of the quantum state.

        Note:
            This method raises error if the quantum state is not computational basis.

        Returns:
            complex: the global phase.
        """
        return self._impl.get_global_phase()

    def get_value(self, name: str) -> int:
        """Get the value of qubits in computational basis.

        For example, if ``foo`` is a name of 5-qubits and its state is ``|01101>``,
        calling this method returns 22 (2 + 4 + 16).

        Note:
            This method raises error if the quantum state is not computational basis.

        Args:
            name (str): the name of qubits.

        Returns:
            int: the value of qubits.
        """
        start, size = self._circuit.argument._get_start_and_size(name)
        return self._impl.get_value(start, size).to_int()

    def get_raw_vector(self) -> list[tuple[complex, list[int]]]:
        """Get the raw vector of the quantum state.

        Returns:
            list[tuple[complex, list[int]]]: list of tuple of (phase, computational basis).
        """
        return self._impl.get_raw_vector()


class FullQuantumState(QuantumState):
    """A quantum state class that simulates all instructions.

    This class consumes approximately ``2^N`` memory if the number of qubits is ``N``.
    """

    def get_num_superpositions(self) -> int:  # noqa:D102
        return self._impl.get_num_superpositions()

    def is_computational_basis(self) -> bool:  # noqa:D102
        return self._impl.is_computational_basis()

    def is_superposed(self, q: int) -> bool:  # noqa:D102
        return self._impl.is_superposed(q)

    def calc_0_prob(self, q: int) -> float:  # noqa:D102
        return self._impl.calc_0_prob(q)

    def calc_1_prob(self, q: int) -> float:  # noqa:D102
        return self._impl.calc_1_prob(q)

    def get_entropy(self) -> float:
        """Get the entropy of the probability distribution when measured on the Z basis.

        Returns:
            float: the entropy.
        """
        return self._impl.get_entropy()

    def sampling(self, num_samples: int) -> list[int]:
        """Measure and sample all the qubits on Z-basis as many times as given by the argument.

        Args:
            num_samples (int): the number of samples.

        Returns:
            list[int]: a list of integers converted from the resulting binaries.
        """
        samples = self._impl.sampling(num_samples)
        ret = []
        for s in samples:
            tmp = BigInt()
            tmp._impl = s
            ret.append(s)
        return ret

    def get_state_vector(self) -> list[complex]:
        """Get the state vector.

        If the number of qubits is 3, this method returns
        [the coef of x for x in [``|000>``, ``|001>``, ``|010>``, ``|011>``, ``|100>``, ``|101>``, ``|110>``, ``|111>``]].

        Returns:
            list[complex]: the state vector.
        """
        return self._impl.get_state_vector()

    def get_operation_matrix(self) -> NDArray[np.complex128]:
        """Get the operation matrix.

        Returns:
            numpy.ndarray: the operation matrix.
        """
        return self._impl.get_operation_matrix()


class Simulator:
    def __init__(self, config: SimulatorConfig, circuit: Circuit) -> None:
        """Simulator of a circuit.

        Args:
            config (SimulatorConfig): config.
            circuit (Circuit): circuit to simulate.
        """
        self._impl = _M.Simulator(config._impl, circuit._impl)
        self._circuit = circuit

    def get_config(self) -> SimulatorConfig:
        """Get the config.

        Returns:
            SimulatorConfig: config.
        """
        ret = SimulatorConfig()
        ret._impl = self._impl.get_config()
        return ret

    def set(self, name: str, value: int) -> None:
        """Initialize qubits before running simulator.

        When a simulator is constructed, all qubits are initialized in the ``|0>`` state.
        If you use this method, you can initialize some qubits in ``|1>`` states.

        For example, if ``foo`` is a name of 5 qubits, calling this method with args (``foo``, 21)
        initializes ``foo[0]``, ``foo[2]``, ``foo[4]`` to ``|1>`` (21 = 1 + 4 + 16).

        Args:
            name (str): the name of qubits.
            value (int): the initialized value.
        """
        self._impl.set(name, BigInt(value)._impl)

    def run(self) -> dict[str, list[bool]]:
        """Run the simulator.

        Returns:
            dict[str, list[bool]]: key: register name, value: measured results.
        """
        return self._impl.run()

    def get_state(self) -> ToffoliState | FullQuantumState:
        """Get the quantum state.

        When state_type is :obj:`QuantumStateType.Toffoli`, returns a :obj:`ToffoliState`,
        and when state_type is :obj:`QuantumStateType.FullQuantum`, returns a :obj:`FullQuantumState`.

        Returns:
            ToffoliState | FullQuantumState: the quantum state.
        """  # noqa: DOC501
        tmp = self._impl.get_state()
        if isinstance(tmp, _M.ToffoliState):
            ret = ToffoliState()
            ret._impl = tmp
        elif isinstance(tmp, _M.FullQuantumState):
            ret = FullQuantumState()
            ret._impl = tmp
        else:
            msg = "Simulator has unknown state"
            raise TypeError(msg)
        ret._circuit = self._circuit
        return ret

    def get_toffoli_state(self) -> ToffoliState:
        """Get :obj:`ToffoliState` .

        Raises:
            RuntimeError: When :obj:`SimulatorConfig.state_type` is not :obj:`QuantumStateType.Toffoli` .

        Returns:
            FullQuantumState: the full quantum state.
        """
        if self.get_config().state_type != QuantumStateType.Toffoli:
            msg = "set Toffoli in SimulatorConfig.state_type"
            raise RuntimeError(msg)
        ret = ToffoliState()
        ret._impl = self._impl.get_toffoli_state()
        ret._circuit = self._circuit
        return ret

    def get_full_quantum_state(self) -> FullQuantumState:
        """Get :obj:`FullQuantumState` .

        Raises:
            RuntimeError: When :obj:`SimulatorConfig.state_type` is not :obj:`QuantumStateType.FullQuantum` .

        Returns:
            FullQuantumState: the full quantum state.
        """
        if self.get_config().state_type != QuantumStateType.FullQuantum:
            msg = "set FullQuantum in SimulatorConfig.state_type"
            raise RuntimeError(msg)
        ret = FullQuantumState()
        ret._impl = self._impl.get_full_quantum_state()
        ret._circuit = self._circuit
        return ret
