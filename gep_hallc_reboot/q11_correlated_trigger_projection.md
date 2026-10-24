# Q2=11 correlated trigger projection

Checked: 2026-10-24.

## Physics channel and normalization

PAC37 identifies endpoint hard bremsstrahlung followed by
`gamma p -> pi0 p` as the principal correlated inelastic background whose
recoil proton can enter HMS near the elastic setting. The maintained SIMC
generator models this process with an Anderson-data wide-angle cross section,
a thin-radiator bremsstrahlung spectrum, exact two-body kinematics, isotropic
`pi0 -> gamma gamma`, HMS proton transport, and NPS geometric acceptance.

For the 30 cm LH2 target and the baseline `s^-7` extrapolation, the accepted
normalization before an NPS energy threshold is 13.352 events/mC, or 1.001 Hz
at 75 microampere. Event-by-event reweighting to the prescribed `s^-6` and
`s^-8` variations gives 25.724 and 6.938 events/mC, respectively.

## Hardware-threshold projection

An event passes when at least one decay photon transported into NPS has
generated energy above threshold. The elastic comparator uses the existing
crystal-level hottest-block 3x3 Geant4 threshold scan, interpolated at the same
absolute threshold. Results are before hodoscope inefficiency, DAQ deadtime,
target boiling, and offline elastic-correlation cuts.

| NPS threshold | Correlated pi0, `s^-8` | Baseline `s^-7` | Correlated pi0, `s^-6` | Elastic 3x3 rate | Baseline pi0 / elastic |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 3.4 GeV | 0.242 Hz | 0.467 Hz | 0.905 Hz | 2.248 Hz | 20.8% |
| 3.6 GeV | 0.195 Hz | 0.377 Hz | 0.731 Hz | 2.234 Hz | 16.9% |
| 3.9 GeV | 0.126 Hz | 0.245 Hz | 0.476 Hz | 2.182 Hz | 11.2% |

The baseline correlated background decreases by about a factor of two across
this threshold range, while the modeled elastic trigger rate decreases by only
about 3%. This makes 3.6--3.9 GeV a materially useful trigger-threshold range.

Combining the baseline physical rates with the data-driven ambient accidental
estimate gives the following working hardware budget for a 20 ns coincidence:

| Threshold | Elastic | Correlated pi0 | S1X AND S1Y accidentals | Working total | 3/4 accidentals | Working 3/4 total |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 3.4 GeV | 2.25 Hz | 0.47 Hz | 13.3 Hz | 16.0 Hz | 1.66 Hz | 4.38 Hz |
| 3.6 GeV | 2.23 Hz | 0.38 Hz | 11.0 Hz | 13.6 Hz | 1.38 Hz | 3.99 Hz |
| 3.9 GeV | 2.18 Hz | 0.24 Hz | 8.8 Hz | 11.2 Hz | 1.10 Hz | 3.52 Hz |

This table uses the positive-polarity run-2875 proton primitive as the central
HMS anchor. Run 6151 provides a second proton setting and brackets the central
accidentals at approximately 6--13 Hz for S1X AND S1Y and 1--3 Hz for
three-of-four. The corresponding accepted-physics NPS cross-check remains much
larger: about 29--48 Hz and 3.6--6.0 Hz, respectively. It should remain visible
as a conservative commissioning envelope rather than be added to the central
budget. The assumed 20 ns window is the documented NPS VTP output width; any
additional counting-house stretching remains to be measured.

## Relation to the run-6151 decomposition

The measured run-6151 prompt remainder after elastic-template and accidental
subtraction was 2--3% of its elastic core. It cannot be transferred directly
as a percentage because the elastic and inelastic cross sections, acceptances,
and thresholds change with kinematics. Its recorded absolute scale,
approximately 0.3--0.5 Hz, is nevertheless similar to the independently
modeled Q2=11 baseline pi0 rate of 0.24--0.47 Hz. This is an encouraging scale
comparison, not a normalization constraint: the run-6151 rate contains its
own prescale, live-time, geometry, and any non-pi0 correlated channels.

## Evidence boundary

This is now an absolute proposal-kinematics estimate for the dominant modeled
correlated channel, rather than a direct extrapolation of the run-6151 ratio.
The factor-of-two cross-section envelope is explicit. Important limitations
remain:

- The pi0 threshold is applied to generated accepted-photon energy. It does
  not yet include crystal shower leakage, trigger-sum topology, or merging of
  the two photon showers. Leakage tends to reduce the rate; merging can raise
  it when both photons occupy one trigger neighborhood.
- The `0.0175 X0` radiator is the mean upstream half-target thickness. The
  thin-radiator yield is correct to first order, but vertex-dependent flux and
  acceptance correlations are not yet represented.
- Aluminum windows and correlated electroproduction channels other than the
  modeled hard-bremsstrahlung pi0 process are not included.
- The elastic and pi0 response calculations are not yet passed through the
  exact installed VTP firmware algorithm.

The next high-value refinement is therefore a pi0 SIMC-to-Geant4 export that
transports both photons through the same maintained NPS geometry and evaluates
the same crystal-level 3x3 trigger sum as the elastic sample. The present
0.13--0.90 Hz envelope is adequate for the PAC trigger-rate feasibility
question: even its upper edge is well below the preferred accidental rate.

## Reproduction

[`model_q11_correlated_trigger.py`](model_q11_correlated_trigger.py) reads the
fixed-seed accepted pi0 binary, applies the event-by-event exponent weights,
and interpolates the elastic Geant4 trigger scan. Its complete numerical output
is [`q11_correlated_trigger_projection.csv`](q11_correlated_trigger_projection.csv).
