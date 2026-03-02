"""Circuit class."""

from __future__ import annotations

import copy
from collections import defaultdict
from dataclasses import dataclass
from enum import StrEnum
from itertools import starmap
from typing import ClassVar

import graphviz

Point = tuple[int, int]
NUM_RESERVED_CSYMBOLS = 10


def _sort_start_list(start_list: list[Point], points: set[Point]) -> None:
    """Sort `start_list` *in place* by increasing priority based on how many neighbors in `points` each start candidate has (4-neighborhood: up, right, down, left).

    Rationale:
        Start from candidates that are less connected to avoid them being stranded.

    Side effects:
        - Mutates `start_list` in place.

    Args:
        start_list (list[Point]): List of grid coordinates to try as path starts.
        points (set[Point]): Set of grid coordinates that can be used to form paths.
    """
    dxy = [(1, 0), (0, 1), (-1, 0), (0, -1)]
    cnt: dict[Point, int] = defaultdict(int)
    for p in points:
        for d in dxy:
            n = (p[0] + d[0], p[1] + d[1])
            if n in start_list:
                cnt[n] += 1

    def priority(p: Point) -> int:
        return cnt[p]

    start_list.sort(key=priority)


def _dfs(
    ret: list[list[Point]],
    remain: set[Point],
    current: list[Point],
    start_list: list[Point],
) -> None:
    """Greedy DFS-like path growth on a 4-neighborhood grid.

    Behavior:
        - If `current` is empty, pop from `start_list` until finding a point still in `remain`;
          start a new path with it.
        - Otherwise, extend `current` to one neighbor in `remain`. If multiple neighbors exist,
          prefer one that is *not* in `start_list` (to avoid consuming future starts).
        - When no extension is possible, finalize `current` into `ret` and start a new path.

    Side effects:
        - Mutates `start_list` (consumes from the head).
        - Mutates `remain` (removes visited points).
        - Appends paths to `ret`.

    Args:
        ret (list[list[Point]]): Accumulator for finished paths.
        remain (set[Point]): Points not yet assigned to any path.
        current (list[Point]): The path currently being extended.
        start_list (list[Point]): Start candidates; will be popped from the front.
    """
    dxy = [(1, 0), (0, 1), (-1, 0), (0, -1)]
    if len(current) == 0:
        if len(start_list) == 0:
            return None
        else:
            n = start_list.pop(0)
            if n in remain:
                current = [n]
                remain.discard(n)
            return _dfs(ret, remain, current, start_list)
    else:
        p = current[-1]
        ns = []
        for d in dxy:
            n = (p[0] + d[0], p[1] + d[1])
            if n in remain:
                ns.append(n)
        if len(ns) >= 2:
            for n in ns:
                if n not in start_list:
                    ns = [n]
                    break

        if len(ns) >= 1:
            n = ns[0]
            current.append(n)
            remain.discard(n)
            return _dfs(ret, remain, current, start_list)
        else:
            # p is the terminal of path
            # search next path
            ret.append(current)
            return _dfs(ret, remain, [], start_list)


def _terminal(ret: list[list[Point]], points: set[Point]) -> None:
    """Post-process `ret` by adding 2-point “terminal paths” from each path endpoint to any adjacent point in `points` that does not belong to that path.

    Note:
        - Possible duplicates are not de-duplicated.
        - Single-point paths consider only their (single) endpoint once.

    Side effects:
        - Extends `ret` with additional 2-point paths.

    Args:
        ret (list[list[Point]]): List of already found paths; will be extended.
        points (set[Point]): Universe of points on the grid.
    """
    dxy = [(1, 0), (0, 1), (-1, 0), (0, -1)]
    terminal_path = []
    for path in ret:
        tmp = set(path)
        s = path[0]
        for d in dxy:
            n = (s[0] + d[0], s[1] + d[1])
            if (n in points) and (n not in tmp):
                terminal_path.append([s, n])
        if len(path) == 1:
            continue
        e = path[-1]
        for d in dxy:
            n = (e[0] + d[0], e[1] + d[1])
            if (n in points) and (n not in tmp):
                terminal_path.append([e, n])
    ret.extend(terminal_path)


def make_paths(points: set[Point], start_list: list[Point]) -> list[list[Point]]:
    """Build paths on a 4-neighborhood grid from `points`, starting (in order) from `start_list`.

    Steps:
        1) Sort `start_list` by increasing neighbor count within `points`.
        2) Greedily grow non-branching paths by consuming `points`.
        3) Add 2-point terminal paths from endpoints to adjacent points outside the path.

    Side effects:
        - Mutates `start_list` (sorted and popped).

    Returns:
        List of paths, where each path is a list of coordinates (tuples).
    """
    ret = []
    _sort_start_list(start_list, points)
    _dfs(ret, copy.copy(points), [], start_list)
    _terminal(ret, points)
    return ret


@dataclass
class FTQCConfig:
    machine_type: str
    topology_count: int
    max_x: int
    max_y: int
    use_magic_state_cultivation: bool
    magic_factory_seed_offset: int
    magic_generation_period: int
    prob_magic_state_creation: float
    maximum_magic_state_stock: int
    reaction_time: int

    def __init__(self, config: dict) -> None:
        machine_type: str = config["machine_option"]["type"]
        if machine_type.lower() not in {
            "dim2",
            "dim3",
            "distributeddim2",
            "distributeddim3",
        }:
            msg = (
                "machine_option.type must be one of "
                "Dim2, Dim3, DistributedDim2, DistributedDim3"
            )
            raise ValueError(msg)

        self.machine_type = machine_type
        self.topology_count = len(config.get("topology", []))
        self.max_x = config["topology"][0]["coord"][0]
        self.max_y = config["topology"][0]["coord"][1]
        self.use_magic_state_cultivation = config["machine_option"]["use_magic_state_cultivation"]
        self.magic_factory_seed_offset = config["machine_option"]["magic_factory_seed_offset"]
        self.magic_generation_period = config["machine_option"]["magic_generation_period"]
        self.prob_magic_state_creation = config["machine_option"]["prob_magic_state_creation"]
        self.maximum_magic_state_stock = config["machine_option"]["maximum_magic_state_stock"]
        self.entanglement_generation_period = config["machine_option"]["entanglement_generation_period"]
        self.maximum_entangled_state_stock = config["machine_option"]["maximum_entangled_state_stock"]
        self.reaction_time = config["machine_option"]["reaction_time"]


@dataclass
class QubitInfo:
    id: int
    x: int
    y: int
    begin: int
    end: int


@dataclass
class FactoryInfo:
    id: int
    x: int
    y: int
    begin: int


class InstructionType(StrEnum):
    allocate = "ALLOCATE"
    allocate_2q = "ALLOCATE_2Q"
    allocate_magic_factory = "ALLOCATE_MAGIC_FACTORY"
    allocate_entanglement_factory = "ALLOCATE_ENTANGLEMENT_FACTORY"
    deallocate = "DEALLOCATE"
    init_zx = "INIT_ZX"
    meas_zx = "MEAS_ZX"
    meas_y = "MEAS_Y"
    twist = "TWIST"
    hadamard = "HADAMARD"
    rotate = "ROTATE"
    lattice_surgery = "LATTICE_SURGERY"
    lattice_surgery_magic = "LATTICE_SURGERY_MAGIC"
    lattice_surgery_multinode = "LATTICE_SURGERY_MULTINODE"
    move = "MOVE"
    move_magic = "MOVE_MAGIC"
    move_entanglement = "MOVE_ENTANGLEMENT"
    cnot = "CNOT"
    cnot_trans = "CNOT_TRANS"
    swap_trans = "SWAP_TRANS"
    move_trans = "MOVE_TRANS"
    xor = "XOR"
    and_ = "AND"
    or_ = "OR"
    probability_hint = "PROBABILITY_HINT"
    await_correction = "AWAIT_CORRECTION"


class Instruction:
    latency_tbl: ClassVar[dict[InstructionType, int]] = {
        InstructionType.allocate: 0,
        InstructionType.allocate_2q: 0,
        InstructionType.allocate_magic_factory: 0,
        InstructionType.allocate_entanglement_factory: 0,
        InstructionType.deallocate: 0,
        InstructionType.init_zx: 0,
        InstructionType.meas_zx: 0,
        InstructionType.meas_y: 2,
        InstructionType.twist: 2,
        InstructionType.hadamard: 0,
        InstructionType.rotate: 3,
        InstructionType.lattice_surgery: 1,
        InstructionType.lattice_surgery_magic: 1,
        InstructionType.lattice_surgery_multinode: 1,
        InstructionType.move: 1,
        InstructionType.move_magic: 1,
        InstructionType.move_entanglement: 1,
        InstructionType.cnot: 2,
        InstructionType.cnot_trans: 0,
        InstructionType.swap_trans: 0,
        InstructionType.move_trans: 0,
        InstructionType.xor: 0,
        InstructionType.and_: 0,
        InstructionType.or_: 0,
        InstructionType.probability_hint: 0,
        InstructionType.await_correction: 0,
    }
    start_correcting_tbl: ClassVar[dict[InstructionType, int]] = {
        InstructionType.allocate: 0,
        InstructionType.allocate_2q: 0,
        InstructionType.allocate_magic_factory: 0,
        InstructionType.allocate_entanglement_factory: 0,
        InstructionType.deallocate: 0,
        InstructionType.init_zx: 0,
        InstructionType.meas_zx: 0,
        InstructionType.meas_y: 1,
        InstructionType.twist: 0,
        InstructionType.hadamard: 0,
        InstructionType.rotate: 0,
        InstructionType.lattice_surgery: 0,
        InstructionType.lattice_surgery_magic: 0,
        InstructionType.lattice_surgery_multinode: 0,
        InstructionType.move: 0,
        InstructionType.move_magic: 0,
        InstructionType.move_entanglement: 0,
        InstructionType.cnot: 0,
        InstructionType.cnot_trans: 0,
        InstructionType.swap_trans: 0,
        InstructionType.move_trans: 0,
        InstructionType.xor: 0,
        InstructionType.and_: 0,
        InstructionType.or_: 0,
        InstructionType.probability_hint: 0,
        InstructionType.await_correction: 0,
    }

    def __init__(self, index: int, inst: dict) -> None:
        """Construct Instruction."""
        self._index = index
        self._raw = inst
        self._type = InstructionType(inst["type"])
        self._qtarget = inst["qtarget"]
        self._ccreate = inst.get("ccreate", [])
        self._cdepend = inst.get("cdepend", [])
        self._mtarget = inst["mtarget"]
        self._etarget = inst.get("etarget", [])
        self._ehtarget = inst.get("ehtarget", [])
        self._condition = inst["condition"]
        self._aux = inst.get("ancilla", [])
        self._terminal = []
        self._paths: list[list[Point]] = []
        self._beat = inst["metadata"]["beat"]
        self._parents: list[Instruction] = []
        self._children: list[Instruction] = []

    def _set_aux(self, c: Circuit) -> None:
        self._aux, self._terminal = self._set_aux_impl(self.aux, c)
        self._paths = self._calc_paths()

    def _set_aux_impl(self, aux: list, c: Circuit) -> tuple[list[Point], list[Point]]:
        if self.type in {
            InstructionType.allocate,
            InstructionType.deallocate,
            InstructionType.init_zx,
            InstructionType.meas_zx,
            InstructionType.hadamard,
        }:
            q = c.get_qubit_info(self.qtarget[0])
            return [(q.x, q.y)], []
        elif self.type == InstructionType.allocate_2q:
            q0 = c.get_qubit_info(self.qtarget[0])
            q1 = c.get_qubit_info(self.qtarget[1])
            return [(q0.x, q0.y), (q1.x, q1.y)], []
        elif self.type == InstructionType.allocate_magic_factory:
            f = c.get_factory_info(self.mtarget[0])
            return [(f.x, f.y)], []
        elif self.type == InstructionType.allocate_entanglement_factory:
            d1 = self._raw["dest1"]
            d2 = self._raw["dest2"]
            return [(d1[0], d1[1]), (d2[0], d2[1])], [(d1[0], d1[1]), (d2[0], d2[1])]
        elif self.type == InstructionType.meas_y:
            q = c.get_qubit_info(self.qtarget[0])
            d = self._raw["dir"]
            if d == 0:
                return [(q.x, q.y), (q.x + 1, q.y), (q.x + 1, q.y + 1), (q.x, q.y + 1)], []
            elif d == 1:
                return [(q.x, q.y), (q.x, q.y + 1), (q.x - 1, q.y + 1), (q.x - 1, q.y)], []
            elif d == 2:
                return [(q.x, q.y), (q.x + 1, q.y), (q.x + 1, q.y - 1), (q.x, q.y - 1)], []
            elif d == 3:
                return [(q.x, q.y), (q.x, q.y - 1), (q.x - 1, q.y - 1), (q.x - 1, q.y)], []
        elif self.type in {InstructionType.twist, InstructionType.rotate}:
            q = c.get_qubit_info(self.qtarget[0])
            d = self._raw["dir"]
            if d == 0:
                return [(q.x, q.y), (q.x + 1, q.y)], []
            if d == 1:
                return [(q.x, q.y), (q.x, q.y + 1)], []
            if d == 2:
                return [(q.x, q.y), (q.x - 1, q.y)], []
            if d == 3:
                return [(q.x, q.y), (q.x, q.y - 1)], []
        elif self.type in {InstructionType.lattice_surgery, InstructionType.cnot}:
            qs = [c.get_qubit_info(q) for q in self.qtarget]
            terminal = [(q.x, q.y) for q in qs]
            return terminal + [(p[0], p[1]) for p in aux], terminal
        elif self.type == InstructionType.lattice_surgery_magic:
            qs = [c.get_qubit_info(q) for q in self.qtarget]
            f = c.get_factory_info(self.mtarget[0])
            terminal = [(q.x, q.y) for q in qs] + [(f.x, f.y)]
            return terminal + [(p[0], p[1]) for p in aux], terminal
        elif self.type == InstructionType.lattice_surgery_multinode:
            qs = [c.get_qubit_info(q) for q in self.qtarget]
            terminal = [(q.x, q.y) for q in qs]
            if self.mtarget:
                f = c.get_factory_info(self.mtarget[0])
                terminal.append((f.x, f.y))
            return terminal + [(p[0], p[1]) for p in aux], terminal
        elif self.type == InstructionType.move:
            q = c.get_qubit_info(self.qtarget[0])
            d = self._raw["dest"]
            terminal = [(q.x, q.y), (d[0], d[1])]
            return terminal + [(p[0], p[1]) for p in aux], terminal
        elif self.type == InstructionType.move_magic:
            q = c.get_qubit_info(self.qtarget[0])
            f = c.get_factory_info(self.mtarget[0])
            terminal = [(q.x, q.y), (f.x, f.y)]
            return terminal + [(p[0], p[1]) for p in aux], terminal
        elif self.type == InstructionType.move_entanglement:
            q = c.get_qubit_info(self.qtarget[0])
            terminal = [(q.x, q.y)]
            return terminal + [(p[0], p[1]) for p in aux], terminal
        elif self.type in {
            InstructionType.cnot_trans,
            InstructionType.swap_trans,
            InstructionType.move_trans,
        }:
            qs = [c.get_qubit_info(q) for q in self.qtarget]
            terminal = [(q.x, q.y) for q in qs]
            return terminal + [(p[0], p[1]) for p in aux], terminal
        return [], []

    def num_parents(self) -> int:
        return len(self._parents)

    def num_children(self) -> int:
        return len(self._children)

    def get_raw(self) -> dict:
        return self._raw

    @property
    def type(self) -> InstructionType:
        return self._type

    @property
    def latency(self) -> int:
        return Instruction.latency_tbl[self.type]

    @property
    def start_correcting(self) -> int:
        return Instruction.start_correcting_tbl[self.type]

    @property
    def qtarget(self) -> list[int]:
        return self._qtarget

    @property
    def ccreate(self) -> list[int]:
        return self._ccreate

    @property
    def cdepend(self) -> list[int]:
        return self._cdepend

    @property
    def mtarget(self) -> list[int]:
        return self._mtarget

    @property
    def etarget(self) -> list[int]:
        return self._etarget

    @property
    def ehtarget(self) -> list[int]:
        return self._ehtarget

    @property
    def condition(self) -> list[int]:
        return self._condition

    @property
    def aux(self) -> list[Point]:
        return self._aux

    @property
    def terminal(self) -> list[Point]:
        return self._terminal

    @property
    def parents(self) -> list[Instruction]:
        return self._parents

    @property
    def children(self) -> list[Instruction]:
        return self._children

    @property
    def beat(self) -> int:
        return self._beat

    @property
    def index(self) -> int:
        return self._index

    def _calc_paths(self) -> list[list[Point]]:
        if self.type in {
            InstructionType.meas_y,
            InstructionType.twist,
            InstructionType.rotate,
        }:
            return [self.aux]
        elif self.type in {
            InstructionType.lattice_surgery,
            InstructionType.lattice_surgery_magic,
            InstructionType.lattice_surgery_multinode,
            InstructionType.move,
            InstructionType.move_magic,
            InstructionType.move_entanglement,
            InstructionType.cnot,
            InstructionType.cnot_trans,
            InstructionType.swap_trans,
            InstructionType.move_trans,
        }:
            return make_paths(set(self.aux), list(self.terminal))
        return []

    def get_paths(self) -> list[list[Point]]:
        return self._paths

    def __str__(self) -> str:
        return str(self._raw["raw"])


def set_dependency(insts: list[Instruction]) -> None:
    qmap: dict[int, Instruction] = {}
    cmap: dict[int, Instruction] = {}

    for inst in insts:
        qs = inst.qtarget
        for q in qs:
            if q in qmap:
                parent = qmap[q]
                parent._children.append(inst)
                inst._parents.append(parent)
            qmap[q] = inst
        cons = inst.condition
        for con in cons:
            if con in cmap:
                parent = cmap[con]
                parent._children.append(inst)
                inst._parents.append(parent)
            elif con < NUM_RESERVED_CSYMBOLS:
                continue
            else:
                print("[WARNING] condition's csymbol is not generated in machine function", con, inst)
        for dep in inst.cdepend:
            if dep in cmap:
                parent = cmap[dep]
                parent._children.append(inst)
                inst._parents.append(parent)
            elif dep < NUM_RESERVED_CSYMBOLS:
                continue
            else:
                print("[WARNING] cdepend csymbol is not generated in machine function", dep, inst)
        for c in inst.ccreate:
            if c in cmap:
                parent = cmap[c]
                parent._children.append(inst)
                inst._parents.append(parent)
            cmap[c] = inst


class Circuit:
    def __init__(self, insts: list[dict]) -> None:
        """Construct circuit."""
        # Drop unknown instructions
        new_insts = []
        for inst in insts:
            try:
                InstructionType(inst["type"])
                new_insts.append(inst)
            except:  # noqa: PERF203
                print("[WARNING] Skip unknown type of instruction:", inst)
        insts = new_insts

        self._insts = list(starmap(Instruction, enumerate(insts)))
        self._begin_beat = min(inst.beat for inst in self._insts)
        self._end_beat = max(inst.beat for inst in self._insts) + 1
        self._beat_to_insts: defaultdict[int, list[Instruction]] = defaultdict(list)
        for inst in self._insts:
            self._beat_to_insts[inst.beat].append(inst)
        set_dependency(self._insts)

        self._qubits: dict[int, QubitInfo] = {}
        self._factories: dict[int, FactoryInfo] = {}
        for inst in self._insts:
            if inst.type == InstructionType.allocate:
                qid = inst.qtarget[0]
                x = inst.get_raw()["dest"][0]
                y = inst.get_raw()["dest"][1]
                self._qubits[qid] = QubitInfo(qid, x, y, inst.beat, self._end_beat)
            elif inst.type == InstructionType.allocate_magic_factory:
                fid = inst.mtarget[0]
                x = inst.get_raw()["dest"][0]
                y = inst.get_raw()["dest"][1]
                self._factories[fid] = FactoryInfo(fid, x, y, inst.beat)
            elif inst.type == InstructionType.allocate_2q:
                raise NotImplementedError
            elif inst.type == InstructionType.deallocate:
                qid = inst.qtarget[0]
                if qid in self._qubits:
                    self._qubits[qid].end = inst.beat

        for inst in self._insts:
            inst._set_aux(self)

    def num_insts(self) -> int:
        return len(self._insts)

    def get_inst(self, index: int) -> Instruction:
        return self._insts[index]

    def insts_of_beat(self, beat: int) -> list[Instruction]:
        return self._beat_to_insts[beat]

    def qubits_of_beat(self, beat: int) -> list[QubitInfo]:
        return [qubit for qubit in self._qubits.values() if qubit.begin <= beat and beat < qubit.end]

    def factories_of_beat(self, beat: int) -> list[FactoryInfo]:
        return [factory for factory in self._factories.values() if factory.begin <= beat]

    def num_qubits(self) -> int:
        return len(self._qubits)

    def get_qubits(self) -> dict[int, QubitInfo]:
        return self._qubits

    def get_qubit_info(self, id: int) -> QubitInfo:
        return self._qubits[id]

    def num_factories(self) -> int:
        return len(self._factories)

    def get_factories(self) -> dict[int, FactoryInfo]:
        return self._factories

    def get_factory_info(self, id: int) -> FactoryInfo:
        return self._factories[id]

    @property
    def begin_beat(self) -> int:
        return self._begin_beat

    @property
    def end_beat(self) -> int:
        return self._end_beat

    def __iter__(self):
        yield from self._insts


def construct_graph(
    circuit: Circuit,
    begin_beat: int,
    end_beat: int,
    current_beat: int,
) -> graphviz.Digraph:
    graph = graphviz.Digraph(graph_attr={"rankdir": "TB"}, node_attr={"shape": "box"})

    nodes: dict[int, graphviz.Node] = {}
    for beat in range(begin_beat, end_beat):
        insts = circuit.insts_of_beat(beat)
        if len(insts) > 0:
            with graph.subgraph() as s:
                for inst in insts:
                    color = "black"
                    if inst.beat == current_beat:
                        color = "red"
                    elif inst.beat < current_beat and current_beat < inst.beat + inst.latency:
                        color = "green"
                    elif inst.beat < current_beat:
                        color = "blue"
                    nodes[inst.index] = s.node(
                        str(inst.index),
                        label=f"[{inst.index}] {inst.type!s}",
                        tooltip=str(inst),
                        color=color,
                    )

    edges: dict[Point, graphviz.Edge] = {}
    for beat in range(begin_beat, end_beat):
        insts = circuit.insts_of_beat(beat)
        for inst in insts:
            for child in inst.children:
                if child.index in nodes:
                    edges[inst.index, child.index] = graph.edge(str(inst.index), str(child.index))

    return graph
