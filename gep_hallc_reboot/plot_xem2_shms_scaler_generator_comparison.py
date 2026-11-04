#!/usr/bin/env python3
"""Compare XEM2 SHMS singles scalers with the inclusive-electron generator."""

from __future__ import annotations

import csv
import os
from pathlib import Path

os.environ.setdefault("MPLBACKEND", "Agg")
os.environ.setdefault("MPLCONFIGDIR", "/tmp/gep_nps_matplotlib")

import matplotlib.pyplot as plt
import numpy as np


HERE = Path(__file__).resolve().parent
SCALERS = HERE / "xem2_shms_singles_scalers.csv"
OUTPUT = HERE / "plots" / "xem2_shms_scaler_generator_comparison"
GENERATOR_RATE_HZ = 447.7393
GENERATOR_CURRENT_UA = 55.868051


def main() -> None:
    rows = list(csv.DictReader(SCALERS.open()))
    run = np.asarray([int(row["run"]) for row in rows])
    current = np.asarray([float(row["mean_current_uA"]) for row in rows])
    ptrig1 = np.asarray([float(row["pTRIG1_rate_Hz"]) for row in rows])
    pel = np.asarray([float(row["pEL_REAL_rate_Hz"]) for row in rows])
    generator_per_ua = GENERATOR_RATE_HZ / GENERATOR_CURRENT_UA
    predicted = generator_per_ua * current
    x = np.arange(len(run))

    output_csv = OUTPUT.with_suffix(".csv")
    with output_csv.open("w", newline="") as stream:
        writer = csv.writer(stream)
        writer.writerow(
            ["run", "current_uA", "pTRIG1_Hz", "pEL_REAL_Hz", "inclusive_generator_Hz", "generator_over_pEL_REAL"]
        )
        for values in zip(run, current, ptrig1, pel, predicted, predicted / pel):
            writer.writerow(values)

    fig, (top, bottom) = plt.subplots(
        2, 1, figsize=(8.4, 7.2), sharex=True,
        gridspec_kw={"height_ratios": [2.3, 1.0]}, constrained_layout=True,
    )
    top.plot(x, ptrig1 / 1000.0, "o-", label="pTRIG1: SHMS 3-of-4")
    top.plot(x, pel / 1000.0, "o-", label="pEL_REAL: electron-enriched trigger")
    top.plot(x, predicted / 1000.0, "s--", label="F1F2IN21 inclusive electrons")
    top.set_ylabel("Measured or predicted rate (kHz)")
    top.set_title("XEM2 SHMS singles: scaler data and inclusive-generator comparison")
    top.grid(alpha=0.25)
    top.legend(frameon=False)

    ratio = predicted / pel
    bottom.plot(x, ratio, "s-", color="#D55E00")
    bottom.axhline(1.0, color="0.35", ls=":")
    bottom.set_xlabel("Run")
    bottom.set_xticks(x, [str(value) for value in run])
    bottom.set_ylabel("Generator / pEL_REAL")
    bottom.set_ylim(0.65, 0.85)
    bottom.grid(alpha=0.25)
    fig.savefig(OUTPUT.with_suffix(".png"), dpi=180)
    fig.savefig(OUTPUT.with_suffix(".pdf"))

    print(f"generator_rate_per_uA_Hz={generator_per_ua:.6f}")
    print(f"mean_pEL_rate_per_uA_Hz={np.mean(pel/current):.6f}")
    print(f"mean_pTRIG1_rate_per_uA_Hz={np.mean(ptrig1/current):.6f}")
    print(f"mean_generator_over_pEL={np.mean(ratio):.6f}")
    print(f"run_to_run_ratio_rms_percent={100*np.std(ratio, ddof=1)/np.mean(ratio):.4f}")


if __name__ == "__main__":
    main()
