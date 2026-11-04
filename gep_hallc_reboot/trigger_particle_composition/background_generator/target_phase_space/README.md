# GEp target phase-space generator

This standalone GEANT4 program separates target-background production from
downstream NPS transport. It fires unbiased beam electrons through a 10 cm
liquid-hydrogen cell, records particles as they leave the aluminum cell, and
then kills those tracks. No NPS or beamline geometry is present.

The target has a 7.5 cm cylindrical liquid section followed by a 2.5 cm-radius
downstream hemisphere, for a 10.0 cm on-axis liquid path. The aluminum wall and
entrance/downstream windows are 0.132 mm thick.

The `phase_space` ROOT tree contains one row per exiting particle. All rows
retain the incident-electron `event_id`, so correlated multiparticle final
states can be reconstructed. An exiting primary electron is retained
independently of its energy; secondaries are retained only when their kinetic
energy exceeds the requested cut. A primary that stops inside the cell has no
phase-space row. `surface` is `-1` for upstream, `0` for radial, and `+1` for
downstream exit.

The `run_info` tree records the number of incident beam electrons and the
generation configuration. Absolute rates are obtained by dividing selected
entries by `completed_events` and multiplying by the beam electron flux.

Weighted SIMC/Wiser source files use a centered `-5 < z < +5 cm` target
coordinate. The physical cylinder-plus-hemisphere cell spans approximately
`-3.75 < z < +6.25 cm`; source mode therefore applies a documented `+1.25 cm`
translation. `source_vertex_z_shift_cm` records this in `run_info`.

`project_phase_space_to_nps.py` performs straight-line transport from the cell
exit to a rectangular NPS front face. Its `nps_x` coordinate lies in the Hall
horizontal scattering plane and increases toward larger polar angle; `nps_y`
is Hall vertical. Canonical Hall axes are `x` downward, `y` beam-left, and
`z` downstream. The default active dimensions are 30 by 36 crystals at
2.05 cm per crystal, or 61.5 by 73.8 cm. The projected energy is incident
particle energy, not calorimeter deposited energy or VTP trigger energy.

Build and run on the farm:

```sh
cmake -S . -B build
cmake --build build -j4
./build/gep_target_phase_space output.root 10000 8.456 1.0 12345
python3 project_phase_space_to_nps.py output.root projected.root \
  --angle-deg 16.57 --distance-m 3.57 --current-uA 29.417
```

## Completed Wiser-pi0 validation

The first production use of this interface transports the 88,122-event
run-6887 Wiser-pi0 sample.  Target transport produced 176,023 exiting-particle
rows.  The face projection retained 110,107 particle intersections belonging
to 87,602 correlated events; 109,688 of the face particles are photons.  The
subsequent NPS GEANT4 job processed all 88,122 input events in 49 minutes and
11 seconds with no event-count loss.

At 3.387, 3.629, and 3.870 GeV the full-face incident-energy rates are 75.794,
43.408, and 25.197 kHz.  The corresponding NPS hottest-3x3 rates are 6.046,
2.893, and 1.288 kHz before the inherited energy correction, or 2.546, 1.091,
and 0.461 kHz after it.  The summary table is retained in
`run6887_wiser_pi0_target_exit_nps_rates.csv`.

Four nonfatal navigation warnings occurred in the inherited
`SweepingMagnetic_pos` volume.  They caused no event loss but reinforce the
need to certify the passive/sweeper geometry before treating its edge shadow
as a measured-configuration acceptance effect.

This is a validation of the separated interface for a weighted Wiser source,
not a complete inclusive target-background calculation.  An unbiased sample
of 100,000 beam electrons was computationally stable but produced no useful
NPS-intersection statistics; the next implementation step is importance
sampling with explicit absolute-weight bookkeeping and a production-energy
convergence scan.

