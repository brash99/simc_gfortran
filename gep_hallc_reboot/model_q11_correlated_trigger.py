#!/usr/bin/env python3
"""Project the Q2=11 correlated pi0 trigger rate with model variations."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path

import numpy as np

from analyze_pi0_elastic_cuts import read_simc_binary


MP_GEV = 0.9382720813
S5_GEV2 = MP_GEV**2 + 2.0 * MP_GEV * 5.0


def interpolate(x: np.ndarray, y: np.ndarray, value: float) -> float:
    if value < x[0] or value > x[-1]:
        raise ValueError(f"interpolation point {value} outside [{x[0]}, {x[-1]}]")
    return float(np.interp(value, x, y))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("background", type=Path)
    parser.add_argument(
        "--elastic-scan",
        type=Path,
        default=Path(__file__).with_name("gep_reboot_q11_trigger_sum.csv"),
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path(__file__).with_name("q11_correlated_trigger_projection.csv"),
    )
    parser.add_argument("--baseline-background-yield", type=float, default=13.352045)
    parser.add_argument("--beam-current-ua", type=float, default=75.0)
    parser.add_argument("--elastic-central-energy-gev", type=float, default=4.838)
    parser.add_argument(
        "--thresholds-gev", type=float, nargs="+", default=(3.4, 3.6, 3.9)
    )
    args = parser.parse_args()

    background = read_simc_binary(args.background)
    weight = background["Weight"]
    egamma = background["phot_Egamma"]
    s_gev2 = MP_GEV**2 + 2.0 * MP_GEV * egamma
    pivot_ratio = s_gev2 / S5_GEV2

    valid_cluster = {}
    cluster_energy = {}
    for suffix in ("1", "2"):
        dx = background[f"phot_dx{suffix}"]
        dy = background[f"phot_dy{suffix}"]
        valid_cluster[suffix] = (
            np.isfinite(dx)
            & np.isfinite(dy)
            & (np.abs(dx) < 1.0e9)
            & (np.abs(dy) < 1.0e9)
        )
        cluster_energy[suffix] = background[f"Egamma{suffix}"] / 1000.0

    with args.elastic_scan.open(newline="") as stream:
        elastic_rows = list(csv.DictReader(stream))
    threshold_fraction = np.array(
        [float(row["threshold_fraction"]) for row in elastic_rows]
    )
    elastic_yield = np.array(
        [float(row["inclusive_3x3_yield_events_per_mC"]) for row in elastic_rows]
    )

    output_rows = []
    for threshold in args.thresholds_gev:
        trigger_pass = np.zeros(len(weight), dtype=bool)
        for suffix in ("1", "2"):
            trigger_pass |= valid_cluster[suffix] & (cluster_energy[suffix] >= threshold)

        elastic_at_threshold = interpolate(
            threshold_fraction,
            elastic_yield,
            threshold / args.elastic_central_energy_gev,
        )
        for exponent in (6.0, 7.0, 8.0):
            # Stored weights use n=7. The generator pivots exponent variations
            # at the 5 GeV invariant mass: w_n/w_7=(s/s5)^(-(n-7)).
            variation = pivot_ratio ** (-(exponent - 7.0))
            varied_weight = weight * variation
            accepted_yield = args.baseline_background_yield * float(
                np.sum(varied_weight) / np.sum(weight)
            )
            threshold_yield = args.baseline_background_yield * float(
                np.sum(varied_weight[trigger_pass]) / np.sum(weight)
            )
            threshold_efficiency = float(
                np.sum(varied_weight[trigger_pass]) / np.sum(varied_weight)
            )
            rate_hz = threshold_yield * args.beam_current_ua / 1000.0
            elastic_rate_hz = elastic_at_threshold * args.beam_current_ua / 1000.0
            output_rows.append(
                {
                    "threshold_GeV": threshold,
                    "scaling_exponent": exponent,
                    "transport_accepted_pi0_yield_events_per_mC": accepted_yield,
                    "generated_photon_threshold_efficiency": threshold_efficiency,
                    "pi0_trigger_yield_events_per_mC": threshold_yield,
                    "pi0_trigger_rate_75uA_Hz": rate_hz,
                    "elastic_3x3_yield_events_per_mC": elastic_at_threshold,
                    "elastic_3x3_rate_75uA_Hz": elastic_rate_hz,
                    "pi0_to_elastic_trigger_ratio": threshold_yield
                    / elastic_at_threshold,
                }
            )

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=output_rows[0].keys())
        writer.writeheader()
        writer.writerows(output_rows)

    for row in output_rows:
        print(
            f"threshold={row['threshold_GeV']:.1f} GeV "
            f"n={row['scaling_exponent']:.0f} "
            f"pi0={row['pi0_trigger_rate_75uA_Hz']:.4f} Hz "
            f"elastic={row['elastic_3x3_rate_75uA_Hz']:.4f} Hz "
            f"ratio={100.0*row['pi0_to_elastic_trigger_ratio']:.2f}%"
        )


if __name__ == "__main__":
    main()
