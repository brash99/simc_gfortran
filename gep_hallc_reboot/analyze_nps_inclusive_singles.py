#!/usr/bin/env python3
"""Compare Born inclusive-electron truth rates with the run-1918 projection."""

from __future__ import annotations

import argparse
import csv
import os
from pathlib import Path

os.environ.setdefault("MPLBACKEND", "Agg")
os.environ.setdefault("MPLCONFIGDIR", "/tmp/gep_nps_matplotlib")
os.environ.setdefault("XDG_CACHE_HOME", "/tmp/gep_nps_cache")

import matplotlib.pyplot as plt
import numpy as np


HERE = Path(__file__).resolve().parent


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("events", type=Path)
    parser.add_argument(
        "--output-prefix",
        type=Path,
        default=HERE / "nps_inclusive_singles_truth_threshold",
    )
    parser.add_argument("--comparison", choices=("gep", "run1918"), default="gep")
    parser.add_argument(
        "--sweeper-bdl-tm",
        type=float,
        default=0.0,
        help="Thin-kick sweeper field integral in T m (run 1918 nominally 0.3)",
    )
    parser.add_argument(
        "--sweeper-center-m",
        type=float,
        default=1.6,
        help="Distance from target to effective sweeper kick point in metres",
    )
    parser.add_argument(
        "--nps-distance-m",
        type=float,
        default=3.0,
        help="Distance from target to the NPS face in metres",
    )
    args = parser.parse_args()

    energies: list[float] = []
    weights: list[float] = []
    face_y: list[float] = []
    with args.events.open(newline="") as stream:
        for row in csv.DictReader(stream):
            energies.append(float(row["electron_energy_GeV"]))
            weights.append(float(row["weight_Hz"]))
            face_y.append(float(row["face_y_cm"]))
    energy = np.asarray(energies)
    weight = np.asarray(weights)
    face_y_cm = np.asarray(face_y)

    thresholds = np.arange(0.3, 5.501, 0.1)
    truth_rates = np.asarray([weight[energy >= threshold].sum() for threshold in thresholds])

    swept_rates: dict[str, np.ndarray] = {}
    if args.sweeper_bdl_tm > 0.0:
        lever_arm_m = args.nps_distance_m - args.sweeper_center_m
        if lever_arm_m <= 0.0:
            parser.error("NPS distance must exceed the sweeper-center distance")
        # Thin-lens charged-particle transport.  The NPS sweeper is a
        # vertical-bend magnet, so the displacement is along Hall/NPS y; the
        # active height is 73.8 cm.  The documented +468 A run-1918 setting
        # has +Bx and bends electrons toward negative y.  The opposite sign
        # is retained as a symmetry/check curve.
        kick_cm = (
            100.0 * 0.299792458 * args.sweeper_bdl_tm * lever_arm_m / energy
        )
        for sign, label in ((-1.0, "toward_negative_y"), (1.0, "toward_positive_y")):
            accepted = np.abs(face_y_cm + sign * kick_cm) <= 36.9
            swept_rates[label] = np.asarray(
                [weight[(energy >= threshold) & accepted].sum() for threshold in thresholds]
            )

    csv_path = args.output_prefix.with_suffix(".csv")
    with csv_path.open("w", newline="") as stream:
        writer = csv.writer(stream)
        header = ["physical_energy_threshold_GeV", "born_inclusive_electron_rate_Hz"]
        header.extend(f"thin_kick_{label}_rate_Hz" for label in swept_rates)
        writer.writerow(header)
        for index, (threshold, rate) in enumerate(zip(thresholds, truth_rates)):
            row = [f"{threshold:.3f}", f"{rate:.6f}"]
            row.extend(f"{rates[index]:.6f}" for rates in swept_rates.values())
            writer.writerow(row)

    data_threshold: list[float] = []
    data_rate: list[float] = []
    with (HERE / "nps_primitive_threshold_result.csv").open(newline="") as stream:
        for row in csv.DictReader(stream):
            threshold = float(row["run1918_offline_equivalent_threshold_GeV"])
            rate_column = (
                "run1918_raw_primitive_rate_Hz"
                if args.comparison == "run1918"
                else "proposal_naively_scaled_rate_Hz"
            )
            rate = float(row[rate_column])
            if threshold >= 0.3 and np.isfinite(rate):
                data_threshold.append(threshold)
                data_rate.append(rate)

    figure, ax = plt.subplots(figsize=(8.4, 5.8), constrained_layout=True)
    ax.semilogy(
        data_threshold,
        np.asarray(data_rate) / 1e3,
        "s-",
        label=(
            "Run-1918 measured EDTM primitive rate"
            if args.comparison == "run1918"
            else "Run-1918 data, conservatively scaled"
        ),
    )
    for label, rates in swept_rates.items():
        direction = "toward -y" if label == "toward_negative_y" else "toward +y"
        ax.semilogy(
            thresholds,
            rates / 1e3,
            "-",
            lw=2,
            label=(
                f"Thin-kick sweeper, {args.sweeper_bdl_tm:.1f} T m, {direction}"
            ),
        )
    ax.semilogy(
        thresholds,
        truth_rates / 1e3,
        "o-",
        ms=3,
        label=(
            "Run-1918 Born inclusive electrons, before sweeper"
            if args.comparison == "run1918"
            else "Born inclusive electrons (SIMC F1F2IN21), truth energy"
        ),
    )
    if args.comparison == "gep":
        for fraction, color in ((0.70, "tab:green"), (0.80, "tab:red")):
            ax.axvline(4.838 * fraction, color=color, ls="--", lw=1.2)
    ax.set_xlabel("Physical energy threshold (GeV)")
    ax.set_ylabel("Full-face NPS rate above threshold (kHz)")
    ax.set_xlim(0.3, 5.5)
    ax.grid(True, which="both", alpha=0.25)
    ax.legend(fontsize=9)
    ax.set_title(
        "Run-1918 generator-level closure test"
        if args.comparison == "run1918"
        else "Pre-GEANT4 inclusive-electron singles comparison"
    )
    figure.savefig(args.output_prefix.with_suffix(".png"), dpi=180)
    figure.savefig(args.output_prefix.with_suffix(".pdf"))

    print(f"Input positive-weight events: {len(weight)}")
    print(f"Total Born inclusive-inelastic rate: {weight.sum() / 1e3:.3f} kHz")
    if args.comparison == "gep":
        for fraction in (0.70, 0.75, 0.80):
            threshold = 4.838 * fraction
            rate = float(np.interp(threshold, thresholds, truth_rates))
            print(f"{fraction:.0%} ({threshold:.4f} GeV): {rate / 1e3:.3f} kHz")
    else:
        for threshold in (1.4, 2.0, 2.7, 3.0, 3.4, 4.0):
            rate = float(np.interp(threshold, thresholds, truth_rates))
            details = [f"unswept={rate / 1e3:.3f} kHz"]
            for label, rates in swept_rates.items():
                swept = float(np.interp(threshold, thresholds, rates))
                details.append(f"{label}={swept / 1e3:.3f} kHz")
            print(f"{threshold:.1f} GeV: " + ", ".join(details))
    print(f"Wrote {csv_path}")


if __name__ == "__main__":
    main()
