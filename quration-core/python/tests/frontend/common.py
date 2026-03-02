from pyqret.frontend import (
    ArgInfo,
    Argument,
    Circuit,
    CircuitBuilder,
    CircuitGenerator,
    Context,
    Module,
    QuantumAttribute,
    Qubit,
)
from pyqret.frontend.gate.intrinsic import measure


def generate_circuit_for_test(name: str, func, args, *, measure_all_operates=True) -> Circuit:  # noqa:ANN001
    class SimpleGenerator(CircuitGenerator):
        def __init__(self, builder: CircuitBuilder, name: str, func, args, measure_all_operates) -> None:  # noqa:ANN001
            super().__init__(builder)
            self._name = name
            self.func = func
            self.args = args
            self.register_prefix = "m_"
            self.to_measures = []
            self.measure_all_operates = measure_all_operates

        def to_register_name(self, name: str) -> str:
            return self.register_prefix + name

        def name(self) -> str:
            return self._name

        def arg(self) -> Argument:
            ret = Argument()
            for tmp in self.args:
                if isinstance(tmp, ArgInfo):
                    ret.add(tmp)
                    if tmp.attribute == QuantumAttribute.Operate:
                        self.to_measures.append(tmp)
            if self.measure_all_operates:
                for to_measure in self.to_measures:
                    ret.add_outputs(self.to_register_name(to_measure.arg_name), to_measure.size)
            return ret

        def logic(self, arg: Argument) -> None:
            # Set args
            values = []
            for tmp in self.args:
                if isinstance(tmp, ArgInfo):
                    values.append(arg[tmp.arg_name])
                else:
                    values.append(tmp)

            self.func(*values)

            # Measure all operate qubits
            if self.measure_all_operates:
                for to_measure in self.to_measures:
                    qs = arg[to_measure.arg_name]
                    if isinstance(qs, Qubit):
                        qs = [qs]
                    rs = arg[self.to_register_name(to_measure.arg_name)]
                    for q, r in zip(qs, rs):
                        measure(q, r)

    context = Context()
    module = Module("test", context)
    builder = CircuitBuilder(module)
    gen = SimpleGenerator(builder, name, func, args, measure_all_operates)
    return gen.generate()
