"""Decompose toffoli."""

import argparse
import json
from pathlib import Path


def decompose_inst(inst: dict) -> list[dict]:
    ret = []
    if inst["opcode"] == "CCX":
        print("Decompose: ", inst)
        # inst is Toffoli
        q0 = inst["q0"]
        q1 = inst["q1"]
        q2 = inst["q2"]

        ret.extend((
            {"opcode": "H", "q": q0},
            {"opcode": "CX", "q0": q0, "q1": q1},
            {"opcode": "TDag", "q": q0},
            {"opcode": "CX", "q0": q0, "q1": q2},
            {"opcode": "T", "q": q0},
            {"opcode": "CX", "q0": q0, "q1": q1},
            {"opcode": "TDag", "q": q0},
            {"opcode": "CX", "q0": q0, "q1": q2},
            {"opcode": "T", "q": q0},
            {"opcode": "T", "q": q1},
            {"opcode": "H", "q": q0},
            {"opcode": "CX", "q0": q1, "q1": q2},
            {"opcode": "TDag", "q": q1},
            {"opcode": "T", "q": q2},
            {"opcode": "CX", "q0": q1, "q1": q2},
        ))
    else:
        ret = [inst]

    return ret


def decompose(input_module: dict, circuit_name: str) -> dict:
    for circuit in input_module["circuit_list"]:
        if circuit["name"] != circuit_name:
            continue

        for bb in circuit["bb_list"]:
            new_inst_list = []
            for inst in bb["inst_list"]:
                new_inst_list.extend(decompose_inst(inst))
            bb["inst_list"] = new_inst_list
    return input_module


def run(ipath: Path, opath: Path, circuit_name: str) -> None:
    with ipath.open("r", encoding="utf-8") as ifile:
        input_module = json.load(ifile)

    output_module = decompose(input_module, circuit_name)

    with opath.open("w", encoding="utf-8") as ofile:
        json.dump(output_module, ofile)


def main() -> None:
    parser = argparse.ArgumentParser()

    parser.add_argument("input", type=str)
    parser.add_argument("output", type=str)
    parser.add_argument("circuit", type=str)

    args = parser.parse_args()

    run(Path(args.input), Path(args.output), args.circuit)


if __name__ == "__main__":
    main()
