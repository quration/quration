"""Visualize compile information using streamlit."""

from __future__ import annotations

import json
from collections import OrderedDict
from dataclasses import dataclass
from typing import TYPE_CHECKING

import numpy as np
import pandas as pd
import plotly.graph_objects as go
import streamlit as st

if TYPE_CHECKING:
    from streamlit.delta_generator import DeltaGenerator


@dataclass(frozen=True)
class Coord:
    x: int
    y: int
    z: int


class Grid:
    @staticmethod
    def load_json(j: dict) -> Grid:
        if "type" not in j:
            msg = "Invalid JSON: missing grid 'type' key"
            raise ValueError(msg)

        ban = []
        qubit = {}
        magic_factory = {}
        entanglement_factory = {}

        if j["type"] == "plane":
            max_x = j["coord"][0]
            max_y = j["coord"][1]
            z = j["coord"][2]

            if "ban" in j:
                ban.extend(Coord(coord[0], coord[1], z) for coord in j["ban"])
            if "qubit" in j:
                for tmp in j["qubit"]:
                    symbol = tmp["symbol"]
                    coord = tmp["coord"]
                    qubit[symbol] = Coord(coord[0], coord[1], z)
            if "magic_factory" in j:
                for tmp in j["magic_factory"]:
                    symbol = tmp["symbol"]
                    coord = tmp["coord"]
                    magic_factory[symbol] = Coord(coord[0], coord[1], z)
            if "entanglement_factory" in j:
                for tmp in j["entanglement_factory"]:
                    symbol = tmp["symbol"]
                    pair = tmp["pair"]
                    coord = tmp["coord"]
                    entanglement_factory[symbol] = (Coord(coord[0], coord[1], z), pair)

            return Grid(max_x, max_y, z, z + 1, ban, qubit, magic_factory, entanglement_factory)

        assert j["type"] == "grid"
        max_x = j["coord"][0]
        max_y = j["coord"][1]
        min_z = j["coord"][2]
        max_z = j["coord"][3]
        if "ban" in j:
            ban.extend(Coord(coord[0], coord[1], coord[2]) for coord in j["ban"])
        if "qubit" in j:
            for tmp in j["qubit"]:
                symbol = tmp["symbol"]
                coord = tmp["coord"]
                qubit[symbol] = Coord(coord[0], coord[1], coord[2])
        if "magic_factory" in j:
            for tmp in j["magic_factory"]:
                symbol = tmp["symbol"]
                coord = tmp["coord"]
                magic_factory[symbol] = Coord(coord[0], coord[1], coord[2])
        if "entanglement_factory" in j:
            for tmp in j["entanglement_factory"]:
                symbol = tmp["symbol"]
                pair = tmp["pair"]
                coord = tmp["coord"]
                entanglement_factory[symbol] = (Coord(coord[0], coord[1], coord[2]), pair)
        return Grid(max_x, max_y, min_z, max_z, ban, qubit, magic_factory, entanglement_factory)

    def __init__(
        self,
        max_x: int,
        max_y: int,
        min_z: int,
        max_z: int,
        ban: list[Coord],
        qubit: dict[Coord, int],
        magic_factory: dict[Coord, int],
        entanglement_factory: dict[Coord, int],
    ):
        self.max_x = max_x
        self.max_y = max_y
        self.min_z = min_z
        self.max_z = max_z
        self.ban = ban
        self.qubit = qubit
        self.magic_factory = magic_factory
        self.entanglement_factory = entanglement_factory

    def volume(self) -> int:
        return self.max_x * self.max_y * (self.max_z - self.min_z)


class Topology:
    @staticmethod
    def load_json(j: dict) -> Topology:
        return Topology([Grid.load_json(tmp) for tmp in j])

    def __init__(self, grids: list[Grid]):
        self.grids = grids

    def num_grids(self) -> int:
        return len(self.grids)


def get_color_set() -> list[str]:
    """Get color set.

    Returns:
        list[str]: color set
    """
    # Plotly default colors: https://community.plotly.com/t/plotly-colours-list/11730/2
    return [
        "#1f77b4",  # muted blue
        "#ff7f0e",  # safety orange
        "#2ca02c",  # cooked asparagus green
        "#d62728",  # brick red
        "#9467bd",  # muted purple
        "#8c564b",  # chestnut brown
        "#e377c2",  # raspberry yogurt pink
        "#7f7f7f",  # middle gray
        "#bcbd22",  # curry yellow-green
        "#17becf",  # blue-teal
    ]


def get_summary(jsons: OrderedDict[str, dict]) -> str:
    """Get summary of compile information.

    Args:
        jsons (OrderedDict[str, dict]): compile information files in json format

    Returns:
        str: summary
    """
    ret = ""
    for name in jsons:
        if len(ret) == 0:
            ret += "`" + name + "`"
        else:
            ret += " and " + "`" + name + "`"
    return "Visualizing " + ret


def stateful_text_input(key: str, value: str) -> str | None:
    """Create a stateful text input."""
    if key not in st.session_state:
        st.session_state[key] = value
    new_value = st.text_input(label=key, value=st.session_state[key])
    st.session_state[key] = new_value
    return new_value  # noqa:DOC201


class CreateTable:
    """Create tables of compile information."""

    @staticmethod
    def _dict_or_empty(value: object) -> dict:
        if isinstance(value, dict):
            return value
        return {}

    @staticmethod
    def _time_series(j: dict, key: str, keys: list[str], values: list[float]) -> None:
        if key not in j:
            return

        time_series = j[key]
        if not isinstance(time_series, list) or len(time_series) == 0:
            return

        try:
            keys.extend(["[ave] " + key, "[peak] " + key])
            values.extend([sum(time_series) / len(time_series), max(time_series)])
        except TypeError:
            return

    @staticmethod
    def constant(jsons: OrderedDict[str, dict]) -> pd.DataFrame:
        """Create table of constant information."""
        df = pd.DataFrame()

        keys = [
            "use_magic_state_cultivation",
            "magic_factory_seed_offset",
            "magic_generation_period",
            "prob_magic_state_creation",
            "maximum_magic_state_stock",
            "reaction_time",
            # TODO: remove chip_x and chip_y
            "chip_x",
            "chip_y",
        ]
        for name, j in jsons.items():
            df[name] = [j.get(key, None) for key in keys]
            df[name] = df[name].astype(str)

        df.index = keys
        return df  # noqa:DOC201

    @staticmethod
    def topology(ts: OrderedDict[str, Topology]) -> pd.DataFrame:
        """Create table of topology information."""
        df = pd.DataFrame()

        keys = [
            "max_x",
            "max_y",
            "min_z",
            "max_z",
            "volume",
            "ban",
            "qubit",
            "magic_factory",
            "entanglement_factory",
        ]
        max_num_grids = max(t.num_grids() for t in ts.values())
        for name, t in ts.items():
            values = []
            for i in range(max_num_grids):
                if i < t.num_grids():
                    grid = t.grids[i]
                    values.extend([
                        grid.max_x,
                        grid.max_y,
                        grid.min_z,
                        grid.max_z,
                        grid.volume(),
                        str(grid.ban) if len(grid.ban) != 0 else None,
                        str(grid.qubit) if len(grid.qubit) != 0 else None,
                        str(grid.magic_factory) if len(grid.magic_factory) != 0 else None,
                        str(grid.entanglement_factory) if len(grid.entanglement_factory) != 0 else None,
                    ])
                else:
                    values.extend(None for _ in range(len(keys)))
            df[name] = values
            df[name] = df[name].astype(str)

        keys_fin = []
        for i in range(max_num_grids):
            keys_fin.extend(f"[grid {i}] {key}" for key in keys)
        df.index = keys_fin
        return df  # noqa:DOC201

    @staticmethod
    def runtime(jsons: OrderedDict[str, dict]) -> pd.DataFrame:
        """Create table of runtime information."""
        df = pd.DataFrame()

        keys = ["runtime", "runtime_without_topology"]
        for name, j in jsons.items():
            df[name] = [j.get(key, None) for key in keys]

        df.index = keys
        return df  # noqa:DOC201

    @staticmethod
    def gate(jsons: OrderedDict[str, dict]) -> pd.DataFrame:
        """Create table of gate information."""
        df = pd.DataFrame()

        keys = None
        for name, j in jsons.items():
            keys = ["gate_count", "gate_depth"]
            values = [j.get(key, None) for key in keys]
            CreateTable._time_series(j, "gate_throughput", keys, values)
            df[name] = values

        if keys is not None:
            df.index = keys
        return df  # noqa:DOC201

    @staticmethod
    def gate_detail(jsons: OrderedDict[str, dict]) -> pd.DataFrame:
        """Create table of detailed gate information."""
        kind_set: set[str] = set()
        for j in jsons.values():
            kind_set.update(CreateTable._dict_or_empty(j.get("gate_count_detail")).keys())
        kind: list[str] = sorted(kind_set)

        df = pd.DataFrame()

        keys = None
        for name, j in jsons.items():
            details = CreateTable._dict_or_empty(j.get("gate_count_detail"))
            keys = []
            values = []
            for key in kind:
                keys.append(key)
                if key in details:
                    values.append(details[key])
                else:
                    values.append(0)
            df[name] = values

        if keys is not None:
            df.index = keys
        return df  # noqa:DOC201

    @staticmethod
    def measurement_depth(jsons: OrderedDict[str, dict]) -> pd.DataFrame:
        """Create table of measurement depth information."""
        df = pd.DataFrame()

        keys = None
        for name, j in jsons.items():
            keys = [
                "measurement_feedback_count",
                "measurement_feedback_depth",
            ]
            values = [j.get(key, None) for key in keys]
            CreateTable._time_series(j, "measurement_feedback_rate", keys, values)
            new_keys = ["gate_depth"]
            keys.extend(new_keys)
            values.extend([j.get(key, None) for key in new_keys])
            CreateTable._time_series(j, "gate_throughput", keys, values)
            df[name] = values

        if keys is not None:
            df.index = keys
        return df  # noqa:DOC201

    @staticmethod
    def magic_state_consumption(jsons: OrderedDict[str, dict]) -> pd.DataFrame:
        """Create table of magic state consumption information."""
        df = pd.DataFrame()

        keys = None
        for name, j in jsons.items():
            keys = [
                "magic_state_consumption_count",
                "magic_state_consumption_depth",
            ]
            values = [j.get(key, None) for key in keys]
            CreateTable._time_series(j, "magic_state_consumption_rate", keys, values)
            new_keys = [
                "runtime_estimation_magic_state_consumption_count",
                "runtime_estimation_magic_state_consumption_depth",
                "magic_factory_count",
            ]
            keys.extend(new_keys)
            values.extend([j.get(key, None) for key in new_keys])
            df[name] = values

        if keys is not None:
            df.index = keys
        return df  # noqa:DOC201

    @staticmethod
    def entanglement_consumption(jsons: OrderedDict[str, dict]) -> pd.DataFrame:
        """Create table of entanglement consumption information."""
        df = pd.DataFrame()

        keys = None
        for name, j in jsons.items():
            keys = [
                "entanglement_consumption_count",
                "entanglement_consumption_depth",
            ]
            values = [j.get(key, None) for key in keys]
            CreateTable._time_series(j, "entanglement_consumption_rate", keys, values)
            new_keys = [
                "runtime_estimation_entanglement_consumption_count",
                "runtime_estimation_entanglement_consumption_depth",
                "entanglement_factory_count",
            ]
            keys.extend(new_keys)
            values.extend([j.get(key, None) for key in new_keys])
            df[name] = values

        if keys is not None:
            df.index = keys
        return df  # noqa:DOC201

    @staticmethod
    def cell_consumption(jsons: OrderedDict[str, dict]) -> pd.DataFrame:
        """Create table of cell consumption information."""
        df = pd.DataFrame()

        keys = None
        for name, j in jsons.items():
            keys = ["chip_cell_count"]
            values = [j.get(key, None) for key in keys]
            CreateTable._time_series(j, "chip_cell_algorithmic_qubit", keys, values)
            CreateTable._time_series(j, "chip_cell_algorithmic_qubit_ratio", keys, values)
            CreateTable._time_series(j, "chip_cell_active_qubit_area", keys, values)
            CreateTable._time_series(j, "chip_cell_active_qubit_area_ratio", keys, values)
            new_keys = ["qubit_volume"]
            keys.extend(new_keys)
            values.extend([j.get(key, None) for key in new_keys])
            df[name] = values

        if keys is not None:
            df.index = keys
        return df  # noqa:DOC201


def visualize_table(col: DeltaGenerator, jsons: OrderedDict[str, dict]) -> None:
    """Visualize table."""
    col.markdown("## Runtime")
    col.dataframe(CreateTable.runtime(jsons), width="stretch")
    col.markdown("## Gate")
    col.dataframe(CreateTable.gate(jsons), width="stretch")
    col.markdown("### Details")
    col.dataframe(CreateTable.gate_detail(jsons), width="stretch")
    col.markdown("## Measurement depth")
    col.dataframe(CreateTable.measurement_depth(jsons), width="stretch")
    col.markdown("## Magic state consumption")
    col.dataframe(CreateTable.magic_state_consumption(jsons), width="stretch")
    col.markdown("## Entanglement consumption")
    col.dataframe(CreateTable.entanglement_consumption(jsons), width="stretch")
    col.markdown("## Cell consumption")
    col.dataframe(CreateTable.cell_consumption(jsons), width="stretch")


def calc_maximum_of_time_series(jsons: OrderedDict[str, dict], key: str = "gate_throughput") -> int:
    """Calculate maximum value of time series."""
    lengths = []
    for j in jsons.values():
        value = j.get(key, [])
        if isinstance(value, list):
            lengths.append(len(value))
    return max(lengths, default=0)  # noqa:DOC201


def process(
    time_series: list[int | float], tmin: int, tmax: int, bin_size: int
) -> tuple[list[int], list[int | float]]:
    """Process time series.

    Args:
        time_series (list[int  |  float]): time series
        tmin (int): minimum value of time series
        tmax (int): minimum value of time series
        bin_size (int): bin size

    Returns:
        tuple[list[int], list[int | float]]: tuple of thw following two values
            1. [tmin, tmin+bin_size, tmin+2*bin_size, ..., tmax], [mean of time_series[tmin:tmin+bin_size]
            2. mean of time_series[tmin+bin_size:tmin+2*bin_size], ...]
    """
    if len(time_series) == 0:
        beats = np.arange(tmin, tmax + bin_size, bin_size)
        return (list(beats), [0.0] * max(0, len(beats) - 1))

    beats = np.arange(tmin, tmax + bin_size, bin_size)
    if len(time_series) < beats[-1]:
        time_series += [0] * (beats[-1] - len(time_series))
    np_time_series = np.array(time_series[tmin : beats[-1]])
    averages = np.mean(np_time_series.reshape(-1, bin_size), axis=1)
    return (list(beats), list(averages))


def create_time_series_fig(  # noqa:PLR0913,PLR0917
    jsons: OrderedDict[str, dict],
    colors: OrderedDict[str, str],
    key: str,
    tmin: int,
    tmax: int,
    bin_size: int,
) -> go.Figure:
    """Create a figure of time series."""
    fig = go.Figure()

    for name, j in jsons.items():
        time_series = j.get(key, [])
        if not isinstance(time_series, list):
            continue
        x, y = process(time_series, tmin, tmax, bin_size)
        fig.add_trace(go.Scatter(x=x, y=y, name=name, marker_color=colors[name]))

    fig.update_layout(xaxis_title="beat")
    return fig  # noqa:DOC201


def visualize_time_series(col: DeltaGenerator, jsons: OrderedDict[str, dict], colors: OrderedDict[str, str]) -> None:
    """Visualize time series."""
    assert (len(jsons)) != 0
    maximum_of_time_series = calc_maximum_of_time_series(jsons)

    # Settings
    col.markdown("## Settings of visualizing time series")
    col1, col2 = col.columns(2)
    tmin, tmax = col1.slider(
        "min/max of time series",
        min_value=0,
        max_value=maximum_of_time_series,
        value=(0, maximum_of_time_series),
    )
    if tmin == tmax:
        col1.write("max must not be equal to min")
        return
    bin_size = col2.slider(
        "bin size",
        min_value=1,
        max_value=max(10, int((tmax - tmin) / 50)),
        value=max(1, int((tmax - tmin) / 500)),
    )

    # Visualize
    def visualize(key: str, *, expanded: bool = False) -> None:
        if not all(key in j for j in jsons.values()):
            return

        title = key.replace("_", " ").title()
        with col.expander(title, expanded=expanded):
            st.plotly_chart(
                create_time_series_fig(jsons, colors, key, tmin, tmax, bin_size),
                width="stretch",
            )

    col.markdown("## Time series")
    col.markdown(f"Visualize time series from {tmin} to {tmax} with bin size {bin_size}")
    visualize("gate_throughput", expanded=True)
    visualize("measurement_feedback_rate")
    visualize("magic_state_consumption_rate")
    visualize("entanglement_consumption_rate")
    visualize("chip_cell_algorithmic_qubit")
    visualize("chip_cell_algorithmic_qubit_ratio")
    visualize("chip_cell_active_qubit_area")
    visualize("chip_cell_active_qubit_area_ratio")


CORE_METRICS = [
    "runtime",
    "gate_count",
    "gate_depth",
    "measurement_feedback_count",
    "magic_state_consumption_count",
    "entanglement_consumption_count",
    "code_distance",
    "num_physical_qubits",
    "execution_time_sec",
]

TIME_SERIES_KEYS = [
    "gate_throughput",
    "measurement_feedback_rate",
    "magic_state_consumption_rate",
    "entanglement_consumption_rate",
    "chip_cell_algorithmic_qubit",
    "chip_cell_algorithmic_qubit_ratio",
    "chip_cell_active_qubit_area",
    "chip_cell_active_qubit_area_ratio",
]


def inject_modern_style() -> None:
    st.markdown(
        """
<style>
.stApp {
  background:
    radial-gradient(1200px 600px at 0% -10%, #e0f2fe 0%, transparent 60%),
    radial-gradient(1200px 600px at 100% -10%, #fee2e2 0%, transparent 60%),
    linear-gradient(180deg, #f8fafc 0%, #eef2f7 100%);
}
.viz-title {
  font-size: 2.0rem;
  font-weight: 800;
  letter-spacing: 0.01em;
  margin-bottom: 0.25rem;
}
.viz-subtitle {
  color: #334155;
  margin-bottom: 1rem;
}
[data-testid="stMetric"] {
  background: #ffffffcc;
  border: 1px solid #dbeafe;
  border-radius: 14px;
  padding: 0.4rem 0.7rem;
  box-shadow: 0 8px 24px rgba(15, 23, 42, 0.06);
}
</style>
        """,
        unsafe_allow_html=True,
    )


def extract_compile_info(raw: dict) -> dict:
    if "opt" in raw and isinstance(raw["opt"], dict) and "compile_info" in raw["opt"]:
        return raw["opt"]["compile_info"]
    return raw


def parse_topology(j: dict) -> Topology:
    if "topology" in j:
        return Topology.load_json(j["topology"])
    chip_x = j.get("chip_x", 0)
    chip_y = j.get("chip_y", 0)
    return Topology([Grid(chip_x, chip_y, 0, 1, [], {}, {}, {})])


def build_overview_df(jsons: OrderedDict[str, dict]) -> pd.DataFrame:
    df = pd.DataFrame(index=CORE_METRICS)
    for name, j in jsons.items():
        df[name] = [j.get(k, None) for k in CORE_METRICS]
    return df


def rename_labels(items: OrderedDict[str, dict]) -> dict[str, str]:
    renamed: dict[str, str] = {}
    with st.expander("Labels", expanded=False):
        st.caption("表示名を変更できます（重複不可）。")
        for name in items:
            renamed[name] = stateful_text_input(key=f"label:{name}", value=name) or name
    return renamed


def apply_rename(items: OrderedDict[str, object], renamed: dict[str, str]) -> OrderedDict[str, object]:
    return OrderedDict((renamed[k], v) for k, v in items.items())


def render_overview_tab(
    jsons: OrderedDict[str, dict],
    colors: OrderedDict[str, str],
    overview_df: pd.DataFrame,
    use_log_scale: bool,
) -> None:
    st.markdown("### Overview")

    metric_cols = st.columns(4)
    summary_metrics = ["runtime", "gate_count", "code_distance", "num_physical_qubits"]
    for i, metric in enumerate(summary_metrics):
        values = overview_df.loc[metric].dropna()
        if len(values) == 0:
            continue
        metric_cols[i].metric(
            label=metric,
            value=f"{values.min():,.4g}",
            delta=f"max {values.max():,.4g}",
        )

    st.dataframe(overview_df, width="stretch")

    col1, col2 = st.columns([2, 1])
    selected_metric = col1.selectbox("Bar chart metric", overview_df.index.tolist(), index=0)
    baseline = col2.selectbox("Baseline", ["None", *overview_df.columns.tolist()], index=0)

    values = overview_df.loc[selected_metric]
    fig = go.Figure()
    for name in overview_df.columns:
        fig.add_bar(
            x=[name],
            y=[values[name]],
            name=name,
            marker_color=colors.get(name, "#0ea5e9"),
            showlegend=False,
        )
    fig.update_layout(
        title=f"{selected_metric} comparison",
        yaxis_type="log" if use_log_scale else "linear",
        xaxis_title="file",
        yaxis_title=selected_metric,
        margin=dict(l=20, r=20, t=45, b=20),
    )
    st.plotly_chart(fig, width="stretch")

    if baseline != "None":
        base = overview_df[baseline]
        delta_df = pd.DataFrame(index=overview_df.index)
        for name in overview_df.columns:
            if name == baseline:
                continue
            delta_df[f"{name} - {baseline}"] = overview_df[name] - base
        st.markdown("#### Delta From Baseline")
        st.dataframe(delta_df, width="stretch")


def render_tables_tab(jsons: OrderedDict[str, dict], topologies: OrderedDict[str, Topology]) -> None:
    st.markdown("### Detailed Tables")
    with st.expander("Constant", expanded=False):
        st.dataframe(CreateTable.constant(jsons), width="stretch")
    with st.expander("Topology", expanded=False):
        st.dataframe(CreateTable.topology(topologies), width="stretch")
    col1, col2 = st.columns(2)
    visualize_table(col1, jsons)
    col2.markdown("## Runtime")
    col2.dataframe(CreateTable.runtime(jsons), width="stretch")
    col2.markdown("## Gate")
    col2.dataframe(CreateTable.gate(jsons), width="stretch")
    col2.markdown("## Cell consumption")
    col2.dataframe(CreateTable.cell_consumption(jsons), width="stretch")


def render_time_series_tab(
    jsons: OrderedDict[str, dict],
    colors: OrderedDict[str, str],
    use_log_scale: bool,
) -> None:
    st.markdown("### Time Series")
    available = [
        k
        for k in TIME_SERIES_KEYS
        if all(isinstance(j.get(k, None), list) and len(j.get(k, [])) > 0 for j in jsons.values())
    ]
    if len(available) == 0:
        st.info("選択したファイルに時系列データがありません。")
        return

    maximum_of_time_series = max(calc_maximum_of_time_series(jsons, key=k) for k in available)
    col1, col2, col3 = st.columns([2, 1, 1])
    tmin, tmax = col1.slider(
        "Beat range",
        min_value=0,
        max_value=maximum_of_time_series,
        value=(0, maximum_of_time_series),
    )
    if tmin == tmax:
        st.warning("Beat range is empty.")
        return
    bin_size = col2.slider(
        "Bin size",
        min_value=1,
        max_value=max(10, int((tmax - tmin) / 40)),
        value=max(1, int((tmax - tmin) / 200)),
    )
    selected = col3.multiselect("Series", options=available, default=available[:2])
    if len(selected) == 0:
        st.info("Series を1つ以上選択してください。")
        return

    for key in selected:
        fig = create_time_series_fig(jsons, colors, key, tmin, tmax, bin_size)
        fig.update_layout(
            title=key.replace("_", " ").title(),
            yaxis_type="log" if use_log_scale else "linear",
            margin=dict(l=20, r=20, t=40, b=20),
        )
        st.plotly_chart(fig, width="stretch")


def render_topology_tab(topologies: OrderedDict[str, Topology], colors: OrderedDict[str, str]) -> None:
    st.markdown("### Topology Footprint")
    fig = go.Figure()
    for name, topology in topologies.items():
        for i, grid in enumerate(topology.grids):
            fig.add_trace(
                go.Scatter(
                    x=[0, grid.max_x, grid.max_x, 0, 0],
                    y=[0, 0, grid.max_y, grid.max_y, 0],
                    mode="lines",
                    name=f"{name} [grid {i}]",
                    marker_color=colors.get(name, "#0ea5e9"),
                )
            )
    fig.update_layout(
        xaxis_title="x",
        yaxis_title="y",
        yaxis_scaleanchor="x",
        margin=dict(l=20, r=20, t=20, b=20),
    )
    st.plotly_chart(fig, width="stretch")
    st.dataframe(CreateTable.topology(topologies), width="stretch")


def render_empty_state() -> None:
    st.markdown('<div class="viz-title">Compile Info Visualizer</div>', unsafe_allow_html=True)
    st.markdown(
        '<div class="viz-subtitle">右のサイドバーから profile JSON / compile_info JSON をアップロードしてください。</div>',
        unsafe_allow_html=True,
    )
    st.info("比較したいファイルを複数選ぶと、指標差分・時系列・トポロジーを同時に確認できます。")


def main() -> None:  # noqa:D103
    st.set_page_config(
        page_title="Visualize compile information",
        layout="wide",
        initial_sidebar_state="expanded",
    )
    inject_modern_style()

    st.sidebar.header("Inputs")
    upload_files = st.sidebar.file_uploader(
        "Upload JSON files",
        type=["json"],
        accept_multiple_files=True,
    )
    use_log_scale = st.sidebar.toggle("Use log scale", value=False)

    if upload_files is None or len(upload_files) == 0:
        render_empty_state()
        return
    if len(upload_files) > len(get_color_set()):
        st.error(f"Cannot upload more than {len(get_color_set())} files")
        return

    raw_jsons: OrderedDict[str, dict] = OrderedDict()
    topologies: OrderedDict[str, Topology] = OrderedDict()
    colors: OrderedDict[str, str] = OrderedDict()
    parse_errors = []

    for i, upload_file in enumerate(upload_files):
        colors[upload_file.name] = get_color_set()[i]
        if not st.sidebar.toggle(label=upload_file.name, value=True):
            continue
        try:
            raw = json.loads(upload_file.getvalue())
            info = extract_compile_info(raw)
            raw_jsons[upload_file.name] = info
            topologies[upload_file.name] = parse_topology(info)
        except Exception as ex:  # noqa: BLE001
            parse_errors.append((upload_file.name, str(ex)))

    if len(parse_errors) != 0:
        for name, message in parse_errors:
            st.error(f"{name}: {message}")
    if len(raw_jsons) == 0:
        st.warning("有効なファイルが選択されていません。")
        return

    st.markdown('<div class="viz-title">Compile Info Visualizer</div>', unsafe_allow_html=True)
    st.markdown(f'<div class="viz-subtitle">{get_summary(raw_jsons)}</div>', unsafe_allow_html=True)

    renamed = rename_labels(raw_jsons)
    if len(renamed) != len(set(renamed.values())):
        st.error("同じ表示名は使えません。")
        return

    jsons = apply_rename(raw_jsons, renamed)
    topologies = apply_rename(topologies, renamed)
    colors = apply_rename(colors, renamed)

    overview_df = build_overview_df(jsons)

    tab1, tab2, tab3, tab4 = st.tabs(["Overview", "Tables", "Time Series", "Topology"])
    with tab1:
        render_overview_tab(jsons, colors, overview_df, use_log_scale)
    with tab2:
        render_tables_tab(jsons, topologies)
    with tab3:
        render_time_series_tab(jsons, colors, use_log_scale)
    with tab4:
        render_topology_tab(topologies, colors)


main()
