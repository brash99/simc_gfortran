# NPS decoded block-pulse comparison

## Purpose

The offline secondary-cluster study found a plausible current dependence in runs 6151 and 2875, but run 3883 was far outside that trend. This follow-up tests whether the run-3883 excess is created by calorimeter clustering or is already present in the decoded FADC block pulses.

This is a comparison of **decoded, pedestal-subtracted sample pulses**, not a waveform-level reanalysis. It bypasses secondary-cluster reconstruction but still depends on the hcana FADC pulse finder and its run-period configuration.

## Samples and common definitions

The initial comparison uses the first 5,000 replayed events from each run so that all three samples have the same event limit:

| Run | Beam energy (GeV) | Nominal elastic-electron energy (GeV) | Elastic-core events | Mean latched current (microampere) |
|---:|---:|---:|---:|---:|
| 6151 | 6.370427 | 4.598 | 1,177 | 18.10 |
| 2875 | 10.539418 | 7.072 | 637 | 38.20 |
| 3883 | 10.538911 | 7.072 | 979 | 28.82 |

The event selection is the same proton-tagged elastic core used in the secondary-cluster study: ordinary triggers, exactly one HMS track, `|H.gtr.dp| < 8%`, at least one NPS cluster, and a fitted 3-sigma calorimeter-energy residual window.

For each accepted event:

1. The largest pedestal-subtracted decoded pulse defines the elastic-shower reference block and time.
2. Every other decoded pulse is assigned a relative time and block-to-block separation from that reference.
3. The NPS geometry is taken from the replay's `nps_geom.param`: 36 rows, 30 columns, and 2.16 cm pitch with standard row-major block numbering.
4. An isolated out-of-time pulse has block separation at least 10 cm and `|delta t|` at least 8 ns.
5. Fixed common pulse-amplitude thresholds of 5, 10, 20, and 50 mV are scanned. The 10 mV working point is comfortably above the replay's 4 mV sample threshold.

The array-level checks pass in all three samples: the pulse counter, amplitude, integral, and time arrays always have matching lengths; every block index is within 0--1079; and every selected event has a usable reference pulse.

## Results

At the 10 mV working point, the event fraction containing at least one isolated out-of-time pulse is:

| Run | Mean current (microampere) | Elastic-core events | Event fraction |
|---:|---:|---:|---:|
| 6151 | 9.32 | 404 | 6.44% |
| 6151 | 19.07 | 337 | 12.46% |
| 6151 | 23.84 | 257 | 17.12% |
| 6151 | 27.81 | 179 | 18.44% |
| 2875 | 38.20 | 637 | 28.89% |
| 3883 | 8.78 | 134 | 74.63% |
| 3883 | 18.64 | 130 | 92.31% |
| 3883 | 24.41 | 96 | 97.92% |
| 3883 | 27.82 | 71 | 95.77% |
| 3883 | 32.02 | 99 | 98.99% |
| 3883 | 38.14 | 449 | 98.66% |

The conclusion is insensitive to the precise amplitude threshold. At 38 microampere, run 2875 gives 28.1% at 20 mV and 14.4% at 50 mV, whereas run 3883 gives 77.5% and 25.8%, respectively. Run 3883 also remains anomalous at low current.

The timing plots make the difference especially clear. Runs 6151 and 2875 have a strong narrow shower-time peak with relatively sparse out-of-time pulses. Run 3883 has that same prompt peak superimposed on a dense, nearly continuous population spanning the decoded time window and a broad range of detector positions.

![Run 6151 decoded block pulses](plots/raw_block_probe_6151_5k.png)

![Run 2875 decoded block pulses](plots/raw_block_probe_2875_5k.png)

![Run 3883 decoded block pulses](plots/raw_block_probe_3883_5k.png)

The machine-readable current-bin and threshold scans are in the corresponding CSV files in `plots/`, and the analysis macro is [`probe_nps_block_pulses.C`](probe_nps_block_pulses.C).

## Interpretation

The run-3883 anomaly is already present before secondary-cluster reconstruction. It therefore cannot be explained by the cluster neighbor-time window, cluster merging, or the choice of which reconstructed secondary cluster to inspect.

## Waveform-level validation

The stored waveforms provide the decisive cross-check. Each event's `adcSampWaveform` array is decoded using the established hcana packet format:

```text
block ID, number of samples, sample 0, sample 1, ...
```

The replay stores 110 samples at 4 ns spacing for each NPS block. Every isolated out-of-time pulse passing the common 10 mV, 10 cm, and 8 ns definition was matched to the waveform packet for the same event and block.

| Run | Candidate pulses | Waveform packet found | Coherent waveform | Coherent fraction |
|---:|---:|---:|---:|---:|
| 2875 | 446 | 446 | 435 | 97.53% |
| 3883 | 4,255 | 4,255 | 4,128 | 97.02% |

A coherent waveform requires:

- a pedestal-subtracted local waveform peak at least half the decoded amplitude;
- a peak time within 8 ns of the sample's fitted timing-convention offset; and
- finite neighboring-sample support, quantified by `(left + right)/(2 peak) > 0.05`.

The waveform peak occurs 10.25 ns after the decoded time marker in run 2875 and 10.75 ns after it in run 3883. This common offset is a pulse-time convention, not a run-dependent discrepancy. In both runs, the waveform-to-decoded amplitude ratio is sharply centered near one, and the average normalized pulse shapes agree qualitatively.

![Run 2875 waveform validation](plots/waveform_2875_5k_diagnostics.png)

![Run 2875 waveform examples](plots/waveform_2875_5k_gallery.png)

![Run 3883 waveform validation](plots/waveform_3883_5k_diagnostics.png)

![Run 3883 waveform examples](plots/waveform_3883_5k_gallery.png)

The waveform analysis is implemented in [`analyze_nps_waveform_pulses.C`](analyze_nps_waveform_pulses.C), with text summaries in `plots/`.

The excess run-3883 entries are therefore genuine structures in the stored FADC waveforms, not artifacts introduced by secondary clustering or unsupported pulse-finder candidates. A simple mismatch between the modern pulse finder and old data is strongly disfavored: the finder returns the correct block, amplitude, time relationship, and pulse shape for approximately 97% of candidates in both runs.

## Configuration resolution and run-3883 disposition

The replay reports reveal the decisive configuration difference:

| Run | FADC250 sparsification | Beam above 2 microampere |
|---:|---:|---:|
| 2875 | 1 | 100.0% of the replayed interval |
| 3883 | 0 | 31.6% of the replayed interval |

All other locally replayed comparison runs—2855, 2875, 5183, 5217, and the 6151--6168 series—report sparsification enabled. Run 3883 is the only available sample with sparsification disabled.

This directly explains why run 3883 is qualitatively different. The NPS VTP/FADC design uses the cluster mask to restrict full-waveform readout to a much smaller subset of the 1,080 channels. With sparsification disabled, the event contains waveforms from the full detector rather than only the cluster-centered region. Applying the replay's low 4 mV sample-pulse threshold across all blocks and the full time window naturally produces many coherent low-amplitude threshold crossings. Their existence in the stored waveforms is real, but their multiplicity is a readout-configuration effect and is not comparable to the sparsified runs. See the [VTP manual, section 9](https://coda.jlab.org/drupal/system/files/VTP-Manual_1.pdf) for the NPS sparsification design.

The poor beam duty within the sampled run—only 31.6% above 2 microampere—is a second reason not to use run 3883 as a controlled high-current point. Event-latched current bins do not repair the fundamentally different channel exposure or the frequent beam transitions.

Run 3883 is therefore excluded from the quantitative pileup/current study. It remains useful as a diagnostic demonstration of how strongly full-detector waveform readout changes decoded multiplicity, but it should not be interpreted as evidence for anomalously large physical pileup in the standard sparsified configuration. A logbook entry could clarify why sparsification was disabled, but that history is no longer required to decide comparability.

As before, the smooth run-6151 trend and the run-2875 point are useful evidence for rate dependence, but they are not a universal pileup curve: the runs differ in energy, kinematics, and period, and the current is an event-latched BCM value in an accepted-trigger stream.
