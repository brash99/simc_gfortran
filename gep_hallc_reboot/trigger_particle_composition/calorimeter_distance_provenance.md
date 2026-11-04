# NPS calorimeter-distance provenance audit

**Verdict:** the best-supported front-face distance is **9.5 m for all six
runs**: 6158, 6165, 6168, 2855, 5183, and 3888.

**Status:** completed 29 October 2026

## Why this audit was necessary

The Hall C RCDB reports 3.0 m for runs 6158, 6165, and 6168; 4.0 m for runs
5183 and 3888; and no distance for run 2855. Those values conflict with the
9--9.5 m distance used in earlier NPS calibration work.

The conflict is not a harmless convention difference. A change from 9.5 m to
3 or 4 m changes the calorimeter solid angle by a large factor. The distance
therefore had to be established from independent evidence rather than selected
from whichever database happened to be queried.

## Evidence hierarchy

### 1. Post-run NPS calibration analysis

The May 2025 NPS Collaboration presentation
[NPS Calorimeter Elastic Analysis](https://indico.jlab.org/event/946/contributions/16503/attachments/12589/20053/NPS_Performance.pdf)
lists the elastic run groups actually analyzed and their calorimeter distance:

| Run group containing selected run | Pass | Listed electron energy | Listed front distance |
|---|---:|---:|---:|
| 6151--6168 | 3 | 4.598 GeV | 9.5 m |
| 2855--2871 | 5 | 7.072 GeV | 9.5 m |
| 3883--3898 | 5 | 7.072 GeV | 9.5 m |
| 5183--5208 | 5 | 7.072 GeV | 9.5 m |

This is the strongest documentary source presently available because it is a
post-run detector-performance analysis using these exact elastic datasets, not
a pre-run plan or a generic run-control field.

### 2. Contemporaneous NPS run plan

The September 2023
[NPS-DVCS run plan](https://hallcweb.jlab.org/wiki/images/archive/1/10/20230913131243!NPS_DVCS_RunPlan.pdf)
places the five-pass elastic calibration at approximately 9 m and elsewhere
specifies 9.5 m for the operational calibration setting. The plan explicitly
distinguishes the much closer 3 m production/commissioning geometry from the
far elastic-calibration geometry.

The run plan is prospective rather than a measurement of the final detector
position, so it is supporting evidence rather than the sole authority.

### 3. Independent reconstruction from the data

For ordinary coincidence events with a high-energy NPS cluster, the electron
direction was reconstructed event by event from the measured HMS proton
momentum vector:

`p_e = p_beam - p_proton`.

The NPS horizontal cluster position was then fitted as

`x_cluster = x0 + D_eff tan(theta_e - theta_NPS)`.

The robust linear fits gave:

| Run | Events in fit | Fitted offset | Effective distance |
|---:|---:|---:|---:|
| 6158 | 43,098 | 0.75 cm | 9.53 m |
| 6165 | 46,272 | 0.77 cm | 9.64 m |
| 6168 | 30,114 | 1.42 cm | 9.19 m |
| 2855 | 5,884 | 1.89 cm | 9.35 m |
| 5183 | 7,266 | 3.27 cm | 8.78 m |
| 3888 | 25,284 | 3.80 cm | 8.44 m |

This is not a survey-quality measurement: it uses reconstructed HMS optics,
simple central-axis geometry, threshold-selected showers, and no detailed
vertex or shower-depth correction. Its purpose is hypothesis discrimination.
Every result is compatible with a far, roughly 9--9.5 m calibration position
and decisively incompatible with a 3--4 m position.

### 4. Why the RCDB distance is not authoritative here

The Hall C RCDB code shows that `calo_distance` was not read from an EPICS
position encoder or calculated from survey coordinates. The run-start program
stored the command-line argument `--dist` directly.

The wrapper history shows hard-coded changes:

- 2 October 2023: the wrapper began supplying `--dist=3.0`;
- 18 October 2023: it changed to `--dist=9.5`;
- 30 October 2023: it changed back to `--dist=3.0`.

The RCDB entries were created immediately before each run, consistent with
automatic insertion by the run-start machinery. Later runs nevertheless
contain 4.0 and 9.0 values, implying that other deployed script versions or
manual arguments were also used. The field therefore records a supplied
run-control value, not an independently measured detector position.

The RCDB values remain useful metadata, but the conflicting distance entries
must be treated as erroneous for these elastic-calibration runs.

## Reconciliation

The evidence is not evenly balanced:

- The post-run analysis explicitly assigns 9.5 m to the exact run groups.
- The run plan expects a far calibration position near 9--9.5 m.
- The data themselves reconstruct a far position near 9 m.
- Only the manually populated RCDB field indicates 3--4 m.

Accordingly, the working and authoritative distance for these six analyses is
9.5 m to the calorimeter front face. The spread in the simple geometric fits
should not be interpreted as six different physical positions without a more
complete optics, vertex, survey-coordinate, and shower-depth model.

## Consequences for the trigger-composition study

The original 9.5 m geometry used in the inclusive-generator comparison should
be retained. The earlier conclusion that five of the six samples had been
simulated with the wrong solid angle is withdrawn.

The RCDB discrepancy should remain documented as a metadata-quality warning,
and future configuration audits should not treat `calo_distance` as a primary
source unless its provenance is independently established.

