# PAC37 GEp-IV Q² = 11 elastic-baseline reconstruction

Checked: 2026-10-22.

## Purpose and status

This is a controlled reconstruction of the 2011 PAC37 HMS+BigCal elastic point with the current checkout. It is intended to ground the reboot study in the older simulation, not to tune the current code until it reproduces the proposal.

The original PAC37 input deck, generated ranges, source revision, and report have not been recovered. This is therefore not an exact reproduction. For the reboot study, agreement within 20% of the proposal's cut-level yields is accepted as a sufficient historical scale check; recovering the old artifacts is optional rather than a prerequisite.

## Reconstructed setup

The fixed-seed input is [gep_pac37_q11_elastic.inp](../infiles/gep_pac37_q11_elastic.inp). Its main settings are:

- 11.0 GeV beam and elastic hydrogen at Q² = 10.9945 GeV²;
- electron momentum 5.138 GeV/c at 25.486° in the calorimeter arm;
- proton momentum 6.735 GeV/c at 19.164° in the HMS;
- 30 cm liquid-hydrogen target, represented by 2169 mg/cm² at 0.0723 g/cm³;
- BigCal at 10 m, with reconstructed historical half-width 64 cm and half-height 109 cm;
- one million fixed trials with random seed 110011; and
- the current checkout's standard elastic, radiation, energy-loss, and HMS transport implementations.

The BigCal dimensions were made input parameters rather than restoring the old hard-coded detector size. The existing NPS defaults remain 30.75 × 36.9 cm in half-width × half-height. A repeat of the NPS smoke test after this change was byte-identical to the recorded outputs, so the configurability change did not alter the NPS baseline.

## Results

With radiative effects enabled, the current checkout reports:

| Quantity | Result |
| --- | ---: |
| Weighted elastic yield | 41.936699 events/mC |
| Equivalent rate at 75 μA | 3.145252 Hz |
| Contributing events | 9,775 |
| HMS trials / HMS successes | 488,717 / 9,782 |
| BigCal trials / successes | 9,782 / 9,775 |
| BigCal horizontal / vertical extent rejects | 7 / 0 |

The PAC37 proposal's Table 1 gives 60 events/mC before quoting alternative elastic-selection retentions of 77.3% for 3σ cuts and 85.7% for 6σ cuts. Those alternatives must not be multiplied together. The resulting comparisons are:

| Historical convention | Historical yield | Current reconstruction difference |
| --- | ---: | ---: |
| Table 1 raw value | 60.000 events/mC | -30.1% |
| Table 1 × 3σ retention | 46.380 events/mC | -9.6% |
| Table 1 × 6σ retention | 51.420 events/mC | -18.4% |

Only 7 of 9,782 calorimeter trials fail the reconstructed BigCal rectangle. The rate difference is therefore not explained by BigCal's finite rectangular aperture in this run.

As a diagnostic only, the same deck was run once with radiative effects disabled. It yielded 50.510831 events/mC, 20.5% above the radiated result and 15.8% below the raw Table 1 value. The checked-in deck was then restored to radiative mode and rerun. This sensitivity shows that the historical radiation configuration and yield/cut convention are necessary inputs to an exact comparison.

## Historical-code caution

The repository contains a June 29, 2011 correction (`7896ebc`) to an event-ordering error in radiative initialization: radiation information could be inherited from the previous event after an earlier gfortran migration. Whether the PAC37 production used code before or after that correction is not yet established. The current implementation should not be judged against the proposal number without identifying the old source revision and deck; conversely, newer code should not be called better solely because it is newer.

## Interpretation and disposition

This reconstruction passes the agreed historical scale check. Depending on the proposal's cut convention, the difference is approximately 10–18%, rather than the 30% suggested by comparing directly with the raw 60 events/mC. The finite BigCal aperture is ruled out as the dominant cause in the reconstructed setup.

No further historical-deck archaeology is required before continuing. The active task is to define the most defensible current configuration—beginning with radiative treatment and the precise signal/cut convention—and to record each choice and sensitivity explicitly. The present deck remains an explicitly reconstructed historical benchmark. If an original deck or report later becomes available, it can be checked without holding up the current study.
