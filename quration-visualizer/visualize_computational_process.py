"""Modern visualizer for SC_LS_FIXED_V0 computational process."""

from __future__ import annotations

import hashlib
import json
from collections import defaultdict
from dataclasses import dataclass
from typing import TYPE_CHECKING

import numpy as np
import pandas as pd
import plotly.graph_objects as go
import streamlit as st
import streamlit.components.v1 as components

from scripts.circuit import Circuit, FTQCConfig, construct_graph

if TYPE_CHECKING:
    from scripts.circuit import Instruction


TYPE_COLORS = [
    "#1d4ed8",
    "#0369a1",
    "#0f766e",
    "#166534",
    "#a16207",
    "#b45309",
    "#be123c",
    "#7f1d1d",
    "#7c3aed",
    "#4c1d95",
]
RUNNING_HIGHLIGHT_COLOR = "#f43f5e"


@dataclass(frozen=True)
class ViewConfig:
    begin_beat: int
    end_beat: int
    current_beat: int
    selected_types: set[str]
    search_text: str


def inject_modern_style() -> None:
    st.markdown(
        """
<style>
.stApp {
  background:
    radial-gradient(1200px 600px at 0% -10%, #cffafe 0%, transparent 60%),
    radial-gradient(1200px 600px at 100% -10%, #fef3c7 0%, transparent 60%),
    linear-gradient(180deg, #f8fafc 0%, #ecfeff 100%);
}
.main-title {
  font-size: 2.05rem;
  font-weight: 800;
  letter-spacing: 0.01em;
  margin-bottom: 0.25rem;
}
.main-subtitle {
  color: #334155;
  margin-bottom: 1rem;
}
[data-testid="stMetric"] {
  background: #ffffffcc;
  border: 1px solid #bae6fd;
  border-radius: 14px;
  padding: 0.45rem 0.7rem;
  box-shadow: 0 10px 24px rgba(15, 23, 42, 0.07);
}
</style>
        """,
        unsafe_allow_html=True,
    )


def inject_playback_style() -> None:
    st.markdown(
        """
<style>
[data-testid="stAppViewContainer"] > .main .block-container {
  max-width: 100%;
  padding-top: 0.25rem;
  padding-bottom: 0.25rem;
  padding-left: 0.45rem;
  padding-right: 0.45rem;
}
</style>
        """,
        unsafe_allow_html=True,
    )


def clamp(value: int, low: int, high: int) -> int:
    return max(min(value, high), low)


def state_of_inst(inst: Instruction, current_beat: int) -> str:
    start = inst.beat
    end = inst.beat + max(inst.latency, 1)
    if current_beat < start:
        return "upcoming"
    if start <= current_beat < end:
        return "running"
    return "done"


@st.cache_resource(show_spinner=False)
def load_pipeline(raw_text: str) -> tuple[FTQCConfig, Circuit]:
    raw = json.loads(raw_text)
    return FTQCConfig(raw["parameter"]["target"]), Circuit(raw["program"])


def build_type_colors(types: list[str]) -> dict[str, str]:
    return {t: TYPE_COLORS[i % len(TYPE_COLORS)] for i, t in enumerate(types)}


def build_base_dataframe(circuit: Circuit) -> pd.DataFrame:
    rows = []
    for inst in circuit:
        rows.append(
            {
                "index": inst.index,
                "beat": inst.beat,
                "latency": inst.latency,
                "end_beat": inst.beat + max(inst.latency, 1),
                "type": str(inst.type),
                "qtarget": list(inst.qtarget),
                "ccreate": list(inst.ccreate),
                "cdepend": list(inst.cdepend),
                "condition": list(inst.condition),
                "raw": str(inst),
                "search_blob": f"{inst.type} {inst}".lower(),
            }
        )
    return pd.DataFrame(rows)


def get_base_dataframe(circuit: Circuit, cache_key: str) -> pd.DataFrame:
    key = f"base_df:{cache_key}"
    if key not in st.session_state:
        st.session_state[key] = build_base_dataframe(circuit)
    return st.session_state[key]


def filter_dataframe(base_df: pd.DataFrame, config: ViewConfig) -> pd.DataFrame:
    if len(config.selected_types) == 0 or len(base_df) == 0:
        return base_df.iloc[0:0].copy()

    mask = (
        (base_df["beat"] >= config.begin_beat)
        & (base_df["beat"] < config.end_beat)
        & (base_df["type"].isin(config.selected_types))
    )

    query = config.search_text.lower().strip()
    if query:
        mask &= base_df["search_blob"].str.contains(query, regex=False)

    return base_df.loc[mask].copy()


def get_filtered_view(
    base_df: pd.DataFrame,
    cache_key: str,
    config: ViewConfig,
) -> tuple[pd.DataFrame, set[int], int, tuple]:
    query_key = (
        cache_key,
        config.begin_beat,
        config.end_beat,
        tuple(sorted(config.selected_types)),
        config.search_text.lower().strip(),
    )
    cache = st.session_state.get("filtered_view_cache")
    if cache is not None and cache.get("query_key") == query_key:
        return (
            cache["filtered_df"],
            cache["visible_indices"],
            cache["type_count"],
            query_key,
        )

    filtered_df = filter_dataframe(base_df, config)
    visible_indices = set(filtered_df["index"].tolist())
    type_count = int(filtered_df["type"].nunique()) if len(filtered_df) != 0 else 0

    st.session_state.filtered_view_cache = {
        "query_key": query_key,
        "filtered_df": filtered_df,
        "visible_indices": visible_indices,
        "type_count": type_count,
    }
    return filtered_df, visible_indices, type_count, query_key


def get_stateful_view(
    filtered_df: pd.DataFrame,
    query_key: tuple,
    current_beat: int,
) -> pd.DataFrame:
    cache = st.session_state.get("stateful_view_cache")
    if cache is not None and cache.get("query_key") == query_key and cache.get("beat") == current_beat:
        return cache["df_with_state"]

    df_with_state = add_state_column(filtered_df, current_beat)
    st.session_state.stateful_view_cache = {
        "query_key": query_key,
        "beat": current_beat,
        "df_with_state": df_with_state,
    }
    return df_with_state


def add_state_column(df: pd.DataFrame, current_beat: int) -> pd.DataFrame:
    if len(df) == 0:
        out = df.copy()
        out["state"] = pd.Series(dtype="string")
        return out

    starts = df["beat"].to_numpy()
    ends = df["end_beat"].to_numpy()

    states = np.full(len(df), "done", dtype=object)
    states[current_beat < starts] = "upcoming"
    states[(starts <= current_beat) & (current_beat < ends)] = "running"

    out = df.copy()
    out["state"] = states
    return out


def render_header(
    circuit: Circuit,
    config: ViewConfig,
    *,
    num_filtered: int,
    active_now: int,
    types_now: int,
) -> None:
    st.markdown('<div class="main-title">Computational Process Visualizer</div>', unsafe_allow_html=True)
    st.markdown(
        '<div class="main-subtitle">命令実行の時系列・依存関係・空間占有を同時に追跡します。</div>',
        unsafe_allow_html=True,
    )

    cols = st.columns(5)
    cols[0].metric("Instructions (filtered)", f"{num_filtered:,}")
    cols[1].metric("Active @ current beat", f"{active_now:,}")
    cols[2].metric("Type count", f"{types_now:,}")
    cols[3].metric("Qubits", f"{circuit.num_qubits():,}")
    cols[4].metric("Beat window", f"{config.begin_beat}..{config.end_beat - 1}")


def render_timeline_panel(df: pd.DataFrame, current_beat: int, *, compact: bool = False) -> None:
    if len(df) == 0:
        st.info("条件に一致する命令がありません。")
        return

    beat_counts = (
        df.groupby(["beat", "state"], dropna=False)
        .size()
        .reset_index(name="count")
        .sort_values(["beat", "state"])
    )

    fig = go.Figure()
    state_colors = {"done": "#0ea5e9", "running": "#f97316", "upcoming": "#64748b"}
    for state in ["done", "running", "upcoming"]:
        sub = beat_counts[beat_counts["state"] == state]
        if len(sub) == 0:
            continue
        fig.add_bar(
            x=sub["beat"],
            y=sub["count"],
            name=state,
            marker_color=state_colors[state],
        )
    fig.add_vline(x=current_beat, line_width=2, line_dash="dash", line_color="#ef4444")
    fig.update_layout(
        barmode="stack",
        xaxis_title="beat",
        yaxis_title="count",
        margin=dict(l=20, r=20, t=20, b=20),
        height=320 if compact else 420,
    )
    st.plotly_chart(fig, width="stretch")

    if compact:
        return

    type_counts = (
        df.groupby("type", dropna=False)
        .size()
        .reset_index(name="count")
        .sort_values("count", ascending=False)
    )
    fig2 = go.Figure(
        data=[
            go.Bar(
                x=type_counts["type"],
                y=type_counts["count"],
                marker_color="#0891b2",
            )
        ]
    )
    fig2.update_layout(
        xaxis_title="instruction type",
        yaxis_title="count",
        margin=dict(l=20, r=20, t=20, b=20),
        height=360,
    )
    st.plotly_chart(fig2, width="stretch")


def render_instructions_panel(
    df: pd.DataFrame,
    current_beat: int,
    show_raw_json: bool,
    *,
    compact: bool = False,
) -> None:
    if len(df) == 0:
        st.info("表示対象がありません。")
        return

    cols = ["index", "beat", "state", "type", "latency", "qtarget", "ccreate", "cdepend", "condition", "raw"]

    if compact:
        compact_df = df.copy()
        compact_df["distance"] = (compact_df["beat"] - current_beat).abs()
        compact_df = compact_df.sort_values(["distance", "beat", "index"]).head(20)
        st.dataframe(compact_df[cols], width="stretch", hide_index=True)
        return

    st.dataframe(df[cols], width="stretch", hide_index=True)

    selected_idx = st.selectbox("Inspect instruction index", options=df["index"].tolist(), index=0)
    selected = df[df["index"] == selected_idx].iloc[0].to_dict()
    st.json(selected)

    if show_raw_json:
        st.caption("Raw row payload")
        st.code(json.dumps(selected, ensure_ascii=False, indent=2), language="json")


def render_dependency_panel(
    circuit: Circuit,
    base_df: pd.DataFrame,
    config: ViewConfig,
    *,
    half_window: int,
    max_nodes: int,
) -> None:
    dep_begin = max(config.begin_beat, config.current_beat - half_window)
    dep_end = min(config.end_beat, config.current_beat + half_window + 1)
    node_count = int(((base_df["beat"] >= dep_begin) & (base_df["beat"] < dep_end)).sum())

    st.caption(f"Dependency window: {dep_begin}..{dep_end - 1} (nodes={node_count:,})")
    if node_count > max_nodes:
        st.warning(
            f"Dependency graph is too large ({node_count:,} nodes). "
            "Narrow the beat range or decrease dependency window."
        )
        return

    graph = construct_graph(circuit, dep_begin, dep_end, config.current_beat)
    st.graphviz_chart(graph)
    st.download_button(
        "Download DOT",
        data=str(graph),
        file_name=f"dependency_{dep_begin}_{dep_end}_{config.current_beat}.dot",
        mime="text/plain",
    )


def collect_active_instruction_ids(
    circuit: Circuit,
    current_beat: int,
    visible_indices: set[int],
    *,
    lookback: int = 4,
) -> set[int]:
    active_ids: set[int] = set()
    begin = max(circuit.begin_beat, current_beat - lookback)
    for beat in range(begin, current_beat + 1):
        for inst in circuit.insts_of_beat(beat):
            if inst.index not in visible_indices:
                continue
            if current_beat < inst.beat + max(inst.latency, 1) or current_beat == inst.beat:
                active_ids.add(inst.index)
    return active_ids


def build_spatial_2d_figure(
    ftqc: FTQCConfig,
    circuit: Circuit,
    current_beat: int,
    inst_ids: set[int],
    type_colors: dict[str, str],
    *,
    max_segments: int,
) -> go.Figure:
    fig = go.Figure()

    shapes = []
    for x in range(ftqc.max_x + 1):
        shapes.append(
            dict(
                type="line",
                x0=x,
                x1=x,
                y0=0,
                y1=ftqc.max_y,
                line=dict(color="#e2e8f0", width=1),
                layer="below",
            )
        )
    for y in range(ftqc.max_y + 1):
        shapes.append(
            dict(
                type="line",
                x0=0,
                x1=ftqc.max_x,
                y0=y,
                y1=y,
                line=dict(color="#e2e8f0", width=1),
                layer="below",
            )
        )

    qubits = circuit.qubits_of_beat(current_beat)
    if len(qubits) != 0:
        fig.add_trace(
            go.Scattergl(
                x=[q.x + 0.5 for q in qubits],
                y=[q.y + 0.5 for q in qubits],
                mode="markers+text",
                marker=dict(size=10, color="#2563eb"),
                text=[f"q{q.id}" for q in qubits],
                textposition="top center",
                name="Qubit",
                hovertemplate="%{text}<extra></extra>",
            )
        )

    factories = circuit.factories_of_beat(current_beat)
    if len(factories) != 0:
        fig.add_trace(
            go.Scattergl(
                x=[f.x + 0.5 for f in factories],
                y=[f.y + 0.5 for f in factories],
                mode="markers+text",
                marker=dict(size=12, symbol="diamond", color="#db2777"),
                text=[f"m{f.id}" for f in factories],
                textposition="bottom center",
                name="MagicFactory",
                hovertemplate="%{text}<extra></extra>",
            )
        )

    grouped: dict[str, dict[str, list[float | str | None]]] = defaultdict(
        lambda: {"x": [], "y": [], "text": []}
    )
    grouped_running: dict[str, dict[str, list[float | str | None]]] = defaultdict(
        lambda: {"x": [], "y": [], "text": []}
    )

    num_segments = 0
    for inst_id in sorted(inst_ids):
        if num_segments >= max_segments:
            break
        inst = circuit.get_inst(inst_id)
        target = grouped_running if state_of_inst(inst, current_beat) == "running" else grouped
        inst_type = str(inst.type)
        hover_text = f"[{inst.index}] {inst.type}"
        for path in inst.get_paths():
            if len(path) == 0:
                continue
            if num_segments >= max_segments:
                break
            for px, py in path:
                target[inst_type]["x"].append(px + 0.5)
                target[inst_type]["y"].append(py + 0.5)
                target[inst_type]["text"].append(hover_text)
            target[inst_type]["x"].append(None)
            target[inst_type]["y"].append(None)
            target[inst_type]["text"].append(None)
            num_segments += 1

    for inst_type, data in grouped.items():
        if len(data["x"]) == 0:
            continue
        fig.add_trace(
            go.Scattergl(
                x=data["x"],
                y=data["y"],
                mode="lines+markers",
                line=dict(width=3, color=type_colors.get(inst_type, "#1d4ed8")),
                marker=dict(size=5, color=type_colors.get(inst_type, "#1d4ed8")),
                text=data["text"],
                name=inst_type,
                hovertemplate="%{text}<extra></extra>",
            )
        )

    for inst_type, data in grouped_running.items():
        if len(data["x"]) == 0:
            continue
        fig.add_trace(
            go.Scattergl(
                x=data["x"],
                y=data["y"],
                mode="lines+markers",
                line=dict(width=6, color=RUNNING_HIGHLIGHT_COLOR),
                marker=dict(size=7, color=RUNNING_HIGHLIGHT_COLOR),
                text=data["text"],
                name=f"{inst_type} (running)",
                showlegend=False,
                hovertemplate="%{text}<extra></extra>",
            )
        )

    fig.update_layout(
        xaxis=dict(range=[0, ftqc.max_x], title="x", constrain="domain"),
        yaxis=dict(range=[0, ftqc.max_y], title="y", scaleanchor="x", scaleratio=1),
        margin=dict(l=20, r=20, t=20, b=20),
        legend=dict(orientation="h", yanchor="bottom", y=1.01, xanchor="left", x=0),
        shapes=shapes,
        uirevision="spatial2d",
        height=520,
    )
    return fig


def build_spatial_3d_figure(
    ftqc: FTQCConfig,
    circuit: Circuit,
    config: ViewConfig,
    visible_indices: set[int],
    highlight_indices: set[int],
    *,
    beat_window: int,
    max_segments: int,
) -> go.Figure:
    begin = max(config.begin_beat, config.current_beat - beat_window)
    end = min(config.end_beat, config.current_beat + beat_window + 1)

    fig = go.Figure()

    qubit_x: list[float | None] = []
    qubit_y: list[float | None] = []
    qubit_z: list[float | None] = []
    qubit_text: list[str | None] = []
    for q in circuit.get_qubits().values():
        qb = max(q.begin, begin)
        qe = min(q.end, end)
        if qb >= qe:
            continue
        qubit_x.extend([q.x, q.x, None])
        qubit_y.extend([q.y, q.y, None])
        qubit_z.extend([qb, qe, None])
        qubit_text.extend([f"q{q.id}", f"q{q.id}", None])

    if len(qubit_x) != 0:
        fig.add_trace(
            go.Scatter3d(
                x=qubit_x,
                y=qubit_y,
                z=qubit_z,
                mode="lines",
                line=dict(color="#1d4ed8", width=3),
                text=qubit_text,
                name="qubit world-line",
                hovertemplate="%{text}<extra></extra>",
            )
        )

    normal_x: list[float | None] = []
    normal_y: list[float | None] = []
    normal_z: list[float | None] = []
    normal_text: list[str | None] = []

    highlight_x: list[float | None] = []
    highlight_y: list[float | None] = []
    highlight_z: list[float | None] = []
    highlight_text: list[str | None] = []

    normal_segments = 0
    highlight_segments = 0
    for beat in range(begin, end):
        for inst in circuit.insts_of_beat(beat):
            if inst.index not in visible_indices:
                continue
            use_highlight = inst.index in highlight_indices
            hover_text = f"[{inst.index}] {inst.type} @ beat {beat}"
            for path in inst.get_paths():
                if len(path) < 2:
                    continue
                if use_highlight:
                    if highlight_segments >= max_segments:
                        continue
                    highlight_segments += 1
                else:
                    if normal_segments >= max_segments:
                        continue
                    normal_segments += 1
                for px, py in path:
                    if use_highlight:
                        highlight_x.append(px)
                        highlight_y.append(py)
                        highlight_z.append(beat)
                        highlight_text.append(hover_text)
                    else:
                        normal_x.append(px)
                        normal_y.append(py)
                        normal_z.append(beat)
                        normal_text.append(hover_text)
                if use_highlight:
                    highlight_x.append(None)
                    highlight_y.append(None)
                    highlight_z.append(None)
                    highlight_text.append(None)
                else:
                    normal_x.append(None)
                    normal_y.append(None)
                    normal_z.append(None)
                    normal_text.append(None)

    if len(normal_x) != 0:
        fig.add_trace(
            go.Scatter3d(
                x=normal_x,
                y=normal_y,
                z=normal_z,
                mode="lines",
                line=dict(color="rgba(30, 41, 59, 0.35)", width=4),
                text=normal_text,
                name="paths",
                hovertemplate="%{text}<extra></extra>",
            )
        )

    if len(highlight_x) != 0:
        fig.add_trace(
            go.Scatter3d(
                x=highlight_x,
                y=highlight_y,
                z=highlight_z,
                mode="lines",
                line=dict(color=RUNNING_HIGHLIGHT_COLOR, width=9),
                text=highlight_text,
                name="2D-linked highlight",
                hovertemplate="%{text}<extra></extra>",
            )
        )

    beat_frame_x = [0, ftqc.max_x, ftqc.max_x, 0, 0]
    beat_frame_y = [0, 0, ftqc.max_y, ftqc.max_y, 0]
    beat_frame_z = [config.current_beat] * 5
    fig.add_trace(
        go.Scatter3d(
            x=beat_frame_x,
            y=beat_frame_y,
            z=beat_frame_z,
            mode="lines",
            line=dict(color="#06b6d4", width=5),
            name="current beat",
            hoverinfo="skip",
        )
    )

    fig.update_layout(
        scene=dict(
            xaxis=dict(title="x", range=[0, ftqc.max_x]),
            yaxis=dict(title="y", range=[0, ftqc.max_y]),
            zaxis=dict(title="beat", range=[begin, end - 1]),
        ),
        margin=dict(l=10, r=10, t=10, b=10),
        height=620,
        uirevision="spatial3d",
    )
    return fig


def render_unified_view(
    ftqc: FTQCConfig,
    circuit: Circuit,
    base_df: pd.DataFrame,
    df_with_state: pd.DataFrame,
    config: ViewConfig,
    type_colors: dict[str, str],
    active_ids: set[int],
    *,
    dependency_half_window: int,
    dependency_max_nodes: int,
    spatial_max_segments: int,
) -> None:
    top_left, top_right = st.columns([1.35, 1])
    with top_left:
        st.markdown("### Timeline")
        render_timeline_panel(df_with_state, config.current_beat, compact=True)
    with top_right:
        st.markdown("### Spatial 2D")
        st.plotly_chart(
            build_spatial_2d_figure(
                ftqc,
                circuit,
                config.current_beat,
                active_ids,
                type_colors,
                max_segments=spatial_max_segments,
            ),
            width="stretch",
        )

    bottom_left, bottom_right = st.columns([1.35, 1])
    with bottom_left:
        st.markdown("### Instructions")
        render_instructions_panel(df_with_state, config.current_beat, show_raw_json=False, compact=True)
    with bottom_right:
        st.markdown("### Dependency")
        render_dependency_panel(
            circuit,
            base_df,
            config,
            half_window=dependency_half_window,
            max_nodes=dependency_max_nodes,
        )


def render_playback_view(
    ftqc: FTQCConfig,
    circuit: Circuit,
    config: ViewConfig,
    visible_indices: set[int],
    type_colors: dict[str, str],
    *,
    spatial_3d_window: int,
    spatial_max_segments: int,
    component_height: int,
) -> None:
    payload = build_playback_payload(
        ftqc,
        circuit,
        config,
        visible_indices,
        type_colors,
        max_paths_per_beat=spatial_max_segments,
        history_window=spatial_3d_window,
    )
    components.html(build_playback_html(payload), width=None, height=component_height, scrolling=False)


def build_playback_payload(
    ftqc: FTQCConfig,
    circuit: Circuit,
    config: ViewConfig,
    visible_indices: set[int],
    type_colors: dict[str, str],
    *,
    max_paths_per_beat: int,
    history_window: int,
) -> dict:
    instructions = []
    active_by_beat: defaultdict[int, list[int]] = defaultdict(list)

    for inst in circuit:
        if inst.index not in visible_indices:
            continue
        paths = [path for path in inst.get_paths() if len(path) >= 2]
        if len(paths) == 0:
            continue

        begin = max(config.begin_beat, inst.beat)
        end = min(config.end_beat, inst.beat + max(inst.latency, 1))
        if begin >= end:
            continue

        included = False
        for beat in range(begin, end):
            if len(active_by_beat[beat]) >= max_paths_per_beat:
                continue
            active_by_beat[beat].append(inst.index)
            included = True

        if included:
            instructions.append(
                {
                    "id": inst.index,
                    "type": str(inst.type),
                    "color": type_colors.get(str(inst.type), "#1d4ed8"),
                    "paths": paths,
                }
            )

    qubits = [
        {
            "id": q.id,
            "x": q.x,
            "y": q.y,
            "begin": max(q.begin, config.begin_beat),
            "end": min(q.end, config.end_beat),
        }
        for q in circuit.get_qubits().values()
        if max(q.begin, config.begin_beat) < min(q.end, config.end_beat)
    ]

    factories = [
        {
            "id": f.id,
            "x": f.x,
            "y": f.y,
            "begin": max(f.begin, config.begin_beat),
            "end": config.end_beat,
        }
        for f in circuit.get_factories().values()
        if f.begin < config.end_beat
    ]

    return {
        "grid": {"maxX": ftqc.max_x, "maxY": ftqc.max_y},
        "beginBeat": config.begin_beat,
        "endBeat": config.end_beat - 1,
        "historyWindow": max(0, int(history_window)),
        "instructions": instructions,
        "activeByBeat": {str(k): v for k, v in active_by_beat.items()},
        "qubits": qubits,
        "factories": factories,
        "runningColor": RUNNING_HIGHLIGHT_COLOR,
    }


def build_playback_html(payload: dict) -> str:
    payload_json = json.dumps(payload, separators=(",", ":"))
    return f"""
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Playback</title>
  <style>
    :root {{
      color-scheme: light;
      --bg: #f8fafc;
      --card: #ffffff;
      --line: #cbd5e1;
      --text: #0f172a;
      --muted: #475569;
      --accent: #f43f5e;
    }}
    html, body {{
      margin: 0;
      padding: 0;
      width: 100%;
      height: 100%;
      overflow: hidden;
      font-family: "Segoe UI", "Noto Sans JP", sans-serif;
      background: linear-gradient(180deg, #f8fafc, #ecfeff);
      color: var(--text);
    }}
    .root {{
      box-sizing: border-box;
      width: 100%;
      height: 100%;
      padding: 4px;
      display: grid;
      grid-template-rows: auto 1fr;
      gap: 6px;
    }}
    .toolbar {{
      background: var(--card);
      border: 1px solid var(--line);
      border-radius: 10px;
      padding: 8px;
      display: grid;
      grid-template-columns: auto auto auto auto auto 1fr auto;
      gap: 6px;
      align-items: center;
    }}
    .toolbar button {{
      border: 1px solid #94a3b8;
      border-radius: 8px;
      background: #f8fafc;
      color: var(--text);
      padding: 8px 12px;
      cursor: pointer;
      font-weight: 600;
    }}
    .toolbar button:hover {{
      background: #e2e8f0;
    }}
    .toolbar label {{
      font-size: 12px;
      color: var(--muted);
    }}
    .toolbar input[type="range"] {{
      width: 100%;
    }}
    .panels {{
      min-height: 0;
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 6px;
    }}
    .panel {{
      min-width: 0;
      min-height: 0;
      background: var(--card);
      border: 1px solid var(--line);
      border-radius: 12px;
      overflow: hidden;
      display: grid;
      grid-template-rows: auto 1fr;
    }}
    .panel h3 {{
      margin: 0;
      padding: 8px 10px;
      border-bottom: 1px solid var(--line);
      font-size: 14px;
      font-weight: 700;
      color: #1e293b;
    }}
    #canvas2d {{
      width: 100%;
      height: 100%;
      display: block;
      background: #ffffff;
    }}
    #webgl-wrap {{
      width: 100%;
      height: 100%;
      position: relative;
      background: #f8fafc;
    }}
    #canvas3d {{
      width: 100%;
      height: 100%;
      display: block;
    }}
  </style>
</head>
<body>
  <div class="root">
    <div class="toolbar">
      <button id="startBtn">Start</button>
      <button id="stopBtn">Stop</button>
      <button id="resetBtn">Reset</button>
      <label>Beat <span id="beatValue"></span></label>
      <input id="beatSlider" type="range">
      <label>FPS <input id="fpsSlider" type="range" min="1" max="30" value="8"></label>
      <label><input id="loopChk" type="checkbox" checked> Loop</label>
    </div>
    <div class="panels">
      <div class="panel">
        <h3>Spatial 2D</h3>
        <canvas id="canvas2d"></canvas>
      </div>
      <div class="panel">
        <h3>Spatial 3D (2D-linked highlight)</h3>
        <div id="webgl-wrap"><canvas id="canvas3d"></canvas></div>
      </div>
    </div>
  </div>

  <script src="https://cdn.jsdelivr.net/npm/three@0.132.2/build/three.min.js"></script>
  <script src="https://cdn.jsdelivr.net/npm/three@0.132.2/examples/js/controls/OrbitControls.js"></script>
  <script>
    const payload = {payload_json};
    const instMap = new Map(payload.instructions.map(inst => [inst.id, inst]));
    const activeByBeat = payload.activeByBeat;

    const state = {{
      beat: payload.beginBeat,
      running: false,
      fps: 8,
      step: 1,
      lastAdvance: 0,
    }};

    const beatSlider = document.getElementById("beatSlider");
    const beatValue = document.getElementById("beatValue");
    const fpsSlider = document.getElementById("fpsSlider");
    const loopChk = document.getElementById("loopChk");
    beatSlider.min = String(payload.beginBeat);
    beatSlider.max = String(payload.endBeat);
    beatSlider.value = String(payload.beginBeat);
    beatValue.textContent = String(payload.beginBeat);

    document.getElementById("startBtn").addEventListener("click", () => {{
      state.running = true;
    }});
    document.getElementById("stopBtn").addEventListener("click", () => {{
      state.running = false;
    }});
    document.getElementById("resetBtn").addEventListener("click", () => {{
      state.running = false;
      setBeat(payload.beginBeat);
    }});
    beatSlider.addEventListener("input", () => {{
      setBeat(Number(beatSlider.value));
    }});
    fpsSlider.addEventListener("input", () => {{
      state.fps = Number(fpsSlider.value);
    }});

    function setBeat(beat) {{
      const clamped = Math.max(payload.beginBeat, Math.min(payload.endBeat, beat));
      state.beat = clamped;
      beatSlider.value = String(clamped);
      beatValue.textContent = String(clamped);
      draw2D();
      redraw3D();
    }}

    function activeIdsAt(beat) {{
      return activeByBeat[String(beat)] || [];
    }}

    // 2D
    const c2 = document.getElementById("canvas2d");
    const ctx = c2.getContext("2d");

    function resize2D() {{
      const rect = c2.getBoundingClientRect();
      const dpr = window.devicePixelRatio || 1;
      c2.width = Math.max(1, Math.floor(rect.width * dpr));
      c2.height = Math.max(1, Math.floor(rect.height * dpr));
      ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
      draw2D();
    }}

    function draw2D() {{
      const w = c2.clientWidth;
      const h = c2.clientHeight;
      if (w <= 0 || h <= 0) return;

      ctx.clearRect(0, 0, w, h);
      const cols = Math.max(1, payload.grid.maxX);
      const rows = Math.max(1, payload.grid.maxY);
      const cell = Math.min(w / cols, h / rows);
      const ox = (w - cols * cell) * 0.5;
      const oy = (h - rows * cell) * 0.5;

      ctx.strokeStyle = "#e2e8f0";
      ctx.lineWidth = 1;
      for (let x = 0; x <= cols; x++) {{
        const px = ox + x * cell;
        ctx.beginPath();
        ctx.moveTo(px, oy);
        ctx.lineTo(px, oy + rows * cell);
        ctx.stroke();
      }}
      for (let y = 0; y <= rows; y++) {{
        const py = oy + y * cell;
        ctx.beginPath();
        ctx.moveTo(ox, py);
        ctx.lineTo(ox + cols * cell, py);
        ctx.stroke();
      }}

      function center(p) {{
        return [ox + (p[0] + 0.5) * cell, oy + (p[1] + 0.5) * cell];
      }}

      const ids = activeIdsAt(state.beat);
      ctx.lineCap = "round";
      ctx.lineJoin = "round";
      for (const id of ids) {{
        const inst = instMap.get(id);
        if (!inst) continue;
        ctx.strokeStyle = inst.color || "#1d4ed8";
        ctx.lineWidth = Math.max(4.2, cell * 0.16);
        ctx.globalAlpha = 0.95;
        for (const path of inst.paths) {{
          if (!path || path.length < 2) continue;
          ctx.beginPath();
          const first = center(path[0]);
          ctx.moveTo(first[0], first[1]);
          for (let i = 1; i < path.length; i++) {{
            const p = center(path[i]);
            ctx.lineTo(p[0], p[1]);
          }}
          ctx.stroke();
        }}
      }}

      for (const q of payload.qubits) {{
        if (!(q.begin <= state.beat && state.beat < q.end)) continue;
        const [x, y] = center([q.x, q.y]);
        ctx.beginPath();
        ctx.fillStyle = "#1d4ed8";
        ctx.arc(x, y, Math.max(3, cell * 0.26), 0, Math.PI * 2);
        ctx.fill();
      }}
      for (const f of payload.factories) {{
        if (!(f.begin <= state.beat && state.beat < f.end)) continue;
        const [x, y] = center([f.x, f.y]);
        ctx.beginPath();
        ctx.fillStyle = "#be185d";
        ctx.arc(x, y, Math.max(3, cell * 0.24), 0, Math.PI * 2);
        ctx.fill();
      }}

      ctx.globalAlpha = 1;
      ctx.fillStyle = "#0f172a";
      ctx.font = "12px Segoe UI, sans-serif";
      ctx.fillText(`active paths: ${{ids.length}}`, 10, 18);
    }}

    // 3D
    const wrap3d = document.getElementById("webgl-wrap");
    const c3 = document.getElementById("canvas3d");
    const scene = new THREE.Scene();
    scene.background = new THREE.Color(0xf8fafc);

    const camera = new THREE.PerspectiveCamera(55, 1, 0.1, 3000);
    const renderer = new THREE.WebGLRenderer({{ canvas: c3, antialias: true }});
    renderer.setPixelRatio(window.devicePixelRatio || 1);

    const controls = new THREE.OrbitControls(camera, c3);
    const gx = payload.grid.maxX;
    const gy = payload.grid.maxY;
    const centerX = (gx - 1) * 0.5;
    const centerY = (gy - 1) * 0.5;

    camera.position.set(centerX + 8, centerY + 8, Math.max(18, (payload.endBeat - payload.beginBeat) * 0.15));
    controls.target.set(centerX, centerY, (payload.beginBeat + payload.endBeat) * 0.18);

    scene.add(new THREE.AmbientLight(0xffffff, 0.7));
    const dl = new THREE.DirectionalLight(0xffffff, 0.7);
    dl.position.set(8, 10, 14);
    scene.add(dl);

    const gridHelper = new THREE.GridHelper(Math.max(gx, gy) + 2, Math.max(gx, gy) + 2, 0xcbd5e1, 0xe2e8f0);
    gridHelper.position.set(centerX, centerY, payload.beginBeat * 0.36);
    scene.add(gridHelper);

    const qubitMat = new THREE.MeshPhongMaterial({{
      color: 0x1d4ed8,
      emissive: 0x0f172a,
      transparent: true,
      opacity: 0.18
    }});
    for (const q of payload.qubits) {{
      const z0 = q.begin * 0.36;
      const z1 = Math.max(z0 + 0.2, q.end * 0.36);
      const geom = new THREE.BoxGeometry(0.33, 0.33, z1 - z0);
      const mesh = new THREE.Mesh(geom, qubitMat);
      mesh.position.set(q.x, q.y, (z0 + z1) * 0.5);
      scene.add(mesh);
    }}

    const factoryMat = new THREE.MeshPhongMaterial({{
      color: 0xdb2777,
      emissive: 0x4a044e,
      transparent: true,
      opacity: 0.55
    }});
    for (const f of payload.factories) {{
      const z0 = f.begin * 0.36;
      const z1 = Math.max(z0 + 0.3, f.end * 0.36);
      const geom = new THREE.CylinderGeometry(0.23, 0.23, z1 - z0, 18);
      const mesh = new THREE.Mesh(geom, factoryMat);
      // CylinderGeometry is Y-axis oriented by default; rotate to align with beat(Z) axis.
      mesh.rotation.x = Math.PI * 0.5;
      mesh.position.set(f.x, f.y, (z0 + z1) * 0.5);
      scene.add(mesh);
    }}

    const pathGroup = new THREE.Group();
    scene.add(pathGroup);

    function clearPathGroup() {{
      while (pathGroup.children.length > 0) {{
        const child = pathGroup.children.pop();
        if (child.geometry) child.geometry.dispose();
        if (child.material) child.material.dispose();
      }}
    }}

    function addPathsForBeat(beat, colorHex, alpha, isCurrent) {{
      const ids = activeIdsAt(beat);
      const radius = isCurrent ? 0.13 : 0.085;
      for (const id of ids) {{
        const inst = instMap.get(id);
        if (!inst) continue;
        for (const path of inst.paths) {{
          if (!path || path.length < 2) continue;
          const pts = path.map(p => new THREE.Vector3(p[0], p[1], beat * 0.36));
          const curve = new THREE.CatmullRomCurve3(pts);
          const geom = new THREE.TubeGeometry(curve, Math.max(6, pts.length * 3), radius, 8, false);
          const mat = new THREE.MeshPhongMaterial({{
            color: colorHex,
            emissive: isCurrent ? colorHex : 0x0f172a,
            transparent: true,
            opacity: alpha,
          }});
          pathGroup.add(new THREE.Mesh(geom, mat));
        }}
      }}
    }}

    function redraw3D() {{
      clearPathGroup();
      const start = Math.max(payload.beginBeat, state.beat - payload.historyWindow);
      for (let b = start; b < state.beat; b++) {{
        addPathsForBeat(b, 0x64748b, 0.28, false);
      }}
      addPathsForBeat(state.beat, parseInt(payload.runningColor.replace("#", "0x")), 1.0, true);
    }}

    function resize3D() {{
      const rect = wrap3d.getBoundingClientRect();
      const w = Math.max(1, rect.width);
      const h = Math.max(1, rect.height);
      renderer.setSize(w, h, false);
      camera.aspect = w / h;
      camera.updateProjectionMatrix();
    }}

    function stepBeat() {{
      let next = state.beat + state.step;
      if (next > payload.endBeat) {{
        if (loopChk.checked) {{
          next = payload.beginBeat;
        }} else {{
          next = payload.endBeat;
          state.running = false;
        }}
      }}
      setBeat(next);
    }}

    function animate(ts) {{
      requestAnimationFrame(animate);
      if (state.running) {{
        const interval = 1000 / Math.max(1, state.fps);
        if (ts - state.lastAdvance >= interval) {{
          state.lastAdvance = ts;
          stepBeat();
        }}
      }} else {{
        state.lastAdvance = ts;
      }}
      controls.update();
      renderer.render(scene, camera);
    }}

    window.addEventListener("resize", () => {{
      resize2D();
      resize3D();
    }});

    resize2D();
    resize3D();
    setBeat(payload.beginBeat);
    animate(0);
  </script>
</body>
</html>
    """


def render_empty_state() -> None:
    st.markdown('<div class="main-title">Computational Process Visualizer</div>', unsafe_allow_html=True)
    st.markdown(
        '<div class="main-subtitle">右サイドバーから pipeline state JSON をアップロードしてください。</div>',
        unsafe_allow_html=True,
    )
    st.info("SC_LS_FIXED_V0 の `compile` 出力 JSON を入力に使います。")


def main() -> None:
    st.set_page_config(
        page_title="Visualize computational process",
        layout="wide",
        initial_sidebar_state="expanded",
    )
    inject_modern_style()

    st.sidebar.header("Input")
    upload_file = st.sidebar.file_uploader("Upload pipeline state JSON", type=["json"])

    if upload_file is None:
        render_empty_state()
        return

    raw_bytes = upload_file.getvalue()
    raw_text = raw_bytes.decode("utf-8")
    cache_key = hashlib.sha1(raw_bytes).hexdigest()

    try:
        ftqc, circuit = load_pipeline(raw_text)
    except Exception as ex:  # noqa: BLE001
        st.error(f"Failed to parse json: {ex}")
        return

    if ftqc.machine_type.lower() != "dim2" or ftqc.topology_count > 1:
        st.error(
            "visualize_computational_process.py currently supports only Dim2 single-chip JSON."
        )
        st.caption(
            f"Detected machine_type={ftqc.machine_type}, topology_count={ftqc.topology_count}. "
            "Dim3 / DistributedDim2 / DistributedDim3 are intentionally unsupported."
        )
        return

    if st.session_state.get("loaded_pipeline_key") != cache_key:
        st.session_state.loaded_pipeline_key = cache_key
        st.session_state.current_beat = circuit.begin_beat
        st.session_state.current_beat_widget = circuit.begin_beat

    base_df = get_base_dataframe(circuit, cache_key)
    all_types = sorted(base_df["type"].unique().tolist())
    type_colors = build_type_colors(all_types)

    st.sidebar.header("View")
    mode = st.sidebar.radio(
        "Mode",
        options=["Single", "Unified", "Playback"],
        help="Single: 1画面だけ描画して高速化 / Unified: 4画面同時 / Playback: 2D+3D連動アニメ",
    )

    begin_beat, end_inclusive = st.sidebar.slider(
        "Beat range",
        min_value=circuit.begin_beat,
        max_value=circuit.end_beat - 1,
        value=(circuit.begin_beat, circuit.end_beat - 1),
    )
    end_beat = end_inclusive + 1

    current_beat = begin_beat
    if mode != "Playback":
        if "current_beat" not in st.session_state:
            st.session_state.current_beat = begin_beat
        if "current_beat_widget" not in st.session_state:
            st.session_state.current_beat_widget = st.session_state.current_beat

        st.session_state.current_beat = clamp(st.session_state.current_beat, begin_beat, end_inclusive)
        st.session_state.current_beat_widget = clamp(
            int(st.session_state.current_beat_widget),
            begin_beat,
            end_inclusive,
        )
        st.sidebar.number_input(
            "Current beat",
            min_value=begin_beat,
            max_value=end_inclusive,
            step=1,
            key="current_beat_widget",
        )
        st.session_state.current_beat = int(st.session_state.current_beat_widget)
        current_beat = st.session_state.current_beat

    selected_types = set(
        st.sidebar.multiselect(
            "Instruction types",
            options=all_types,
            default=all_types,
        )
    )
    search_text = st.sidebar.text_input("Search (type/raw)", value="")

    spatial_3d_window = st.sidebar.slider(
        "Spatial 3D beat window",
        min_value=10,
        max_value=400,
        value=70,
        step=10,
    )
    spatial_max_segments = st.sidebar.slider(
        "Spatial max path segments",
        min_value=200,
        max_value=30000,
        value=6000,
        step=200,
    )
    dependency_half_window = 40
    dependency_max_nodes = 800
    show_raw_json = False
    playback_height = 1240
    if mode == "Playback":
        playback_height = st.sidebar.slider(
            "Playback height",
            min_value=760,
            max_value=1800,
            value=1240,
            step=20,
        )
    else:
        dependency_half_window = st.sidebar.slider(
            "Dependency half-window",
            min_value=10,
            max_value=300,
            value=40,
            step=5,
        )
        dependency_max_nodes = st.sidebar.slider(
            "Dependency max nodes",
            min_value=100,
            max_value=5000,
            value=800,
            step=100,
        )
        show_raw_json = st.sidebar.toggle("Show raw detail payload", value=False)

    config = ViewConfig(
        begin_beat=begin_beat,
        end_beat=end_beat,
        current_beat=int(current_beat),
        selected_types=selected_types,
        search_text=search_text,
    )

    filtered_df, visible_indices, filtered_type_count, query_key = get_filtered_view(
        base_df,
        cache_key,
        config,
    )
    active_ids: set[int] = set()
    if mode != "Playback":
        active_ids = collect_active_instruction_ids(circuit, config.current_beat, visible_indices)
        render_header(
            circuit,
            config,
            num_filtered=len(filtered_df),
            active_now=len(active_ids),
            types_now=filtered_type_count,
        )
    else:
        inject_playback_style()
        st.markdown(
            f"### Playback | beat `{config.begin_beat}..{config.end_beat - 1}` | instructions `{len(filtered_df):,}`"
        )

    if mode == "Single":
        panel = st.radio(
            "Panel",
            options=["Timeline", "Instructions", "Dependency", "Spatial 2D", "Spatial 3D"],
            horizontal=True,
        )

        if panel == "Timeline":
            df_with_state = get_stateful_view(filtered_df, query_key, config.current_beat)
            render_timeline_panel(df_with_state, config.current_beat)
        elif panel == "Instructions":
            df_with_state = get_stateful_view(filtered_df, query_key, config.current_beat)
            render_instructions_panel(df_with_state, config.current_beat, show_raw_json=show_raw_json)
        elif panel == "Dependency":
            render_dependency_panel(
                circuit,
                base_df,
                config,
                half_window=dependency_half_window,
                max_nodes=dependency_max_nodes,
            )
        elif panel == "Spatial 2D":
            st.plotly_chart(
                build_spatial_2d_figure(
                    ftqc,
                    circuit,
                    config.current_beat,
                    active_ids,
                    type_colors,
                    max_segments=spatial_max_segments,
                ),
                width="stretch",
            )
        elif panel == "Spatial 3D":
            st.plotly_chart(
                build_spatial_3d_figure(
                    ftqc,
                    circuit,
                    config,
                    visible_indices,
                    active_ids,
                    beat_window=spatial_3d_window,
                    max_segments=spatial_max_segments,
                ),
                width="stretch",
            )

    elif mode == "Unified":
        df_with_state = get_stateful_view(filtered_df, query_key, config.current_beat)
        render_unified_view(
            ftqc,
            circuit,
            base_df,
            df_with_state,
            config,
            type_colors,
            active_ids,
            dependency_half_window=dependency_half_window,
            dependency_max_nodes=dependency_max_nodes,
            spatial_max_segments=spatial_max_segments,
        )

    else:
        render_playback_view(
            ftqc,
            circuit,
            config,
            visible_indices,
            type_colors,
            spatial_3d_window=spatial_3d_window,
            spatial_max_segments=spatial_max_segments,
            component_height=playback_height,
        )


if __name__ == "__main__":
    main()
