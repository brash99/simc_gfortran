#!/usr/bin/env python3
"""Analyze the run-6887 SIMC SIDIS pi0 pilot in its valid model region."""

from __future__ import annotations

import argparse
import csv
import math
import re
from pathlib import Path

import ROOT


def report_value(text: str, label: str) -> float:
    match = re.search(rf"{re.escape(label)}\s*=\s*([+-]?[0-9.]+(?:E[+-]?[0-9]+)?)",
                      text, re.IGNORECASE)
    if not match:
        raise RuntimeError(f"could not find {label!r} in SIMC report")
    return float(match.group(1))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--current-ua", type=float, default=29.41712856)
    args = parser.parse_args()

    report = args.report.read_text()
    normfac = report_value(report, "normfac")
    ntried = int(report_value(report, "Ntried"))
    # For a fixed-trial (negative-ngen) run SIMC reports yield as
    # normfac/ntried times the sum of stored main%weight values.
    event_scale = normfac / ntried
    hz_per_event_weight = event_scale * args.current_ua * 1.0e-3

    root_file = ROOT.TFile.Open(str(args.root))
    tree = root_file.Get("h10")
    if not tree:
        raise RuntimeError("missing h10 tree")

    thresholds = [0.0, 3387.0, 3629.0, 3870.0]
    sum_w = [0.0] * len(thresholds)
    sum_w2 = [0.0] * len(thresholds)
    counts = [0] * len(thresholds)
    nonfinite = 0
    outside_model = 0

    for event in tree:
        weight = float(event.Weight)
        if not math.isfinite(weight):
            nonfinite += 1
            continue

        # Reproduce the SIMC population cuts used in its integrated yield.
        pass_population = (
            -90.0 < event.hsdelta < 160.0
            and -0.350 < event.hsyptar < 0.400
            and -0.300 < event.hsxptar < 0.300
            and -70.0 < event.ssdelta < 80.0
            and -0.120 < event.ssyptar < 0.120
            and -0.140 < event.ssxptar < 0.140
        )
        if not pass_population or weight <= 0.0:
            continue
        if event.Q2 <= 1.0 or event.W <= 2.0:
            outside_model += 1
            continue

        face_energy = 0.0
        if event.xcal_gamma1 > -1.0e9:
            face_energy += event.Egamma1
        if event.xcal_gamma2 > -1.0e9:
            face_energy += event.Egamma2
        for index, threshold in enumerate(thresholds):
            if face_energy > threshold:
                counts[index] += 1
                sum_w[index] += weight
                sum_w2[index] += weight * weight

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", newline="") as stream:
        writer = csv.writer(stream)
        writer.writerow(("threshold_GeV", "accepted_events", "rate_Hz",
                         "mc_stat_error_Hz"))
        for threshold, count, total, total2 in zip(thresholds, counts, sum_w, sum_w2):
            # Zeros from all unrecorded trials are part of the MC estimator.
            variance_sum = ntried / (ntried - 1.0) * max(
                0.0, total2 - total * total / ntried)
            writer.writerow((threshold / 1000.0, count,
                             total * hz_per_event_weight,
                             math.sqrt(variance_sum) * hz_per_event_weight))

    print(f"Trials: {ntried}")
    print(f"Stored events with non-finite weight: {nonfinite}")
    print(f"Finite positive events outside Q2>1, W>2: {outside_model}")
    for threshold, total, total2 in zip(thresholds, sum_w, sum_w2):
        variance_sum = ntried / (ntried - 1.0) * max(
            0.0, total2 - total * total / ntried)
        print(f"Eface > {threshold / 1000.0:.3f} GeV: "
              f"{total * hz_per_event_weight:.3f} +/- "
              f"{math.sqrt(variance_sum) * hz_per_event_weight:.3f} Hz")
    print(f"Output: {args.output}")


if __name__ == "__main__":
    main()
