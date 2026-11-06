#!/usr/bin/env python3
"""Transfer the stable run-6887 NPS 3x3 response to GEp face rates."""

from __future__ import annotations

import csv
import math
from pathlib import Path

import numpy as np


HERE = Path(__file__).resolve().parent
THRESHOLDS = np.asarray((2.5, 3.0, 3.387, 3.629, 3.870, 4.0))


def columns(path: Path) -> dict[str, np.ndarray]:
    with path.open(newline="") as stream:
        rows = list(csv.DictReader(stream))
    return {
        key: np.asarray([float(row[key]) for row in rows])
        for key in rows[0]
    }


def interp(table: dict[str, np.ndarray], x_name: str, y_name: str) -> np.ndarray:
    return np.interp(THRESHOLDS, table[x_name], table[y_name])


def ratio_with_error(
    numerator: np.ndarray,
    numerator_error: np.ndarray,
    denominator: np.ndarray,
    denominator_error: np.ndarray,
) -> tuple[np.ndarray, np.ndarray]:
    ratio = numerator / denominator
    error = ratio * np.hypot(
        numerator_error / numerator, denominator_error / denominator
    )
    return ratio, error


def main() -> int:
    run_face = columns(
        HERE / "run6887_xsbias3e5_1000000_cut0p25_nps.thresholds.csv"
    )
    run_response = columns(
        HERE / "run6887_xsbias3e5_1000000_cut0p25_nps_rates_6pt.csv"
    )
    gep_face = columns(HERE / "gepq11_xsbias3e5_1000000_cut0p25_face_rates.csv")
    inclusive = columns(HERE.parents[2] / "gep_q11_nps_inclusive_prediction.csv")

    run_face_rate = interp(run_face, "threshold_GeV", "rate_Hz")
    run_face_error = interp(run_face, "threshold_GeV", "rate_stat_unc_Hz")
    gep_face_rate = interp(gep_face, "threshold_GeV", "rate_Hz")
    gep_face_error = interp(gep_face, "threshold_GeV", "rate_stat_unc_Hz")
    inclusive_rate = interp(
        inclusive, "truth_energy_threshold_GeV", "combined_rate_Hz"
    )
    inclusive_error = interp(
        inclusive, "truth_energy_threshold_GeV", "combined_mc_error_Hz"
    )

    result: dict[str, np.ndarray] = {}
    for convention in ("raw", "corrected"):
        response_rate = run_response[f"{convention}_3x3_rate_Hz"]
        response_error = run_response[f"{convention}_3x3_mc_error_Hz"]
        transfer, transfer_error = ratio_with_error(
            response_rate, response_error, run_face_rate, run_face_error
        )
        projected = gep_face_rate * transfer
        projected_error = projected * np.hypot(
            gep_face_error / gep_face_rate, transfer_error / transfer
        )
        result[f"{convention}_transfer"] = transfer
        result[f"{convention}_transfer_error"] = transfer_error
        result[f"{convention}_electronuclear"] = projected
        result[f"{convention}_electronuclear_error"] = projected_error
        result[f"{convention}_total"] = inclusive_rate + projected
        result[f"{convention}_total_error"] = np.hypot(
            inclusive_error, projected_error
        )

    output = HERE / "gepq11_response_transfer_projection.csv"
    fields = [
        "threshold_GeV", "gepq11_face_rate_Hz", "gepq11_face_mc_error_Hz",
        "inclusive_plus_elastic_rate_Hz", "inclusive_plus_elastic_mc_error_Hz",
    ] + list(result)
    with output.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, lineterminator="\n")
        writer.writeheader()
        for index, threshold in enumerate(THRESHOLDS):
            row = {
                "threshold_GeV": threshold,
                "gepq11_face_rate_Hz": gep_face_rate[index],
                "gepq11_face_mc_error_Hz": gep_face_error[index],
                "inclusive_plus_elastic_rate_Hz": inclusive_rate[index],
                "inclusive_plus_elastic_mc_error_Hz": inclusive_error[index],
            }
            row.update({name: values[index] for name, values in result.items()})
            writer.writerow(row)
            print(
                f"{threshold:.3f} GeV: corrected eN="
                f"{result['corrected_electronuclear'][index]/1000:.1f} +/- "
                f"{result['corrected_electronuclear_error'][index]/1000:.1f} kHz; "
                f"total={result['corrected_total'][index]/1000:.1f} +/- "
                f"{result['corrected_total_error'][index]/1000:.1f} kHz"
            )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
