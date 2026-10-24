#!/usr/bin/env python3
"""Build the proposal-level NPS x HMS-S1 hardware-trigger rate envelope."""

from __future__ import annotations

import csv
import math
from pathlib import Path


ROOT = Path(__file__).resolve().parent
NPS_RUN1918_RATE_HZ = 497396.0
NPS_RUN1918_CURRENT_UA = 27.583
PROPOSAL_CURRENT_UA = 75.0
TARGET_LENGTH_SCALE = 30.0 / 10.0
BUCKET_SPACING_NS = 4.0
S1_RATES_HZ = {
    "S1Y_plane": 19970.978,
    "S1X_plane": 23111.637,
    # Hall C trigger documentation defines S1 = S1X OR S1Y. The scaler map
    # calls this channel S1T; its measured rate is consistent with that logic.
    "S1T_S1X_OR_S1Y": 41310.591,
}
# The scaler provides both plane rates and their OR, so the simultaneous
# two-plane overlap follows exactly from inclusion-exclusion.
S1_RATES_HZ["S1X_AND_S1Y_derived"] = (
    S1_RATES_HZ["S1X_plane"]
    + S1_RATES_HZ["S1Y_plane"]
    - S1_RATES_HZ["S1T_S1X_OR_S1Y"]
)
# Direct linear fit to the run-6151 hTRIG3 scaler.
S1_RATES_HZ["S1_S2_three_of_four"] = 607.2557709131447
OFFLINE_THRESHOLDS_GEV = {
    "70pct_3p4GeV": 3.4,
    "75pct_3p6GeV": 3.6,
    "80pct_3p9GeV": 3.9,
}
VTP_INTERCEPT_GEV = 0.463791
VTP_SLOPE = 0.668158
WINDOWS_NS = (10.0, 20.0, 40.0)

with (ROOT / "nps_run1918_100k_threshold_scan.csv").open() as stream:
    scan = list(csv.DictReader(stream))

def interpolated_count(vtp_threshold_gev: float, column: str) -> float:
    """Log-interpolate the 0.1-GeV threshold scan at a calibrated cut."""
    lower = math.floor(vtp_threshold_gev * 10) / 10
    upper = lower + 0.1
    counts = {round(float(row["threshold_nominal_GeV"]), 1): float(row[column])
              for row in scan}
    fraction = (vtp_threshold_gev - lower) / (upper - lower)
    return math.exp(math.log(counts[round(lower, 1)])
                    + fraction * (math.log(counts[round(upper, 1)])
                                  - math.log(counts[round(lower, 1)])))

BASE_COUNTS = {
    "edtm_tagged": next(float(row["edtm_events"]) for row in scan
                        if float(row["threshold_nominal_GeV"]) == 0.5),
    "accepted_physics": next(float(row["ordinary_physics_events"]) for row in scan
                             if float(row["threshold_nominal_GeV"]) == 0.5),
}
COLUMNS = {
    "edtm_tagged": "edtm_events",
    "accepted_physics": "ordinary_physics_events",
}

rows = []
for threshold, offline_threshold in OFFLINE_THRESHOLDS_GEV.items():
    vtp_threshold = VTP_INTERCEPT_GEV + VTP_SLOPE * offline_threshold
    for spectrum_sample, column in COLUMNS.items():
        count = interpolated_count(vtp_threshold, column)
        retention = count / BASE_COUNTS[spectrum_sample]
        nps_rate_run1918_at_75ua = (
            NPS_RUN1918_RATE_HZ * PROPOSAL_CURRENT_UA / NPS_RUN1918_CURRENT_UA
        )
        nps_rate = nps_rate_run1918_at_75ua * TARGET_LENGTH_SCALE * retention
        for s1_logic, s1_rate in S1_RATES_HZ.items():
            proposal_s1_rate = s1_rate * TARGET_LENGTH_SCALE
            for window_ns in WINDOWS_NS:
                accepted_bucket_offsets = window_ns / BUCKET_SPACING_NS
                continuous_rate = nps_rate*proposal_s1_rate*window_ns*1.e-9
                bucket_rate = (nps_rate*proposal_s1_rate
                               * accepted_bucket_offsets
                               * BUCKET_SPACING_NS*1.e-9)
                rows.append({
                    "nps_threshold_proxy": threshold,
                    "offline_threshold_GeV": offline_threshold,
                    "calibrated_vtp_threshold_GeV": vtp_threshold,
                    "spectrum_sample": spectrum_sample,
                    "interpolated_events_above_threshold": count,
                    "nps_rate_retention": retention,
                    "nps_rate_run1918_27p583uA_10cm_500MeV_Hz": NPS_RUN1918_RATE_HZ,
                    "nps_rate_run1918_geometry_75uA_500MeV_Hz": nps_rate_run1918_at_75ua,
                    "target_length_scale": TARGET_LENGTH_SCALE,
                    "nps_rate_proposal_geometry_Hz": nps_rate,
                    "hms_s1_primitive": s1_logic,
                    "hms_s1_rate_run6151_target_75uA_Hz": s1_rate,
                    "hms_s1_rate_30cm_target_Hz": proposal_s1_rate,
                    "full_coincidence_window_ns": window_ns,
                    "accepted_4ns_bucket_offsets": accepted_bucket_offsets,
                    "accidental_continuous_Hz": continuous_rate,
                    "accidental_bucket_counting_Hz": bucket_rate,
                })

output = ROOT / "gep_coincidence_trigger_budget_75uA.csv"
with output.open("w", newline="") as stream:
    writer = csv.DictWriter(stream, fieldnames=rows[0].keys())
    writer.writeheader()
    writer.writerows(rows)

for row in rows:
    if (row["full_coincidence_window_ns"] == 20.0
            and row["hms_s1_primitive"] in
            {"S1X_AND_S1Y_derived", "S1_S2_three_of_four"}):
        print(
            f"{row['nps_threshold_proxy']} {row['spectrum_sample']} "
            f"x {row['hms_s1_primitive']}: "
            f"NPS={row['nps_rate_proposal_geometry_Hz']/1e3:.1f} kHz, "
            f"S1={row['hms_s1_rate_30cm_target_Hz']/1000:.1f} kHz, "
            f"Racc(20 ns)={row['accidental_continuous_Hz']:.1f} Hz"
        )

