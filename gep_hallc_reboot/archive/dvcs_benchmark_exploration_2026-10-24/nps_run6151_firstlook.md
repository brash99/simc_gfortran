# NPS run 6151 private-replay first look

Established 2026-10-23 from private replays of events 1-10,000 and 1-100,000 in raw segment 0.

## Reproducible inputs

Run 6151 belongs to the Hall C NPS Run Group 1a 2023-2024 campaign. More specifically, it is an elastic-calibration run acquired during the E12-13-010 DVCS running period, not a DVCS production run. The [NPS performance analysis](https://indico.jlab.org/event/946/contributions/16503/attachments/12589/20053/NPS_Performance.pdf) places it at the start of the 6151-6168 calibration group: 3-pass beam, 4.598 GeV elastic-electron energy, 2.639 GeV/c HMS central proton momentum, and 9.5 m target-to-calorimeter distance. The [Run Group 1a program](https://wiki.jlab.org/cuawiki/index.php/NPS_Experiments) also served other approved NPS experiments, so describing this merely as an “NPS RG1a run” would be less precise.

The private ifarm workspace is `/work/hallc/gep/brash/gep4_nps_run6151_private`. The replay uses hcana 1.1, Podd 1.7.12, ROOT 6.30.04, official NPSlib commit `fddb8e9d5355212510b8e2f5b5e2fffc8ce0f3bd`, and NPS replay commit `267ca6fbdeb15a1f96b6f4653c9d066d7af51159`.

The verified replay products are:

- `nps_replay_develop/ROOTfiles/COIN/PRODUCTION/nps_hms_coin_6151_1_10000.root`: 64 MB and 10,000 physics-tree entries;
- `nps_replay_develop/REPORT_OUTPUT/COIN/coin_NPS_HMS_report_6151_10000.report`: 17 KB; and
- the event-137 settings 1600 cluster-trigger, 350 cluster-readout, and 800 pair-trigger threshold units.

The checked-in analysis is [`analyze_nps_run6151.C`](analyze_nps_run6151.C). Its complete numerical products are [`nps_run6151_10k_summary.txt`](nps_run6151_10k_summary.txt) and [`nps_run6151_10k_threshold_scan.csv`](nps_run6151_10k_threshold_scan.csv).

A second replay extends the same contiguous sample to 100,000 physics events. Its 831 MB ROOT file has exactly 100,000 tree entries, and the same unchanged macro produced [`nps_run6151_100k_summary.txt`](nps_run6151_100k_summary.txt), [`nps_run6151_100k_threshold_scan.csv`](nps_run6151_100k_threshold_scan.csv), and updated diagnostic plots. The 100k results supersede the 10k numbers for quantitative use; the smaller sample remains a reproducibility and stability cross-check.

## EDTM separation

The replay cut definition identifies an accepted EDTM event by `T.hms.hEDTM_tdcTimeRaw != 0`. On this definition, the 100k sample contains:

| Category | Events | Mean BCM4A current |
| --- | ---: | ---: |
| Ordinary physics | 38,325 | 23.29 microampere |
| EDTM | 61,675 | 14.36 microampere |

The scaler-time branch spans 1,543.331 s. Dividing accepted counts by that span gives 24.83 Hz ordinary events and 39.96 Hz EDTM events in the recorded stream. These are merely accepted-count densities over the wall-clock span, not underlying trigger or detector singles rates: the trigger prescales, live time, beam-current history, and scaler definitions have not been unfolded.

All 38,325 ordinary events have at least one decoded VTP cluster, and 38,082 have an offline cluster. By contrast, 40,269 EDTM events have VTP cluster objects but only 240 have an offline cluster. This striking difference requires care: VTP banks contain every qualifying firmware object, and the offline and firmware definitions and timing windows differ.

## Firmware versus offline energy

For 37,974 ordinary events having both maxima above 1.5 GeV, an event-level comparison gives:

- mean nominal-VTP/offline energy ratio: 0.9674;
- RMS of that ratio: 0.0739;
- Pearson correlation: 0.8895; and
- unweighted fit `E_VTP = 0.2230 + 0.9112 E_offline`, with energies nominally in GeV.

This supports the documented approximate mapping of 1,000 VTP integer units per GeV for run 6151, but it is not a final channel-by-channel trigger calibration. It compares the largest firmware and offline clusters per event rather than geometrically matched clusters, and the nonzero intercept and six-percent ratio spread matter for a sharp threshold.

The VTP coordinates are zero-based column/row indices. Converting them with the replay geometry (`x = -31.32 + 2.16 column`, `y = -37.80 + 2.16 row`, in cm) validates the convention directly. Among 38,082 ordinary events with both cluster types, the largest-cluster residuals have means of -0.002 cm in x and +0.009 cm in y, with RMS values 1.17 and 1.26 cm. Fully 38,004 events (99.80%) agree within 4 cm. This establishes that event-maximum comparisons are normally geometrically genuine, while leaving a small mismatched or multi-cluster population for dedicated study.

![Run 6151 VTP and offline diagnostics](plots/nps_run6151_100k_diagnostics.png)

The PDF version is [here](plots/nps_run6151_100k_diagnostics.pdf). The original 10k [PNG](plots/nps_run6151_10k_diagnostics.png) and [PDF](plots/nps_run6151_10k_diagnostics.pdf) are retained for comparison.

## Threshold scan

The table below counts ordinary physics events whose largest firmware cluster exceeds each threshold. “Elastic fraction” means the fraction of the 38,325 ordinary recorded events; it is not an independently background-subtracted calorimeter efficiency.

| VTP threshold | Fraction of 4.598 GeV | Ordinary events | Elastic fraction | EDTM events |
| ---: | ---: | ---: | ---: | ---: |
| 1.6 GeV | 34.8% | 38,274 | 99.87% | 798 |
| 3.2 GeV | 69.6% | 34,572 | 90.21% | 43 |
| 3.5 GeV | 76.1% | 32,726 | 85.39% | 21 |
| 3.7 GeV | 80.5% | 30,210 | 78.83% | 15 |
| 4.0 GeV | 87.0% | 21,394 | 55.82% | 6 |

This directly supports Andrew's expectation that a 70-80% threshold is operationally plausible, while showing that the upper end of that range is not free: in this setting the nominal 3.7 GeV threshold loses about 21% of the ordinary elastic-trigger sample. A nominal 3.5 GeV threshold retains about 85%. The larger sample also corrects the initial 10k impression that no EDTM events survive: small high-energy tails are present. Their observed count densities are 0.0136/s at 3.5 GeV and 0.00972/s at 3.7 GeV over the wall-clock span, but these are not yet absolute accidental-background rates because beam current, live time, and trigger sampling vary.

The threshold fractions are stable between the nested samples: the 10k retentions were 85.07% at 3.5 GeV and 77.90% at 3.7 GeV, compared with 85.39% and 78.83% at 100k. The firmware/offline mean energy ratio similarly changes only from 0.9638 to 0.9674.

## Proton-tagged elastic core

A second checked-in macro, [`analyze_nps_run6151_elastic.C`](analyze_nps_run6151_elastic.C), uses the HMS proton to define an elastic-enriched sample independently of the VTP energy. It requires an ordinary event, exactly one HMS track, `|H.gtr.dp| < 8%`, and at least one offline NPS cluster. The predicted scattered-electron energy is

`E_e_expected = E_beam + M_p - sqrt(p_HMS^2 + M_p^2)`,

with the run energy 6.37042664 GeV. Of 38,325 ordinary events, 35,338 have exactly one HMS track, 33,505 also pass the HMS momentum-acceptance cut, and 33,406 additionally have an offline NPS cluster.

An iterative Gaussian fit to `E_NPS - E_e_expected` gives a core mean of -0.226 GeV and sigma of 0.202 GeV. A provisional three-sigma core from -0.832 to +0.380 GeV contains 28,327 events. The negative offset is a measured feature of this replay and must not be silently interpreted as intrinsic calorimeter resolution: calibration, containment, radiative loss, and the asymmetric low-energy tail all contribute. The core definition is useful for measuring firmware-threshold behavior, but it is not yet a final elastic-selection efficiency.

| Nominal VTP threshold | Fraction of 4.598 GeV | All HMS-track/NPS events | Offline elastic-core events | Core retention |
| ---: | ---: | ---: | ---: | ---: |
| 3.5 GeV | 76.1% | 29,501 | 27,770 | 98.03% |
| 3.7 GeV | 80.5% | 27,403 | 26,309 | 92.88% |

The key distinction from the inclusive scan is that the elastic-core sample is selected with offline energy and proton kinematics, while the table tests the independent firmware sum. The much higher 98.0% and 92.9% retentions show that a large part of the apparent inclusive loss at 3.5-3.7 GeV comes from the low-energy/radiative/nonelastic population rather than the fitted elastic core. This is encouraging for a 70-80% trigger threshold, although a final efficiency still needs position-dependent calibration, a radiative-tail convention, and an unbiased denominator.

![Run 6151 proton-tagged elastic diagnostics](plots/nps_run6151_100k_elastic_diagnostics.png)

The vector version is [here](plots/nps_run6151_100k_elastic_diagnostics.pdf), and the complete numerical output is [`nps_run6151_100k_elastic_summary.txt`](nps_run6151_100k_elastic_summary.txt). The cluster-time distribution contains more than one structure, so no timing cut was imposed. The mean-residual map also shows spatial structure at the calorimeter scale; both observations motivate dedicated timing and position-calibration studies rather than an arbitrary cut in this first pass.

## Within-run current dependence and pileup proxies

[`analyze_nps_run6151_rate.C`](analyze_nps_run6151_rate.C) divides the same proton-tagged sample into bands of the event-latched BCM4A scaler current. The run does not provide a smooth controlled current scan: most accepted events lie in bands near 20.0, 25.5, and 27.4 microampere, while the lowest-current bin includes beam transitions. Therefore the stable bands are the meaningful comparison.

| Mean current | Candidates | Fitted residual mean | Fitted sigma | VTP efficiency at 3.5 GeV | VTP efficiency at 3.7 GeV | Offline multi-cluster fraction | Mean VTP/offline energy |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 20.00 μA | 8,977 | -0.222 GeV | 0.194 GeV | 98.01% | 92.84% | 17.78% | 0.9654 |
| 25.48 μA | 16,215 | -0.209 GeV | 0.189 GeV | 98.04% | 92.90% | 22.00% | 0.9672 |
| 27.37 μA | 6,044 | -0.222 GeV | 0.191 GeV | 97.98% | 92.89% | 22.95% | 0.9679 |

Across 20-27 microampere, the elastic peak moves by at most 13 MeV, its fitted width remains within about 3%, the firmware/offline scale changes by only 0.25%, and the two threshold efficiencies are constant to roughly 0.1 percentage point. There is consequently no evidence in this sample for a current-dependent degradation of the central elastic energy response or high-threshold efficiency over that range.

The offline probability of reconstructing more than one cluster rises from 17.8% to 22.9%, a relative increase of about 29%. That is the first concrete rate-dependent effect and is qualitatively consistent with increasing accidental activity or pileup. It is not yet uniquely attributable to pileup: these are accepted triggered events, the current is a latched scaler value, and changes in beam conditions or event mixture can produce the same trend. The VTP-object multiplicity is saturated at essentially 100% above one object in the stable-current bands and is not a useful pileup discriminator in this decoded format.

![Run 6151 within-run current dependence](plots/nps_run6151_100k_rate.png)

The vector plot is [here](plots/nps_run6151_100k_rate.pdf); the complete table is [`nps_run6151_100k_rate.csv`](nps_run6151_100k_rate.csv), with run-level metadata in [`nps_run6151_100k_rate_summary.txt`](nps_run6151_100k_rate_summary.txt). The most direct follow-up is to measure the energy, time, and spatial separation of the additional offline clusters versus current. Out-of-time or spatially uncorrelated extra clusters that grow linearly with current would provide much stronger evidence for accidental pileup than multiplicity alone.

A subsequent [survey of the other NPS elastic-calibration settings](nps_calibration_current_survey.md) identifies run 2875 as the best immediately available higher-current sample: a stable plateau near 38.2 microampere in the first 5,000 events. Runs 2855 and 3883 also contain plateaus near 38 microampere but have substantial lower-current intervals. Run 2875 has a different 7.072 GeV elastic-electron setting, so it is a rate-dependence cross-check rather than a kinematically identical control.

The resulting [secondary-cluster rate study](nps_secondary_cluster_study.md) finds a 27.98% multi-cluster fraction in run 2875 at 38.27 microampere, compared with 20.62% in run 6151 at 25.48 microampere. The secondary clusters are less often spatially near the elastic cluster, while their in-time fraction remains near 13%, favoring added accidental activity over simple shower splitting. Different electron energies and run periods prevent treating this as a universal pileup curve.

## Scaler-rate caution

The replay report quotes 1,231,001 `hTRIG1` scaler counts at 12.808 kHz, 13,667 `hTRIG3` counts at 0.142 kHz, and 4,384 `hTRIG4` counts at 0.046 kHz. It also quotes 10,069 physics triggers at 0.102 kHz and a BCM4A beam-cut current of 13.752 microampere. These rates use about 96-99 s of beam-cut clock rather than the full 194.221 s wall-clock span, consistent with substantial beam-off time in the subsample.

The partial replay's reported computer-live-time diagnostics are internally unphysical: several per-trigger values are negative, while the EDTM value is 100.45%. This most likely reflects applying report formulas intended for a complete run to an event-limited replay, not a physical negative dead time. Therefore the report's raw scaler rates may be used as descriptive operating conditions, but its partial-run live-time corrections must not be used in a GEp-IV projection.

The embedded VTP configuration sets trigger prescales 0, 3, and 4 to one and
the others to zero. The subsequent source/configuration and scaler audit in
[`nps_trigger_rate_study.md`](nps_trigger_rate_study.md) establishes the Hall C
channel meanings: `hTRIG1` is the NPS VTP trigger, `hTRIG3` is HMS 3/4, and
`hTRIG4` is HMS EL-REAL. Stable-current scaler snapshots, rather than the
partial-run live-time calculation, are used there for the first 75-microampere
envelope.

## Next checks before a GEp-IV rate projection

1. Decode scaler increments and trigger prescales over stable-current intervals to obtain actual accepted and raw trigger rates; do not use the partial-replay live-time report.
2. Confirm the run-specific VTP energy calibration and extend the validated position match to time-matched firmware/offline clusters.
3. Characterize additional offline clusters versus current using their energy, time, and spatial separation from the proton-tagged elastic cluster; test whether the excess is out of time or spatially uncorrelated as expected for accidental pileup.
4. Refine the proton-tagged elastic selection by modeling the radiative tail, resolving the cluster-time structures, and correcting the observed position dependence.
5. Repeat with dedicated current-scan runs and, ideally, a production setting with an electron energy close to the proposed 4.838 GeV.
6. Only then extrapolate the measured firmware-cluster spectrum to GEp-IV luminosity and scan the 70-80% threshold region.

The intended final architecture is a simulation/data hybrid: use run data to determine firmware response, threshold turn-on, spatial nonuniformity, current dependence, and pileup constraints, while using the GEp-IV SIMC plus maintained NPS GEANT4 chain to supply the different signal and background phase space at the proposed kinematics.

