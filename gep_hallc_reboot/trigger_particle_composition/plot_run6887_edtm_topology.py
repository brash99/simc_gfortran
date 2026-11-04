#!/usr/bin/env python3
"""Plot data-driven NPS shower-topology diagnostics for run 6887 EDTM windows."""

from pathlib import Path
import os

os.environ.setdefault("MPLCONFIGDIR", "/tmp/nps_run6887_topology_matplotlib")
os.environ.setdefault("XDG_CACHE_HOME", "/tmp/nps_run6887_topology_cache")
os.environ.setdefault("MPLBACKEND", "Agg")

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

HERE = Path(__file__).resolve().parent
INPUT = HERE / "nps_run6887_edtm_topology.csv"
OUTPUT = HERE / "nps_run6887_edtm_topology"

data = pd.read_csv(INPUT)
thresholds = np.array([2.5, 3.387, 3.629, 3.870])
labels = ["2.5", "3.387", "3.629", "3.870"]
columns = ["2p5", "3p387", "3p629", "3p870"]

fig, axes = plt.subplots(2, 2, figsize=(11.0, 8.4))

selected = data[data.max_energy_GeV >= 2.5]
hexplot = axes[0, 0].hexbin(
    selected.max_energy_GeV,
    selected.max_size,
    gridsize=(30, 8),
    extent=(2.5, 8.0, 1.5, 9.5),
    mincnt=1,
    cmap="viridis",
)
fig.colorbar(hexplot, ax=axes[0, 0], label="EDTM windows")
axes[0, 0].set_xlim(2.5, 8.0)
axes[0, 0].set_ylim(1.5, 9.5)
axes[0, 0].set_xlabel("Leading VTP 3x3 energy (GeV)")
axes[0, 0].set_ylabel("Number of crystals in leading cluster")
axes[0, 0].set_title("Shower compactness")

reference = data[data.max_energy_GeV >= 3.387]
position = axes[0, 1].hist2d(
    reference.max_x_column,
    reference.max_y_row,
    bins=(30, 36),
    range=((-0.5, 29.5), (-0.5, 35.5)),
    cmap="magma",
)
fig.colorbar(position[3], ax=axes[0, 1], label="EDTM windows")
axes[0, 1].set_xlabel("Leading-cluster column")
axes[0, 1].set_ylabel("Leading-cluster row")
axes[0, 1].set_title("Position for E >= 3.387 GeV")

sizes = np.arange(1.5, 10.5, 1.0)
for threshold, label in zip((2.5, 3.387, 3.870), ("2.5", "3.387", "3.870")):
    sample = data[data.max_energy_GeV >= threshold]
    axes[1, 0].hist(
        sample.max_size,
        bins=sizes,
        density=True,
        histtype="step",
        linewidth=1.8,
        label=f">= {label} GeV (N={len(sample)})",
    )
axes[1, 0].set_xlabel("Number of crystals in leading cluster")
axes[1, 0].set_ylabel("Fraction per crystal-count bin")
axes[1, 0].set_title("Leading-cluster size")
axes[1, 0].legend(frameon=False, fontsize=8.5)

fractions = []
errors = []
counts = []
for threshold, column in zip(thresholds, columns):
    sample = data[data.max_energy_GeV >= threshold]
    multiple = (sample[f"n_separate_{column}"] > 0).sum()
    fraction = multiple / len(sample)
    fractions.append(fraction)
    errors.append(np.sqrt(fraction * (1.0 - fraction) / len(sample)))
    counts.append((len(sample), multiple))
axes[1, 1].errorbar(
    thresholds,
    100.0 * np.array(fractions),
    yerr=100.0 * np.array(errors),
    fmt="o-",
    color="tab:red",
    capsize=3,
)
axes[1, 1].set_xlabel("Energy threshold (GeV)")
axes[1, 1].set_ylabel("Windows with a separate shower candidate (%)")
axes[1, 1].set_title("Non-overlapping high-energy 3x3 regions")
axes[1, 1].set_ylim(bottom=0)

for axis in axes.flat:
    axis.grid(alpha=0.2)

fig.suptitle(
    "NPS run 6887: topology in unbiased 1-us EDTM windows (25-35 uA)",
    fontsize=14,
)
fig.text(
    0.5,
    0.012,
    "Topology is not particle identification: run 6887 has no independent charged-particle tag in front of NPS.",
    ha="center",
    fontsize=9,
)
fig.tight_layout(rect=(0, 0.035, 1, 0.96))
fig.savefig(OUTPUT.with_suffix(".png"), dpi=180)
fig.savefig(OUTPUT.with_suffix(".pdf"))

print(f"selected_EDTM_windows={len(data)}")
for threshold, (total, multiple), fraction, error in zip(
    labels, counts, fractions, errors
):
    print(
        f"threshold={threshold} GeV windows={total} "
        f"separate={multiple} fraction={fraction:.5f}+/-{error:.5f}"
    )
