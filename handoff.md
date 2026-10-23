# SIMC repository handoff

Reviewed 2026-10-22. This document records the initial repository orientation and static review; it is not a physics certification.

## Session context and baseline

- User: this is a newly downloaded JLab Hall C SIMC repository. Requested a comprehensive review to learn the code and a persistent handoff for future sessions.
- Workspace: `/Users/brash/simc_gfortran` on Apple Silicon macOS; shell zsh.
- HEAD: `59ccdf62b37ad606e1885a793cd209df7036aa6a` (2026-10-01, merge PR #74).
- Origin: `git@github.com:brash99/simc_gfortran` (fetch/push). No fetch or remote changes performed.
- Initial working tree was clean; 597 tracked files. No applicable AGENTS.md found in this repository. Check again in future sessions.
- Only this handoff was added. No production code, input decks, tests, compiler configuration, dependencies, or tracked data were changed.
- Review used the code-verification skill. Future implementation requires the code-work workflow and a user request authorizing fixes.

## What SIMC does

SIMC is an acceptance-focused, weighted Monte Carlo for electron coincidence reactions. It generates a selected reaction over phase space relevant to spectrometer acceptance, applies target/radiative/transport effects, reconstructs observables, and computes weighted yields. It is not a GEANT detector simulation or a generator of all backgrounds at once.

Supported families in the source include elastic/quasielastic (e,e′p), exclusive pion and kaon production, Delta production, semi-inclusive pions/kaons, diffractive rho production and decay, nuclear elastic scattering, and neutral-pion/photon calorimeter configurations. Availability in code does not establish model accuracy for arbitrary kinematics or targets. In particular dbase.f warns that the Delta cross-section model is only set up for proton targets; phase-space mode has an explicit implementation caveat.

## Architecture and navigation

| Location | Responsibility |
| --- | --- |
| `simc.f` | Main program, event loop, spectrometer dispatch (`montecarlo`), central kinematics, normalization, reports and generated histograms. |
| `dbase.f` | CTP input loading, primary/extra input decks, parameter registration (`regallvars`), defaults, integer-to-logical conversion, reaction and arm selection. |
| `modules.f` | `structureModule`: sequenced derived types for events, arms, targets, cuts, ranges and weights. |
| `histograms_module.f` | Histogram type module; used with histogram include files. |
| `simulate.inc`, `structures.inc` | Shared runtime declarations/common blocks and event-related state. |
| `constants.inc`, `spectrometers.inc`, `target.inc` | Physical constants, instrument definitions and target state. |
| `init.f` | Target, generation-limit, radiative and theory-table initialization. |
| `event.f` | Generation, vertex/reconstructed kinematics, physics weights, coordinate-angle helpers and two-body decay. |
| `target.f`, `enerloss_new.f` | Target geometry/material traversal, energy loss and related corrections. |
| `radc.f`, `brem.f`, related `.inc` files | Radiative corrections and bremsstrahlung. |
| `physics_proton.f` | Elastic electron-proton and de Forest off-shell prescriptions. |
| `physics_pion.f`, `physics_kaon.f`, `physics_delta.f` | Exclusive-production cross-section models. |
| `semi_physics.f`, `semi_dilution.f` | Semi-inclusive cross sections and associated nuclear treatment. |
| `rho_physics.f`, `generate_rho.f`, `rho_decay.f`, `pizero_decay.f` | Rho generation/model/decay and neutral-pion decay. |
| `jacobians.f`, `loren.f` | Jacobians and Lorentz transformations. |
| `sf_lookup.f`, `benharsf/` | Spectral-function lookup and auxiliary spectral-function code/data. |
| `hms/`, `sos/`, `shms/`, `hrsl/`, `hrsr/` | Instrument apertures, transport model data, hut response and reconstruction. Historical optics coexist with active files. |
| `calo/` | Calorimeter transport/reconstruction. |
| `shared/` | COSY transport (`transp.f`), projection, scattering and coordinate rotations shared by instruments. |
| `trg_track.f`, `poltar/` | Target magnetic-field tracking and polarized-target supporting material. |
| `cteq5/`, `fdss/`, `f1f2tables/` | PDF, fragmentation and structure-function support/tables. |
| `cern/` | Vendored numerical routines including RANLUX; not a requirement to install CERNLIB for the core. |
| `CTP/` | Bundled C parameter parser/registry and RPC-related support library. |
| `NtupleInit.f`, `results_write.f`, `NtupleClose.f` | Reaction-specific binary-output schema and serialized event values. |
| `util/root_tree/` | Fortran binary-to-ROOT converter plus C++ ROOT bridge `froot.c`. |
| `util/ntuple/` | Optional PAW ntuple converter, requiring CERNLIB. |
| `util/recon_hcana/` | ROOT postprocessing to add hcana-style reconstructed variables and normalized weights. |
| `util/sidisxsec/`, `cosy/` | Auxiliary cross-section and optics tooling, not part of the main executable. |
| `infiles/` | Example input decks and old-to-new parameter-name converter. |
| `documents/`, `CHANGES*.TXT`, `readme.gfortran` | Historical physics/build documentation and change notes. Treat old instructions as historical evidence. |

The main Makefile is the authority for active source selection. It links `simc.f` and `event.f`, not `simc_multipiNew.f` or `event_multipiNew.f`. It selects `F1F2IN21_v1.0.f`, not the other similarly named top-level model files. Do not assume every source in the checkout participates in the executable.

## Event and data flow

1. `dbase_read` registers variables, reads a filename from stdin, loads a CTP deck, then loads its optional `extra_dbase_file`. No slash means lookup under `infiles/`; `.inp` is added if no dot is found.
2. Input processing derives reaction flags and particle/recoil masses. `init.f` establishes limits, target properties and radiative state.
3. The program initializes central kinematics and RANLUX, and saves the initial generator state.
4. `generate` produces an event and applies generation/vertex effects. `montecarlo` transports particles through selected arms, apertures and hut response, including optional decay and field tracking.
5. `complete_recon_ev` reconstructs measured kinematics; `complete_main` computes cross sections, spectral-function factors, Jacobians and event weights.
6. Successful events contribute to diagnostic distributions and optionally binary output. Normalization and reports are calculated at the end.

Keep distinct the records `vertex`, `orig`, `recon`, and `main`: they represent different stages/quantities, not interchangeable copies. The code uses global common blocks, saved/static storage, and implicit external procedure interfaces. Assume serial, non-reentrant execution unless proven otherwise.

`modules.f` explicitly warns that `neventfields` must track changes to the event type. Any record-layout change requires checking copy operations, sequenced storage, common blocks and every output consumer.

## Inputs, units and reproducibility

- Input decks use CTP `begin parm ...` / `end parm ...` groups and semicolon comments. Registration in `dbase.f` determines accepted parameter names.
- Modern derived-type names use `%`, for example `spec%e%P`, `targ%A`, `EXPER%charge`. Several shipped examples (including `test_eep_h.inp`) still use old dotted names. Inspect the primary AND secondary deck before running; do not assume examples are ready to use unchanged.
- `infiles/convert_inputfile.sh` is a historical sed conversion tool. Inspect its full behavior and preserve originals before using it. `run_simc_recon.sh` invokes it automatically.
- Typical input units: beam energy MeV; momentum MeV/c; central angles degrees; acceptances often mrad; momentum deviation percent; charge mC; target density g/cm³; target thickness mg/cm². Internal calculations and output conversions differ: inspect `dbase.f` and `results_write.f` for the specific variable. Many output energies/momenta are divided by 1000.
- `ngen > 0` requests successful events; `ngen < 0` limits attempts. Prefer a small negative count for an initial smoke run: impossible acceptance can otherwise run indefinitely seeking successes.
- Arm codes in `dbase.f`: 1 HMS, 2 SOS, 3 HRSR, 4 HRSL, 5 SHMS, 6 obsolete/stops, 7 calorimeter on HMS side, 8 calorimeter on SHMS side. Legacy comments may omit newer codes.
- `call_ranlux.f`: luxury level 3. `random_seed=-1` uses time; otherwise seed is passed to RANLUX. `random_state_file` can initialize from 25 saved integers. The initial state is written under `outfiles/<stem>_start_random_state.dat`.
- Reproducibility requires input and extra deck, compiler/flags, commit, optics/model tables and RNG state, not merely a seed. Buffered `grnd` restart behavior mid-run was not verified.

## Outputs and normalization

Run from the repository root: runtime tables and output paths are relative. A fresh checkout has `runout/` and `outfiles/`, but no `worksim/`; create a suitable work directory before enabling binary output.

| Output | Purpose |
| --- | --- |
| `runout/<stem>.out` | Redirected stdout from wrapper scripts. |
| `outfiles/<stem>.hist` | Text run report with settings, statistics and normalization. |
| `outfiles/<stem>.gen`, `.geni` | Generated-variable histogram diagnostics. |
| `outfiles/<stem>_start_random_state.dat` | Initial RNG state. |
| `worksim/<stem>.bin` | Fortran sequential unformatted event stream, when ntuple output is enabled. |
| `worksim/<stem>.root` | Converted ROOT tree. |
| `worksim/recon_hcana_<stem>.root` | Optional hcana-style postprocessing output. |

The binary header contains branch count and 16-character labels; each event quantity is written separately as an unformatted record. This is compiler-record-framed output, not a portable flat array. Use compatible Fortran runtime conventions when converting it. The ROOT converter narrows real*8 values to real*4 branch values.

`event.f:complete_main` forms weight from spectral-function weight × Jacobian × generation weight × cross section × target-nucleon factor, with survival corrections where applicable. `simc.f` forms `normfac = luminosity / ntried * nevent`, then multiplies by reaction-dependent generation volume. `recon_hcana` documents `fWeight = Weight * normfac / Ngen`. Verify the meaning of its parsed Ngen for the chosen run mode before analysis. Do not equate raw entries or raw Weight with physical yield.

## Build and local environment

- Installed and checked: GNU Fortran 14.2.0 (`/opt/homebrew/bin/gfortran`); Apple clang 21.0.0 as `gcc`; ROOT 6.40.04 (`/Users/brash/root_install/bin/root-config`); `/usr/bin/make`.
- Core build entry point: GNU `make` in the root; it builds bundled CTP and links `simc`. Main executable does not require external CERNLIB or ROOT.
- Core flags include fixed line length 132, `-ff2c`, `-fno-automatic`, `-fdefault-real-8`. These affect precision, calling conventions and static lifetime; do not casually remove them. Modules are compiled separately by the `mods` target.
- CTP intentionally uses `CTP/O.Linux/Linux/lib/libctp.a` even on Darwin. It includes RPC generation and C/Fortran interoperability headers. `CTP/Makefile.variables` contains obsolete GCC-major detection and Linux-specific include paths; a full modern macOS build is not established by this review.
- `util/root_tree/Makefile` hardcodes a JLab CVMFS ROOT 6.30.04 path, configures Fortran explicitly only for Linux, and links Linux-style libraries/paths. Local ROOT being installed does not make that Makefile ready for macOS.
- Optional PAW conversion has separate makefiles and CERNLIB requirements.
- Avoid starting with parallel make: module dependencies are not explicitly attached to all objects that USE them. `make simc` bypasses the aggregate `mods` prerequisite; `make -j` can race it.
- Main Makefile dependency generation scans INCLUDE statements, not full module dependencies. The `$(CH)/%.o` rule incorrectly names the SHMS source directory; generic rules may mask it. `clean` omits fdss objects; `real_clean` uses an OS path inconsistent with Darwin's O.Linux CTP layout. These are static build-system observations, not reproduced build failures.

After separately authorizing/building any needed portability fixes, the intended manual workflow is: build core; prepare valid decks; create worksim; feed a stem to `./simc`; inspect the text report and binary; build the ROOT helper separately and run it from `util/root_tree`. Wrappers are conveniences, not verification of successful simulation.

## Review findings and operational risks

### High: output arrays can overflow for combined neutral-pion/target-field output

High-confidence static finding in `NtupleInit.f` (`NtupleTag(80)`), `results_write.f` (`ntu(80)`), and `util/root_tree/make_root_tree.f` (80-element buffers). The ordinary exclusive-pion branch has 64 fields; neutral-pion output adds 12, then target-field output adds 8, reaching 84. No capacity guard precedes writes. Other combinations should be counted explicitly rather than trusting numbered comments. This can corrupt memory without bounds checking or abort with checks. Increase/centralize capacity or derive schemas dynamically, and validate all supported flag combinations. No runtime reproducer was run.

### Medium: ROOT converter silently limits output to 10 million events

High-confidence static finding: `util/root_tree/make_root_tree.f` sets `nev=10000000` and loops only to that count, then closes the ROOT output normally even if the input contains more events. Consumers can receive an apparently successful truncated dataset. Read until EOF or explicitly reject excess data. Both `run_simc_tree` and `run_simc_ntup` remove the binary after conversion without checking conversion success; the tree wrapper would also discard the full binary after truncation. Preserve binary input until conversion and entry counts are verified.

### Medium: wrapper failure handling and platform assumptions

`run_simc` has no shebang and no argument/error checks. Tree/PAW wrappers require `/usr/bin/csh`, do not reliably propagate failures, and unconditionally remove binary input. `run_simc_recon.sh` uses `set -e`, but a nested wrapper that reports success can hide an earlier failure; its existence-only output checks can accept stale files from an earlier run. It rebuilds the ROOT helper but does not build the core executable. Use fresh stems and validate logs, timestamps, schema and entry counts. Plain Fortran STOP paths may not provide a nonzero status.

### Verification gaps affecting scientific use

No automated regression suite, CI configuration, or golden numerical oracles were found. Files named test*.inp are example decks, not self-checking tests. Physics tables, optics validity, reference yields, radiative tails, nuclear models and hcana agreement were not independently validated. Old documentation and comments can describe superseded code. Do not infer scientific correctness from successful compilation.

## Verification evidence and boundaries

| Question | Evidence | Result |
| --- | --- | --- |
| Understand architecture/configuration/output | Read README/build files, key Fortran orchestration and calculation paths, include/type definitions, example decks, wrappers and converters. | Repository orientation completed; this is a map and targeted review, not a line-by-line proof of every file. |
| Main Fortran compiler acceptance | In a temporary directory compile modules with `gfortran -fsyntax-only -ffixed-line-length-132 -ff2c -fno-automatic -fdefault-real-8 -I<repo> -J<tmp>`, then each source selected from Makefile OBJ groups plus simc.f. | Modules and all 63 selected source files exit 0. Temporary module files removed automatically. |
| Full executable build/link | Not executed; CTP C/RPC build, linking and module scheduling remain unchecked. | Unverified. |
| Runtime end-to-end and ROOT conversion | Not executed. | Unverified. |
| Physics correctness | No independent numerical reference comparison performed. | Unverified. |
| Preserve checkout | Initial git status clean; only handoff.md intentionally added. | Production files preserved. |

No dependencies installed; no input conversion, simulation, cleanup, commits, pushes, external documentation fetches, or source repairs performed. No full PDF physics-document review was performed. Findings above distinguish direct syntax-check evidence from static deductions.

## Suggested continuation

1. Re-read this handoff, `git status`, current instructions, README and Makefile; verify HEAD/toolchain before relying on recorded facts.
2. If the user requests setup, establish a serial full core build on this Mac and diagnose actual CTP/compiler/link failures before proposing portability edits.
3. Resolve the confirmed output capacity and conversion integrity issues before affected production workloads.
4. Prepare a small fixed-seed, fixed-attempt hydrogen elastic deck in modern syntax, including its extra deck. Run and retain log, report and binary; verify attempted/accepted counts and finite physical values.
5. Establish ROOT helper compatibility; compare binary and tree entry counts/schema. Validate hcana postprocessing separately, particularly arm selection, mass hypotheses, sentinel handling and normalization.
6. Obtain a trusted reference configuration/yield from the user's experiment before claiming physics validation. Then extend checks to the reaction, optics and corrections relevant to that experiment.

Useful starting sources: README.md for intended scope; dbase.f for actual parameter semantics; simc.f/event.f for execution and weighting; Makefile for active sources; NtupleInit.f/results_write.f for actual output schema; util/recon_hcana/README.md for postprocessing usage. Historical documents are supporting context, not current executable specifications.

## GEp-IV reboot study added after the initial repository review

The user is exploring an elastic ep recoil-polarization measurement near Q² = 11 GeV² with electrons in NPS and protons in HMS+FPP. [gep_hallc_reboot/findings.md](gep_hallc_reboot/findings.md) is the main evolving study record. It includes Andrew Puckett's preliminary email/kinematics review and a detailed review of the 2011 PAC37 HMS+BigCal proposal. Email PDFs are in `gep_hallc_reboot/messages/`; the original and revised proposals are in `gep_hallc_reboot/proposals/`, with source links in its README.

Key historical anchors: E = 11 GeV and Q² = 11 (current email uses E = 10.7); 60 events/mC in Table 1; 85.7% elastic retention and 0.61% π0 contamination with loose cuts; separate GEANT3 FPP COM and COSY spin transport. The old proposal also considered a compact PbWO4 HyCal option. Table 4's ratio-error normalization is ambiguous relative to the μp-scaled Figure 28; resolve before comparing precision. Exposure conventions and old/current cut efficiencies must also be reconciled. Neither the historical run nor Andrew's exact preliminary run has been reproduced. Keep the preliminary email estimates within the core group as requested in the source message.

Local continuation on 2026-10-22 established a working Apple Silicon macOS build. The Darwin Makefile now supplies GNU Fortran with the active SDK and host deployment version, and compiles modules with the common Fortran flags. `make all` exits 0. A new fixed-seed deck, `infiles/gep_reboot_q11_smoke.inp`, ran twice for one million fixed trials at the preliminary 10.7 GeV/Q²≈11 setting. Both runs were byte-for-byte reproducible in the generated diagnostics and RNG state. There were 11,013 contributing/HMS-successful events and zero modeled NPS horizontal or vertical extent rejections, implying a rule-of-three upper bound near 0.027% for the rectangular-aperture loss within this provisional sample. This is not detector-response or production-yield validation. The run emitted low-energy `Eloss>Total KE` warnings that require follow-up. Full details and limitations are in [gep_hallc_reboot/q11_smoke_test.md](gep_hallc_reboot/q11_smoke_test.md).

The current checkout now also has input-configurable calorimeter half-width and half-height, with the existing NPS dimensions preserved as defaults. This enabled a fixed-seed reconstruction of the PAC37 Q² = 11 HMS+BigCal elastic baseline without changing the NPS model. The radiated result is 41.936699 events/mC; BigCal rejects only 7 of 9,782 calorimeter trials. This is 30.1% below the proposal's raw 60 events/mC, 9.6% below its 3σ-cut comparator, and 18.4% below its alternative 6σ-cut comparator. A no-radiation diagnostic gives 50.510831 events/mC. The checked-in deck is radiated. See [gep_hallc_reboot/pac37_elastic_baseline.md](gep_hallc_reboot/pac37_elastic_baseline.md). The user accepts agreement within 20% of the cut-level historical yields as sufficient, so this benchmark is closed and recovery of the original artifacts is optional. The critical path now begins with selecting and documenting the best current radiative treatment and signal/cut convention. Repository commit `7896ebc` documents a June 2011 radiative-initialization bug fix that remains useful historical context.

[gep_hallc_reboot/beam_time_yield_check.md](gep_hallc_reboot/beam_time_yield_check.md) verifies Andrew's displayed exposure arithmetic. His 3.25 Hz for 45 full 24-hour days gives exactly 12.636 million events and implies 43.333 events/mC at 75 μA. The local provisional smoke normalization is 41.482 events/mC, or 3.111 Hz and 12.096 million events over the same full exposure, 4.27% lower. The displayed spreadsheet applies no separate floor-time or detector-efficiency factor; those conventions and the contents of the quoted rate remain unresolved.

User decision: use the reproducible local SIMC normalization—not Andrew's unreproduced estimate—as the working baseline for continuing studies. Projections should therefore start from 41.481955 events/mC, 3.111147 Hz at 75 μA, and 12,096,138 elastic events for 45 full 24-hour beam-on days. Retain Andrew's numbers only as an external comparison until his exact artifacts can be reproduced.
