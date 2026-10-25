#!/usr/bin/env python3
"""Combine run-1918 EDTM occupancy rates with run-6151 elastic efficiency."""

from __future__ import annotations

import csv
import os
from pathlib import Path

os.environ.setdefault("MPLCONFIGDIR", "/tmp/gep_nps_matplotlib")
os.environ.setdefault("XDG_CACHE_HOME", "/tmp/gep_nps_cache")
os.environ.setdefault("MPLBACKEND", "Agg")

import matplotlib.pyplot as plt
import numpy as np


HERE = Path(__file__).resolve().parent
RATE_CSV = HERE / "nps_run1918_100k_threshold_scan.csv"
EFFICIENCY_CSV = HERE / "nps_run6151_elastic_efficiency.csv"
OUTPUT_CSV = HERE / "nps_primitive_threshold_result.csv"
KEY_POINTS_CSV = HERE / "nps_primitive_threshold_key_points.csv"
OUTPUT_PNG = HERE / "plots" / "nps_primitive_rate_and_elastic_efficiency.png"
OUTPUT_PDF = HERE / "plots" / "nps_primitive_rate_and_elastic_efficiency.pdf"
RATE_PNG = HERE / "plots" / "nps_primitive_rate_vs_threshold.png"
RATE_PDF = HERE / "plots" / "nps_primitive_rate_vs_threshold.pdf"
EFFICIENCY_PNG = HERE / "plots" / "nps_elastic_efficiency_vs_threshold.png"
EFFICIENCY_PDF = HERE / "plots" / "nps_elastic_efficiency_vs_threshold.pdf"

N_EDTM = 14_450
READOUT_WINDOW_S = 1.0e-6
RUN1918_CURRENT_UA = 28.787
RUN1918_TARGET_CM = 10.0
PROPOSAL_CURRENT_UA = 75.0
PROPOSAL_TARGET_CM = 30.0
PROPOSAL_ELASTIC_E_GEV = 4.838
RUN6151_ELASTIC_E_GEV = 4.598
P_TRIG1_SCALER_HZ = 353_486.0
NOMINAL_HARDWARE_THRESHOLD_GEV = 1.4
RUN1918_VTP_INTERCEPT_GEV = 0.463791
RUN1918_VTP_SLOPE = 0.668158
RUN6151_VTP_INTERCEPT_GEV = 0.222980
RUN6151_VTP_SLOPE = 0.911203


def load_columns(path: Path, names: tuple[str, ...]) -> dict[str, np.ndarray]:
    with path.open(newline="") as stream:
        rows = list(csv.DictReader(stream))
    return {name: np.array([float(row[name]) for row in rows]) for name in names}


rate_data = load_columns(RATE_CSV, ("threshold_nominal_GeV", "edtm_events"))
eff_data = load_columns(EFFICIENCY_CSV, ("threshold_nominal_GeV", "efficiency", "binomial_error"))

thresholds = rate_data["threshold_nominal_GeV"]
counts = rate_data["edtm_events"]
occupancy = counts / N_EDTM
occupancy_error = np.sqrt(occupancy * (1.0 - occupancy) / N_EDTM)
unsaturated = occupancy < 1.0
rates_hz = np.full_like(occupancy, np.nan)
rates_error_hz = np.full_like(occupancy, np.nan)
rates_hz[unsaturated] = -np.log1p(-occupancy[unsaturated]) / READOUT_WINDOW_S
rates_error_hz[unsaturated] = occupancy_error[unsaturated] / (
    (1.0 - occupancy[unsaturated]) * READOUT_WINDOW_S
)

luminosity_scale = (PROPOSAL_CURRENT_UA / RUN1918_CURRENT_UA) * (
    PROPOSAL_TARGET_CM / RUN1918_TARGET_CM
)
projected_rates_hz = rates_hz * luminosity_scale
projected_rates_error_hz = rates_error_hz * luminosity_scale


def run1918_vtp_from_offline(offline_energy: float | np.ndarray) -> float | np.ndarray:
    return RUN1918_VTP_INTERCEPT_GEV + RUN1918_VTP_SLOPE * offline_energy


def run1918_offline_from_vtp(vtp_energy: float | np.ndarray) -> float | np.ndarray:
    return (vtp_energy - RUN1918_VTP_INTERCEPT_GEV) / RUN1918_VTP_SLOPE


def run6151_vtp_from_offline(offline_energy: float | np.ndarray) -> float | np.ndarray:
    return RUN6151_VTP_INTERCEPT_GEV + RUN6151_VTP_SLOPE * offline_energy

with OUTPUT_CSV.open("w", newline="") as stream:
    writer = csv.writer(stream)
    writer.writerow(
        [
            "threshold_nominal_GeV",
            "run1918_offline_equivalent_threshold_GeV",
            "threshold_fraction_of_GEp_elastic_energy",
            "run1918_edtm_windows",
            "run1918_windows_above_threshold",
            "run1918_raw_primitive_rate_Hz",
            "run1918_stat_error_Hz",
            "proposal_naively_scaled_rate_Hz",
            "proposal_stat_error_Hz",
            "run6151_efficiency_at_same_physical_fraction",
            "run6151_fraction_matched_stat_error",
        ]
    )
    for threshold, count, rate, rate_error, projected, projected_error in zip(
        thresholds, counts, rates_hz, rates_error_hz, projected_rates_hz, projected_rates_error_hz
    ):
        offline_equivalent = run1918_offline_from_vtp(threshold)
        physical_fraction = offline_equivalent / PROPOSAL_ELASTIC_E_GEV
        run6151_fraction_matched_threshold = run6151_vtp_from_offline(
            physical_fraction * RUN6151_ELASTIC_E_GEV
        )
        fraction_matched_efficiency = np.interp(
            run6151_fraction_matched_threshold,
            eff_data["threshold_nominal_GeV"],
            eff_data["efficiency"],
        )
        fraction_matched_error = np.interp(
            run6151_fraction_matched_threshold,
            eff_data["threshold_nominal_GeV"],
            eff_data["binomial_error"],
        )
        writer.writerow(
            [
                f"{threshold:.3f}",
                f"{offline_equivalent:.6f}",
                f"{physical_fraction:.6f}",
                N_EDTM,
                int(count),
                f"{rate:.3f}",
                f"{rate_error:.3f}",
                f"{projected:.3f}",
                f"{projected_error:.3f}",
                f"{fraction_matched_efficiency:.8f}",
                f"{fraction_matched_error:.8f}",
            ]
        )


def interpolate_log_rate(threshold: float, values: np.ndarray) -> float:
    finite = np.isfinite(values) & (values > 0.0)
    return float(np.exp(np.interp(threshold, thresholds[finite], np.log(values[finite]))))


print(f"Naive luminosity scale (75/28.787)*(30/10) = {luminosity_scale:.6f}")
nominal_rate = interpolate_log_rate(NOMINAL_HARDWARE_THRESHOLD_GEV, rates_hz)
print(
    f"Run 1918 at 1.4 GeV: EDTM occupancy rate = {nominal_rate / 1e3:.3f} kHz; "
    f"pTRIG1 scaler = {P_TRIG1_SCALER_HZ / 1e3:.3f} kHz; "
    f"ratio = {nominal_rate / P_TRIG1_SCALER_HZ:.4f}"
)
print("fraction,physical_threshold_GeV,run1918_VTP_threshold_GeV,run1918_kHz,proposal_kHz,run6151_efficiency")
with KEY_POINTS_CSV.open("w", newline="") as stream:
    writer = csv.writer(stream)
    writer.writerow(
        [
            "physical_threshold_fraction",
            "physical_threshold_GeV",
            "run1918_VTP_threshold_GeV",
            "run1918_raw_primitive_rate_Hz",
            "proposal_naively_scaled_rate_Hz",
            "run6151_VTP_threshold_GeV",
            "run6151_elastic_core_efficiency",
        ]
    )
    for fraction in (0.50, 0.60, 0.70, 0.75, 0.80, 0.85):
        physical_threshold = fraction * PROPOSAL_ELASTIC_E_GEV
        run1918_vtp_threshold = run1918_vtp_from_offline(physical_threshold)
        run_rate = interpolate_log_rate(run1918_vtp_threshold, rates_hz)
        projected_rate = run_rate * luminosity_scale
        run6151_vtp_threshold = run6151_vtp_from_offline(fraction * RUN6151_ELASTIC_E_GEV)
        efficiency = float(
            np.interp(
                run6151_vtp_threshold,
                eff_data["threshold_nominal_GeV"],
                eff_data["efficiency"],
            )
        )
        writer.writerow(
            [
                f"{fraction:.4f}",
                f"{physical_threshold:.6f}",
                f"{run1918_vtp_threshold:.6f}",
                f"{run_rate:.3f}",
                f"{projected_rate:.3f}",
                f"{run6151_vtp_threshold:.6f}",
                f"{efficiency:.8f}",
            ]
        )
        print(
            f"{fraction:.2f},{physical_threshold:.4f},{run1918_vtp_threshold:.4f},{run_rate / 1e3:.3f},"
            f"{projected_rate / 1e3:.3f},{efficiency:.5f}"
        )

OUTPUT_PNG.parent.mkdir(parents=True, exist_ok=True)
plt.rcParams.update({"font.size": 11})
figure, axes = plt.subplots(1, 2, figsize=(12.4, 5.0), constrained_layout=True)

ax = axes[0]
ax.semilogy(thresholds, rates_hz / 1e3, "o-", ms=4, label="Run 1918, EDTM occupancy")
ax.semilogy(
    thresholds,
    projected_rates_hz / 1e3,
    "s-",
    ms=3,
    label="75 µA, 30 cm naive scaling",
)
ax.scatter(
    [NOMINAL_HARDWARE_THRESHOLD_GEV],
    [P_TRIG1_SCALER_HZ / 1e3],
    marker="*",
    s=130,
    color="black",
    zorder=5,
    label="Run 1918 pTRIG1 scaler",
)
for fraction, color in ((0.70, "tab:green"), (0.80, "tab:red")):
    vtp_threshold = run1918_vtp_from_offline(fraction * PROPOSAL_ELASTIC_E_GEV)
    ax.axvline(
        vtp_threshold,
        color=color,
        ls="--",
        lw=1.2,
        label=f"{int(100 * fraction)}% physical threshold",
    )
ax.set_xlabel("Nominal VTP cluster threshold (GeV)")
ax.set_ylabel("Raw NPS primitive rate (kHz)")
ax.set_xlim(0.8, 4.0)
ax.grid(True, which="both", alpha=0.25)
ax.legend(fontsize=9)
ax.set_title("Rate from unbiased 1 µs EDTM windows")
secondary = ax.secondary_xaxis(
    "top", functions=(run1918_offline_from_vtp, run1918_vtp_from_offline)
)
secondary.set_xlabel("Run-1918 offline-equivalent cluster energy (GeV)")

ax = axes[1]
run6151_offline_equivalent = (
    eff_data["threshold_nominal_GeV"] - RUN6151_VTP_INTERCEPT_GEV
) / RUN6151_VTP_SLOPE
run6151_fraction = run6151_offline_equivalent / RUN6151_ELASTIC_E_GEV
ax.plot(
    100.0 * run6151_fraction,
    100.0 * eff_data["efficiency"],
    color="tab:blue",
    lw=2,
    label="Run 6151 proton-tagged elastic core",
)
ax.fill_between(
    100.0 * run6151_fraction,
    100.0 * (eff_data["efficiency"] - eff_data["binomial_error"]),
    100.0 * (eff_data["efficiency"] + eff_data["binomial_error"]),
    color="tab:blue",
    alpha=0.2,
)
for fraction, color in ((0.70, "tab:green"), (0.80, "tab:red")):
    ax.axvline(
        100.0 * fraction,
        color=color,
        ls="--",
        lw=1.2,
        label=f"{int(100 * fraction)}% threshold fraction",
    )
ax.set_xlabel("Threshold / elastic-electron energy (%)")
ax.set_ylabel("Elastic efficiency (%)")
ax.set_xlim(20, 92)
ax.set_ylim(45, 101)
ax.grid(True, alpha=0.25)
ax.legend(fontsize=9, loc="lower left")
ax.set_title("Measured efficiency at nearby elastic energy")

figure.suptitle("NPS threshold tradeoff from Hall C data", fontsize=15)
figure.savefig(OUTPUT_PNG, dpi=180)
figure.savefig(OUTPUT_PDF)

# Standalone rate figure: this result uses run 1918 only.
rate_figure, rate_ax = plt.subplots(figsize=(8.4, 5.8), constrained_layout=True)
rate_ax.semilogy(thresholds, rates_hz / 1e3, "o-", ms=4, label="Run 1918, EDTM occupancy")
rate_ax.semilogy(
    thresholds,
    projected_rates_hz / 1e3,
    "s-",
    ms=3,
    label="75 µA, 30 cm conservative scaling",
)
rate_ax.scatter(
    [NOMINAL_HARDWARE_THRESHOLD_GEV],
    [P_TRIG1_SCALER_HZ / 1e3],
    marker="*",
    s=130,
    color="black",
    zorder=5,
    label="Run 1918 pTRIG1 scaler",
)
for fraction, color in ((0.70, "tab:green"), (0.80, "tab:red")):
    rate_ax.axvline(
        run1918_vtp_from_offline(fraction * PROPOSAL_ELASTIC_E_GEV),
        color=color,
        ls="--",
        lw=1.2,
        label=f"{int(100 * fraction)}% physical threshold",
    )
rate_ax.set_xlabel("Nominal VTP cluster threshold (GeV)")
rate_ax.set_ylabel("Raw NPS primitive rate (kHz)")
rate_ax.set_xlim(0.8, 4.0)
rate_ax.grid(True, which="both", alpha=0.25)
rate_ax.legend(fontsize=9)
rate_ax.set_title("NPS primitive rate from unbiased run-1918 EDTM windows")
rate_secondary = rate_ax.secondary_xaxis(
    "top", functions=(run1918_offline_from_vtp, run1918_vtp_from_offline)
)
rate_secondary.set_xlabel("Run-1918 offline-equivalent cluster energy (GeV)")
rate_figure.savefig(RATE_PNG, dpi=180)
rate_figure.savefig(RATE_PDF)

# Standalone efficiency figure: this independent cross-check uses run 6151.
efficiency_figure, efficiency_ax = plt.subplots(figsize=(8.4, 5.8), constrained_layout=True)
efficiency_ax.plot(
    100.0 * run6151_fraction,
    100.0 * eff_data["efficiency"],
    color="tab:blue",
    lw=2,
    label="Run 6151 proton-tagged elastic core",
)
efficiency_ax.fill_between(
    100.0 * run6151_fraction,
    100.0 * (eff_data["efficiency"] - eff_data["binomial_error"]),
    100.0 * (eff_data["efficiency"] + eff_data["binomial_error"]),
    color="tab:blue",
    alpha=0.2,
)
for fraction, color in ((0.70, "tab:green"), (0.80, "tab:red")):
    efficiency_ax.axvline(
        100.0 * fraction,
        color=color,
        ls="--",
        lw=1.2,
        label=f"{int(100 * fraction)}% threshold fraction",
    )
efficiency_ax.set_xlabel("Physical threshold / elastic-electron energy (%)")
efficiency_ax.set_ylabel("Elastic-core trigger efficiency (%)")
efficiency_ax.set_xlim(20, 92)
efficiency_ax.set_ylim(45, 101)
efficiency_ax.grid(True, alpha=0.25)
efficiency_ax.legend(fontsize=9, loc="lower left")
efficiency_ax.set_title("Independent elastic-core efficiency check from run 6151")
efficiency_figure.savefig(EFFICIENCY_PNG, dpi=180)
efficiency_figure.savefig(EFFICIENCY_PDF)

print(f"Wrote {OUTPUT_CSV}")
print(f"Wrote {KEY_POINTS_CSV}")
print(f"Wrote {OUTPUT_PNG}")
print(f"Wrote {RATE_PNG}")
print(f"Wrote {EFFICIENCY_PNG}")
