#!/usr/bin/env python3
"""Fixed-seed bucket simulation of run-6151 NPS--HMS accidentals."""

import argparse
import csv
import math
import random


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--nps-rate-hz", type=float, default=24565.8087)
    parser.add_argument("--hms-rate-hz", type=float, default=233.8063538)
    parser.add_argument("--bucket-ns", type=float, default=4.0)
    parser.add_argument("--window-ns", type=float, default=20.0)
    parser.add_argument("--hms-trials", type=int, default=10_000_000)
    parser.add_argument("--seed", type=int, default=6151401)
    parser.add_argument(
        "--output", default="gep_hallc_reboot/run6151_accidental_simulation.csv"
    )
    args = parser.parse_args()

    buckets = round(args.window_ns / args.bucket_ns)
    if not math.isclose(buckets * args.bucket_ns, args.window_ns):
        raise ValueError("window must contain an integer number of beam buckets")

    bucket_rate_hz = 1.0e9 / args.bucket_ns
    mean_nps_per_bucket = args.nps_rate_hz / bucket_rate_hz
    # Poisson probability for at least one independent NPS candidate in the
    # buckets covered by the coincidence gate.
    coincidence_probability = 1.0 - math.exp(-buckets * mean_nps_per_bucket)
    analytic_rate_hz = args.hms_rate_hz * coincidence_probability
    continuous_limit_hz = (
        args.nps_rate_hz * args.hms_rate_hz * args.window_ns * 1.0e-9
    )

    rng = random.Random(args.seed)
    coincidences = sum(
        rng.random() < coincidence_probability for _ in range(args.hms_trials)
    )
    simulated_probability = coincidences / args.hms_trials
    simulated_rate_hz = args.hms_rate_hz * simulated_probability
    sigma_rate_hz = args.hms_rate_hz * math.sqrt(
        coincidence_probability * (1.0 - coincidence_probability)
        / args.hms_trials
    )

    with open(args.output, "w", newline="") as stream:
        writer = csv.writer(stream)
        writer.writerow(
            [
                "nps_rate_Hz",
                "hms_rate_Hz",
                "bucket_ns",
                "window_ns",
                "buckets",
                "hms_trials",
                "coincidences",
                "analytic_rate_Hz",
                "continuous_limit_Hz",
                "simulated_rate_Hz",
                "simulation_sigma_Hz",
            ]
        )
        writer.writerow(
            [
                args.nps_rate_hz,
                args.hms_rate_hz,
                args.bucket_ns,
                args.window_ns,
                buckets,
                args.hms_trials,
                coincidences,
                analytic_rate_hz,
                continuous_limit_hz,
                simulated_rate_hz,
                sigma_rate_hz,
            ]
        )

    print(f"buckets={buckets}")
    print(f"coincidence_probability_per_HMS={coincidence_probability:.12g}")
    print(f"analytic_rate_Hz={analytic_rate_hz:.12g}")
    print(f"continuous_limit_Hz={continuous_limit_hz:.12g}")
    print(f"simulated_rate_Hz={simulated_rate_hz:.12g}")
    print(f"simulation_sigma_Hz={sigma_rate_hz:.12g}")


if __name__ == "__main__":
    main()
