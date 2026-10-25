#!/usr/bin/env python3
"""Add radiated elastic ep singles to the run-1918 NPS closure test."""

from __future__ import annotations

import argparse
import csv
import os
import re
from pathlib import Path

os.environ.setdefault("MPLBACKEND", "Agg")
os.environ.setdefault("MPLCONFIGDIR", "/tmp/gep_nps_matplotlib")
os.environ.setdefault("XDG_CACHE_HOME", "/tmp/gep_nps_cache")

import matplotlib.pyplot as plt
import numpy as np


HERE = Path(__file__).resolve().parent
RUN1918_CURRENT_MA = 0.028787


def read_simc_yield(histogram: Path) -> float:
    text = histogram.read_text()
    match = re.search(r"wtcontr=\s*([0-9.Ee+-]+)", text)
    if not match:
        raise ValueError(f"Could not find SIMC integrated weight in {histogram}")
    return float(match.group(1))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("events", type=Path)
    parser.add_argument("histogram", type=Path)
    parser.add_argument(
        "--inclusive",
        type=Path,
        default=HERE / "nps_run1918_inclusive_closure.csv",
    )
    parser.add_argument(
        "--output-prefix",
        type=Path,
        default=HERE / "nps_run1918_inclusive_elastic_closure",
    )
    args = parser.parse_args()

    weight: list[float] = []
    electron_energy: list[float] = []
    electron_plus_fsr_energy: list[float] = []
    with args.events.open(newline="") as stream:
        reader = csv.DictReader(line for line in stream if not line.startswith("#"))
        for row in reader:
            w = float(row["weight"])
            energy = float(row["primary_E_GeV"])
            # In SIMC's peaking approximation rad2 is emitted along the final
            # electron.  This second proxy therefore includes its energy in
            # the same NPS trigger sum; rad1 follows the incident beam.
            fsr = float(row["rad2_GeV"]) if int(row["rad_tail"]) == 2 else 0.0
            weight.append(w)
            electron_energy.append(energy)
            electron_plus_fsr_energy.append(energy + fsr)

    weight_array = np.asarray(weight)
    electron_energy_array = np.asarray(electron_energy)
    combined_energy_array = np.asarray(electron_plus_fsr_energy)
    yield_per_mc = read_simc_yield(args.histogram)
    raw_sum = float(weight_array.sum())
    if raw_sum <= 0.0:
        raise ValueError("SIMC hybrid file has no positive integrated weight")
    rate_scale = yield_per_mc * RUN1918_CURRENT_MA / raw_sum

    inclusive_threshold: list[float] = []
    inclusive_rate: list[float] = []
    with args.inclusive.open(newline="") as stream:
        for row in csv.DictReader(stream):
            inclusive_threshold.append(float(row["physical_energy_threshold_GeV"]))
            inclusive_rate.append(float(row["born_inclusive_electron_rate_Hz"]))
    threshold = np.asarray(inclusive_threshold)
    inclusive = np.asarray(inclusive_rate)

    elastic_e = np.asarray(
        [weight_array[electron_energy_array >= t].sum() * rate_scale for t in threshold]
    )
    elastic_e_fsr = np.asarray(
        [weight_array[combined_energy_array >= t].sum() * rate_scale for t in threshold]
    )

    measured_threshold: list[float] = []
    measured_rate: list[float] = []
    with (HERE / "nps_primitive_threshold_result.csv").open(newline="") as stream:
        for row in csv.DictReader(stream):
            t = float(row["run1918_offline_equivalent_threshold_GeV"])
            rate = float(row["run1918_raw_primitive_rate_Hz"])
            if t >= threshold.min() and np.isfinite(rate):
                measured_threshold.append(t)
                measured_rate.append(rate)

    output_csv = args.output_prefix.with_suffix(".csv")
    with output_csv.open("w", newline="") as stream:
        writer = csv.writer(stream)
        writer.writerow(
            [
                "physical_energy_threshold_GeV",
                "born_inclusive_electron_rate_Hz",
                "radiated_elastic_electron_only_rate_Hz",
                "radiated_elastic_electron_plus_fsr_rate_Hz",
                "inclusive_plus_elastic_fsr_rate_Hz",
            ]
        )
        for values in zip(threshold, inclusive, elastic_e, elastic_e_fsr, inclusive + elastic_e_fsr):
            writer.writerow([f"{values[0]:.3f}", *(f"{value:.6f}" for value in values[1:])])

    figure, ax = plt.subplots(figsize=(8.8, 5.9), constrained_layout=True)
    ax.semilogy(
        measured_threshold,
        np.asarray(measured_rate) / 1e3,
        "s-",
        label="Run-1918 measured EDTM primitive rate",
    )
    ax.semilogy(threshold, inclusive / 1e3, "-", lw=2, label="Born inclusive electrons")
    ax.semilogy(
        threshold,
        elastic_e_fsr / 1e3,
        "-.",
        lw=2,
        label="Radiated elastic ep (electron + collinear FSR photon)",
    )
    ax.semilogy(
        threshold,
        (inclusive + elastic_e_fsr) / 1e3,
        "--",
        lw=2.4,
        label="Inclusive + radiated elastic",
    )
    ax.set(xlabel="Physical energy threshold (GeV)", ylabel="Full-face NPS rate above threshold (kHz)")
    ax.set_xlim(0.3, 5.5)
    ax.grid(True, which="both", alpha=0.25)
    ax.legend(fontsize=8.5)
    ax.set_title("Run-1918 electron-singles closure: elastic component added")
    figure.savefig(args.output_prefix.with_suffix(".png"), dpi=180)
    figure.savefig(args.output_prefix.with_suffix(".pdf"))

    print(f"SIMC accepted hybrid events: {len(weight_array)}")
    print(f"SIMC integrated radiated-elastic yield: {yield_per_mc:.3f} events/mC")
    print(f"Run-1918 all-energy radiated-elastic rate: {yield_per_mc * RUN1918_CURRENT_MA / 1e3:.3f} kHz")
    for t in (1.4, 2.0, 2.7, 3.0, 3.4, 4.0):
        inc = float(np.interp(t, threshold, inclusive))
        e = float(np.interp(t, threshold, elastic_e))
        ef = float(np.interp(t, threshold, elastic_e_fsr))
        print(
            f"{t:.1f} GeV: inclusive={inc / 1e3:.3f}, elastic(e)={e / 1e3:.3f}, "
            f"elastic(e+FSR)={ef / 1e3:.3f}, total={((inc + ef) / 1e3):.3f} kHz"
        )
    print(f"Wrote {output_csv}")


if __name__ == "__main__":
    main()
