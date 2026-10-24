# Provisional NPS occupancy projection to 75 microampere

Checked: 2026-10-23.

## Purpose and limits

This is the first numerical bridge between the measured NPS activity in the
private run-6151/run-2875 replays and the GEp-IV operating current. It is an
occupancy envelope, not yet a literal waveform or crystal overlay on GEANT4.
The two calibration settings differ in energy and run period, and the highest
measured stable point is approximately 38 microampere. Extrapolation to 75
microampere is consequently model dependent.

For each event-level probability, the transform

```text
lambda = -ln(1-p)
```

is fitted as proportional to beam current through the origin. The projected
probability is then `1-exp(-lambda)`. This is the simplest independent-Poisson
occupancy model. The narrow quoted intervals contain binomial counting
statistics only; they do **not** represent the larger cross-setting,
current-extrapolation, timing-window, or reconstruction systematics.

## Data anchors and projection

The fit uses the populated run-6151 current bands and the stable 38.27
microampere run-2875 point. The decoded-pulse curve uses the equal 5,000-event
raw-pulse samples and a common 10 mV, 10 cm, and 8 ns definition. Run 3883 is
excluded because its FADC sparsification was disabled.

| Observable | Projected probability at 75 microampere |
| --- | ---: |
| Any reconstructed extra cluster | 48.64% |
| Energetic, far, out-of-time cluster | 20.76% |
| In-time and near cluster | 4.26% |
| Isolated out-of-time decoded pulse at least 10 mV | 45.36% |

![Provisional NPS occupancy projection](plots/nps_occupancy_projection_75uA.png)

The central distinction is physical. Roughly half of accepted events may
contain some additional activity at 75 microampere under this extrapolation,
but only about 4% are projected to contain an extra reconstructed cluster both
near the elastic shower and within 4 ns. The much larger far/out-of-time and
decoded-pulse probabilities mainly describe the trigger, sparsification,
readout, and accidental environment; they must not automatically be applied as
elastic-efficiency losses.

This supports Andrew's prioritization: the central elastic calorimeter response
can remain clean while singles/coincidence trigger rates and DAQ load are the
more serious questions. It also supports excluding the beamline-side NPS
region, where occupancy is expected to be largest, at little useful elastic
yield cost.

## Use in the hybrid model

For now:

- carry 4.3% as the provisional scale of spatially and temporally overlapping
  extra-cluster activity, not as a fixed inefficiency;
- carry approximately 45-49% as an event-environment/DAQ occupancy scale;
- do not add far or out-of-time energy to the elastic cluster sum;
- do not extrapolate these probabilities beyond 75 microampere; and
- treat variation of the Poisson/current assumption as a systematic envelope.

A literal overlay requires an event library containing block position,
amplitude or energy, and time, followed by the same crystal-level trigger-sum
and clustering logic used for simulation and data. That is the next technical
stage.

The resolved-cluster part of that stage is now complete in
[`nps_cluster_overlay.md`](nps_cluster_overlay.md). Measured additional
clusters do not enter the elastic 3x3 window in the exported library; even a
broader 5x5, 4 ns overlay changes the 80%-threshold efficiency by only 0.018
percentage points. The decoded-pulse part of unresolved same-area pileup has
now been tested with independently triggered EDTM readout windows; see
[`nps_edtm_pulse_overlay.md`](nps_edtm_pulse_overlay.md). At 75 microampere the
projected same-3x3, 4 ns overlap is 0.00375%, with no observed threshold change.
This projection uses detector activity per EDTM readout window and makes no
assumption about the future GEp-IV EDTM trigger rate. Subthreshold analog
activity hidden by run-6151 sparsification remains unbounded by these data.

The reproducible model and numerical output are
[`model_nps_occupancy.py`](model_nps_occupancy.py) and
[`nps_occupancy_projection_75uA.csv`](nps_occupancy_projection_75uA.csv). The
vector plot is
[`nps_occupancy_projection_75uA.pdf`](plots/nps_occupancy_projection_75uA.pdf).
