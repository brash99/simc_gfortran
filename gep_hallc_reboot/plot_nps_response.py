#!/usr/bin/env python3
"""Make static diagnostic figures for the parameterized NPS response scan."""

from __future__ import annotations

import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.colors import LogNorm

from analyze_pi0_elastic_cuts import (
    ellipse_radius2,
    fit_elastic_core,
    read_simc_binary,
    smear_energy,
    smear_position,
    weighted_fraction,
)


def response_samples(signal, background, args):
    rng = np.random.default_rng(args.response_seed)
    signal_dx = smear_position(signal["el_dx"], args.position_sigma_cm, rng)
    signal_dy = smear_position(signal["el_dy"], args.position_sigma_cm, rng)

    background_dx = {}
    background_dy = {}
    background_energy = {}
    for suffix in ("1", "2"):
        background_dx[suffix] = smear_position(
            background[f"phot_dx{suffix}"], args.position_sigma_cm, rng
        )
        background_dy[suffix] = smear_position(
            background[f"phot_dy{suffix}"], args.position_sigma_cm, rng
        )
        background_energy[suffix] = smear_energy(
            background[f"Egamma{suffix}"] / 1000.0,
            args.stochastic,
            args.constant,
            rng,
        )
    signal_energy = smear_energy(
        args.signal_central_energy_gev * (1.0 + signal["ssdelta"] / 100.0),
        args.stochastic,
        args.constant,
        rng,
    )

    center, covariance, dp_center, dp_sigma = fit_elastic_core(
        signal_dx, signal_dy, signal["el_dpelastic"]
    )
    signal_r2 = ellipse_radius2(signal_dx, signal_dy, center, covariance)

    sentinel = 1.0e9
    background_r2 = {}
    for suffix in ("1", "2"):
        dx = background_dx[suffix]
        dy = background_dy[suffix]
        valid = (
            np.isfinite(dx)
            & np.isfinite(dy)
            & (np.abs(dx) < sentinel)
            & (np.abs(dy) < sentinel)
        )
        trial = np.full(len(dx), np.inf)
        trial[valid] = ellipse_radius2(dx[valid], dy[valid], center, covariance)
        background_r2[suffix] = trial

    return {
        "signal_energy": signal_energy,
        "signal_r2": signal_r2,
        "background_energy": background_energy,
        "background_r2": background_r2,
        "dp_center": dp_center,
        "dp_sigma": dp_sigma,
    }


def threshold_scan(signal, background, response, args):
    rows = []
    for threshold in args.thresholds:
        signal_threshold = response["signal_energy"] >= threshold
        background_threshold_r2 = np.full(len(background["Weight"]), np.inf)
        for suffix in ("1", "2"):
            passes = response["background_energy"][suffix] >= threshold
            background_threshold_r2[passes] = np.minimum(
                background_threshold_r2[passes],
                response["background_r2"][suffix][passes],
            )
        for width in (3.0, 6.0):
            signal_mask = signal_threshold & (
                response["signal_r2"] < width * width
            ) & (
                np.abs(signal["el_dpelastic"] - response["dp_center"])
                < width * response["dp_sigma"]
            )
            background_mask = (background_threshold_r2 < width * width) & (
                np.abs(background["phot_dpelastic"] - response["dp_center"])
                < width * response["dp_sigma"]
            )
            signal_eff = weighted_fraction(signal_mask, signal["Weight"])
            background_eff = weighted_fraction(background_mask, background["Weight"])
            signal_yield = args.signal_yield * signal_eff
            background_yield = args.background_yield * background_eff
            rows.append(
                (
                    threshold,
                    width,
                    signal_eff,
                    background_eff,
                    background_yield / (signal_yield + background_yield),
                )
            )
    return np.asarray(rows)


def save_figure(fig, output_dir, stem):
    for suffix in ("png", "pdf"):
        fig.savefig(output_dir / f"{stem}.{suffix}", dpi=220, bbox_inches="tight")
    plt.close(fig)


def plot_threshold_tradeoff(rows, output_dir):
    fig, axes = plt.subplots(2, 1, figsize=(7.2, 7.4), sharex=True)
    styles = {3.0: ("o", "tab:blue"), 6.0: ("s", "tab:orange")}
    for width in (3.0, 6.0):
        subset = rows[rows[:, 1] == width]
        marker, color = styles[width]
        label = rf"{width:.0f}$\sigma$ elastic cuts"
        axes[0].plot(
            subset[:, 0], 100.0 * subset[:, 2], marker=marker, color=color,
            linewidth=2, markersize=5, label=label,
        )
        axes[1].plot(
            subset[:, 0], 100.0 * subset[:, 4], marker=marker, color=color,
            linewidth=2, markersize=5, label=label,
        )
    for axis in axes:
        axis.axvspan(4.0, 4.4, color="0.7", alpha=0.25, zorder=0)
        axis.grid(alpha=0.25)
        axis.legend(frameon=False)
    axes[0].set_ylabel("Elastic efficiency (%)")
    axes[1].set_ylabel(r"Selected $\pi^0$ fraction (%)")
    axes[1].set_xlabel("NPS cluster threshold (GeV)")
    axes[0].set_ylim(60, 92)
    axes[1].set_ylim(bottom=0)
    axes[0].set_title(
        "5 mm position smearing; no assumed energy smearing",
        fontsize=10,
        pad=8,
    )
    axes[0].text(
        4.2, 61.2, "threshold turn-on\nregion", ha="center", va="bottom",
        color="0.35",
    )
    fig.suptitle("NPS threshold tradeoff at the Q² ≈ 11 GeV² setting", y=0.995)
    fig.tight_layout()
    save_figure(fig, output_dir, "nps_threshold_tradeoff")


def weighted_density(values, weights, bins):
    hist, edges = np.histogram(values, bins=bins, weights=weights, density=True)
    centers = 0.5 * (edges[1:] + edges[:-1])
    return centers, hist


def plot_residuals(signal, background, response, output_dir):
    best = np.where(
        response["background_r2"]["1"] <= response["background_r2"]["2"],
        "1", "2",
    )
    background_best_r2 = np.minimum(
        response["background_r2"]["1"], response["background_r2"]["2"]
    )
    background_best_energy = np.where(
        best == "1",
        response["background_energy"]["1"],
        response["background_energy"]["2"],
    )
    valid = np.isfinite(background_best_r2)

    fig, axes = plt.subplots(1, 3, figsize=(13.2, 4.1))
    r_bins = np.linspace(0, 12, 97)
    dp_bins = np.linspace(-15, 15, 121)
    e_bins = np.linspace(0, 5.6, 90)
    comparisons = (
        (
            np.sqrt(response["signal_r2"]), signal["Weight"],
            np.sqrt(background_best_r2[valid]), background["Weight"][valid],
            r_bins, r"Position residual $r_{ellipse}$ ($\sigma$ units)",
        ),
        (
            (signal["el_dpelastic"] - response["dp_center"]) / response["dp_sigma"],
            signal["Weight"],
            (background["phot_dpelastic"][valid] - response["dp_center"])
            / response["dp_sigma"],
            background["Weight"][valid], dp_bins,
            r"Proton momentum residual ($\sigma$ units)",
        ),
        (
            response["signal_energy"], signal["Weight"],
            background_best_energy[valid], background["Weight"][valid],
            e_bins, "Measured cluster energy (GeV)",
        ),
    )
    for axis, (sx, sw, bx, bw, bins, xlabel) in zip(axes, comparisons):
        sc, sh = weighted_density(sx, sw, bins)
        bc, bh = weighted_density(bx, bw, bins)
        axis.step(sc, sh, where="mid", linewidth=2, color="tab:blue", label="Elastic")
        axis.step(bc, bh, where="mid", linewidth=2, color="tab:orange", label=r"$\pi^0$")
        axis.set_yscale("log")
        axis.set_xlabel(xlabel)
        axis.set_ylabel("Weighted probability density")
        axis.grid(alpha=0.2)
    axes[0].axvline(3, color="0.35", linestyle="--", linewidth=1)
    axes[0].axvline(6, color="0.35", linestyle=":", linewidth=1)
    axes[1].axvline(-3, color="0.35", linestyle="--", linewidth=1)
    axes[1].axvline(3, color="0.35", linestyle="--", linewidth=1)
    axes[2].axvspan(4.0, 4.4, color="0.7", alpha=0.25)
    axes[0].legend(frameon=False)
    fig.suptitle("Elastic and π⁰ observables after the parameterized NPS response")
    fig.tight_layout()
    save_figure(fig, output_dir, "nps_residual_distributions")


def plot_background_energy_position(background, response, output_dir):
    best_is_one = response["background_r2"]["1"] <= response["background_r2"]["2"]
    r2 = np.minimum(response["background_r2"]["1"], response["background_r2"]["2"])
    energy = np.where(
        best_is_one,
        response["background_energy"]["1"],
        response["background_energy"]["2"],
    )
    valid = np.isfinite(r2) & (np.sqrt(r2) < 12.0)
    histogram, xedges, yedges = np.histogram2d(
        energy[valid], np.sqrt(r2[valid]), bins=(80, 80),
        range=((0.0, 5.6), (0.0, 12.0)), weights=background["Weight"][valid],
    )
    histogram /= histogram.max()
    positive = histogram[histogram > 0]
    fig, axis = plt.subplots(figsize=(7.4, 5.2))
    mesh = axis.pcolormesh(
        xedges, yedges, histogram.T,
        norm=LogNorm(vmin=max(positive.min(), positive.max() * 1.0e-5), vmax=positive.max()),
        cmap="viridis", shading="auto",
    )
    axis.axhline(3, color="white", linestyle="--", linewidth=1.3, label=r"3$\sigma$ position cut")
    axis.axhline(6, color="white", linestyle=":", linewidth=1.5, label=r"6$\sigma$ position cut")
    axis.axvspan(4.0, 4.4, color="white", alpha=0.14)
    axis.set_xlabel(r"Best-position $\pi^0$ photon-cluster energy (GeV)")
    axis.set_ylabel(r"Position residual $r_{ellipse}$ ($\sigma$ units)")
    axis.set_title(r"Where the $\pi^0$ background approaches the elastic locus")
    colorbar = fig.colorbar(mesh, ax=axis)
    colorbar.set_label("Relative weighted event density")
    axis.legend(
        facecolor="white", edgecolor="0.8", framealpha=0.9,
        labelcolor="black", loc="upper left",
    )
    fig.tight_layout()
    save_figure(fig, output_dir, "nps_pi0_energy_position")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("signal", type=Path)
    parser.add_argument("background", type=Path)
    parser.add_argument("--output-dir", type=Path, default=Path("gep_hallc_reboot/plots"))
    parser.add_argument("--signal-yield", type=float, default=41.481955)
    parser.add_argument("--background-yield", type=float, default=13.352045)
    parser.add_argument("--signal-central-energy-gev", type=float, default=4.838)
    parser.add_argument("--position-sigma-cm", type=float, default=0.5)
    parser.add_argument("--stochastic", type=float, default=0.0)
    parser.add_argument("--constant", type=float, default=0.0)
    parser.add_argument("--response-seed", type=int, default=20261022)
    parser.add_argument(
        "--thresholds", type=float, nargs="+",
        default=(0.0, 1.0, 2.0, 2.4, 3.0, 3.5, 4.0, 4.2, 4.4, 4.6),
    )
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)

    signal = read_simc_binary(args.signal)
    background = read_simc_binary(args.background)
    response = response_samples(signal, background, args)
    rows = threshold_scan(signal, background, response, args)
    plot_threshold_tradeoff(rows, args.output_dir)
    plot_residuals(signal, background, response, args.output_dir)
    plot_background_energy_position(background, response, args.output_dir)
    for path in sorted(args.output_dir.glob("nps_*")):
        print(path)


if __name__ == "__main__":
    main()
