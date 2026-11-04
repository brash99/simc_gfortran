#!/usr/bin/env python3
"""Compare the measured run-6887 NPS threshold curve with GEANT4-folded electrons."""

from __future__ import annotations

import csv
import os
from pathlib import Path

os.environ.setdefault("MPLBACKEND", "Agg")
os.environ.setdefault("MPLCONFIGDIR", "/tmp/nps_run6887_geant4_matplotlib")

import matplotlib.pyplot as plt
import numpy as np


HERE = Path(__file__).resolve().parent
MEASURED = HERE / "nps_run6887_edtm_threshold_curve.csv"
FULL_EVENTS = Path("/tmp/nps_run6887_inclusive_5m.csv")
HYBRID = Path("/tmp/nps_run6887_inclusive_geant4_pilot.hybrid.csv")
RESPONSE = Path("/tmp/nps_run6887_inclusive_geant4_response.csv")
SCALER_ANCHORS = HERE / "nps_ptrig1_scaler_anchors.csv"
OUTPUT = HERE / "nps_run6887_total_vs_geant4_electrons"
MINIMUM_TRANSPORT_ENERGY_GEV = 2.3


def read_measured() -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    with MEASURED.open(newline="") as stream:
        rows = list(csv.DictReader(stream))
    return (
        np.asarray([float(row["threshold_GeV"]) for row in rows]),
        np.asarray([float(row["poisson_rate_Hz"]) for row in rows]),
        np.asarray([float(row["stat_error_Hz"]) for row in rows]),
    )


def read_scaler_anchors() -> tuple[np.ndarray, ...]:
    with SCALER_ANCHORS.open(newline="") as stream:
        rows = list(csv.DictReader(stream))
    return (
        np.asarray([float(row["hardware_threshold_GeV"]) for row in rows]),
        np.asarray([float(row["ptrig1_scaler_rate_Hz"]) for row in rows]),
        np.asarray([float(row["ptrig1_scaler_sem_Hz"]) for row in rows]),
        np.asarray([int(row["run"]) for row in rows]),
    )


def full_weight_above_transport_cut() -> float:
    total = 0.0
    with FULL_EVENTS.open(newline="") as stream:
        for row in csv.DictReader(stream):
            if float(row["electron_energy_GeV"]) >= MINIMUM_TRANSPORT_ENERGY_GEV:
                total += float(row["weight_Hz"])
    return total


def read_simulated_events() -> tuple[np.ndarray, ...]:
    primary: list[float] = []
    weight: list[float] = []
    with HYBRID.open(newline="") as stream:
        rows = csv.DictReader(line for line in stream if not line.startswith("#"))
        for index, row in enumerate(rows):
            if index == 20000:
                break
            primary.append(float(row["primary_E_GeV"]))
            weight.append(float(row["weight"]))

    raw_3x3: list[float] = []
    corrected_3x3: list[float] = []
    with RESPONSE.open(newline="") as stream:
        for row in csv.DictReader(stream):
            raw_3x3.append(float(row["raw_3x3_energy_GeV"]))
            corrected_3x3.append(float(row["corrected_3x3_energy_GeV"]))

    arrays = tuple(
        np.asarray(values) for values in (primary, weight, raw_3x3, corrected_3x3)
    )
    if len({len(values) for values in arrays}) != 1:
        raise ValueError("hybrid input and GEANT4 response event counts do not match")
    return arrays


def cumulative_rate(
    observable: np.ndarray, weight: np.ndarray, thresholds: np.ndarray, scale: float
) -> tuple[np.ndarray, np.ndarray]:
    rates = []
    errors = []
    for threshold in thresholds:
        selected = weight[observable >= threshold]
        rates.append(scale * selected.sum())
        errors.append(scale * np.sqrt(np.square(selected).sum()))
    return np.asarray(rates), np.asarray(errors)


def main() -> None:
    all_thresholds, all_measured, all_measured_error = read_measured()
    scaler_thresholds, scaler_rates, scaler_errors, scaler_runs = read_scaler_anchors()
    keep = all_thresholds >= 2.5
    thresholds = all_thresholds[keep]
    measured = all_measured[keep]
    measured_error = all_measured_error[keep]

    primary, pilot_weight, raw_3x3, corrected_3x3 = read_simulated_events()
    full_weight = full_weight_above_transport_cut()
    scale = full_weight / pilot_weight.sum()
    truth, truth_error = cumulative_rate(primary, pilot_weight, thresholds, scale)
    raw, raw_error = cumulative_rate(raw_3x3, pilot_weight, thresholds, scale)
    corrected, corrected_error = cumulative_rate(
        corrected_3x3, pilot_weight, thresholds, scale
    )

    with OUTPUT.with_suffix(".csv").open("w", newline="") as stream:
        writer = csv.writer(stream)
        writer.writerow(
            [
                "threshold_GeV",
                "measured_total_rate_Hz",
                "measured_stat_error_Hz",
                "generator_truth_rate_Hz",
                "generator_truth_mc_error_Hz",
                "geant4_raw_3x3_rate_Hz",
                "geant4_raw_3x3_mc_error_Hz",
                "geant4_corrected_3x3_rate_Hz",
                "geant4_corrected_3x3_mc_error_Hz",
                "corrected_3x3_over_measured",
            ]
        )
        for values in zip(
            thresholds,
            measured,
            measured_error,
            truth,
            truth_error,
            raw,
            raw_error,
            corrected,
            corrected_error,
            corrected / measured,
        ):
            writer.writerow([f"{value:.10g}" for value in values])

    fig, (top, bottom) = plt.subplots(
        2,
        1,
        figsize=(9.0, 8.2),
        sharex=True,
        gridspec_kw={"height_ratios": [3.0, 1.25]},
    )
    top.errorbar(
        all_thresholds,
        all_measured / 1000.0,
        yerr=all_measured_error / 1000.0,
        fmt="o-",
        capsize=2,
        label="Measured total NPS event-sample rate",
    )
    top.errorbar(
        scaler_thresholds,
        scaler_rates / 1000.0,
        yerr=scaler_errors / 1000.0,
        fmt="X",
        ms=8,
        capsize=3,
        color="black",
        label="Direct pTRIG1 hardware scaler",
        zorder=6,
    )
    for threshold, rate, run in zip(scaler_thresholds, scaler_rates, scaler_runs):
        top.annotate(
            f"run {run}",
            (threshold, rate / 1000.0),
            xytext=(7, -13 if run == 6887 else 7),
            textcoords="offset points",
            fontsize=8,
            color="black",
        )
    top.errorbar(
        thresholds,
        truth / 1000.0,
        yerr=truth_error / 1000.0,
        fmt="s--",
        ms=4,
        capsize=2,
        label="Inclusive electrons: generator truth energy",
    )
    top.errorbar(
        thresholds,
        corrected / 1000.0,
        yerr=corrected_error / 1000.0,
        fmt="D-.",
        ms=4,
        capsize=2,
        color="tab:red",
        label="Inclusive electrons: GEANT4 3x3 with inherited energy correction",
    )
    top.set_yscale("log")
    top.set_ylabel("Rate above threshold (kHz)")
    top.set_title("NPS run 6887: measured threshold curve and GEANT4-folded electrons")
    top.grid(True, which="both", alpha=0.25)
    top.legend(frameon=False, fontsize=8.5)
    top.axvspan(0.5, 2.5, color="0.8", alpha=0.18)
    top.text(
        1.5,
        0.06,
        "Measured data shown; GEANT4 pilot not evaluated below 2.5 GeV",
        transform=top.get_xaxis_transform(),
        ha="center",
        va="bottom",
        fontsize=8.5,
        color="0.35",
    )

    bottom.errorbar(
        thresholds,
        truth / measured,
        yerr=truth_error / measured,
        fmt="s--",
        ms=4,
        capsize=2,
        label="Truth / data",
    )
    bottom.errorbar(
        thresholds,
        raw / measured,
        yerr=raw_error / measured,
        fmt="^:",
        ms=4,
        capsize=2,
        label="Raw 3x3 / data",
    )
    bottom.errorbar(
        thresholds,
        corrected / measured,
        yerr=corrected_error / measured,
        fmt="D-.",
        ms=4,
        capsize=2,
        color="tab:red",
        label="Corrected 3x3 / data",
    )
    bottom.axhline(1.0, color="0.4", lw=1.0, ls="--")
    bottom.axvspan(0.5, 2.5, color="0.8", alpha=0.18)
    bottom.set_xlim(0.45, 5.05)
    bottom.set_ylim(0.0, 2.0)
    bottom.set_xlabel("Numerical threshold (GeV)")
    bottom.set_ylabel("Electron calculation / data")
    bottom.grid(True, alpha=0.25)
    bottom.legend(frameon=False, fontsize=8)

    fig.text(
        0.5,
        0.012,
        "20,000-event response pilot; generator normalization is absolute, not fitted to data",
        ha="center",
        fontsize=8.5,
    )
    fig.tight_layout(rect=(0, 0.035, 1, 1))
    fig.savefig(OUTPUT.with_suffix(".png"), dpi=180)
    fig.savefig(OUTPUT.with_suffix(".pdf"))

    print(f"events={len(primary)} full_rate_above_2.3={full_weight/1000:.3f} kHz scale={scale:.6g}")
    for reference in (3.387, 3.629, 3.870):
        values = [
            float(np.interp(reference, thresholds, curve))
            for curve in (measured, truth, raw, corrected)
        ]
        print(
            f"{reference:.3f} GeV: data={values[0]/1000:.3f} kHz "
            f"truth={values[1]/1000:.3f} kHz raw3x3={values[2]/1000:.3f} kHz "
            f"corrected3x3={values[3]/1000:.3f} kHz "
            f"ratios={values[1]/values[0]:.3f}/{values[2]/values[0]:.3f}/"
            f"{values[3]/values[0]:.3f}"
        )


if __name__ == "__main__":
    main()
