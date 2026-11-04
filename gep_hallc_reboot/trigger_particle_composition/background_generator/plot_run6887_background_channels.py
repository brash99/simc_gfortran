#!/usr/bin/env python3
"""Plot the run-6887 residual and generator-level background channels."""

import csv
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np

HERE = Path(__file__).resolve().parent
rows = list(csv.DictReader((HERE / "run6887_wiser_pilot_rates.csv").open()))


def column(name: str) -> np.ndarray:
    return np.array([float(row[name]) for row in rows])


x = column("threshold_GeV")
fig, ax = plt.subplots(figsize=(7.2, 4.8))
ax.errorbar(x, column("residual_kHz"), fmt="o", color="black",
            label="data minus folded inclusive electrons")
ax.errorbar(x, column("wiser_pi0_incident_kHz"),
            yerr=column("wiser_pi0_stat_kHz"), fmt="s-", color="#d95f02",
            label="Wiser low-Q2 pi0 (incident energy)")
ax.plot(x, column("high_Q2_SIDIS_pi0_kHz"), "^-", color="#1b9e77",
        label="high-Q2 SIDIS pi0 (incident energy)")
ax.plot(x, column("exclusive_pi0_kHz"), "v-", color="#7570b3",
        label="exclusive real-photon pi0 (incident energy)")
ax.set(xlabel="NPS energy threshold (GeV)", ylabel="rate (kHz)",
       title="Run 6887: target-produced neutral-pion backgrounds")
ax.set_yscale("log")
ax.grid(alpha=0.25, which="both")
ax.legend(frameon=False, fontsize=9)
fig.tight_layout()
out = HERE.parent / "plots" / "run6887_background_channel_pilot.pdf"
out.parent.mkdir(parents=True, exist_ok=True)
fig.savefig(out)
print(out)
