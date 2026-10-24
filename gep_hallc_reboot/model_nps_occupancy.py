#!/usr/bin/env python3
"""Build a provisional data-anchored NPS occupancy envelope versus current."""

from __future__ import annotations

import csv
import math
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np

ROOT = Path(__file__).resolve().parent


def read_csv(path: Path) -> list[dict[str, float]]:
    with path.open(newline="") as stream:
        return [{key: float(value) for key, value in row.items()}
                for row in csv.DictReader(stream)]


def fit_poisson_through_origin(points: list[tuple[float, int, float]]) -> tuple[float, float]:
    """Fit -ln(1-p)=a*I using binomial errors propagated into occupancy."""
    x, y, weights = [], [], []
    for current, events, probability in points:
        probability = min(max(probability, 1e-9), 1. - 1e-9)
        occupancy = -math.log1p(-probability)
        sigma_p = math.sqrt(probability * (1. - probability) / events)
        sigma_occupancy = sigma_p / (1. - probability)
        x.append(current)
        y.append(occupancy)
        weights.append(1. / sigma_occupancy**2)
    x = np.asarray(x)
    y = np.asarray(y)
    weights = np.asarray(weights)
    slope = float(np.sum(weights*x*y) / np.sum(weights*x*x))
    slope_error = float(math.sqrt(1. / np.sum(weights*x*x)))
    return slope, slope_error


def probability(slope: float, current: np.ndarray | float) -> np.ndarray | float:
    return 1. - np.exp(-slope * current)


secondary_6151 = read_csv(ROOT / "nps_run6151_100k_secondary.csv")
secondary_2875 = read_csv(ROOT / "nps_run2875_100k_secondary.csv")
pulse_6151 = read_csv(ROOT / "plots/raw_block_probe_6151_5k.csv")
pulse_2875 = read_csv(ROOT / "plots/raw_block_probe_2875_5k.csv")

# Stable populated bands only. The 2875 low-current rows are beam transitions
# with tens to hundreds of elastic events and are not treated as scan points.
secondary_rows = secondary_6151 + [row for row in secondary_2875
                                   if 35. <= row["mean_current_uA"] < 40.]
pulse_rows = pulse_6151 + pulse_2875

series: dict[str, list[tuple[float, int, float]]] = {
    "Any extra cluster": [
        (r["mean_current_uA"], int(r["elastic_core"]), r["multicluster_fraction"])
        for r in secondary_rows
    ],
    "Energetic, far, out of time": [
        (r["mean_current_uA"], int(r["elastic_core"]),
         r["energetic_far_outtime_of_core"])
        for r in secondary_rows
    ],
    "In time and near": [
        (r["mean_current_uA"], int(r["elastic_core"]),
         r["multicluster_fraction"] * r["intime_and_near_fraction"])
        for r in secondary_rows
    ],
    "Isolated out-of-time pulse >=10 mV": [
        (r["mean_current_uA"], int(r["elastic_core"]),
         r["isolated_outtime_amp_ge_10mV_fraction"])
        for r in pulse_rows
    ],
}

colors = ["#1f77b4", "#d62728", "#2ca02c", "#9467bd"]
currents = np.linspace(0., 75., 301)
rows = []

fig, ax = plt.subplots(figsize=(10, 7))
for (label, points), color in zip(series.items(), colors):
    slope, slope_error = fit_poisson_through_origin(points)
    projected = probability(slope, 75.)
    projected_low = probability(max(0., slope-slope_error), 75.)
    projected_high = probability(slope+slope_error, 75.)
    rows.append({
        "observable": label,
        "poisson_slope_per_uA": slope,
        "slope_stat_error": slope_error,
        "projected_probability_75uA": projected,
        "projected_stat_low_75uA": projected_low,
        "projected_stat_high_75uA": projected_high,
    })
    x = np.asarray([point[0] for point in points])
    y = np.asarray([point[2] for point in points])
    n = np.asarray([point[1] for point in points])
    yerr = np.sqrt(y*(1.-y)/n)
    ax.errorbar(x, y, yerr=yerr, fmt="o", color=color, capsize=3)
    ax.plot(currents, probability(slope, currents), color=color, label=label)

ax.axvline(75., color="black", linestyle="--", linewidth=1)
ax.set(xlabel="Beam current (microampere)", ylabel="Event probability",
       title="Provisional data-anchored NPS occupancy envelope", xlim=(0., 78.), ylim=(0., .65))
ax.grid(alpha=.25)
ax.legend(loc="upper left", fontsize=9)
fig.tight_layout()
fig.savefig(ROOT / "plots/nps_occupancy_projection_75uA.png", dpi=180)
fig.savefig(ROOT / "plots/nps_occupancy_projection_75uA.pdf")

with (ROOT / "nps_occupancy_projection_75uA.csv").open("w", newline="") as stream:
    writer = csv.DictWriter(stream, fieldnames=rows[0].keys())
    writer.writeheader()
    writer.writerows(rows)

for row in rows:
    print(f"{row['observable']}: {100*row['projected_probability_75uA']:.2f}% "
          f"(stat {100*row['projected_stat_low_75uA']:.2f}--"
          f"{100*row['projected_stat_high_75uA']:.2f}%)")
