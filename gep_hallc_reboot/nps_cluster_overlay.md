# Data-shaped NPS cluster overlay on the GEp-IV hybrid sample

Checked: 2026-10-23.

## Event libraries

The private 100k replays were revisited event by event with the established
proton-tagged elastic-core selection. One row is retained for every additional
offline cluster, including its measured energy, position, separation from the
elastic cluster, and relative time. Events without an additional cluster are
also retained explicitly.

| Library | Current range | Elastic-core events | Multi-cluster events | CSV rows |
| --- | ---: | ---: | ---: | ---: |
| Run 6151 | 22.5--30 microampere | 19,025 | 3,968 | 19,848 |
| Run 2875 | 35--40 microampere | 10,825 | 3,029 | 11,811 |

The corresponding files are
[`nps_overlay_6151_22p5_30uA.csv`](nps_overlay_6151_22p5_30uA.csv) and
[`nps_overlay_2875_35_40uA.csv`](nps_overlay_2875_35_40uA.csv). They are
generated reproducibly by
[`export_nps_overlay_library.C`](export_nps_overlay_library.C).

The GEANT4 side is represented by 10,923 event-level records containing the
SIMC weight, primary energy, projected face position, hottest-block crystal
index, total deposited fraction, and correctly mapped 3x3 sum. Those records
are in
[`gep_reboot_q11_trigger_events.csv`](gep_reboot_q11_trigger_events.csv).

## Conservative 75-microampere overlay

The first overlay uses the run-6151 additional-cluster shapes because its
4.598 GeV elastic energy is closest to the 4.838 GeV GEp-IV setting. The number
of sampled additional clusters follows the provisional 75-microampere Poisson
occupancy from the data-anchored current model. A measured cluster is allowed
to add its full reconstructed energy to the simulated elastic sum only when
its measured relative position lies inside the chosen local window and its
relative time lies inside the chosen timing window.

This is deliberately conservative: an offline secondary near the primary can
be a split piece of the same shower, whose energy GEANT4 already contains.
Treating it as an independent addition can only promote events above threshold;
the model contains no dead time, pulse-shape distortion, baseline shift, or
reconstruction loss.

![Measured-cluster overlay](plots/nps_cluster_overlay_75uA.png)

| Overlay window | Events receiving added energy | Change at 70% threshold | Change at 80% threshold |
| --- | ---: | ---: | ---: |
| 3x3, `|dt| < 2 ns` | 0.000% | 0.000 percentage points | 0.000 percentage points |
| 3x3, `|dt| < 4 ns` | 0.000% | 0.000 percentage points | 0.000 percentage points |
| 5x5, `|dt| < 4 ns` | 1.736% | +0.005 percentage points | +0.018 percentage points |

Distinct reconstructed clusters never place their centers inside the elastic
3x3 window in this library, which is consistent with the clustering algorithm
merging activity at that separation. Even the deliberately broader 5x5 window
changes the 80%-threshold yield only from 28.917 to 28.925 events/mC.

## Interpretation

Measured *resolved* extra clusters have negligible influence on the elastic
3x3 threshold sum. This is encouraging and strengthens the separation between
elastic efficiency and the much larger global occupancy/DAQ problem.

That next boundary has now been tested with decoded block pulses from
independently triggered run-6151 EDTM readout windows. The EDTM trigger rate is
not used or projected; the windows sample accidental detector activity at the
measured current. The resulting same-3x3, 4 ns overlap probability is only
0.00375% at 75 microampere, with no observed change to the 70% or 80% elastic
threshold efficiency. See
[`nps_edtm_pulse_overlay.md`](nps_edtm_pulse_overlay.md). Low-amplitude analog
activity hidden by run-6151 sparsification remains outside that bound.

The overlay implementation and numerical results are
[`overlay_nps_clusters.py`](overlay_nps_clusters.py) and
[`nps_cluster_overlay_75uA.csv`](nps_cluster_overlay_75uA.csv). The vector plot
is [`nps_cluster_overlay_75uA.pdf`](plots/nps_cluster_overlay_75uA.pdf).
