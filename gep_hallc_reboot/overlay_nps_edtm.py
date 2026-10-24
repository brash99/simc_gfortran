#!/usr/bin/env python3
"""Overlay random-trigger NPS pulse packets on simulated elastic 3x3 sums.

The run-6151 EDTM events are independent detector readout windows.  A 75-uA
window is approximated by superposing I_target/<I_EDTM> randomly selected
packets (integer part plus one Bernoulli packet for the fractional part).
"""

from __future__ import annotations

import csv
from collections import defaultdict
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

ROOT = Path(__file__).resolve().parent
RNG_SEED = 615175
REPLICAS = 100
TARGET_CURRENT_UA = 75.0
SIMC_YIELD = 41.341739
TIME_CENTER_NS = 162.4
GEV_PER_MV = 0.01278389994
MEAN_LIBRARY_CURRENT_UA = 25.98825366


def read_rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="") as stream:
        return list(csv.DictReader(stream))


sim = read_rows(ROOT / "gep_reboot_q11_trigger_events.csv")
weights = np.asarray([float(row["weight"]) for row in sim])
energies = np.asarray([float(row["primary_energy_GeV"]) for row in sim])
base = np.asarray([float(row["sum3x3_fraction"]) for row in sim])
seed_rows = np.asarray([int(row["seed_vertical"]) for row in sim])
# GEANT numbers horizontal crystals from +x to -x; hcana columns run -x to +x.
seed_cols = np.asarray([29 - int(row["seed_horizontal"]) for row in sim])
fiducial = np.asarray([
    -140.0 <= float(row["local_x_mm"]) <= 303.66
    and abs(float(row["local_y_mm"])) <= 368.73 for row in sim
])

packets: dict[int, list[tuple[int, float, float]]] = defaultdict(list)
packet_ids: list[int] = []
seen: set[int] = set()
for row in read_rows(ROOT / "nps_edtm_overlay_6151_22p5_30uA.csv"):
    event = int(row["source_entry"])
    if event not in seen:
        seen.add(event)
        packet_ids.append(event)
    block = int(row["block"])
    if block >= 0:
        packets[event].append((block, float(row["amplitude_mV"]),
                               float(row["pulse_time_ns"])))

packet_ids = np.asarray(packet_ids)
current_ratio = TARGET_CURRENT_UA / MEAN_LIBRARY_CURRENT_UA
n_full = int(np.floor(current_ratio))
fractional = current_ratio - n_full

scenarios = [
    ("No overlay", 0.0, np.inf),
    ("|dt|<2 ns, all decoded", 2.0, 0.0),
    ("|dt|<4 ns, all decoded", 4.0, 0.0),
    ("|dt|<4 ns, amp>=5 mV", 4.0, 5.0),
    ("|dt|<4 ns, amp>=10 mV", 4.0, 10.0),
]
thresholds = (0.7, 0.8)
results: list[dict[str, float | str]] = []

for name, time_half_width, amp_min in scenarios:
    # Reuse identical packet draws so nested cuts can be compared event by event.
    rng = np.random.default_rng(RNG_SEED)
    efficiencies = {threshold: [] for threshold in thresholds}
    rescued = {threshold: [] for threshold in thresholds}
    overlap = []
    added_mev_all = []
    for _ in range(REPLICAS):
        added_gev = np.zeros(len(sim))
        overlapped = np.zeros(len(sim), dtype=bool)
        if time_half_width > 0.0:
            multiplicities = n_full + (rng.random(len(sim)) < fractional)
            for event_index, multiplicity in enumerate(multiplicities):
                for packet_id in rng.choice(packet_ids, size=multiplicity, replace=True):
                    for block, amplitude, pulse_time in packets[int(packet_id)]:
                        row, col = divmod(block, 30)
                        if (abs(row - seed_rows[event_index]) <= 1
                                and abs(col - seed_cols[event_index]) <= 1
                                and abs(pulse_time - TIME_CENTER_NS) < time_half_width
                                and amplitude >= amp_min):
                            added_gev[event_index] += amplitude * GEV_PER_MV
                            overlapped[event_index] = True
        response = base + added_gev / energies
        overlap.append(np.sum(weights[fiducial & overlapped]) / np.sum(weights[fiducial]))
        if np.any(fiducial & overlapped):
            added_mev_all.extend(1000.0 * added_gev[fiducial & overlapped])
        for threshold in thresholds:
            passed = fiducial & (response >= threshold)
            base_passed = fiducial & (base >= threshold)
            efficiencies[threshold].append(np.sum(weights[passed]) / np.sum(weights))
            rescued[threshold].append(np.sum(weights[passed & ~base_passed]) / np.sum(weights))
    for threshold in thresholds:
        values = np.asarray(efficiencies[threshold])
        promoted = np.asarray(rescued[threshold])
        results.append({
            "scenario": name,
            "threshold_fraction": threshold,
            "mean_overlap_fraction_within_fiducial": float(np.mean(overlap)),
            "mean_added_energy_MeV_given_overlap":
                float(np.mean(added_mev_all)) if added_mev_all else 0.0,
            "mean_combined_efficiency": float(np.mean(values)),
            "replica_rms_efficiency": float(np.std(values)),
            "mean_rescued_fraction": float(np.mean(promoted)),
            "yield_events_per_mC": float(SIMC_YIELD * np.mean(values)),
        })

with (ROOT / "nps_edtm_overlay_75uA.csv").open("w", newline="") as stream:
    writer = csv.DictWriter(stream, fieldnames=results[0].keys())
    writer.writeheader()
    writer.writerows(results)

names = [scenario[0] for scenario in scenarios]
x = np.arange(len(names))
fig, axes = plt.subplots(1, 2, figsize=(13.5, 5.6))
overlap_values = [100.0 * next(row["mean_overlap_fraction_within_fiducial"]
                               for row in results if row["scenario"] == name)
                  for name in names]
axes[0].plot(x, overlap_values, "o-", color="tab:blue")
axes[0].set_title("Random-window pulses entering shower 3x3")
axes[0].set_ylabel("Weighted fiducial event fraction (%)")
axes[0].set_xticks(x, names, rotation=24, ha="right")
axes[0].grid(axis="y", alpha=0.25)
selected = [row for row in results if row["threshold_fraction"] == thresholds[0]]
axes[1].plot(x, [row["mean_added_energy_MeV_given_overlap"] for row in selected],
             "s-", color="tab:orange")
axes[1].set_title("Added energy when an overlap occurs")
axes[1].set_ylabel("Mean inferred added energy (MeV)")
axes[1].set_xticks(x, names, rotation=24, ha="right")
axes[1].grid(axis="y", alpha=0.25)
fig.suptitle(f"Run 6151 EDTM pulse overlay scaled to {TARGET_CURRENT_UA:.0f} microampere")
fig.tight_layout()
fig.savefig(ROOT / "plots/nps_edtm_overlay_75uA.png", dpi=180)
fig.savefig(ROOT / "plots/nps_edtm_overlay_75uA.pdf")

print(f"EDTM packets={len(packet_ids)}, current ratio={current_ratio:.5f}, "
      f"sampling={n_full}+Bernoulli({fractional:.5f})")
for row in results:
    print(f"{row['scenario']}, threshold={row['threshold_fraction']:.1f}: "
          f"overlap={100*row['mean_overlap_fraction_within_fiducial']:.5f}% "
          f"added|overlap={row['mean_added_energy_MeV_given_overlap']:.1f} MeV "
          f"eff={100*row['mean_combined_efficiency']:.5f}% "
          f"rescued={100*row['mean_rescued_fraction']:.5f}% "
          f"yield={row['yield_events_per_mC']:.5f} events/mC")
