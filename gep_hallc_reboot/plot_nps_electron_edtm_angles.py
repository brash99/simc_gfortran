#!/usr/bin/env python3
"""Plot unbiased NPS electron primitive rates for runs 6158, 6165, and 6168."""

from __future__ import annotations

import csv
import math
import os
from pathlib import Path

os.environ.setdefault("MPLCONFIGDIR", "/tmp/gep_nps_matplotlib")

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt


HERE = Path(__file__).resolve().parent
RUNS = (6158, 6165, 6168)
COLORS = {6158: "#0072B2", 6165: "#E69F00", 6168: "#009E73"}
PROPOSAL_ELECTRON_ENERGY_GEV = 4.838
FRACTIONS = (0.70, 0.75, 0.80)
TARGET_LENGTH_SCALE = 30.0 / 10.0
PROPOSAL_CURRENT_UA = 75.0


def read_run(run: int) -> list[dict[str, float]]:
    path = HERE / f"nps_run{run}_100k_edtm_physical.csv"
    with path.open(newline="") as stream:
        return [{key: float(value) for key, value in row.items()} for row in csv.DictReader(stream)]


def log_interpolate(rows: list[dict[str, float]], threshold: float) -> tuple[float, float]:
    """Log-interpolate rate; interpolate its fractional statistical error linearly."""
    for low, high in zip(rows, rows[1:]):
        x0 = low["physical_threshold_GeV"]
        x1 = high["physical_threshold_GeV"]
        if x0 <= threshold <= x1:
            fraction = (threshold - x0) / (x1 - x0)
            rate = math.exp(
                math.log(low["primitive_rate_Hz"])
                + fraction
                * (math.log(high["primitive_rate_Hz"]) - math.log(low["primitive_rate_Hz"]))
            )
            rel0 = low["stat_error_Hz"] / low["primitive_rate_Hz"]
            rel1 = high["stat_error_Hz"] / high["primitive_rate_Hz"]
            relative_error = rel0 + fraction * (rel1 - rel0)
            return rate, rate * relative_error
    raise ValueError(f"threshold {threshold} GeV lies outside measured scan")


def main() -> None:
    data = {run: read_run(run) for run in RUNS}
    key_rows: list[dict[str, float]] = []

    for run, rows in data.items():
        current = rows[0]["mean_selected_current_uA"]
        scale = (PROPOSAL_CURRENT_UA / current) * TARGET_LENGTH_SCALE
        for fraction in FRACTIONS:
            threshold = fraction * PROPOSAL_ELECTRON_ENERGY_GEV
            rate, error = log_interpolate(rows, threshold)
            key_rows.append(
                {
                    "run": run,
                    "nps_angle_deg": rows[0]["nps_angle_deg"],
                    "mean_selected_current_uA": current,
                    "proposal_fraction": fraction,
                    "physical_threshold_GeV": threshold,
                    "measured_rate_Hz": rate,
                    "measured_stat_error_Hz": error,
                    "naive_luminosity_scale": scale,
                    "naive_scaled_rate_Hz": rate * scale,
                    "naive_scaled_stat_error_Hz": error * scale,
                }
            )

    output_csv = HERE / "nps_electron_edtm_angle_key_points.csv"
    with output_csv.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=key_rows[0].keys())
        writer.writeheader()
        writer.writerows(key_rows)

    fig, axes = plt.subplots(1, 2, figsize=(12.4, 5.1), sharex=True)
    for run, rows in data.items():
        angle = rows[0]["nps_angle_deg"]
        thresholds = [row["physical_threshold_GeV"] for row in rows]
        rates = [row["primitive_rate_Hz"] / 1000.0 for row in rows]
        errors = [row["stat_error_Hz"] / 1000.0 for row in rows]
        current = rows[0]["mean_selected_current_uA"]
        scale = (PROPOSAL_CURRENT_UA / current) * TARGET_LENGTH_SCALE
        label = f"Run {run}: {angle:.2f} deg"
        axes[0].errorbar(thresholds, rates, yerr=errors, marker="o", ms=3.5,
                         capsize=2, lw=1.5, color=COLORS[run], label=label)
        axes[1].errorbar(thresholds, [value * scale for value in rates],
                         yerr=[value * scale for value in errors], marker="o", ms=3.5,
                         capsize=2, lw=1.5, color=COLORS[run], label=label)

    for fraction in FRACTIONS:
        threshold = fraction * PROPOSAL_ELECTRON_ENERGY_GEV
        for axis in axes:
            axis.axvline(threshold, color="0.45", ls="--", lw=0.9)
        axes[0].text(threshold + 0.015, axes[0].get_ylim()[1] * 0.78,
                     f"{fraction:.0%}", rotation=90, va="top", fontsize=9, color="0.3")

    axes[0].set_title("Measured electron-run primitive rate")
    axes[0].set_ylabel("NPS primitive rate (kHz)")
    axes[1].set_title("Naive luminosity-scaled reference only")
    axes[1].set_ylabel("Rate x (75 uA / I) x (30 cm / 10 cm) (kHz)")
    for axis in axes:
        axis.set_xlabel("Offline-equivalent NPS threshold (GeV)")
        axis.set_xlim(2.25, 4.65)
        axis.set_ylim(bottom=0)
        axis.grid(alpha=0.25)
        axis.legend(frameon=False, fontsize=9)
    fig.suptitle("Unbiased EDTM windows: NPS used as an electron detector at 9.5 m", y=1.01)
    fig.tight_layout()

    output_base = HERE / "plots" / "nps_electron_edtm_angle_scan"
    output_base.parent.mkdir(exist_ok=True)
    fig.savefig(output_base.with_suffix(".png"), dpi=180, bbox_inches="tight")
    fig.savefig(output_base.with_suffix(".pdf"), bbox_inches="tight")

    for row in key_rows:
        print(
            f"run {int(row['run'])} angle={row['nps_angle_deg']:.2f} deg "
            f"threshold={row['proposal_fraction']:.0%} ({row['physical_threshold_GeV']:.4f} GeV): "
            f"measured={row['measured_rate_Hz']/1000:.3f} +/- "
            f"{row['measured_stat_error_Hz']/1000:.3f} kHz; "
            f"naive_scaled={row['naive_scaled_rate_Hz']/1000:.2f} kHz"
        )


if __name__ == "__main__":
    main()
