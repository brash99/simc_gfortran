# Phase 1, Sanity Check 1: six-run configuration audit

**Status:** configuration and distance-provenance audit completed; waveform details remain, 29 October 2026

## Purpose

This audit establishes the actual experimental configuration of NPS runs 6158,
6165, 6168, 2855, 5183, and 3888 before their EDTM event samples are
interpreted as electron-, photon-, or pion-induced trigger populations.

The working values and their verification status are stored in
[`six_run_configuration_audit.csv`](six_run_configuration_audit.csv).

## Primary configuration results

| Run | Group | Beam energy | Selected EDTM current | NPS angle | NPS front distance | HMS setting | VTP trigger threshold |
|---:|---|---:|---:|---:|---:|---|---:|
| 6158 | three-pass | 6.370200 GeV | 25.6416 uA | 21.17 deg | 9.5 m | +2.6390 GeV/c at 35.97 deg | 1600 |
| 6165 | three-pass | 6.370236 GeV | 25.5974 uA | 19.87 deg | 9.5 m | +2.6390 GeV/c at 35.97 deg | 1600 |
| 6168 | three-pass | 6.370304 GeV | 25.5059 uA | 18.56 deg | 9.5 m | +2.6390 GeV/c at 35.97 deg | 1600 |
| 2855 | five-pass | 10.540482 GeV | 37.7535 uA | 16.58 deg | 9.5 m | +4.0872 GeV/c at 29.86 deg | 1400 |
| 5183 | five-pass | 10.537709 GeV | 29.2179 uA | 15.57 deg | 9.5 m | +4.3100 GeV/c at 28.67 deg | 1600 |
| 3888 | five-pass | 10.539113 GeV | 38.0349 uA | 14.76 deg | 9.5 m | +4.0782 GeV/c at 29.85 deg | 1400 |

The 9.5 m distance is established by the separate
[calorimeter-distance provenance audit](calorimeter_distance_provenance.md).
The decisive evidence is the post-run NPS calibration analysis of these exact
run groups, supported by the run plan and an independent event-by-event
geometric reconstruction from the HMS proton and NPS shower positions.

RCDB's conflicting 3.0 and 4.0 m values are not used. The RCDB implementation
shows that this field was a manually supplied run-start argument rather than an
EPICS or survey measurement.

## Run purpose

RCDB explicitly labels runs 6158, 6165, 6168, and 5183 as `Elastic`. Run 2855
has run type `Production`, but its user comment says `5pass Elastic
Calibration`. Run 3888 likewise has run type `Production` and the comment
`5Pass_Elastic`.

Thus, all six are elastic-calibration runs in the operational sense, but they
are not one uniform kinematic series.

## Elastic-calibration energy

The post-run NPS calibration analysis associates 4.598 GeV electrons with the
three-pass run group 6151--6168 and 7.072 GeV electrons with all three selected
five-pass run groups. Those values should be retained for the calibration
analysis.

A simple calculation from the nominal central HMS momentum does not reproduce
those energies exactly and should not be substituted for the collaboration's
post-run elastic reconstruction without first accounting for the actual
spectrometer optics, momentum corrections, and event-by-event kinematics.

## Trigger and readout differences

| Run(s) | Run configuration | Active prescale | FADC250 sparsification | Cluster readout threshold | Pair threshold |
|---|---|---|---:|---:|---:|
| 6158, 6165, 6168 | `coin_sparse` | PS5 unprescaled | 1 | 350 | 800 |
| 2855 | `coin_sparse` | PS3 unprescaled | 1 | 500 | 650 |
| 5183 | `coin_sparse` | PS5 unprescaled | 1 | 500 | 800 |
| 3888 | `coin` | PS5 unprescaled | 0 | 400 | 750 |

The report prints an active prescale as factor 1, whereas RCDB stores the same
setting as zero; the table states the physical meaning. Run 3888 is not sparse,
and run 2855 uses different trigger routing. These differences must be
considered before treating all six EDTM samples as detector-identical.

## Sweeper status

RCDB records `nps_sweeper = 0` for runs 6158, 6165, 6168, 5183, and 3888.
Run 2855 lacks the field, but run 2856 at the same apparatus setting records
zero. The evidence therefore supports sweeper off for all six, with run 2855
still dependent on the adjacent-run inference.

## Items still requiring verification

- Recover the embedded event-137 VTP seed, timing, and local-maximum settings.
- Recover the FADC waveform window and detailed waveform-readout settings.
- If survey-level distance precision becomes necessary, locate the April 2024
  NPS survey coordinates and reproduce the target-to-front-face distance in
  the same convention used by the calibration analysis.

## Evidence

- Read-only Hall C RCDB conditions for the six runs and adjacent runs
  2850--2860.
- Private replay reports
  `coin_NPS_HMS_report_<run>_100000.report`.
- `DBASE/NPS/standard_coin.kinematics` from the private replay checkout.
- The local EDTM/scaler closure CSV files for selected-window currents.
- `make_nps_six_run_elastic_inputs.py`, which documents the geometry used by
  the existing simulation and reveals the now-identified 9.5 m assumption.

## Sanity Check 1 conclusion

All six selected runs are elastic-calibration runs and were taken with the NPS
front face at 9.5 m. They do differ in HMS kinematics, trigger routing,
thresholds, and sparsification settings, so they should not be treated as a
completely uniform detector sample.

The original 9.5 m generator geometry is retained. The brief RCDB-based
conclusion that the runs were at 3--4 m is withdrawn and preserved only in the
distance-provenance audit as a warning about metadata quality.
