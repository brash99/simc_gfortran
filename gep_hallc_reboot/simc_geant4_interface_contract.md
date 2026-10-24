# SIMC--GEANT4 event and material contract

## Purpose

The GEp-IV hybrid must assign every physics and detector effect to exactly one
program. In particular, an electron must not receive outgoing radiative loss,
ionization loss, or target multiple scattering once in SIMC and then traverse
the same target material a second time in GEANT4.

This note records the source-level audit and defines the initial interface. The
arm-8 target/cell path and the first strict event adapter described below have
now been implemented and exercised end to end.

## What the SIMC states actually mean

The executable logic is more reliable than the old summary comment in
`init.f`, whose description of `orig` as being before any radiation is
misleading.

- `vertex` contains the hard-scattering state after any incoming tail-1 photon
  has reduced the beam energy. `vertex%e%E` is before outgoing tail-2 radiation.
- `orig` is copied from `vertex`; tail 1 is added back to `orig%Ein`, while
  tail 2 is subtracted from `orig%e%E` and tail 3 from the hadron. Thus
  `orig%e` is after SIMC radiative-tail generation but before ordinary target
  energy loss.
- `main%SP%e` is passed toward electron-arm transport. Its momentum includes
  target ionization loss and the Coulomb term, and its angles include target
  multiple scattering when `mc_smear=1`.

The GEp baseline enables these mechanisms with `using_rad=1`, `one_tail=0`,
`using_Eloss=1`, and `mc_smear=1`.

## Current output is insufficient

The standard coincidence ntuple writes the `orig` electron momentum/slopes,
reconstructed quantities, and only the *sum* `radphot` of all radiated-photon
energies. It does not write the `vertex` electron state, individual tail-1/2/3
energies, `main%target%Eloss(2)`, or an explicit post-target four-vector.
Consequently, it cannot reconstruct the outgoing pre-radiation or post-target
state unambiguously.

The dedicated hybrid record adds, with explicit units and coordinates:

- event identifier and weight;
- generated vertex `(x,y,z)`;
- `vertex` scattered-electron four-vector;
- the three separate radiated-photon energies and selected tail;
- outgoing target/cell ionization loss and effective radiation length;
- post-target electron position, direction, and four-vector;
- corresponding pi0 decay-photon records for the background sample.

The first implementation is enabled with `hybrid_export=1` and writes
`worksim/<input-stem>.hybrid.csv`. It is versioned as
`simc_geant4_hybrid_v1`, uses GeV, cm, and Hall coordinates (`x` down, `y`
left, `z` downstream), and leaves the legacy ntuple unchanged. It presently
supports elastic H(e,e'p) only and fails fast for other reactions until their
material boundary has been defined.

The raw `main%weight` is preserved in each row. Detector response must operate
on that weight without renormalizing it; SIMC's run-level normalization remains
authoritative.

An opt-in 10,000-trial smoke test produced 78 accepted records. All contained
the 23 declared columns, every primary satisfied the electron mass shell, and
`vertex_E - rad2 - target_Eloss = primary_E` closed to
1.8e-15 GeV. Restoring the default `hybrid_export=0` and rerunning the full
fixed-seed baseline reproduced 40.869596 events/mC.

## Arm-8 material correction

The GEp deck uses `electron_arm=8` for the beam-left calorimeter. In
`trip_thru_target`, however, scattered-electron material branches cover only
arms 1--6. Arm 8 reaches later calculations with `forward_path`, `s_air`,
`s_kevlar`, and `s_mylar` not assigned by an arm-8 branch. Reported arm-8
`Eloss_e`, `teff_e`, and multiple-scattering values therefore depend on
undefined Fortran state and are not a validated NPS material model.

This affects more than the small ionization correction: `teff(2)` enters the
outgoing external-radiation exponent, so it can alter the tail and event weight.
The correction now initializes all optional material paths to zero and gives
calorimeter arms 7 and 8 explicit target paths with the appropriate Hall-side
target-angle sign. These branches intentionally add no air, chamber window, or
spectrometer entrance material: the existing target-can calculation adds the
liquid and aluminum cell wall, while downstream material belongs to GEANT4.
This avoids silently borrowing the HMS or SHMS material prescription.

For the fixed-seed million-trial Q2=11 baseline, the corrected arm-8 result is
2.61479 MeV mean outgoing loss and 0.01387 mean radiation lengths. The weighted
yield changes from 41.936699 to 40.869596 events/mC (-2.54%). A checked build
using signaling-NaN initialization, floating-point traps, and runtime bounds
checks completes and is byte-identical to the optimized result after removing
the report timestamps.

## Initial hybrid contract

The least disruptive first implementation keeps the existing SIMC radiative
formalism and normalization authoritative:

| Effect | Authoritative program |
| --- | --- |
| Hard-scattering generation and cross section | SIMC |
| Incoming, internal, and outgoing tail-2 treatment | SIMC |
| Target liquid/cell ionization loss and multiple scattering | SIMC |
| Proton acceptance and HMS transport | SIMC |
| Material downstream of the declared handoff | GEANT4 |
| NPS shower and per-crystal deposits | GEANT4 |
| Clustering, trigger emulation, and accidental overlay | response analysis |

GEANT4 receives the explicit **post-target/cell** electron state. Its liquid
target and target-cell volumes must be disabled or replaced by vacuum in hybrid
mode, so they cannot duplicate SIMC. The primary may be placed at the SIMC
vertex in that vacuumized region, provided the exported post-target direction
and energy are used and the first active GEANT4 material is exactly the
documented downstream handoff.

The same rule applies to pi0 photons: the SIMC state and upstream material
already included must be declared explicitly before GEANT4 transport.

## Required closure tests

1. The GEANT4 CSV reader and first field-by-field primary transfer are complete
   for the elastic-v1 ten-event smoke test described below.
2. Tabulate SIMC radiation length by component and match the handoff to the
   GEANT4 volumes disabled in hybrid mode.
3. With radiation, loss, and smearing disabled, verify event-by-event equality
   between the exported SIMC vector and GEANT4 primary.
4. Enable each upstream effect separately and verify the recorded changes.
5. Compare GEANT4 target material on and off; production must contain only one
   intended traversal.
6. Preserve the SIMC event weight through GEANT4 and apply detector response as
   acceptance, not as a second physics-weight factor.

## Longer-term alternative

A future material-driven GEANT4 treatment could start from the pre-tail-2
`vertex` electron and transport the physical target. That is not a drop-in
change: SIMC tail-2 generation, the external-radiation exponent, acceptance
sampling, and event weight are coupled. They require a controlled
reformulation and normalization test first. Sending `vertex%e` through the
present GEANT4 target while retaining the current SIMC radiative weight would
double count radiation.

## GEANT4 adapter and first end-to-end test

The private maintained checkout at
`/work/hallc/gep/brash/gep4_nps_geant4`, branch `gep-external-primary`, now
accepts

```text
--simc-events <csv> <nps_angle_deg> <nps_distance_m>
```

The reader requires the exact `simc_geant4_hybrid_v1` version line, two
metadata lines, and exact 23-column schema before initialization. Each record
must contain 23 finite numbers, a nonnegative SIMC weight, and a physical
on-shell electron. Premature stream exhaustion is an error. The detector
geometry remains present, but the target liquid and aluminum cell are assigned
vacuum in this mode so the post-target electron does not traverse them twice.

Slurm job `13493645` exercised the first ten records from
`gep_pac37_q11_elastic.hybrid.csv` with NPS at 25.486 degrees and 3.0 m. It
completed with exit code `0:0` on `farm180221` in 12.13 seconds, used 406156 kB
maximum resident memory, and produced a ten-entry `t_dvcs` ROOT tree. Stored
`psf`, `GV_x/y/z`, and `GSE_px/py/pz` values agree numerically with the first
ten CSV weights, post-target positions, and momenta. The log confirms that
SIMC-event mode and vacuum target/cell material were active.

That first test verified serialization but did not verify NPS interception.
The subsequent material-closure test exposed and corrected the axis mapping:
this GEANT4 geometry receives `(x,y,z) = (y,-x,z)` from SIMC Hall coordinates.
Corrected job `13494160` produced physical NPS entries and compared the vacuum
boundary against deliberately duplicated target/cell material. Results and
the important interpretation limits are in
[`simc_geant4_material_closure.md`](simc_geant4_material_closure.md).

This closes basic serialization, coordinate transfer, and the first controlled
material on/off comparison. The component material ledger, larger weighted
sample, and response/trigger analysis remain open.
