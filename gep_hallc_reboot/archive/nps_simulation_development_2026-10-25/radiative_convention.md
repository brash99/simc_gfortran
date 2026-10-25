# GEp-IV Hall C reboot: radiative and elastic-yield convention

Checked: 2026-10-22.

## Decision

Use radiative effects in the working Q² ≈ 11 elastic simulation. Retain the repository-standard settings already used by the fixed-seed deck:

    using_rad = 1
    rad_flag = 0
    extrad_flag = 2
    one_tail = 0
    intcor_mode = 1
    use_offshell_rad = 1
    use_expon = 0

This is the simulation baseline, not a claim that radiative effects are experimentally “corrected away.” The analysis convention must later distinguish generated Born kinematics, radiated measured kinematics, detector reconstruction, elastic selection, and any unfolding or acceptance correction.

## Meaning established from the source

- `using_rad=1` calls `generate_rad`; disabling it copies the vertex event directly to the unradiated event state.
- `rad_flag=0` selects the code's documented “best available formulas” in the peaked `(tail, photon energy)` basis.
- `one_tail=0` permits all three modeled radiation directions: incoming electron, scattered electron, and proton.
- `intcor_mode=1` uses the `brem`/`bremos` internal-correction calculation; `use_offshell_rad=1` selects `bremos`. `intcor_mode=0` instead uses the older Schwinger prescription.
- `extrad_flag=2` uses the basic external-radiation expression multiplied by its correction function. The source describes `extrad_flag=3` as the Friedrich approximation and makes it the default when `rad_flag=0` and no explicit external option is supplied. The repository's example decks consistently specify option 2 explicitly.
- `use_expon=0` follows the convention marked “leave at 0” in the repository's example decks.

## Fixed-seed sensitivity at the current working point

All comparisons use one million fixed trials and seed 110011 at 10.7 GeV and Q² ≈ 11. Only the listed radiative option was changed; the checked-in deck was restored to the baseline and rerun afterward.

| Configuration | Weighted yield (events/mC) | Change from baseline | Contributing events |
| --- | ---: | ---: | ---: |
| Working baseline: radiated, `intcor_mode=1`, off-shell, external option 2 | 41.481955 | — | 11,013 |
| External option 3 (Friedrich) | 41.481955 | 0.000% at printed precision | 11,013 |
| On-shell internal calculation (`use_offshell_rad=0`) | 41.481955 | 0.000% at printed precision | 11,013 |
| Older Schwinger internal prescription (`intcor_mode=0`) | 42.885791 | +3.384% | 11,255 |
| Radiation disabled | 50.335640 | +21.344% | 11,393 |

Equivalently, the working radiated result is 17.589% below the no-radiation result. This large change is physical/model-convention sensitivity and is why the unradiated rate should not be used as the experimental yield baseline. The 3.4% Schwinger comparison is a useful initial radiative-model systematic, but it is not yet a rigorous uncertainty: the alternatives are not necessarily equally complete, and a one-seed run does not supply a weighted Monte Carlo error.

The external-option-3 run reproduced the baseline `.gen` and `.geni` hashes. The on-shell run reproduced the same integrated yield and event count. These null differences apply only to this elastic configuration and observable; they do not establish equivalence for radiative-tail shapes or background channels.

## What the quoted yield includes

The integrated `wtcontr` is not a detector-ready elastic sample. In the current code:

1. the event must survive generation, radiation, target effects, HMS transport, and the rectangular NPS interception test;
2. `pass_cuts` applies the configured `SPedge` momentum and angular limits before adding the event weight to `wtcontribute`, even when `hard_cuts=0`; and
3. `hard_cuts=0` means those limits do not additionally suppress the broader output-event success flag, and the configured `cuts%Em%max=0` is not applied as a reconstructed missing-energy analysis cut.

Therefore 41.481955 events/mC is best labeled the **radiated, transport-accepted, configured-SPedge elastic yield before a realistic reconstructed elastic-selection efficiency**. Detector response, shower containment, trigger/reconstruction efficiency, and final exclusivity cuts remain outside this number.

## Verification finding

The `pass_cuts` expression in `simc.f` compares the proton upper momentum deviation with `SPedge%e%delta%max` rather than `SPedge%p%delta%max`. This latent typo does not affect the current or reconstructed PAC37 decks because both arms use the same ±15% momentum limits, and accepted HMS events in these runs do not approach the upper bound. It should be corrected before any study uses unequal electron and proton momentum limits.

## Next analysis step

Keep the working radiative configuration fixed and define realistic reconstructed elastic selections. Produce weighted distributions for the variables that will separate elastic events from π⁰ and other backgrounds—at minimum proton momentum/angle consistency, electron direction, missing energy or equivalent overconstraint, and calorimeter energy/position residuals. Cut-width scans should report both elastic efficiency and background leakage. The 3.4% internal-prescription shift should be retained as an initial sensitivity until a modern radiative calculation or data benchmark justifies a narrower uncertainty.
