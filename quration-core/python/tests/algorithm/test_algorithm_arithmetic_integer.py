# ruff: noqa: I001
import pytest
pytest.importorskip("pyqret.algorithm.arithmetic", exc_type=ImportError)
from pyqret.algorithm.arithmetic import *
from pyqret.frontend import ArgInfo
from pyqret.runtime import QuantumStateType, Simulator, SimulatorConfig

from pyqret import bool_array_as_int

from frontend.common import generate_circuit_for_test


@pytest.mark.parametrize(
    ("size", "tgt"),
    [
        (5, 3),
        (5, 31),
        (10, 42),
        (1000, 2259781389479135703987517),
    ],
)
def test_inc(size: int, tgt: int):
    circuit = generate_circuit_for_test(
        f"inc_{size}",
        inc,
        [
            ArgInfo.operates("tgt", size),
            ArgInfo.dirty_ancillae("aux", size),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    sim = Simulator(config, circuit)
    sim.set("tgt", tgt)
    sim.set("aux", 5)
    result = sim.run()
    assert bool_array_as_int(result["m_tgt"]) == (tgt + 1) % (2**size)


@pytest.mark.parametrize(
    ("size", "tgt"),
    [
        (5, 3),
        (5, 31),
        (10, 42),
        (1000, 2259781389479135703987517),
    ],
)
def test_multi_controlled_inc(size: int, tgt: int):
    csize = 2
    circuit = generate_circuit_for_test(
        f"multi_controlled_inc_{size}",
        multi_controlled_inc,
        [
            ArgInfo.operates("cs", csize),
            ArgInfo.operates("tgt", size),
            ArgInfo.dirty_ancillae("aux", size),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)

    for v in [0, 2**csize - 1]:
        sim = Simulator(config, circuit)
        sim.set("cs", v)
        sim.set("tgt", tgt)
        sim.set("aux", 5)
        result = sim.run()
        assert bool_array_as_int(result["m_cs"]) == v
        assert bool_array_as_int(result["m_tgt"]) == (tgt if v == 0 else (tgt + 1) % (2**size))


@pytest.mark.parametrize(
    ("size", "dst", "src"),
    [
        (5, 3, 5),
        (5, 27, 17),
        (10, 4, 42),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
    ],
)
def test_add(size: int, dst: int, src: int):
    circuit = generate_circuit_for_test(
        f"add_{size}",
        add,
        [
            ArgInfo.operates("dst", size),
            ArgInfo.operates("src", size),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    sim = Simulator(config, circuit)
    sim.set("dst", dst)
    sim.set("src", src)
    result = sim.run()
    assert bool_array_as_int(result["m_dst"]) == (dst + src) % (2**size)
    assert bool_array_as_int(result["m_src"]) == src


@pytest.mark.parametrize(
    ("size", "dst", "src"),
    [
        (5, 3, 5),
        (5, 27, 17),
        (10, 4, 42),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
    ],
)
def test_add_cuccaro(size: int, dst: int, src: int):
    circuit = generate_circuit_for_test(
        f"add_cuccaro_{size}",
        add_cuccaro,
        [
            ArgInfo.operates("dst", size),
            ArgInfo.operates("src", size),
            ArgInfo.clean_ancilla("aux"),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    sim = Simulator(config, circuit)
    sim.set("dst", dst)
    sim.set("src", src)
    result = sim.run()
    assert bool_array_as_int(result["m_dst"]) == (dst + src) % (2**size)
    assert bool_array_as_int(result["m_src"]) == src


@pytest.mark.parametrize(
    ("size", "dst", "src"),
    [
        (5, 3, 5),
        (5, 27, 17),
        (10, 4, 42),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
    ],
)
def test_add_craig(size: int, dst: int, src: int):
    circuit = generate_circuit_for_test(
        f"add_craig_{size}",
        add_craig,
        [
            ArgInfo.operates("dst", size),
            ArgInfo.operates("src", size),
            ArgInfo.clean_ancillae("aux", size - 1),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    sim = Simulator(config, circuit)
    sim.set("dst", dst)
    sim.set("src", src)
    result = sim.run()
    assert int(str(bool_array_as_int(result["m_dst"]))) == (dst + src) % (2**size)
    assert int(str(bool_array_as_int(result["m_src"]))) == src


@pytest.mark.parametrize(
    ("size", "dst", "src"),
    [
        (5, 3, 5),
        (5, 27, 17),
        (10, 4, 42),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
    ],
)
def test_controlled_add_craig(size: int, dst: int, src: int):
    csize = 1
    circuit = generate_circuit_for_test(
        f"controlled_add_craig_{size}",
        controlled_add_craig,
        [
            ArgInfo.operate("c"),
            ArgInfo.operates("dst", size),
            ArgInfo.operates("src", size),
            ArgInfo.clean_ancillae("aux", size),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    for v in [0, 2**csize - 1]:
        sim = Simulator(config, circuit)
        sim.set("c", v)
        sim.set("dst", dst)
        sim.set("src", src)
        result = sim.run()
        assert bool_array_as_int(result["m_c"]) == v
        assert int(str(bool_array_as_int(result["m_dst"]))) == (dst if v == 0 else (dst + src) % (2**size))
        assert int(str(bool_array_as_int(result["m_src"]))) == src


@pytest.mark.parametrize(
    ("size", "dst", "src"),
    [
        (5, 3, 5),
        (5, 27, 17),
        (10, 4, 42),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
    ],
)
def test_add_imm(size: int, dst: int, src: int):
    circuit = generate_circuit_for_test(
        f"add_imm_{size}",
        add_imm,
        [
            src,
            ArgInfo.operates("tgt", size),
            ArgInfo.dirty_ancilla("aux"),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    sim = Simulator(config, circuit)
    sim.set("tgt", dst)
    result = sim.run()
    assert bool_array_as_int(result["m_tgt"]) == (dst + src) % (2**size)


@pytest.mark.parametrize(
    ("size", "dst", "src"),
    [
        (5, 3, 5),
        (5, 27, 17),
        (10, 4, 42),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
    ],
)
def test_multi_controlled_add_imm(size: int, dst: int, src: int):
    csize = 3
    circuit = generate_circuit_for_test(
        f"multi_controlled_add_imm_{size}",
        multi_controlled_add_imm,
        [
            src,
            ArgInfo.operates("cs", csize),
            ArgInfo.operates("tgt", size),
            ArgInfo.dirty_ancilla("aux"),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    for v in [0, 2**csize - 1]:
        sim = Simulator(config, circuit)
        sim.set("cs", v)
        sim.set("tgt", dst)
        result = sim.run()
        assert bool_array_as_int(result["m_cs"]) == v
        assert bool_array_as_int(result["m_tgt"]) == (dst if v == 0 else (dst + src) % (2**size))


@pytest.mark.parametrize(
    ("size", "dst", "src"),
    [
        (5, 3, 5),
        (5, 27, 17),
        (10, 4, 42),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
    ],
)
def test_add_general(size: int, dst: int, src: int):
    circuit = generate_circuit_for_test(
        f"add_general_{size}",
        add_general,
        [
            ArgInfo.operates("dst", size + 3),
            ArgInfo.operates("src", size),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    sim = Simulator(config, circuit)
    sim.set("dst", dst)
    sim.set("src", src)
    result = sim.run()
    assert bool_array_as_int(result["m_dst"]) == (dst + src) % (2 ** (size + 3))
    assert bool_array_as_int(result["m_src"]) == src


@pytest.mark.parametrize(
    ("size", "dst", "src"),
    [
        (5, 3, 5),
        (5, 27, 17),
        (10, 4, 42),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
    ],
)
def test_multi_controlled_add_general(size: int, dst: int, src: int):
    csize = 3
    circuit = generate_circuit_for_test(
        f"multi_controlled_add_general_{size}",
        multi_controlled_add_general,
        [
            ArgInfo.operates("cs", csize),
            ArgInfo.operates("dst", size + 3),
            ArgInfo.operates("src", size),
            ArgInfo.dirty_ancilla("aux"),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    for v in [0, 2**csize - 1]:
        sim = Simulator(config, circuit)
        sim.set("cs", v)
        sim.set("dst", dst)
        sim.set("src", src)
        result = sim.run()
        assert bool_array_as_int(result["m_cs"]) == v
        assert bool_array_as_int(result["m_dst"]) == (dst if v == 0 else (dst + src) % (2 ** (size + 3)))
        assert bool_array_as_int(result["m_src"]) == src


@pytest.mark.parametrize(
    ("size", "dst", "src"),
    [
        (5, 3, 5),
        (5, 27, 17),
        (10, 4, 42),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
    ],
)
def test_sub(size: int, dst: int, src: int):
    circuit = generate_circuit_for_test(
        f"sub_{size}",
        sub,
        [
            ArgInfo.operates("dst", size),
            ArgInfo.operates("src", size),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    sim = Simulator(config, circuit)
    sim.set("dst", dst)
    sim.set("src", src)
    result = sim.run()
    assert bool_array_as_int(result["m_dst"]) == (2**size + dst - src) % (2**size)
    assert bool_array_as_int(result["m_src"]) == src


@pytest.mark.parametrize(
    ("size", "dst", "src"),
    [
        (5, 3, 5),
        (5, 27, 17),
        (10, 4, 42),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
    ],
)
def test_sub_cuccaro(size: int, dst: int, src: int):
    circuit = generate_circuit_for_test(
        f"sub_cuccaro_{size}",
        sub_cuccaro,
        [
            ArgInfo.operates("dst", size),
            ArgInfo.operates("src", size),
            ArgInfo.clean_ancilla("aux"),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    sim = Simulator(config, circuit)
    sim.set("dst", dst)
    sim.set("src", src)
    result = sim.run()
    assert bool_array_as_int(result["m_dst"]) == (2**size + dst - src) % (2**size)
    assert bool_array_as_int(result["m_src"]) == src


@pytest.mark.parametrize(
    ("size", "dst", "src"),
    [
        (5, 3, 5),
        (5, 27, 17),
        (10, 4, 42),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
    ],
)
def test_sub_craig(size: int, dst: int, src: int):
    circuit = generate_circuit_for_test(
        f"sub_craig_{size}",
        sub_craig,
        [
            ArgInfo.operates("dst", size),
            ArgInfo.operates("src", size),
            ArgInfo.clean_ancillae("aux", size - 1),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    sim = Simulator(config, circuit)
    sim.set("dst", dst)
    sim.set("src", src)
    result = sim.run()
    assert int(str(bool_array_as_int(result["m_dst"]))) == (2**size + dst - src) % (2**size)
    assert int(str(bool_array_as_int(result["m_src"]))) == src


@pytest.mark.parametrize(
    ("size", "dst", "src"),
    [
        (5, 3, 5),
        (5, 27, 17),
        (10, 4, 42),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
    ],
)
def test_controlled_sub_craig(size: int, dst: int, src: int):
    csize = 1
    circuit = generate_circuit_for_test(
        f"controlled_sub_craig_{size}",
        controlled_sub_craig,
        [
            ArgInfo.operate("c"),
            ArgInfo.operates("dst", size),
            ArgInfo.operates("src", size),
            ArgInfo.clean_ancillae("aux", size),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    for v in [0, 2**csize - 1]:
        sim = Simulator(config, circuit)
        sim.set("c", v)
        sim.set("dst", dst)
        sim.set("src", src)
        result = sim.run()
        assert bool_array_as_int(result["m_c"]) == v
        assert int(str(bool_array_as_int(result["m_dst"]))) == (dst if v == 0 else (2**size + dst - src) % (2**size))
        assert int(str(bool_array_as_int(result["m_src"]))) == src


@pytest.mark.parametrize(
    ("size", "dst", "src"),
    [
        (5, 3, 5),
        (5, 27, 17),
        (10, 4, 42),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
    ],
)
def test_sub_imm(size: int, dst: int, src: int):
    circuit = generate_circuit_for_test(
        f"sub_imm_{size}",
        sub_imm,
        [
            src,
            ArgInfo.operates("tgt", size),
            ArgInfo.dirty_ancilla("aux"),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    sim = Simulator(config, circuit)
    sim.set("tgt", dst)
    result = sim.run()
    assert bool_array_as_int(result["m_tgt"]) == (2**size + dst - src) % (2**size)


@pytest.mark.parametrize(
    ("size", "dst", "src"),
    [
        (5, 3, 5),
        (5, 27, 17),
        (10, 4, 42),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
    ],
)
def test_multi_controlled_sub_imm(size: int, dst: int, src: int):
    csize = 3
    circuit = generate_circuit_for_test(
        f"multi_controlled_sub_imm_{size}",
        multi_controlled_sub_imm,
        [
            src,
            ArgInfo.operates("cs", csize),
            ArgInfo.operates("tgt", size),
            ArgInfo.dirty_ancilla("aux"),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    for v in [0, 2**csize - 1]:
        sim = Simulator(config, circuit)
        sim.set("cs", v)
        sim.set("tgt", dst)
        result = sim.run()
        assert bool_array_as_int(result["m_cs"]) == v
        assert bool_array_as_int(result["m_tgt"]) == (dst if v == 0 else (2**size + dst - src) % (2**size))


@pytest.mark.parametrize(
    ("size", "dst", "src"),
    [
        (5, 3, 5),
        (5, 27, 17),
        (10, 4, 42),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
    ],
)
def test_sub_general(size: int, dst: int, src: int):
    circuit = generate_circuit_for_test(
        f"sub_general_{size}",
        sub_general,
        [
            ArgInfo.operates("dst", size + 3),
            ArgInfo.operates("src", size),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    sim = Simulator(config, circuit)
    sim.set("dst", dst)
    sim.set("src", src)
    result = sim.run()
    assert bool_array_as_int(result["m_dst"]) == (2 ** (size + 3) + dst - src) % (2 ** (size + 3))
    assert bool_array_as_int(result["m_src"]) == src


@pytest.mark.parametrize(
    ("size", "dst", "src"),
    [
        (5, 3, 5),
        (5, 27, 17),
        (10, 4, 42),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
    ],
)
def test_multi_controlled_sub_general(size: int, dst: int, src: int):
    csize = 3
    circuit = generate_circuit_for_test(
        f"multi_controlled_sub_general_{size}",
        multi_controlled_sub_general,
        [
            ArgInfo.operates("cs", csize),
            ArgInfo.operates("dst", size + 3),
            ArgInfo.operates("src", size),
            ArgInfo.dirty_ancilla("aux"),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    for v in [0, 2**csize - 1]:
        sim = Simulator(config, circuit)
        sim.set("cs", v)
        sim.set("dst", dst)
        sim.set("src", src)
        result = sim.run()
        assert bool_array_as_int(result["m_cs"]) == v
        assert bool_array_as_int(result["m_dst"]) == (
            dst if v == 0 else (2 ** (size + 3) + dst - src) % (2 ** (size + 3))
        )
        assert bool_array_as_int(result["m_src"]) == src


@pytest.mark.parametrize(
    ("size", "mul", "tgt"),
    [
        (5, 3, 5),
        (5, 27, 17),
        (10, 5, 42),
    ],
)
def test_mul_w(size: int, mul: int, tgt: int):
    wsize = 2
    circuit = generate_circuit_for_test(
        f"mul_w_{size}",
        mul_w,
        [
            wsize,
            mul,
            ArgInfo.operates("tgt", size),
            ArgInfo.clean_ancillae("table", size),
            ArgInfo.clean_ancillae("aux", size - 1),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=8)
    sim = Simulator(config, circuit)
    sim.set("tgt", tgt)
    result = sim.run()
    assert bool_array_as_int(result["m_tgt"]) == (mul * tgt) % 2**size


@pytest.mark.parametrize(
    ("size", "scale", "dst", "src"),
    [
        (5, 3, 5, 4),
        (5, 4, 5, 4),
        (5, 27, 17, 2),
        (10, 5, 42, 3),
    ],
)
def test_scaled_add(size: int, scale: int, dst: int, src: int):
    circuit = generate_circuit_for_test(
        f"scaled_add_{size}",
        scaled_add,
        [
            scale,
            ArgInfo.operates("dst", size),
            ArgInfo.operates("src", size),
            ArgInfo.clean_ancillae("aux", size - 1),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    sim = Simulator(config, circuit)
    sim.set("dst", dst)
    sim.set("src", src)
    result = sim.run()
    assert bool_array_as_int(result["m_dst"]) == (dst + scale * src) % 2**size
    assert bool_array_as_int(result["m_src"]) == src


@pytest.mark.parametrize(
    ("size", "scale", "dst", "src"),
    [
        (5, 3, 5, 4),
        (5, 4, 5, 4),
        (5, 27, 17, 2),
        (10, 5, 42, 3),
    ],
)
def test_scaled_add_w(size: int, scale: int, dst: int, src: int):
    wsize = 2
    circuit = generate_circuit_for_test(
        f"scaled_add_w_{size}",
        scaled_add_w,
        [
            wsize,
            scale,
            ArgInfo.operates("dst", size),
            ArgInfo.operates("src", size),
            ArgInfo.clean_ancillae("table", size),
            ArgInfo.clean_ancillae("aux", size - 1),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=8)
    sim = Simulator(config, circuit)
    sim.set("dst", dst)
    sim.set("src", src)
    result = sim.run()
    assert bool_array_as_int(result["m_dst"]) == (dst + scale * src) % 2**size
    assert bool_array_as_int(result["m_src"]) == src


@pytest.mark.parametrize(
    ("size", "lhs", "rhs"),
    [
        (5, 3, 5),
        (5, 27, 27),
        (10, 4, 42),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
        (1000, 5713485918297413529048534577, 5713485918297413529048534577),
    ],
)
def test_equal_to(size: int, lhs: int, rhs: int):
    circuit = generate_circuit_for_test(
        f"equal_to_{size}",
        equal_to,
        [
            ArgInfo.operates("lhs", size),
            ArgInfo.operates("rhs", size),
            ArgInfo.operate("cmp"),
            ArgInfo.clean_ancillae("aux", size - 1),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    for i in [0, 1]:
        sim = Simulator(config, circuit)
        sim.set("lhs", lhs)
        sim.set("rhs", rhs)
        sim.set("cmp", i)
        result = sim.run()
        assert bool_array_as_int(result["m_lhs"]) == lhs
        assert bool_array_as_int(result["m_rhs"]) == rhs
        assert bool_array_as_int(result["m_cmp"]) == ((i + 1) % 2 if lhs == rhs else i)


@pytest.mark.parametrize(
    ("size", "imm", "tgt"),
    [
        (5, 3, 5),
        (5, 27, 27),
        (10, 4, 42),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
        (1000, 5713485918297413529048534577, 5713485918297413529048534577),
    ],
)
def test_equal_to_imm(size: int, imm: int, tgt: int):
    circuit = generate_circuit_for_test(
        f"equal_to_imm_{size}",
        equal_to_imm,
        [
            imm,
            ArgInfo.operates("tgt", size),
            ArgInfo.operate("cmp"),
            ArgInfo.clean_ancillae("aux", size - 1),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    for i in [0, 1]:
        sim = Simulator(config, circuit)
        sim.set("tgt", tgt)
        sim.set("cmp", i)
        result = sim.run()
        assert bool_array_as_int(result["m_tgt"]) == tgt
        assert bool_array_as_int(result["m_cmp"]) == ((i + 1) % 2 if imm == tgt else i)


@pytest.mark.parametrize(
    ("size", "lhs", "rhs"),
    [
        (5, 3, 5),
        (5, 27, 27),
        (5, 30, 27),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
        (1000, 5713485918297413529048534577, 5713485918297413529048534577),
        (1000, 57134859182974135290485345771, 5713485918297413529048534577),
    ],
)
def test_less_than(size: int, lhs: int, rhs: int):
    circuit = generate_circuit_for_test(
        f"less_than_{size}",
        less_than,
        [
            ArgInfo.operates("lhs", size),
            ArgInfo.operates("rhs", size),
            ArgInfo.operate("cmp"),
            ArgInfo.clean_ancillae("aux", 2),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    for i in [0, 1]:
        sim = Simulator(config, circuit)
        sim.set("lhs", lhs)
        sim.set("rhs", rhs)
        sim.set("cmp", i)
        result = sim.run()
        assert bool_array_as_int(result["m_lhs"]) == lhs
        assert bool_array_as_int(result["m_rhs"]) == rhs
        assert bool_array_as_int(result["m_cmp"]) == ((i + 1) % 2 if lhs < rhs else i)


@pytest.mark.parametrize(
    ("size", "lhs", "rhs"),
    [
        (5, 3, 5),
        (5, 27, 27),
        (5, 30, 27),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
        (1000, 5713485918297413529048534577, 5713485918297413529048534577),
        (1000, 57134859182974135290485345771, 5713485918297413529048534577),
    ],
)
def test_less_than_or_equal_to(size: int, lhs: int, rhs: int):
    circuit = generate_circuit_for_test(
        f"less_than_or_equal_to_{size}",
        less_than_or_equal_to,
        [
            ArgInfo.operates("lhs", size),
            ArgInfo.operates("rhs", size),
            ArgInfo.operate("cmp"),
            ArgInfo.clean_ancillae("aux", 2),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    for i in [0, 1]:
        sim = Simulator(config, circuit)
        sim.set("lhs", lhs)
        sim.set("rhs", rhs)
        sim.set("cmp", i)
        result = sim.run()
        assert bool_array_as_int(result["m_lhs"]) == lhs
        assert bool_array_as_int(result["m_rhs"]) == rhs
        assert bool_array_as_int(result["m_cmp"]) == ((i + 1) % 2 if lhs <= rhs else i)


@pytest.mark.parametrize(
    ("size", "imm", "tgt"),
    [
        (5, 3, 5),
        (5, 27, 27),
        (5, 30, 27),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
        (1000, 5713485918297413529048534577, 5713485918297413529048534577),
        (1000, 57134859182974135290485345771, 5713485918297413529048534577),
    ],
)
def test_less_than_imm(size: int, imm: int, tgt: int):
    circuit = generate_circuit_for_test(
        f"less_than_imm_{size}",
        less_than_imm,
        [
            imm,
            ArgInfo.operates("tgt", size),
            ArgInfo.operate("cmp"),
            ArgInfo.dirty_ancilla("aux"),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    for i in [0, 1]:
        sim = Simulator(config, circuit)
        sim.set("tgt", tgt)
        sim.set("cmp", i)
        result = sim.run()
        assert bool_array_as_int(result["m_tgt"]) == tgt
        assert bool_array_as_int(result["m_cmp"]) == ((i + 1) % 2 if tgt < imm else i)


@pytest.mark.parametrize(
    ("size", "imm", "tgt"),
    [
        (5, 3, 5),
        (5, 27, 27),
        (5, 30, 27),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
        (1000, 5713485918297413529048534577, 5713485918297413529048534577),
        (1000, 57134859182974135290485345771, 5713485918297413529048534577),
    ],
)
def test_multi_controlled_less_than_imm(size: int, imm: int, tgt: int):
    csize = 3
    circuit = generate_circuit_for_test(
        f"multi_controlled_less_than_imm_{size}",
        multi_controlled_less_than_imm,
        [
            imm,
            ArgInfo.operates("cs", csize),
            ArgInfo.operates("tgt", size),
            ArgInfo.operate("cmp"),
            ArgInfo.dirty_ancilla("aux"),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    for v in [0, 2**csize - 1]:
        for i in [0, 1]:
            sim = Simulator(config, circuit)
            sim.set("cs", v)
            sim.set("tgt", tgt)
            sim.set("cmp", i)
            result = sim.run()
            assert bool_array_as_int(result["m_cs"]) == v
            assert bool_array_as_int(result["m_tgt"]) == tgt
            assert bool_array_as_int(result["m_cmp"]) == (i if v == 0 else ((i + 1) % 2 if tgt < imm else i))


@pytest.mark.parametrize(
    ("size", "dst", "src"),
    [
        (5, 3, 5),
        (5, 27, 27),
        (5, 30, 27),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
        (1000, 5713485918297413529048534577, 5713485918297413529048534577),
        (1000, 57134859182974135290485345771, 5713485918297413529048534577),
    ],
)
def test_bi_flip(size: int, dst: int, src: int):
    circuit = generate_circuit_for_test(
        f"bi_flip_{size}",
        bi_flip,
        [
            ArgInfo.operates("dst", size),
            ArgInfo.operates("src", size),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    sim = Simulator(config, circuit)
    sim.set("dst", dst)
    sim.set("src", src)
    result = sim.run()
    assert bool_array_as_int(result["m_dst"]) == (src - 1 - dst if dst < src else 2**size - 1 + src - dst)
    assert bool_array_as_int(result["m_src"]) == src


@pytest.mark.parametrize(
    ("size", "dst", "src"),
    [
        (5, 3, 5),
        (5, 27, 27),
        (5, 30, 27),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
        (1000, 5713485918297413529048534577, 5713485918297413529048534577),
        (1000, 57134859182974135290485345771, 5713485918297413529048534577),
    ],
)
def test_multi_controlled_bi_flip(size: int, dst: int, src: int):
    csize = 4
    circuit = generate_circuit_for_test(
        f"multi_controlled_bi_flip_{size}",
        multi_controlled_bi_flip,
        [
            ArgInfo.operates("cs", csize),
            ArgInfo.operates("dst", size),
            ArgInfo.operates("src", size),
            ArgInfo.dirty_ancilla("aux"),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    for v in [0, 2**csize - 1]:
        sim = Simulator(config, circuit)
        sim.set("cs", v)
        sim.set("dst", dst)
        sim.set("src", src)
        result = sim.run()
        assert bool_array_as_int(result["m_cs"]) == v
        assert bool_array_as_int(result["m_dst"]) == (
            dst if v == 0 else (src - 1 - dst if dst < src else 2**size - 1 + src - dst)
        )
        assert bool_array_as_int(result["m_src"]) == src


@pytest.mark.parametrize(
    ("size", "imm", "tgt"),
    [
        (5, 3, 5),
        (5, 27, 27),
        (5, 30, 27),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
        (1000, 5713485918297413529048534577, 5713485918297413529048534577),
        (1000, 57134859182974135290485345771, 5713485918297413529048534577),
    ],
)
def test_bi_flip_imm(size: int, imm: int, tgt: int):
    circuit = generate_circuit_for_test(
        f"bi_flip_imm_{size}",
        bi_flip_imm,
        [
            imm,
            ArgInfo.operates("tgt", size),
            ArgInfo.dirty_ancilla("aux"),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    sim = Simulator(config, circuit)
    sim.set("tgt", tgt)
    result = sim.run()
    assert bool_array_as_int(result["m_tgt"]) == (imm - 1 - tgt if tgt < imm else 2**size - 1 + imm - tgt)


@pytest.mark.parametrize(
    ("size", "imm", "tgt"),
    [
        (5, 3, 5),
        (5, 27, 27),
        (5, 30, 27),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
        (1000, 5713485918297413529048534577, 5713485918297413529048534577),
        (1000, 57134859182974135290485345771, 5713485918297413529048534577),
    ],
)
def test_multi_controlled_bi_flip_imm(size: int, imm: int, tgt: int):
    csize = 4
    circuit = generate_circuit_for_test(
        f"multi_controlled_bi_flip_imm_{size}",
        multi_controlled_bi_flip_imm,
        [
            imm,
            ArgInfo.operates("cs", csize),
            ArgInfo.operates("tgt", size),
            ArgInfo.dirty_ancilla("aux"),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    for v in [0, 2**csize - 1]:
        sim = Simulator(config, circuit)
        sim.set("cs", v)
        sim.set("tgt", tgt)
        result = sim.run()
        assert bool_array_as_int(result["m_cs"]) == v
        assert bool_array_as_int(result["m_tgt"]) == (
            tgt if v == 0 else (imm - 1 - tgt if tgt < imm else 2**size - 1 + imm - tgt)
        )


@pytest.mark.parametrize(
    ("size", "dst", "src"),
    [
        (5, 3, 5),
        (5, 27, 27),
        (5, 30, 27),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
        (1000, 5713485918297413529048534577, 5713485918297413529048534577),
        (1000, 57134859182974135290485345771, 5713485918297413529048534577),
    ],
)
def test_pivot_flip(size: int, dst: int, src: int):
    circuit = generate_circuit_for_test(
        f"pivot_flip_{size}",
        pivot_flip,
        [
            ArgInfo.operates("dst", size),
            ArgInfo.operates("src", size),
            ArgInfo.dirty_ancillae("aux", 2),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    sim = Simulator(config, circuit)
    sim.set("dst", dst)
    sim.set("src", src)
    result = sim.run()
    assert bool_array_as_int(result["m_dst"]) == (src - 1 - dst if dst < src else dst)
    assert bool_array_as_int(result["m_src"]) == src


@pytest.mark.parametrize(
    ("size", "dst", "src"),
    [
        (5, 3, 5),
        (5, 27, 27),
        (5, 30, 27),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
        (1000, 5713485918297413529048534577, 5713485918297413529048534577),
        (1000, 57134859182974135290485345771, 5713485918297413529048534577),
    ],
)
def test_multi_controlled_pivot_flip(size: int, dst: int, src: int):
    csize = 4
    circuit = generate_circuit_for_test(
        f"multi_controlled_pivot_flip_{size}",
        multi_controlled_pivot_flip,
        [
            ArgInfo.operates("cs", csize),
            ArgInfo.operates("dst", size),
            ArgInfo.operates("src", size),
            ArgInfo.dirty_ancillae("aux", 2),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    for v in [0, 2**csize - 1]:
        sim = Simulator(config, circuit)
        sim.set("cs", v)
        sim.set("dst", dst)
        sim.set("src", src)
        result = sim.run()
        assert bool_array_as_int(result["m_cs"]) == v
        assert bool_array_as_int(result["m_dst"]) == (dst if v == 0 else (src - 1 - dst if dst < src else dst))
        assert bool_array_as_int(result["m_src"]) == src


@pytest.mark.parametrize(
    ("size", "imm", "tgt"),
    [
        (5, 3, 5),
        (5, 27, 27),
        (5, 30, 27),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
        (1000, 5713485918297413529048534577, 5713485918297413529048534577),
        (1000, 57134859182974135290485345771, 5713485918297413529048534577),
    ],
)
def test_pivot_flip_imm(size: int, imm: int, tgt: int):
    circuit = generate_circuit_for_test(
        f"pivot_flip_imm_{size}",
        pivot_flip_imm,
        [
            imm,
            ArgInfo.operates("tgt", size),
            ArgInfo.dirty_ancillae("aux", 2),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    sim = Simulator(config, circuit)
    sim.set("tgt", tgt)
    result = sim.run()
    assert bool_array_as_int(result["m_tgt"]) == (imm - 1 - tgt if tgt < imm else tgt)


@pytest.mark.parametrize(
    ("size", "imm", "tgt"),
    [
        (5, 3, 5),
        (5, 27, 27),
        (5, 30, 27),
        (1000, 225978138947913570398751, 5713485918297413529048534577),
        (1000, 5713485918297413529048534577, 5713485918297413529048534577),
        (1000, 57134859182974135290485345771, 5713485918297413529048534577),
    ],
)
def test_multi_controlled_pivot_flip_imm(size: int, imm: int, tgt: int):
    csize = 4
    circuit = generate_circuit_for_test(
        f"multi_controlled_pivot_flip_imm_{size}",
        multi_controlled_pivot_flip_imm,
        [
            imm,
            ArgInfo.operates("cs", csize),
            ArgInfo.operates("tgt", size),
            ArgInfo.dirty_ancillae("aux", 2),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=2)
    for v in [0, 2**csize - 1]:
        sim = Simulator(config, circuit)
        sim.set("cs", v)
        sim.set("tgt", tgt)
        result = sim.run()
        assert bool_array_as_int(result["m_cs"]) == v
        assert bool_array_as_int(result["m_tgt"]) == (tgt if v == 0 else (imm - 1 - tgt if tgt < imm else tgt))
