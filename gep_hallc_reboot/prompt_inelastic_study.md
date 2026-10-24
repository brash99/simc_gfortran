# Prompt-correlated nonelastic study

Checked: 2026-10-24.

## Measurement

The 100,000-event private replay of NPS run 6151 supplies the closest available
data topology to the proposal: an electromagnetic NPS cluster correlated with
a proton in HMS. Events require one HMS track, `|delta| < 8%`, both S1X and S1Y
TDC signals, and an NPS cluster above the stated threshold.

The reconstructed NPS coincidence-time peak is centered at 157.5 ns. The
central 4 ns RF bucket was compared with sixteen neighboring 4 ns buckets.
Only 12--17 total events occur in all side buckets because the hardware
coincidence was already imposed before recording. Those sidebands therefore
cannot by themselves estimate the in-gate accidentals.

Instead, the accidental contribution is integrated snapshot by snapshot using

```text
R_acc = R_NPS R_HMS(3/4) (20 ns),
```

with the measured run-6151 scaler rates and threshold retention. Subtracting
that expectation from the prompt yield outside the three-sigma elastic
expected-energy core gives:

| NPS threshold | Raw prompt nonelastic | Expected accidentals | Corrected correlated nonelastic | Nonelastic / elastic | Recorded corrected rate |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 3.4 GeV | 1870.8 | 79.4 | 1791.4 | 7.74% | 1.161 Hz |
| 3.6 GeV | 1179.9 | 75.1 | 1104.8 | 4.77% | 0.716 Hz |
| 3.9 GeV | 571.0 | 60.0 | 511.0 | 2.35% | 0.331 Hz |

The recorded rates include the run-6151 trigger, prescale, live-time, geometry,
and kinematics. They are not direct Q2=11 hardware-rate predictions. The ratios
are more portable diagnostics, but their transfer to the proposal kinematics
must still be modeled.

## Interpretation limit

This measurement establishes a correlated **nonelastic-core** component, not a
pure inelastic cross section. The complement of a three-sigma elastic core also
contains:

- radiative elastic events;
- non-Gaussian NPS response and leakage tails;
- imperfect proton momentum or energy reconstruction; and
- true electroproduction and other inelastic channels.

Therefore 7.74%, 4.77%, and 2.35% are data-driven upper bounds on the true
inelastic/elastic ratios for this calibration setting. The next required step
is a template decomposition of the energy-residual distribution using the
radiative elastic SIMC plus NPS response model, leaving the unexplained prompt
remainder as the inelastic component. A proposal-kinematics inclusive model is
then needed to transport that component to Q2=11.

## First radiative-elastic template decomposition

The fixed-seed run-6151 SIMC sample contains 2,514 accepted radiative-elastic
events. A companion truth export supplies the generated proton four-vector for
each accepted row; row order was verified as the SIMC-to-Geant event mapping.
The existing Geant sample could therefore be reused without another detector
simulation.

The raw Geant template has a response mean of 0.9277, residual mean of
-0.3281 GeV, and a narrow fitted core at -0.1503 GeV with sigma 0.0703 GeV.
For this first decomposition, its peak is shifted to the measured data mean
(-0.2258 GeV) and convolved in quadrature to the measured 0.2021 GeV core
width. At each NPS threshold, the aligned template is normalized to the data
inside the measured three-sigma core. Its predicted radiative/leakage tail and
the independently integrated accidental expectation are then subtracted from
the prompt outside-core population:

| NPS threshold | Prompt outside core | Elastic-template tail | Accidentals | Unexplained prompt | Unexplained / elastic core |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 3.4 GeV | 1,871 | 1,076.2 | 79.4 | 715.4 | 3.09% |
| 3.6 GeV | 1,180 | 632.8 | 75.1 | 472.1 | 2.04% |
| 3.9 GeV | 571 | 25.0 | 60.0 | 486.0 | 2.24% |

This is meaningful progress: at 3.4 and 3.6 GeV, the modeled elastic tail
explains a substantial fraction of the earlier nonelastic-core upper bound.
The remainder is still not yet a pure inelastic measurement. The current
2,514-event template is statistically limited, the peak shift and added
Gaussian resolution are empirical, and no explicit smooth inelastic component
has yet been fitted. The 3.9-GeV result is especially sensitive to the
threshold/response modeling: that threshold removes nearly all of the modeled
low-energy elastic tail, so the unexplained fraction does not decrease
monotonically. Treat 2--3% as the present run-6151 correlated-background scale,
not as a final transported Q2=11 rate.

## Reproduction

- [`analyze_prompt_inelastic.C`](analyze_prompt_inelastic.C) measures the RF
  bucket yields and elastic-core complement.
- [`prompt_inelastic_run6151.csv`](prompt_inelastic_run6151.csv) contains the
  raw prompt and sideband result.
- [`correct_prompt_inelastic.py`](correct_prompt_inelastic.py) integrates the
  measured singles product and produces
  [`prompt_inelastic_run6151_corrected.csv`](prompt_inelastic_run6151_corrected.csv).
- [`analyze_run6151_elastic_template.C`](analyze_run6151_elastic_template.C)
  joins the SIMC proton truth to the existing Geant response.
- [`decompose_run6151_prompt_tail.C`](decompose_run6151_prompt_tail.C) performs
  the aligned-template subtraction; the numerical output is in
  [`prompt_inelastic_run6151_decomposed.csv`](prompt_inelastic_run6151_decomposed.csv).
