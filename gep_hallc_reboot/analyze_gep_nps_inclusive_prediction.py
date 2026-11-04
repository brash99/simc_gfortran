#!/usr/bin/env python3
"""Summarize the absolute GEp Q2=11 inclusive-electron generator prediction."""

from __future__ import annotations

import argparse
import csv
import os
import re
from pathlib import Path

os.environ.setdefault("MPLBACKEND", "Agg")
os.environ.setdefault("MPLCONFIGDIR", "/tmp/gep_nps_matplotlib")

import matplotlib.pyplot as plt
import numpy as np


HERE = Path(__file__).resolve().parent
ELASTIC_ENERGY_GEV = 4.838
FRACTIONS = (0.70, 0.75, 0.80)
BEAM_CURRENT_MA = 0.075
# Run-17193 scaler inclusion-exclusion and paddle-level EDTM measurements give
# a 40--60 kHz working range after naive GEp luminosity scaling.  The old
# 806-kHz value is the broad S1X OR S1Y rate, not the practical trigger rate.
HMS_S1XY_RATE_RANGE_HZ = (40_000.0, 60_000.0)
VALIDATION_UPPER_FACTOR = 1.35


def cumulative_rates(
    energy: np.ndarray, weight: np.ndarray, thresholds: np.ndarray
) -> tuple[np.ndarray, np.ndarray]:
    rates = np.asarray([weight[energy >= value].sum() for value in thresholds])
    errors = np.asarray(
        [np.sqrt(np.square(weight[energy >= value]).sum()) for value in thresholds]
    )
    return rates, errors


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("events", type=Path)
    parser.add_argument("--elastic-events", type=Path, required=True)
    parser.add_argument("--elastic-histogram", type=Path, required=True)
    parser.add_argument(
        "--output-prefix",
        type=Path,
        default=HERE / "gep_q11_nps_inclusive_prediction",
    )
    args = parser.parse_args()

    energy: list[float] = []
    weight: list[float] = []
    face_x: list[float] = []
    face_y: list[float] = []
    with args.events.open(newline="") as stream:
        for row in csv.DictReader(stream):
            energy.append(float(row["electron_energy_GeV"]))
            weight.append(float(row["weight_Hz"]))
            face_x.append(float(row["face_x_cm"]))
            face_y.append(float(row["face_y_cm"]))
    energy_array = np.asarray(energy)
    weight_array = np.asarray(weight)
    face_x_array = np.asarray(face_x)
    face_y_array = np.asarray(face_y)

    thresholds = np.arange(0.30, 5.501, 0.05)
    rates, errors = cumulative_rates(energy_array, weight_array, thresholds)

    elastic_energy: list[float] = []
    elastic_raw_weight: list[float] = []
    with args.elastic_events.open(newline="") as stream:
        reader = csv.DictReader(line for line in stream if not line.startswith("#"))
        for row in reader:
            fsr_energy = (
                float(row["rad2_GeV"]) if int(row["rad_tail"]) == 2 else 0.0
            )
            elastic_energy.append(float(row["primary_E_GeV"]) + fsr_energy)
            elastic_raw_weight.append(float(row["weight"]))
    elastic_energy_array = np.asarray(elastic_energy)
    elastic_raw_weight_array = np.asarray(elastic_raw_weight)
    match = re.search(
        r"wtcontr=\s*([0-9.Ee+-]+)", args.elastic_histogram.read_text()
    )
    if not match:
        raise ValueError("elastic histogram has no wtcontr normalization")
    elastic_yield_per_mC = float(match.group(1))
    elastic_weight_array = elastic_raw_weight_array * (
        elastic_yield_per_mC * BEAM_CURRENT_MA / elastic_raw_weight_array.sum()
    )
    elastic_rates, elastic_errors = cumulative_rates(
        elastic_energy_array, elastic_weight_array, thresholds
    )
    combined_rates = rates + elastic_rates
    combined_errors = np.hypot(errors, elastic_errors)
    with args.output_prefix.with_suffix(".csv").open("w", newline="") as stream:
        writer = csv.writer(stream)
        writer.writerow(
            [
                "truth_energy_threshold_GeV",
                "inclusive_inelastic_rate_Hz",
                "inclusive_mc_error_Hz",
                "radiated_elastic_electron_plus_fsr_rate_Hz",
                "elastic_mc_error_Hz",
                "combined_rate_Hz",
                "combined_mc_error_Hz",
            ]
        )
        for values in zip(
            thresholds,
            rates,
            errors,
            elastic_rates,
            elastic_errors,
            combined_rates,
            combined_errors,
        ):
            writer.writerow([f"{values[0]:.3f}", *(f"{value:.8f}" for value in values[1:])])

    figure, axes = plt.subplots(1, 2, figsize=(12.4, 5.1), constrained_layout=True)
    axes[0].semilogy(thresholds, rates / 1000.0, lw=2, label="Inclusive inelastic")
    axes[0].semilogy(
        thresholds, elastic_rates / 1000.0, "-.", lw=1.8, label="Radiated elastic"
    )
    axes[0].semilogy(
        thresholds, combined_rates / 1000.0, "--", lw=2, label="Combined"
    )
    for fraction in FRACTIONS:
        threshold = fraction * ELASTIC_ENERGY_GEV
        rate = float(np.interp(threshold, thresholds, combined_rates))
        axes[0].plot(threshold, rate / 1000.0, "o")
        axes[0].annotate(
            f"{fraction:.0%}: {rate/1000:.1f} kHz",
            (threshold, rate / 1000.0),
            xytext=(7, 4),
            textcoords="offset points",
            fontsize=9,
        )
    axes[0].set_xlabel("Incident-electron truth threshold (GeV)")
    axes[0].set_ylabel("Full-face inclusive-inelastic rate (kHz)")
    axes[0].set_title("Absolute cumulative rate")
    axes[0].grid(alpha=0.25, which="both")
    axes[0].legend(frameon=False)

    reference_threshold = 0.75 * ELASTIC_ENERGY_GEV
    selected = energy_array >= reference_threshold
    histogram = axes[1].hist2d(
        face_x_array[selected],
        face_y_array[selected],
        bins=(30, 36),
        range=((-30.75, 30.75), (-36.9, 36.9)),
        weights=weight_array[selected] / 1000.0,
        cmap="viridis",
    )
    figure.colorbar(histogram[3], ax=axes[1], label="Rate per face bin (kHz)")
    axes[1].set_xlabel("NPS horizontal face coordinate (cm)")
    axes[1].set_ylabel("NPS vertical face coordinate (cm)")
    axes[1].set_title("Rate map above 75% truth threshold")

    figure.suptitle(
        "GEp Q2 = 11 inclusive-electron prediction: 75 uA, 30 cm LH2, NPS at 3 m\n"
        "Generator-level F1F2IN21 result; no NPS shower/trigger response applied"
    )
    figure.savefig(args.output_prefix.with_suffix(".png"), dpi=180)
    figure.savefig(args.output_prefix.with_suffix(".pdf"))

    print(f"positive_weight_events={len(weight_array)}")
    print(f"total_rate_kHz={weight_array.sum()/1000.0:.6f}")
    budget_path = args.output_prefix.with_name("gep_q11_nps_trigger_budget").with_suffix(".csv")
    with budget_path.open("w", newline="") as stream:
        writer = csv.writer(stream)
        writer.writerow(
            [
                "threshold_fraction",
                "threshold_GeV",
                "inclusive_rate_Hz",
                "elastic_rate_Hz",
                "combined_rate_Hz",
                "conservative_combined_rate_Hz",
                "hms_s1xy_rate_low_Hz",
                "hms_s1xy_rate_high_Hz",
                "accidental_20ns_low_Hz",
                "accidental_20ns_high_Hz",
                "accidental_40ns_low_Hz",
                "accidental_40ns_high_Hz",
                "conservative_accidental_20ns_low_Hz",
                "conservative_accidental_20ns_high_Hz",
                "conservative_accidental_40ns_low_Hz",
                "conservative_accidental_40ns_high_Hz",
            ]
        )
        for fraction in FRACTIONS:
            threshold = fraction * ELASTIC_ENERGY_GEV
            inclusive_rate = float(np.interp(threshold, thresholds, rates))
            elastic_rate = float(np.interp(threshold, thresholds, elastic_rates))
            combined_rate = inclusive_rate + elastic_rate
            conservative_rate = VALIDATION_UPPER_FACTOR * inclusive_rate + elastic_rate
            writer.writerow(
                [
                    fraction,
                    threshold,
                    inclusive_rate,
                    elastic_rate,
                    combined_rate,
                    conservative_rate,
                    *HMS_S1XY_RATE_RANGE_HZ,
                    combined_rate * HMS_S1XY_RATE_RANGE_HZ[0] * 20e-9,
                    combined_rate * HMS_S1XY_RATE_RANGE_HZ[1] * 20e-9,
                    combined_rate * HMS_S1XY_RATE_RANGE_HZ[0] * 40e-9,
                    combined_rate * HMS_S1XY_RATE_RANGE_HZ[1] * 40e-9,
                    conservative_rate * HMS_S1XY_RATE_RANGE_HZ[0] * 20e-9,
                    conservative_rate * HMS_S1XY_RATE_RANGE_HZ[1] * 20e-9,
                    conservative_rate * HMS_S1XY_RATE_RANGE_HZ[0] * 40e-9,
                    conservative_rate * HMS_S1XY_RATE_RANGE_HZ[1] * 40e-9,
                ]
            )
    for fraction in FRACTIONS:
        threshold = fraction * ELASTIC_ENERGY_GEV
        inclusive_rate = float(np.interp(threshold, thresholds, rates))
        elastic_rate = float(np.interp(threshold, thresholds, elastic_rates))
        rate = inclusive_rate + elastic_rate
        error = float(np.interp(threshold, thresholds, combined_errors))
        print(
            f"fraction={fraction:.2f} threshold_GeV={threshold:.4f} "
            f"inclusive_kHz={inclusive_rate/1000.0:.6f} "
            f"elastic_kHz={elastic_rate/1000.0:.6f} "
            f"combined_kHz={rate/1000.0:.6f} mc_error_kHz={error/1000.0:.6f}"
        )
    print(f"wrote_budget={budget_path}")


if __name__ == "__main__":
    main()
