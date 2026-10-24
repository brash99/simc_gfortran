#!/usr/bin/env python3
"""Subtract in-gate accidentals from the run-6151 prompt nonelastic yield."""

from __future__ import annotations

import csv
from pathlib import Path


ROOT = Path(__file__).resolve().parent
ELAPSED_S = 1543.330862
WINDOW_S = 20e-9
RETENTION = {
    3.4: 33494 / 38274,
    3.6: 31669 / 38274,
    3.9: 25298 / 38274,
}

with (ROOT / "nps_run6151_trigger_rates_snapshots.csv").open() as stream:
    snapshots = list(csv.DictReader(stream))
with (ROOT / "prompt_inelastic_run6151.csv").open() as stream:
    prompt_rows = list(csv.DictReader(stream))


def integrated_accidentals(retention: float) -> float:
    total = 0.0
    for index, row in enumerate(snapshots):
        start = float(row["time_s"])
        stop = (float(snapshots[index + 1]["time_s"])
                if index + 1 < len(snapshots) else ELAPSED_S)
        duration = max(0.0, stop - start)
        total += (float(row["hTRIG1_rate_Hz"]) * retention
                  * float(row["hTRIG3_rate_Hz"]) * WINDOW_S * duration)
    return total


output_rows = []
for row in prompt_rows:
    threshold = float(row["threshold_GeV"])
    accidental = integrated_accidentals(RETENTION[threshold])
    raw_nonelastic = float(row["prompt_excess_inelastic"])
    elastic = float(row["prompt_excess_elastic"])
    corrected = raw_nonelastic - accidental
    output_rows.append({
        "threshold_GeV": threshold,
        "raw_prompt_nonelastic_events": raw_nonelastic,
        "integrated_accidental_events_20ns": accidental,
        "corrected_correlated_nonelastic_events": corrected,
        "prompt_elastic_events": elastic,
        "corrected_nonelastic_to_elastic_ratio": corrected / elastic,
        "recorded_corrected_nonelastic_rate_Hz": corrected / ELAPSED_S,
    })

output = ROOT / "prompt_inelastic_run6151_corrected.csv"
with output.open("w", newline="") as stream:
    writer = csv.DictWriter(stream, fieldnames=output_rows[0])
    writer.writeheader()
    writer.writerows(output_rows)

for row in output_rows:
    print(
        f"{row['threshold_GeV']:.1f} GeV: nonelastic={row['corrected_correlated_nonelastic_events']:.1f}, "
        f"ratio={100*row['corrected_nonelastic_to_elastic_ratio']:.2f}%, "
        f"recorded rate={row['recorded_corrected_nonelastic_rate_Hz']:.3f} Hz"
    )
