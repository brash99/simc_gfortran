#!/usr/bin/env python3
"""Plot EDTM-occupancy closure against the direct pTRIG1 scaler."""

from __future__ import annotations

import csv
import os
from pathlib import Path

os.environ.setdefault("MPLCONFIGDIR", "/tmp/gep_nps_matplotlib")
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt


HERE = Path(__file__).resolve().parent
RUNS = (6158, 6165, 6168)


def read_row(run: int) -> dict[str, float]:
    path = HERE / f"nps_run{run}_100k_edtm_scaler_closure.csv"
    with path.open(newline="") as stream:
        row = next(csv.DictReader(stream))
    return {key: float(value) for key, value in row.items()}


def main() -> None:
    rows = [read_row(run) for run in RUNS]
    rows.sort(key=lambda row: row["nps_angle_deg"])
    combined_csv = HERE / "nps_electron_edtm_scaler_closure.csv"
    with combined_csv.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=rows[0].keys())
        writer.writeheader()
        writer.writerows(rows)
    angles = [row["nps_angle_deg"] for row in rows]
    scaler = [row["ptrig1_scaler_rate_Hz"] / 1000.0 for row in rows]
    scaler_rms = [row["ptrig1_scaler_rms_Hz"] / 1000.0 for row in rows]
    edtm = [row["edtm_rate_Hz"] / 1000.0 for row in rows]
    edtm_error = [row["edtm_stat_error_Hz"] / 1000.0 for row in rows]
    ratios = [row["edtm_over_scaler"] for row in rows]
    ratio_errors = [row["edtm_stat_error_Hz"] / row["ptrig1_scaler_rate_Hz"] for row in rows]

    fig, axes = plt.subplots(1, 2, figsize=(11.2, 4.5), gridspec_kw={"width_ratios": [1.4, 1.0]})
    axes[0].errorbar(angles, scaler, yerr=scaler_rms, marker="s", ms=6, capsize=3,
                     color="#D55E00", label="Direct pTRIG1 scaler (RMS)")
    axes[0].errorbar(angles, edtm, yerr=edtm_error, marker="o", ms=6, capsize=3,
                     color="#0072B2", label="1-us EDTM occupancy (stat.)")
    axes[0].set_xlabel("NPS central angle (deg)")
    axes[0].set_ylabel("Primitive rate at 1.6 GeV VTP threshold (kHz)")
    axes[0].set_title("Independent rate comparison")
    axes[0].grid(alpha=0.25)
    axes[0].legend(frameon=False)

    axes[1].axhline(1.0, color="black", lw=1.2)
    axes[1].axhspan(0.98, 1.02, color="0.75", alpha=0.3, label="within 2%")
    axes[1].errorbar(angles, ratios, yerr=ratio_errors, marker="o", ms=6, capsize=3,
                     color="#0072B2")
    annotation_offsets = {6168: (5, 7), 6165: (5, 7), 6158: (-58, 7)}
    for row in rows:
        run = int(row["run"])
        axes[1].annotate(f"run {int(row['run'])}",
                         (row["nps_angle_deg"], row["edtm_over_scaler"]),
                         xytext=annotation_offsets[run], textcoords="offset points", fontsize=8)
    axes[1].set_xlabel("NPS central angle (deg)")
    axes[1].set_ylabel("EDTM-derived rate / pTRIG1 scaler")
    axes[1].set_xlim(18.25, 21.5)
    axes[1].set_ylim(0.94, 1.06)
    axes[1].set_title("Closure ratio")
    axes[1].grid(alpha=0.25)
    axes[1].legend(frameon=False, loc="lower right")

    fig.suptitle("NPS electron runs: EDTM occupancy reproduces the hardware primitive scaler", y=1.02)
    fig.tight_layout()
    output = HERE / "plots" / "nps_electron_edtm_scaler_closure"
    fig.savefig(output.with_suffix(".png"), dpi=180, bbox_inches="tight")
    fig.savefig(output.with_suffix(".pdf"), bbox_inches="tight")

    for row in rows:
        print(f"run {int(row['run'])}: EDTM={row['edtm_rate_Hz']/1000:.3f} +/- "
              f"{row['edtm_stat_error_Hz']/1000:.3f} kHz, "
              f"pTRIG1={row['ptrig1_scaler_rate_Hz']/1000:.3f} kHz, "
              f"ratio={row['edtm_over_scaler']:.5f}")


if __name__ == "__main__":
    main()
