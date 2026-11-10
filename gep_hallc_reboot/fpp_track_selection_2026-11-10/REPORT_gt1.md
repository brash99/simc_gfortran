# Smallest-angle track strictly above one degree

Updated November 10, 2026 using the saved 100,000-event transport sample from
Slurm job 14574698. No GEANT4 rerun or transport-code change was required.

First exclude tracks with theta <= 1 degree; then choose the smallest angle
among the remaining front-pair tracks. A sub-degree track does not veto
another track above one degree. After choosing, apply the existing chamber,
cone, z-close, s-close, and useful-pT cuts. No replacement is attempted if the
chosen eligible track fails a remaining cut. Outgoing momentum is not used
for ranking. Original multiplicity is counted before the angle filter.

| Quantity | Previous no-1-degree-bound rule | New theta > 1 degree rule |
|---|---:|---:|
| Useful events | 28,939 | 25,892 |
| Useful efficiency | 28.939% | 25.892% |
| RMS effective Ay | 0.06380 | 0.06704 |
| Intrinsic FOM / incident proton | 0.00117797011 | 0.00116370570 |
| Polarized FOM / incident proton, Pe=0.85 | 0.000851083403 | 0.000840777365 |

The new rule retains 98.79% of the previous provisional FOM.
It includes 9,663 useful exclusive single-track events above one degree and
16,229 useful multi-track events. The unchanged exclusive-single-track
reference with no one-degree cut remains at 12,492 useful events; only the
smallest-angle rule was changed.

All six CTest checks pass. The policy suite now contains nine tests, including
strict exclusion of the 1-degree boundary, choosing another track above the
bound, original multiplicity preservation, and configurable thresholds.
All 41 original physics branches still agree exactly with the old transport
sample. The legacy, single-track, and earlier no-one-degree-bound policy
statistics reproduce the previous JSON exactly. An independent audit of all
100,000 rows confirms every chosen new track has theta strictly above 1 degree.

The output stem is comparison_gt1, preserving all previous comparison files.
The default minimum-angle lower bound is now 1 degree, configurable via
--min-angle-deg. The multi-track FOM still provisionally applies the GEp-III
single-track Ay calibration to ideal truth-associated tracks. Detector
realism and multi-track analyzing-power validation remain unchanged limits.

Authoritative farm outputs:
/work/hallc/gep/brash/gep4_fpp_geant4/runs/track_selection_14574698
