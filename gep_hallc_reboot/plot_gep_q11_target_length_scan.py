#!/usr/bin/env python3
"""Plot the matched GEp Q2=11 SIMC target-length scan."""

from __future__ import annotations

import csv
import os
from pathlib import Path

os.environ.setdefault("MPLCONFIGDIR", "/tmp/gep_target_scan_matplotlib")

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np


HERE = Path(__file__).resolve().parent


def main() -> None:
    with (HERE / "gep_q11_target_length_scan.csv").open(newline="") as stream:
        rows = list(csv.DictReader(stream))

    lengths = np.array([float(row["target_length_cm"]) for row in rows])
    yields = np.array(
        [float(row["selected_yield_events_per_mC"]) for row in rows]
    )
    linear = yields[0] * lengths / lengths[0]

    fig, axis = plt.subplots(figsize=(7.2, 5.2))
    axis.plot(
        lengths,
        linear,
        "--",
        color="0.45",
        lw=1.8,
        label="Linear scaling from 10 cm",
    )
    axis.plot(
        lengths,
        yields,
        "o-",
        color="#0072B2",
        lw=2.4,
        ms=7,
        label="SIMC: HMS + NPS at 3 m",
    )
    for length, value in zip(lengths, yields):
        axis.annotate(
            f"{value:.2f} events/mC",
            (length, value),
            xytext=(0, 9),
            textcoords="offset points",
            ha="center",
            fontsize=9,
        )

    axis.set(
        xlabel="Centered LH2 target length (cm)",
        ylabel="Accepted elastic yield (events/mC)",
        xticks=lengths,
        title="GEp Q2 = 11 target-length scan",
    )
    axis.grid(alpha=0.25)
    axis.legend(frameon=False, loc="upper left")
    axis.text(
        0.98,
        0.05,
        "10.7 GeV beam; HMS 6.735 GeV/c at 18.797 deg\n"
        "NPS 4.838 GeV at 26.651 deg and 3.0 m",
        transform=axis.transAxes,
        ha="right",
        va="bottom",
        fontsize=8.5,
    )
    fig.tight_layout()
    for suffix in ("png", "pdf"):
        fig.savefig(HERE / "plots" / f"gep_q11_target_length_scan.{suffix}", dpi=180)


if __name__ == "__main__":
    main()
