#!/usr/bin/env python3
"""Project the measured 1.6-GeV pTRIG1 rate to the GEp 3 m geometry."""

from __future__ import annotations

import csv
import math
from pathlib import Path

import numpy as np
from scipy.optimize import least_squares

import model_nps_electron_gep_acceptance as acceptance


HERE = Path(__file__).resolve().parent
HMS_S1_RATE_HZ = 806_000.0
HMS_THREE_OF_FOUR_RATE_HZ = 2_900.0
COINCIDENCE_WINDOWS_S = (20e-9, 40e-9)


def read_measurements() -> list[tuple[int, float, float]]:
    measurements = []
    for run in acceptance.RUNS:
        path = HERE / f"nps_run{run}_100k_edtm_scaler_closure.csv"
        with path.open(newline="") as stream:
            row = {key: float(value) for key, value in next(csv.DictReader(stream)).items()}
        # Use the direct scaler as the central value. Its snapshot RMS is retained
        # as a conservative point-to-point variation, not an error on the mean.
        measurements.append((
            run,
            row["ptrig1_scaler_rate_Hz"] / row["mean_scaler_current_uA"],
            row["ptrig1_scaler_rms_Hz"] / row["mean_scaler_current_uA"],
        ))
    return measurements


def fit_and_project(measured: list[tuple[int, float, float]], model: str) -> tuple[float, float, float]:
    def residual(parameters: np.ndarray) -> np.ndarray:
        norm, slope = np.exp(parameters)
        return np.asarray([
            (math.log(norm * acceptance.face_integral(
                (run, acceptance.CALIBRATION_DISTANCE_M), slope, model)) - math.log(rate))
            / (error / rate)
            for run, rate, error in measured
        ])

    initial_slope = 12.0 if model == "exponential" else 5.0
    initial_norm = measured[0][1] / acceptance.face_integral(
        (measured[0][0], acceptance.CALIBRATION_DISTANCE_M), initial_slope, model)
    fit = least_squares(residual, np.log([initial_norm, initial_slope]))
    covariance = np.linalg.inv(fit.jac.T @ fit.jac)

    def log_projection(parameters: np.ndarray) -> float:
        norm, slope = np.exp(parameters)
        reference = norm * acceptance.face_integral(
            (0, acceptance.GEP_DISTANCE_M), slope, model)
        return math.log(reference * acceptance.GEP_CURRENT_UA * acceptance.TARGET_LENGTH_RATIO)

    step = 1e-5
    gradient = np.asarray([
        (log_projection(fit.x + np.eye(2)[index] * step)
         - log_projection(fit.x - np.eye(2)[index] * step)) / (2 * step)
        for index in range(2)
    ])
    projected_rate = math.exp(log_projection(fit.x))
    projected_error = projected_rate * math.sqrt(gradient @ covariance @ gradient)
    return projected_rate, projected_error, float(np.sum(fit.fun**2))


def main() -> None:
    measured = read_measurements()
    rows = []
    for model in ("exponential", "power"):
        rate, error, chi2 = fit_and_project(measured, model)
        row: dict[str, float | str] = {
            "vtp_threshold_GeV": 1.6,
            "mean_offline_equivalent_threshold_GeV": 1.4583,
            "fraction_of_GEp_electron_energy": 1.4583 / acceptance.GEP_ELECTRON_ENERGY_GEV,
            "angular_model": model,
            "projected_nps_rate_Hz": rate,
            "projected_point_variation_Hz": error,
            "fit_chi2_1dof": chi2,
        }
        for trigger_name, trigger_rate in (("hms_s1", HMS_S1_RATE_HZ),
                                           ("hms_3of4", HMS_THREE_OF_FOUR_RATE_HZ)):
            for width in COINCIDENCE_WINDOWS_S:
                width_ns = int(round(width * 1e9))
                row[f"{trigger_name}_accidental_{width_ns}ns_Hz"] = rate * trigger_rate * width
                row[f"{trigger_name}_accidental_{width_ns}ns_error_Hz"] = error * trigger_rate * width
        rows.append(row)

    output = HERE / "nps_electron_gep_hardware_threshold_projection.csv"
    with output.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=rows[0].keys())
        writer.writeheader()
        writer.writerows(rows)

    for row in rows:
        print(f"{row['angular_model']}: NPS={float(row['projected_nps_rate_Hz'])/1e3:.1f} +/- "
              f"{float(row['projected_point_variation_Hz'])/1e3:.1f} kHz; "
              f"S1 accidental 20 ns={float(row['hms_s1_accidental_20ns_Hz'])/1e3:.2f} kHz; "
              f"40 ns={float(row['hms_s1_accidental_40ns_Hz'])/1e3:.2f} kHz")


if __name__ == "__main__":
    main()
