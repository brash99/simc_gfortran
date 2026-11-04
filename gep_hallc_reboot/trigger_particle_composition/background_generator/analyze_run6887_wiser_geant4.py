#!/usr/bin/env python3
"""Audit Wiser pi0 truth topology and run-6887 NPS GEANT4 response."""

from __future__ import annotations

import argparse
import csv
import math
import os
from pathlib import Path

os.environ.setdefault("MPLBACKEND", "Agg")
os.environ.setdefault("MPLCONFIGDIR", "/tmp/run6887_wiser_geant4_matplotlib")

import matplotlib.pyplot as plt
import numpy as np

TRIALS = 5_000_000
THRESHOLDS = np.array([3.387, 3.629, 3.870])
MEASURED_RESIDUAL_KHZ = np.array([48.08, 36.47, 29.24])
HERE = Path(__file__).resolve().parent
PLOT_BASE = HERE.parent / "plots" / "run6887_wiser_geant4_closure"
SUMMARY = HERE / "run6887_wiser_geant4_closure.csv"


def face_intersection(direction: np.ndarray, vertex_z: np.ndarray):
    angle = math.radians(16.57)
    normal = np.array([0.0, math.sin(angle), math.cos(angle)])
    horizontal = np.array([0.0, math.cos(angle), -math.sin(angle)])
    denominator = direction @ normal
    distance = np.divide(357.0 - vertex_z * normal[2], denominator,
                         out=np.full_like(vertex_z, -1.0),
                         where=denominator > 0.0)
    point = direction * distance[:, None]
    point[:, 2] += vertex_z
    relative = point - 357.0 * normal
    face_x = relative @ horizontal
    face_y = relative[:, 0]
    hit = ((distance > 0.0) & (np.abs(face_x) <= 61.5 / 2.0) &
           (np.abs(face_y) <= 73.8 / 2.0))
    return hit, face_x, face_y


def mc_integral(weight: np.ndarray, mask: np.ndarray) -> tuple[float, float]:
    selected = weight[mask]
    total = float(selected.sum())
    variance = TRIALS / (TRIALS - 1.0) * max(
        0.0, float(np.square(selected).sum()) - total * total / TRIALS
    )
    return total, math.sqrt(variance)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--hybrid", type=Path,
                        default=Path("/tmp/nps_run6887_wiser_pi0_5m.hybrid.csv"))
    parser.add_argument("--response", type=Path,
                        default=Path("/tmp/nps_run6887_wiser_pi0_geant4_response.csv"))
    args = parser.parse_args()

    with args.hybrid.open(newline="") as stream:
        truth_rows = list(csv.DictReader(line for line in stream
                                         if not line.startswith("#")))
    with args.response.open(newline="") as stream:
        response_rows = list(csv.DictReader(stream))
    if len(truth_rows) != 88122 or len(response_rows) != len(truth_rows):
        raise ValueError("expected matching 88,122-event truth and response samples")

    weight = np.array([float(row["weight"]) for row in truth_rows])
    vertex_z = np.array([float(row["vertex_z_cm"]) for row in truth_rows])
    incident = []
    coordinates = []
    hits = []
    for photon in (1, 2):
        energy = np.array([float(row[f"gamma{photon}_E_GeV"])
                           for row in truth_rows])
        momentum = np.array([
            [float(row[f"gamma{photon}_{axis}_GeV"])
             for axis in ("px", "py", "pz")]
            for row in truth_rows
        ])
        hit, face_x, face_y = face_intersection(momentum / energy[:, None], vertex_z)
        incident.append(hit * energy)
        coordinates.append((face_x, face_y))
        hits.append(hit)

    summed_incident = incident[0] + incident[1]
    maximum_photon = np.maximum(incident[0], incident[1])
    both_hit = hits[0] & hits[1]
    separation = np.hypot(coordinates[0][0] - coordinates[1][0],
                          coordinates[0][1] - coordinates[1][1])
    raw_3x3 = np.array([float(row["raw_3x3_energy_GeV"])
                        for row in response_rows])
    corrected_3x3 = np.array([float(row["corrected_3x3_energy_GeV"])
                              for row in response_rows])

    output_rows = []
    for threshold, measured in zip(THRESHOLDS, MEASURED_RESIDUAL_KHZ):
        sum_mask = summed_incident >= threshold
        close_mask = sum_mask & both_hit & (separation < 3.0 * 2.05)
        quantities = {
            "summed_face_energy": mc_integral(weight, sum_mask),
            "maximum_single_photon": mc_integral(weight, maximum_photon >= threshold),
            "raw_geant4_3x3": mc_integral(weight, raw_3x3 >= threshold),
            "corrected_geant4_3x3": mc_integral(weight, corrected_3x3 >= threshold),
        }
        both_rate, _ = mc_integral(weight, sum_mask & both_hit)
        close_rate, _ = mc_integral(weight, close_mask)
        row = {
            "threshold_GeV": threshold,
            "measured_residual_Hz": measured * 1000.0,
            "summed_face_energy_Hz": quantities["summed_face_energy"][0],
            "summed_face_energy_mc_Hz": quantities["summed_face_energy"][1],
            "maximum_single_photon_Hz": quantities["maximum_single_photon"][0],
            "maximum_single_photon_mc_Hz": quantities["maximum_single_photon"][1],
            "raw_geant4_3x3_Hz": quantities["raw_geant4_3x3"][0],
            "raw_geant4_3x3_mc_Hz": quantities["raw_geant4_3x3"][1],
            "corrected_geant4_3x3_Hz": quantities["corrected_geant4_3x3"][0],
            "corrected_geant4_3x3_mc_Hz": quantities["corrected_geant4_3x3"][1],
            "both_hit_fraction_of_summed_truth": both_rate / quantities["summed_face_energy"][0],
            "close_both_hit_fraction_of_summed_truth": close_rate / quantities["summed_face_energy"][0],
        }
        output_rows.append(row)

    with SUMMARY.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=output_rows[0].keys(),
                                lineterminator="\n")
        writer.writeheader()
        writer.writerows(output_rows)

    fig, (rates_ax, response_ax) = plt.subplots(1, 2, figsize=(12.0, 5.0))
    rates_ax.plot(THRESHOLDS, MEASURED_RESIDUAL_KHZ, "o-", color="black",
                  label="measured residual")
    styles = [
        ("summed_face_energy_Hz", "s--", "sum of both photons on full face"),
        ("maximum_single_photon_Hz", "^--", "maximum individual photon"),
        ("raw_geant4_3x3_Hz", "D-", "GEANT4 raw hottest 3x3"),
        ("corrected_geant4_3x3_Hz", "v-.", "GEANT4 corrected hottest 3x3"),
    ]
    for key, style, label in styles:
        rates_ax.plot(THRESHOLDS,
                      np.array([row[key] for row in output_rows]) / 1000.0,
                      style, label=label)
    rates_ax.set(xlabel="threshold (GeV)", ylabel="rate above threshold (kHz)",
                 title="Run 6887 Wiser pi0: truth definitions and response")
    rates_ax.set_yscale("log")
    rates_ax.grid(alpha=0.25, which="both")
    rates_ax.legend(frameon=False, fontsize=8.5)

    valid = maximum_photon >= THRESHOLDS[0]
    ratio = raw_3x3[valid] / maximum_photon[valid]
    low_response = ratio < 0.1
    low_response_weight_fraction = (weight[valid][low_response].sum() /
                                    weight[valid].sum())
    response_ax.hist(ratio, bins=np.linspace(0.0, 1.1, 67),
                     weights=weight[valid], histtype="stepfilled", alpha=0.65,
                     color="tab:blue")
    median = np.median(ratio)
    q10, q90 = np.quantile(ratio, [0.1, 0.9])
    response_ax.axvline(median, color="black", ls="--", lw=1.2,
                        label=f"median = {median:.3f}")
    response_ax.set(xlabel="raw hottest-3x3 energy / maximum incident-photon energy",
                    ylabel="weighted rate per bin (Hz)",
                    title="Local 3x3 response for photons above 3.387 GeV")
    response_ax.grid(alpha=0.25)
    response_ax.legend(frameon=False)
    response_ax.text(0.04, 0.95, f"unweighted 10-90% range: {q10:.3f}-{q90:.3f}",
                     transform=response_ax.transAxes, va="top", fontsize=9)
    response_ax.text(
        0.04, 0.87,
        f"E3x3/Egamma < 0.1: {low_response.mean()*100:.1f}% of events, "
        f"{low_response_weight_fraction*100:.1f}% of weight",
        transform=response_ax.transAxes, va="top", fontsize=9,
    )

    fig.tight_layout()
    PLOT_BASE.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(PLOT_BASE.with_suffix(".png"), dpi=180)
    print(SUMMARY)
    print(PLOT_BASE.with_suffix(".png"))
    for row in output_rows:
        print(
            f"{row['threshold_GeV']:.3f} GeV: sum={row['summed_face_energy_Hz']/1000:.3f}, "
            f"max={row['maximum_single_photon_Hz']/1000:.3f}, "
            f"raw3x3={row['raw_geant4_3x3_Hz']/1000:.3f}, "
            f"corrected3x3={row['corrected_geant4_3x3_Hz']/1000:.3f} kHz; "
            f"both-hit fraction={row['both_hit_fraction_of_summed_truth']:.3f}"
        )


if __name__ == "__main__":
    main()
