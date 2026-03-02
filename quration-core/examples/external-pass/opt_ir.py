"""Optimize IR."""

import argparse
import json
from pathlib import Path


def opt_impl(circuit: dict) -> None:
    with Path.open(Path("tmp.json"), "w") as ofile:
        json.dump(circuit, ofile)


def opt(input_file: str, circuit_name: str, output_file: str) -> None:
    with Path.open(Path(input_file), "r") as ifile:
        ir = json.load(ifile)
    for circuit in ir["circuit_list"]:
        if circuit["name"] == circuit_name:
            opt_impl(circuit)
    with Path.open(Path(output_file), "w") as ofile:
        json.dump(ir, ofile)


def main() -> None:
    parser = argparse.ArgumentParser()

    parser.add_argument("input", type=str)
    parser.add_argument("circuit", type=str)
    parser.add_argument("output", type=str)

    args = parser.parse_args()

    opt(args.input, args.circuit, args.output)


if __name__ == "__main__":
    main()
