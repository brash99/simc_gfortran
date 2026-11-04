#!/usr/bin/env python3
"""Compare current-normalized three-pass and five-pass NPS electron rates."""

from __future__ import annotations

import csv
import math
import os
from pathlib import Path

os.environ.setdefault("MPLCONFIGDIR", "/tmp/gep_nps_matplotlib")

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np


HERE = Path(__file__).resolve().parent
PROPOSAL_ELECTRON_ENERGY_GEV = 4.838
FRACTIONS = (0.70, 0.75, 0.80)
SETS = {
    "6.370 GeV beam": {
        "runs": (6158, 6165, 6168),
        "color": "#0072B2",
        "marker": "o",
        "central_electron_energy_GeV": 4.598,
    },
    "10.54 GeV beam": {
        "runs": (2855, 5183, 3888),
        "color": "#D55E00",
        "marker": "s",
        "central_electron_energy_GeV": 7.072,
    },
}


def read_run(run: int) -> list[dict[str, float]]:
    path = HERE / f"nps_run{run}_100k_edtm_physical.csv"
    with path.open(newline="") as stream:
        return [
            {key: float(value) for key, value in row.items()}
            for row in csv.DictReader(stream)
        ]


def log_interpolate(
    rows: list[dict[str, float]], threshold_GeV: float
) -> tuple[float, float]:
    for low, high in zip(rows, rows[1:]):
        x0 = low["physical_threshold_GeV"]
        x1 = high["physical_threshold_GeV"]
        if x0 <= threshold_GeV <= x1:
            fraction = (threshold_GeV - x0) / (x1 - x0)
            rate = math.exp(
                math.log(low["primitive_rate_Hz"])
                + fraction
                * (
                    math.log(high["primitive_rate_Hz"])
                    - math.log(low["primitive_rate_Hz"])
                )
            )
            relative_error = (
                low["stat_error_Hz"] / low["primitive_rate_Hz"]
                + fraction
                * (
                    high["stat_error_Hz"] / high["primitive_rate_Hz"]
                    - low["stat_error_Hz"] / low["primitive_rate_Hz"]
                )
            )
            return rate, rate * relative_error
    raise ValueError(f"threshold {threshold_GeV} GeV is outside the scan")


def main() -> None:
    run_data = {
        run: read_run(run)
        for setting in SETS.values()
        for run in setting["runs"]
    }
    gap_low = max(
        run_data[run][0]["nps_angle_deg"] for run in SETS["10.54 GeV beam"]["runs"]
    )
    gap_high = min(
        run_data[run][0]["nps_angle_deg"] for run in SETS["6.370 GeV beam"]["runs"]
    )
    comparison_angle = 0.5 * (gap_low + gap_high)
    output_rows: list[dict[str, float | int | str]] = []

    fig, axes = plt.subplots(1, 3, figsize=(15.2, 4.8), sharex=True, sharey=True)
    for axis, fraction in zip(axes, FRACTIONS):
        threshold = fraction * PROPOSAL_ELECTRON_ENERGY_GEV
        fits: dict[str, tuple[float, float]] = {}
        for label, setting in SETS.items():
            points = []
            for run in setting["runs"]:
                rows = run_data[run]
                rate, error = log_interpolate(rows, threshold)
                current = rows[0]["mean_selected_current_uA"]
                points.append(
                    (
                        rows[0]["nps_angle_deg"],
                        rate / current / 1000.0,
                        error / current / 1000.0,
                        run,
                    )
                )
            points.sort()
            angles = np.array([point[0] for point in points])
            rates = np.array([point[1] for point in points])
            errors = np.array([point[2] for point in points])
            slope, intercept = np.polyfit(angles, np.log(rates), 1)
            fits[label] = (slope, intercept)

            axis.errorbar(
                angles,
                rates,
                yerr=errors,
                fmt=setting["marker"],
                ms=5.5,
                capsize=2.5,
                color=setting["color"],
                label=(
                    f"{label}; E'e={setting['central_electron_energy_GeV']:.3f} GeV"
                    if fraction == FRACTIONS[0]
                    else None
                ),
                zorder=3,
            )
            measured_grid = np.linspace(angles.min(), angles.max(), 100)
            axis.plot(
                measured_grid,
                np.exp(intercept + slope * measured_grid),
                color=setting["color"],
                lw=1.8,
            )
            gap_grid = np.linspace(gap_low, gap_high, 100)
            axis.plot(
                gap_grid,
                np.exp(intercept + slope * gap_grid),
                color=setting["color"],
                lw=1.4,
                ls="--",
            )
            for angle, rate, error, run in points:
                output_rows.append(
                    {
                        "threshold_fraction": fraction,
                        "physical_threshold_GeV": threshold,
                        "beam_setting": label,
                        "run": run,
                        "nps_angle_deg": angle,
                        "rate_per_uA_kHz": rate,
                        "stat_error_per_uA_kHz": error,
                        "fit_log_slope_per_deg": slope,
                        "fit_log_intercept": intercept,
                        "comparison_angle_deg": comparison_angle,
                        "fit_at_comparison_angle_kHz_per_uA": math.exp(
                            intercept + slope * comparison_angle
                        ),
                    }
                )

        low_fit = fits["6.370 GeV beam"]
        high_fit = fits["10.54 GeV beam"]
        low_value = math.exp(low_fit[1] + low_fit[0] * comparison_angle)
        high_value = math.exp(high_fit[1] + high_fit[0] * comparison_angle)
        ratio = high_value / low_value
        axis.axvspan(gap_low, gap_high, color="0.88", alpha=0.75, zorder=0)
        axis.axvline(comparison_angle, color="0.45", lw=0.8, ls=":")
        axis.text(
            0.04,
            0.05,
            f"Fit ratio at {comparison_angle:.2f} deg\n10.54 / 6.37 GeV = {ratio:.2f}",
            transform=axis.transAxes,
            fontsize=9,
            va="bottom",
        )
        axis.set_title(f"{fraction:.0%}: {threshold:.3f} GeV")
        axis.set_xlabel("NPS central angle (degrees)")
        axis.grid(alpha=0.22)

    axes[0].set_ylabel("Cluster-trigger rate / beam current (kHz/uA)")
    axes[0].legend(frameon=False, fontsize=8.5, loc="upper right")
    axes[0].set_xlim(14.3, 21.6)
    axes[0].set_yscale("log")
    fig.suptitle(
        "NPS electron singles: separate angular trends at two beam energies\n"
        "Points are data; solid lines span measured angles; dashed lines cross the unmeasured gap",
        y=1.04,
    )
    fig.tight_layout()

    output_base = HERE / "plots" / "nps_beam_energy_angle_comparison"
    output_base.parent.mkdir(exist_ok=True)
    fig.savefig(output_base.with_suffix(".png"), dpi=180, bbox_inches="tight")
    fig.savefig(output_base.with_suffix(".pdf"), bbox_inches="tight")

    output_csv = HERE / "nps_beam_energy_angle_comparison.csv"
    with output_csv.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=output_rows[0].keys())
        writer.writeheader()
        writer.writerows(output_rows)

    for fraction in FRACTIONS:
        rows = [row for row in output_rows if row["threshold_fraction"] == fraction]
        values = {}
        for label in SETS:
            row = next(row for row in rows if row["beam_setting"] == label)
            values[label] = float(row["fit_at_comparison_angle_kHz_per_uA"])
        print(
            f"{fraction:.0%}: at {comparison_angle:.2f} deg, "
            f"6.370 GeV={values['6.370 GeV beam']:.3f} kHz/uA, "
            f"10.54 GeV={values['10.54 GeV beam']:.3f} kHz/uA, "
            f"ratio={values['10.54 GeV beam']/values['6.370 GeV beam']:.3f}"
        )


if __name__ == "__main__":
    main()
