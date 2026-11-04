#!/usr/bin/env python3
"""Analyze 3x3 NPS response for the two inclusive-electron GEANT4 pilots."""

from __future__ import annotations

import csv
import math
import os
from pathlib import Path

os.environ.setdefault("MPLBACKEND", "Agg")
os.environ.setdefault("MPLCONFIGDIR", "/tmp/gep_nps_matplotlib")

import matplotlib.pyplot as plt
import numpy as np


HERE = Path(__file__).resolve().parent
RUNS = (6168, 5183)
PROPOSAL_ELECTRON_ENERGY_GEV = 4.838
FRACTIONS = (0.70, 0.75, 0.80)
MINIMUM_TRANSPORT_ENERGY_GEV = 2.3
INHERITED_ENERGY_SCALE = 1.04**2


def read_hybrid(run: int) -> tuple[np.ndarray, np.ndarray]:
    path = Path(f"/tmp/nps_run{run}_inclusive_geant4_pilot.hybrid.csv")
    energy: list[float] = []
    weight: list[float] = []
    with path.open(newline="") as stream:
        for row in csv.DictReader(line for line in stream if not line.startswith("#")):
            energy.append(float(row["primary_E_GeV"]))
            weight.append(float(row["weight"]))
    return np.asarray(energy), np.asarray(weight)


def read_full_weight(run: int) -> float:
    total = 0.0
    with Path(f"/tmp/nps_run{run}_inclusive_1m.csv").open(newline="") as stream:
        for row in csv.DictReader(stream):
            if float(row["electron_energy_GeV"]) >= MINIMUM_TRANSPORT_ENERGY_GEV:
                total += float(row["weight_Hz"])
    return total


def read_response(run: int) -> tuple[np.ndarray, np.ndarray]:
    total: list[float] = []
    local: list[float] = []
    path = Path(f"/tmp/nps_run{run}_inclusive_geant4_response.csv")
    with path.open(newline="") as stream:
        for row in csv.DictReader(stream):
            total.append(float(row["total_energy_GeV"]))
            local.append(float(row["corrected_3x3_energy_GeV"]))
    return np.asarray(total), np.asarray(local)


def log_interpolate(rows: list[dict[str, float]], threshold: float, key: str) -> float:
    x = np.asarray([row["physical_threshold_GeV"] for row in rows])
    y = np.asarray([row[key] for row in rows])
    return float(math.exp(np.interp(threshold, x, np.log(y))))


def main() -> None:
    output_rows: list[dict[str, float | int]] = []
    fig, axes = plt.subplots(1, 2, figsize=(12.4, 5.0), sharex=True, sharey=True)

    for axis, run in zip(axes, RUNS):
        measured_path = HERE / f"nps_run{run}_100k_edtm_physical.csv"
        with measured_path.open(newline="") as stream:
            measured = [
                {key: float(value) for key, value in row.items()}
                for row in csv.DictReader(stream)
            ]
        current = measured[0]["mean_selected_current_uA"]
        angle = measured[0]["nps_angle_deg"]
        thresholds = np.asarray([row["physical_threshold_GeV"] for row in measured])
        data_rate = np.asarray([row["primitive_rate_Hz"] for row in measured])
        data_error = np.asarray([row["stat_error_Hz"] for row in measured])

        primary_energy, pilot_weight = read_hybrid(run)
        total_energy, local_energy = read_response(run)
        if not (len(primary_energy) == len(total_energy) == len(local_energy)):
            raise ValueError(f"run {run}: hybrid/ROOT event-count mismatch")
        full_weight = read_full_weight(run)
        scale = full_weight / pilot_weight.sum()
        truth_rate = np.asarray(
            [scale * pilot_weight[primary_energy >= threshold].sum() for threshold in thresholds]
        )
        local_rate = np.asarray(
            [scale * pilot_weight[local_energy >= threshold].sum() for threshold in thresholds]
        )
        vtp_thresholds = np.asarray([row["vtp_threshold_GeV"] for row in measured])
        inherited_local_energy = local_energy * INHERITED_ENERGY_SCALE
        vtp_rate = np.asarray(
            [
                scale * pilot_weight[inherited_local_energy >= threshold].sum()
                for threshold in vtp_thresholds
            ]
        )

        positive = truth_rate > 0.0
        axis.errorbar(
            thresholds,
            data_rate / 1000.0,
            yerr=data_error / 1000.0,
            fmt="o-",
            capsize=2,
            label="Measured cluster-trigger rate",
        )
        axis.plot(
            thresholds[positive],
            truth_rate[positive] / 1000.0,
            "s--",
            label="Generator truth, pilot-weighted",
        )
        axis.plot(
            thresholds,
            local_rate / 1000.0,
            "D--",
            label="Raw-deposition 3x3",
        )
        axis.plot(
            thresholds,
            vtp_rate / 1000.0,
            "^-",
            label="3x3 in measured VTP convention",
        )
        axis.set_title(f"Run {run}: {angle:.2f} deg, {current:.2f} uA")
        axis.set_xlabel("Physical threshold (GeV)")
        axis.set_yscale("log")
        axis.grid(alpha=0.22, which="both")
        axis.legend(frameon=False, fontsize=8.5)

        response_ratio = np.divide(
            local_energy,
            primary_energy,
            out=np.zeros_like(local_energy),
            where=primary_energy > 0.0,
        )
        for fraction in FRACTIONS:
            threshold = fraction * PROPOSAL_ELECTRON_ENERGY_GEV
            data_value = log_interpolate(measured, threshold, "primitive_rate_Hz")
            truth_value = float(math.exp(np.interp(threshold, thresholds, np.log(truth_rate))))
            local_value = float(
                math.exp(np.interp(threshold, thresholds, np.log(np.maximum(local_rate, 1e-300))))
            )
            vtp_value = float(
                math.exp(np.interp(threshold, thresholds, np.log(np.maximum(vtp_rate, 1e-300))))
            )
            vtp_threshold_value = float(np.interp(threshold, thresholds, vtp_thresholds))
            output_rows.append(
                {
                    "run": run,
                    "nps_angle_deg": angle,
                    "beam_current_uA": current,
                    "threshold_fraction": fraction,
                    "physical_threshold_GeV": threshold,
                    "measured_primitive_rate_Hz": data_value,
                    "pilot_truth_rate_Hz": truth_value,
                    "geant4_3x3_rate_Hz": local_value,
                    "geant4_3x3_over_data": local_value / data_value,
                    "measured_vtp_threshold_GeV": vtp_threshold_value,
                    "geant4_vtp_convention_rate_Hz": vtp_value,
                    "geant4_vtp_convention_over_data": vtp_value / data_value,
                    "weighted_mean_3x3_over_primary": float(
                        np.average(response_ratio, weights=pilot_weight)
                    ),
                    "pilot_to_full_weight_scale": scale,
                }
            )

    axes[0].set_ylabel("Full-face rate above threshold (kHz)")
    fig.suptitle(
        "Inclusive-electron GEANT4 response pilot\n"
        "Run-specific measured VTP convention compared with raw-deposition 3x3",
        y=1.02,
    )
    fig.tight_layout()
    output_base = HERE / "plots" / "nps_inclusive_geant4_pilot"
    output_base.parent.mkdir(exist_ok=True)
    fig.savefig(output_base.with_suffix(".png"), dpi=180, bbox_inches="tight")
    fig.savefig(output_base.with_suffix(".pdf"), bbox_inches="tight")

    output_csv = HERE / "nps_inclusive_geant4_pilot.csv"
    with output_csv.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=output_rows[0].keys())
        writer.writeheader()
        writer.writerows(output_rows)

    for row in output_rows:
        print(
            f"run {int(row['run'])} {row['threshold_fraction']:.0%}: "
            f"data={row['measured_primitive_rate_Hz']/1000:.3f} kHz, "
            f"truth={row['pilot_truth_rate_Hz']/1000:.3f} kHz, "
            f"raw3x3={row['geant4_3x3_rate_Hz']/1000:.3f} kHz, "
            f"VTP3x3={row['geant4_vtp_convention_rate_Hz']/1000:.3f} kHz, "
            f"VTP3x3/data={row['geant4_vtp_convention_over_data']:.3f}"
        )


if __name__ == "__main__":
    main()
