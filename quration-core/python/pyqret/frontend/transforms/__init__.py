"""pyqret.frontend.transforms module."""

from __future__ import annotations

from typing import TYPE_CHECKING

from ..._qret_impl.frontend import Opcode  # pyright: ignore[reportMissingImports]
from ..._qret_impl.frontend import transforms as _M  # pyright: ignore[reportMissingImports]

if TYPE_CHECKING:
    from .. import Instruction, IRFunction


def inline_call_inst(inst: Instruction, *, merge_trivial: bool = True) -> bool:
    """Inline the callee function referred to by a call instruction.

    Args:
        inst (Instruction): The call instruction to inline.
        merge_trivial (bool): Whether to merge trivial basic blocks after inlining.

    Returns:
        bool: True if inlining was performed; otherwise, False.

    Raises:
        ValueError: If the provided instruction is not a call instruction.
    """
    if inst.get_opcode() != Opcode.Call:
        msg = f"Instruction is not call inst: {inst!s}"
        raise ValueError(msg)
    return _M.inline_call_inst(inst._impl, merge_trivial)


def inline_circuit(func: IRFunction, depth: int = 1, *, merge_trivial: bool = True) -> bool:
    """Inline all call instructions in the given IR function.

    Args:
        func (IRFunction): The function whose call instructions will be inlined.
        depth (int): The maximum number of inlining iterations to perform.

            - If `depth = 1`, all call instructions in the original function are inlined once.
            - If `depth = 2`, all call instructions in the original function are inlined, and
                then any new call instructions introduced by the first inlining are also inlined.
            - If `depth = 3`, the same process is repeated up to three levels deep.
            - If `depth` is negative, inlining continues recursively until no call instructions remain.

        merge_trivial (bool): If True, merges trivial basic blocks after inlining.

    Returns:
        bool: True if any inlining was performed; otherwise, False.
    """
    return _M.inline_circuit(func._impl, depth, merge_trivial)
