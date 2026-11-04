#!/usr/bin/env python3
"""Compare six NPS electron-run threshold curves with Born inelastic rates."""

from __future__ import annotations

import csv
import math
import os
import re
from pathlib import Path

os.environ.setdefault("MPLBACKEND", "Agg")
os.environ.setdefault("MPLCONFIGDIR", "/tmp/gep_nps_matplotlib")

import matplotlib.pyplot as plt
import numpy as np


HERE = Path(__file__).resolve().parent
RUNS = (6158, 6165, 6168, 2855, 5183, 3888)
THRESHOLD_FRACTIONS = (0.70, 0.75, 0.80)
PROTON_MASS_GEV = 0.9382720813
BEAM_ENERGY_GEV = {
    6158: 6.370,
    6165: 6.370,
    6168: 6.370,
    2855: 10.5394,
    5183: 10.5394,
    3888: 10.5394,
}


def elastic_endpoint(beam_energy: float, angle_deg: float) -> float:
    """Elastic scattered-electron energy at the NPS central angle."""
    angle = math.radians(angle_deg)
    return beam_energy / (
        1.0 + 2.0 * beam_energy / PROTON_MASS_GEV * math.sin(angle / 2.0) ** 2
    )


def read_measured(run: int) -> list[dict[str, float]]:
    path = HERE / f"nps_run{run}_100k_edtm_physical.csv"
    with path.open(newline="") as stream:
        return [
            {key: float(value) for key, value in row.items()}
            for row in csv.DictReader(stream)
        ]


def read_generated(run: int) -> tuple[np.ndarray, np.ndarray]:
    path = Path(f"/tmp/nps_run{run}_inclusive_1m.csv")
    energy: list[float] = []
    weight: list[float] = []
    with path.open(newline="") as stream:
        for row in csv.DictReader(stream):
            energy.append(float(row["electron_energy_GeV"]))
            weight.append(float(row["weight_Hz"]))
    return np.asarray(energy), np.asarray(weight)


def read_radiated_elastic(
    run: int, current_uA: float
) -> tuple[np.ndarray, np.ndarray, float]:
    histogram = HERE.parent / "outfiles" / f"nps_run{run}_elastic_singles.hist"
    match = re.search(r"wtcontr=\s*([0-9.Ee+-]+)", histogram.read_text())
    if not match:
        raise ValueError(f"missing wtcontr in {histogram}")
    yield_per_mC = float(match.group(1))

    events = HERE.parent / "worksim" / f"nps_run{run}_elastic_singles.hybrid.csv"
    energy: list[float] = []
    weight: list[float] = []
    with events.open(newline="") as stream:
        reader = csv.DictReader(line for line in stream if not line.startswith("#"))
        for row in reader:
            electron_energy = float(row["primary_E_GeV"])
            fsr_energy = float(row["rad2_GeV"]) if int(row["rad_tail"]) == 2 else 0.0
            energy.append(electron_energy + fsr_energy)
            weight.append(float(row["weight"]))
    energy_array = np.asarray(energy)
    weight_array = np.asarray(weight)
    raw_sum = float(weight_array.sum())
    if raw_sum <= 0.0:
        raise ValueError(f"no positive elastic weight for run {run}")
    rate_scale = yield_per_mC * (current_uA / 1000.0) / raw_sum
    return energy_array, weight_array * rate_scale, yield_per_mC


def cumulative_rates(
    energy: np.ndarray, weight: np.ndarray, thresholds: np.ndarray
) -> tuple[np.ndarray, np.ndarray]:
    order = np.argsort(energy)
    sorted_energy = energy[order]
    sorted_weight = weight[order]
    reverse_sum = np.cumsum(sorted_weight[::-1])[::-1]
    reverse_sum2 = np.cumsum(np.square(sorted_weight[::-1]))[::-1]
    indices = np.searchsorted(sorted_energy, thresholds, side="left")
    rates = np.zeros_like(thresholds)
    errors = np.zeros_like(thresholds)
    valid = indices < len(sorted_energy)
    rates[valid] = reverse_sum[indices[valid]]
    errors[valid] = np.sqrt(reverse_sum2[indices[valid]])
    return rates, errors


def log_interpolate(x: np.ndarray, y: np.ndarray, value: float) -> float:
    if value < x.min() or value > x.max():
        raise ValueError(
            f"threshold {value:.4f} GeV is outside measured scan "
            f"[{x.min():.4f}, {x.max():.4f}] GeV"
        )
    positive = y > 0.0
    if positive.sum() < 2:
        raise ValueError("at least two positive rate points are required")
    return float(math.exp(np.interp(value, x[positive], np.log(y[positive]))))


def main() -> None:
    fig, axes = plt.subplots(2, 3, figsize=(14.8, 8.5), sharex=True, sharey=True)
    output_rows: list[dict[str, float | int]] = []

    for axis, run in zip(axes.flat, RUNS):
        measured = read_measured(run)
        thresholds = np.asarray([row["physical_threshold_GeV"] for row in measured])
        measured_rate = np.asarray([row["primitive_rate_Hz"] for row in measured])
        measured_error = np.asarray([row["stat_error_Hz"] for row in measured])
        current = measured[0]["mean_selected_current_uA"]
        angle = measured[0]["nps_angle_deg"]
        endpoint = elastic_endpoint(BEAM_ENERGY_GEV[run], angle)

        generated_energy, generated_weight = read_generated(run)
        generated_rate, generated_error = cumulative_rates(
            generated_energy, generated_weight, thresholds
        )
        elastic_energy, elastic_weight, elastic_yield_per_mC = read_radiated_elastic(
            run, current
        )
        elastic_rate, elastic_error = cumulative_rates(
            elastic_energy, elastic_weight, thresholds
        )
        total_rate = generated_rate + elastic_rate
        total_error = np.hypot(generated_error, elastic_error)

        axis.errorbar(
            thresholds,
            measured_rate / current / 1000.0,
            yerr=measured_error / current / 1000.0,
            fmt="o-",
            ms=3.5,
            capsize=2,
            label="Measured cluster-trigger rate",
        )
        positive_generated = generated_rate > 0.0
        axis.plot(
            thresholds[positive_generated],
            generated_rate[positive_generated] / current / 1000.0,
            "s-",
            ms=3.2,
            label="Born inclusive-inelastic",
        )
        axis.fill_between(
            thresholds,
            (generated_rate - generated_error) / current / 1000.0,
            (generated_rate + generated_error) / current / 1000.0,
            alpha=0.18,
        )
        axis.plot(
            thresholds,
            elastic_rate / current / 1000.0,
            "^-.",
            ms=3.2,
            label="Radiated elastic (electron + FSR)",
        )
        axis.plot(
            thresholds,
            total_rate / current / 1000.0,
            "D--",
            ms=3.0,
            label="Born inelastic + radiated elastic",
        )
        axis.set_title(f"Run {run}: {angle:.2f} deg, {current:.2f} uA")
        axis.set_yscale("log")
        axis.set_ylim(1.0e-3, 3.0)
        axis.grid(alpha=0.22, which="both")
        axis.set_xlabel("Offline-equivalent threshold (GeV)")
        if run in (6158, 2855):
            axis.set_ylabel("Rate / beam current (kHz/uA)")
        if run == 6158:
            axis.legend(frameon=False, fontsize=8.5)

        for fraction in THRESHOLD_FRACTIONS:
            threshold = fraction * endpoint
            data_value = log_interpolate(thresholds, measured_rate, threshold)
            data_error = float(np.interp(threshold, thresholds, measured_error))
            generated_value = log_interpolate(thresholds, generated_rate, threshold)
            generated_stat = float(np.interp(threshold, thresholds, generated_error))
            elastic_value = log_interpolate(thresholds, elastic_rate, threshold)
            elastic_stat = float(np.interp(threshold, thresholds, elastic_error))
            total_value = generated_value + elastic_value
            total_stat = math.hypot(generated_stat, elastic_stat)
            output_rows.append(
                {
                    "run": run,
                    "nps_angle_deg": angle,
                    "beam_current_uA": current,
                    "elastic_endpoint_GeV": endpoint,
                    "threshold_fraction": fraction,
                    "physical_threshold_GeV": threshold,
                    "measured_primitive_rate_Hz": data_value,
                    "measured_stat_error_Hz": data_error,
                    "born_inelastic_rate_Hz": generated_value,
                    "born_inelastic_mc_error_Hz": generated_stat,
                    "born_inelastic_over_data": generated_value / data_value,
                    "radiated_elastic_yield_per_mC": elastic_yield_per_mC,
                    "radiated_elastic_electron_plus_fsr_rate_Hz": elastic_value,
                    "radiated_elastic_mc_error_Hz": elastic_stat,
                    "inclusive_plus_elastic_rate_Hz": total_value,
                    "inclusive_plus_elastic_mc_error_Hz": total_stat,
                    "inclusive_plus_elastic_over_data": total_value / data_value,
                }
            )

    fig.suptitle(
        "NPS electron runs: absolute inclusive plus radiated-elastic calculation\n"
        "Thresholds are evaluated relative to each run's elastic endpoint; "
        "neither component is normalized to data",
        y=1.01,
    )
    fig.tight_layout()
    output_base = HERE / "plots" / "nps_inclusive_generator_six_run_validation"
    output_base.parent.mkdir(exist_ok=True)
    fig.savefig(output_base.with_suffix(".png"), dpi=180, bbox_inches="tight")
    fig.savefig(output_base.with_suffix(".pdf"), bbox_inches="tight")

    output_csv = HERE / "nps_inclusive_generator_six_run_validation.csv"
    with output_csv.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=output_rows[0].keys())
        writer.writeheader()
        writer.writerows(output_rows)

    beam_sets = {
        "6.370 GeV": {"runs": (6158, 6165, 6168), "color": "#0072B2"},
        "10.54 GeV": {"runs": (2855, 5183, 3888), "color": "#D55E00"},
    }
    gap_low = 16.58
    gap_high = 18.56
    comparison_angle = 0.5 * (gap_low + gap_high)
    angular_fig, angular_axes = plt.subplots(
        1, 3, figsize=(15.2, 4.8), sharex=True, sharey=True
    )
    for axis, fraction in zip(angular_axes, THRESHOLD_FRACTIONS):
        fits: dict[tuple[str, str], tuple[float, float]] = {}
        for beam_label, setting in beam_sets.items():
            selected = [
                row
                for row in output_rows
                if row["run"] in setting["runs"]
                and row["threshold_fraction"] == fraction
            ]
            selected.sort(key=lambda row: float(row["nps_angle_deg"]))
            angles = np.asarray([row["nps_angle_deg"] for row in selected], dtype=float)
            data = np.asarray(
                [row["measured_primitive_rate_Hz"] / row["beam_current_uA"] / 1000.0 for row in selected],
                dtype=float,
            )
            generated = np.asarray(
                [row["born_inelastic_rate_Hz"] / row["beam_current_uA"] / 1000.0 for row in selected],
                dtype=float,
            )
            total = np.asarray(
                [row["inclusive_plus_elastic_rate_Hz"] / row["beam_current_uA"] / 1000.0 for row in selected],
                dtype=float,
            )
            for series_label, values, marker, linestyle in (
                ("data", data, "o", "-"),
                ("Born inelastic", generated, "s", "--"),
                ("inclusive + elastic", total, "D", ":"),
            ):
                slope, intercept = np.polyfit(angles, np.log(values), 1)
                fits[(beam_label, series_label)] = (slope, intercept)
                grid = np.linspace(angles.min(), angles.max(), 100)
                axis.plot(
                    grid,
                    np.exp(intercept + slope * grid),
                    color=setting["color"],
                    ls=linestyle,
                    lw=1.5,
                )
                axis.plot(
                    angles,
                    values,
                    marker=marker,
                    ls="none",
                    color=setting["color"],
                    markerfacecolor=(
                        setting["color"] if series_label == "data" else "white"
                    ),
                    label=(
                        f"{beam_label} {series_label}"
                        if fraction == THRESHOLD_FRACTIONS[0]
                        else None
                    ),
                )
        ratios = {}
        for series_label in ("data", "Born inelastic", "inclusive + elastic"):
            low_slope, low_intercept = fits[("6.370 GeV", series_label)]
            high_slope, high_intercept = fits[("10.54 GeV", series_label)]
            ratios[series_label] = math.exp(
                high_intercept
                + high_slope * comparison_angle
                - low_intercept
                - low_slope * comparison_angle
            )
        axis.axvspan(gap_low, gap_high, color="0.88", alpha=0.75, zorder=0)
        axis.text(
            0.04,
            0.05,
            f"10.54 / 6.37 at {comparison_angle:.2f} deg\n"
            f"data = {ratios['data']:.2f}; inelastic = {ratios['Born inelastic']:.2f}\n"
            f"inelastic + elastic = {ratios['inclusive + elastic']:.2f}",
            transform=axis.transAxes,
            fontsize=9,
            va="bottom",
        )
        axis.set_title(f"{fraction:.0%} of each run's elastic endpoint")
        axis.set_xlabel("NPS central angle (degrees)")
        axis.set_yscale("log")
        axis.grid(alpha=0.22, which="both")
    angular_axes[0].set_ylabel("Rate / beam current (kHz/uA)")
    angular_axes[0].legend(frameon=False, fontsize=8)
    angular_fig.suptitle(
        "Measured and generated angular trends at two beam energies\n"
        "Absolute cross-section components; no normalization to data",
        y=1.04,
    )
    angular_fig.tight_layout()
    angular_base = HERE / "plots" / "nps_inclusive_generator_angular_validation"
    angular_fig.savefig(angular_base.with_suffix(".png"), dpi=180, bbox_inches="tight")
    angular_fig.savefig(angular_base.with_suffix(".pdf"), bbox_inches="tight")

    for row in output_rows:
        print(
            f"run {int(row['run'])} {row['threshold_fraction']:.0%}: "
            f"data={row['measured_primitive_rate_Hz']/1000:.3f} kHz, "
            f"Born inelastic={row['born_inelastic_rate_Hz']/1000:.3f} kHz, "
            f"elastic={row['radiated_elastic_electron_plus_fsr_rate_Hz']/1000:.3f} kHz, "
            f"total/data={row['inclusive_plus_elastic_over_data']:.3f}"
        )


if __name__ == "__main__":
    main()
