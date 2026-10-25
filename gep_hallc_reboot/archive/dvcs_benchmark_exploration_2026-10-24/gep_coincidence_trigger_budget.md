# GEp-IV NPS x HMS experimental trigger-rate envelope

Checked: 2026-10-24.

## Trigger question

The primary proposal trigger considered here is a hardware coincidence between
a high-threshold NPS cluster and `HMS S1X AND S1Y`. A second candidate is the
standard S1/S2 three-of-four hodoscope trigger, if the FPP can be configured to
retain or reproduce the required downstream hodoscope information. An HMS
calorimeter condition is a third possible refinement.

## Data anchors

The run-6151 scaler tree contains direct measurements of `S1X`, `S1Y`, and a
channel named `S1XS1Y` in the ROOT tree and `S1T` in the scaler map. Fits over
15--30 microampere extrapolate to 75 microampere as follows:

| Primitive | Rate at 75 microampere |
| --- | ---: |
| S1Y plane | 20.0 kHz |
| S1X plane | 23.1 kHz |
| S1T = S1X OR S1Y | 41.3 kHz |

Hall C trigger documentation defines the existing front-pair signal as
`S1 = S1X OR S1Y` (and the standard time-of-flight primitive as
`STOF = (S1X OR S1Y) AND (S2X OR S2Y)`). This resolves the scaler behavior:
the `S1XS1Y` ROOT branch / `S1T` scaler-map channel is the S1 OR, not an X--Y
coincidence.

Because all three rates are measured, the intended X--Y overlap can be derived
without treating the planes as statistically independent:

```text
R(S1X AND S1Y) = R(S1X) + R(S1Y) - R(S1X OR S1Y)
                = 1.77 kHz at 75 microampere and 10 cm LH2.
```

Linear target-length scaling gives 5.32 kHz for the proposed 30 cm target. The
proton-tagged run-6151 sample has both S1 TDC signals in 98.64% of events, so
this is already a plausible efficient proton primitive, subject to a proper
timing-aligned trigger-efficiency measurement.

A timing-aligned measurement on 28,327 elastic-core events from the full
100,000-event replay finds the corrected S1X--S1Y peak at -1.875 ns. The
efficiency relative to the full selected elastic sample is:

| Full S1X--S1Y window | Efficiency |
| ---: | ---: |
| 6 ns | 86.64% |
| 10 ns | 98.42% |
| 20 ns | 98.53% |

The 20 ns result is within 0.11 percentage point of the 98.64% both-planes-
present ceiling. Increasing the full window from 10 to 20 ns recovers only
0.11 percentage point, while 6 ns is clearly too narrow. Thus 10 ns is already
near the practical efficiency plateau and 20 ns supplies modest timing margin.

The direct run-6151 fit for the existing S1/S2 three-of-four trigger is 607 Hz
at 75 microampere and 10 cm LH2, or 1.82 kHz after the same provisional target
scaling. Run 6151 is a useful low-momentum proton anchor, but it is not the
only proton-side evidence and should not be assigned exact proposal-kinematics
status.

### Direct positive-polarity cross-check: run 2875

Run 2875 is the closest available independent trigger validation with the HMS
in positive polarity: 10.539 GeV beam, approximately 4.04 GeV/c protons in
HMS, a 10 cm LH2 target, and NPS at 9.5 m. On the stable 38.266-microampere
plateau its scaler rates were:

| Scaler | Run-2875 rate |
| --- | ---: |
| NPS Trigger 1 | 223.026 kHz |
| HMS S1/S2 three-of-four (Trigger 3) | 158.064 Hz |
| NPS AND HMS three-of-four (Trigger 5) | 55.699 Hz |
| HMS EL-REAL (Trigger 4) | 47.299 Hz |
| NPS AND HMS EL-REAL (Trigger 6) | 42.415 Hz |

The Fall-2023 trigger definitions make these coincidences unambiguous:
Trigger 1 is the NPS VTP trigger, Trigger 3 is HMS three-of-four, Trigger 4 is
HMS EL-REAL, and Triggers 5 and 6 are their respective coincidences with
Trigger 1. Run 2875 used unprescaled Trigger 5, so its approximately 56 Hz
Level-1 rate is a direct observation of an NPS--proton coincidence—not a
product-of-singles estimate. It is dominated by its elastic calibration
topology and low 1.4 GeV VTP threshold, and therefore is a validation of scale
and trigger meaning rather than a direct projection to the proposal setting.

Run-2875 scaler fits give a derived `S1X AND S1Y` rate of approximately 1.26
kHz at 38.266 microampere. Simple current and target-length scaling gives 7.43
kHz at 75 microampere and 30 cm. Its measured three-of-four rate scales to
0.929 kHz. Compared with the corresponding run-6151 projections of 5.32 and
1.82 kHz, the two proton settings define practical HMS-rate brackets:

| Proton primitive | Run-2875 projection | Run-6151 projection |
| --- | ---: | ---: |
| S1X AND S1Y | 7.43 kHz | 5.32 kHz |
| S1/S2 three-of-four | 0.929 kHz | 1.82 kHz |

The settings differ in proton momentum, spectrometer acceptance, run period,
and trigger configuration. The spread is consequently carried as a systematic
bracket; neither endpoint is claimed to be an exact proposal rate.

The stronger NPS-singles anchor is production run 1918. Its setting matches the
Fall-2023 `x60-3` configuration: 10.558 GeV beam, 5.878 GeV/c HMS momentum,
and NPS at 3 m. The run report identifies an LH2 target, 27.583 microampere
beam-cut current, and `hTRIG1 = 497.396 kHz`. The Fall-2023 trigger table defines
Trigger 1 as the NPS trigger, and the production run plan specifies a 500 MeV
NPS cluster threshold. Thus this is an empirical NPS singles measurement at
the relevant distance, not an inverse-square extrapolation from 9.5 m.

An independent audit of the 187 scaler snapshots found a stable current of
28.78 microampere and rates of 520.6 kHz for Trigger 1, 2.469 kHz for HMS
three-of-four, 344.4 Hz for Trigger 5, 1.866 kHz for HMS EL-REAL, 276.1 Hz for
Trigger 6, and 276.0 Hz for Level 1. Prescales 1--5 were disabled and prescale
6 was one. The active recorded path was therefore the unprescaled
NPS--EL-REAL coincidence. The raw run configuration also confirms a 1.4 GeV
VTP cluster-trigger setting and a 20 ns VTP pulse width. These audited values
cross-check the trigger interpretation; the spectrum projection below retains
the run-report normalization used when its CSV was generated.

Scaling run 1918 linearly in current to 75 microampere gives 1.352 MHz for its
10 cm target; scaling to the proposed 30 cm target gives **4.057 MHz at the
500 MeV threshold**. This assumes the Fall-2023 10 cm cryotarget and linear
luminosity scaling.

A private 100,000-event replay of run 1918 now supplies the missing 3 m
spectrum. For clusters above 1.5 GeV, the event-level calibration is

```text
E_VTP = 0.4638 GeV + 0.6682 E_offline.
```

The intended offline thresholds of 3.4, 3.6 and 3.9 GeV therefore correspond
to nominal VTP cuts of 2.736, 2.869 and 3.070 GeV. Log interpolation of the
0.1-GeV scan gives two deliberately separate normalizations:

| Offline threshold | EDTM-tagged NPS rate | Accepted-physics NPS rate |
| ---: | ---: | ---: |
| 3.4 GeV | 89.3 kHz | 323 kHz |
| 3.6 GeV | 74.1 kHz | 263 kHz |
| 3.9 GeV | 59.3 kHz | 195 kHz |

Both columns are scaled to 75 microampere and 30 cm from the measured 497.396
kHz run-1918 trigger rate. The EDTM-tagged spectrum is the better ambient-rate
proxy, but EDTM pulses can overlap physics triggers and are not a perfectly
pure random sample. The accepted-physics spectrum is intentionally retained as
an upper cross-check because it is correlated with the trigger. This
59--323 kHz data-driven bracket replaces the earlier 2.68--3.55 MHz geometric
upper envelope. Run 1918 had negative HMS polarity and detected electrons, so
its multi-kilohertz HMS rates must **not** be used as the proton-side proposal
anchor.

## Coincidence rates

For independent NPS and S1 inputs and full coincidence width `Delta t`,

```text
R_acc = R_NPS R_S1 Delta t.
```

At a 20 ns full window:

| NPS threshold | NPS x (S1X AND S1Y) | NPS x S1/S2 3-of-4 |
| ---: | ---: | ---: |
| 3.4 GeV | 13.3 Hz (48.0 Hz upper check) | 1.66 Hz (6.0 Hz) |
| 3.6 GeV | 11.0 Hz (39.1 Hz upper check) | 1.38 Hz (4.9 Hz) |
| 3.9 GeV | 8.8 Hz (28.9 Hz upper check) | 1.10 Hz (3.6 Hz) |

The table uses the run-2875 proton primitive for the central calculation. The
rates scale linearly with the effective coincidence width: halve these values
for 10 ns and double them for 40 ns. Applying the full run-6151/run-2875 HMS
bracket moves the ambient estimates to approximately 6--13 Hz for S1X AND S1Y
and 1--3 Hz for three-of-four. The few-hertz elastic rate and all true
inelastic correlated coincidences add to these accidentals. The
accepted-physics NPS spectrum gives the deliberately conservative values in
parentheses; it is not a pure random sample and should not be labeled the best
estimate.
The first proposal-kinematics hard-bremsstrahlung `gamma p -> pi0 p`
calculation gives **0.24--0.47 Hz** for the baseline model over the
3.4--3.9 GeV threshold range, with a full `s^-8` to `s^-6` model envelope of
**0.13--0.90 Hz**. It is material relative to the approximately 2.2 Hz elastic
trigger, but it does not dominate the preferred accidental estimate. Details
and limitations are in
[`q11_correlated_trigger_projection.md`](q11_correlated_trigger_projection.md).

## Four-nanosecond CEBAF microstructure

At 4 ns bunch spacing the beam supplies 250 million buckets/s. For example,
an NPS rate of 74.1 kHz corresponds to a mean probability about 0.000296 per
bucket, while the run-2875-based 7.43 kHz S1X--S1Y projection corresponds to
about 0.0000297 per bucket. Their same-bucket accidental rate is then about
2.20 Hz. A 20 ns coincidence accepts approximately five relative bucket
positions, giving about 11.0 Hz.

Thus the bunch structure does not add another factor beyond the assumed 20 ns
window when the inputs are independent: explicitly counting five 4 ns bucket offsets
reproduces `R_NPS R_S1 Delta t`. It does make the physical interpretation
clear—many accepted triggers pair particles from different buckets. Correlated
same-interaction backgrounds must be added separately and cannot be computed
from the product of singles rates. The NPS VTP documentation specifies a 20 ns
output-pulse width, but the effective overlap width after any downstream
counting-house stretching or alignment has not yet been established. The
20 ns results are therefore a reference scenario, not a verified final
hardware gate.

## HMS calorimeter option

The same scaler data give 75-microampere extrapolations of approximately 165
Hz for `PRLO`, 121 Hz for `PRHI`, and 120 Hz for `SHLO`. If an S1-plus-calorimeter
primitive had a rate no larger than 165 Hz at the run-6151 target, scaling it
linearly to 495 Hz for 30 cm would give less than approximately 0.6--3.2 Hz of
accidentals across the data-driven NPS-rate bracket in 20 ns.

The presently configured calorimeter primitives are not acceptable proton
triggers. In the run-6151 proton-tagged elastic core, event-level TDC presence
is only 15.2% for `PRLO`, 10.4% for `PRHI`, and 12.1% for `SHWR`.

A direct reconstructed-energy threshold scan strengthens that conclusion. In
the first 1,664 elastic-core events, more than 5% have `H.cal.etot = 0`, and
more than 10% have zero preshower energy. Consequently no positive threshold
on either reconstructed quantity can reach even 95% efficiency, much less the
approximately 98% desired for a primary proton trigger. These reconstructed
energies are not identical to analog trigger sums, but the zero-energy tails
make a calorimeter-assisted primary trigger unattractive unless the detector
readout or geometry changes materially.

## PAC-level conclusion

At a 20 ns effective window, using run 2875 as the central positive-polarity
proton anchor gives **9--13 Hz of accidentals for NPS x (S1X AND S1Y)** and
**1--2 Hz for NPS x three-of-four**. Including the approximately 2.2 Hz elastic
rate and the baseline pi0 calculation gives central total rates of about
**11--16 Hz** and **3.5--4.5 Hz**, respectively. The trigger-correlated NPS
spectrum expands those totals to approximately **31--51 Hz** and **6--9 Hz**.
These are commissioning envelopes, not statistical confidence intervals.

The earlier 8.7--12.2 Hz S1X--S1Y total and 4.6--6.0 Hz three-of-four total
were based only on the run-6151 HMS extrapolation. They are superseded by the
two-setting proton bracket and the direct run-2875 coincidence check above.
The result remains well below a kilohertz because the 3.4--3.9 GeV NPS
threshold reduces the projected NPS singles rate from roughly 4 MHz at the
loose 500 MeV threshold to 59--89 kHz, and because the proton primitive is an
X--Y or three-of-four coincidence rather than a single plane or S1 OR. At the
loose NPS threshold, the same 20 ns product already approaches 0.6 kHz for
S1X AND S1Y; an S1 OR or a longer effective gate can readily produce
kilohertz-level rates.

The three-of-four option is therefore worth carrying as a serious alternate
trigger, not merely as a cross-check. Its feasibility question is detector and
layout specific: identify which downstream timing plane(s) remain available in
the proposed FPP configuration, then demonstrate efficiency and timing using a
GEp-like proton sample.

The most important remaining measurements and refinements are:

1. run a combined commissioning test at 3 m with positive-polarity,
   high-momentum protons and the intended 3.4--3.9 GeV NPS threshold;
2. measure the effective counting-house coincidence width, including all
   pulse stretching and alignment, rather than assuming it equals the 20 ns
   VTP output width;
3. measure the timing-aligned efficiency of S1X AND S1Y for elastic protons;
4. document Andrew's proposed FPP configuration for retaining an S1/S2
   three-of-four primitive and evaluate its proton efficiency;
5. pass the pi0 photons through the maintained crystal-level Geant4/VTP-like
   trigger-sum model; and
6. treat an HMS calorimeter condition as a fallback only if a modified detector
   configuration eliminates the observed zero-energy proton tail.

## Reproduction

- [`analyze_hms_trigger_primitives.C`](analyze_hms_trigger_primitives.C)
  extracts the scaler fits into
  [`hms_trigger_primitive_fits.csv`](hms_trigger_primitive_fits.csv).
- [`analyze_hms_proton_trigger_efficiency.C`](analyze_hms_proton_trigger_efficiency.C)
  checks primitive presence in the proton-tagged elastic core; results are in
  [`hms_proton_trigger_efficiency.csv`](hms_proton_trigger_efficiency.csv).
- [`analyze_hms_s1xy_timing.C`](analyze_hms_s1xy_timing.C) measures the
  timing-aligned X--Y efficiency; the full result is in
  [`hms_s1xy_timing_efficiency_full.csv`](hms_s1xy_timing_efficiency_full.csv).
- [`analyze_hms_calorimeter_threshold.C`](analyze_hms_calorimeter_threshold.C)
  produces the proton-retention scan in
  [`hms_calorimeter_proton_thresholds_10k.csv`](hms_calorimeter_proton_thresholds_10k.csv).
- The run-1918 replay products used for the calibrated 3 m spectrum are
  [`nps_run1918_100k_summary.txt`](nps_run1918_100k_summary.txt) and
  [`nps_run1918_100k_threshold_scan.csv`](nps_run1918_100k_threshold_scan.csv).
- [`model_gep_coincidence_budget.py`](model_gep_coincidence_budget.py) produces
  [`gep_coincidence_trigger_budget_75uA.csv`](gep_coincidence_trigger_budget_75uA.csv),
  the original run-6151 HMS/window sensitivity grid. The PAC-level central
  values above replace its single-setting interpretation with the independently
  audited run-2875 proton anchor; the CSV should not be read as the final
  combined envelope.
- [`model_q11_correlated_trigger.py`](model_q11_correlated_trigger.py) produces
  [`q11_correlated_trigger_projection.csv`](q11_correlated_trigger_projection.csv).
