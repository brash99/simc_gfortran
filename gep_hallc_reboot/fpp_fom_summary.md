# GEp-4 Hall C focal-plane-polarimeter FOM status

**Updated:** 5 November 2026

## Bottom line

The new Hall C single-analyzer GEANT4 calculation produces an integrated
polarimeter figure of merit comparable to the SBS GEp best-case planning
baseline.  After applying the current provisional NPS-response retention, the
Hall C polarized FOM is approximately 5,670--5,820, compared with approximately
5,350 for the SBS baseline constructed with 9.3 million events, 20% useful FPP
efficiency, the same effective analyzing power, and 85% beam polarization.
This corresponds to only a 3--4% reduction in the polarization-component
statistical uncertainty, so the appropriate conclusion is **competitive and
possibly modestly better**, not yet decisively superior.

The most important design result is angular: approximately 76% of the Hall C
intrinsic FOM lies below 5 degrees, but a practical 1-degree lower cut retains
97.3% of the available FOM.  Four-chamber tracking at small polar angle is
therefore essential, while reconstructing reliably below approximately one
degree offers little additional statistical return.

## Simulation configuration

The calculation uses the public
[`gep4_fpp_geant4`](https://github.com/brash99/gep4_fpp_geant4) implementation:

- one polyethylene analyzer with thickness 55.88 cm;
- four Hall C FPP drift chambers, each represented by its U/X/V wire planes;
- 6.7 GeV/c monoenergetic incident protons;
- GEANT4 `FTFP_BERT` hadronic transport;
- the historical closest-approach and cone-test selections;
- a twelve-plane left/right-permutation track fit for small-angle events; and
- the GEp-III high-energy analyzing-power parameterization used in the SBS
  GEp projection software.

The current track reconstruction is an ideal closure calculation.  Hits are
truth-associated and use exact unsigned drift distances.  Chamber efficiency,
drift resolution, noise, competing tracks, dead regions, and full pattern
recognition have not yet been included.

## Analyzing power and FOM convention

The adopted GEp-III parameterization is

```text
Ay = (0.124207234 + 1.26386308/p) pT exp(-2.42614689 pT^2),
```

where `p` and `pT` are in GeV/c.  The adopted useful range is
`0.07 < pT < 1.0 GeV/c`.  At 6.7 GeV/c this corresponds approximately to
`0.6 < theta_polar < 8.6 degrees`.

The intrinsic polarimeter FOM per incident proton is

```text
FOM_intrinsic = sum(selected weight * Ay^2) / sum(incident weight).
```

The beam-polarization-weighted FOM is

```text
FOM_polarized = Pe^2 * FOM_intrinsic.
```

For `N` incident elastic protons, the approximate uncertainty of either fitted
FPP polarization component is

```text
sigma_component = sqrt(2 / (N * FOM_polarized)).
```

This is not yet the uncertainty in `mu_p GE/GM`; the latter requires
event-by-event HMS spin transport and the polarization likelihood/covariance.

## 55.88 cm single-analyzer result

For the full useful angular region:

| Quantity | Result |
|---|---:|
| Useful-pT efficiency | 26.264% |
| RMS effective Ay within useful-pT events | 0.06311 |
| Intrinsic FOM per incident proton | 0.001046 |
| Polarized FOM per incident proton, Pe = 0.85 | 0.0007558 |

The lower-angle-cut dependence is:

| Minimum polar angle | Intrinsic FOM | FOM retained | Component sigma for 10.69 million incident events |
|---:|---:|---:|---:|
| 0 degrees | 0.001046 | 100.0% | 0.01573 |
| 1 degree | 0.001018 | 97.3% | 0.01595 |
| 2 degrees | 0.000877 | 83.8% | 0.01719 |
| 3 degrees | 0.000686 | 65.6% | 0.01943 |
| 4 degrees | 0.000459 | 43.9% | 0.02374 |
| 5 degrees | 0.000255 | 24.3% | 0.03189 |

The 10.69-million column is a polarimeter-only upper-bound normalization: that
event count precedes the NPS response requirement.  The current provisional
70--80% NPS threshold and fiducial retain approximately 7.5--7.7 million
events, giving an integrated polarized FOM of approximately 5,670--5,820 and a
component uncertainty of approximately 0.0188--0.0185.

- [FOM and angular-contribution plot](https://github.com/brash99/gep4_fpp_geant4/blob/main/plots/single_55p88cm_fom.pdf)
- [Four-chamber small-angle closure plot](https://github.com/brash99/gep4_fpp_geant4/blob/main/plots/single_55p88cm_four_chamber_reco_closure.pdf)

## Four-chamber reconstruction closure

The 100,000-event sample contains 59,113 truth-selected events between zero
and ten degrees.  All were reconstructed by the ideal twelve-plane fit.  No
event has a polar-angle error above 0.5 degrees or a wrapped azimuthal error
above 30 degrees.  The polar-angle RMS ranges from approximately 0.000003
degrees below 0.5 degrees to 0.0021 degrees in the 7--10-degree bin.

This establishes the internal geometry, wire mapping, left/right permutation,
and fitting closure.  The extremely small residuals should not be interpreted
as a prediction of experimental angular resolution because the digitized
drifts are presently exact and the correct hit set is supplied to the fitter.

## Comparison with SBS GEp

No independently recovered final SBS analysis FOM is yet available in this
study.  The comparison therefore uses Andrew's approximately 9.3-million-event
best-case SBS planning sample and the stated conservative 20% useful
polarimeter efficiency.  Using the same effective `Ay = 0.06311` and
`Pe = 0.85` gives:

| Scenario | Integrated polarized FOM | Component uncertainty |
|---|---:|---:|
| SBS best-case planning baseline | approximately 5,350 | approximately 0.0193 |
| Hall C, provisional NPS-response-reduced yield | approximately 5,670--5,820 | approximately 0.0188--0.0185 |
| Hall C, pre-NPS-response upper bound | approximately 8,080 | approximately 0.0157 |

The response-reduced Hall C estimate is only 6--9% above the SBS planning FOM.
Moreover, the SBS 20% efficiency is intended as an effective reconstructed
polarimeter efficiency, while the Hall C 26.264% is still an ideal
truth-associated result.  Realistic Hall C chamber and pattern-recognition
losses could consume the present small advantage.  Conversely, the Hall C
four-chamber lever arm and acceptance may ultimately outperform the
conservative SBS-style efficiency assumption.  Both possibilities must be
tested rather than assumed.

## What is established and what remains

Established:

- a functioning GEANT4 single-analyzer Hall C geometry;
- historical U/X/V wire mapping and exhaustive left/right fitting;
- ideal four-chamber closure into the sub-degree region;
- an interchangeable, tested GEp-III analyzing-power model;
- a reproducible integrated-FOM calculation; and
- an SBS-scale Hall C FOM after provisional NPS-response losses.

Still required before a proposal precision claim:

1. Add measured chamber efficiency, drift resolution, noise, dead regions, and
   multi-track pattern recognition.
2. Process the weighted GEp-4 SIMC proton phase space rather than a central-ray
   monoenergetic sample.
3. Validate the useful-scattering efficiency and angular spectrum against
   GEp-III or GEp-2gamma data.
4. Evaluate GEANT4 hadronic-model dependence.
5. Replace the provisional NPS-response yield with the final signal-efficiency
   convention.
6. Apply event-by-event HMS spin transport and a polarization pseudo-analysis
   to obtain the form-factor-ratio uncertainty.

## Current assessment

The FPP is no longer an unquantified feasibility assumption.  The present
calculation establishes that a single-analyzer, four-chamber Hall C design can
deliver an SBS-scale integrated polarimetry FOM and that the crucial small-angle
region is geometrically and algebraically reconstructible in the ideal limit.
The remaining work is detector realism, data validation, and conversion from
polarization-component information to the final form-factor-ratio precision.
