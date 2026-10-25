# NPS GEANT4 simulation and validation survey

Established 2026-10-23.

## Question

Does an NPS GEANT4 detector simulation exist, and how far has it been benchmarked against Hall C operating data for the GEp-IV questions of calorimeter response, trigger rate, and trigger efficiency?

## Located simulation

The public [HallC_NPS repository](https://github.com/gboon18/HallC_NPS) is the NPS GEANT4 package identified by the [CUA NPS documentation index](https://wiki.jlab.org/cuawiki/index.php/Documents). The index points users to `DVCS_evt_gen/`, a [2020 user guide](https://wiki.jlab.org/cuawiki/images/f/fa/User_Guide.pdf), and a [2023 beam-pipe geometry update](https://wiki.jlab.org/cuawiki/images/e/e3/NPS_GEANT.pdf).

The public package models the target and scattering chamber, downstream beam pipe, sweeper magnet and field map, and the 30 x 36 PbWO4 NPS array. It records deposited energy per crystal and passes the crystal signals to external NPS clustering/reconstruction software. The user guide says that optical-photon transport is disabled by default because of its computational cost.

The public master branch was inspected at commit `659b1443bd075063b87a12a17c90d4da04c58e81`, dated 2020-11-02. Its documented ifarm setup uses Geant4 4.10.03-era software and external packages under `/work/hallc/nps/hosan/`. It is therefore an identifiable historical starting point, not the maintained environment.

The maintained workflow was subsequently located on ifarm. `/group/nps/hhuang/analysis/DVCS_NPS2023/DVCS_analysis/Simulation/DVCS` is an active checkout of [`hhuang-hep/NPS2023_Simulation`](https://github.com/hhuang-hep/NPS2023_Simulation), commit `2cd1a04d...` (2026-01-21). Its setup points to these component checkouts: `HallC_NPS` commit `5070d37e...` (2026-08-11), `NPS_SOFT` commit `2cb5668a...` (2026-01-21), `dvcs_gen` commit `49e05c62...` (2026-01-21), and the NPS branch of `mc-single-arm` at `024637c4...` (2025-12-15). The documented and observed environment uses ROOT 6.30.04 and GEANT4 11.2.1. January 2026 batch logs confirm that this chain ran with the NPS geometry, event generator, HMS transport, and reconstruction; this is operational evidence beyond the old public snapshot.

The workflow writes all 1080 crystal energy deposits and final reconstructed photon clusters, supports exclusive pi0 generation, and records HMS transport/reconstruction variables in one ROOT tree. This is a promising bridge from the present SIMC generator to realistic shower containment and cluster reconstruction. It is not, by itself, a VTP trigger simulation.

## What the public source does and does not model

In `DVCS_evt_gen/DVCS/src/EventAction.cc`, the simulation:

- saves energy deposition for all 1080 crystals;
- applies a fixed 1.04 correction described as compensation for shower energy lost beyond the crystal ends;
- gives every simulated pulse time zero because timing is not available from the GEANT4 calculation;
- invokes external `TCaloEvent` reconstruction with `TriggerSim(0.5)` and a -3 to +3 ns clustering window; and
- writes reconstructed cluster energy, position, and size.

The 0.5 GeV call is not a documented model of the production NPS VTP trigger and must not be interpreted as a hardware trigger threshold or rate prediction. The inspected public repository contains no realistic pulse-shape generation, fADC sampling, VTP window sums, accidental overlay, pileup, or rate-dependent dead time. The actual clustering implementation is also an external dependency rather than source contained in this repository.

## Evidence of post-installation comparison with data

A [May 2025 NPS collaboration presentation](https://indico.jlab.org/event/946/contributions/16514/attachments/12609/20085/20250506_DVCS_simulation_Hao_Huang.pdf) describes a newer workflow consisting of the DVCS generator, NPS GEANT4, an HMS single-arm Monte Carlo, and reconstruction. It compares reconstructed simulation with 2023-2024 production data at NPS distances of 3, 3.5, 4, and 6 m. The authors report that the NPS acceptance and the sweeper-magnet shadow match the data well.

That is meaningful validation of geometry and gross acceptance. It is not yet validation of the calorimeter energy response or trigger behavior. The same presentation lists photon calibration and smearing to reproduce the data resolution as future work, along with removal of dead blocks. It also notes simulated extra clusters associated with photons passing through the magnet wings.

A [June 2025 overview](https://indico.jlab.org/event/949/contributions/16809/attachments/12748/20546/DVCS_NPS_in_HallC_at_JLab_HUGS.pdf) says the combined event-generator + GEANT4 + HMS simulation package is ready and documents elastic and pi0 calibration work with real NPS data. It shows that suitable calibration samples and reconstruction machinery exist, but it does not establish a published quantitative GEANT4-versus-data energy-resolution or trigger-efficiency benchmark.

A dedicated [May 2025 NPS elastic-performance study](https://indico.jlab.org/event/946/contributions/16503/attachments/12589/20053/NPS_Performance.pdf) supplies the detector-response information that was missing from the preliminary GEp-IV scan. Across ten elastic settings with electron energies from 4.598 to 7.072 GeV, the reported cluster-energy resolution is 1.2-1.3%. The 4.598 GeV setting is particularly close to the 4.838 GeV GEp-IV central electron energy. The study also reports sub-nanosecond intrinsic single-block timing resolution. These are measured NPS performance numbers and are a better basis for the next response scan than an unsupported generic PbWO4 resolution formula. They do not by themselves validate the GEANT4 response or predict high-luminosity trigger rates.

A [2026 Hall C presentation](https://indico.jlab.org/event/992/contributions/17930/attachments/13651/22024/hallc_winter_meeting2026.pdf) describes a newer two-component SIMC + NPS GEANT4 pipeline for exclusive/SIDIS pi0 analysis. This is especially relevant to the desired GEp-IV architecture: SIMC supplies physics generation, cross sections, radiative effects, and HMS transport, while GEANT4 supplies the full NPS detector simulation. The maintained 2026 workflow and its component paths have now been resolved on ifarm as described above.

## Trigger-model status

The NPS experimental-readiness review explicitly called for simulation of pulse shapes, fADC sampling, the FPGA algorithm, and trigger validation. The collaboration response proposed a basic GEANT4-based study of 2x2, 3x3, and 4x4 sums with background overlay and thresholds. The inspected public 2020 source does not implement that program.

The installed trigger is nevertheless well documented. A [July 2024 NPS Trigger/DAQ presentation](https://indico.jlab.org/event/866/contributions/14914/subcontributions/234/attachments/11505/17805/NPS_July17_2024_Raydo.pdf) states that all 3x3 crystal views are evaluated in parallel. A seed hit must exceed its threshold and be a local maximum in space and time; the reported cluster contains energy, time, position, and hit multiplicity. Separate thresholds control the trigger decision and the surrounding 5x5 or 7x7 waveform readout. Trigger configuration files are stored in the run's EVIO data and in the Hall C logbook. The VTP banks record every qualifying 3x3 cluster and the trigger decisions, and a ROOT-based trigger emulator reconstructs the trigger banks from FADC waveforms for direct comparison.

This means that a GEp-IV rate study need not begin with a speculative trigger algorithm. The existing production firmware definition can be emulated, and recorded cluster scalers/banks can be used to anchor its response and rate behavior.

The [May 2025 deadtime study](https://indico.jlab.org/event/946/contributions/16513/attachments/12607/20081/Deadtime%20analysis_pjlin.pdf) found that NPS-specific deadtime had not yet been estimated. It also identified unresolved discrepancies between EDTM and TI livetime determinations. Thus, the existence of the hardware emulator and recorded banks should not be confused with a completed deadtime calibration.

The present evidence therefore supports the following bounded conclusion:

1. A detailed NPS GEANT4 geometry and shower simulation exists.
2. A later version has been compared with production data and reproduces gross NPS acceptance and the sweeper-magnet shadow.
3. Real elastic data show 1.2-1.3% cluster-energy resolution from 4.598 to 7.072 GeV, and real elastic and pi0 samples are available for further calibration and response studies.
4. The production 3x3 VTP trigger, its configuration provenance, recorded cluster banks, and a waveform-based emulator are documented.
5. No source or report inspected so far demonstrates a validated absolute singles-rate, NPS deadtime, or pileup model suitable for the proposed GEp-IV luminosity.

## Immediate next actions

1. Preserve the exact maintained revisions and determine the input adapter needed to pass the present GEp-IV SIMC elastic and pi0 samples through `/group/nps/hhuang/software/HallC_NPS` and its reconstruction chain.
2. Recover representative VTP cluster bank data and calibrated trigger-gain information. Run 6151 already supplies embedded trigger/readout thresholds and timing settings; its event-level spectra remain to be replayed or located.
3. Obtain the ROOT trigger emulator and reproduce its FADC-versus-VTP validation on selected production runs.
4. Use the 4.598 GeV elastic setting as the nearest response anchor to the proposed 4.838 GeV NPS signal. Quantify the full energy-response shape, position resolution, threshold bias, and dead-block effects rather than retaining only the quoted Gaussian width.
5. Resolve the EDTM/TI livetime discrepancy and establish an NPS-specific deadtime measurement before assigning a DAQ efficiency.
6. Feed the GEp-IV SIMC signal and inclusive-background samples into the maintained GEANT4/reconstruction chain and scan the actual 3x3 trigger near 70-80% of the elastic energy.

The public hcana/NPSlib code paths that enable this recovery are mapped in the archived [`nps_hcana_trigger_audit.md`](archive/dvcs_benchmark_exploration_2026-10-24/nps_hcana_trigger_audit.md).

## Maintained-workflow integration audit

The maintained workflow was inspected directly on ifarm after the data-driven rate study established runs 6151 and 2875 as the useful empirical anchors. The top-level driver, `NPS_HMS_simu.sh`, currently performs three coupled operations:

1. an internal DVCS or exclusive-pi0 generator embedded in the GEANT4 executable;
2. a separate HMS `mc-single-arm` transport; and
3. offline NPS clustering and photon reconstruction through `NPS_SOFT`.

That topology is not directly appropriate for GEp-IV. In the present GEp-IV chain, SIMC already generates the physics event, applies its event weight, and transports the proton through HMS. GEANT4 should receive only the NPS-bound electromagnetic particles—one scattered electron for elastic events or the accepted pi0-decay photons for the hard-bremsstrahlung background—and return the crystal response. Running the separate HMS Monte Carlo would duplicate proton transport and break the existing normalization.

The source-level radiation and material boundary is specified in
[`simc_geant4_interface_contract.md`](simc_geant4_interface_contract.md).
That audit also identified and corrected a defect in the SIMC calorimeter path:
`trip_thru_target` previously had no defined scattered-electron material branch
for `electron_arm=8`, although the GEp deck uses arm 8. Calorimeter arms 7 and 8
now include the liquid target and cell only; downstream material is deliberately
reserved for GEANT4.

### Minimal external-event contract

The smallest useful adapter should read one SIMC event at a time and provide:

- a stable SIMC event identifier;
- production vertex `(x,y,z)` in a documented Hall coordinate system;
- particle count and PDG identifiers;
- each NPS-bound particle's four-momentum, with explicit GeV units;
- the SIMC physics weight and optional model-variation weights; and
- enough provenance to identify the source deck and generator channel.

GEANT4 should emit, without applying physics weights:

- the same event identifier;
- raw deposited energy for all 1,080 crystals;
- total deposited energy and any material-loss diagnostics;
- generated-particle truth at the NPS face; and
- reconstructed cluster energy, position, and size in a subsequent, separately versioned response step.

The current `PrimaryGeneratorAction` is hard-wired to `TGenDVCS`, applies DVCS-specific HMS and calorimeter acceptance tests, and fires an electron plus a photon. The GEp adapter should be a separate primary-generator mode rather than an edit that silently changes the DVCS behavior. For elastic GEp events it should fire the scattered electron toward NPS; for the hard-bremsstrahlung pi0 sample it should fire both decay photons that SIMC generated. The proton remains entirely in the existing SIMC/HMS path.

### Configuration differences that must be made explicit

The executable currently obtains NPS distance, angle, HMS settings, and masked blocks from the DVCS run database. It also hard-codes a 10 cm target length. The GEp-IV baseline instead uses:

- 10.7 GeV beam energy;
- a 30 cm liquid-hydrogen target;
- a 4.838 GeV scattered electron at 26.651 degrees;
- a 6.735 GeV/c proton at 18.797 degrees; and
- NPS at a nominal 3 m distance.

These values must be supplied by an explicit GEp configuration file or command-line interface, not by pretending that an existing DVCS run number represents the geometry. The target vertex from SIMC should be authoritative; GEANT4 must not regenerate it.

The maintained `DVCS.cc` also has the sweeper-magnet field flag set to false. The GEp configuration must state whether the sweeper magnet and its material are physically present, and whether a field should be applied. This matters much more for an electron detected in NPS than for a DVCS photon and cannot be inherited silently from the photon workflow.

### Energy-scale defect found in the current chain

The maintained source presently applies the same 1.04 longitudinal-leakage correction twice:

1. `EventAction.cc` stores `fHadCalEdep[i] = eDep * 1.04` in the GEANT4 output tree; then
2. `Reconstruction.C` calls `AddPulse(1.04 * edep[i] * 1e-3, 0.)`.

Thus reconstructed cluster energies receive an effective factor `1.04^2 = 1.0816`, not the documented single 4% correction. This must be resolved before comparing the simulated energy scale with runs 6151 or 2875. The safest contract is to store raw GEANT4 energy deposition in MeV, apply no empirical correction in the transport stage, and treat any leakage/response calibration as a named reconstruction option validated against data.

The reconstruction also uses `TriggerSim(0.5)` for DVCS or `TriggerSim(0.2)` for pi0, followed by a -3 to +3 ns clustering window with every simulated block time fixed to zero. These are cluster-finding seeds, not the production 3x3 VTP trigger. Trigger efficiency must therefore be evaluated later from crystal energies using the documented firmware/emulator logic and the measured accidental overlay.

### First validation milestone

Before importing the full weighted SIMC samples, the private GEp branch should support a deterministic mono-electron gun and demonstrate:

1. energy conservation and the uncorrected 1,080-crystal sum;
2. stable block numbering, position axes, and NPS-face coordinates;
3. a central 4.598 GeV electron response comparable to run 6151 after applying an explicit response model;
4. a central 7.072 GeV response comparable to run 2875;
5. position and containment scans across the active face, including dead-block masks; and
6. reproducibility under fixed random seeds.

Only after these checks should the GEp-IV elastic and pi0 SIMC events be passed through the adapter. The data overlay and 3x3 trigger emulation remain downstream steps; they should not be folded into the GEANT4 transport itself.

## Private mono-electron validation

A private branch of the maintained `HallC_NPS` component now exists at
`/work/hallc/gep/brash/gep4_nps_geant4` on branch `gep-external-primary`, based
on maintained commit `5070d37ed4bbe24037b090abbe67d9a2c152ba80`. The shared
collaboration installation was not modified. The private executable adds a
backward-compatible `--mono-electron` primary mode, stores raw GEANT4 deposits
without the first of the two historical 1.04 factors, and records the primary
electron's first entrance into active PbWO4. The extended form

```text
--mono-electron E_GeV theta_deg vertex_z_cm nps_local_x_mm nps_local_y_mm
```

aims the electron at an explicit point on the NPS front face. Fixed seeds give
an exact event-by-event match for all 10,800 crystal deposits in repeated
ten-event runs.

The first 4.598 GeV test aimed at local `(0,0)` exposed an important geometry
artifact rather than a calorimeter-resolution problem. NPS has an even 30 x 36
array, so the nominal center is the intersection of four crystals. With 20.5 mm
crystals, 0.095 mm wrapping on each side, and a 1.0 mm gap, the pitch is
21.69 mm. Only four of ten primary electrons entered active PbWO4; the others
remained in or scattered through the central inter-crystal structure and made
small secondary deposits. This explains the initially broad 1 MeV--4.44 GeV
raw-deposit distribution.

A second test aimed half a pitch in both local coordinates,
`(x,y) = (10.845,10.845) mm`. All ten primaries entered crystal 522. This first
centered run still inherited a 3 m DVCS-database distance and gave a raw mean
of 4.51225 GeV with 17.64 MeV RMS. It established the coordinate convention and
removed the central-gap artifact, but it is superseded as a run-6151 response
anchor.

The elastic-performance run table shows that both the 6151--6168 and
2875--2885 calibration groups placed NPS at 9.5 m. The private interface now
accepts an explicit NPS distance and, when supplied, uses the stated central
angle for both the detector geometry and the face-coordinate transformation.
The resulting common centered tests are:

| Calibration anchor | Explicit geometry | Crystal entries | Raw mean | Raw RMS | RMS/mean | Mean containment | Range |
|---|---|---:|---:|---:|---:|---:|---:|
| run 6151, 4.598 GeV | 20.58 degrees, 9.5 m | 10/10 | 4.50101 GeV | 57.95 MeV | 1.287% | 97.89% | 4.34974--4.56059 GeV |
| run 2875, 7.072 GeV | 16.991 degrees, 9.5 m | 10/10 | 6.95769 GeV | 28.66 MeV | 0.412% | 98.38% | 6.90591--7.00541 GeV |

The 16.991-degree run-2875 angle is the elastic value derived from the verified
10.53941793 GeV beam energy and 7.072 GeV electron energy; the maintained DVCS
database incorrectly returned the run-6151-like 20.58-degree/3 m geometry for
run 2875. These ten-event numbers are transport/containment smoke tests, not
predictions of measured NPS resolution. Photostatistics, electronics,
calibration spread, pileup, and the measured response convolution remain
downstream. The 4.598 GeV raw RMS happens to agree with the measured 1.2--1.3%
cluster-energy resolution, but that agreement is not significant with ten
events and without the detector-response terms. Larger fixed-geometry samples
and face/edge scans are required before interpreting the energy dependence.

The validation runs must execute through Slurm rather than directly on an
ifarm login node. Normal compute-node runs used roughly 0.4 GB maximum resident
memory and 12--16 seconds for ten events. One run on `farm180207` anomalously
grew past an 8 GB allocation and was killed; an identical job excluding that
node completed normally, so that node is excluded from the current smoke-test
scripts. This infrastructure anomaly did not alter the validated ROOT output.

### Thousand-event centered samples

The two explicit-geometry centered samples were extended to 1,000 events with
the same seeds and crystal-522 aim point. Both jobs completed normally in less
than 75 seconds with approximately 0.4 GB peak resident memory. The larger
samples reveal a narrow contained peak plus a small, strongly non-Gaussian
low-energy tail that the ten-event tests could not characterize:

| Energy | Active-crystal entry | Same crystal 522 | Below 90% of incident energy | Full mean | Full RMS | Core mean (`Edep > 0.9 E`) | Core RMS |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 4.598 GeV | 989/1000 | 956/1000 | 23/1000 | 4.45031 GeV | 459.38 MeV | 4.50626 GeV | 45.45 MeV (1.009%) |
| 7.072 GeV | 993/1000 | 975/1000 | 9/1000 | 6.88781 GeV | 509.59 MeV | 6.93007 GeV | 66.18 MeV (0.955%) |

The unrestricted RMS values, 10.3% and 7.4%, are not useful calorimeter core
resolutions; they are dominated by rare upstream/non-entry events. The medians
are 4.51566 and 6.94633 GeV. The contained-core widths are approximately 1% at
both energies before a measured detector/electronics convolution. The low tail
must remain in efficiency and threshold studies rather than being discarded
from the final response model. Its treatment also has to avoid double counting
external radiation already represented in the SIMC event definition.

![Centered 1,000-event GEANT4 response](plots/nps_geant4_centered_1000.png)

The vector version is retained as
[`nps_geant4_centered_1000.pdf`](plots/nps_geant4_centered_1000.pdf), and the
reproducible ROOT plotting macro is
[`plot_nps_geant4_centered.C`](plot_nps_geant4_centered.C).

### Crystal face and gap scan

A correlated face scan used 200 events with identical seeds at five local NPS
coordinates for both calibration energies: crystal center
`(10.845,10.845) mm`, mid-face `(5,10.845) mm`, near-edge
`(2,10.845) mm`, edge-center `(0,10.845) mm`, and the four-crystal corner
`(0,0) mm`. All runs use the explicit 9.5 m geometry. The primary-entry and
low-tail results are:

| Energy | Metric | Center | Mid-face | Near edge | Edge center | Corner |
|---:|---|---:|---:|---:|---:|---:|
| 4.598 GeV | Primary enters active crystal | 100.0% | 94.5% | 92.5% | 87.5% | 81.5% |
| 4.598 GeV | Deposit below 90% | 0.5% | 8.0% | 12.0% | 19.0% | 32.5% |
| 7.072 GeV | Primary enters active crystal | 99.5% | 95.5% | 87.0% | 85.0% | 64.0% |
| 7.072 GeV | Deposit below 90% | 0.5% | 5.0% | 19.0% | 23.0% | 49.0% |

The surviving contained core also shifts downward and broadens toward the
gaps. From center to corner, its mean fraction changes from 0.9801 to 0.9664
at 4.598 GeV and from 0.9799 to 0.9580 at 7.072 GeV. Its RMS/mean increases
from 1.00% to 1.84% and from 1.10% to 1.99%, respectively. The higher-energy
corner entry is especially low because its straighter primary trajectory is
less likely to scatter out of the gap into active material. These results
demonstrate that crystal-level geometry and shower sharing must be retained;
a position-independent Gaussian response cannot reproduce the threshold tail.

![NPS GEANT4 face scan](plots/nps_geant4_face_scan_200.png)

The [vector PDF](plots/nps_geant4_face_scan_200.pdf),
[numerical CSV](nps_geant4_face_scan_200.csv), and
[ROOT analysis macro](analyze_nps_geant4_face_scan.C) are retained for
reproduction. The scan is a local single-cell study; a full active-face scan
must additionally include outer calorimeter boundaries and known dead-block
masks.

### SIMC event-stream adapter

The private `gep-external-primary` branch now has a strict reader for the
versioned 23-column SIMC hybrid CSV. Its `--simc-events` mode preserves each
SIMC event weight and post-target electron vector, uses explicit NPS angle and
distance, and replaces target-liquid and cell materials with vacuum only for
this mode. The historical DVCS and mono-electron paths remain available.

The first serialization/primary-transfer ten-event Slurm test completed as job
`13493645`. Its ROOT tree contains ten entries, and the recorded weights,
vertices, and electron momenta reproduce the first ten SIMC rows. Full command,
resource, and contract details are in
[`simc_geant4_interface_contract.md`](simc_geant4_interface_contract.md).

The next interception test found that SIMC Hall axes must be mapped into this
GEANT4 geometry as `(x,y,z) = (y,-x,z)`. After correction, paired 78-event job
`13494160` produced physical NPS entries and demonstrated migration in the
entry and low-energy-tail categories when the already-counted target/cell
material is deliberately restored. The production mode remains vacuumized.
See the dedicated
[`material-boundary closure`](simc_geant4_material_closure.md).

The first statistically useful current-geometry hybrid sample is documented in
[`gep_q11_hybrid_response.md`](gep_q11_hybrid_response.md). Slurm job
`13494409` transported all 10,923 fixed-seed SIMC events in 9 minutes 20
seconds. The raw weighted primary-entry fraction is 75.62%, while 98.69% make
at least 1 MeV of deposit. A sharp negative-horizontal low-response band is
visible despite the projected footprint lying mostly inside NPS. Audit the
inherited run-6151 sweeper/passive geometry before interpreting that entry
fraction as GEp-IV acceptance.
