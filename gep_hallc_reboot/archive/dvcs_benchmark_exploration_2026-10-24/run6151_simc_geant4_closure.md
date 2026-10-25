# Run 6151 SIMC--GEANT4 trigger closure

Checked: 2026-10-24.

## Closure target

Run 6151 is an elastic-calibration run acquired during the Hall C E12-13-010
DVCS running period.  The actual-condition simulation uses a 6.37042664 GeV
beam, 10 cm LH2 target, HMS proton setting of 2.639 GeV/c at 35.97 degrees,
and NPS at a replay-database central angle of 21.17 degrees and 9.5 m.  The
4.598 GeV elastic electron lies at approximately 20.58 degrees; that kinematic
angle must not be substituted for the calorimeter's mechanical central angle.

Two measured rates must be kept distinct:

- The 100,000-event replay contains 38,325 ordinary events over 1,543.331 s,
  or **24.83 Hz averaged over the replay wall-clock interval**.
- In the stable 22.5--26.5 microampere scaler band, the mean current is
  25.474 microampere, `hTRIG5` is 85.265 Hz, and EDTM is 39.950 Hz.  Their
  difference, **45.315 Hz**, is the corresponding beam-on physics-trigger
  scaler rate under the present channel interpretation.

SIMC predicts yield per accumulated charge.  Its nominal-current output should
therefore first be compared with the second quantity.  Reproducing 24.83 Hz
also requires the run's beam interruptions and DAQ/readout acceptance.

## Elastic simulation

[`nps_run6151_elastic_closure.inp`](../infiles/nps_run6151_elastic_closure.inp)
generated one million radiative elastic trials.  There were 2,514 contributing
events and an integrated yield of 2,108.3645 events/mC.  These events were
transported through the maintained NPS GEANT4 model with the target and cell
vacuumized at the established SIMC--GEANT4 material boundary.  Slurm job
13547749 completed successfully and wrote 2,514 detector events.

[`analyze_run6151_elastic_trigger_closure.C`](analyze_run6151_elastic_trigger_closure.C)
forms a hottest-block 3x3 crystal sum and applies the SIMC run-level yield
normalization.  The raw event weights stored in the GEANT4 ROOT tree are
relative weights; their sum is not itself the yield in events/mC.

| NPS 3x3 threshold | Elastic efficiency | Yield (events/mC) | Rate at 25 uA |
| ---: | ---: | ---: | ---: |
| 0.0 GeV | 100.00% | 2,108.36 | 52.71 Hz |
| 1.6 GeV | 98.52% | 2,077.08 | 51.93 Hz |
| 3.2 GeV | 96.32% | 2,030.79 | 50.77 Hz |
| 3.5 GeV | 93.87% | 1,979.22 | 49.48 Hz |
| 3.7 GeV | 88.71% | 1,870.41 | 46.76 Hz |

The numerical output is
[`nps_run6151_elastic_trigger_closure.csv`](nps_run6151_elastic_trigger_closure.csv).

### Actual-geometry correction

A subsequent audit against `standard_coin.kinematics` and the NPS replay
geometry found that the first pass centered SIMC/GEANT4 on the 20.58-degree
elastic ray rather than the actual 21.17-degree NPS central angle.  It also
used a 2.05 cm pitch rather than the replay geometry's 2.16 cm pitch.  The
corrected active-face half dimensions are 32.4 by 38.88 cm.

Repeating both SIMC and GEANT4 with the corrected geometry gives 1,751.82
events/mC before threshold and 1,709.29 events/mC after the 1.6 GeV 3x3
threshold (97.57% efficiency).  At the measured 25.474 microampere current,
the corrected elastic prediction is **43.54 Hz**, compared with the measured
beam-on physics-trigger rate of **45.315 Hz**.  Thus the earlier 52.91 Hz
number is superseded.  The remaining approximately 1.77 Hz is not by itself a
background measurement: correlated backgrounds, accidentals, and residual
trigger/detector efficiency must be treated consistently.

## Interpretation

The elastic prediction being modestly above the provisional beam-on total does
not imply that the data contain no inelastic or accidental triggers.  The
measured rate is a sum of physics components after component-dependent trigger,
detector, and DAQ efficiencies.  Before fitting that ledger, the missing
physics components are being simulated independently.

### First correlated-background component

[`nps_run6151_pi0phot_closure.inp`](../infiles/nps_run6151_pi0phot_closure.inp)
simulates hard bremsstrahlung followed by `gamma p -> pi0 p` and isotropic
`pi0 -> gamma gamma` decay.  It uses the measured Anderson angular data with
the documented `s^-7` extrapolation, a provisional mean upstream radiator of
0.00573 radiation lengths for the 10 cm LH2 target, and the actual run-6151
HMS/NPS geometry.  In 100,000 fixed-seed trials, 6,065 events pass HMS
transport and place at least one decay photon in the NPS rectangular
acceptance.  The acceptance-folded yield is **22.097 events/mC**, corresponding
to **0.563 Hz at 25.474 microampere** before the NPS trigger threshold.

Both decay photons were subsequently transported in the same GEANT4 event,
with the physical target and cell enabled because the photons are generated at
the reaction vertex.  Slurm job 13557800 completed all 6,065 events in 3:06
with exit code 0.  Applying the same hottest-block 3x3 sum as the elastic
sample gives:

| NPS 3x3 threshold | Pi0 trigger efficiency | Yield (events/mC) | Rate at 25.474 uA |
| ---: | ---: | ---: | ---: |
| 1.6 GeV | 77.01% | 17.018 | 0.4335 Hz |
| 3.2 GeV | 21.07% | 4.656 | 0.1186 Hz |
| 3.5 GeV | 10.19% | 2.252 | 0.0574 Hz |
| 3.7 GeV | 4.63% | 1.023 | 0.0261 Hz |

The analysis is
[`analyze_run6151_pi0_trigger_closure.C`](analyze_run6151_pi0_trigger_closure.C),
and the numerical output is
[`nps_run6151_pi0_trigger_closure.csv`](nps_run6151_pi0_trigger_closure.csv).
At the actual 1.6 GeV threshold this modeled channel contributes about 0.96%
of the provisional 45.315 Hz beam-on physics-trigger rate.  This is the first
correlated component, not the sum of all inelastic channels.

The legacy SIMC `doing_delta` mode was explicitly rejected for normalization:
`physics_delta.f` discards its calculated cross section and assigns the
placeholder value `1.0`.  A run-6151 smoke test consequently produced an
unphysical normalization and was not passed to GEANT4.

### Accidental component

[`simulate_run6151_accidentals.py`](simulate_run6151_accidentals.py) samples
independent NPS and HMS candidates in the five 4 ns CEBAF buckets covered by
a 20 ns coincidence window.  At the measured 25.474-microampere plateau it
uses 24.566 kHz NPS singles and 233.806 Hz HMS 3/4 singles.  The fixed-seed
ten-million-HMS-trigger simulation gives **0.11398 +/- 0.00164 Hz**, consistent
with the exact bucket-Poisson result of **0.11484 Hz** and the continuous-time
limit of 0.11487 Hz.  The numerical record is
[`run6151_accidental_simulation.csv`](run6151_accidental_simulation.csv).

This baseline accidental simulation uses the actual 1.6 GeV NPS scaler rate.
Threshold-dependent accidentals require the corresponding unprescaled NPS
singles spectrum; recorded coincidence-event retention is not a substitute.

## Remaining closure work

The following must still be placed on a common footing:

1. verify whether `hTRIG5 - EDTM` is the correct pre-deadtime physics-trigger
   comparator and recover the full-run trigger, prescale, and live-time ledger;
2. normalize the recorded ordinary-event count to charge as well as wall time;
3. apply the measured HMS trigger efficiency and the run-specific NPS trigger
   response consistently;
4. quantify the weighted Monte Carlo uncertainty and increase the 2,514-event
   detector sample if required; and
5. validate the nominal central-setting convention against the elastic locus
   across the HMS and NPS acceptances.

Other correlated electroproduction channels can now be added with separately
validated cross-section models.  The pi0 radiator-path and `s^-6`/`s^-8`
model variations must also be propagated.  The component rates must remain
separate until their efficiencies and uncertainties are explicit.
