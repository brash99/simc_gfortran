# SIMC--GEANT4 material-boundary closure

Checked: 2026-10-23.

## Purpose

SIMC already applies outgoing radiation, target/cell ionization loss, and
multiple scattering before exporting the electron. The production hybrid mode
therefore replaces the GEANT4 target liquid and aluminum cell with vacuum. This
test compares that intended boundary with an explicitly labeled diagnostic
mode that restores the physical target and cell, intentionally double counting
their material.

## Coordinate correction found by the test

The initial serialization smoke test preserved all CSV truth fields but did
not check that the transported particle struck NPS. The first closure attempt
showed zero active-crystal entries because SIMC and this GEANT4 geometry use
different Cartesian conventions:

```text
SIMC Hall:       x = down, y = left, z = downstream
GEANT4 geometry: x = left, y = up,   z = downstream
```

The primary position and momentum now use the explicit transport mapping

```text
GEANT4 (x,y,z) = SIMC (y,-x,z).
```

The ROOT truth branches remain in the documented SIMC Hall convention. The
corrected paired job produced physical NPS entries.

## Paired test

Slurm job `13494160` processed all 78 records in the fixed-seed
`gep_pac37_q11_elastic.hybrid.csv` sample twice, using identical random seeds,
NPS at 25.486 degrees and 3.0 m, and the same physics list. The first pass used
the production vacuum boundary. The second used the diagnostic
`--simc-events-target-on` option. Both passes exited successfully on
`farm19106`; maximum resident memory was 408576 kB.

The input truth records agree exactly between the two ROOT trees
(`max_truth_difference = 0`). The small sample gives:

| Quantity | Target/cell vacuum | Target/cell duplicated |
| --- | ---: | ---: |
| Weighted active-crystal entry fraction | 75.61% | 74.10% |
| Weighted fraction below 90% deposited energy | 27.62% | 29.14% |
| Weighted mean deposited-energy fraction | 73.16% | 73.17% |

One of 78 events changes active-entry classification. The event-by-event
deposit difference has mean 8.5 MeV and RMS 634 MeV, dominated by a few shower
and acceptance-tail changes. Because enabling upstream material changes the
GEANT4 random-consumption history, the paired deposit difference is not a
deterministic estimate of mean ionization loss. The relevant conclusion is
that duplicated material can migrate events between entry and low-energy-tail
categories; it must remain disabled in production.

This 78-event PAC37-derived sample is a boundary test, not a precision NPS
acceptance or response prediction. A larger current-geometry sample is needed
for stable response distributions.

![SIMC--GEANT4 material closure](plots/simc_material_closure_78.png)

The vector plot is
[`simc_material_closure_78.pdf`](plots/simc_material_closure_78.pdf), and the
event-level table is
[`simc_material_closure_78.csv`](simc_material_closure_78.csv).

## Status and next step

The coordinate transformation, invariant SIMC truth transfer, and production
vacuum boundary are now directly exercised. Next generate a substantially
larger hybrid export at the current 10.7 GeV GEp-IV geometry and run it through
the production boundary. That sample can support NPS acceptance, containment,
threshold, and position-dependent response studies.
