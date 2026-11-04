# Trigger-particle-composition investigation

This directory contains the active investigation of which particle species
contribute to the measured NPS cluster-trigger rate.

## Contents

- [Investigation plan](investigation_plan.md)
- [Phase 1, Sanity Check 1: six-run configuration audit](six_run_configuration_audit.md)
- [NPS calorimeter-distance provenance audit](calorimeter_distance_provenance.md)
- [Machine-readable working table](six_run_configuration_audit.csv)
- [Background-generator and transport status](background_generator/README.md)
- [Separated target-phase-space implementation](background_generator/target_phase_space/README.md)

The configuration table distinguishes values recovered directly from the
existing replay products from values that still require confirmation in the
run report, run database, logbook, or embedded event-137 configuration. A
blank or `not_yet_verified` entry must not be filled from a neighboring run.

## Current status

Run 6887 supplies the principal total-rate benchmark.  At the 70%, 75%, and
80% reference thresholds its measured rates are 95.1, 73.7, and 57.7 kHz.
The GEANT4-folded inclusive-electron calculation gives 47.0, 37.2, and
28.4 kHz, leaving 48.1, 36.5, and 29.2 kHz of unmodeled activity.

The separated Wiser-pi0 chain is now operational:

1. generate weighted low-Q2 pi0 events;
2. transport the correlated final state through the physical target and
   record target-exit phase space;
3. project target-exit particles to NPS; and
4. transport the surviving correlated events through the NPS GEANT4 model.

Using the working corrected-3x3 convention, this channel contributes 2.546,
1.091, and 0.461 kHz at the three reference thresholds.  The corresponding
raw-3x3 values are 6.046, 2.893, and 1.288 kHz.  The raw and corrected values
bracket an unresolved inherited energy-scale convention and must not be
averaged.  Transported pi-minus contributes less than 0.04 kHz.

These results validate the two-phase machinery for a weighted Wiser channel;
they do not complete the fully inclusive target-background calculation.  In
particular, the unbiased program in which beam electrons strike the target and
GEANT4 produces all electromagnetic and nuclear secondaries still requires an
importance-sampling strategy and a convergence study before it can predict an
absolute NPS trigger rate.

