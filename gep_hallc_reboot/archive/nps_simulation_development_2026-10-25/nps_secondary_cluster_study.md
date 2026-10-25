# NPS elastic response and secondary-cluster rate study

Established 2026-10-23 from private hcana replays of NPS elastic-calibration runs 6151, 2875, 2855, and 3883.

## Run 2875 elastic response

Run 2875 was replayed through 100,000 events using the same private hcana 1.1/NPSlib environment as run 6151. The verified ROOT file is 889 MB and contains exactly 100,000 tree entries. The run database gives a 10.53941793 GeV beam, 4.0872 GeV/c HMS central momentum, and 7.072 GeV nominal elastic-electron energy.

The generalized [`analyze_nps_run6151_elastic.C`](analyze_nps_run6151_elastic.C) uses the HMS proton to predict the electron energy and tests VTP thresholds at the same fractions used for run 6151:

| Quantity | Run 2875 result |
| --- | ---: |
| Ordinary events | 25,504 |
| One HMS track, `|dp| < 8%`, and NPS cluster | 16,519 |
| Fitted elastic-core mean | +0.119 GeV |
| Fitted elastic-core sigma | 0.212 GeV |
| Events in provisional three-sigma core | 10,577 |
| VTP retention at 76.1% = 5.382 GeV | 99.63% |
| VTP retention at 80.5% = 5.693 GeV | 97.62% |

![Run 2875 proton-tagged elastic response](plots/nps_run2875_100k_elastic_diagnostics.png)

The [PDF](plots/nps_run2875_100k_elastic_diagnostics.pdf) and [numerical summary](nps_run2875_100k_elastic_summary.txt) are retained. As for run 6151, the fitted core width includes calibration, containment, radiative, spatial, and selection effects and is not an intrinsic NPS resolution measurement. The opposite-signed residual offsets in runs 6151 and 2875 also warn against transferring a single absolute energy correction across settings.

## Secondary-cluster definition

[`analyze_nps_secondary_clusters.C`](analyze_nps_secondary_clusters.C) applies the same ordinary-event, one-track, `|dp| < 8%`, NPS-cluster, and fitted elastic-core requirements. In each accepted event, the largest-energy NPS cluster is the primary elastic candidate. If another cluster exists, the analysis records the highest-energy secondary and its:

- energy;
- time relative to the primary;
- spatial separation from the primary; and
- dependence on the event-latched BCM4A current.

Three categories are diagnostic labels rather than final cuts: “energetic” means at least 0.2 GeV, “in time” means `|delta t| < 4 ns`, and “near” means separation below 10 cm.

## Runs 6151 and 2875

The cleanest populated current bands give:

| Run and setting | Mean current | Elastic-core events | Multi-cluster fraction | Mean secondary energy | Energetic fraction | In-time fraction | Near fraction | In-time and near |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 6151, 4.598 GeV electron | 25.48 μA | 13,834 | 20.62% | 0.310 GeV | 45.0% | 13.3% | 54.4% | 7.7% |
| 6151, 4.598 GeV electron | 27.37 μA | 5,191 | 21.48% | 0.310 GeV | 46.8% | 13.2% | 54.7% | 7.9% |
| 2875, 7.072 GeV electron | 38.27 μA | 10,825 | 27.98% | 0.589 GeV | 60.6% | 12.6% | 44.7% | 6.6% |

From the 25.5-microampere run-6151 band to the 38.3-microampere run-2875 plateau, the multi-cluster probability increases by 7.36 percentage points, or about 36% relative. The fraction of secondary clusters close to the primary falls by about ten percentage points, while the in-time fraction remains near 13%. Qualitatively, that is more consistent with adding spatially uncorrelated activity than with an increase in split pieces of the same shower.

The secondary energy also increases, but this cannot be assigned to current: run 2875 has a substantially larger elastic energy and belongs to an earlier run period. Differences in clustering thresholds, gains, trigger configuration, detector state, and background phase space may all contribute. Thus the comparison supports—but does not uniquely prove—a pileup/accidental interpretation.

### Common energetic, far, and out-of-time definition

To reduce sensitivity to negligible reconstructed clusters and shower splitting, the analysis also counts elastic-core events whose highest secondary cluster satisfies all three conditions: energy at least 0.2 GeV, separation at least 10 cm, and `|delta t|` at least 4 ns.

| Run | Mean current | Elastic-core events | Energetic secondary | Energetic and far | Energetic and out of time | Energetic, far, and out of time |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 6151 | 25.48 μA | 13,834 | 9.29% | 8.15% | 7.92% | 7.11% |
| 6151 | 27.37 μA | 5,191 | 10.06% | 8.53% | 8.51% | 7.46% |
| 2875 | 38.27 μA | 10,825 | 16.95% | 13.83% | 14.78% | 12.30% |

The strictest common fraction therefore rises from roughly 7.1-7.5% at 25-27 microampere to 12.3% at 38 microampere, approximately a 65% relative increase compared with the 27.37-microampere point. This is a more pileup-specific result than raw cluster multiplicity: the selected activity is energetic, well separated from the elastic shower, and outside the narrow same-time category.

![Run 6151 secondary clusters](plots/nps_run6151_100k_secondary.png)

![Run 2875 secondary clusters](plots/nps_run2875_100k_secondary.png)

Vector plots and complete tables are retained as [`run 6151 PDF`](plots/nps_run6151_100k_secondary.pdf), [`run 2875 PDF`](plots/nps_run2875_100k_secondary.pdf), [`run 6151 CSV`](nps_run6151_100k_secondary.csv), and [`run 2875 CSV`](nps_run2875_100k_secondary.csv).

## Within-run checks from runs 2855 and 3883

The available 5,000-event samples provide limited but useful diagnostics:

- Run 2855 has only 170 elastic-core events in its well-populated 37.20-microampere band. Its multi-cluster fraction there is 24.1%, broadly consistent with the 28.0% run-2875 result but statistically and systematically weaker. The lower-current transition bins contain too few events for a precise slope.
- Run 3883 is qualitatively incompatible with the other runs: its multi-cluster fraction is already 59.7% below 15 microampere and exceeds 92% above 15 microampere. This is not credible as a simple continuation of the run-6151/2875 current trend. It indicates a materially different reconstruction, detector, or acquisition condition and should not be used as a pileup cross-check until that difference is identified.

The tighter common definition does not rescue run 3883. At 38.14 microampere, 46.55% of its elastic-core events contain an energetic, far, out-of-time secondary, versus 12.30% in run 2875. This confirms that the anomaly is physically present in the reconstructed secondary activity rather than being caused only by tiny extra clusters.

## Replay-configuration audit

All four private replays load the same current replay choices:

- `nps_cal_clustering = 1` from `PARAM/NPS/CAL/nps_cuts_eel108.param`;
- a 5 ns neighbor time window;
- the same 110-190 ns block ADC-time window; and
- the same `PARAM/NPS/CAL/nps_calib_nps23.param` calibration coefficients.

Consequently, the run-3883 anomaly is not explained by the replay selecting a different clustering algorithm or explicit clustering window for that run. However, applying one modern NPS23 calibration file to several historical run periods is itself a major limitation. It can distort energy thresholds and response comparisons, and it does not guarantee that waveform/readout behavior was equivalent. A defensible run-3883 interpretation now requires either recovering its period-appropriate calibration/replay convention or constructing a raw block-level comparison with common pulse-amplitude and timing definitions.

![Run 2855 secondary clusters](plots/nps_run2855_5k_secondary.png)

![Run 3883 secondary clusters](plots/nps_run3883_5k_secondary.png)

The corresponding [run-2855 CSV](nps_run2855_5k_secondary.csv), [run-3883 CSV](nps_run3883_5k_secondary.csv), [run-2855 PDF](plots/nps_run2855_5k_secondary.pdf), and [run-3883 PDF](plots/nps_run3883_5k_secondary.pdf) preserve the complete results.

## Present conclusion

The data establish three useful facts:

1. The central elastic response and high fractional VTP-threshold efficiency remain strong at the 38-microampere run-2875 plateau.
2. Offline multi-cluster probability increases between the best populated 25-27 and 38-microampere samples.
3. The added run-2875 clusters are not preferentially close or in time with the elastic shower, which favors accidental activity over simple shower splitting.

The result is evidence for rate-dependent accidental/pileup activity, not yet a universal quantitative pileup model. A dedicated same-kinematics current scan—or larger samples from a run group containing stable plateaus at multiple currents—is still needed to disentangle current from run-period and energy dependence.

The proposed [decoded block-pulse comparison](nps_raw_block_pulse_study.md) is now complete for equal 5,000-event samples of runs 6151, 2875, and 3883. It shows that the run-3883 anomaly is already present before secondary-cluster reconstruction: at the common 10 mV, 10 cm, and 8 ns working point, 98.66% of run-3883 elastic-core events near 38.14 microampere contain an isolated out-of-time pulse, versus 28.89% in run 2875 near 38.20 microampere. Run 3883 is anomalous even below 15 microampere.

The subsequent waveform-level test finds a matching stored waveform packet for every candidate and a coherent pulse shape for 97.02% of run-3883 candidates, essentially the same as 97.53% in run 2875. Thus the excess consists of genuine structures in the saved FADC waveforms, not secondary-cluster artifacts or unsupported pulse-finder candidates.

The replay report resolves the discrepancy: run 3883 had FADC250 sparsification disabled, whereas run 2875 and every other available comparison replay had it enabled. Full-detector waveform readout exposes all 1,080 channels to the low sample-pulse threshold and naturally produces the dense low-amplitude population seen in run 3883. Moreover, beam exceeded 2 microampere for only 31.6% of its sampled interval. Run 3883 is therefore a useful configuration diagnostic but is excluded from the quantitative pileup/current comparison.
