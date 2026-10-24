# Random-window NPS pulse overlay using run-6151 EDTM events

Checked: 2026-10-23.

## What an EDTM event means here

EDTM is the Hall C **Electronic Dead Time Measurement** system. An EDTM event
is an electronically injected/forced trigger, not a physics event. Its rate is
configured by the experimenters and must not be assumed to be the same for
GEp-IV as it was for the 2023 DVCS running period.

The EDTM *rate is not used in this projection*. The useful feature is that an
EDTM trigger supplies a forced detector readout window that is not required to
have been selected by an NPS physics trigger. Real NPS pulses found in that
window provide a sample of
accidental detector activity per readout window at the measured beam current.
The proposed GEp-IV EDTM rate would only determine how many such monitoring
windows were collected; it would not set the accidental occupancy of a window.

This interpretation still assumes that NPS pulse decoding and sparsification
do not bias the activity retained in an EDTM readout. That caveat is important
below.

## Pulse library and local energy calibration

The run-6151 100k private replay contains 22,646 EDTM events in the 22.5--30
microampere interval, at mean current 25.988 microampere. Of these, 4,656 have
at least one decoded NPS pulse; the library contains 15,521 pulses in total.
Every EDTM window, including zero-pulse windows, is retained in
[`nps_edtm_overlay_6151_22p5_30uA.csv`](nps_edtm_overlay_6151_22p5_30uA.csv).

For a local pulse-energy estimate, 19,025 proton-tagged ordinary elastic
events were used to regress offline primary-cluster energy against the decoded
pulse amplitudes in the seed-centered 3x3 and within 4 ns of the seed pulse.
The fitted through-origin conversion is

```text
local 3x3 energy / summed pulse amplitude = 0.0127839 GeV/mV.
```

This is an empirical run-6151 conversion for the present overlay, not a claim
that the detector response is universally linear in pulse amplitude. The
export is reproducible with
[`export_nps_edtm_library.C`](export_nps_edtm_library.C); its numerical audit
is in
[`nps_edtm_overlay_6151_22p5_30uA_summary.txt`](nps_edtm_overlay_6151_22p5_30uA_summary.txt).

## Projection to 75 microampere

The 75-microampere accidental window is modeled by superposing
`75/25.988 = 2.8859` independently sampled run-6151 EDTM windows: two always,
plus a third with probability 0.8859. This assumes accidental pulse intensity
scales linearly with current and that the sampled windows are independent.
Beam-independent electronic activity is also multiplied in this construction,
so that contribution is conservatively overlaid. It
does **not** scale or prescribe the EDTM trigger rate.

For each of 100 overlay replicas over the 10,923-event weighted GEp-IV hybrid
sample, decoded pulses are mapped onto the simulated shower's hottest-block
3x3. The hcana row-major block convention is reconciled with GEANT4's
column-major convention and opposite horizontal indexing. Pulse energy is
added only when its decoded time lies within the stated window around the
run-6151 elastic pulse time.

![Run-6151 EDTM pulse overlay](plots/nps_edtm_overlay_75uA.png)

| Pulse selection | Fiducial showers with local overlap | Mean added energy when overlapping | Change at 80% threshold |
| --- | ---: | ---: | ---: |
| `|dt| < 2 ns`, all decoded pulses | 0.00103% | 131 MeV | 0.00000 percentage points |
| `|dt| < 4 ns`, all decoded pulses | 0.00375% | 344 MeV | 0.00000 percentage points |
| `|dt| < 4 ns`, amplitude at least 5 mV | 0.00375% | 339 MeV | 0.00000 percentage points |
| `|dt| < 4 ns`, amplitude at least 10 mV | 0.00327% | 373 MeV | 0.00000 percentage points |

No overlaid event crosses either the 70% or 80% elastic-energy threshold in
these replicas. The 80%-threshold weighted efficiency and yield therefore
remain 69.94644% and 28.91708 events/mC at the displayed precision. The mean
energy conditional on overlap is based on very few overlaps and should be
treated as descriptive, not as a precise response measurement.

## What this resolves—and what it does not

The result strongly indicates that **decoded, in-time accidental pulses in the
same 3x3 as an elastic shower are negligible for the elastic threshold
efficiency** under this model. Together with the resolved-cluster overlay, it
closes the part of the uncertainty that can be tested using reconstructed
clusters and decoded pulse records from run 6151.

It is not yet a complete upper bound on analog pileup. NPS waveform storage is
cluster-mask sparsified in run 6151, so low-amplitude pulses in blocks not
exposed by that mask may be absent from the decoded arrays. Baseline shifts,
subthreshold charge underneath the elastic pulse, pulse-shape distortion, and
electronics saturation also cannot be reconstructed by adding decoded pulse
amplitudes. A truly complete bound requires unsparsified random-window
waveforms or a validated FADC/VTP electronics simulation. Those missing
effects should be carried as a detector-response systematic rather than
inflating the measured decoded-pulse overlay into an assumed inefficiency.

The overlay implementation and full numerical table are
[`overlay_nps_edtm.py`](overlay_nps_edtm.py) and
[`nps_edtm_overlay_75uA.csv`](nps_edtm_overlay_75uA.csv). The vector plot is
[`nps_edtm_overlay_75uA.pdf`](plots/nps_edtm_overlay_75uA.pdf).
