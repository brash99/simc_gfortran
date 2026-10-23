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

The public hcana/NPSlib code paths that enable this recovery are mapped in [`nps_hcana_trigger_audit.md`](nps_hcana_trigger_audit.md).
