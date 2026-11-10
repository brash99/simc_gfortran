# FPP single-track and smallest-angle results

Completed November 10, 2026 on ifarm/farm180222, Slurm job 14574698.
100,000 incident 6.7 GeV/c central-ray protons; one 55.88 cm CH2 analyzer;
FTFP_BERT; unchanged geometry, cone, z-close, s-close, and useful-pT cuts.

| Selection | Useful events | Useful efficiency | RMS effective Ay | Intrinsic FOM / incident | Polarized FOM / incident (Pe=0.85) |
|---|---:|---:|---:|---:|---:|
| Original leading-momentum reference | 26,264 | 26.264% | 0.06311 | 0.00104613432 | 0.000755832045 |
| Exactly one front-pair track | 12,492 | 12.492% | 0.05859 | 0.000428895242 | 0.000309876813 |
| Smallest-angle track, including singles | 28,939 | 28.939% | 0.06380 | 0.00117797011 | 0.000851083403 |

The single-track result retains 41.0% of the original FOM, rather than simply
scaling it by the retained event fraction. The original useful sample included
13,772 multi-track events (52.44% of useful events), contributing 59.00% of its
Ay-squared sum. Thus the original 26.264% was not an exclusive single-track
efficiency.

The smallest-angle policy adds 16,447 useful multi-track events to the 12,492
single-track events. Its provisional FOM is 2.747 times the single-track FOM
and 12.60% above the original leading-momentum result. Relative to the
original useful-event selection, it retains 25,668 events, gains 3,271, and
loses 596: a net gain of 2,675. A selected smallest-angle track that fails a
cut is not replaced with another track.

## Definitions and physics limits

Multiplicity counts distinct GEANT4 charged-particle IDs crossing both FPP1
and FPP2 before analysis cuts. Exactly-one selection vetoes additional such
tracks even when they would fail the downstream or physics cuts. The chosen
track must also cross FPP3 and FPP4 below 12 degrees. The minimum-angle policy
ranks all front-pair tracks by polar angle before cuts; outgoing track
momentum is never used in that ranking. Incident proton momentum enters pT
and Ay, as it would from the upstream spectrometer.

These are ideal, truth-associated geometry selections, using the same
truth-level angles and closest-approach quantities as the original FOM.
Multiplicity is not yet experimentally reconstructed multiplicity: no finite
chamber efficiency/resolution, noise, merged hits, or multi-track pattern
recognition is modeled. The new candidate table preserves the per-track
inputs for further work.

The GEp-III Ay model was calibrated for single-track events. The multi-track
FOM uses that model as a provisional common assumption. It does not establish
that the selected multi-track population has the same analyzing power,
background dilution, or experimental selection behavior. No form-factor-ratio
uncertainty is derived here.

For the earlier 7.5-7.7 million incident-event assumption, the corresponding
polarization-component statistical uncertainties would be 0.02934-0.02895
(single-track) and provisionally 0.01770-0.01747 (smallest-angle), compared
with 0.01878-0.01854 for the old reference. These retain the earlier yield,
Pe=0.85, ideal-response, and analyzing-power assumptions.

Monte Carlo standard errors on useful efficiencies are 0.139 percentage
points (reference), 0.105 (single-track), and 0.143 (smallest-angle). They do
not include physics-model, detector, or analyzing-power uncertainty.

## Verification

- Farm build succeeds; all six CTest checks pass with ROOT-compatible
  /usr/bin/python3 (3.9.25). The new pure-policy suite contains six examples.
- The 100,000-event job completed normally, exit 0:0, in 12 minutes 2 seconds.
  ROOT file closed normally and is neither zombie nor recovered.
- All 41 original physics branches agree exactly with the original sample
  by entry. The three legacy metadata fields are explicitly excluded:
  the old file uniformly stored event_id=-1, weight=1, incident_p_GeV=0.
  New IDs, weights, and incident momenta are retained correctly.
- 240,057 candidate rows have unique (event_id,track_id) keys and valid
  front-pair chamber masks; every candidate belongs to an incident event.
- Candidate-table reconstruction reproduces the original accepted selection
  and the published 26.264% / 0.00104613432 intrinsic FOM.
- Single-track useful events are a subset of minimum-angle useful events;
  recovered count and Ay-squared sum close independently.
- The audit contains exactly 100,000 incident-event rows. An independent
  CSV check confirms exclusive multiplicities and forward useful tracks.
- The comparison plot has been visually inspected; git diff --check passes.

Authoritative farm repository:
/work/hallc/gep/brash/gep4_fpp_geant4

Run outputs:
/work/hallc/gep/brash/gep4_fpp_geant4/runs/track_selection_14574698

New workflow documentation: ../TRACK_SELECTION_STUDY.md
Machine-readable results: comparison.json and comparison.csv
Per-event choices: comparison_events.csv
Plots: comparison.png and comparison.pdf
Frozen changed-source snapshot: source_snapshot.tar.gz

No commits or pushes were made. The pre-existing README changes were preserved.
