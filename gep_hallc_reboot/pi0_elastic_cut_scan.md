# First common elastic/pi0 cut scan

Established 2026-10-22.

## Purpose

This study applies identically defined reconstructed elasticity variables to the current radiated elastic SIMC sample and the new hard-bremsstrahlung `gamma p -> pi0 p` sample. It is the first cut-level background comparison in the current checkout, but it is not yet a full NPS detector-response prediction.

The source proposal defines the relevant cuts on PAC37 pages 28-32 and Figures 20-23:

1. an ellipse in the two calorimeter-position residuals between the measured hit and the hit predicted from reconstructed HMS proton kinematics under the elastic hypothesis; and
2. the proton angle-momentum residual `(p_recon-p_elastic(theta_p))/p_HMS`.

The proposal reports 3-sigma and 6-sigma results but does not publish the numerical resolutions. The present study therefore derives the center, covariance, and momentum-residual width from the current radiated elastic sample rather than assigning historical BigCal widths to NPS.

## Samples and observable definition

The signal file contains the 11,013 transport-accepted events from the fixed-seed one-million-trial radiated deck `gep_reboot_q11_smoke.inp`. Its accepted normalization is 41.481955 events/mC.

The background file contains the 48,114 transport-accepted events from the fixed-seed 100,000-trial baseline `gep_reboot_q11_pi0phot.inp`. Its accepted normalization is 13.352045 events/mC for the provisional `0.0175 X0` radiator and `s^-7` cross-section model.

For both channels, the reconstructed proton angle determines the elastic proton momentum

    p_el(theta_p) = 2 M E (E+M) cos(theta_p)
                    / [(E+M)^2 - E^2 cos^2(theta_p)].

Momentum conservation then predicts the elastic electron ray and its intercept on NPS. For the pi0 sample, either accepted decay photon may satisfy the position ellipse. Both the position ellipse and momentum cut must pass.

The elastic core is fitted deterministically with iterative three-standard-deviation clipping. This avoids allowing the physical radiative tail to define the detector-resolution scale. The fitted parameters are:

| Quantity | Value |
| --- | ---: |
| Position center x | 0.0280 cm |
| Position center y | 0.1250 cm |
| Position variance x | 0.33492 cm2 |
| Position variance y | 0.24234 cm2 |
| Position covariance xy | 0.03046 cm2 |
| Momentum-residual center | -0.09608% of HMS central momentum |
| Momentum-residual sigma | 0.28754% of HMS central momentum |

## Results

| Common cut width | Elastic efficiency | pi0 efficiency after transport | Elastic yield (/mC) | pi0 yield (/mC) | pi0 fraction of selected sample |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 3 sigma | 77.367% | 0.6919% | 32.0934 | 0.09239 | 0.2870% |
| 6 sigma | 87.685% | 5.0171% | 36.3734 | 0.66989 | 1.8084% |

The independently obtained elastic efficiencies are a useful sanity check. PAC37 Table 1 reported 77.3% and 85.7% at Q2 = 11 GeV2 for its 3-sigma and 6-sigma cuts. The present 3-sigma result agrees almost exactly and the 6-sigma result differs by about two percentage points, despite the different beam energy, detector geometry, and newly derived core widths.

The background fractions are larger than PAC37's reported 0.15% and 0.61%. That difference should not yet be interpreted as a conflict. The current result uses a new data-driven cross-section extrapolation, a provisional mean radiator, 10.7 rather than 11 GeV beam energy, NPS geometry at 3 m, and geometrical photon interception without shower response, clustering, thresholds, or position smearing. The cross-section exponent alone carries an approximately factor-two envelope.

## Reproduction

The analysis is implemented in `gep_hallc_reboot/analyze_pi0_elastic_cuts.py`. With binary output enabled for the two decks, run:

    python3 gep_hallc_reboot/analyze_pi0_elastic_cuts.py \
      worksim/gep_reboot_q11_smoke.bin \
      worksim/gep_reboot_q11_pi0phot.bin

The script reads SIMC's sequential unformatted records directly, fits the elastic core, applies the common cuts, and weights each channel with its SIMC event weights and accepted yield.

## Evidence boundary and next work

Established: the common reconstructed-variable definitions, current elastic core resolution, fixed-seed cut efficiencies, and baseline `s^-7` weighted leakage for geometrically accepted photons.

Not yet established: final NPS energy/position response, shower containment and clustering, trigger threshold, photon/electron response differences, vertex-dependent hard-photon flux, model variations `s^-6` and `s^-8`, or aluminum and accidental backgrounds.

The first parameterized position/energy-threshold convolution is now recorded in [the NPS response scan](nps_response_scan.md). The 0.287% and 1.81% geometry-only values remain useful controls, not expected experimental contaminations. The response study shows that the important threshold turn-on is near 4.0–4.4 GeV; a full shower/trigger model and the three cross-section exponents plus vertex-dependent radiator still remain.
