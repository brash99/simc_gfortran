#!/usr/bin/env python3
"""Compare run-6887 EDTM illumination with absolute folded-electron rates."""

from __future__ import annotations

import csv
import os
import argparse
from pathlib import Path

os.environ.setdefault("MPLBACKEND", "Agg")
os.environ.setdefault("MPLCONFIGDIR", "/tmp/nps_run6887_spatial_matplotlib")

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

HERE = Path(__file__).resolve().parent
DATA = HERE / "nps_run6887_edtm_topology.csv"
MEASURED_CURVE = HERE / "nps_run6887_edtm_threshold_curve.csv"
FULL_EVENTS = Path("/tmp/nps_run6887_inclusive_5m.csv")
HYBRID = Path("/tmp/nps_run6887_inclusive_geant4_pilot.hybrid.csv")
RESPONSE = Path("/tmp/nps_run6887_inclusive_geant4_response.csv")
MINIMUM_TRANSPORT_ENERGY_GEV = 2.3


def measured_rate(threshold_gev: float) -> tuple[float, float]:
    with MEASURED_CURVE.open(newline="") as stream:
        rows = list(csv.DictReader(stream))
    threshold = np.asarray([float(row["threshold_GeV"]) for row in rows])
    rate = np.asarray([float(row["poisson_rate_Hz"]) for row in rows])
    error = np.asarray([float(row["stat_error_Hz"]) for row in rows])
    return (
        float(np.interp(threshold_gev, threshold, rate)),
        float(np.interp(threshold_gev, threshold, error)),
    )


def full_weight_above_transport_cut() -> float:
    total = 0.0
    with FULL_EVENTS.open(newline="") as stream:
        for row in csv.DictReader(stream):
            if float(row["electron_energy_GeV"]) >= MINIMUM_TRANSPORT_ENERGY_GEV:
                total += float(row["weight_Hz"])
    return total


def simulated_events() -> pd.DataFrame:
    weights = []
    with HYBRID.open(newline="") as stream:
        rows = csv.DictReader(line for line in stream if not line.startswith("#"))
        for index, row in enumerate(rows):
            if index == 20_000:
                break
            weights.append(float(row["weight"]))
    response = pd.read_csv(RESPONSE)
    required = {"hottest_column", "hottest_row", "corrected_3x3_energy_GeV"}
    missing = required.difference(response.columns)
    if missing:
        raise ValueError(
            "GEANT4 response must be re-extracted with coordinates; missing "
            + ", ".join(sorted(missing))
        )
    if len(response) != len(weights):
        raise ValueError("Hybrid weights and GEANT4 response lengths differ")
    response["weight"] = np.asarray(weights)
    scale = full_weight_above_transport_cut() / response.weight.sum()
    response["absolute_weight_Hz"] = scale * response.weight
    return response


def coarse_map(x, y, weights=None):
    return np.histogram2d(
        x,
        y,
        bins=(np.arange(-0.5, 30.0, 5.0), np.arange(-0.5, 36.0, 6.0)),
        weights=weights,
    )[0]


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--threshold", type=float, default=3.387)
    args = parser.parse_args()
    threshold_gev = args.threshold
    threshold_tag = f"{threshold_gev:.3f}".replace(".", "p")
    output = HERE / f"nps_run6887_spatial_residual_{threshold_tag}"

    data = pd.read_csv(DATA)
    data = data[data.max_energy_GeV >= threshold_gev]
    total_rate, total_rate_error = measured_rate(threshold_gev)
    data_counts = coarse_map(data.max_x_column, data.max_y_row)
    data_map = total_rate * data_counts / data_counts.sum()
    # The shape uncertainty dominates per bin; include the independent total-rate
    # uncertainty in quadrature.  This is an approximate diagnostic covariance,
    # not a fit likelihood.
    data_error = np.sqrt(
        np.square(total_rate * np.sqrt(data_counts) / data_counts.sum())
        + np.square(total_rate_error * data_counts / data_counts.sum())
    )

    simulation = simulated_events()
    simulation = simulation[
        simulation.corrected_3x3_energy_GeV >= threshold_gev
    ]
    electron_map = coarse_map(
        simulation.hottest_column,
        simulation.hottest_row,
        simulation.absolute_weight_Hz,
    )
    electron_variance = coarse_map(
        simulation.hottest_column,
        simulation.hottest_row,
        np.square(simulation.absolute_weight_Hz),
    )
    electron_error = np.sqrt(electron_variance)
    residual = data_map - electron_map
    residual_error = np.sqrt(np.square(data_error) + electron_variance)
    pull = np.divide(
        residual,
        residual_error,
        out=np.zeros_like(residual),
        where=residual_error > 0,
    )

    fig, axes = plt.subplots(2, 2, figsize=(11.2, 8.6), sharex=True, sharey=True)
    maps = [data_map, electron_map, residual, pull]
    titles = [
        "Measured total activity",
        "Folded inclusive electrons",
        "Data - modeled electrons",
        "Residual / statistical uncertainty",
    ]
    sequential_max = max(data_map.max(), electron_map.max()) / 1000.0
    residual_limit = np.max(np.abs(residual)) / 1000.0
    for axis, values, title in zip(axes.flat, maps, titles):
        if title == titles[3]:
            limit = max(3.0, float(np.max(np.abs(values))))
            image = axis.imshow(
                values.T,
                origin="lower",
                extent=(-0.5, 29.5, -0.5, 35.5),
                aspect="auto",
                cmap="RdBu_r",
                vmin=-limit,
                vmax=limit,
            )
            label = "Approximate significance"
        elif title == titles[2]:
            image = axis.imshow(
                values.T / 1000.0,
                origin="lower",
                extent=(-0.5, 29.5, -0.5, 35.5),
                aspect="auto",
                cmap="RdBu_r",
                vmin=-residual_limit,
                vmax=residual_limit,
            )
            label = "Rate per coarse bin (kHz)"
        else:
            image = axis.imshow(
                values.T / 1000.0,
                origin="lower",
                extent=(-0.5, 29.5, -0.5, 35.5),
                aspect="auto",
                cmap="magma",
                vmin=0.0,
                vmax=sequential_max,
            )
            label = "Rate per coarse bin (kHz)"
        fig.colorbar(image, ax=axis, label=label)
        axis.set_title(title)
        axis.set_xlabel("NPS column")
        axis.set_ylabel("NPS row")

    fig.suptitle(
        f"NPS run 6887 spatial comparison at {threshold_gev:.3f} GeV",
        fontsize=14,
    )
    fig.text(
        0.5,
        0.012,
        "Absolute generator normalization; 5x6-crystal bins; residual is unmodeled electromagnetic activity, not an identified photon map.",
        ha="center",
        fontsize=8.5,
    )
    fig.tight_layout(rect=(0, 0.035, 1, 0.96))
    fig.savefig(output.with_suffix(".png"), dpi=180)
    fig.savefig(output.with_suffix(".pdf"))

    def weighted_moments(values, weights):
        mean = float(np.average(values, weights=weights))
        rms = float(np.sqrt(np.average(np.square(values - mean), weights=weights)))
        return mean, rms

    data_column_mean, data_column_rms = weighted_moments(
        data.max_x_column.to_numpy(), np.ones(len(data))
    )
    data_row_mean, data_row_rms = weighted_moments(
        data.max_y_row.to_numpy(), np.ones(len(data))
    )
    electron_column_mean, electron_column_rms = weighted_moments(
        simulation.hottest_column.to_numpy(), simulation.absolute_weight_Hz.to_numpy()
    )
    electron_row_mean, electron_row_rms = weighted_moments(
        simulation.hottest_row.to_numpy(), simulation.absolute_weight_Hz.to_numpy()
    )
    with output.with_suffix(".csv").open("w", newline="") as stream:
        writer = csv.writer(stream)
        writer.writerow(
            [
                "threshold_GeV", "data_windows", "simulated_events",
                "data_rate_Hz", "data_stat_error_Hz", "electron_rate_Hz",
                "electron_mc_error_Hz", "residual_rate_Hz",
                "data_column_mean", "data_column_rms", "electron_column_mean",
                "electron_column_rms", "data_row_mean", "data_row_rms",
                "electron_row_mean", "electron_row_rms",
            ]
        )
        writer.writerow(
            [
                threshold_gev, len(data), len(simulation), data_map.sum(),
                total_rate_error, electron_map.sum(),
                np.sqrt(electron_variance.sum()), residual.sum(),
                data_column_mean, data_column_rms, electron_column_mean,
                electron_column_rms, data_row_mean, data_row_rms,
                electron_row_mean, electron_row_rms,
            ]
        )

    print(
        f"data={data_map.sum()/1000:.3f}+/-{total_rate_error/1000:.3f} kHz "
        f"electrons={electron_map.sum()/1000:.3f}+/-"
        f"{np.sqrt(electron_variance.sum())/1000:.3f} kHz "
        f"residual={residual.sum()/1000:.3f} kHz"
    )


if __name__ == "__main__":
    main()
