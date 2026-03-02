# ruff: noqa: I001
import pytest
pytest.importorskip("pyqret.algorithm.arithmetic", exc_type=ImportError)
from pyqret.algorithm.arithmetic import *
from pyqret.frontend import ArgInfo
from pyqret.runtime import QuantumStateType, Simulator, SimulatorConfig

from pyqret import bool_array_as_int, modular_multiplicative_inverse

from frontend.common import generate_circuit_for_test


@pytest.mark.parametrize(
    ("mod", "size", "tgt"),
    [
        (28, 5, 3),
        (31, 5, 30),
        (381, 10, 42),
        (22597813894791357039875172, 1000, 2259781389479135703987517),
    ],
)
def test_mod_neg(mod: int, size: int, tgt: int):
    circuit = generate_circuit_for_test(
        f"mod_neg_{size}",
        mod_neg,
        [
            mod,
            ArgInfo.operates("tgt", size),
            ArgInfo.dirty_ancillae("aux", 2),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    sim = Simulator(config, circuit)
    sim.set("tgt", tgt)
    sim.set("aux", 5)
    result = sim.run()
    assert bool_array_as_int(result["m_tgt"]) == mod - tgt


@pytest.mark.parametrize(
    ("mod", "size", "tgt"),
    [
        (28, 5, 3),
        (31, 5, 30),
        (381, 10, 42),
        (22597813894791357039875172, 1000, 2259781389479135703987517),
    ],
)
def test_multi_controlled_mod_neg(mod: int, size: int, tgt: int):
    csize = 3
    circuit = generate_circuit_for_test(
        f"multi_controlled_mod_neg_{size}",
        multi_controlled_mod_neg,
        [
            mod,
            ArgInfo.operates("cs", csize),
            ArgInfo.operates("tgt", size),
            ArgInfo.dirty_ancillae("aux", 2),
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
        assert bool_array_as_int(result["m_tgt"]) == (tgt if v == 0 else mod - tgt)


@pytest.mark.parametrize(
    ("mod", "size", "dst", "src"),
    [
        (28, 5, 3, 5),
        (31, 5, 27, 17),
        (381, 10, 4, 42),
        (
            5713485918297413529048534578,
            1000,
            225978138947913570398751,
            5713485918297413529048534577,
        ),
    ],
)
def test_mod_add(mod: int, size: int, dst: int, src: int):
    circuit = generate_circuit_for_test(
        f"mod_add_{size}",
        mod_add,
        [
            mod,
            ArgInfo.operates("dst", size),
            ArgInfo.operates("src", size),
            ArgInfo.dirty_ancillae("aux", 2),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    sim = Simulator(config, circuit)
    sim.set("dst", dst)
    sim.set("src", src)
    result = sim.run()
    assert bool_array_as_int(result["m_dst"]) == (dst + src) % mod
    assert bool_array_as_int(result["m_src"]) == src


@pytest.mark.parametrize(
    ("mod", "size", "dst", "src"),
    [
        (28, 5, 3, 5),
        (31, 5, 27, 17),
        (381, 10, 4, 42),
        (
            5713485918297413529048534578,
            1000,
            225978138947913570398751,
            5713485918297413529048534577,
        ),
    ],
)
def test_multi_controlled_mod_add(mod: int, size: int, dst: int, src: int):
    csize = 4
    circuit = generate_circuit_for_test(
        f"multi_controlled_mod_add_{size}",
        multi_controlled_mod_add,
        [
            mod,
            ArgInfo.operates("cs", csize),
            ArgInfo.operates("dst", size),
            ArgInfo.operates("src", size),
            ArgInfo.dirty_ancillae("aux", max(0, 2 - csize)),
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
        assert bool_array_as_int(result["m_dst"]) == (dst if v == 0 else (dst + src) % mod)
        assert bool_array_as_int(result["m_src"]) == src


@pytest.mark.parametrize(
    ("mod", "size", "tgt", "imm"),
    [
        (28, 5, 3, 5),
        (31, 5, 27, 17),
        (381, 10, 4, 42),
        (
            5713485918297413529048534578,
            1000,
            225978138947913570398751,
            5713485918297413529048534577,
        ),
    ],
)
def test_mod_add_imm(mod: int, size: int, tgt: int, imm: int):
    circuit = generate_circuit_for_test(
        f"mod_add_imm_{size}",
        mod_add_imm,
        [
            mod,
            imm,
            ArgInfo.operates("tgt", size),
            ArgInfo.dirty_ancillae("aux", 2),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    sim = Simulator(config, circuit)
    sim.set("tgt", tgt)
    result = sim.run()
    assert bool_array_as_int(result["m_tgt"]) == (tgt + imm) % mod


@pytest.mark.parametrize(
    ("mod", "size", "tgt", "imm"),
    [
        (28, 5, 3, 5),
        (31, 5, 27, 17),
        (381, 10, 4, 42),
        (
            5713485918297413529048534578,
            1000,
            225978138947913570398751,
            5713485918297413529048534577,
        ),
    ],
)
def test_multi_controlled_mod_add_imm(mod: int, size: int, tgt: int, imm: int):
    csize = 4
    circuit = generate_circuit_for_test(
        f"multi_controlled_mod_add_imm_{size}",
        multi_controlled_mod_add_imm,
        [
            mod,
            imm,
            ArgInfo.operates("cs", csize),
            ArgInfo.operates("tgt", size),
            ArgInfo.dirty_ancillae("aux", 2),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    for v in [0, 2**csize - 1]:
        sim = Simulator(config, circuit)
        sim.set("cs", v)
        sim.set("tgt", tgt)
        result = sim.run()
        assert bool_array_as_int(result["m_cs"]) == v
        assert bool_array_as_int(result["m_tgt"]) == (tgt if v == 0 else (tgt + imm) % mod)


@pytest.mark.parametrize(
    ("mod", "size", "dst", "src"),
    [
        (28, 5, 3, 5),
        (31, 5, 27, 17),
        (381, 10, 4, 42),
        (
            5713485918297413529048534578,
            1000,
            225978138947913570398751,
            5713485918297413529048534577,
        ),
    ],
)
def test_mod_sub(mod: int, size: int, dst: int, src: int):
    circuit = generate_circuit_for_test(
        f"mod_sub_{size}",
        mod_sub,
        [
            mod,
            ArgInfo.operates("dst", size),
            ArgInfo.operates("src", size),
            ArgInfo.dirty_ancillae("aux", 2),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    sim = Simulator(config, circuit)
    sim.set("dst", dst)
    sim.set("src", src)
    result = sim.run()
    assert bool_array_as_int(result["m_dst"]) == (mod + dst - src) % mod
    assert bool_array_as_int(result["m_src"]) == src


@pytest.mark.parametrize(
    ("mod", "size", "dst", "src"),
    [
        (28, 5, 3, 5),
        (31, 5, 27, 17),
        (381, 10, 4, 42),
        (
            5713485918297413529048534578,
            1000,
            225978138947913570398751,
            5713485918297413529048534577,
        ),
    ],
)
def test_multi_controlled_mod_sub(mod: int, size: int, dst: int, src: int):
    csize = 4
    circuit = generate_circuit_for_test(
        f"multi_controlled_mod_sub_{size}",
        multi_controlled_mod_sub,
        [
            mod,
            ArgInfo.operates("cs", csize),
            ArgInfo.operates("dst", size),
            ArgInfo.operates("src", size),
            ArgInfo.dirty_ancillae("aux", max(0, 2 - csize)),
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
        assert bool_array_as_int(result["m_dst"]) == (dst if v == 0 else (mod + dst - src) % mod)
        assert bool_array_as_int(result["m_src"]) == src


@pytest.mark.parametrize(
    ("mod", "size", "tgt", "imm"),
    [
        (28, 5, 3, 5),
        (31, 5, 27, 17),
        (381, 10, 4, 42),
        (
            5713485918297413529048534578,
            1000,
            225978138947913570398751,
            5713485918297413529048534577,
        ),
    ],
)
def test_mod_sub_imm(mod: int, size: int, tgt: int, imm: int):
    circuit = generate_circuit_for_test(
        f"mod_sub_imm_{size}",
        mod_sub_imm,
        [
            mod,
            imm,
            ArgInfo.operates("tgt", size),
            ArgInfo.dirty_ancillae("aux", 2),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    sim = Simulator(config, circuit)
    sim.set("tgt", tgt)
    result = sim.run()
    assert bool_array_as_int(result["m_tgt"]) == (mod + tgt - imm) % mod


@pytest.mark.parametrize(
    ("mod", "size", "tgt", "imm"),
    [
        (28, 5, 3, 5),
        (31, 5, 27, 17),
        (381, 10, 4, 42),
        (
            5713485918297413529048534578,
            1000,
            225978138947913570398751,
            5713485918297413529048534577,
        ),
    ],
)
def test_multi_controlled_mod_sub_imm(mod: int, size: int, tgt: int, imm: int):
    csize = 4
    circuit = generate_circuit_for_test(
        f"multi_controlled_mod_sub_imm_{size}",
        multi_controlled_mod_sub_imm,
        [
            mod,
            imm,
            ArgInfo.operates("cs", csize),
            ArgInfo.operates("tgt", size),
            ArgInfo.dirty_ancillae("aux", 2),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    for v in [0, 2**csize - 1]:
        sim = Simulator(config, circuit)
        sim.set("cs", v)
        sim.set("tgt", tgt)
        result = sim.run()
        assert bool_array_as_int(result["m_cs"]) == v
        assert bool_array_as_int(result["m_tgt"]) == (tgt if v == 0 else (mod + tgt - imm) % mod)


@pytest.mark.parametrize(
    ("mod", "size", "tgt"),
    [
        (29, 5, 3),
        (31, 5, 27),
        (381, 10, 42),
        (
            5713485918297413529048534579,
            1000,
            225978138947913570398751,
        ),
    ],
)
def test_mod_doubling(mod: int, size: int, tgt: int):
    circuit = generate_circuit_for_test(
        f"mod_doubling_{size}",
        mod_doubling,
        [
            mod,
            ArgInfo.operates("tgt", size),
            ArgInfo.dirty_ancilla("aux"),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    sim = Simulator(config, circuit)
    sim.set("tgt", tgt)
    result = sim.run()
    assert bool_array_as_int(result["m_tgt"]) == (2 * tgt) % mod


@pytest.mark.parametrize(
    ("mod", "size", "tgt"),
    [
        (29, 5, 3),
        (31, 5, 27),
        (381, 10, 42),
        (
            5713485918297413529048534579,
            1000,
            225978138947913570398751,
        ),
    ],
)
def test_multi_controlled_mod_doubling(mod: int, size: int, tgt: int):
    csize = 4
    circuit = generate_circuit_for_test(
        f"multi_controlled_mod_doubling_{size}",
        multi_controlled_mod_doubling,
        [
            mod,
            ArgInfo.operates("cs", csize),
            ArgInfo.operates("tgt", size),
            ArgInfo.dirty_ancilla("aux"),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    for v in [0, 2**csize - 1]:
        sim = Simulator(config, circuit)
        sim.set("cs", v)
        sim.set("tgt", tgt)
        result = sim.run()
        assert bool_array_as_int(result["m_cs"]) == v
        assert bool_array_as_int(result["m_tgt"]) == (tgt if v == 0 else (2 * tgt) % mod)


@pytest.mark.parametrize(
    ("mod", "size", "imm", "lhs", "rhs"),
    [
        (27, 5, 5, 3, 5),
        (31, 5, 27, 4, 18),
        (383, 10, 42, 93, 222),
    ],
)
def test_mod_bi_mul_imm(mod: int, size: int, imm: int, lhs: int, rhs: int):
    circuit = generate_circuit_for_test(
        f"mod_bi_mul_imm_{size}",
        mod_bi_mul_imm,
        [
            mod,
            imm,
            ArgInfo.operates("lhs", size),
            ArgInfo.operates("rhs", size),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    sim = Simulator(config, circuit)
    sim.set("lhs", lhs)
    sim.set("rhs", rhs)
    inv = modular_multiplicative_inverse(imm, mod)
    result = sim.run()
    assert bool_array_as_int(result["m_lhs"]) == (imm * lhs) % mod
    assert bool_array_as_int(result["m_rhs"]) == (inv * rhs) % mod


@pytest.mark.parametrize(
    ("mod", "size", "imm", "lhs", "rhs"),
    [
        (27, 5, 5, 3, 5),
        (31, 5, 27, 4, 18),
        (383, 10, 42, 93, 222),
    ],
)
def test_multi_controlled_mod_bi_mul_imm(mod: int, size: int, imm: int, lhs: int, rhs: int):
    csize = 2
    circuit = generate_circuit_for_test(
        f"multi_controlled_mod_bi_mul_imm_{size}",
        multi_controlled_mod_bi_mul_imm,
        [
            mod,
            imm,
            ArgInfo.operates("cs", csize),
            ArgInfo.operates("lhs", size),
            ArgInfo.operates("rhs", size),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    for v in [0, 2**csize - 1]:
        sim = Simulator(config, circuit)
        sim.set("cs", v)
        sim.set("lhs", lhs)
        sim.set("rhs", rhs)
        inv = modular_multiplicative_inverse(imm, mod)
        result = sim.run()
        assert bool_array_as_int(result["m_cs"]) == v
        assert bool_array_as_int(result["m_lhs"]) == (lhs if v == 0 else (imm * lhs) % mod)
        assert bool_array_as_int(result["m_rhs"]) == (rhs if v == 0 else (inv * rhs) % mod)


@pytest.mark.parametrize(
    ("mod", "size", "scale", "dst", "src"),
    [
        (27, 5, 5, 3, 5),
        (31, 5, 27, 4, 18),
        (383, 10, 42, 93, 222),
    ],
)
def test_mod_scaled_add(mod: int, size: int, scale: int, dst: int, src: int):
    circuit = generate_circuit_for_test(
        f"mod_scaled_add_{size}",
        mod_scaled_add,
        [
            mod,
            scale,
            ArgInfo.operates("dst", size),
            ArgInfo.operates("src", size),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=1)
    sim = Simulator(config, circuit)
    sim.set("dst", dst)
    sim.set("src", src)
    result = sim.run()
    assert bool_array_as_int(result["m_dst"]) == (dst + scale * src) % mod
    assert bool_array_as_int(result["m_src"]) == src


@pytest.mark.parametrize(
    ("mod", "size", "scale", "dst", "src"),
    [
        (27, 5, 5, 3, 5),
        (31, 5, 27, 4, 18),
        (383, 10, 42, 93, 222),
    ],
)
def test_mod_scaled_add_w(mod: int, size: int, scale: int, dst: int, src: int):
    wsize = 3
    circuit = generate_circuit_for_test(
        f"mod_scaled_add_w_{size}",
        mod_scaled_add_w,
        [
            wsize,
            mod,
            scale,
            ArgInfo.operates("dst", size),
            ArgInfo.operates("src", size),
            ArgInfo.clean_ancillae("table", size),
            ArgInfo.clean_ancillae("aux", max(size - 1, 2)),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=8)
    sim = Simulator(config, circuit)
    sim.set("dst", dst)
    sim.set("src", src)
    result = sim.run()
    assert bool_array_as_int(result["m_dst"]) == (dst + scale * src) % mod
    assert bool_array_as_int(result["m_src"]) == src


@pytest.mark.parametrize(
    ("mod", "size", "scale", "dst", "src"),
    [
        (27, 5, 5, 3, 5),
        (31, 5, 27, 4, 18),
        (383, 10, 42, 93, 222),
    ],
)
def test_multi_controlled_mod_scaled_add(mod: int, size: int, scale: int, dst: int, src: int):
    csize = 3
    circuit = generate_circuit_for_test(
        f"multi_controlled_mod_scaled_add_{size}",
        multi_controlled_mod_scaled_add,
        [
            mod,
            scale,
            ArgInfo.operates("cs", csize),
            ArgInfo.operates("dst", size),
            ArgInfo.operates("src", size),
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
        assert bool_array_as_int(result["m_dst"]) == (dst if v == 0 else (dst + scale * src) % mod)
        assert bool_array_as_int(result["m_src"]) == src


@pytest.mark.parametrize(
    ("mod", "size", "base", "tgt", "exp"),
    [
        (27, 5, 5, 3, 5),
        (31, 5, 27, 4, 18),
        (383, 10, 42, 93, 222),
    ],
)
def test_mod_exp_w(mod: int, size: int, base: int, tgt: int, exp: int):
    mwsize = 3
    ewsize = 2
    circuit = generate_circuit_for_test(
        f"mod_exp_w_{size}",
        mod_exp_w,
        [
            mwsize,
            ewsize,
            mod,
            base,
            ArgInfo.operates("tgt", size),
            ArgInfo.operates("exp", size),
            ArgInfo.clean_ancillae("table", size),
            ArgInfo.clean_ancillae("tmp", size),
            ArgInfo.clean_ancillae("aux", mwsize + ewsize - 1),
        ],
    )
    config = SimulatorConfig(state_type=QuantumStateType.Toffoli, seed=0, max_superpositions=8)
    sim = Simulator(config, circuit)
    sim.set("tgt", tgt)
    sim.set("exp", exp)
    result = sim.run()
    assert bool_array_as_int(result["m_tgt"]) == (tgt * base**exp) % mod
    assert bool_array_as_int(result["m_exp"]) == exp
