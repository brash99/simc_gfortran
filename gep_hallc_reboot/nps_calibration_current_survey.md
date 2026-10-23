# NPS elastic-calibration current survey

Established 2026-10-23 from uniform private replays of the first 5,000 events in selected raw segment-0 files.

## Purpose and provenance

The initial run-6151 study found stable central NPS response from about 20 to 27 microampere but a rising offline multi-cluster fraction. This survey asks whether another NPS elastic-calibration run provides a higher-current comparison.

The [NPS elastic-performance analysis](https://indico.jlab.org/event/946/contributions/16503/attachments/12589/20053/NPS_Performance.pdf) identifies ten calibration settings spanning the experiment. The private replay uses the same hcana 1.1, Podd 1.7.12, ROOT 6.30.04, official NPSlib `fddb8e9d...`, and replay `267ca6fb...` environment established for run 6151. All outputs remain under `/work/hallc/gep/brash/gep4_nps_run6151_private`.

[`survey_nps_calibration_currents.C`](survey_nps_calibration_currents.C) excludes accepted EDTM events and calculates exact current quantiles from `H.BCM4A.scalerCurrent`. The event-latched scaler current describes the current associated with accepted events; it is not an integrated-charge or unbiased live-time measurement.

## Runs 6151-6168

All 18 raw files were available and produced nonempty 5,000-event ROOT files. Runs 6152-6168 generally occupy a narrow 25-26 microampere plateau. None contains an ordinary event at or above 30 microampere in the survey sample. Run 6151 itself has the widest current range, but its maximum is only 29.67 microampere.

Complete results are in [`nps_calibration_run_current_survey.csv`](nps_calibration_run_current_survey.csv).

## Other elastic settings

Representative cached raw files were available for six other calibration groups:

| Run | Calibration group | Elastic-electron energy | Median current | Central 80% current interval | Plateau estimate | Fraction at least 30 μA |
| ---: | --- | ---: | ---: | ---: | ---: | ---: |
| 6172 | 6171-6183 | 5.681 GeV | 20.46 μA | 20.11-20.68 μA | 20.25 μA | 0.0% |
| 5217 | 5217-5236 | 4.598 GeV | 29.65 μA | 27.76-29.93 μA | 29.75 μA | 5.6% |
| 5183 | 5183-5208 | 7.072 GeV | 29.20 μA | 21.40-29.52 μA | 29.25 μA | 0.0% |
| 3883 | 3883-3898 | 7.072 GeV | 33.23 μA | 13.67-38.71 μA | 37.75 μA | 58.7% |
| 2875 | 2875-2885 | 7.072 GeV | 38.10 μA | 37.52-38.92 μA | 38.25 μA | 100.0% |
| 2855 | 2855-2871 | 7.072 GeV | 37.07 μA | 20.26-38.10 μA | 37.25 μA | 65.5% |

Complete numerical results are in [`nps_cross_setting_current_survey.csv`](nps_cross_setting_current_survey.csv).

## Interpretation and next run

Run 2875 is the best immediately available higher-current candidate. Unlike runs 2855 and 3883, its first 5,000-event sample is entirely above 30 microampere and has a compact current distribution centered near 38.2 microampere. It is therefore well suited to a larger replay and the same extra-cluster energy/time/separation analysis planned for run 6151.

It is not a kinematically identical control. Run 2875 belongs to the 7.072 GeV elastic-electron setting, compared with 4.598 GeV for run 6151. Shower energy, NPS occupancy, HMS acceptance, trigger configuration, calibration pass, and possible run-period changes can all affect cluster multiplicity. The useful comparison is therefore not simply “17.8% versus some run-2875 percentage.” We should measure pileup-sensitive extra-cluster properties in both datasets and, where possible, compare events in matched current bands or model their dependence on current within each setting.

That follow-up is now complete in the [NPS secondary-cluster rate study](nps_secondary_cluster_study.md). The 100,000-event run-2875 replay confirms excellent elastic-core VTP retention at fractional thresholds and a higher multi-cluster probability near 38 microampere. Run 2855 is statistically consistent but small; run 3883 has nearly saturated multi-cluster reconstruction and is not comparable without a run-configuration audit.
