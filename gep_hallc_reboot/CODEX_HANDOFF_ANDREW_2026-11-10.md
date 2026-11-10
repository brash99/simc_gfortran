# GEp-4 / Andrew discussion: Codex handoff

Updated 10 November 2026. Read this before continuing the Andrew email discussion.

## Resume here

The user is preparing for a Zoom discussion with Andrew Puckett about the GEp-4
Hall C feasibility study. We have completed and discussed Andrew's questions 1
(single versus multiple tracks) and 2 (analyzing-power implementation), and
updated the presentation. We also made a rough analyzing-power extraction from
Andrew's preliminary SBS asymmetry plots, related to his question 5.

The user explicitly accepted the question 1 investigation and the question 2
interpretation. A natural next discussion is question 3 (SBS yield/FOM
normalization), unless the user steers elsewhere. Do not restart question 1 or
rerun transport unnecessarily. The presentation is the working reference for
his Zoom meeting, not a final proposal precision claim.

This Codex thread initially opened in the CDet replay checkout, but the active
work moved to the SIMC GEp-4 study and its separate FPP simulation. The old
CDet `CODEX_HANDOFF.md` concerns an unrelated CDet/FTROI task.

## Repositories, locations, and transfer to laptop

- Local SIMC: `/Users/brash/simc_gfortran`, branch `master`, HEAD `a0e6c67 updates`.
- Study: `gep_hallc_reboot/` inside that repository.
- Presentation source and PDF:
  `gep_hallc_reboot/presentations/gep4_reboot_feasibility_2026-11-04.{tex,pdf}`.
  Keep this established filename; its title page now says Updated 10 November 2026.
- Presentation plot:
  `gep_hallc_reboot/presentations/figures/fpp_track_selection_gt1_2026-11-10.png`.
- Stable result copies accompanying this handoff:
  `gep_hallc_reboot/fpp_track_selection_2026-11-10/` in SIMC. These include
  summary JSON/CSV, reports, and frozen FPP source archives. Per-event CSVs and
  ROOT data remain on the farm; they are not required just to discuss the slides.
- Authoritative FPP checkout on ifarm:
  `/work/hallc/gep/brash/gep4_fpp_geant4`, HEAD `71a0829`, branch `main`.
- Completed farm run:
  `/work/hallc/gep/brash/gep4_fpp_geant4/runs/track_selection_14574698/`.
- Original email export: `/Users/brash/andrew.pdf`, 12 actual PDF pages despite
  printed footers saying “of 200”. Andrew replied 10 November to Ed's 6 November email.
- Local scratch checkout `/private/tmp/gep4-fpp-question1` and presentation
  staging `/private/tmp/gep4-presentation-update` are disposable. Do not rely
  on them existing on the laptop.

No commits or pushes were made. Merely opening this handoff on another machine
will not transfer uncommitted presentation changes. Copy/sync the source, PDF,
new figure, handoff, and stable results directory, or use the user's normal
repository-transfer workflow. Do not commit/push without a request.

Before editing on the laptop, inspect fresh status and applicable repository
instructions. Existing SIMC `handoff.md`, FPP summaries, and readiness notes
contain older results; this dated handoff and current presentation supersede
those older FPP numerical conclusions. Most older broad-study notes were not
rewritten during this discussion.

## Andrew's five questions

1. Does the quoted 26% efficiency include multiple-track events, whereas the
   GEp-III analyzing-power calibration was for single-track events?
2. Exactly how is GEp-III Ay implemented? Andrew uses a weighted average of
   the three high-Q2 b parameters and a linear peak-Ay fit versus inverse p.
3. His detailed SBS reconstruction estimate gives about 2.9M useful events,
   versus about 5.1M in his quick spreadsheet; ideal reconstruction, tight
   exclusivity cuts, and Born-only versus radiated generation affect comparison.
   He estimates missing radiative losses could make SBS yields 10–15% optimistic.
4. He supports the one-analyzer tracking idea, suggests 75–90 cm CH2, and has
   an older 12-plane tracking implementation. Ed's current hardware baseline
   is one legacy 22-inch / 55.88 cm analyzer, limited by the existing frame.
5. How well is the Ay extrapolation anchored? Andrew points to preliminary SBS
   asymmetries consistent with his simulation at Q2=11 GeV2.

## Question 1: what the code actually did

The original calculation was not an exclusive single-track calculation.
`SteppingAction.cc` records crossings of charged primary and secondary particles
by GEANT4 track ID. `EventAction::bestCandidate` selected the highest-momentum
track crossing the required chambers, even when multiple tracks existed.

The original FOM used truth-associated angles and closest-approach quantities.
The unsigned-drift wire fits were closure diagnostics, not conditions in the
FOM selection. Therefore “one selected track” did not mean “a single-track event”.

We added a `candidates` ROOT tree retaining every distinct charged track ID
crossing FPP1 and FPP2, before analysis cuts. It records event/track/parent IDs,
chamber mask, angle, azimuth, outgoing momentum, closest approach, and cone flags.
Original event-tree physics calculations were preserved.

### Current policy definitions

- `legacy`: original highest-momentum choices, retained only as a regression
  reference. Outgoing momentum would not be experimentally available for ranking.
- `single`: exactly one front-pair track before any cuts, regardless of whether
  additional tracks would have failed later cuts. This reference has no explicit
  one-degree cut.
- `min_angle_uncut`: previous minimum-angle rule without an explicit one-degree
  lower bound, retained to reproduce earlier results.
- `min_angle`: CURRENT requested rule. First discard tracks with polar angle
  <= 1 degree, then choose the smallest angle among remaining front-pair tracks.
  A track exactly at 1 degree is excluded. A sub-degree track does not veto
  another track above the bound. Rank without outgoing momentum; track ID breaks
  exact angle ties. A chosen eligible track failing another cut is not replaced.
  `--min-angle-deg` configures the strict bound; default is 1.0.

Multiplicity always refers to the original front-pair population, not the
angle-filtered population. Below 12 degrees, the chosen track must also cross
FPP3–FPP4; at/above 12 degrees the front pair suffices. Other requirements:
legacy cone test, analyzer z-close interval (111.4215 to 167.3015 cm for this
block), s-close <= 1 cm, and 0.07 <= pT <= 1.0 GeV/c. pT uses known incident
momentum, not the unmeasured outgoing momentum.

### Verified numerical results

Same 100,000 central-ray 6.7 GeV/c protons, one 55.88 cm CH2 block, FTFP_BERT.
All FOM entries below include Pe=0.85 once (Pe squared).

| Selection | Useful events / efficiency | RMS Ay | Intrinsic FOM/proton | Polarized FOM/proton |
|---|---:|---:|---:|---:|
| Original momentum ranking | 26,264 / 26.264% | 0.0631122 | 0.00104613432 | 0.000755832045 |
| Exclusive single track | 12,492 / 12.492% | 0.0585949 | 0.000428895242 | 0.000309876813 |
| Previous minimum angle, no 1-degree cut | 28,939 / 28.939% | 0.0638007 | 0.00117797011 | 0.000851083403 |
| CURRENT minimum angle strictly >1 degree | 25,892 / 25.892% | 0.0670407 | 0.00116370570 | 0.000840777365 |

The original useful sample included 13,772 multi-track events, contributing
59% of its Ay-squared sum. The exclusive single-track result retains 41% of
that original FOM. Andrew's quoted planning FOM is about 0.00036/proton;
our exclusive-single-track 0.000310 is about 14% lower. This explains why the
initial apparent factor-of-two disagreement was misleading.

The CURRENT >1-degree sample comprises 9,663 single-track and 16,229 multi-track
useful events. It retains 98.789% of the previous no-one-degree-bound minimum-
angle FOM. Keep the distinction between the unchanged single-track reference
and the new angular restriction on the minimum-angle policy explicit.

With the earlier provisional incident yield N=7.5–7.7M, component errors are
0.02934–0.02895 for exclusive singles and provisionally 0.01781–0.01758 for the
CURRENT minimum-angle rule. These are NOT uncertainties in mu_p GE/GM.

### Interpretation accepted by user

Single-track FOM agrees reasonably with Andrew's estimate. Recovering some
multi-track events could improve statistical reach. However, the numerical
multi-track FOM assigns the GEp-III SINGLE-track Ay model to those events;
this assumption has not been validated. All tracking remains ideal and
truth-associated, without detector inefficiency/resolution/noise/merged-hit
or full multi-track pattern recognition. Do not present the gain as established.

The user explicitly requested removing the “Next test” block from the last
slide. Do not reintroduce that proposed test or a replacement “Open issue” block.
The brief provisional-Ay footnote remains.

## Question 2: parametrization and averaging

Our `analysis/analyzing_power.py` matches the locally available primary SBS
macro `/Users/brash/g4sbs/root_macros/GEP_FOM_quick_and_dirty.C` (lines 95–131,
467–468, 502–504) and Andrew's email description. His current private source
was not inspected. A numerical comparison on 1,407 momentum/angle combinations
agreed within 9.5e-16 relative difference.

Model, with p and pT in GeV/c:

    b = weighted mean of 2.376 +/- 0.189, 2.776 +/- 0.212, 2.292 +/- 0.145
      = 2.426146890797317
    Ay_max(p) = 0.0342 + 0.348/p
    Ay(p,pT) = Ay_max(p) sqrt(2 e b) pT exp(-b pT^2)
    pT = p sin(theta)

Equivalently Ay=(0.124207234 + 1.26386308/p) pT exp(-2.42614689 pT^2),
zero outside the adopted useful-pT window. At p=6.7: peak Ay=0.0861403 at
pT=0.453969, theta=3.88514 degrees. Peak, ordinary mean, and RMS are different.

For the SAME 12,492 useful exclusive single-track events:

- Ordinary mean Ay = 0.05483990686.
- RMS Ay = sqrt(mean(Ay^2)) = 0.05859487425.
- Pe^2 epsilon mean(Ay)^2 = 0.0002714333733.
- Pe^2 epsilon mean(Ay^2) = 0.0003098768126.
- Event-dependent Ay weighting therefore adds 14.163% FOM, not a factor of two.

Our FOM integrates Ay^2 event by event. Table VII's published bar(Ay) is an
asymmetry-calibrated effective average, not the RMS; its 0.06–1.2 GeV/c pT
range also differs from the 0.07–1.0 window used here. For detailed extraction,
spin-transport/acceptance weighting matters; do not equate every published
effective mean with an unweighted arithmetic mean.

Primary paper: Puckett et al., Phys. Rev. C 96, 055203 (2017), Sec. III.B.7,
https://arxiv.org/html/1707.08587 . No Ay code change was needed for question 2.

## Rough SBS analyzing-power extraction

The user proposed extrapolating R=mu_p GE/GM to Q2=11, estimating Pl/Pt, then
using Andrew's plotted asymmetry to estimate effective Ay. We checked Andrew's
March 2, 2026 collaboration talk, slides 9 (spin), 10 (layout/polarization),
36 (data versus simulated asymmetry):

https://indico.jlab.org/event/1009/contributions/18155/attachments/13773/22270/PuckettGEP.pdf

Local downloaded copy was `/private/tmp/PuckettGEP_March2026.pdf`; re-download
on the laptop if needed. The talk's slide 36 is also embedded on page 1 of
`andrew.pdf`.

Read directly from the fit boxes:

- Data: c1 = 0.001119 +/- 0.003662; s1 = -0.02971 +/- 0.00403.
- Simulation: c1 = 0.004153 +/- 0.002613; s1 = -0.03147 +/- 0.00261.
- Use absolute sine amplitudes to avoid convention/sign ambiguity.
- Data/simulation sine-amplitude ratio = 0.944 +/- 0.150, propagating independent
  fit errors. Interpreting this as an Ay scale ratio requires matching Pe,
  accepted kinematics, spin transport, and sample conditions.

Illustrative Born kinematics E=10.7 GeV, Q2=11 GeV2, R=0.2:

    epsilon = 0.6836690815; tau = 3.1237416342
    Pl(Pe=1) = 0.7289739740; Pt(Pe=1) = -0.0266174758
    Ay_eff ~= |s| / (Pe Pl |sin chi|)

Andrew quotes about 87% beam polarization and shows this setting near maximal
longitudinal-to-transverse precession. Conditional estimates:

- |sin chi|=1: Ay_eff=0.0468 +/- 0.0064.
- chi=1.3 rad: Ay_eff=0.0486 +/- 0.0066.

Chi=1.3 is illustrative, not a measured acceptance average. The errors above
propagate ONLY the plotted asymmetry error. Approximate central kinematics,
acceptance-weighted spin transport, beam polarization, background dilution,
and sample definition limit an absolute extraction. Varying R from 0 to 0.4
changes Pl by <0.5%, so the rough result is insensitive to the guessed R.

User agreed this is roughly consistent with our SINGLE-track ordinary mean
0.05484, within about one statistical sigma. Compare to the mean, not our RMS
0.05859. This cross-check supports the nominal Ay scale but does not calibrate
multi-track Ay or directly measure the Ay^2 moment used in weighted FOM.

## Presentation: current 13-slide deck

Established source is editable LaTeX/Beamer, Madrid/dolphin, 16:9, 10pt.
Preserve this format rather than converting to PowerPoint or a cloud deck.

1. Title / working configuration (Updated 10 November 2026).
2. What we set out to establish.
3. Elastic yield benchmark.
4. HMS trigger.
5. NPS inclusive-electron singles.
6. Run 6887 non-electron excess.
7. Single-analyzer small-angle acceptance.
8. Chamber geometry and wire fits (clarified that FOM selection is truth-associated).
9. **Single-track vs. Multi-track FOM** (EXACT user-requested title).
10. Analyzing-power parametrization and FOM weighting (question 2).
11. SBS asymmetry supports the analyzing-power scale (rough extraction).
12. Smallest-angle selection recovers useful multi-track events (latest plot).
13. Multi-track recovery could improve the FPP statistical reach (component
    precision table and brief provisional-Ay footnote; NO “Next test” block).

Source notes contain detailed source references, assumptions, and exact values.
All changed slides were rendered and visually inspected. Final PDF has 13 pages.
The build has an existing small overfull-hbox warning around line 170 on the
unchanged NPS slide; no new overflow was observed in the edited FPP slides.

Build from the presentations directory with:

    latexmk -pdf -interaction=nonstopmode -halt-on-error gep4_reboot_feasibility_2026-11-04.tex

Local `/Library/TeX/texbin/latexmk` and `pdflatex` are installed. Generation was
done in temporary staging with the original `../plots` and
`../trigger_particle_composition` assets available, then the checked .tex/.pdf
and new figure were installed in SIMC. Tracked LaTeX auxiliary/build files in
the SIMC checkout were not refreshed; regenerate them as needed, without
assuming the old .nav/.aux reflect the current deck.

## Farm changes and verification

Farm default remote shell is tcsh. For shell scripts use `ssh ifarm 'bash -s'`
and feed a Bash script. ROOT 6.30.04 / Geant4 11.2.1 are available. ROOT Python
bindings require the environment's `/usr/bin/python3` (3.9.25); CMake otherwise
finds Python 3.13, which lacks matching ROOT bindings.

Modified tracked FPP files:

- CMakeLists.txt: policy and single-analyzer integration checks.
- include/EventAction.hh, src/EventAction.cc: extracted shared per-track
  calculation and added candidate-table serialization.
- src/RunAction.cc: candidates ntuple schema.
- README.md: PRE-EXISTING user edit about 22-inch hardware; preserved unchanged.

New FPP files:

- analysis/track_selection.py
- analysis/compare_track_selection.py
- analysis/plot_track_selection.py
- tests/test_track_selection.py
- jobs/run_track_selection.slurm
- TRACK_SELECTION_STUDY.md
- analysis/__pycache__/ is also untracked; not substantive work.

A separate `build-track-selection` preserved the original executable:

    cmake -S . -B build-track-selection -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DPython3_EXECUTABLE=/usr/bin/python3
    cmake --build build-track-selection -j4
    ctest --test-dir build-track-selection --output-on-failure

All six CTest entries passed. The policy suite now contains nine examples,
including the strict >1-degree boundary and filter-before-ranking behavior.
Slurm job 14574698 completed 100,000 transport events normally (exit 0:0,
12m02s, farm180222). ROOT closed normally. 240,057 unique candidate rows were
recorded; event/track identity and multiplicity checks passed.

Every one of the 41 original physics branches matched the old transport file
EXACTLY by entry. The old ROOT sample predates a metadata persistence fix
already in HEAD: all old event IDs=-1, incident p=0, weight=1. The comparison
explicitly checks those uniform defaults and excludes only these three fields.
The new output correctly retains actual CSV IDs, weights, and 6.7 GeV/c p.
Original published FOM reproduction used the monoenergetic p=6.7 override.

The candidate table independently reproduces the old selection. Original,
single-track, and no-one-degree minimum-angle summary statistics reproduced
identically after adding the >1-degree rule. An independent CSV check verified
all new chosen angles >1 degree; single-above-threshold plus recovered-multiple
counts and Ay^2 sums close. All useful selected tracks were forward.

No new transport is needed to change these offline selection cuts. Recalculate:

    python3 analysis/compare_track_selection.py \
      runs/track_selection_14574698/transport.root \
      runs/track_selection_14574698/NEW_OUTPUT_STEM \
      --min-angle-deg 1 \
      --baseline-root runs/single_55p88cm/transport.root
    python3 analysis/plot_track_selection.py \
      runs/track_selection_14574698/NEW_OUTPUT_STEM

Use a fresh output stem to preserve earlier results. The existing outputs are
`comparison.*` (earlier policy), `comparison_gt1.*` (current policy), their
`*_events.csv` audit files, REPORT.md, REPORT_gt1.md, source_snapshot.tar.gz,
and source_snapshot_gt1.tar.gz. The archives contain changed/new source and
analysis files; inspect/extract into scratch before applying anything over a
checkout, especially because the farm has uncommitted user work.

## Remaining work / boundaries

- Continue Andrew question 3 when requested; reconcile incident versus useful
  proton counts, event selections, 70% global factor, Born versus radiated yields,
  and beam-time normalization before comparing Hall C/SBS precision.
- Question 4: investigate actual mechanical space before claiming thicker CH2
  fits. Current 55.88 cm is a constructed legacy baseline, not an optimization.
- Question 5 has a rough empirical consistency check, not a full SBS Ay extraction.
- Full FF-ratio uncertainty requires spin transport and covariance. Never label
  the quoted component errors as sigma(mu_p GE/GM).
- Multi-track FOM remains conditional on analyzing power for that selected
  population. The GEp-III paper discusses lower Ay for historical multi-track
  samples; it does not validate this new smallest-angle policy.
- No permission to commit/push was given. Preserve pre-existing changes. The
  handoff authoring request does not itself authorize publishing or syncing
  private correspondence to an external service.
