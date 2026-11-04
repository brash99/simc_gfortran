#!/usr/bin/env python3
import csv
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np


HERE = Path(__file__).resolve().parent
INPUT = HERE / "nps_run6887_edtm_threshold_curve.csv"
OUTPUT = HERE / "nps_run6887_edtm_threshold_curve"

with INPUT.open(newline="") as stream:
    rows = list(csv.DictReader(stream))

threshold = np.array([float(row["threshold_GeV"]) for row in rows])
rate_khz = np.array([float(row["poisson_rate_Hz"]) / 1000.0 for row in rows])
error_khz = np.array([float(row["stat_error_Hz"]) / 1000.0 for row in rows])
empty = np.array([int(row["empty_windows"]) for row in rows])
near_saturation = empty < 100

fig, ax = plt.subplots(figsize=(9.0, 6.2))
ax.errorbar(
    threshold[~near_saturation],
    rate_khz[~near_saturation],
    yerr=error_khz[~near_saturation],
    fmt="o-",
    ms=4,
    lw=1.4,
    color="#1565c0",
    label="EDTM occupancy measurement",
)
ax.errorbar(
    threshold[near_saturation],
    rate_khz[near_saturation],
    yerr=error_khz[near_saturation],
    fmt="s",
    ms=5,
    color="#d55e00",
    label="Near saturation (<100 empty windows)",
)

for fraction, value, color in (
    ("70%", 3.387, "#009e73"),
    ("75%", 3.629, "#cc79a7"),
    ("80%", 3.870, "#e69f00"),
):
    ax.axvline(value, color=color, lw=1.2, ls="--")
    ax.text(value + 0.025, 22, fraction, color=color, rotation=90,
            ha="left", va="bottom", fontsize=9)

ax.set_yscale("log")
ax.set_xlim(0.45, 5.05)
ax.set_ylim(10, 8000)
ax.set_xlabel("Offline VTP 3x3 cluster threshold (GeV)")
ax.set_ylabel("Face-integrated cluster rate (kHz)")
ax.set_title("NPS run 6887: threshold dependence from unbiased 1 us EDTM windows")
ax.grid(True, which="both", alpha=0.25)
ax.legend(loc="upper right", frameon=False)
fig.text(
    0.5,
    0.015,
    "10 cm LH2, 29.4 uA, 8.456 GeV beam, NPS 16.57 deg at 3.57 m; "
    "8,972 selected EDTM windows",
    ha="center",
    fontsize=8.5,
)
fig.tight_layout(rect=(0, 0.04, 1, 1))
fig.savefig(OUTPUT.with_suffix(".png"), dpi=180)
fig.savefig(OUTPUT.with_suffix(".pdf"))
