#!/usr/bin/env python3
"""Visualize a SC_LS_FIXED_V0 pipeline state at one beat."""

from __future__ import annotations

import argparse
import json
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import matplotlib.pyplot as plt
from matplotlib import patches

INSTRUCTION_LATENCY = {
    "ALLOCATE": 0,
    "ALLOCATE_2Q": 0,
    "ALLOCATE_MAGIC_FACTORY": 0,
    "ALLOCATE_ENTANGLEMENT_FACTORY": 0,
    "DEALLOCATE": 0,
    "INIT_ZX": 0,
    "MEAS_ZX": 0,
    "MEAS_Y": 2,
    "TWIST": 2,
    "ROTATE": 3,
    "HADAMARD": 0,
    "LATTICE_SURGERY": 1,
    "LATTICE_SURGERY_MAGIC": 1,
    "LATTICE_SURGERY_MULTINODE": 1,
    "MOVE": 1,
    "MOVE_MAGIC": 1,
    "MOVE_ENTANGLEMENT": 1,
    "CNOT": 2,
    "CNOT_TRANS": 0,
    "SWAP_TRANS": 0,
    "MOVE_TRANS": 0,
    "XOR": 0,
    "AND": 0,
    "OR": 0,
    "PROBABILITY_HINT": 0,
    "AWAIT_CORRECTION": 0,
}

CELL_SIZE = 1.0
CELL_HALF = CELL_SIZE / 2
CELL_OFFSET = 0.5
GRID_MARGIN = 0.45


def format_dir(dir_value: int | None) -> str:
    if dir_value == 0:
        return "Z:L-R X:U-D"
    if dir_value == 1:
        return "Z:U-D X:L-R"
    return "-"


def to_canvas(x: int, y: int) -> tuple[float, float]:
    """Translate logical coords to matplotlib canvas coordinates (cell centers)."""
    return x + CELL_OFFSET, y + CELL_OFFSET


@dataclass
class QubitSpan:
    qid: int
    x: int
    y: int
    begin: int
    end: int | None
    dir: int | None


@dataclass
class Factory:
    kind: str
    id: int | None
    x: int
    y: int


def parse_int(value: Any) -> int | None:
    if isinstance(value, bool):
        return None
    if isinstance(value, int):
        return value
    if isinstance(value, float) and value.is_integer():
        return int(value)
    if isinstance(value, str):
        try:
            return int(value)
        except ValueError:
            return None
    return None


def parse_coord(raw: Any) -> tuple[int, int] | None:
    if not isinstance(raw, (list, tuple)) or len(raw) < 2:
        return None
    x = parse_int(raw[0])
    y = parse_int(raw[1])
    if x is None or y is None:
        return None
    return x, y


def parse_beat(raw: dict[str, Any]) -> int | None:
    metadata = raw.get("metadata") if isinstance(raw, dict) else None
    if not isinstance(metadata, dict):
        return None
    return parse_int(metadata.get("beat"))


def instruction_duration(inst_type: str) -> int:
    return max(int(INSTRUCTION_LATENCY.get(inst_type, 1)), 1)


def update_qubit_lifetimes(
    program: list[dict[str, Any]],
    *,
    mapping_only: bool,
) -> tuple[list[QubitSpan], int, int]:
    open_spans: dict[int, QubitSpan] = {}
    spans: list[QubitSpan] = []
    max_beat = 0
    min_beat = 0
    for inst in program:
        inst_type = str(inst.get("type", ""))
        if inst_type != "ALLOCATE" and inst_type != "DEALLOCATE":
            continue

        q_targets = inst.get("qtarget", [])
        if not isinstance(q_targets, list) or len(q_targets) == 0:
            continue
        qid = parse_int(q_targets[0])
        if qid is None:
            continue

        if mapping_only and inst_type == "DEALLOCATE":
            # In mapping-only mode there is no schedule yet, so DEALLOCATE should
            # not immediately close the lifetime at beat 0.
            continue

        beat = parse_beat(inst)
        if beat is None:
            if not mapping_only:
                continue
            beat = 0

        if inst_type == "ALLOCATE":
            coord = parse_coord(inst.get("dest", []))
            if coord is None:
                continue
            if qid in open_spans:
                prev = open_spans.pop(qid)
                prev.end = beat
                spans.append(prev)
            raw_dir = parse_int(inst.get("dir"))
            open_spans[qid] = QubitSpan(
                qid,
                coord[0],
                coord[1],
                beat,
                None,
                raw_dir & 1 if raw_dir is not None else None,
            )
            max_beat = max(max_beat, beat)
            min_beat = min(min_beat, beat) if spans or open_spans else beat
        else:
            existing = open_spans.get(qid)
            if existing is None:
                continue
            existing.end = beat
            if beat < existing.begin:
                existing.end = existing.begin + 1
            spans.append(open_spans.pop(qid))
            max_beat = max(max_beat, beat)
            min_beat = min(min_beat, beat)

    for q_span in open_spans.values():
        spans.append(q_span)

    if spans:
        min_beat = min(min_beat, min(span.begin for span in spans))
        max_beat = max(max_beat, max(span.begin for span in spans))
    return spans, min_beat, max_beat


def collect_factories(parameter_target: dict[str, Any], program: list[dict[str, Any]]) -> list[Factory]:
    factories: dict[tuple[str, int, int, int], Factory] = {}

    topology = parameter_target.get("topology", [])
    if isinstance(topology, dict):
        topology = topology.get("grids", topology)
    if not isinstance(topology, list):
        topology = []
    for item in topology:
        magic = item.get("magic_factory", [])
        if isinstance(magic, list):
            for mf in magic:
                if not isinstance(mf, dict):
                    continue
                fid = parse_int(mf.get("symbol"))
                coord = parse_coord(mf.get("coord", []))
                if coord is None:
                    continue
                factories[("magic", fid or -1, coord[0], coord[1])] = Factory("magic", fid, coord[0], coord[1])

    for inst in program:
        inst_type = str(inst.get("type", ""))
        if inst_type == "ALLOCATE_MAGIC_FACTORY":
            mtarget = inst.get("mtarget", [])
            fid = parse_int(mtarget[0]) if mtarget else None
            coord = parse_coord(inst.get("dest", []))
            if coord is None:
                continue
            factories[("magic", fid or -1, coord[0], coord[1])] = Factory("magic", fid, coord[0], coord[1])
        elif inst_type == "ALLOCATE_ENTANGLEMENT_FACTORY":
            d1 = parse_coord(inst.get("dest1", []))
            d2 = parse_coord(inst.get("dest2", []))
            if d1 is not None:
                factories[("ent", -1, d1[0], d1[1])] = Factory("entanglement", None, d1[0], d1[1])
            if d2 is not None:
                factories[("ent", -2, d2[0], d2[1])] = Factory("entanglement", None, d2[0], d2[1])
    return list(factories.values())


def collect_running_instructions(program: list[dict[str, Any]], beat: int) -> list[dict[str, Any]]:
    running: list[dict[str, Any]] = []
    for idx, inst in enumerate(program):
        inst_type = str(inst.get("type", ""))
        start = parse_beat(inst)
        if start is None:
            continue
        dur = instruction_duration(inst_type)
        if start <= beat < start + dur:
            ancilla = []
            for coord in inst.get("ancilla", []):
                p = parse_coord(coord)
                if p is not None:
                    ancilla.append(p)
            path = []
            for coord in inst.get("path", []):
                p = parse_coord(coord)
                if p is not None:
                    path.append(p)
            qtarget = [parse_int(q) for q in inst.get("qtarget", []) if parse_int(q) is not None]
            conditions = [parse_int(c) for c in inst.get("condition", []) if parse_int(c) is not None]
            running.append({
                "index": idx,
                "type": inst_type,
                "start": start,
                "dur": dur,
                "raw": str(inst.get("raw", "")),
                "qtarget": qtarget,
                "condition": conditions,
                "ancilla": ancilla,
                "dir": parse_int(inst.get("dir")),
                "dest": parse_coord(inst.get("dest", [])),
                "path": path,
            })
    return running


def collect_active_qubits(
    spans: list[QubitSpan],
    beat: int,
    default_end: int,
) -> list[QubitSpan]:
    active: list[QubitSpan] = []
    for span in spans:
        end = span.end if span.end is not None else default_end
        if span.begin <= beat < end:
            active.append(span)
    return active


def apply_dynamic_dirs(
    active_qubits: list[QubitSpan],
    program: list[dict[str, Any]],
    beat: int,
) -> list[QubitSpan]:
    current = {q.qid: QubitSpan(q.qid, q.x, q.y, q.begin, q.end, q.dir) for q in active_qubits}
    for inst in program:
        start = parse_beat(inst)
        if start is None or start > beat:
            continue
        if str(inst.get("type", "")) not in {"HADAMARD", "ROTATE"}:
            continue
        qtarget = inst.get("qtarget", [])
        if not isinstance(qtarget, list) or len(qtarget) == 0:
            continue
        qid = parse_int(qtarget[0])
        if qid is None or qid not in current or current[qid].dir is None:
            continue
        current[qid].dir ^= 1
    return list(current.values())


def collect_bounds(
    active_qubits: list[QubitSpan],
    factories: list[Factory],
    running_cells: list[tuple[int, int]],
    topology: dict[str, Any],
) -> tuple[int, int, int, int]:
    xs: list[int] = []
    ys: list[int] = []

    for q in active_qubits:
        xs.extend([q.x])
        ys.extend([q.y])
    for f in factories:
        xs.append(f.x)
        ys.append(f.y)
    for x, y in running_cells:
        xs.append(x)
        ys.append(y)

    topology_x, topology_y = 10, 10
    target = topology.get("target", {})
    if isinstance(target, dict):
        topo = target.get("topology", [])
        if isinstance(topo, list):
            for t in topo:
                coord = parse_coord((t or {}).get("coord", []))
                if coord is not None:
                    topology_x = max(topology_x, coord[0])
                    topology_y = max(topology_y, coord[1])

    xs.extend([0, topology_x])
    ys.extend([0, topology_y])

    min_x = min(xs) if xs else 0
    max_x = max(xs) if xs else topology_x
    min_y = min(ys) if ys else 0
    max_y = max(ys) if ys else topology_y

    if min_x == max_x:
        min_x -= 1
        max_x += 1
    if min_y == max_y:
        min_y -= 1
        max_y += 1

    return min_x, max_x, min_y, max_y


def draw_qubit_cell(ax: plt.Axes, q: QubitSpan) -> None:
    x, y = to_canvas(q.x, q.y)
    rect = patches.Rectangle(
        (x - CELL_HALF, y - CELL_HALF),
        CELL_SIZE,
        CELL_SIZE,
        facecolor="#e2e8f0",
        edgecolor="#0f172a",
        alpha=0.9,
        linewidth=1.0,
        zorder=3,
    )
    ax.add_patch(rect)
    ax.text(x, y + 0.03, f"q{q.qid}", ha="center", va="center", color="#111827", fontsize=9, zorder=5)

    if q.dir is None:
        return

    z_color = "#ef4444"
    x_color = "#0284c7"
    if q.dir == 0:
        # 左右が Z 境界、上下が X 境界
        edge_list = [
            ((x - CELL_HALF, y - CELL_HALF), (x - CELL_HALF, y + CELL_HALF), z_color),
            ((x + CELL_HALF, y - CELL_HALF), (x + CELL_HALF, y + CELL_HALF), z_color),
            ((x - CELL_HALF, y + CELL_HALF), (x + CELL_HALF, y + CELL_HALF), x_color),
            ((x - CELL_HALF, y - CELL_HALF), (x + CELL_HALF, y - CELL_HALF), x_color),
        ]
    else:
        # 上下が Z 境界、左右が X 境界
        edge_list = [
            ((x - CELL_HALF, y - CELL_HALF), (x - CELL_HALF, y + CELL_HALF), x_color),
            ((x + CELL_HALF, y - CELL_HALF), (x + CELL_HALF, y + CELL_HALF), x_color),
            ((x - CELL_HALF, y + CELL_HALF), (x + CELL_HALF, y + CELL_HALF), z_color),
            ((x - CELL_HALF, y - CELL_HALF), (x + CELL_HALF, y - CELL_HALF), z_color),
        ]
    for (x1, y1), (x2, y2), c in edge_list:
        ax.plot([x1, x2], [y1, y2], color=c, linewidth=1.8, zorder=4, solid_capstyle="butt")
    ax.text(
        x,
        y - 0.29,
        format_dir(q.dir),
        color="#0f172a",
        fontsize=6,
        ha="center",
        va="top",
        zorder=5,
        fontfamily="monospace",
    )


def draw_factories(ax: plt.Axes, factories: list[Factory]) -> None:
    used_ent = 0
    for f in sorted(factories, key=lambda item: (item.kind, item.id or 0)):
        x, y = to_canvas(f.x, f.y)
        if f.kind == "magic":
            marker = patches.Rectangle(
                (x - CELL_HALF, y - CELL_HALF),
                CELL_SIZE,
                CELL_SIZE,
                facecolor="#f97316",
                edgecolor="#7c2d12",
                linewidth=1.0,
                zorder=3,
            )
            ax.add_patch(marker)
            label = f"MF{f.id}" if f.id is not None else "MF"
            ax.text(x, y, label, ha="center", va="center", fontsize=8, color="#7c2d12", fontweight="bold")
        else:
            used_ent += 1
            marker = patches.Rectangle(
                (x - CELL_HALF, y - CELL_HALF),
                CELL_SIZE,
                CELL_SIZE,
                facecolor="#8b5cf6",
                edgecolor="#312e81",
                linewidth=1.0,
                zorder=3,
            )
            ax.add_patch(marker)
            label = f"EF{used_ent}"
            ax.text(x, y, label, ha="center", va="center", fontsize=7, color="#312e81", fontweight="bold")


def _ordered_unique(points: list[tuple[int, int]]) -> list[tuple[int, int]]:
    seen = set()
    uniq = []
    for point in points:
        if point in seen:
            continue
        seen.add(point)
        uniq.append(point)
    return uniq


def infer_running_cells(inst: dict[str, Any], active_by_id: dict[int, QubitSpan]) -> list[tuple[int, int]]:
    inst_type = str(inst.get("type", ""))
    dir_value = inst.get("dir")
    qtargets = inst.get("qtarget", [])
    path = inst.get("path", [])
    dest = inst.get("dest")

    def normalize_points(points: Any) -> list[tuple[int, int]]:
        if not isinstance(points, list):
            return []
        out: list[tuple[int, int]] = []
        for p in points:
            parsed = parse_coord(p)
            if parsed is None:
                continue
            out.append(parsed)
        return out

    def one_q() -> QubitSpan | None:
        if not qtargets:
            return None
        q0 = parse_int(qtargets[0])
        if q0 is None:
            return None
        return active_by_id.get(q0)

    path_points = normalize_points(path)
    if path_points:
        return _ordered_unique(path_points)

    if inst_type in {"MOVE", "MOVE_TRANS", "MOVE_MAGIC", "MOVE_ENTANGLEMENT"}:
        qspan = one_q()
        if qspan is not None:
            p0 = (qspan.x, qspan.y)
            parsed_dest = parse_coord(dest)
            if parsed_dest is not None:
                return _ordered_unique([p0, parsed_dest])
            return [p0]
        return []

    if inst_type in {"LATTICE_SURGERY", "LATTICE_SURGERY_MAGIC", "LATTICE_SURGERY_MULTINODE"}:
        cells: list[tuple[int, int]] = []
        for qid in qtargets:
            qspan = active_by_id.get(parse_int(qid))
            if qspan is not None:
                cells.append((qspan.x, qspan.y))
        return _ordered_unique(cells)

    if inst_type == "CNOT":
        q1 = parse_int(qtargets[0]) if qtargets else None
        q2 = parse_int(qtargets[1]) if len(qtargets) > 1 else None
        if q1 is None or q2 is None:
            return []
        p1 = active_by_id.get(q1)
        p2 = active_by_id.get(q2)
        if p1 is None or p2 is None:
            return []
        x0, y0 = p1.x, p1.y
        x1, y1 = p2.x, p2.y
        if x0 == x1 or y0 == y1:
            return _ordered_unique([(x0, y0), (x1, y1)])
        return _ordered_unique([(x0, y0), (x0, y1), (x1, y1)])

    if inst_type in {"MEAS_Y", "ROTATE", "TWIST"}:
        qspan = one_q()
        if qspan is None:
            return []
        x, y = qspan.x, qspan.y
        d = dir_value if isinstance(dir_value, int) else None
        if inst_type == "MEAS_Y":
            if d == 0:
                return _ordered_unique([(x, y), (x + 1, y), (x + 1, y + 1), (x, y + 1)])
            if d == 1:
                return _ordered_unique([(x, y), (x, y + 1), (x - 1, y + 1), (x - 1, y)])
            if d == 2:
                return _ordered_unique([(x, y), (x + 1, y), (x + 1, y - 1), (x, y - 1)])
            if d == 3:
                return _ordered_unique([(x, y), (x, y - 1), (x - 1, y - 1), (x - 1, y)])
            return [(x, y), (x + 1, y), (x + 1, y + 1), (x, y + 1)]
        if d == 0:
            return _ordered_unique([(x, y), (x + 1, y)])
        if d == 1:
            return _ordered_unique([(x, y), (x, y + 1)])
        if d == 2:
            return _ordered_unique([(x, y), (x - 1, y)])
        if d == 3:
            return _ordered_unique([(x, y), (x, y - 1)])
        return [(x, y), (x + 1, y)]

    return []


def collect_running_cells(running: list[dict[str, Any]], active_by_id: dict[int, QubitSpan]) -> list[tuple[int, int]]:
    cells: list[tuple[int, int]] = []
    for inst in running:
        inst_cells = list(inst["ancilla"])
        if not inst_cells:
            inst_cells = infer_running_cells(inst, active_by_id)
        for qid in inst.get("qtarget", []):
            qspan = active_by_id.get(parse_int(qid))
            if qspan is not None:
                inst_cells.append((qspan.x, qspan.y))
        cells.extend(inst_cells)
    if not cells:
        return []
    if cells:
        return _ordered_unique(cells)
    return cells


def draw_running_ancilla(
    ax: plt.Axes,
    running: list[dict[str, Any]],
    active_by_id: dict[int, QubitSpan],
) -> list[tuple[str, str]]:
    palette = plt.get_cmap("tab20").colors
    labels: list[tuple[str, str]] = []
    if not running:
        return labels
    for i, inst in enumerate(running):
        color = palette[int(inst["index"]) % len(palette)]
        inst_type = str(inst["type"])
        displayed_cells = list(inst["ancilla"])
        inferred = False
        if not displayed_cells:
            displayed_cells = infer_running_cells(inst, active_by_id)
            inferred = True

        target_cells: set[tuple[int, int]] = set()
        for qid in inst["qtarget"]:
            span = active_by_id.get(parse_int(qid))
            if span is not None:
                target_cells.add((span.x, span.y))

        text_anchor: tuple[float, float] | None = None
        if displayed_cells:
            pts: list[tuple[float, float]] = []
            for x, y in displayed_cells:
                cx, cy = to_canvas(x, y)
                pts.append((cx, cy))
                marker_style = "s"
                face = color
                edge = "#111827"
                linewidth = 0.8
                fill = True
                if inferred and inst["type"] in {"TWIST", "ROTATE"} and (x, y) not in target_cells:
                    fill = False
                    face = "none"
                    edge = "#be185d"
                    linewidth = 1.6
                if inst_type == "MEAS_Y" and (x, y) in target_cells:
                    edge = "#0f172a"
                ax.scatter(
                    cx,
                    cy,
                    marker=marker_style,
                    s=180,
                    c=[face] if fill else "none",
                    edgecolor=edge,
                    linewidth=linewidth,
                    zorder=2,
                )
            if len(pts) >= 2 and inst_type == "CNOT":
                xs, ys = zip(*pts)
                ax.plot(xs, ys, color=color, ls=":", lw=1.2, zorder=1, alpha=0.7)
            if text_anchor is None:
                text_anchor = pts[0]

        for x, y in target_cells:
            cx, cy = to_canvas(x, y)
            ax.scatter(
                cx,
                cy,
                marker="o",
                s=220,
                facecolor="none",
                edgecolor=color,
                linewidth=1.9,
                zorder=6,
            )
            if text_anchor is None:
                text_anchor = (cx + 0.02, cy + 0.24)

        label = f"{inst['index']}:{inst['type']}"
        if text_anchor is not None:
            ax.text(
                text_anchor[0] + 0.15,
                text_anchor[1] + 0.18,
                label,
                color="#111827",
                fontsize=7,
                zorder=4,
            )

        if len(inst["qtarget"]) > 0:
            target = ",".join(str(qid) for qid in inst["qtarget"])
            suffix = f" q={target}"
        else:
            suffix = ""
        conds = inst.get("condition", [])
        if len(conds) > 0:
            cond_text = ",".join(f"c{x}" for x in conds)
            suffix += f" cond=[{cond_text}]"
        labels.append((label + suffix, f"#{i + 1}"))
    return labels


def validate_topology(target: dict[str, Any], *, allow_non_dim2: bool) -> None:
    machine_option = target.get("machine_option", {})
    machine_type = str(machine_option.get("type", "")).lower()
    if machine_type not in {"", "dim2"} and not allow_non_dim2:
        msg = (
            f"machine_option.type is '{machine_type.upper()}' (not Dim2). "
            "This viewer is optimized for Dim2 and may show unexpected layout."
        )
        print(f"[WARN] {msg}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Visualize one-beat state of SC_LS_FIXED_V0 pipeline state with matplotlib."
    )
    parser.add_argument("pipeline_json", type=Path, help="SC_LS_FIXED_V0 pipeline state JSON.")
    parser.add_argument(
        "--beat",
        "-b",
        type=int,
        required=True,
        help="Beat B to visualize.",
    )
    parser.add_argument(
        "--mapping-only",
        action="store_true",
        help="Enable if pipeline was mapped but not routed (Routing fields may be missing).",
    )
    parser.add_argument(
        "--allow-non-dim2",
        action="store_true",
        help="Allow running on non-Dim2 machine type.",
    )
    parser.add_argument("--output", type=Path, help="If set, save image to this path instead of showing.")
    return parser.parse_args()


def make_figure(
    *,
    beat: int,
    parameter_target: dict[str, Any],
    spans: list[QubitSpan],
    program: list[dict[str, Any]],
    factories: list[Factory],
    running: list[dict[str, Any]],
    title_suffix: str = "",
) -> tuple[plt.Figure, plt.Axes]:
    # Determine default end beat for open lifetimes.
    if spans:
        default_end = max(span.end or 0 for span in spans) + 1
        all_beats_max = max(span.begin for span in spans) + 1
    else:
        default_end = beat + 1
        all_beats_max = beat + 1

    active_qubits = collect_active_qubits(spans, beat, max(default_end, all_beats_max))
    active_qubits = apply_dynamic_dirs(active_qubits, program, beat)
    active_by_id = {q.qid: q for q in active_qubits}
    running_cells = collect_running_cells(running, active_by_id)
    bounds = collect_bounds(active_qubits, factories, running_cells, {"target": parameter_target})

    min_x, max_x, min_y, max_y = bounds

    fig, ax = plt.subplots(figsize=(12, 10))
    ax.set_aspect("equal", adjustable="box")
    ax.set_xlim(min_x - GRID_MARGIN, max_x + 1 + GRID_MARGIN)
    ax.set_ylim(min_y - GRID_MARGIN, max_y + 1 + GRID_MARGIN)
    ax.set_xticks(range(min_x, max_x + 2))
    ax.set_yticks(range(min_y, max_y + 2))
    ax.tick_params(labelsize=8)
    ax.grid(True, linestyle="--", linewidth=0.6, color="#94a3b8", alpha=0.5, zorder=0)
    ax.set_axisbelow(True)

    for q in active_qubits:
        draw_qubit_cell(ax, q)

    draw_factories(ax, factories)
    inst_labels = draw_running_ancilla(ax, running, active_by_id)

    header = [
        f"SC_LS_FIXED_V0 Pipeline State @ Beat {beat}{title_suffix}",
        f"Active logical qubits: {len(active_qubits)}",
        f"Running instructions: {len(running)}",
    ]
    fig.suptitle(" / ".join(header), fontsize=14, y=0.96)

    if inst_labels:
        MAX_LEGEND_ITEMS = 20
        label_lines = ["Running instructions:"]
        for text in inst_labels[:MAX_LEGEND_ITEMS]:
            label_lines.append(f"  - {text[0]}")
        if len(inst_labels) > MAX_LEGEND_ITEMS:
            label_lines.append(f"  - ... +{len(inst_labels) - MAX_LEGEND_ITEMS} more")
        fig.text(0.02, 0.91, "\n".join(label_lines), fontsize=8, va="top", family="monospace")

        fig.text(
            0.02,
            0.02,
            "Legend: Blue square=Logical qubit, Orange square=Magic factory, Purple square=Entanglement factory, "
            "Colored square=Ancilla path, Circle=Instruction target(qubit), Pink square=TWIST expanded auxiliary cell",
            fontsize=8,
        )

    # Highlight top-left and top-right for easier board readability.
    ax.plot([min_x, max_x], [min_y, min_y], color="none")

    ax.set_xlabel("x")
    ax.set_ylabel("y")
    return fig, ax


def main() -> None:
    args = parse_args()

    state = json.loads(args.pipeline_json.read_text(encoding="utf-8"))
    if not isinstance(state, dict):
        raise RuntimeError("Invalid pipeline state JSON")
    if "program" not in state or not isinstance(state["program"], list):
        raise RuntimeError("pipeline_json must contain a program list")

    parameter = state.get("parameter", {})
    target = parameter.get("target", {})
    if not isinstance(target, dict):
        raise RuntimeError("parameter.target must be a dict")

    validate_topology(target, allow_non_dim2=args.allow_non_dim2)

    program = state["program"]
    spans, min_beat, max_beat = update_qubit_lifetimes(program, mapping_only=args.mapping_only)
    factories = collect_factories(target, program)

    if args.mapping_only:
        if args.beat != 0:
            print(f"[INFO] mapping-only mode: requested beat {args.beat} is ignored. Using beat 0.")
        beat = 0
        running = []
    else:
        running = collect_running_instructions(program, args.beat)

        if args.beat < min_beat:
            print(
                f"[WARN] requested beat {args.beat} is before first known event ({min_beat}). Clamped to {min_beat}."
            )
            beat = min_beat
            running = collect_running_instructions(program, beat)
        elif args.beat > max_beat + 1:
            print(
                f"[WARN] requested beat {args.beat} is beyond known range ({max_beat}). No active instructions expected."
            )
            beat = args.beat
        else:
            beat = args.beat

    if args.mapping_only and not running:
        print(
            "[INFO] mapping-only mode: no running instructions detected at this beat (likely due no timing/ancilla after mapping only)."
        )

    title_suffix = " [mapping-only mode]" if args.mapping_only else ""
    fig, _ = make_figure(
        beat=beat,
        parameter_target=target,
        spans=spans,
        program=program,
        factories=factories,
        running=running,
        title_suffix=title_suffix,
    )

    if args.output is not None:
        fig.savefig(args.output, dpi=200, bbox_inches="tight")
        print(f"saved: {args.output}")
        return

    plt.show()


if __name__ == "__main__":
    main()
