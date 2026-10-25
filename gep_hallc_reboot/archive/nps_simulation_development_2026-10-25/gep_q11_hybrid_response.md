# GEp-IV Q2 approximately 11 SIMC--GEANT4 hybrid response

Checked: 2026-10-23.

## Sample and execution

The dedicated fixed-seed deck
[`gep_reboot_q11_hybrid.inp`](../infiles/gep_reboot_q11_hybrid.inp) uses the
current 10.7 GeV GEp-IV settings, one million SIMC trials, and the versioned
post-target hybrid export. It produced 10,923 transport-accepted elastic
events. The corrected arm-8 integrated SIMC yield is 41.341739 events/mC.

Slurm job `13494409` transported all 10,923 electrons through the private NPS
GEANT4 checkout with NPS at 26.651 degrees and 3.0 m. The production hybrid
boundary was active: target liquid and cell were vacuum, preventing duplicate
material. The job completed with exit code `0:0` on `farm1957` in 9 minutes
20 seconds and used 406528 kB maximum resident memory. Its ROOT tree contains
10,923 entries.

## First weighted response

The raw 1,080-crystal energy deposits give:

| Quantity | Weighted result |
| --- | ---: |
| Primary active-crystal entry | 75.62% |
| Any deposit above 1 MeV | 98.69% |
| Mean deposited energy / primary energy | 75.00% |
| Deposit below 50% of primary energy | 23.29% |
| Deposit below 70% | 26.01% |
| Deposit below 80% | 27.23% |
| Deposit below 90% | 30.24% |

Applying these raw deposited-energy thresholds directly to the corrected SIMC
normalization gives provisional retained yields of 31.715, 30.589, 30.083, and
28.839 events/mC above 50%, 70%, 80%, and 90%, respectively. These are GEANT4
transport/containment results, not final trigger predictions: measured
photoelectron statistics, electronics response, calibration spread, pileup,
trigger sums, and reconstruction are not yet applied.

![GEp-IV hybrid NPS response](plots/gep_reboot_q11_hybrid_response.png)

The vector plot is
[`gep_reboot_q11_hybrid_response.pdf`](plots/gep_reboot_q11_hybrid_response.pdf),
the numerical summary is
[`gep_reboot_q11_hybrid_summary.csv`](gep_reboot_q11_hybrid_summary.csv), and
the reproducible ROOT macro is
[`analyze_gep_q11_hybrid.C`](analyze_gep_q11_hybrid.C).

## Important geometry observation

The transformed SIMC face projection is largely contained within the nominal
NPS rectangle. Nevertheless, the response map has a sharp low-response region
on the negative-horizontal side, and the primary active-crystal entry fraction
is only about 76%. The near-unity fraction with *some* deposited energy shows
that many affected events still create secondaries or small deposits.

This is not the signature of a position-independent calorimeter resolution.
It is consistent with upstream passive geometry or the sweeper-magnet shadow
in the run-6151-derived GEANT4 configuration. The present run deliberately
uses an explicit NPS angle/distance but still inherits other apparatus geometry
and masks from run 6151. Therefore 75.62% must not yet be promoted to the GEp-IV
acceptance efficiency.

## Next diagnostic

Repeat the same fixed event stream and seeds with the sweeper/passive obstruction
removed or separately tagged, leaving NPS unchanged. An event-by-event face map
will show whether the low-response band is caused by the sweeper geometry,
another inherited run-6151 element, or a remaining coordinate/placement issue.
Only after that audit should the threshold scan feed the beam-time efficiency
ledger.

## Geometry-only fiducial scan

A follow-up scan defines the fiducial region entirely from the projected face
coordinate, without using deposited energy to select good events. It removes
one full 21.69 mm crystal pitch from the outer physical boundaries and varies
only the negative-horizontal boundary associated with the localized
low-response band.

![GEp-IV hybrid fiducial scan](plots/gep_reboot_q11_fiducial_scan.png)

The combined threshold yield is nearly flat until the negative-horizontal
boundary passes approximately -150 mm: removing the poorest-response region
raises the conditional threshold efficiency while initially costing almost no
accepted yield. Representative 80%-threshold results are:

| Minimum local x | Geometric retention | Conditional 80% efficiency | Combined retention | Yield |
| ---: | ---: | ---: | ---: | ---: |
| -200 mm | 98.24% | 74.07% | 72.77% | 30.08 events/mC |
| -140 mm | 84.67% | 85.09% | 72.04% | 29.78 events/mC |
| -120 mm | 78.51% | 89.06% | 69.93% | 28.91 events/mC |
| -100 mm | 71.59% | 91.87% | 65.77% | 27.19 events/mC |

The provisional good-response fiducial is therefore

```text
-140 mm < local x < +303.66 mm
|local y| < 368.73 mm
```

It is the scan's practical elbow: at an 80% energy threshold it retains 29.78
events/mC, only about 1% below the inclusive 30.08 events/mC, while increasing
the conditional threshold efficiency from 74.1% to 85.1%. A tighter -120 mm
boundary is a useful systematic variation rather than the working choice.

At 75 microampere, the loose-fiducial 80%-threshold yield is approximately
2.234 Hz or 8.685 million events in 45 PAC days before DAQ, tracking,
elastic-selection, and FPP factors. This is still a raw crystal-deposit
criterion, not a final trigger efficiency.

The vector plot, scan table, and analysis macro are retained as
[`gep_reboot_q11_fiducial_scan.pdf`](plots/gep_reboot_q11_fiducial_scan.pdf),
[`gep_reboot_q11_fiducial_scan.csv`](gep_reboot_q11_fiducial_scan.csv), and
[`analyze_gep_q11_fiducial.C`](analyze_gep_q11_fiducial.C).

## First data-driven occupancy envelope

The private run-6151/run-2875 activity measurements have now been extrapolated
to 75 microampere with a provisional independent-Poisson current model. The
projection gives approximately 49% probability for any extra cluster and 45%
for an isolated out-of-time pulse above 10 mV, but only 4.3% for an extra
cluster that is both within 10 cm and within 4 ns of the elastic cluster. These
are occupancy categories, not efficiency factors. Full assumptions, plots,
and limitations are in
[`nps_occupancy_projection.md`](nps_occupancy_projection.md).

## Crystal-level 3x3 trigger-like sum

The initial threshold curves used the sum over all 1,080 crystals. That is
appropriate for total containment diagnostics but not for accidental overlay
or an NPS trigger approximation: a far-away deposit must not raise the elastic
cluster sum.

A new analysis finds the hottest GEANT4 crystal and sums its 3x3 neighborhood.
The maintained simulation numbers crystals column-by-column as
`vertical + 36*horizontal`; this differs from hcana's commonly used row-major
description and was verified directly from the sensitive-detector and placement
source before constructing neighbors. An initial row-major trial was rejected
because it gave an implausible result.

![GEp-IV 3x3 trigger-like response](plots/gep_reboot_q11_trigger_sum.png)

| Threshold / primary energy | Inclusive 3x3 retention | Fiducial 3x3 combined retention | Fiducial yield |
| ---: | ---: | ---: | ---: |
| 50% | 74.07% | 73.33% | 30.32 events/mC |
| 70% | 72.54% | 71.91% | 29.73 events/mC |
| 80% | 70.55% | 69.95% | 28.92 events/mC |
| 90% | 46.82% | 46.50% | 19.22 events/mC |

For the working `x > -140 mm` fiducial, the 80% result corresponds to 82.62%
conditional threshold efficiency inside the fiducial, 2.169 Hz at 75
microampere, and approximately 8.432 million raw events in 45 PAC days. This
supersedes the all-crystal-sum number for trigger-like yield estimates.

The calculation is still simpler than the installed VTP algorithm. It has no
seed threshold, local space/time maximum requirement, electronics response,
calibration spread, trigger-time association, or firmware energy convention.
It is best described as a crystal-level local-sum baseline. The measured
run-6151 threshold behavior provides an encouraging scale comparison, not yet
a formal validation.

The vector plot, threshold table, and reproducible macro are
[`gep_reboot_q11_trigger_sum.pdf`](plots/gep_reboot_q11_trigger_sum.pdf),
[`gep_reboot_q11_trigger_sum.csv`](gep_reboot_q11_trigger_sum.csv), and
[`analyze_gep_q11_trigger_sum.C`](analyze_gep_q11_trigger_sum.C).

The first measured-cluster overlay is documented separately in
[`nps_cluster_overlay.md`](nps_cluster_overlay.md). Resolved additional clusters
have no effect on the elastic 3x3 sum under 2 or 4 ns timing windows. Expanding
to a conservative 5x5, 4 ns window changes the 80%-threshold retention by only
0.018 percentage points. This does not cover unresolved activity already
merged into the primary cluster.
