# NPS singles and HMS--NPS accidental-rate baseline

Checked: 2026-10-23.

## Scope

This study separates three quantities that must not be conflated:

1. the raw NPS local-sum candidate rate presented by the VTP;
2. the prescaled NPS-singles stream accepted by the DAQ for monitoring; and
3. the HMS--NPS coincidence rate, including random coincidences.

Prescaling can make the recorded singles stream manageable, but it does not
reduce crystal occupancy, VTP processing load, or candidates presented to the
coincidence logic. The present calculation therefore starts from unprescaled
scaler rates rather than accepted event counts.

## Trigger-channel identification

The private run-6151 replay contains 775 entries in its `TSH` scaler tree.
The run-specific `thms_nps23.param` defines the coincidence inputs as
`hTRIG1 hTRIG3 hTRIG1 hTRIG4`. The maintained NPSlib coincidence code labels
those inputs explicitly:

| Scaler channel | Run-6151 meaning |
| --- | --- |
| `hTRIG1` | NPS VTP trigger |
| `hTRIG3` | HMS 3/4 trigger |
| `hTRIG4` | HMS EL-REAL trigger |

NPSlib independently labels VTP trigger type 0 as “production cluster
singles.” These source-level definitions resolve the ambiguity left in the
initial replay report. The decoded event audit also finds 39,067 accepted
events containing a type-0 VTP flag, but accepted-event frequencies are not
used as raw rates.

## Measured stable-current rates

The scaler snapshots give the following means. The RMS values describe the
variation among snapshots, not an uncertainty on the mean.

| Mean current | Snapshots | NPS VTP `hTRIG1` | HMS 3/4 `hTRIG3` | HMS EL-REAL `hTRIG4` |
| ---: | ---: | ---: | ---: | ---: |
| 19.882 microampere | 144 | 18.746 kHz | 190.9 Hz | 48.3 Hz |
| 25.474 microampere | 211 | 24.566 kHz | 233.8 Hz | 50.8 Hz |
| 27.391 microampere | 76 | 26.595 kHz | 247.2 Hz | 51.2 Hz |

The NPS rate is strikingly linear over the measured 15--30 microampere range:
its correlation with current is 0.9944. A free-intercept fit gives

```text
R_NPS = (-1.940 kHz) + (1.0407 kHz/microampere) I.
```

The corresponding HMS 3/4 fit is

```text
R_HMS = 41.09 Hz + (7.5488 Hz/microampere) I.
```

Extrapolated to 75 microampere these give 76.1 kHz NPS and 607 Hz HMS 3/4.
Through-origin fits give 72.1 kHz and 693 Hz. The spread between these two fit
choices is shown as a simple model variation; it is not the full extrapolation
systematic.

![NPS trigger-rate projection](plots/nps_trigger_projection_75uA.png)

## Accidental coincidence envelope

For independent inputs and a full coincidence window `Delta t`, the leading
random-coincidence estimate is

```text
R_accidental = R_NPS R_HMS Delta t.
```

Using the free-intercept projections:

| Full coincidence window | Accidental HMS--NPS rate |
| ---: | ---: |
| 10 ns | 0.462 Hz |
| 20 ns | 0.924 Hz |
| 40 ns | 1.849 Hz |
| 80 ns | 3.698 Hz |

The through-origin variation gives 0.499, 0.999, 1.997, and 3.995 Hz,
respectively. A roughly 20 ns coincidence window would therefore put the
provisional accidental rate near 1 Hz even with the much looser run-6151 NPS
threshold. This is comparable to, but below, the present 2.169 Hz simulated
GEp-IV elastic rate after the fiducial and 80%-energy cuts. Offline timing and
kinematic cuts would further suppress recorded random coincidences, but they
do not recover trigger/DAQ bandwidth.

## Important boundary: this is deliberately a loose-threshold envelope

Run 6151 used a VTP cluster-trigger threshold of 1,600 nominal units, only
34.8% of its 4.598 GeV elastic-electron energy. The proposed GEp-IV operating
threshold is closer to 70--80% of the elastic energy. Consequently, the
76.1-kHz projection is **not** a prediction of the final GEp-IV NPS trigger
rate. It is a measured, current-scaled loose-threshold envelope and should be
conservative with respect to raising the energy threshold, although the
different beam energy, angle, target length, and radiation field prevent
turning that expectation into a numerical reduction yet.

The extrapolation also extends from a highest stable measured point of 27.4
microampere to 75 microampere. Linearity is well established within run 6151
but not directly measured over that full range. The coincidence calculation
assumes independent input streams and uses the specified full window width;
correlated backgrounds and the eventual trigger logic must be added
separately.

The next useful measurement is therefore a threshold-dependent NPS singles
rate. That requires either scaler data taken with several VTP thresholds, or a
validated data/simulation model of the inclusive NPS local-sum spectrum at the
GEp-IV geometry. Once that spectrum exists, the present scaler normalization
provides the bridge to absolute rates and the coincidence-window scan can be
repeated without using the loose 1,600-unit envelope.

## Reproducibility

The ROOT audit is
[`analyze_nps_trigger_rates.C`](analyze_nps_trigger_rates.C). Its scaler
snapshots, current bands, accepted-event channel audit, and summary are:

- [`nps_run6151_trigger_rates_snapshots.csv`](nps_run6151_trigger_rates_snapshots.csv)
- [`nps_run6151_trigger_rates_bands.csv`](nps_run6151_trigger_rates_bands.csv)
- [`nps_run6151_trigger_rates_event_correlations.csv`](nps_run6151_trigger_rates_event_correlations.csv)
- [`nps_run6151_trigger_rates_summary.txt`](nps_run6151_trigger_rates_summary.txt)

The fit and projection are reproducible with
[`model_nps_trigger_rates.py`](model_nps_trigger_rates.py); numerical outputs
are [`nps_trigger_rate_fits.csv`](nps_trigger_rate_fits.csv) and
[`nps_trigger_projection_75uA.csv`](nps_trigger_projection_75uA.csv). The
vector plot is
[`nps_trigger_projection_75uA.pdf`](plots/nps_trigger_projection_75uA.pdf).
