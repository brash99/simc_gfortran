#!/usr/bin/env python3
"""Fit run-6151 scaler rates and form a provisional 75-uA accidental envelope."""

from __future__ import annotations

import csv
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

ROOT = Path(__file__).resolve().parent
INPUT = ROOT / "nps_run6151_trigger_rates_snapshots.csv"
TARGET_CURRENT = 75.0
FIT_LOW, FIT_HIGH = 15.0, 30.0
COINCIDENCE_WINDOWS_NS = (10.0, 20.0, 40.0, 80.0)

with INPUT.open(newline="") as stream:
    rows = list(csv.DictReader(stream))

current = np.asarray([float(row["current_uA"]) for row in rows])
fit_mask = (current >= FIT_LOW) & (current < FIT_HIGH)
channels = {
    "NPS VTP singles (hTRIG1)": "hTRIG1_rate_Hz",
    "HMS 3/4 (hTRIG3)": "hTRIG3_rate_Hz",
    "HMS EL-REAL (hTRIG4)": "hTRIG4_rate_Hz",
}

fits: dict[str, dict[str, float]] = {}
for label, branch in channels.items():
    rate = np.asarray([float(row[branch]) for row in rows])
    slope, intercept = np.polyfit(current[fit_mask], rate[fit_mask], 1)
    origin_slope = np.dot(current[fit_mask], rate[fit_mask]) / np.dot(
        current[fit_mask], current[fit_mask])
    fits[label] = {
        "slope_Hz_per_uA": float(slope),
        "intercept_Hz": float(intercept),
        "correlation": float(np.corrcoef(current[fit_mask], rate[fit_mask])[0, 1]),
        "rate_75uA_linear_Hz": float(intercept + slope * TARGET_CURRENT),
        "rate_75uA_through_origin_Hz": float(origin_slope * TARGET_CURRENT),
    }

nps_linear = fits["NPS VTP singles (hTRIG1)"]["rate_75uA_linear_Hz"]
hms_linear = fits["HMS 3/4 (hTRIG3)"]["rate_75uA_linear_Hz"]
nps_origin = fits["NPS VTP singles (hTRIG1)"]["rate_75uA_through_origin_Hz"]
hms_origin = fits["HMS 3/4 (hTRIG3)"]["rate_75uA_through_origin_Hz"]

output_rows = []
for width_ns in COINCIDENCE_WINDOWS_NS:
    output_rows.append({
        "coincidence_window_ns": width_ns,
        "nps_rate_75uA_linear_Hz": nps_linear,
        "hms_rate_75uA_linear_Hz": hms_linear,
        "accidental_linear_Hz": nps_linear*hms_linear*width_ns*1.e-9,
        "nps_rate_75uA_through_origin_Hz": nps_origin,
        "hms_rate_75uA_through_origin_Hz": hms_origin,
        "accidental_through_origin_Hz": nps_origin*hms_origin*width_ns*1.e-9,
    })

with (ROOT / "nps_trigger_projection_75uA.csv").open("w", newline="") as stream:
    writer = csv.DictWriter(stream, fieldnames=output_rows[0].keys())
    writer.writeheader()
    writer.writerows(output_rows)

with (ROOT / "nps_trigger_rate_fits.csv").open("w", newline="") as stream:
    fields = ["channel", *next(iter(fits.values())).keys()]
    writer = csv.DictWriter(stream, fieldnames=fields)
    writer.writeheader()
    for channel, values in fits.items():
        writer.writerow({"channel": channel, **values})

fig, axes = plt.subplots(1, 2, figsize=(13.2, 5.4))
colors = ("tab:blue", "tab:orange", "tab:green")
xfit = np.linspace(15., 75., 300)
for (label, branch), color in zip(channels.items(), colors):
    rate = np.asarray([float(row[branch]) for row in rows])
    axes[0].scatter(current[fit_mask], rate[fit_mask]/1000., s=8, alpha=.25,
                    color=color)
    fit = fits[label]
    axes[0].plot(xfit, (fit["intercept_Hz"]+fit["slope_Hz_per_uA"]*xfit)/1000.,
                 color=color, linewidth=2, label=label)
axes[0].axvline(30., color="0.4", linestyle="--", linewidth=1)
axes[0].axvline(75., color="0.4", linestyle=":", linewidth=1)
axes[0].set(xlabel="BCM4A current (microampere)", ylabel="Raw scaler rate (kHz)",
            title="Run 6151 stable-current scaler rates")
axes[0].grid(alpha=.25)
axes[0].legend(fontsize=9)

widths = np.asarray(COINCIDENCE_WINDOWS_NS)
axes[1].plot(widths, [row["accidental_linear_Hz"] for row in output_rows], "o-",
             label="linear fit with intercept")
axes[1].plot(widths, [row["accidental_through_origin_Hz"] for row in output_rows], "s--",
             label="through-origin variation")
axes[1].set(xlabel="Full coincidence window (ns)",
            ylabel="Estimated accidental coincidence rate (Hz)",
            title="75-microampere NPS x HMS accidental envelope")
axes[1].grid(alpha=.25)
axes[1].legend()
fig.tight_layout()
fig.savefig(ROOT / "plots/nps_trigger_projection_75uA.png", dpi=180)
fig.savefig(ROOT / "plots/nps_trigger_projection_75uA.pdf")

for channel, values in fits.items():
    print(f"{channel}: slope={values['slope_Hz_per_uA']:.3f} Hz/uA, "
          f"intercept={values['intercept_Hz']:.2f} Hz, "
          f"R(75)={values['rate_75uA_linear_Hz']:.2f} Hz, "
          f"r={values['correlation']:.5f}")
for row in output_rows:
    print(f"window={row['coincidence_window_ns']:.0f} ns: "
          f"accidental={row['accidental_linear_Hz']:.3f} Hz "
          f"(origin variation {row['accidental_through_origin_Hz']:.3f} Hz)")
