"""pyqret module."""

from __future__ import annotations

from typing import TYPE_CHECKING, Any, Callable

from typing_extensions import Self

import importlib.util

import os

import platform

from pathlib import Path

from . import _qret_impl as _M

if TYPE_CHECKING:
    from collections.abc import Iterable, Sequence

__version__: str = _M.__version__


def get_major_version() -> int:
    """Get major version.

    Returns:
        int: major version.
    """
    return _M.get_major_version()


def get_minor_version() -> int:
    """Get minor version.

    Returns:
        int: minor version.
    """
    return _M.get_minor_version()


def get_patch_version() -> int:
    """Get patch version.

    Returns:
        int: patch version.
    """
    return _M.get_patch_version()


class BigInt:
    """A class that binds a multi-precision integer from C++ to Python.

    When calling a function that uses a multi-precision integer in pyqret,
    you must wrap it with this class.

    Args:
        x (int | str, optional): an integer or a string of integer. Defaults to 0.
    """

    def to_int(self) -> int:
        """Get python :obj:`int` .

        Returns:
            int: a python representation of the multi-precision integer
        """
        return self._impl.to_int()

    def to_str(self) -> str:
        """Get python :obj:`str` .

        Returns:
            str: a string representation of the integer value
        """
        return self._impl.to_str()

    def __init__(self, x: int | str = 0) -> None:  # noqa:D107
        if isinstance(x, int) or isinstance(x, str):  # noqa:SIM101
            self._impl = _M.BigInt(x)
        elif isinstance(x, _M.BigInt):
            self._impl = x
        else:
            msg = "`x` must be int or str"
            raise TypeError(msg)

    def __pos__(self) -> BigInt:
        return BigInt(self.to_int())

    def __neg__(self) -> BigInt:
        return BigInt(-self.to_int())

    def __add__(self, rhs: int | BigInt) -> BigInt:
        ret = BigInt()
        if isinstance(rhs, BigInt):
            ret._impl = self._impl + rhs._impl
        elif isinstance(rhs, int):
            ret._impl = self._impl + rhs
        else:
            msg = "`rhs` must be BigInt or int"
            raise TypeError(msg)
        return ret

    def __radd__(self, lhs: int) -> BigInt:
        return BigInt(BigInt(lhs)._impl + self._impl)

    def __iadd__(self, rhs: int | BigInt) -> Self:
        if isinstance(rhs, BigInt):
            self._impl += rhs._impl
        elif isinstance(rhs, int):
            self._impl += rhs
        else:
            msg = "`rhs` must be BigInt or int"
            raise TypeError(msg)
        return self

    def __sub__(self, rhs: int | BigInt) -> BigInt:
        ret = BigInt()
        if isinstance(rhs, BigInt):
            ret._impl = self._impl - rhs._impl
        elif isinstance(rhs, int):
            ret._impl = self._impl - rhs
        else:
            msg = "`rhs` must be BigInt or int"
            raise TypeError(msg)
        return ret

    def __rsub__(self, lhs: int) -> BigInt:
        return BigInt(BigInt(lhs)._impl - self._impl)

    def __isub__(self, rhs: int | BigInt) -> Self:
        if isinstance(rhs, BigInt):
            self._impl -= rhs._impl
        elif isinstance(rhs, int):
            self._impl -= rhs
        else:
            msg = "`rhs` must be BigInt or int"
            raise TypeError(msg)
        return self

    def __mul__(self, rhs: int | BigInt) -> BigInt:
        ret = BigInt()
        if isinstance(rhs, BigInt):
            ret._impl = self._impl * rhs._impl
        elif isinstance(rhs, int):
            ret._impl = self._impl * rhs
        else:
            msg = "`rhs` must be BigInt or int"
            raise TypeError(msg)
        return ret

    def __rmul__(self, lhs: int) -> BigInt:
        return BigInt(BigInt(lhs)._impl * self._impl)

    def __imul__(self, rhs: int | BigInt) -> Self:
        if isinstance(rhs, BigInt):
            self._impl = self._impl * rhs._impl  # noqa:PLR6104
        elif isinstance(rhs, int):
            self._impl = self._impl * rhs  # noqa:PLR6104
        else:
            msg = "`rhs` must be BigInt or int"
            raise TypeError(msg)
        return self

    def __eq__(self, rhs: object) -> bool:
        if isinstance(rhs, BigInt):
            return self._impl == rhs._impl
        if isinstance(rhs, int):
            return self._impl == rhs
        msg = "`rhs` must be BigInt or int"
        raise TypeError(msg)

    def __ne__(self, rhs: object) -> bool:
        if isinstance(rhs, BigInt):
            return self._impl != rhs._impl
        if isinstance(rhs, int):
            return self._impl != rhs
        msg = "`rhs` must be BigInt or int"
        raise TypeError(msg)

    def __hash__(self) -> int:
        return hash(self._impl.to_int())

    def __str__(self) -> str:
        return str(self._impl)


def bool_array_as_int(ba: Sequence[bool]) -> int:
    """Produce a non-negative integer from a bits in little-endian format.

    ``ba[0]`` is the least significant bit.

    Args:
        ba (Sequence[bool]): bits in binary representation of number.

    Returns:
        int: non-negative integer.

    Examples:
        >>> bool_array_as_int([False])
            0
        >>> bool_array_as_int([True, False, True])
            5
        >>> bool_array_as_int([False, True, True, False, True])
            22
    """
    return _M.bool_array_as_int(ba).to_int()


def int_as_bool_array(x: int | BigInt) -> list[bool]:
    """Produce a binary representation of a non-negative integer,using the little-endian representation for the returned array.

    Args:
        x (int | BigInt): non-negative integer.

    Returns:
        list[bool]: binary representation.

    Raises:
        TypeError: If argument is not int or BigInt.

    Examples:
        >>> int_as_bool_array(0)
            [False]
        >>> int_as_bool_array(5)
            [True, False, True]
        >>> int_as_bool_array(22)
            [False, True, True, False, True]
    """
    if isinstance(x, int):
        return _M.int_as_bool_array(BigInt(x)._impl)
    if isinstance(x, BigInt):
        return _M.int_as_bool_array(x._impl)
    msg = "only int or BigInt is allowed"
    raise TypeError(msg)


def modular_multiplicative_inverse(x: int, mod: int) -> int:
    """Calculate modular multiplicative inverse.

    Requirement: gcd of `x` and `mod` is 1.

    Args:
        x (int): positive integer
        mod (int): mod

    Returns:
        int: `i` s,t, `i` * `x` = 1 (mod `mod`)
    """
    ret = BigInt()
    ret._impl = _M.modular_multiplicative_inverse(BigInt(x)._impl, BigInt(mod)._impl)
    return ret.to_int()


def preprocess_lcu_coefficients_for_reversible_sampling(
    lcu_coefficients: list[float],
    sub_bit_precision: int,
) -> list[tuple[int, int]]:
    """Prepares data used to perform efficient reversible roulette selection.

    Args:
        lcu_coefficients (list[float]): a list of non-negative floating points.
        sub_bit_precision (int): the exponent mu such that denominator = n * 2^{mu} where n = lcu_coefficients.size().

    Returns:
        list[tuple[int, int]]: list of (alternated_index: an alternate index for each index from 0 to n - 1, switch_weight: indicates how often one should switch to alternates[i] instead of staying at index i)
    """
    ret = _M.preprocess_lcu_coefficients_for_reversible_sampling(lcu_coefficients, sub_bit_precision)
    return [(alternate_index, switch_weight.to_int()) for alternate_index, switch_weight in ret]


class IteratorWrapper:
    """Wrap the native iterator of the _qret_impl and create an iterator for the pyqret library."""

    def __init__(self, iterable: Iterable[Any], convert_to: Callable[[], Any]) -> None:
        """Create wrapped iterator.

        Args:
            iterable : tha native iterator of the _qret_impl.
            convert_to : the class of pyqret, which wraps the class of the instance pointed to by the native iterator.
        """
        self._iter = iter(iterable)
        self._convert_to = convert_to

    def __iter__(self) -> Self:
        return self

    def __next__(self):  # noqa:ANN204
        ret = self._convert_to()
        ret._impl = next(self._iter)
        return ret


_ENV_PATH = "GRIDSYNTH_PATH"


def _qret_package_root() -> Path | None:
    spec = importlib.util.find_spec("pyqret")
    if spec is None:
        return None
    if spec.origin:
        return Path(spec.origin).resolve().parent
    return None


def _set_gridsynth_env_from_package() -> None:
    if _ENV_PATH in os.environ:
        return

    package_root = _qret_package_root()
    if package_root is None:
        return

    bin_dir = package_root / "externals" / "newsynth"
    if not bin_dir.is_dir():
        return

    candidates = [
        bin_dir / "gridsynth",
        bin_dir / "gridsynth.exe",
    ]
    if platform.system().lower() == "windows":
        candidates.reverse()

    for path in candidates:
        if path.is_file() and os.access(path, os.X_OK):
            os.environ[_ENV_PATH] = str(path.resolve())
            break


_set_gridsynth_env_from_package()
