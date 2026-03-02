"""Mapping."""

from __future__ import annotations

import argparse
import itertools
import json
import math
from collections import defaultdict
from copy import deepcopy
from dataclasses import dataclass
from pathlib import Path

from ortools.sat.python import cp_model


@dataclass
class Graph:
    n: list[int]  # node ids
    c: list[int]  # capacity
    e: dict[tuple[int, int], int]  # pair of nodes to its weight


def get_key(a: int, b: int) -> tuple[int, int]:
    if a <= b:
        return (a, b)
    else:
        return (b, a)


def warshall_floyd(g: Graph) -> Graph:
    e = deepcopy(g.e)
    for k in g.n:
        for i in g.n:
            for j in g.n:
                if k == i or i == j or j == k:  # noqa: PLR1714
                    continue

                key_ki = get_key(i, k)
                key_kj = get_key(k, j)
                key_ij = get_key(i, j)
                if key_ki in e and key_kj in e:
                    if key_ij in e:
                        e[key_ij] = min(e[key_ij], e[key_ki] + e[key_kj])
                    else:
                        e[key_ij] = e[key_ki] + e[key_kj]

    return Graph(g.n, g.c, e)


def min_cost_hom(g: Graph, t: Graph) -> dict[int, int]:  # (node of g) to (node of t)
    print(g.n, g.e)
    print(list(zip(t.n, t.c)), t.e)

    # --- CP-SAT model ---
    m = cp_model.CpModel()
    x = {(i, a): m.NewBoolVar(f"x[{i},{a}]") for i in g.n for a in t.n}

    # each i maps to exactly one a
    for i in g.n:
        m.Add(sum(x[i, a] for a in t.n) == 1)

    # each a used at most c (capacity)
    for a, c in zip(t.n, t.c):
        m.Add(sum(x[i, a] for i in g.n) <= c)

    # forbid mapping a G-edge to a non-edge in T (allow self pairs)
    for i, j in g.e:
        for a, b in itertools.permutations(t.n, 2):
            if get_key(a, b) not in t.e:
                # forbid x[i,a] = 1 AND x[j,b] = 1
                m.AddForbiddenAssignments([x[i, a], x[j, b]], [(1, 1)])

    # linearize with z_{ijab} = x[i,a] AND x[j,b], only if (a,b) is an edge in T
    terms = []
    for i, j in g.e:
        for a, b in itertools.permutations(t.n, 2):
            if get_key(a, b) in t.e:
                z = m.NewBoolVar(f"z[{i},{j},{a},{b}]")
                # z <= x[i,a], z <= x[j,b], z >= x[i,a]+x[j,b]-1
                m.AddImplication(z, x[i, a])
                m.AddImplication(z, x[j, b])
                m.AddBoolOr([z, x[i, a].Not(), x[j, b].Not()])  # z >= x[i,a]+x[j,b]-1
                w = g.e[i, j] * t.e[get_key(a, b)]
                terms.append(w * z)

    m.Minimize(sum(terms) if terms else 0)

    solver = cp_model.CpSolver()
    solver.parameters.max_time_in_seconds = 60
    solver.parameters.num_search_workers = 8

    status = solver.Solve(m)
    if status not in {cp_model.OPTIMAL, cp_model.FEASIBLE}:
        msg = "No feasible mapping found."
        raise RuntimeError(msg)

    print(solver.value(sum(terms)))
    print("Bound:", solver.best_objective_bound)

    mp = {}
    for i in g.n:
        for a in t.n:
            if solver.Value(x[i, a]) == 1:
                mp[i] = a
                break
    return mp


def get_topology_graph(topology: dict) -> tuple[Graph, dict[int, list[tuple[int, int]]]] | None:
    qubit_place_candidate: dict[int, list[tuple[int, int]]] = {}
    entanglement_factory: dict[int, tuple[int, int]] = {}  # ef to (plane, pair ef)
    for plane in topology:
        if plane["type"] != "plane":
            return None
        z = plane["coord"][2]
        qubit_place_candidate[z] = [(c["coord"][0], c["coord"][1]) for c in plane["qubit"]]
        for e in plane["entanglement_factory"]:
            entanglement_factory[e["symbol"]] = (z, e["pair"])
    plane_list = sorted(qubit_place_candidate.keys())
    capacity = [len(qubit_place_candidate[z]) for z in plane_list]
    plane_connection: dict[tuple[int, int], int] = defaultdict(int)  # pair of plane to its connection weight
    for e, v in entanglement_factory.items():
        z, pair = v
        pair_z, pair_pair = entanglement_factory[pair]
        assert e == pair_pair
        if z >= pair_z:
            z, pair_z = pair_z, z
        assert z <= pair_z
        plane_connection[z, pair_z] += 1
    lcm = math.lcm(*plane_connection.values())
    for k, v in plane_connection.items():
        plane_connection[k] = lcm // v
    return (warshall_floyd(Graph(plane_list, capacity, plane_connection)), qubit_place_candidate)


def get_qubit_graph(program: dict) -> tuple[Graph, dict[int, int]]:
    qubit_list: dict[int, int] = {}  # qubit index to index of allocation instruction
    qubit_connection: dict[tuple[int, int], int] = defaultdict(int)  # pair of qubit to its connection weight
    for index, inst in enumerate(program):
        inst_type = inst["type"]
        if inst_type == "ALLOCATE":
            qubit_list[inst["qtarget"][0]] = index
        if inst_type == "LATTICE_SURGERY_MULTINODE":
            qt = sorted(inst["qtarget"])
            for q1, q2 in itertools.combinations(qt, 2):
                qubit_connection[q1, q2] += 1
        if inst_type == "CNOT":
            qt = sorted(inst["qtarget"])
            for q1, q2 in itertools.combinations(qt, 2):
                qubit_connection[q1, q2] += 2
    return Graph(sorted(qubit_list.keys()), [], qubit_connection), qubit_list


def mapping(input_program: dict) -> dict:
    parameter = input_program["parameter"]

    if parameter["target"]["machine_option"]["type"] != "DistributedDim2":
        return input_program

    topology = get_topology_graph(parameter["target"]["topology"])
    if topology is None:
        return input_program

    plane_graph, qubit_place_candidate = topology

    qubit_graph, qubit_list = get_qubit_graph(input_program["program"])

    mp = min_cost_hom(qubit_graph, plane_graph)

    for q, index in qubit_list.items():
        plane = mp[q]
        coord = qubit_place_candidate[plane][0]
        input_program["program"][index]["dest"] = [coord[0], coord[1], plane]
        input_program["program"][index]["metadata"]["z"] = plane

        del qubit_place_candidate[plane][0]

    return input_program


def run(ipath: Path, opath: Path) -> None:
    with ipath.open("r", encoding="utf-8") as ifile:
        input_program = json.load(ifile)

    try:
        output_program = mapping(input_program)
    except Exception as e:
        print("Mapping error:", e)
        output_program = input_program

    with opath.open("w", encoding="utf-8") as ofile:
        json.dump(output_program, ofile)


def main() -> None:
    parser = argparse.ArgumentParser()

    parser.add_argument("input", type=str)
    parser.add_argument("output", type=str)

    args = parser.parse_args()

    run(Path(args.input), Path(args.output))


if __name__ == "__main__":
    main()
