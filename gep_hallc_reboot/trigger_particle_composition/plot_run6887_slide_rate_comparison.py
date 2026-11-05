#!/usr/bin/env python3
"""Make the detector-level run-6887 rate comparison used in the GEp briefing."""

from __future__ import annotations

import csv
import os
from pathlib import Path

os.environ.setdefault("MPLBACKEND", "Agg")
os.environ.setdefault("MPLCONFIGDIR", "/tmp/run6887_slide_plot_matplotlib")

import matplotlib.pyplot as plt
import numpy as np


HERE = Path(__file__).resolve().parent
MEASURED = HERE / "nps_run6887_edtm_threshold_curve.csv"
RESPONSE = HERE / "nps_run6887_total_vs_geant4_electrons.csv"
OUTPUT = HERE / "nps_run6887_measured_vs_geant4_electrons_slide"
ELECTRONUCLEAR_RESPONSE = (
    HERE
    / "background_generator"
    / "target_phase_space"
    / "run6887_xsbias3e5_1000000_cut0p25_nps_rates_6pt.csv"
)


def read_columns(path: Path) -> list[dict[str, str]]:
    with path.open(newline="") as stream:
        return list(csv.DictReader(stream))


def main() -> None:
    measured_rows = read_columns(MEASURED)
    response_rows = read_columns(RESPONSE)
    electronuclear_rows = read_columns(ELECTRONUCLEAR_RESPONSE)

    threshold = np.asarray([float(row["threshold_GeV"]) for row in measured_rows])
    data = np.asarray([float(row["poisson_rate_Hz"]) for row in measured_rows]) / 1000.0
    data_error = np.asarray([float(row["stat_error_Hz"]) for row in measured_rows]) / 1000.0

    sim_threshold = np.asarray([float(row["threshold_GeV"]) for row in response_rows])
    geant4 = np.asarray(
        [float(row["geant4_corrected_3x3_rate_Hz"]) for row in response_rows]
    ) / 1000.0
    geant4_error = np.asarray(
        [float(row["geant4_corrected_3x3_mc_error_Hz"]) for row in response_rows]
    ) / 1000.0
    electronuclear_threshold = np.asarray(
        [float(row["threshold_GeV"]) for row in electronuclear_rows]
    )
    electronuclear_raw = np.asarray(
        [float(row["raw_3x3_rate_Hz"]) for row in electronuclear_rows]
    ) / 1000.0
    electronuclear_raw_error = np.asarray(
        [float(row["raw_3x3_mc_error_Hz"]) for row in electronuclear_rows]
    ) / 1000.0
    electronuclear_corrected = np.asarray(
        [float(row["corrected_3x3_rate_Hz"]) for row in electronuclear_rows]
    ) / 1000.0
    electronuclear_corrected_error = np.asarray(
        [float(row["corrected_3x3_mc_error_Hz"]) for row in electronuclear_rows]
    ) / 1000.0
    electron_at_electronuclear = np.interp(
        electronuclear_threshold, sim_threshold, geant4
    )
    electron_error_at_electronuclear = np.interp(
        electronuclear_threshold, sim_threshold, geant4_error
    )
    combined_raw = electron_at_electronuclear + electronuclear_raw
    combined_raw_error = np.hypot(
        electron_error_at_electronuclear, electronuclear_raw_error
    )
    combined_corrected = electron_at_electronuclear + electronuclear_corrected
    combined_corrected_error = np.hypot(
        electron_error_at_electronuclear, electronuclear_corrected_error
    )

    fig, axis = plt.subplots(figsize=(10.0, 5.4))
    axis.errorbar(
        threshold,
        data,
        yerr=data_error,
        fmt="o-",
        ms=5,
        capsize=2,
        lw=2,
        color="tab:blue",
        label="Measured total NPS rate (unbiased EDTM windows)",
        zorder=4,
    )
    axis.errorbar(
        sim_threshold,
        geant4,
        yerr=geant4_error,
        fmt="D-.",
        ms=4,
        capsize=2,
        lw=2,
        color="tab:red",
        label="GEANT4-folded inclusive electrons (3x3)",
        zorder=5,
    )
    axis.errorbar(
        electronuclear_threshold,
        combined_raw,
        yerr=combined_raw_error,
        fmt="s--",
        ms=6.5,
        capsize=3,
        lw=1.8,
        color="#E69F00",
        label=r"Combined: inclusive $e^-$ + $e$-nuclear (raw 3x3)",
        zorder=9,
    )
    axis.errorbar(
        electronuclear_threshold,
        combined_corrected,
        yerr=combined_corrected_error,
        fmt="s--",
        ms=6.5,
        capsize=3,
        lw=1.8,
        mfc="white",
        mec="#E69F00",
        mew=1.5,
        ecolor="#E69F00",
        color="#E69F00",
        label=r"Combined: inclusive $e^-$ + $e$-nuclear (corrected 3x3)",
        zorder=10,
    )

    for fraction, value, color in (
        (70, 3.387, "#009E73"),
        (75, 3.629, "#CC79A7"),
        (80, 3.870, "#E69F00"),
    ):
        axis.axvline(value, color=color, ls="--", lw=1.4, alpha=0.9)
        axis.text(
            value + 0.025,
            16.0,
            f"{fraction}%",
            rotation=90,
            va="bottom",
            ha="left",
            color=color,
            fontsize=10,
        )

    axis.axvspan(0.5, 2.5, color="0.8", alpha=0.16)
    axis.text(
        1.50,
        11.5,
        "GEANT4 pilot not evaluated below 2.5 GeV",
        ha="center",
        va="bottom",
        fontsize=9,
        color="0.35",
    )
    axis.set_yscale("log")
    axis.set_xlim(0.45, 5.05)
    axis.set_ylim(0.25, 7000.0)
    axis.set_xlabel("Offline VTP 3x3 cluster threshold (GeV)")
    axis.set_ylabel("Rate above threshold (kHz)")
    axis.set_title("NPS run 6887: measured rate and detector-level modeled components")
    axis.grid(True, which="both", alpha=0.25)
    axis.legend(frameon=False, fontsize=8.7, loc="upper right")
    fig.tight_layout()
    fig.savefig(OUTPUT.with_suffix(".png"), dpi=180)
    fig.savefig(OUTPUT.with_suffix(".pdf"))


if __name__ == "__main__":
    main()
