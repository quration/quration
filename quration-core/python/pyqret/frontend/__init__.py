"""pyqret.frontend module."""

from __future__ import annotations

import json
from abc import ABC, abstractmethod
from typing import TYPE_CHECKING

from typing_extensions import Self

from .. import IteratorWrapper
from .._qret_impl import frontend as _M  # pyright: ignore[reportMissingImports]
from .._qret_impl.frontend import Opcode, QuantumAttribute, QuantumType  # pyright: ignore[reportMissingImports]

if TYPE_CHECKING:
    from ..backend import MachineFunction

######################################################
# frontend.cpp
######################################################


class CircuitBuilder:
    def __init__(self, m: Module) -> None:
        """Builder of circuit.

        Args:
            m (Module): Module.
                        When you create a circuit using this builder, the IR of the created circuit is stored in this module.
        """
        self._impl = _M.CircuitBuilder(m._impl)
        self._arg_dict: dict[str, Argument] = {}

    def load_module(self, module: Module) -> None:
        """Load module into circuit builder.

        Args:
            module (Module): the module to load
        """
        self._impl._load_module(module._impl)

    def contains(self, name: str) -> bool:
        """Return true if the circuit named `name` is created by this builder.

        Args:
            name (str): the name of the circuit

        Returns:
            bool: true if the circuit named `name` is created by this builder.
        """
        return self._impl.contains(name)

    def get_circuit(self, name: str) -> Circuit:
        """Get the circuit created by this builder.

        Args:
            name (str): the name of the circuit.

        Raises:
            RuntimeError: if the circuit named `name` is not found.

        Returns:
            Circuit: the circuit.
        """
        if not self.contains(name):
            msg = f"unknown name: {name}"
            raise RuntimeError(msg)

        ret = Circuit()
        ret._impl = self._impl.get_circuit(name)
        # If the circuit is created on the Python side, overwrite the argument
        if name in self._arg_dict:
            ret._impl.argument = self._arg_dict[name]._impl
        return ret

    def get_circuit_list(self) -> list[str]:
        """Get the list of circuits created by this builder.

        Returns:
            list[str]: list of circuit names.
        """
        return self._impl.get_circuit_list()

    def parse_openqasm2(self, ipath: str, name: str) -> Circuit:
        """Parse OpenQASM2 file and construct circuit.

        Args:
            ipath (str): path to OpenQASM2 file
            name (str): the name of circuit to construct

        Returns:
            Circuit: constructed circuit
        """
        ret = Circuit()
        ret._impl = self._impl.parse_openqasm2(ipath, name)
        return ret

    def parse_openqasm3(self, ipath: str, name: str) -> Circuit:
        """Parse OpenQASM3 file and construct circuit.

        Args:
            ipath (str): path to OpenQASM3 file
            name (str): the name of circuit to construct

        Returns:
            Circuit: constructed circuit
        """
        ret = Circuit()
        ret._impl = self._impl.parse_openqasm3(ipath, name)
        return ret

    def _create_circuit(self, name: str, arg: Argument) -> Circuit:
        self._arg_dict[name] = arg
        ret = Circuit()
        ret._impl = self._impl._create_circuit(name, arg._impl)
        return ret

    def _begin_circuit_definition(self, circuit: Circuit) -> None:
        self._impl._begin_circuit_definition(circuit._impl)

    def _end_circuit_definition(self, circuit: Circuit) -> None:
        self._impl._end_circuit_definition(circuit._impl)


class QuantumObject(ABC):
    """Base class of quantum objects.

    Quantum objects are used to define quantum circuits.
    In pyqret, all gates and circuits take :class:`QuantumObject` as arguments.

    There are four types of :class:`QuantumObject`.

    +--------------------+------------------------------+
    | :class:`Qubit`     | A single qubit               |
    +--------------------+------------------------------+
    | :class:`Qubits`    | An array of qubits           |
    +--------------------+------------------------------+
    | :class:`Register`  | A single register            |
    +--------------------+------------------------------+
    | :class:`Registers` | An array of registers        |
    +--------------------+------------------------------+
    """

    @abstractmethod
    def is_qubit(self) -> bool:
        """Is the quantum object a qubit or not.

        Returns:
            bool: true if the quantum object is a qubit.
        """

    @abstractmethod
    def is_register(self) -> bool:
        """Is the quantum object a register or not.

        Returns:
            bool: true if the quantum object is a register.
        """

    @abstractmethod
    def size(self) -> int:
        """The number of qubits or registers.

        If the object is a :class:`Qubit` or :class:`Register`, this method returns 1.

        Returns:
            int: the number of qubits or registers.
        """

    @abstractmethod
    def __str__(self) -> str:
        pass


class Qubit(QuantumObject):
    """A single qubit."""

    def is_qubit(self) -> bool:  # noqa:D102,PLR6301
        return True

    def is_register(self) -> bool:  # noqa:D102,PLR6301
        return False

    def size(self) -> int:  # noqa:D102,PLR6301
        return 1

    def get_id(self) -> int:
        """Get the id of the qubit.

        Returns:
            int: the id.
        """
        return self._impl.get_id()

    def __str__(self) -> str:
        return str(self._impl)


class Qubits(QuantumObject):
    """An array of qubits."""

    def is_qubit(self) -> bool:  # noqa:D102,PLR6301
        return True

    def is_register(self) -> bool:  # noqa:D102,PLR6301
        return False

    def size(self) -> int:  # noqa:D102
        return self._impl.size()

    def range(self, start: int, size: int) -> Qubits:
        """Get a sub-array of the qubits.

        Args:
            start (int): the index of the first qubit.
            size (int): the number of qubits in the sub-array.

        Examples:
            >>> qubits
                {q0,q1,q2,q3,q4,q5,q6,q7,q8,q9}
            >>> qubits.range(0, 2)
                {q0,q1}
            >>> qubits.range(1, 1)
                {q1}
            >>> qubits.range(3, 3)
                {q3,q4,q5}

        Returns:
            Qubits: sub-array of the qubits.
        """
        ret = Qubits()
        ret._impl = self._impl.range(start, size)
        return ret

    def __getitem__(self, idx: int) -> Qubit:
        ret = Qubit()
        ret._impl = self._impl[idx]
        return ret

    def __add__(self, rhs: Qubit | Qubits) -> Qubits:
        ret = Qubits()
        ret._impl = self._impl + rhs._impl  # pyright: ignore[reportOperatorIssue]
        return ret

    def __radd__(self, lhs: Qubit) -> Qubits:
        ret = Qubits()
        ret._impl = lhs._impl + self._impl
        return ret

    def __iadd__(self, rhs: Qubit | Qubits) -> Self:
        self._impl += rhs._impl  # pyright: ignore[reportOperatorIssue]
        return self

    def __iter__(self):  # noqa:ANN204
        return IteratorWrapper(self._impl, Qubit)

    def __str__(self) -> str:
        return str(self._impl)


class Register(QuantumObject):
    """A single register."""

    def is_qubit(self) -> bool:  # noqa:D102,PLR6301
        return False

    def is_register(self) -> bool:  # noqa:D102,PLR6301
        return True

    def size(self) -> int:  # noqa:D102,PLR6301
        return 1

    def get_id(self) -> int:
        """Get the id of the register.

        Returns:
            int: the id.
        """
        return self._impl.get_id()

    def __str__(self) -> str:
        return str(self._impl)


class Registers(QuantumObject):
    """An array of registers."""

    def is_qubit(self) -> bool:  # noqa:D102,PLR6301
        return False

    def is_register(self) -> bool:  # noqa:D102,PLR6301
        return True

    def size(self) -> int:  # noqa:D102
        return self._impl.size()

    def range(self, start: int, size: int) -> Registers:
        """Get a sub-array of the registers.

        Args:
            start (int): the index of the first register.
            size (int): the number of registers in the sub-array.

        Examples:
            >>> registers
                {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9}
            >>> registers.range(0, 2)
                {r0,r1}
            >>> registers.range(1, 1)
                {r1}
            >>> registers.range(3, 3)
                {r3,r4,r5}

        Returns:
            Registers: sub-array of the registers.
        """
        ret = Registers()
        ret._impl = self._impl.range(start, size)
        return ret

    def __getitem__(self, idx: int) -> Register:
        ret = Register()
        ret._impl = self._impl[idx]
        return ret

    def __add__(self, rhs: Register | Registers) -> Registers:
        ret = Registers()
        ret._impl = self._impl + rhs._impl  # pyright: ignore[reportOperatorIssue]
        return ret

    def __radd__(self, lhs: Register) -> Registers:
        ret = Registers()
        ret._impl = lhs._impl + self._impl
        return ret

    def __iadd__(self, rhs: Register | Registers) -> Self:
        self._impl += rhs._impl  # pyright: ignore[reportOperatorIssue]
        return self

    def __iter__(self):  # noqa:ANN204
        return IteratorWrapper(self._impl, Register)

    def __str__(self) -> str:
        return str(self._impl)


# TODO: Implement following aliases correctly
CleanAncilla = Qubit
CleanAncillae = Qubits
DirtyAncilla = Qubit
DirtyAncillae = Qubits
# Catalyst = Qubit
# Catalysts = Qubits
# Input = Register
# Inputs = Registers
# Output = Register
# Outputs = Registers


def _get_id_list_of_quantum_object(obj: QuantumObject) -> list[int]:
    ret = []
    if obj.is_qubit():
        if isinstance(obj, Qubit):
            ret.append(obj.get_id())
        if isinstance(obj, Qubits):
            ret.extend(q.get_id() for q in obj)
    elif obj.is_register():
        if isinstance(obj, Register):
            ret.append(obj.get_id())
        if isinstance(obj, Registers):
            ret.extend(r.get_id() for r in obj)
    return ret


######################################################
# pycircuit.cpp
######################################################


class ArgInfo:
    """Argument information."""

    @staticmethod
    def __create(method, *args) -> ArgInfo:  # noqa:ANN001,ANN002
        ret = ArgInfo()
        ret._impl = getattr(_M.ArgInfo, method)(*args)
        return ret

    @staticmethod
    def operate(arg_name: str) -> ArgInfo:
        """Create an argument of :obj:`QuantumAttribute.Operate` with qubit size 1.

        Args:
            arg_name (str): name of argument.

        Returns:
            ArgInfo: information of argument.
        """
        return ArgInfo.__create("operate", arg_name)

    @staticmethod
    def operates(arg_name: str, size: int) -> ArgInfo:
        """Create an argument of :obj:`QuantumAttribute.Operate` with arbitrary qubit size.

        Args:
            arg_name (str): name of argument.
            size (int): size of qubits.

        Returns:
            ArgInfo: information of argument.
        """
        return ArgInfo.__create("operates", arg_name, size)

    @staticmethod
    def clean_ancilla(arg_name: str) -> ArgInfo:
        """Create an argument of :obj:`QuantumAttribute.CleanAncilla` with qubit size 1.

        Args:
            arg_name (str): name of argument.

        Returns:
            ArgInfo: information of argument.
        """
        return ArgInfo.__create("clean_ancilla", arg_name)

    @staticmethod
    def clean_ancillae(arg_name: str, size: int) -> ArgInfo:
        """Create an argument of :obj:`QuantumAttribute.CleanAncilla` with arbitrary qubit size.

        Args:
            arg_name (str): name of argument.
            size (int): size of qubits.

        Returns:
            ArgInfo: information of argument.
        """
        return ArgInfo.__create("clean_ancillae", arg_name, size)

    @staticmethod
    def dirty_ancilla(arg_name: str) -> ArgInfo:
        """Create an argument of :obj:`QuantumAttribute.DirtyAncilla` with qubit size 1.

        Args:
            arg_name (str): name of argument.

        Returns:
            ArgInfo: information of argument.
        """
        return ArgInfo.__create("dirty_ancilla", arg_name)

    @staticmethod
    def dirty_ancillae(arg_name: str, size: int) -> ArgInfo:
        """Create an argument of :obj:`QuantumAttribute.DirtyAncilla` with arbitrary qubit size.

        Args:
            arg_name (str): name of argument.
            size (int): size of qubits.

        Returns:
            ArgInfo: information of argument.
        """
        return ArgInfo.__create("dirty_ancillae", arg_name, size)

    @staticmethod
    def catalyst(arg_name: str) -> ArgInfo:
        """Create an argument of :obj:`QuantumAttribute.Catalyst` with qubit size 1.

        Args:
            arg_name (str): name of argument.

        Returns:
            ArgInfo: information of argument.
        """
        return ArgInfo.__create("catalyst", arg_name)

    @staticmethod
    def catalysts(arg_name: str, size: int) -> ArgInfo:
        """Create an argument of :obj:`QuantumAttribute.Catalyst` with arbitrary qubit size.

        Args:
            arg_name (str): name of argument.
            size (int): size of qubits.

        Returns:
            ArgInfo: information of argument.
        """
        return ArgInfo.__create("catalysts", arg_name, size)

    @staticmethod
    def input(arg_name: str) -> ArgInfo:
        """Create an argument of :obj:`QuantumAttribute.Input` with register size 1.

        Args:
            arg_name (str): name of argument.

        Returns:
            ArgInfo: information of argument.
        """
        return ArgInfo.__create("input", arg_name)

    @staticmethod
    def inputs(arg_name: str, size: int) -> ArgInfo:
        """Create an argument of :obj:`QuantumAttribute.Input` with arbitrary register size.

        Args:
            arg_name (str): name of argument.
            size (int): size of qubits.

        Returns:
            ArgInfo: information of argument.
        """
        return ArgInfo.__create("inputs", arg_name, size)

    @staticmethod
    def output(arg_name: str) -> ArgInfo:
        """Create an argument of :obj:`QuantumAttribute.Output` with register size 1.

        Args:
            arg_name (str): name of argument.

        Returns:
            ArgInfo: information of argument.
        """
        return ArgInfo.__create("output", arg_name)

    @staticmethod
    def outputs(arg_name: str, size: int) -> ArgInfo:
        """Create an argument of :obj:`QuantumAttribute.Output` with arbitrary register size.

        Args:
            arg_name (str): name of argument.
            size (int): size of qubits.

        Returns:
            ArgInfo: information of argument.
        """
        return ArgInfo.__create("outputs", arg_name, size)

    @property
    def arg_name(self) -> str:
        """Get name of argument.

        Returns:
            str: name of argument
        """
        return self._impl.arg_name

    @property
    def size(self) -> int:
        """Get size of argument.

        Returns:
            int: the size of qubits.
        """
        return self._impl.size

    @property
    def type(self) -> QuantumType:
        """Get type of argument.

        Returns:
            QuantumType: type of argument.
        """
        return self._impl.type

    @property
    def attribute(self) -> QuantumAttribute:
        """Get attribute of argument.

        Returns:
            QuantumAttribute: attribute of argument.
        """
        return self._impl.attribute

    @property
    def is_array(self) -> bool:
        """Check if argument is an array of quantum objects.

        Returns:
            bool: true if argument is an array of quantum objects.
        """
        return self._impl.is_array

    def __str__(self) -> str:
        return str(self._impl)


class Argument:
    def __init__(self) -> None:
        """Argument of :class:`Circuit`.

        The :meth:`CircuitGenerator.arg` returns an instance of this class and this class should not be used outside of that context.
        """
        self._impl = _M.Argument()

    def add(self, info: ArgInfo) -> Self:  # noqa:D102
        self._impl.add(info._impl)
        return self

    def add_operate(self, arg_name: str) -> Self:  # noqa:D102
        self._impl.add_operate(arg_name)
        return self

    def add_operates(self, arg_name: str, size: int) -> Self:  # noqa:D102
        self._impl.add_operates(arg_name, size)
        return self

    def add_clean_ancilla(self, arg_name: str) -> Self:  # noqa:D102
        self._impl.add_clean_ancilla(arg_name)
        return self

    def add_clean_ancillae(self, arg_name: str, size: int) -> Self:  # noqa:D102
        self._impl.add_clean_ancillae(arg_name, size)
        return self

    def add_dirty_ancilla(self, arg_name: str) -> Self:  # noqa:D102
        self._impl.add_dirty_ancilla(arg_name)
        return self

    def add_dirty_ancillae(self, arg_name: str, size: int) -> Self:  # noqa:D102
        self._impl.add_dirty_ancillae(arg_name, size)
        return self

    def add_catalyst(self, arg_name: str) -> Self:  # noqa:D102
        self._impl.add_catalyst(arg_name)
        return self

    def add_catalysts(self, arg_name: str, size: int) -> Self:  # noqa:D102
        self._impl.add_catalysts(arg_name, size)
        return self

    def add_input(self, arg_name: str) -> Self:  # noqa:D102
        self._impl.add_input(arg_name)
        return self

    def add_inputs(self, arg_name: str, size: int) -> Self:  # noqa:D102
        self._impl.add_inputs(arg_name, size)
        return self

    def add_output(self, arg_name: str) -> Self:  # noqa:D102
        self._impl.add_output(arg_name)
        return self

    def add_outputs(self, arg_name: str, size: int) -> Self:  # noqa:D102
        self._impl.add_outputs(arg_name, size)
        return self

    def get_num_args(self) -> int:
        """Get the number of arguments.

        Returns:
            int: the number of arguments.
        """
        return self._impl.get_num_args()

    def get_arg_names(self) -> list[str]:
        """Get the list of argument names.

        Returns:
            list[str]: the list of argument names
        """
        return self._impl.get_arg_names()

    def view_arg_info(self, arg_name: str) -> ArgInfo:
        """View the argument information.

        Args:
            arg_name (str): the number of argument to view.

        Returns:
            ArgInfo:information.
        """
        ret = ArgInfo()
        ret._impl = self._impl.view_arg_info(arg_name)
        return ret

    def get(self, arg_name: str) -> Qubit | Qubits | Register | Registers:
        """Get the quantum object.

        This method should only be used within :meth:`CircuitGenerator.logic`.

        Args:
            arg_name (str): the name of quantum object.

        Returns:
            Qubit | Qubits | Register | Registers: the quantum object.
        """  # noqa:DOC501
        quantum_object_impl = self._impl.get(arg_name)
        if isinstance(quantum_object_impl, _M.Qubit):
            ret = Qubit()
        elif isinstance(quantum_object_impl, _M.Qubits):
            ret = Qubits()
        elif isinstance(quantum_object_impl, _M.Register):
            ret = Register()
        elif isinstance(quantum_object_impl, _M.Registers):
            ret = Registers()
        else:
            msg = f"Argument '{arg_name}' has unknown type: {type(quantum_object_impl)}"
            raise TypeError(msg)
        ret._impl = quantum_object_impl
        return ret

    def _get_start_and_size(self, arg_name: str) -> tuple[int, int]:
        return self._impl._get_start_and_size(arg_name)

    def __getitem__(self, arg_name: str) -> Qubit | Qubits | Register | Registers:
        """Get the quantum object.

        This method is the same as :meth:`get`.

        Args:
            arg_name (str): the name of quantum object.

        Returns:
            Qubit | Qubits | Register | Registers: the quantum object.
        """
        return self.get(arg_name)


class Circuit:
    """Represents a quantum circuit. This is one of the most important classes in pyqret.

    There are three types of circuits in pyqret:

    * ir (:class:`IRFunction`) : In pyqret, the body of the circuit is represented by the intermediate representation (IR).
    * frontend : Frontend circuits are used to define circuits. Specifically by calling the fronted circuit, you can embed this circuit into other circuits.
    * backend (:class:`MachineFunction`) : By compiling the IR circuit, you can generate the backend circuit described in the target assembly.

    How to create a quantum circuit:

    #. Define circuit logic with :class:`CircuitGenerator`.
    #. Create a container :class:`Context` and :class:`Module` to store circuits.
    #. Create a builder :class:`CircuitBuilder` to build a circuit.
    #. Using :class:`CircuitBuilder` and :class:`CircuitGenerator`, circuits are created by calling the generator's ``generate()`` method.
    """

    @property
    def argument(self) -> Argument:
        """Get argument.

        Returns:
            Argument: argument.
        """
        ret = Argument()
        ret._impl = self._impl.argument
        return ret

    def __call__(self, *args: QuantumObject) -> None:
        """Embed this circuit within another circuit.

        Args:
            *args (QuantumObject): arguments.

        Raises:
            RuntimeError: If circuit does not have frontend expression.
        """
        if not self.has_frontend():
            msg = "Circuit does not have frontend expression"
            raise RuntimeError(msg)

        if self._impl._frontend_loaded_from_ir():
            q = self._set_call_arg_q(*args)
            self._impl._call_impl(q, [], [])
        else:
            q, in_r, out_r = self._set_call_arg(*args)
            self._impl._call_impl(q, in_r, out_r)

    @staticmethod
    def _set_call_arg_q(*args: QuantumObject) -> list[int]:
        q = []
        for arg in args:
            q.extend(_get_id_list_of_quantum_object(arg))
        return q

    def _set_call_arg(self, *args: QuantumObject) -> tuple[list[int], list[int], list[int]]:
        q = []
        in_r = []
        out_r = []
        c_arg = self.argument

        if len(args) != c_arg.get_num_args():
            msg = f"the number of arguments (={len(args)}) does not match the number of arguments in the circuit (={c_arg.get_num_args()})"
            raise ValueError(msg)

        arg_names = c_arg.get_arg_names()
        for i, (arg, name) in enumerate(zip(args, arg_names)):
            info = c_arg.view_arg_info(name)
            self._check_arg(i, info, arg)
            if info.attribute == QuantumAttribute.Input:
                in_r.extend(_get_id_list_of_quantum_object(arg))
            elif info.attribute == QuantumAttribute.Output:
                out_r.extend(_get_id_list_of_quantum_object(arg))
            else:
                q.extend(_get_id_list_of_quantum_object(arg))

        return q, in_r, out_r

    def _check_arg(self, i: int, info: ArgInfo, arg: QuantumObject) -> None:  # noqa:PLR6301
        # Check type
        if info.type == QuantumType.Qubit and (not (isinstance(arg, Qubit) or isinstance(arg, Qubits))):  # noqa:SIM101
            msg = f"invalid type of {i}-th argument (expected: {info.type})"
            raise TypeError(msg)
        if info.type == QuantumType.Register and (not (isinstance(arg, Register) or isinstance(arg, Registers))):  # noqa:SIM101
            msg = f"invalid type of {i}-th argument (expected: {info.type})"
            raise TypeError(msg)

        # Check size
        if info.size != arg.size():
            msg = f"invalid size of {i}-th argument (expected: {info.size})"
            raise ValueError(msg)

    def has_frontend(self) -> bool:
        """Return true if the circuit has a frontend.

        Returns:
            bool: true if the circuit has a frontend.
        """
        return self._impl.has_frontend()

    def has_ir(self) -> bool:
        """Return true if the circuit has an IR.

        Returns:
            bool: true if the circuit has an IR.
        """
        return self._impl.has_ir()

    def has_mf(self) -> bool:
        """Return true if the circuit has a backend.

        Returns:
            bool: true if the circuit has a backend.
        """
        return self._impl.has_mf()

    def get_ir(self) -> IRFunction:
        """Get the IR of circuit.

        Returns:
            bool: IR
        """
        ret = IRFunction()
        ret._impl = self._impl.get_ir()
        return ret

    def get_mf(self) -> MachineFunction:
        """Get the backend of circuit.

        Returns:
            MachineFunction: backend.
        """
        from ..backend import MachineFunction  # noqa:PLC0415

        ret = MachineFunction()
        ret._impl = self._impl.get_mf()
        return ret


######################################################
# ir.cpp
######################################################


class Context:
    """Context in IR. Context is a container of :class:`Module`."""

    def __init__(self) -> None:
        """Initialize context."""
        self._impl = _M.Context()

    def load(self, path: str) -> Module:
        """Load module from file.

        Args:
            path (str): path of module file.

        Returns:
            Module: loaded module.
        """
        ret = Module("load", self)
        ret._impl._load(path)
        return ret

    def loads(self, s: str) -> Module:
        """Load module from json string.

        Args:
            s (str): json string of module.

        Returns:
            Module: loaded module.
        """
        ret = Module("loads", self)
        ret._impl._loads(s)
        return ret


class Module:
    def __init__(self, name: str, context: Context) -> None:
        """Module in IR. Module is a container of :class:`IRFunction`.

        Args:
            name (str): name of module.
            context (Context): context of module.
        """
        self._impl = _M.Module.Create(name, context._impl)

    def get_name(self) -> str:
        """Get the name of module.

        Returns:
            str: name.
        """
        return self._impl.get_name()

    def contains(self, name: str) -> bool:
        """Checks if a function exists within the module.

        Args:
            name (str): the name of function.

        Returns:
            bool: true if module contains function.
        """
        return self._impl.contains(name)

    def get_function(self, name: str) -> Circuit:
        """Get the function.

        Args:
            name (str): the name of function.

        Returns:
            Circuit: function.

        Raises:
            RuntimeError: if module does not contain function.
        """
        if not self.contains(name):
            msg = "Module does not contain function of name '" + name + "'"
            raise RuntimeError(msg)

        ret = Circuit()
        ret._impl = self._impl.get_function(name)
        return ret

    def get_circuit(self, name: str) -> Circuit:
        """Get the function.

        Args:
            name (str): the name of function.

        Returns:
            Circuit: function.
        """
        return self.get_function(name)

    def dump(self, path: str) -> None:
        """Dump module.

        Args:
            path (str): output file path.
        """
        self._impl.dump(path)

    def dumps(self) -> dict:
        """Dump module.

        Returns:
            dict: json of dumped module.
        """
        return json.loads(self._impl.dumps())

    def get_function_list(self) -> list[str]:
        """Get list of functions which the module contains.

        Returns:
            list[str]: list of function names.
        """
        return self._impl.get_function_list()

    def get_circuit_list(self) -> list[str]:
        """Get list of functions which the module contains.

        Returns:
            list[str]: list of function names.
        """
        return self.get_function_list()


class Instruction:
    """Instruction in IR."""

    def get_opcode(self) -> Opcode:
        """Get opcode.

        Returns:
            Opcode: opcode.
        """
        return self._impl.get_opcode()

    def get_metadata(self) -> str:
        """Get metadata.

        Returns:
            str: metadata.
        """
        return self._impl.get_metadata()

    def __str__(self) -> str:
        return str(self._impl)


class BasicBlock:
    """Basic block in IR.

    A basic block is simply a container of instructions that execute sequentially.
    A well formed basic block is formed of a list of non-terminating instructions followed by a single terminator instruction.
    """

    def name(self) -> str:
        """Get name.

        Returns:
            str: name.
        """
        return self._impl.name()

    def num_instructions(self) -> int:
        """Get the number of instructions in this bb.

        Returns:
            int:the number of instructions.
        """
        return self._impl.num_instructions()

    def _num_predecessors(self) -> int:
        """Get the number of predecessors of this basic block.

        Returns:
            int: the number of predecessors.
        """
        return self._impl.num_predecessors()

    def _num_successors(self) -> int:
        """Get the number of successors of this basic block.

        Returns:
            int: the number of successors.
        """
        return self._impl.num_successors()

    def is_entry_block(self) -> bool:
        """Return true if this bb is the entry block of the function.

        Returns:
            bool: true if this bb is the entry block of the function.
        """
        return self._impl.is_entry_block()

    def is_unitary(self) -> bool:
        """Return true if this bb does not run measurement instructions.

        Returns:
            bool: true if this bb does not run measurement instructions.
        """
        return self._impl.is_unitary()

    def does_measurement(self) -> bool:
        """Return true if this bb runs measurement instructions.

        Returns:
            bool: true if this bb runs measurement instructions.
        """
        return self._impl.does_measurement()

    def __iter__(self) -> IteratorWrapper:
        return IteratorWrapper(self._impl, Instruction)


class IRFunction:
    """Function in IR.

    A function is a container of basic blocks.
    Each basic block has dependency determined by its terminator.
    This dependency is referred to as Control Flow Graph (CFG) and can be visualized as a directed graph with :meth:`gen_cfg` method.
    """

    def name(self) -> str:
        """Get name.

        Returns:
            str: name.
        """
        return self._impl.name()

    def get_entry_bb(self) -> BasicBlock:
        """Get entry basic block.

        Returns:
            BasicBlock: entry basic block
        """
        ret = BasicBlock()
        ret._impl = self._impl.get_entry_bb()
        return ret

    def num_instructions(self) -> int:
        """Get the number of instructions.

        Returns:
            int: the number of instructions.
        """
        return self._impl.num_instructions()

    def num_qubits(self) -> int:
        """Get the number of qubits.

        Returns:
            int: the number of qubits.
        """
        return self._impl.num_qubits()

    def num_registers(self) -> int:
        """Get the number of registers.

        Returns:
            int: the number of registers.
        """
        return self._impl.num_registers()

    def num_tmp_registers(self) -> int:
        """Get the number of temporal registers.

        Returns:
            int: the number of temporal registers.
        """
        return self._impl.num_tmp_registers()

    def is_unitary(self) -> bool:
        """Check if the function is unitary or not.

        Returns:
            bool: true if the function is unitary.
        """
        return self._impl.is_unitary()

    def does_measurement(self) -> bool:
        """Check if the function measures qubits or not.

        Returns:
            bool: true if the function measures qubits.
        """
        return self._impl.does_measurement()

    def gen_cfg(self) -> str:
        """Generate the control flow graph of this function in dot-language.

        Returns:
            str: dot-language representing control flow graph.
        """
        return self._impl.gen_cfg()

    def gen_call_graph(self, *, display_num_calls: bool = False) -> str:
        """Generate the call graph of this function in dot-language.

        Args:
            display_num_calls (bool, optional): display how many times it is called for each edge. Defaults to False.

        Returns:
            str: dot-language representing call graph.
        """
        return self._impl.gen_call_graph(display_num_calls)

    def __iter__(self) -> IteratorWrapper:
        return IteratorWrapper(self._impl, BasicBlock)


######################################################
# Python original implementation
######################################################


class CircuitGenerator(ABC):
    def __init__(self, builder: CircuitBuilder) -> None:
        """Generator of :class:`Circuit`.

        When creating a new circuit, inherit from this class and correctly implement the following methods:

        * :meth:`name`
        * :meth:`arg`
        * :meth:`logic`
        """
        self.builder = builder

    @abstractmethod
    def name(self) -> str:
        """Get the name of a circuit to be generated.

        Note:
            The name of the circuit must be unique.

        Returns:
            str: the name of a circuit.
        """

    @abstractmethod
    def arg(self) -> Argument:
        """Get the argument of a circuit to be generated.

        Returns:
            Argument: the argument of a circuit.
        """

    @abstractmethod
    def logic(self, arg: Argument) -> None:
        """Define a circuit to be generated.

        Args:
            arg (Argument): the argument.
        """

    def _is_cached(self) -> bool:
        return self.builder._impl.contains(self.name())

    def _get_cached_circuit(self) -> Circuit:
        ret = Circuit()
        ret._impl = self.builder._impl.get_circuit(self.name())
        return ret

    def get_temporal_register(self) -> Register:
        """Get a temporal register.

        In pyqret, a temporal register is defined as a register that is used solely within the circuit.

        Returns:
            Register: a temporal register.
        """
        ret = Register()
        ret._impl = self.builder._impl._get_temporal_register()
        return ret

    def get_temporal_registers(self, size: int) -> Registers:
        """Get temporal registers.

        In pyqret, a temporal register is defined as a register that is used solely within the circuit.

        Args:
            size (int): the number of registers

        Returns:
            Registers: temporal registers
        """
        ret = Registers()
        ret._impl = self.builder._impl._get_temporal_registers(size)
        return ret

    def generate(self) -> Circuit:
        """Generate a circuit defined by :meth:`logic` .

        Note:
            The method caches the generated circuit whe called for the first time.
            On subsequent calls, it returns the cached circuit and does not generate the circuit again.

        Returns:
            Circuit: the generated circuit.
        """
        if self._is_cached():
            return self._get_cached_circuit()

        circuit = self.builder._create_circuit(self.name(), self.arg())
        self.builder._begin_circuit_definition(circuit)
        self.logic(circuit.argument)
        self.builder._end_circuit_definition(circuit)
        return circuit
