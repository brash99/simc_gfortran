#!/usr/bin/env python3
"""Fold the inclusive generator through the run-17193 SHMS transport map."""

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

from analyze_pi0_elastic_cuts import read_simc_binary


HERE = Path(__file__).resolve().parent
CENTRAL_MOMENTUM_GEV = 4.935
CURRENT_MA = 0.055868051
N_GENERATED_PHASE_SPACE = 5_000_000


def cumulative(energy: np.ndarray, weight: np.ndarray, threshold: np.ndarray) -> np.ndarray:
    return np.asarray([weight[energy >= value].sum() for value in threshold])


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("inclusive_events", type=Path)
    parser.add_argument("phase_space", type=Path)
    parser.add_argument("elastic_events", type=Path)
    parser.add_argument("elastic_histogram", type=Path)
    parser.add_argument(
        "--output-prefix",
        type=Path,
        default=HERE / "plots" / "xem2_run17193_shms_transport_closure",
    )
    args = parser.parse_args()

    phase = read_simc_binary(args.phase_space)
    edges = (
        np.linspace(-15.5, 25.5, 13),
        np.linspace(-0.095, 0.095, 13),
        np.linspace(-0.055, 0.055, 11),
    )
    accepted_hist, _ = np.histogramdd(
        np.column_stack((phase["ssdeltai"], phase["ssyptari"], phase["ssxptari"])),
        bins=edges,
    )
    expected_per_bin = N_GENERATED_PHASE_SPACE / accepted_hist.size
    efficiency = np.clip(accepted_hist / expected_per_bin, 0.0, 1.0)

    energy: list[float] = []
    weight: list[float] = []
    coordinates: list[tuple[float, float, float]] = []
    theta0 = np.radians(26.0)
    with args.inclusive_events.open(newline="") as stream:
        for row in csv.DictReader(stream):
            electron_energy = float(row["electron_energy_GeV"])
            px = float(row["px_GeV"]) / electron_energy
            py = float(row["py_GeV"]) / electron_energy
            pz = float(row["pz_GeV"]) / electron_energy
            central = py * np.sin(theta0) + pz * np.cos(theta0)
            # SIMC target coordinates follow physics_angles/event.f:
            # xptar is Hall vertical and yptar is the in-plane slope.
            xptar = px / central
            yptar = (py * np.cos(theta0) - pz * np.sin(theta0)) / central
            energy.append(electron_energy)
            weight.append(float(row["weight_Hz"]))
            coordinates.append(
                (100.0 * (electron_energy / CENTRAL_MOMENTUM_GEV - 1.0), yptar, xptar)
            )
    energy_array = np.asarray(energy)
    weight_array = np.asarray(weight)
    coordinate_array = np.asarray(coordinates)
    indices = [
        np.searchsorted(edge, coordinate_array[:, axis], side="right") - 1
        for axis, edge in enumerate(edges)
    ]
    valid = np.ones(len(weight_array), dtype=bool)
    for axis, index in enumerate(indices):
        valid &= (index >= 0) & (index < len(edges[axis]) - 1)
    transport_efficiency = np.zeros(len(weight_array))
    transport_efficiency[valid] = efficiency[
        indices[0][valid], indices[1][valid], indices[2][valid]
    ]
    transported_weight = weight_array * transport_efficiency

    elastic = read_simc_binary(args.elastic_events)
    elastic_energy = CENTRAL_MOMENTUM_GEV * (1.0 + elastic["ssdelta"] / 100.0)
    match = re.search(r"wtcontr=\s*([0-9.Ee+-]+)", args.elastic_histogram.read_text())
    if not match:
        raise ValueError("missing elastic wtcontr")
    elastic_rate = float(match.group(1)) * CURRENT_MA
    elastic_weight = elastic["Weight"] * (elastic_rate / elastic["Weight"].sum())

    measured_path = HERE / "plots" / "xem2_run17193_shms_singles.csv"
    measured_rows = list(csv.DictReader(measured_path.open()))
    thresholds = np.asarray([float(row["offline_total_energy_threshold_GeV"]) for row in measured_rows])
    measured = np.asarray([float(row["pTRIG1_sample_rate_Hz"]) for row in measured_rows])
    inclusive = cumulative(energy_array, transported_weight, thresholds)
    elastic_rates = cumulative(elastic_energy, elastic_weight, thresholds)
    combined = inclusive + elastic_rates

    output_csv = args.output_prefix.with_suffix(".csv")
    with output_csv.open("w", newline="") as stream:
        writer = csv.writer(stream)
        writer.writerow(
            ["threshold_GeV", "measured_rate_Hz", "inclusive_rate_Hz", "elastic_rate_Hz", "combined_rate_Hz"]
        )
        for values in zip(thresholds, measured, inclusive, elastic_rates, combined):
            writer.writerow(values)

    fig, ax = plt.subplots(figsize=(8.8, 6.0), constrained_layout=True)
    ax.semilogy(thresholds, measured, lw=2.5, label="Run 17193 pTRIG1 data")
    ax.semilogy(thresholds, inclusive, "--", lw=2, label="Inclusive + SHMS transport")
    ax.semilogy(thresholds, elastic_rates, "-.", lw=2, label="Radiated elastic SIMC")
    ax.semilogy(thresholds, combined, lw=2.5, label="Inclusive + elastic")
    for fraction in (0.70, 0.75, 0.80):
        ax.axvline(fraction * CENTRAL_MOMENTUM_GEV, color="0.65", ls=":")
    ax.set_xlim(0.0, 5.2)
    ax.set_ylim(1.0, 5000.0)
    ax.set_xlabel("Offline SHMS calorimeter-energy threshold (GeV)")
    ax.set_ylabel("Rate above threshold (Hz)")
    ax.set_title("XEM2 run 17193: SHMS singles closure")
    ax.grid(alpha=0.25, which="both")
    ax.legend(frameon=False)
    fig.savefig(args.output_prefix.with_suffix(".png"), dpi=180)
    fig.savefig(args.output_prefix.with_suffix(".pdf"))

    print(f"phase_space_acceptance={len(phase['Weight'])/N_GENERATED_PHASE_SPACE:.8f}")
    print(f"inclusive_hard_envelope_Hz={weight_array.sum():.6f}")
    print(f"inclusive_transport_Hz={transported_weight.sum():.6f}")
    print(f"elastic_transport_Hz={elastic_weight.sum():.6f}")
    for fraction in (0.70, 0.75, 0.80):
        threshold = fraction * CENTRAL_MOMENTUM_GEV
        values = [np.interp(threshold, thresholds, series) for series in (measured, inclusive, elastic_rates, combined)]
        print(f"fraction={fraction:.2f} threshold={threshold:.4f} data={values[0]:.4f} inclusive={values[1]:.4f} elastic={values[2]:.4f} combined={values[3]:.4f} ratio={values[3]/values[0]:.4f}")


if __name__ == "__main__":
    main()
