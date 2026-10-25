# NPS validation status for the GEp Hall C reboot

**Status:** current synthesis, 25 October 2026

## Purpose

This document records what the completed NPS data and simulation work establishes, while keeping it separate from the unresolved raw-trigger and DAQ questions. Detailed chronological studies are preserved in the [NPS simulation-development archive](archive/nps_simulation_development_2026-10-25/README.md).

## Established capabilities

- Real SIMC elastic events can be serialized with their weights, vertices, and post-target electron vectors and transported through the maintained NPS GEANT4 geometry.
- The Hall-to-GEANT4 coordinate mapping and the SIMC/GEANT4 material boundary have been tested explicitly. Target and cell material already traversed in SIMC are disabled in the external-primary GEANT4 mode.
- Crystal-level energy deposition, shower sharing, 3x3 local sums, cluster position, fiducial response, and threshold retention can be studied event by event.
- NPS/DVCS data provide empirical checks of reconstructed elastic response, resolved secondary clusters, decoded pulses, waveform behavior, sparsification effects, and current-dependent occupancy.
- A hard-bremsstrahlung `gamma p -> pi0 p`, `pi0 -> gamma gamma` background generator and common elastic-selection variables have been implemented and exercised.

## Valid conclusions with limited proposal scope

- The run-6151 and run-2875 studies are useful validations of NPS response and the SIMC-GEANT4 workflow, but their selected background rates are not raw GEp trigger-rate predictions.
- Resolved extra clusters and decoded in-time pulses in the same 3x3 region produced negligible changes to the modeled elastic threshold efficiency in the studied samples. This does not bound unsparsified subthreshold analog activity or the global NPS trigger rate.
- The inherited detector geometry and passive-material choices materially affect acceptance and response tails and must remain explicit in production configurations.
- A 3x3 trigger sum contains less energy than the full reconstructed shower. Thresholds must therefore be expressed in the actual VTP trigger-energy convention rather than only as a percentage of incident electron energy.
- Final-state bremsstrahlung photons near the outgoing electron direction may contribute to the same calorimeter cluster; transporting only the post-radiation electron can overstate radiative threshold losses.

## Superseded numerical baselines

- The earlier 8.43-million-event hybrid projection is not the current proposal yield. The run-17193 data anchor gives 12.053 million events at its own kinematics after applying the measured data/SIMC factor of 0.7734 once. A matched high-statistics SIMC calculation gives a Q2 = 11 / run-17193 yield ratio of 0.88647, reducing the original-kinematics projection to 10.685 million before the NPS response requirement. The new GEANT4 sample projects approximately 7.70 million events at a 70% incident-energy threshold and 7.51 million at 80%, using a 3x3 sum and the working fiducial. Those response-reduced values retain important threshold, radiative-photon, and passive-geometry caveats.
- Offline-selected accidental or correlated-background rates at the hertz level must not be used for online-trigger planning. The current HMS S1-only baseline is approximately 0.8--1 MHz after simple GEp luminosity scaling, making raw accidental coincidences potentially kHz to tens of kHz depending on NPS rate and coincidence width.
- The 3 m NPS position is a reasonable design standard but remains subject to acceptance-matching optimization.

## Electron-run NPS primitive-rate benchmark

Runs 6158, 6165, and 6168 used NPS as an electron detector with the sweeper off. They share a 6.370 GeV beam, 10 cm LH2 target, approximately 25.6 microamp beam current, 4.598 GeV central electron energy, and 9.5 m NPS distance. Their central NPS angles are 21.17, 19.87, and 18.56 degrees. Unbiased 1 microsecond EDTM windows give the following direct full-face primitive rates after converting each run through its own measured VTP/offline-energy relation:

The EDTM normalization is independently validated at the actual 1.6 GeV VTP hardware threshold. With the same BCM4A current cut of at least 20 microamps, the EDTM/scaler ratios are 0.991 for run 6158, 1.011 for run 6165, and 0.983 for run 6168. The agreement is within 1.7% in every run across direct pTRIG1 rates from 25.69 to 52.60 kHz.

![EDTM occupancy closure against the pTRIG1 scaler](plots/nps_electron_edtm_scaler_closure.png)

This closure establishes that the 1 microsecond occupancy conversion and channel selection reproduce the hardware primitive before the method is used to scan higher thresholds.

At the same 1.6 GeV VTP setting, corresponding to approximately 1.46 GeV offline or 30.1% of the proposal electron energy, the acceptance-weighted 75 microamp, 30 cm projection is 652 +/- 40 kHz for the exponential angular model and 774 +/- 46 kHz for the power-law model. With the 806 kHz HMS S1 primitive, these imply 10.52 +/- 0.65 and 12.48 +/- 0.73 kHz of accidentals at 20 ns, respectively; the 40 ns values are twice as large. The quoted uncertainties propagate scaler snapshot RMS and do not include the angular-model choice or beam-energy transfer.

| NPS angle | 70% = 3.387 GeV | 75% = 3.629 GeV | 80% = 3.870 GeV |
|---:|---:|---:|---:|
| 21.17 degrees | 1.26 +/- 0.17 kHz | 0.89 +/- 0.14 kHz | 0.55 +/- 0.11 kHz |
| 19.87 degrees | 2.44 +/- 0.24 kHz | 1.76 +/- 0.20 kHz | 1.08 +/- 0.16 kHz |
| 18.56 degrees | 3.76 +/- 0.32 kHz | 2.70 +/- 0.27 kHz | 2.01 +/- 0.23 kHz |

The monotonic factor-of-three angular dependence demonstrates why neither luminosity scaling alone nor addition of the three overlapping views is appropriate.

The 30 by 36 crystal active face is approximately 0.615 by 0.738 m. At the proposed 26.651-degree central angle and 3 m distance it covers polar angles from about 20.82 degrees at the lowest-angle edge to 33.14 degrees at the corners. A first acceptance-weighted extrapolation fits the three measured, face-integrated rates with either an exponential or power-law angular density and integrates that density over the proposed face. Scaling from 10 to 30 cm LH2 and to 75 microamps gives:

| Physical threshold | Exponential model | Power-law model | S1 accidental at 20 ns | S1 accidental at 40 ns |
|---:|---:|---:|---:|---:|
| 70% | 23.0 kHz | 28.2 kHz | 0.37--0.45 kHz | 0.74--0.91 kHz |
| 75% | 16.2 kHz | 20.0 kHz | 0.26--0.32 kHz | 0.52--0.64 kHz |
| 80% | 7.6 kHz | 9.4 kHz | 0.12--0.15 kHz | 0.24--0.30 kHz |

The accidental columns use the 806 kHz projected HMS S1X AND S1Y primitive. The model spread is not a confidence interval. Only the lowest-angle edge of the proposed face overlaps the calibration coverage, and the calibration beam energy is substantially lower than the proposal beam energy. These values are therefore a data-anchored working bracket, not a final proposal prediction. A generator validated against these electron data is still needed to transfer the spectrum in beam energy and angle.

![Electron-run angular benchmark and GEp acceptance-weighted extrapolation](plots/nps_electron_gep_acceptance_projection.png)

*Points are the measured EDTM-window rates. Color denotes physical threshold; solid and dashed curves are exponential and power-law fits to the three face-integrated measurements. The shaded band is the polar-angle range sampled by the proposed 3 m face. The right panel shows the integrated and luminosity-scaled working bracket.*

The numerical inputs and reproducible projection are retained in [the measured threshold key points](nps_electron_edtm_angle_key_points.csv), [the GEp projection table](nps_electron_gep_acceptance_projection.csv), and [the acceptance-weighting script](model_nps_electron_gep_acceptance.py).

### Run-1918 photon-arm cross-check

Run 1918 used NPS as a real-photon detector in DVCS rather than as an electron detector. Its unbiased EDTM occupancy still reproduces the measured `pTRIG1` scaler within 3.1%, validating the primitive channel and occupancy method. Its earlier luminosity-scaled values of 175.5, 140.0, and 117.0 kHz at the three thresholds describe a different detected particle and production spectrum and must not be substituted for the electron-singles benchmark above.

### Separate elastic-core efficiency check

Run 6151 addresses a different question: how efficiently would a high threshold retain clean elastic electrons? It is not used to derive the electron-singles rate benchmark above. Run 6151 provides a proton-tagged elastic core with 4.598 GeV electrons, close to the 4.838 GeV proposal energy. Applying its separate calibration, `E_VTP = 0.2230 + 0.9112 E_offline`, gives:

| Physical threshold fraction | Run-6151 elastic-core trigger efficiency |
|---:|---:|
| 70% | 99.3% |
| 75% | 98.8% |
| 80% | 96.5% |

![Run-6151 elastic-core trigger efficiency](plots/nps_elastic_efficiency_vs_threshold.png)

*This is a conditional trigger efficiency for the selected, non-radiative elastic core. It is not the efficiency for the full radiative SIMC distribution or the NPS geometric fiducial, and it should not replace the lower full-sample GEANT4 retention in the yield estimate.*

![High-statistics Q2 = 11 NPS trigger-sum response](plots/gep_q11_nps_30cm_3m_10M_trigger_sum.png)

*The high-statistics GEANT4 sample shows total and 3x3 shower response, threshold efficiency, and the hottest-crystal distribution. The working selected-yield projections use the red `x > -140 mm` fiducial-plus-3x3 efficiency curve. That fiducial is not applied to the full-face primitive or accidental-rate tables above.*

## Unresolved critical-path measurements

1. Validate an inclusive electron generator against the three measured electron-run angular curves, then use it to transfer the primitive spectrum to the proposal beam energy and integrate the proposed 3 m face.
2. Establish the effective end-to-end HMS-NPS online coincidence width.
3. Determine whether HMS S1/S2 3-of-4 logic can be retained with the proposed FPP configuration and measure its elastic efficiency.
4. Establish sustainable Hall C DAQ rate, event-size dependence, deadtime, and livetime measurement conventions.
5. Optimize NPS distance, choosing the largest distance consistent with solid-angle matching to the HMS elastic envelope while tracking yield, edge losses, resolution, occupancy, and trigger rate.
6. Complete the FPP figure-of-merit and final precision projection.

These questions, rather than further refinement of offline-selected DVCS background rates, control proposal readiness.
