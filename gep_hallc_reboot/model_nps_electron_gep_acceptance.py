#!/usr/bin/env python3
"""Data-anchored NPS electron-rate extrapolation to the GEp 3 m geometry.

The calibration observables are rates integrated over the complete NPS face.
For each threshold, this script fits their angular dependence after integrating
an assumed differential rate density over the actual 9.5 m face, then performs
the same integral over the proposed 3 m face.  Exponential and power-law angular
models expose the dominant out-of-range extrapolation dependence.
"""

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
from matplotlib.lines import Line2D
from matplotlib.patches import Patch
from scipy.optimize import least_squares


HERE = Path(__file__).resolve().parent
RUNS = (6158, 6165, 6168)
PITCH_M = 0.0205
FACE_WIDTH_M = 30 * PITCH_M
FACE_HEIGHT_M = 36 * PITCH_M
CALIBRATION_DISTANCE_M = 9.5
GEP_DISTANCE_M = 3.0
GEP_CENTRAL_ANGLE_DEG = 26.651
GEP_CURRENT_UA = 75.0
TARGET_LENGTH_RATIO = 30.0 / 10.0
GEP_ELECTRON_ENERGY_GEV = 4.838
THRESHOLD_FRACTIONS = (0.70, 0.75, 0.80)


def read_run(run: int) -> list[dict[str, float]]:
    with (HERE / f"nps_run{run}_100k_edtm_physical.csv").open(newline="") as stream:
        return [{key: float(value) for key, value in row.items()} for row in csv.DictReader(stream)]


def log_interpolate(rows: list[dict[str, float]], threshold: float) -> tuple[float, float]:
    for low, high in zip(rows, rows[1:]):
        x0, x1 = low["physical_threshold_GeV"], high["physical_threshold_GeV"]
        if x0 <= threshold <= x1:
            f = (threshold - x0) / (x1 - x0)
            rate = math.exp(math.log(low["rate_per_uA_Hz"]) + f * (
                math.log(high["rate_per_uA_Hz"]) - math.log(low["rate_per_uA_Hz"])))
            rel0 = low["stat_error_Hz"] / low["primitive_rate_Hz"]
            rel1 = high["stat_error_Hz"] / high["primitive_rate_Hz"]
            return rate, rate * (rel0 + f * (rel1 - rel0))
    raise ValueError(f"threshold {threshold} GeV is outside the measured scan")


def face_quadrature(center_deg: float, distance_m: float) -> tuple[np.ndarray, np.ndarray]:
    """Return polar angles and solid-angle weights for midpoint face elements."""
    nx, ny = 240, 288
    x = (np.arange(nx) + 0.5) * FACE_WIDTH_M / nx - FACE_WIDTH_M / 2
    y = (np.arange(ny) + 0.5) * FACE_HEIGHT_M / ny - FACE_HEIGHT_M / 2
    xx, yy = np.meshgrid(x, y, indexing="xy")
    center = np.deg2rad(center_deg)
    # r = D*n + x*u + y*v, with n=(sin(theta0),0,cos(theta0))
    rx = distance_m * np.sin(center) + xx * np.cos(center)
    rz = distance_m * np.cos(center) - xx * np.sin(center)
    radius = np.sqrt(distance_m**2 + xx**2 + yy**2)
    theta = np.arccos(rz / radius)
    area = (FACE_WIDTH_M / nx) * (FACE_HEIGHT_M / ny)
    domega = distance_m * area / radius**3
    return theta, domega


QUADRATURE = {
    (run, CALIBRATION_DISTANCE_M): face_quadrature(read_run(run)[0]["nps_angle_deg"], CALIBRATION_DISTANCE_M)
    for run in RUNS
}
QUADRATURE[(0, GEP_DISTANCE_M)] = face_quadrature(GEP_CENTRAL_ANGLE_DEG, GEP_DISTANCE_M)


def shape(theta: np.ndarray, slope: float, model: str) -> np.ndarray:
    if model == "exponential":
        return np.exp(-slope * theta)
    if model == "power":
        return theta ** (-slope)
    raise ValueError(model)


def face_integral(key: tuple[int, float], slope: float, model: str) -> float:
    theta, domega = QUADRATURE[key]
    return float(np.sum(shape(theta, slope, model) * domega))


def face_integral_at_angle(center_deg: float, distance_m: float, slope: float, model: str) -> float:
    theta, domega = face_quadrature(center_deg, distance_m)
    return float(np.sum(shape(theta, slope, model) * domega))


def fit_model(
    measured: list[tuple[int, float, float]], model: str
) -> tuple[float, float, float, float]:
    """Return normalization, slope, GEp per-uA/10-cm rate, and chi2."""
    def residual(parameters: np.ndarray) -> np.ndarray:
        log_norm, log_slope = parameters
        norm, slope = math.exp(log_norm), math.exp(log_slope)
        values = []
        for run, rate, error in measured:
            prediction = norm * face_integral((run, CALIBRATION_DISTANCE_M), slope, model)
            values.append((math.log(prediction) - math.log(rate)) / (error / rate))
        return np.asarray(values)

    initial_slope = 12.0 if model == "exponential" else 5.0
    first_integral = face_integral((measured[0][0], CALIBRATION_DISTANCE_M), initial_slope, model)
    initial_norm = measured[0][1] / first_integral
    result = least_squares(residual, np.log([initial_norm, initial_slope]))
    norm, slope = np.exp(result.x)
    gep_rate = norm * face_integral((0, GEP_DISTANCE_M), slope, model)
    return norm, slope, gep_rate, float(np.sum(result.fun**2))


def main() -> None:
    run_data = {run: read_run(run) for run in RUNS}
    rows: list[dict[str, float | str]] = []
    for fraction in THRESHOLD_FRACTIONS:
        threshold = fraction * GEP_ELECTRON_ENERGY_GEV
        measured = []
        for run in RUNS:
            rate, error = log_interpolate(run_data[run], threshold)
            measured.append((run, rate, error))
        for model in ("exponential", "power"):
            _, slope, reference_rate, chi2 = fit_model(measured, model)
            rows.append({
                "threshold_fraction": fraction,
                "physical_threshold_GeV": threshold,
                "angular_model": model,
                "fitted_slope": slope,
                "fit_chi2_1dof": chi2,
                "gep_rate_per_uA_at_10cm_Hz": reference_rate,
                "gep_rate_75uA_30cm_Hz": reference_rate * GEP_CURRENT_UA * TARGET_LENGTH_RATIO,
            })

    output_csv = HERE / "nps_electron_gep_acceptance_projection.csv"
    with output_csv.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=rows[0].keys())
        writer.writeheader()
        writer.writerows(rows)

    fig, axes = plt.subplots(1, 2, figsize=(12.4, 5.0), gridspec_kw={"width_ratios": [1.55, 1.0]})
    theta_grid = np.linspace(17.5, 33.5, 240)
    gep_theta = np.rad2deg(QUADRATURE[(0, GEP_DISTANCE_M)][0])
    gep_theta_min, gep_theta_max = float(gep_theta.min()), float(gep_theta.max())
    colors = {0.70: "#0072B2", 0.75: "#E69F00", 0.80: "#009E73"}
    axis = axes[0]
    for model, line_style in (("exponential", "-"), ("power", "--")):
        for fraction in THRESHOLD_FRACTIONS:
            threshold = fraction * GEP_ELECTRON_ENERGY_GEV
            measured = [(run, *log_interpolate(run_data[run], threshold)) for run in RUNS]
            norm, slope, _, _ = fit_model(measured, model)
            fitted_face_rates = [
                norm * face_integral_at_angle(angle, CALIBRATION_DISTANCE_M, slope, model)
                for angle in theta_grid
            ]
            axis.plot(theta_grid, fitted_face_rates, color=colors[fraction], ls=line_style)
            if model == "exponential":
                measured_angles = [run_data[run][0]["nps_angle_deg"] for run, _, _ in measured]
                measured_rates = [rate for _, rate, _ in measured]
                measured_errors = [error for _, _, error in measured]
                axis.errorbar(measured_angles, measured_rates, yerr=measured_errors,
                              marker="o", ls="none", ms=5.5, capsize=3,
                              color=colors[fraction], markeredgecolor="black", markeredgewidth=0.5)
    axis.axvspan(gep_theta_min, gep_theta_max, color="0.75", alpha=0.25)
    axis.set_title("Measured angular dependence and two fitted models")
    axis.set_xlabel("NPS central angle / sampled electron polar angle (deg)")
    axis.set_yscale("log")
    axis.grid(alpha=0.25)
    axes[0].set_ylabel("Full-face primitive rate at 9.5 m (Hz/uA)")
    threshold_handles = [
        Line2D([0], [0], color=colors[fraction], marker="o", lw=2,
               markeredgecolor="black", markeredgewidth=0.5, label=f"{fraction:.0%}")
        for fraction in THRESHOLD_FRACTIONS
    ]
    meaning_handles = [
        Line2D([0], [0], color="black", marker="o", ls="none", label="Measured data"),
        Line2D([0], [0], color="black", ls="-", label="Exponential model"),
        Line2D([0], [0], color="black", ls="--", label="Power-law model"),
        Patch(facecolor="0.75", alpha=0.25, label="GEp 3 m face span"),
    ]
    threshold_legend = axis.legend(handles=threshold_handles, title="Threshold / 4.838 GeV",
                                    loc="upper right", frameon=False, fontsize=8)
    axis.add_artist(threshold_legend)
    axis.legend(handles=meaning_handles, loc="lower left", frameon=False, fontsize=8)

    x = np.arange(len(THRESHOLD_FRACTIONS))
    width = 0.34
    for offset, model, color in ((-width / 2, "exponential", "#56B4E9"),
                                  (width / 2, "power", "#D55E00")):
        values = [next(float(row["gep_rate_75uA_30cm_Hz"]) for row in rows
                       if row["threshold_fraction"] == fraction and row["angular_model"] == model) / 1000
                  for fraction in THRESHOLD_FRACTIONS]
        axes[1].bar(x + offset, values, width, label=model.capitalize(), color=color)
    axes[1].set_xticks(x, [f"{value:.0%}" for value in THRESHOLD_FRACTIONS])
    axes[1].set_xlabel("GEp threshold / 4.838 GeV")
    axes[1].set_ylabel("Projected full-face primitive rate (kHz)")
    axes[1].set_title("75 uA, 30 cm LH2, NPS at 3 m")
    axes[1].grid(axis="y", alpha=0.25)
    axes[1].legend(frameon=False)
    fig.suptitle("NPS electron singles: data-anchored extrapolation to the GEp geometry", y=1.02)
    fig.tight_layout()
    output_base = HERE / "plots" / "nps_electron_gep_acceptance_projection"
    fig.savefig(output_base.with_suffix(".png"), dpi=180, bbox_inches="tight")
    fig.savefig(output_base.with_suffix(".pdf"), bbox_inches="tight")

    theta, _ = QUADRATURE[(0, GEP_DISTANCE_M)]
    print(f"NPS active face: {FACE_WIDTH_M:.4f} x {FACE_HEIGHT_M:.4f} m")
    print(f"GEp face polar-angle extent: {np.rad2deg(theta).min():.3f} to {np.rad2deg(theta).max():.3f} deg")
    for row in rows:
        print(f"{float(row['threshold_fraction']):.0%} {row['angular_model']}: "
              f"{float(row['gep_rate_75uA_30cm_Hz']) / 1000:.2f} kHz "
              f"(chi2/1dof={float(row['fit_chi2_1dof']):.2f})")


if __name__ == "__main__":
    main()
