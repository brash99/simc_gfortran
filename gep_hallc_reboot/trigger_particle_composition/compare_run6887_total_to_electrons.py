#!/usr/bin/env python3
"""Compare run-6887 total NPS activity with generated inclusive electrons."""

from __future__ import annotations

import csv
import math
import os
from pathlib import Path

os.environ.setdefault("MPLBACKEND", "Agg")
os.environ.setdefault("MPLCONFIGDIR", "/tmp/nps_run6887_matplotlib")
os.environ.setdefault("XDG_CACHE_HOME", "/tmp/nps_run6887_cache")

import matplotlib.pyplot as plt
import numpy as np


HERE = Path(__file__).resolve().parent
MEASURED = HERE / "nps_run6887_edtm_threshold_curve.csv"
EVENTS = Path("/tmp/nps_run6887_inclusive_5m.csv")
OUTPUT = HERE / "nps_run6887_total_vs_inclusive_electrons"


def generated_threshold_curves(
    path: Path, thresholds: np.ndarray
) -> tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    bin_width = 0.1
    number_bins = 100
    sum_weight = np.zeros(number_bins)
    sum_weight2 = np.zeros(number_bins)
    swept_weight = np.zeros(number_bins)
    swept_weight2 = np.zeros(number_bins)
    sweeper_bdl_tm = 0.3
    lever_arm_m = 3.57 - 1.6
    with path.open(newline="") as stream:
        for row in csv.DictReader(stream):
            energy = float(row["electron_energy_GeV"])
            weight = float(row["weight_Hz"])
            face_y_cm = float(row["face_y_cm"])
            index = min(number_bins - 1, max(0, int(math.floor(energy / bin_width))))
            sum_weight[index] += weight
            sum_weight2[index] += weight * weight
            # The +468 A setting has +Bx and bends electrons toward negative
            # Hall/NPS y.  This is a thin-kick face-edge estimate, not a field
            # map or a model of absorption in the sweeper and nearby material.
            kick_cm = 100.0 * 0.299792458 * sweeper_bdl_tm * lever_arm_m / energy
            if abs(face_y_cm - kick_cm) <= 36.9:
                swept_weight[index] += weight
                swept_weight2[index] += weight * weight
    cumulative = np.cumsum(sum_weight[::-1])[::-1]
    cumulative2 = np.cumsum(sum_weight2[::-1])[::-1]
    swept_cumulative = np.cumsum(swept_weight[::-1])[::-1]
    swept_cumulative2 = np.cumsum(swept_weight2[::-1])[::-1]
    indices = np.rint(thresholds / bin_width).astype(int)
    return (
        cumulative[indices],
        np.sqrt(cumulative2[indices]),
        swept_cumulative[indices],
        np.sqrt(swept_cumulative2[indices]),
    )


with MEASURED.open(newline="") as stream:
    measured_rows = list(csv.DictReader(stream))

threshold = np.array([float(row["threshold_GeV"]) for row in measured_rows])
measured = np.array([float(row["poisson_rate_Hz"]) for row in measured_rows])
measured_error = np.array([float(row["stat_error_Hz"]) for row in measured_rows])
electron, electron_mc_error, swept_electron, swept_electron_mc_error = (
    generated_threshold_curves(EVENTS, threshold)
)
electron_fraction = np.divide(electron, measured, out=np.zeros_like(electron), where=measured > 0)
swept_electron_fraction = np.divide(
    swept_electron, measured, out=np.zeros_like(swept_electron), where=measured > 0
)

with OUTPUT.with_suffix(".csv").open("w", newline="") as stream:
    writer = csv.writer(stream)
    writer.writerow(
        [
            "threshold_GeV",
            "measured_total_rate_Hz",
            "measured_stat_error_Hz",
            "inclusive_electron_rate_Hz",
            "inclusive_electron_mc_error_Hz",
            "thin_kick_swept_electron_rate_Hz",
            "thin_kick_swept_electron_mc_error_Hz",
            "unswept_electron_over_total",
            "thin_kick_swept_electron_over_total",
        ]
    )
    for values in zip(
        threshold,
        measured,
        measured_error,
        electron,
        electron_mc_error,
        swept_electron,
        swept_electron_mc_error,
        electron_fraction,
        swept_electron_fraction,
    ):
        writer.writerow([f"{value:.9g}" for value in values])

fig, (top, bottom) = plt.subplots(
    2,
    1,
    figsize=(9.0, 8.0),
    sharex=True,
    gridspec_kw={"height_ratios": [3.0, 1.25]},
)
top.errorbar(
    threshold,
    measured / 1000.0,
    yerr=measured_error / 1000.0,
    fmt="o-",
    ms=4,
    lw=1.5,
    label="Measured total NPS activity",
)
top.plot(
    threshold,
    electron / 1000.0,
    "s--",
    ms=3.5,
    lw=1.5,
    label="F1F2IN21 inclusive electrons",
)
top.plot(
    threshold,
    swept_electron / 1000.0,
    "^:",
    ms=3.5,
    lw=1.5,
    label="Electrons with 0.3 T m thin-kick face-edge loss",
)
top.set_yscale("log")
top.set_ylabel("Rate above threshold (kHz)")
top.set_title("NPS run 6887: measured VTP activity and truth-energy electron calculation")
top.grid(True, which="both", alpha=0.25)
top.legend(frameon=False)

bottom.plot(threshold, electron_fraction, "o-", ms=3.5, label="Unswept")
bottom.plot(threshold, swept_electron_fraction, "^:", ms=3.5, label="Thin kick")
bottom.axhline(1.0, color="0.4", lw=1.0, ls=":")
bottom.set_ylim(0, max(1.05, 1.1 * electron_fraction.max()))
bottom.set_xlim(0.45, 5.05)
bottom.set_xlabel("Numerical threshold (data: VTP 3x3; generator: electron truth energy) (GeV)")
bottom.set_ylabel("Generator / data")
bottom.grid(True, alpha=0.25)
bottom.legend(frameon=False, fontsize=8)

for fraction, value, color in (
    ("70%", 3.387, "#009e73"),
    ("75%", 3.629, "#cc79a7"),
    ("80%", 3.870, "#e69f00"),
):
    for axis in (top, bottom):
        axis.axvline(value, color=color, lw=1.1, ls="--")
    top.text(value + 0.025, 20, fraction, color=color, rotation=90,
             ha="left", va="bottom", fontsize=9)

fig.text(
    0.5,
    0.012,
    "Diagnostic overlay only: detector response and full sweeper transport have not been applied",
    ha="center",
    fontsize=8.5,
)
fig.tight_layout(rect=(0, 0.035, 1, 1))
fig.savefig(OUTPUT.with_suffix(".png"), dpi=180)
fig.savefig(OUTPUT.with_suffix(".pdf"))

print(f"Wrote {OUTPUT.with_suffix('.csv')}")
for reference in (3.387, 3.629, 3.870):
    total_value = float(np.interp(reference, threshold, measured))
    electron_value = float(np.interp(reference, threshold, electron))
    swept_value = float(np.interp(reference, threshold, swept_electron))
    print(
        f"{reference:.3f} GeV: total={total_value/1000:.3f} kHz "
        f"electrons={electron_value/1000:.3f} kHz "
        f"thin_kick={swept_value/1000:.3f} kHz "
        f"fractions={electron_value/total_value:.4f}/{swept_value/total_value:.4f}"
    )
