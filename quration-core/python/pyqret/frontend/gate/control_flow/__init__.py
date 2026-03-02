"""pyqret.frontend.gate.control_flow module."""

from ...._qret_impl.frontend.gate import control_flow as _M  # pyright: ignore[reportMissingImports]
from ... import Register, Registers


def qu_if(cond: Register) -> None:
    """Insert a branching instruction to the quantum circuit.

    By using this function, you can implement a quantum circuit
    that switches the gates to be executed based on the measurement results.

    There are two ways to use this function.

    Case1: :func:`qu_if` ~ :func:`qu_else` ~ :func:`qu_end_if`

    .. code-block:: python

        # Let r be an instance of Register class
        qu_if(r)
        # define a circuit for the case when r == 1
        qu_else(r)
        # define a circuit for the case when r == 0
        qu_end_if(r)

    Case2: :func:`qu_if` ~ :func:`qu_end_if`

    .. code-block:: python

        # Let r be an instance of Register class
        qu_if(r)
        # define a circuit for the case when r == 1
        qu_end_if(r)

    Note:
        If you call ``qu_if(r)``, you must always call ``qu_end_if(r)``.

    Args:
        cond (Register): condition register.
    """
    _M.qu_if(cond._impl)


def qu_else(cond: Register) -> None:
    """Insert a branching instruction to the quantum circuit.

    See :func:`qu_if` for more details.

    Args:
        cond (Register): condition register.
    """
    _M.qu_else(cond._impl)


def qu_end_if(cond: Register) -> None:
    """Insert a branching instruction to the quantum circuit.

    See :func:`qu_if` for more details.

    Args:
        cond (Register): condition register.
    """
    _M.qu_end_if(cond._impl)


def qu_switch(value: Registers) -> None:
    """Begin a multi-way branch on the given register value.

    This starts a *switch* block. Inside a switch block, you must add
    two or more :func:`qu_case` arms, exactly one :func:`qu_default`,
    and then close with :func:`qu_end_switch`.

    The branch key is interpreted from ``value`` (a collection of classical
    bits/registers) as a non-negative integer. Each case key must be unique
    within the same switch block.

    There are two common usage patterns.

    Case1: two or more cases + default + end

    .. code-block:: python

        # Let v be an instance of Registers class
        qu_switch(v)
        qu_case(v, 0)
        # define a circuit for the case when v == 0
        qu_case(v, 1)
        # define a circuit for the case when v == 1
        qu_default(v)
        # define a circuit for all other values
        qu_end_switch(v)

    Case2: nested switches are allowed

    .. code-block:: python

        qu_switch(v)
        qu_case(v, 3)
        # ...
        qu_case(v, 7)
        # You may nest another switch inside a case arm
        qu_switch(w)
        qu_case(w, 1)
        # ...
        qu_case(w, 2)
        # ...
        qu_default(w)
        # ...
        qu_end_switch(w)
        qu_default(v)
        # ...
        qu_end_switch(v)

    Note:
        The valid sequence is strictly:

        1) ``qu_switch(value)`` → 2) two or more ``qu_case(value, key)`` → \
        3) exactly one ``qu_default(value)`` → 4) ``qu_end_switch(value)``.

        Omitting ``qu_default``, using duplicate keys, calling ``qu_case`` after
        ``qu_default``, or mixing different ``value`` objects in one block is an error.

    See Also:
        :func:`qu_case`, :func:`qu_default`, :func:`qu_end_switch`

    Args:
        value (Registers): Register(s) whose bits form the branch key.
    """
    _M.qu_switch(value._impl)


def qu_case(value: Registers, key: int) -> None:
    """Add a case arm to the current switch block.

    Introduces a branch that is taken when ``value`` equals ``key``. May be
    called multiple times (keys must be unique). Must appear after the matching
    :func:`qu_switch` and before :func:`qu_default`.

    All gate/appending calls that follow this function belong to this case arm
    until the next :func:`qu_case`, :func:`qu_default`, or :func:`qu_end_switch`.

    Args:
        value (Registers): The same register(s) passed to the corresponding :func:`qu_switch`.
        key (int): Non-negative integer key to match; must be representable within
            the bit-width of ``value`` and be unique among cases in this block.
    """
    _M.qu_case(value._impl, key)


def qu_default(value: Registers) -> None:
    """Add the default (fallback) arm to the current switch block.

    The default arm is taken when none of the :func:`qu_case` keys match.
    Exactly one :func:`qu_default` is required in each switch block, and it
    must appear before :func:`qu_end_switch`.

    All gate/appending calls that follow this function belong to the default arm
    until :func:`qu_end_switch` is called.

    Args:
        value (Registers): The same register(s) passed to the corresponding :func:`qu_switch`.
    """
    _M.qu_default(value._impl)


def qu_end_switch(value: Registers) -> None:
    """End the current switch block.

    Closes the switch construct that was opened by :func:`qu_switch`.
    This must be called after adding **two or more** :func:`qu_case` arms and
    **exactly one** :func:`qu_default`.

    After this call, control flow continues after the switch block.

    Args:
        value (Registers): The same register(s) passed to the corresponding :func:`qu_switch`.
    """
    _M.qu_end_switch(value._impl)
