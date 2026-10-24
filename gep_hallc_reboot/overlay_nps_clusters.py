#!/usr/bin/env python3
"""Overlay measured NPS extra-cluster shapes on simulated elastic 3x3 sums."""

from __future__ import annotations

import csv
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np

ROOT = Path(__file__).resolve().parent
RNG_SEED = 751401
REPLICAS = 200
SIMC_YIELD = 41.341739
PITCH_CM = 2.169
LAMBDA_75UA = 0.008885310423698296 * 75.0


def rows(path: Path) -> list[dict[str, float]]:
    with path.open(newline="") as stream:
        return [{key: float(value) for key, value in row.items()}
                for row in csv.DictReader(stream)]


simulation = rows(ROOT / "gep_reboot_q11_trigger_events.csv")
library_rows = [row for row in rows(ROOT / "nps_overlay_6151_22p5_30uA.csv")
                if row["secondary_index"] >= 0 and row["secondary_energy_GeV"] > 0]

weights = np.asarray([row["weight"] for row in simulation])
energies = np.asarray([row["primary_energy_GeV"] for row in simulation])
base = np.asarray([row["sum3x3_fraction"] for row in simulation])
fiducial = np.asarray([
    row["local_x_mm"] >= -140. and row["local_x_mm"] <= 303.66
    and abs(row["local_y_mm"]) <= 368.73 for row in simulation
])
lib_energy = np.asarray([row["secondary_energy_GeV"] for row in library_rows])
lib_dt = np.asarray([row["secondary_dt_ns"] for row in library_rows])
lib_dx = np.asarray([row["secondary_x_cm"] - row["primary_x_cm"] for row in library_rows])
lib_dy = np.asarray([row["secondary_y_cm"] - row["primary_y_cm"] for row in library_rows])

scenarios = [
    ("No overlay", 0., 0.),
    ("3x3, |dt|<2 ns", 1.5*PITCH_CM, 2.),
    ("3x3, |dt|<4 ns", 1.5*PITCH_CM, 4.),
    ("5x5, |dt|<4 ns", 2.5*PITCH_CM, 4.),
]
thresholds = (0.7, 0.8)
output_rows = []

for scenario_index, (name, half_width, time_window) in enumerate(scenarios):
    rng = np.random.default_rng(RNG_SEED + scenario_index)
    efficiencies = {threshold: [] for threshold in thresholds}
    overlap_fractions = []
    rescued_fractions = {threshold: [] for threshold in thresholds}
    for _ in range(REPLICAS):
        added = np.zeros(len(simulation))
        overlapped = np.zeros(len(simulation), dtype=bool)
        if half_width > 0.:
            multiplicities = rng.poisson(LAMBDA_75UA, len(simulation))
            for event, multiplicity in enumerate(multiplicities):
                if multiplicity == 0:
                    continue
                indices = rng.integers(0, len(library_rows), multiplicity)
                selected = ((np.abs(lib_dt[indices]) < time_window)
                            & (np.abs(lib_dx[indices]) < half_width)
                            & (np.abs(lib_dy[indices]) < half_width))
                if np.any(selected):
                    added[event] = np.sum(lib_energy[indices][selected]) / energies[event]
                    overlapped[event] = True
        response = base + added
        fid_weight = np.sum(weights[fiducial])
        overlap_fractions.append(np.sum(weights[fiducial & overlapped]) / fid_weight)
        for threshold in thresholds:
            passed = fiducial & (response >= threshold)
            base_passed = fiducial & (base >= threshold)
            efficiencies[threshold].append(np.sum(weights[passed]) / np.sum(weights))
            rescued_fractions[threshold].append(
                np.sum(weights[passed & ~base_passed]) / np.sum(weights))
    for threshold in thresholds:
        values = np.asarray(efficiencies[threshold])
        rescued = np.asarray(rescued_fractions[threshold])
        output_rows.append({
            "scenario": name,
            "threshold_fraction": threshold,
            "mean_overlap_fraction_within_fiducial": np.mean(overlap_fractions),
            "mean_combined_efficiency": np.mean(values),
            "replica_rms_efficiency": np.std(values),
            "mean_rescued_fraction": np.mean(rescued),
            "yield_events_per_mC": SIMC_YIELD*np.mean(values),
        })

with (ROOT / "nps_cluster_overlay_75uA.csv").open("w", newline="") as stream:
    writer = csv.DictWriter(stream, fieldnames=output_rows[0].keys())
    writer.writeheader()
    writer.writerows(output_rows)

fig, axes = plt.subplots(1, 2, figsize=(13, 5.5))
scenario_names = [scenario[0] for scenario in scenarios]
x = np.arange(len(scenario_names))
overlap = [100*next(row["mean_overlap_fraction_within_fiducial"] for row in output_rows
                    if row["scenario"] == name) for name in scenario_names]
axes[0].plot(x, overlap, "o", markersize=8)
axes[0].set_title("Measured clusters entering local sum")
axes[0].set_ylabel("Fiducial event fraction (%)")
axes[0].set_xticks(x, scenario_names, rotation=22, ha="right")
axes[0].grid(axis="y", alpha=.25)
for threshold, marker in zip(thresholds, ("o", "s")):
    selected = [row for row in output_rows if row["threshold_fraction"] == threshold]
    rescued = 100*np.asarray([row["mean_rescued_fraction"] for row in selected])
    error = 100*np.asarray([row["replica_rms_efficiency"] for row in selected])
    axes[1].errorbar(x, rescued, yerr=error, fmt=marker+"-", capsize=4,
                     label=f"threshold / E = {threshold:.1f}")
axes[1].set_title("Events promoted above threshold")
axes[1].set_ylabel("Weighted fraction of SIMC sample (%)")
axes[1].set_xticks(x, scenario_names, rotation=22, ha="right")
axes[1].grid(axis="y", alpha=.25)
axes[1].legend()
fig.suptitle("Conservative measured-cluster overlay at 75 microampere")
fig.tight_layout()
fig.savefig(ROOT / "plots/nps_cluster_overlay_75uA.png", dpi=180)
fig.savefig(ROOT / "plots/nps_cluster_overlay_75uA.pdf")

for row in output_rows:
    print(f"{row['scenario']}, threshold={row['threshold_fraction']:.1f}: "
          f"overlap={100*row['mean_overlap_fraction_within_fiducial']:.3f}% "
          f"eff={100*row['mean_combined_efficiency']:.3f}% "
          f"rescued={100*row['mean_rescued_fraction']:.3f}% "
          f"yield={row['yield_events_per_mC']:.3f} events/mC")
