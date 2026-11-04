# Inclusive NPS background-generator development

This directory contains the channel contract and the first target-level
generator used to investigate the run-6887 NPS rate that is not explained by
the inclusive scattered-electron calculation.

## First channel: real-photon pi0 production

`generate_pi0phot_singles.py` implements beam-electron bremsstrahlung,
`gamma p -> pi0 p`, and `pi0 -> gamma gamma`.

Unlike SIMC's existing correlated implementation, it imposes no HMS
acceptance. It records a parent event whenever at least one decay photon
intersects the NPS face. The cross section and its `s^-6`, `s^-7`, and `s^-8`
variations are identical to the existing `physics_pi0phot.f` implementation.
The liquid-hydrogen radiator thickness is calculated from the target entrance
to the sampled interaction vertex. Target windows and other upstream material
are not yet included.

Example:

    python3 generate_pi0phot_singles.py \
      --trials 5000000 \
      --output /tmp/run6887_pi0phot_5m.csv

## Initial run-6887 result

Five million common-seed trials gave the following generator-level rates.
The threshold is applied to the sum of the energies of decay photons that
geometrically enter NPS; it is not yet a simulated VTP threshold.

| scaling model | E > 3.387 GeV | E > 3.629 GeV | E > 3.870 GeV |
| --- | ---: | ---: | ---: |
| `s^-6` | 412.8 +/- 4.3 Hz | 310.9 +/- 3.1 Hz | 237.5 +/- 2.3 Hz |
| `s^-7` | 398.1 +/- 4.7 Hz | 284.3 +/- 3.2 Hz | 207.2 +/- 2.3 Hz |
| `s^-8` | 393.3 +/- 5.2 Hz | 265.5 +/- 3.3 Hz | 184.1 +/- 2.2 Hz |

For comparison, the measured run-6887 excess above the GEANT4-folded
inclusive-electron calculation is approximately 48.08, 36.47, and 29.24 kHz
at those thresholds. Ordinary containment losses reduce these numbers;
resolution migration and pileup can move some events upward, but would have
to produce an implausible two-order-of-magnitude enhancement to close the
gap. This particular mechanism is therefore less than about 1.5% of the
residual and cannot be its dominant source under the stated model and
material assumptions.

This is a channel-level negative result, not evidence that photons are
unimportant. The calculation does not yet contain electroproduced pi0/eta
mesons, other inclusive real-photon reactions, direct photons from upstream
material, or charged pions. Those are the next channel classes to audit and
implement.

## Second channel: SIDIS pi0 electroproduction pilot

The existing SIMC `H(e,e' pi0)X` fragmentation model has now been exercised
without an electron-arm transport requirement. The checked-in pilot deck is
`infiles/nps_run6887_sidis_pi0_singles_pilot.inp`. It integrates a broad
scattered-electron phase space, decays each generated pi0 to two photons, and
requires at least one photon to enter the run-6887 NPS face.

The SIDIS model is interpreted only in the explicit region `Q2 > 1 GeV2` and
`W > 2 GeV`. At the run-6887 current, 200,000 fixed trials give:

| incident photon energy on NPS | rate |
| --- | ---: |
| any | 35.95 +/- 0.67 kHz |
| > 3.387 GeV | 1.214 +/- 0.053 kHz |
| > 3.629 GeV | 0.788 +/- 0.038 kHz |
| > 3.870 GeV | 0.519 +/- 0.029 kHz |

These are generator-level incident-energy rates before GEANT4 response. The
high-threshold contribution is only about 1.8--2.5% of the measured excess,
so high-Q2 SIDIS fragmentation is not its dominant source in this pilot.

This result also establishes an important missing region. The existing SIDIS
fit is not defined for the low-Q2, quasi-real-photon events that appeared when
the integration was extended toward the beam direction. That region cannot be
discarded physically: inclusive low-Q2/real-photon hadron production is now a
leading candidate for the missing NPS rate. It requires a photoproduction
model rather than extrapolation of the SIDIS fit.

The numerical result is recorded in `run6887_sidis_pi0_pilot_rates.csv` and
reproduced by `analyze_run6887_sidis_pi0_pilot.py`. A scattered-electron
overlap veto is not needed for this pilot geometry because the generated
electron arm and NPS are on opposite Hall sides. That ownership condition
must be revisited for any more general azimuthal integration.

## Third channel: low-Q2 inclusive pion production (Wiser)

`generate_wiser_background.py` implements the Wiser inclusive-hadron fit as
used by Jefferson Lab's `evgen_inclusive`. This is the channel that the first
two generators did not cover: electron-induced bremsstrahlung followed by
inclusive, predominantly low-Q2 pion photoproduction. The implementation was
checked numerically at fixed kinematic points against the official
`wiser_pion.h` implementation.

Wiser independently fits pi+ and pi-. The baseline physical pi0 parent cross
section is taken to be `(pi+ + pi-)/2`. This is also the prescription left in
a comment in the older Jefferson Lab wrapper. Later `evgen_inclusive` code
uses `2*(pi+ + pi-)`, but does not document whether that factor represents a
parent cross section or a two-photon counting convention. It is therefore
available as the explicit `--pi0-scale evgen` sensitivity variation, not used
as the baseline. That alternative is exactly four times the baseline.

At the run-6887 conditions, two million trials integrated over parent-pion
angles from 0 to 60 degrees give:

| threshold | measured residual | Wiser pi0 incident-energy rate |
| --- | ---: | ---: |
| 3.387 GeV | 48.08 kHz | 77.28 +/- 2.84 kHz |
| 3.629 GeV | 36.47 kHz | 44.11 +/- 1.67 kHz |
| 3.870 GeV | 29.24 kHz | 25.61 +/- 1.02 kHz |

Changing the upper parent-angle limit from 30 to 60 degrees leaves all three
high-threshold rates statistically consistent. This is the first modeled
target-background channel with the correct tens-of-kHz scale. Its threshold
shape is also encouraging: the generator-level rate is above the residual at
the two lower thresholds and close to it at the highest threshold. Detector
response normally moves the Wiser curve downward, so this is not yet a
closure claim. It is a strong justification for passing this sample through
the same GEANT4 and VTP-emulation chain used for inclusive electrons.

The generator also predicts about 59 MHz of pi-minus parents geometrically
intersecting NPS. That is not a predicted calorimeter trigger rate: charged
pions have a very different deposited-energy response, and a high GeV-scale
calorimeter threshold cannot be applied to their incident momentum. Their
trigger contribution must be determined in GEANT4.

The numerical comparison is recorded in `run6887_wiser_pilot_rates.csv`;
run `plot_run6887_background_channels.py` to reproduce the channel plot.

For detector transport, pass `--pi0-output FILE` to the generator. It writes
all events with at least one decay photon geometrically incident on NPS using
the existing strict `simc_geant4_pi0_v1` two-photon interface. The five-million
trial run-6887 production sample contains 88,122 such events. Both photons are
retained in each record so overlapping showers are handled by GEANT4 rather
than by an independent-photon efficiency approximation. The corresponding
farm job is `run_nps_run6887_wiser_pi0_geant4.slurm`.

### Run-6887 Wiser pi0 GEANT4 closure

The 88,122-event production completed in 51 minutes. The event-level audit is
reproduced by `analyze_run6887_wiser_geant4.py` and recorded numerically in
`run6887_wiser_geant4_closure.csv`.

The first incident-energy comparison was not a valid trigger proxy: it added
both decay photons anywhere on the full calorimeter face. At the three quoted
thresholds, 69--75% of that weighted rate consists of two photons hitting
widely separated face positions; none of those pairs is within one nominal
three-crystal width. Replacing the sum with the maximum individual incident
photon reduces the truth rates from 77.57, 44.69, and 25.74 kHz to 30.77,
16.23, and 8.49 kHz.

For events with an individual photon above 3.387 GeV, the main GEANT4 response
population has median `raw 3x3 / incident photon = 0.901`. This directly
supports the expected 8--10% loss outside a 3x3 cluster. A distinct low-response
population is also present: 7.0% of events, carrying 32.5% of the Wiser weight,
deposit less than 10% of the incident-photon energy. Those photons are strongly
concentrated at local face coordinate `x about -27 cm`, where the current
inherited sweeper/passive geometry shadows the beam-side edge. This shadow
must be checked against the authoritative run-6887 sweeper position before it
is treated as a measured-configuration acceptance loss. The single GEANT4
navigation warning also occurred in the sweeper volume, although it was
nonfatal and caused no event-count loss.

The resulting rates are:

| threshold | measured residual | maximum photon truth | raw GEANT4 3x3 | corrected GEANT4 3x3 |
| --- | ---: | ---: | ---: | ---: |
| 3.387 GeV | 48.08 kHz | 30.77 kHz | 6.66 +/- 0.28 kHz | 2.69 +/- 0.13 kHz |
| 3.629 GeV | 36.47 kHz | 16.23 kHz | 3.02 +/- 0.14 kHz | 1.17 +/- 0.06 kHz |
| 3.870 GeV | 29.24 kHz | 8.49 kHz | 1.33 +/- 0.07 kHz | 0.519 +/- 0.034 kHz |

The raw and corrected columns bracket the unresolved inherited `1.04^2`
energy-scale convention; they must not be averaged. Under the baseline
physical pi0 normalization, low-Q2 Wiser pi0 is therefore a real but
subdominant contribution to the measured residual after local-cluster and
geometry effects. The calculation does not close the run-6887 background by
itself. Charged pions and other inclusive photoproduction channels remain
separate outstanding contributions.

The first pi0 production job accidentally invoked the vacuum-target generic
hybrid option rather than the target-on pi0 option. This omitted downstream
target/cell interactions and cannot create the missing rate.  That deficiency
has now been removed by the separated target-phase-space calculation below.

### Separated target transport and NPS transport

The Wiser sample has now been repeated with generation and transport fully
separated.  The 88,122 weighted Wiser events are first transported through the
physical 10 cm LH2 cell.  All target-exit particles retain their parent event
identifier, so correlated photons are kept together.  The target-exit sample
is then projected to the run-6887 NPS face and passed through the full NPS
GEANT4 calculation.

The target-exit full-face incident-energy integral remains large:

| threshold | target-exit full-face incident-energy rate |
| --- | ---: |
| 3.387 GeV | 75.794 kHz |
| 3.629 GeV | 43.408 kHz |
| 3.870 GeV | 25.197 kHz |

These are not trigger rates.  They apply the threshold to the summed incident
energy of all particles crossing the full calorimeter face in an event.  A
local NPS trigger instead thresholds the hottest 3x3 cluster.  After the NPS
detector simulation the rates are:

| threshold | raw GEANT4 3x3 | corrected GEANT4 3x3 |
| --- | ---: | ---: |
| 3.387 GeV | 6.046 +/- 0.266 kHz | 2.546 +/- 0.122 kHz |
| 3.629 GeV | 2.893 +/- 0.137 kHz | 1.091 +/- 0.057 kHz |
| 3.870 GeV | 1.288 +/- 0.067 kHz | 0.461 +/- 0.030 kHz |

The target-on result is consistent with, and slightly below, the earlier
direct-transport pilot.  The large reduction from full-face incident energy
to a local trigger is driven by the spatial separation of the two decay
photons, detector containment and threshold migration, and the present
passive/sweeper geometry.  The further difference between the raw and
corrected 3x3 columns is the unresolved inherited `1.04^2` energy convention.

This completes the two-phase calculation for the baseline Wiser-pi0 channel.
It does **not** complete a fully inclusive GEANT4 target-background generator.
The unbiased beam-electron target test is computationally stable, but a
100,000-electron sample produced no useful NPS-intersection statistics because
the relevant phase space is too rare.  A biased or importance-sampled
generation scheme, with absolute-weight bookkeeping and a production-cut
convergence scan, remains required.

### Charged-pion transport interface

`--pim-output` writes strict `simc_geant4_hadron_v1` records for pi-minus
parents directed at NPS. The adapter accepts PDG -211, verifies the pion mass
shell, transforms Hall coordinates explicitly, and fires the charged pion
from its generated target vertex. The farm command uses
`--simc-hadron-events`, which retains the physical downstream target and cell.
For the run-6887 high-threshold pilot, only pions with total energy at least
2.5 GeV are written; lower-energy pions cannot supply the studied deposited
energy in the absence of pileup. Five million trials produce 39,603 transport
events.

The completed target-on GEANT4 transport shows that this very large incident
pi-minus flux does not translate into a GeV-scale NPS cluster-trigger rate.
The results are:

| threshold | incident-pion truth rate | raw GEANT4 3x3 | corrected GEANT4 3x3 |
| --- | ---: | ---: | ---: |
| 3.387 GeV | 122.0 +/- 2.4 kHz | 37.98 +/- 10.14 Hz | 19.05 +/- 7.10 Hz |
| 3.629 GeV | 71.17 +/- 1.47 kHz | 19.26 +/- 7.10 Hz | 6.29 +/- 2.23 Hz |
| 3.870 GeV | 40.81 +/- 0.89 kHz | 6.43 +/- 2.23 Hz | 5.14 +/- 2.13 Hz |

The median hottest-3x3 energy is only about 6% of the incident pion energy.
Thus pi-minus production is decisively negligible for the run-6887
high-threshold excess: even the larger raw response is below 0.04 kHz, versus
29--48 kHz of residual data. This closes charged pi-minus as a candidate for
the dominant missing component under the present single-particle transport
model. It does not test accidental pileup involving multiple sub-threshold
particles.

The first full job exposed two legacy transport hazards. The executable reads
four stdin records (output filename, Ranecu table index, and two seeds), while
the inherited job inputs supplied only two; this left the seeds uninitialized
and produced an invalid `(0,0)` RNG state. The adapter now fails fast on
missing/invalid seed input and the Wiser jobs provide explicit reproducible
seeds. Charged secondaries can also curl or become trapped indefinitely in the
sweeper/passive geometry. A prompt-study transport bound now kills a track
only after 100 ns global time, 20 m track length, or 100,000 steps. These
limits are far beyond the roughly 12 ns direct flight to NPS. The formerly
pathological ten-event seed then completed with 178 MB rather than exhausting
8 GB.

## Verification

Run `python3 test_generate_pi0phot_singles.py`. The tests reproduce the
original SIMC cross-section reference at the `3e-9 nb/GeV2` level, check all
three scaling variations, and verify the NPS face-coordinate intersection.

The SIDIS pilot additionally exposed a mathematical-domain defect in
`semi_physics.f`: an unphysical negative square-root argument in the modified
fragmentation variable was allowed to propagate as a NaN weight. The model
now rejects that event. The remaining non-finite events occur only below
`Q2 = 0.17 GeV2`, outside the stated SIDIS interpretation, and are explicitly
excluded and counted by the analysis rather than entering the quoted rates.

Run `python3 test_generate_wiser_background.py` for the low-Q2 generator. It
checks fixed pi+ and pi- cross sections against the official Jefferson Lab
implementation, forbidden kinematics, pi0 decay four-momentum conservation,
and the NPS face intersection.

