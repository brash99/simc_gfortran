# Hall C FPP GEANT3-to-GEANT4 assessment

## Purpose

This note began as a review of the Hall C focal-plane polarimeter simulation
used for GEp-III and an assessment of what was required to obtain a defensible
GEp-4 figure of merit (FOM) in GEANT4.  The first implementation and ideal
single-analyzer FOM calculation are now complete; current numerical results
and the SBS comparison are consolidated in
[the focused FPP FOM summary](fpp_fom_summary.md).

## Code identification

The relevant legacy program is

```text
/work/hallc/gep/brash/e93027/brash/geant_gep
```

The directory name is misleading.  This is distinct from the older Hall A
straw-chamber program in `e93027/brash/fpp_geant3`.  The `geant_gep` source
contains the Hall C/HMS and GEp-III detector layout:

- HMS forward and reconstruction transport routines;
- two HMS drift chambers;
- the S1 and S2 scintillator planes;
- two polyethylene analyzers;
- four FPP drift chambers; and
- production files and outputs named `geant_gep3_*`.

The source and executables were rebuilt and used as recently as 2021.

## Legacy execution path

The active program can be summarized as follows:

```text
elastic ep or generated focal-plane phase space
                    |
                    v
       HMS transport to the focal plane
                    |
                    v
  HMS DC1 -> HMS DC2 -> S1 -> S2
                    |
                    v
       legacy 22-inch CH2 analyzer 1
                    |
                    v
          FPP DC1 -> FPP DC2
                    |
                    v
       legacy 22-inch CH2 analyzer 2
                    |
                    v
          FPP DC3 -> FPP DC4
                    |
                    v
  secondary-track reconstruction and HBOOK output
```

The default analyzer transverse half-sizes are 71.12 cm by 53.975 cm.  The
FPP chamber transverse half-sizes are 83 cm by 67 cm.  Most downstream
elements are offset horizontally by 5.08 cm relative to the HMS chambers.
These numbers should be checked against the GEp-III engineering survey before
being treated as the proposed GEp-4 geometry.

The analyzer thickness of 55.88 cm is exactly 22 inches.  It represents the
legacy constructed analyzer module rather than a GEp-4 optimization result.
The ported coordinates provide 61.38 cm between the analyzer upstream face
and the nominal entrance to FPP1, leaving approximately 5.50 cm beyond the
polyethylene for clearance and frame structure.  Although a 60 cm block fits
inside the simplified GEANT4 spacing, its mechanical compatibility with the
existing frame has not been established.

### Event generation and HMS transport

`src/gukine_gep.f` generates elastic ep target variables, transports the
proton through the HMS using the embedded SIMC routines, and launches the
proton at the HMS focal plane.  The same tree also contains an older
MCEEP-input path and a random focal-plane generator.

For GEp-4 there is no reason to port the elastic generator or the HMS
transport into GEANT4.  The current SIMC calculation should remain the source
of accepted, weighted proton events.  A new interface should provide, for
each accepted event, the proton position, direction, momentum, event weight,
and identifiers at a clearly defined reference plane upstream of the FPP.

### Hadronic transport

The active `src/guhadr.f` calls GEANT3 `GHEISHA`.  GEANT3 energy loss and
multiple scattering are enabled, and secondary protons and charged pions are
retained for tracking.  This is the scientifically important part to replace,
not translate literally.  GEANT4 should use a documented modern hadronic
physics list and production cuts, with model variations used to assess the
uncertainty in useful proton-carbon and proton-hydrogen scattering.

An old polarization-dependent rejection calculation remains in `guhadr.f`,
but it is commented out.  The production program therefore supplies the
secondary-scattering efficiency and angular acceptance; it does not generate
the measured analyzing-power asymmetry itself.  This is a useful separation
to preserve.

### Detector response and reconstruction

`src/gustep.f` records the entrance and exit points of charged particles in
the four FPP chamber volumes.  It then derives idealized U, X, and V wire
coordinates and reconstructs front- and rear-analyzer scattering angles,
azimuths, closest-approach quantities, and cone-test variables.  It retains
protons and charged pions as possible downstream tracks.

This machinery is valuable as a statement of the historical event-selection
logic, but it should not be translated line by line.  For an initial FOM
calculation, planar sensitive detectors with the measured active areas are
enough to determine whether a reconstructible track exists.  Chamber
resolution, efficiency, dead regions, alignment, drift-distance simulation,
and pattern-recognition ambiguities can be added only when their effect on the
FOM must be quantified.

### Output

The program writes an 83-variable HBOOK ntuple containing the generated HMS
focal-plane track, chamber hit information, front- and rear-analyzer
scattering angles and azimuths, closest-approach quantities, and reconstructed
track parameters.  The replacement should write a ROOT tree.  It should keep
truth and reconstructed quantities separate and retain the SIMC event weight.

## What the GEp-4 FOM calculation actually needs

For a selected set of incident SIMC protons, define a useful polarimeter event
by explicit reconstruction and analysis requirements.  At minimum these will
include:

1. a valid incident HMS track;
2. a hadronic interaction in the selected analyzer;
3. a reconstructible downstream charged track in the required chamber pair;
4. geometrical acceptance and track-quality requirements;
5. a useful polar-angle range excluding the Coulomb-dominated region; and
6. any required momentum, vertex, or closest-approach cuts.

The simulation should determine the weighted efficiency and angular
distribution.  The FOM should then be calculated externally as

```text
FOM = sum over selected events of weight * Ay(p, theta)^2
```

If beam polarization is included in the statistical-error convention, it
must be applied explicitly and documented once.  The analyzing-power model
should be replaceable so that GEp-III/GEp-2gamma measurements and alternative
extrapolations can be compared without rerunning transport.

For two analyzers, events must be assigned consistently.  The first useful
scattering should not also be counted as an independent second-analyzer event.
The incident direction for analyzer 2 must be the track reconstructed after
analyzer 1, not merely the original HMS track.

## Recommended migration scope

### Phase 1: geometry-level FOM prototype

Port only:

- the surveyed longitudinal ordering and transverse active areas;
- configurable CH2 analyzer thicknesses;
- a SIMC proton event reader;
- GEANT4 hadronic transport;
- idealized sensitive tracking planes;
- truth-level analyzer interaction and secondary-track records; and
- ROOT output sufficient to calculate efficiency versus momentum and
  scattering angle.

Do not initially port:

- HBOOK/PAW infrastructure;
- the internal elastic generator and legacy HMS transport;
- individual wire/drift-distance simulation;
- misalignment and instrumental-asymmetry machinery;
- legacy histogram definitions; or
- the commented polarization-rejection code.

This phase now supplies a first momentum- and geometry-dependent replacement
for the assumed constant 20% FPP efficiency.  For the legacy 22-inch analyzer and
6.7 GeV/c central-ray protons, the ideal useful-pT efficiency is 26.264% and
the intrinsic FOM per incident proton is 0.001046.  Analyzer-thickness and
detector-realism studies remain.

### Phase 2: reconstructed-track efficiency

Add chamber resolutions and efficiencies, a minimal track fit, measured dead
regions, the actual available GEp-4 detector configuration, and analysis cuts
matching the intended reconstruction.  Compare truth-level and reconstructed
FOM to quantify detector losses.

### Phase 3: precision and instrumental-asymmetry studies

Only if required, add detailed wire response, realistic occupancies, pattern
recognition, alignment distortions, and polarized azimuthal distributions.
This is the appropriate stage for pseudo-experiment studies and for the
instrumental-asymmetry use case of the old simulation.

## Validation strategy

A credible GEp-4 result requires more than agreement with the old GEANT3
program.  The proposed validation ladder is:

1. **Geometry closure:** reproduce the legacy material sequence, analyzer
   thicknesses, active areas, and reference-plane coordinates.
2. **Transport sanity checks:** compare energy loss, interaction depth,
   charged-secondary multiplicity, and scattering-angle distributions for
   monoenergetic protons in CH2.
3. **Legacy comparison:** run identical focal-plane proton samples through
   GEANT3 and GEANT4 and compare useful-scattering efficiency before applying
   analyzing power.
4. **Data anchor:** reproduce GEp-III or GEp-2gamma FPP efficiencies and
   angular distributions using their actual proton momenta, analyzer
   thicknesses, and analysis cuts.
5. **Physics-model envelope:** repeat with credible GEANT4 hadronic physics
   lists or model variations.  The spread is a systematic uncertainty, not a
   coding uncertainty.
6. **GEp-4 projection:** process the weighted Q2 = 11 SIMC sample and combine
   the selected events with the chosen analyzing-power parameterization.

The data comparison is the decisive validation.  GEANT3-to-GEANT4 agreement
alone could merely reproduce an old hadronic-model bias.

## Risks and open inputs

The main risks are:

- the accuracy of GEANT4 hadronic scattering in thick CH2 at approximately
  6--7 GeV/c;
- extrapolation of the measured analyzing power to the GEp-4 momentum and
  angular range;
- uncertainty about the exact detector and analyzer configuration available
  for a reboot;
- track reconstruction and occupancy losses at the proposed luminosity; and
- accidental double counting between the two analyzers.

Inputs still needed are the final proposed FPP layout, usable CH2 inventory
and thickness options, chamber survey/active-area information, GEp-III
efficiency and angular-distribution benchmarks, and the analyzing-power model
Andrew used in his current spreadsheet estimate.

## Assessment

A literal rewrite of the full GEANT3 program would be substantial and would
carry unnecessary legacy complexity.  A clean GEANT4 implementation aimed at
the FOM is a moderate, well-bounded project because the old code clearly
documents the geometry and historical selection logic, while the current
SIMC pipeline already supplies the incident proton sample.

The recommended first deliverable has now been achieved for a monoenergetic
central-ray sample: a geometry-level useful-scattering calculation, historical
wire mapping, ideal twelve-plane reconstruction, and external analyzing-power
weighting.  The result establishes an SBS-scale FOM but is not yet data
validated.  The next deliverable is the same calculation with realistic
chamber response and weighted SIMC protons, followed by GEp-III/2gamma
validation and event-by-event spin transport.

## Review evidence and limitations

Files inspected include `src/ugeom.f`, `src/gukine_gep.f`,
`src/gukine_random.f`, `src/gukine_simc.f`, `src/gustep.f`, `src/guhadr.f`,
`src/guout.f`, `src/uginit.f`, `src/uglast.f`, the local common-block headers,
the build files, run-control files, and historical MCEEP input code.

The installed legacy executable initializes successfully with the available
GEANT3/CERNLIB environment and creates its HBOOK output.  A noninteractive
test did not advance events through the PAW command interface, so full runtime
reproduction remains to be established before using the legacy program as a
numerical comparison baseline.  No legacy source or external files were
modified during this review.
