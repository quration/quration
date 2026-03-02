import pytest
from pyqret.frontend import Context, Module


def test_empty_module():
    context = Context()
    module = Module("empty_module", context)
    assert [] == module.get_circuit_list()


def test_load_module_from_json():
    context = Context()
    module = context.load("quration-core/examples/data/circuit/add_craig_5.json")
    assert "craig" == module.get_name()
    assert module.contains("AddCraig(5)")
    assert module.contains("TemporalAnd")
    assert module.contains("UncomputeTemporalAnd")
    assert not module.contains("Foo")
    assert {"AddCraig(5)", "TemporalAnd", "UncomputeTemporalAnd"} == set(module.get_circuit_list())

    print(module.dumps())


def test_load_circuit_from_json():
    context = Context()
    module = context.load("quration-core/examples/data/circuit/add_craig_5.json")
    assert module.contains("TemporalAnd")
    circuit = module.get_circuit("TemporalAnd")
    assert not circuit.has_frontend()
    assert circuit.has_ir()
    assert not circuit.has_mf()


def test_load_non_exist_circuit():
    context = Context()
    module = context.load("quration-core/examples/data/circuit/add_craig_5.json")
    assert not module.contains("Foo")
    with pytest.raises(RuntimeError):
        module.get_circuit("Foo")
