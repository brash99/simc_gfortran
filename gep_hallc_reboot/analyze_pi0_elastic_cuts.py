#!/usr/bin/env python3
"""Scan common elastic-correlation cuts with a parameterized NPS response."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path

import numpy as np


def read_record(stream):
    marker = stream.read(4)
    if not marker:
        return None
    (size,) = struct.unpack("=i", marker)
    payload = stream.read(size)
    trailer = stream.read(4)
    if len(payload) != size or len(trailer) != 4:
        raise ValueError("truncated Fortran sequential record")
    if struct.unpack("=i", trailer)[0] != size:
        raise ValueError("mismatched Fortran record markers")
    return payload


def read_simc_binary(path: Path):
    with path.open("rb") as stream:
        first = read_record(stream)
        if first is None:
            raise ValueError(f"empty SIMC file: {path}")
        (nbranch,) = struct.unpack("=i", first)
        tags = []
        for _ in range(nbranch):
            record = read_record(stream)
            if record is None:
                raise ValueError("truncated SIMC branch header")
            tags.append(record.decode("ascii").rstrip())
        columns = [[] for _ in tags]
        while True:
            first_value = read_record(stream)
            if first_value is None:
                break
            row = [struct.unpack("=d", first_value)[0]]
            for _ in range(1, nbranch):
                record = read_record(stream)
                if record is None:
                    raise ValueError("partial event at end of SIMC file")
                row.append(struct.unpack("=d", record)[0])
            for column, value in zip(columns, row):
                column.append(value)
    return {tag: np.asarray(column) for tag, column in zip(tags, columns)}


def robust_sigma(values):
    center = np.median(values)
    sigma = 1.4826 * np.median(np.abs(values - center))
    if not np.isfinite(sigma) or sigma <= 0:
        raise ValueError("cannot estimate a positive robust scale")
    return center, sigma


def fit_elastic_core(dx, dy, dp):
    cx, sx = robust_sigma(dx)
    cy, sy = robust_sigma(dy)
    cp, sp = robust_sigma(dp)
    center = np.array([cx, cy])
    covariance = np.diag([sx * sx, sy * sy])
    for _ in range(5):
        inverse = np.linalg.inv(covariance)
        delta = np.column_stack((dx, dy)) - center
        radius2 = np.einsum("ij,jk,ik->i", delta, inverse, delta)
        mask = (radius2 < 9.0) & (np.abs(dp - cp) < 3.0 * sp)
        if np.count_nonzero(mask) < 20:
            raise ValueError("too few events remain in elastic core fit")
        center = np.mean(np.column_stack((dx[mask], dy[mask])), axis=0)
        covariance = np.cov(np.column_stack((dx[mask], dy[mask])), rowvar=False)
        cp = float(np.mean(dp[mask]))
        sp = float(np.std(dp[mask], ddof=1))
    return center, covariance, cp, sp


def ellipse_radius2(dx, dy, center, covariance):
    delta = np.column_stack((dx, dy)) - center
    inverse = np.linalg.inv(covariance)
    return np.einsum("ij,jk,ik->i", delta, inverse, delta)


def weighted_fraction(mask, weights):
    denominator = np.sum(weights)
    if denominator <= 0:
        raise ValueError("non-positive total event weight")
    return float(np.sum(weights[mask]) / denominator)


def smear_energy(true_energy_gev, stochastic, constant, rng):
    """Apply sigma(E)/E = stochastic/sqrt(E/GeV) (+) constant."""
    true_energy_gev = np.asarray(true_energy_gev, dtype=float)
    if stochastic == 0.0 and constant == 0.0:
        return true_energy_gev.copy()
    relative_sigma = np.sqrt(
        stochastic**2 / np.maximum(true_energy_gev, 1.0e-12) + constant**2
    )
    measured = true_energy_gev * (1.0 + relative_sigma * rng.standard_normal(len(true_energy_gev)))
    return np.maximum(measured, 0.0)


def smear_position(values, sigma_cm, rng):
    values = np.asarray(values, dtype=float)
    if sigma_cm == 0.0:
        return values.copy()
    return values + sigma_cm * rng.standard_normal(len(values))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("signal", type=Path)
    parser.add_argument("background", type=Path)
    parser.add_argument("--signal-yield", type=float, default=41.481955)
    parser.add_argument("--background-yield", type=float, default=13.352045)
    parser.add_argument("--signal-central-energy-gev", type=float, default=4.838)
    parser.add_argument("--nps-position-sigma-cm", type=float, default=0.5)
    parser.add_argument("--nps-stochastic", type=float, default=0.0)
    parser.add_argument("--nps-constant", type=float, default=0.0)
    parser.add_argument(
        "--thresholds-gev",
        type=float,
        nargs="+",
        default=(0.0, 1.0, 2.0, 2.4, 3.0, 3.5, 4.0, 4.2, 4.4, 4.6),
    )
    parser.add_argument("--response-seed", type=int, default=20261022)
    args = parser.parse_args()

    if args.nps_position_sigma_cm < 0.0:
        parser.error("--nps-position-sigma-cm must be non-negative")
    if args.nps_stochastic < 0.0 or args.nps_constant < 0.0:
        parser.error("energy-resolution terms must be non-negative")
    if any(threshold < 0.0 for threshold in args.thresholds_gev):
        parser.error("thresholds must be non-negative")

    signal = read_simc_binary(args.signal)
    background = read_simc_binary(args.background)
    rng = np.random.default_rng(args.response_seed)

    signal_dx = smear_position(signal["el_dx"], args.nps_position_sigma_cm, rng)
    signal_dy = smear_position(signal["el_dy"], args.nps_position_sigma_cm, rng)
    background_dx = {}
    background_dy = {}
    background_energy = {}
    for suffix in ("1", "2"):
        background_dx[suffix] = smear_position(
            background[f"phot_dx{suffix}"], args.nps_position_sigma_cm, rng
        )
        background_dy[suffix] = smear_position(
            background[f"phot_dy{suffix}"], args.nps_position_sigma_cm, rng
        )
        background_energy[suffix] = smear_energy(
            background[f"Egamma{suffix}"] / 1000.0,
            args.nps_stochastic,
            args.nps_constant,
            rng,
        )
    signal_energy = smear_energy(
        args.signal_central_energy_gev * (1.0 + signal["ssdelta"] / 100.0),
        args.nps_stochastic,
        args.nps_constant,
        rng,
    )

    center, covariance, dp_center, dp_sigma = fit_elastic_core(
        signal_dx, signal_dy, signal["el_dpelastic"]
    )
    signal_r2 = ellipse_radius2(signal_dx, signal_dy, center, covariance)

    sentinel = 1.0e9
    background_r2 = np.full(len(background["Weight"]), np.inf)
    background_r2_by_cluster = {}
    for suffix in ("1", "2"):
        dx = background_dx[suffix]
        dy = background_dy[suffix]
        valid = np.isfinite(dx) & np.isfinite(dy) & (np.abs(dx) < sentinel) & (
            np.abs(dy) < sentinel
        )
        trial = np.full(len(dx), np.inf)
        trial[valid] = ellipse_radius2(dx[valid], dy[valid], center, covariance)
        background_r2_by_cluster[suffix] = trial
        background_r2 = np.minimum(background_r2, trial)

    print(f"signal events: {len(signal['Weight'])}")
    print(f"background events: {len(background['Weight'])}")
    print(f"position center (cm): {center[0]:.6f} {center[1]:.6f}")
    print("position covariance (cm^2):")
    print(f"  {covariance[0,0]:.8f} {covariance[0,1]:.8f}")
    print(f"  {covariance[1,0]:.8f} {covariance[1,1]:.8f}")
    print(f"dp center, sigma (% p0): {dp_center:.6f} {dp_sigma:.6f}")
    print(
        "response: position_sigma_cm="
        f"{args.nps_position_sigma_cm:g} stochastic={args.nps_stochastic:g} "
        f"constant={args.nps_constant:g} seed={args.response_seed}"
    )
    print("threshold_gev width signal_eff background_eff signal_per_mC background_per_mC contamination")
    for threshold in args.thresholds_gev:
        signal_threshold = signal_energy >= threshold
        background_threshold_r2 = np.full(len(background["Weight"]), np.inf)
        for suffix in ("1", "2"):
            passes = background_energy[suffix] >= threshold
            background_threshold_r2[passes] = np.minimum(
                background_threshold_r2[passes], background_r2_by_cluster[suffix][passes]
            )
        for width in (3.0, 6.0):
            signal_mask = signal_threshold & (signal_r2 < width * width) & (
                np.abs(signal["el_dpelastic"] - dp_center) < width * dp_sigma
            )
            background_mask = (background_threshold_r2 < width * width) & (
                np.abs(background["phot_dpelastic"] - dp_center) < width * dp_sigma
            )
            signal_eff = weighted_fraction(signal_mask, signal["Weight"])
            background_eff = weighted_fraction(background_mask, background["Weight"])
            signal_yield = args.signal_yield * signal_eff
            background_yield = args.background_yield * background_eff
            denominator = signal_yield + background_yield
            contamination = background_yield / denominator if denominator > 0.0 else 0.0
            print(
                f"{threshold:.6g} {width:.0f} {signal_eff:.8f} {background_eff:.8f} "
                f"{signal_yield:.8f} {background_yield:.8f} {contamination:.8f}"
            )


if __name__ == "__main__":
    main()
