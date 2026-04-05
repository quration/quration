"""Example of pyqret: Trotter expansion."""

import argparse
import json
from pathlib import Path
from pprint import pprint
from typing import Literal

from pydantic import BaseModel, Field
from pyqret.frontend import Argument, CircuitBuilder, CircuitGenerator, Context, Module, Opcode, Qubits
from pyqret.frontend.gate import intrinsic
from pyqret.frontend.transforms import inline_call_inst

PauliChars = Literal["X", "Y", "Z"]


class PauliTerm(BaseModel):
    """A single Pauli string with a coefficient.

    Attributes:
        coeff (float): Coefficient
        pauli_string (dict[int, str]): Pauli string with qubit index as key and operator as value.
    """

    coeff: float = Field()
    pauli_string: dict[int, PauliChars] = Field(default_factory=dict)


class Hamiltonian(BaseModel):
    """Hamiltonian defined by Pauli strings.

    Attributes:
        num_qubits (int): The number of qubits in the Hamiltonian.
        pauli_terms (list[PauliTerm]): List of Pauli terms that make up the Hamiltonian.
    """

    num_qubits: int = Field(ge=1)
    pauli_terms: list[PauliTerm] = Field(default_factory=list)


def read_hamiltonian(file: Path) -> Hamiltonian:
    """Read Hamiltonian from a JSON file.

    Args:
        file (Path): Path to the JSON file.

    Returns:
        Hamiltonian: The Hamiltonian object.
    """
    with file.open(mode="r", encoding="utf-8") as f:
        json_obj = json.load(f)

    return Hamiltonian(**json_obj)


class PauliRotation(CircuitGenerator):
    def __init__(
        self,
        builder: CircuitBuilder,
        pauli_string: dict[int, PauliChars],
        angle: float,
        num_qubits: int,
    ) -> None:
        super().__init__(builder)
        self.pauli_string = pauli_string
        self.angle = angle
        self.num_qubits = num_qubits

    def name(self) -> str:
        serialized_pauli_string = "".join([
            f"{pauli_char}{qubit_idx}" for qubit_idx, pauli_char in self.pauli_string.items()
        ])
        return f"pauli_rotation({serialized_pauli_string}, {self.angle:.4f})"

    def arg(self) -> Argument:
        ret = Argument()
        ret.add_operates("q", self.num_qubits)
        return ret

    def logic(self, arg: Argument) -> None:  # noqa: C901
        q: Qubits = arg["q"]  # pyright: ignore[reportAssignmentType]
        for index, pauli_char in self.pauli_string.items():
            if pauli_char == "X":
                intrinsic.h(q[index])
            elif pauli_char == "Y":
                intrinsic.sdag(q[index])
                intrinsic.h(q[index])
            elif pauli_char == "Z":
                pass

        target_qubits = list(self.pauli_string.keys())
        for i in range(len(target_qubits) - 1):
            intrinsic.cx(q[target_qubits[i]], q[target_qubits[i + 1]])

        intrinsic.rz(q[target_qubits[-1]], self.angle)

        for i in reversed(range(len(target_qubits) - 1)):
            intrinsic.cx(q[target_qubits[i]], q[target_qubits[i + 1]])

        for index, pauli_char in self.pauli_string.items():
            if pauli_char == "X":
                intrinsic.h(q[index])
            elif pauli_char == "Y":
                intrinsic.h(q[index])
                intrinsic.s(q[index])
            elif pauli_char == "Z":
                pass


class TrotterStep(CircuitGenerator):
    def __init__(
        self,
        builder: CircuitBuilder,
        hamiltonian: Hamiltonian,
        step_factor: float,
    ) -> None:
        super().__init__(builder)
        self.hamiltonian = hamiltonian
        self.step_factor = step_factor
        self.step_factor = step_factor

    def name(self) -> str:
        return "trotter_step"

    def arg(self) -> Argument:
        ret = Argument()
        ret.add_operates("q", self.hamiltonian.num_qubits)
        return ret

    def logic(self, arg: Argument) -> None:
        q: Qubits = arg["q"]  # pyright: ignore[reportAssignmentType]

        def add_pauli_rotation(pauli_string: dict[int, PauliChars], angle: float) -> None:
            gen = PauliRotation(self.builder, pauli_string=pauli_string, angle=angle, num_qubits=q.size())
            circuit = gen.generate()
            circuit(q)

        for pauli_term in self.hamiltonian.pauli_terms:
            step_angle = 2 * pauli_term.coeff * self.step_factor
            add_pauli_rotation(pauli_term.pauli_string, step_angle)


class TrotterCircuit(CircuitGenerator):
    def __init__(self, builder: CircuitBuilder, hamiltonian: Hamiltonian, time: float, num_trotter_steps: int) -> None:
        super().__init__(builder)
        self.hamiltonian = hamiltonian
        self.time = time
        self.num_trotter_steps = num_trotter_steps

    def name(self) -> str:
        return f"trotter_circuit({self.num_trotter_steps})"

    def arg(self) -> Argument:
        ret = Argument()
        ret.add_operates("q", self.hamiltonian.num_qubits)
        return ret

    def logic(self, arg: Argument) -> None:
        q = arg["q"]

        def add_trotter_step() -> None:
            gen = TrotterStep(self.builder, self.hamiltonian, self.time / self.num_trotter_steps)
            circuit = gen.generate()
            circuit(q)

        for _ in range(self.num_trotter_steps):
            add_trotter_step()


def main() -> None:
    """Simulating Trotter decomposition of the specied Hamiltonian."""
    parser = argparse.ArgumentParser(description="Simulation of Trotter decomposition")
    parser.add_argument(
        "-f",
        "--file",
        type=str,
        required=True,
        help="Path to JSON file of input Hamiltonian",
    )
    parser.add_argument(
        "-t",
        "--time",
        type=float,
        default=1.0,
        help="Time for time evolution",
    )
    parser.add_argument(
        "-n",
        "--num_trotter_steps",
        type=int,
        default=1,
        help="Number of trotter steps",
    )
    parser.add_argument(
        "-o",
        "--out",
        type=str,
        default="out.json",
        help="Path to the output IR file",
    )
    parser.add_argument(
        "--inline",
        default=False,
        action="store_true",
        help="Option to enable inline expansion for all modules in the Trotter circuit.",
    )

    args = parser.parse_args()
    hamiltonian = read_hamiltonian(Path(args.file))

    # Create the circuit
    context = Context()
    module = Module("simulation_time_evolution_with_trotter", context)
    builder = CircuitBuilder(module)
    gen = TrotterCircuit(builder, hamiltonian, args.time, args.num_trotter_steps)
    circuit = gen.generate()

    # Inline expansion
    if args.inline:
        call_in_step = []
        step_circuit = module.get_circuit("trotter_step")
        for bb in step_circuit.get_ir():
            for inst in bb:
                if inst.get_opcode() == Opcode.Call:
                    call_in_step.append(inst)  # noqa: PERF401
        for i in range(len(call_in_step)):
            inline_call_inst(call_in_step[i])

        call_insts = []
        for bb in circuit.get_ir():
            for inst in bb:
                if inst.get_opcode() == Opcode.Call:
                    call_insts.append(inst)  # noqa: PERF401
        for i in range(len(call_insts)):
            inline_call_inst(call_insts[i])

    module.dump(args.out)


if __name__ == "__main__":
    main()
