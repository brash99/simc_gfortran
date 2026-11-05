#!/usr/bin/env python3
"""Combine weighted phase-space event weights with NPS GEANT4 response."""

from __future__ import annotations

import argparse
import csv
import math
from pathlib import Path

import numpy as np


DEFAULT_THRESHOLDS = (3.387, 3.629, 3.870)
def integral(weight: np.ndarray, mask: np.ndarray,
             trials: int | None = None) -> tuple[float, float]:
    selected = weight[mask]
    total = float(selected.sum())
    sum_squares = float(np.square(selected).sum())
    variance = (sum_squares if trials is None else
                trials / (trials - 1.0) * max(
                    0.0, sum_squares - total * total / trials))
    return total, math.sqrt(variance)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("manifest", type=Path)
    parser.add_argument("response", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument(
        "--trials", type=int,
        help="Optional original generator trial count for finite-sample "
             "variance subtraction (for example, Wiser samples)")
    parser.add_argument(
        "--thresholds", type=float, nargs="+", default=DEFAULT_THRESHOLDS,
        help="Cluster-energy thresholds in GeV")
    args = parser.parse_args()
    with args.manifest.open(newline="") as stream:
        manifest = list(csv.DictReader(stream))
    with args.response.open(newline="") as stream:
        response = list(csv.DictReader(stream))
    if len(manifest) != len(response):
        raise RuntimeError(
            f"manifest/response length mismatch: {len(manifest)} != {len(response)}")
    for index, (source, detector) in enumerate(zip(manifest, response)):
        if int(source["entry"]) != index or int(detector["entry"]) != index:
            raise RuntimeError(f"entry alignment failure at {index}")
    weight = np.array([float(row["event_weight_Hz"]) for row in manifest])
    raw = np.array([float(row["raw_3x3_energy_GeV"]) for row in response])
    corrected = np.array([
        float(row["corrected_3x3_energy_GeV"]) for row in response
    ])
    rows = []
    for threshold in args.thresholds:
        raw_rate, raw_error = integral(weight, raw >= threshold, args.trials)
        corrected_rate, corrected_error = integral(
            weight, corrected >= threshold, args.trials)
        rows.append({
            "threshold_GeV": threshold,
            "raw_3x3_rate_Hz": raw_rate,
            "raw_3x3_mc_error_Hz": raw_error,
            "corrected_3x3_rate_Hz": corrected_rate,
            "corrected_3x3_mc_error_Hz": corrected_error,
        })
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=rows[0].keys(),
                                lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)
    for row in rows:
        print(
            f"{row['threshold_GeV']:.3f} GeV: "
            f"raw={row['raw_3x3_rate_Hz']/1000:.4f} +/- "
            f"{row['raw_3x3_mc_error_Hz']/1000:.4f} kHz; "
            f"corrected={row['corrected_3x3_rate_Hz']/1000:.4f} +/- "
            f"{row['corrected_3x3_mc_error_Hz']/1000:.4f} kHz"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

