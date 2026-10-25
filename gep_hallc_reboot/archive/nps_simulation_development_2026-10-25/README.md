# Archived NPS and SIMC-GEANT4 development notes

Archived on 25 October 2026.

These files preserve the detailed development record for the NPS response, occupancy, pi0 background, radiative convention, and SIMC-to-GEANT4 interface. They contain valid technical results, but many were written sequentially and their concluding “next step” sections were overtaken by later work. They should not be read as the current proposal work plan or as the current raw-trigger-rate baseline.

The current synthesis is maintained in:

- [Proposal-readiness summary](../../proposal_readiness_summary.md)
- [XEM2 HMS/NPS benchmark](../../xem2_hms_nps_benchmark.md)
- [NPS validation status](../../nps_validation_status.md)

## What remains valid

- The SIMC-to-GEANT4 external-event interface, coordinate transformation, and material-boundary tests.
- The crystal-level NPS shower, containment, and local-sum studies, within their documented assumptions.
- The data-based occupancy, reconstructed-cluster, decoded-pulse, and sparsification observations from the NPS/DVCS runs.
- The hard-bremsstrahlung pi0 generator, cross-section model, and early elastic-selection studies, with their stated model uncertainties.
- The radiative convention and its documented sensitivity tests.

## What has been superseded

- The 8.43-million-event GEp projection in `gep_q11_hybrid_response.md` has been superseded by the XEM2-data-anchored projection of approximately 12.0 million selected elastic events in 45 PAC days.
- The few-hertz accidental-background calculations in the development chain do not predict raw online accidental coincidences. Current raw estimates use measured HMS hardware primitives and `R_NPS x R_HMS x coincidence_width`.
- Statements that a larger hybrid sample, pi0 generator, response convolution, or common elastic/pi0 scan still needs to be created are historical; those milestones were subsequently completed.
- The 3 m NPS distance is a current design standard, not an optimized final location.

## Contents

- `nps_geant4_survey.md`, `simc_geant4_interface_contract.md`, `simc_geant4_material_closure.md`: simulation discovery, interface, and closure record.
- `gep_q11_hybrid_response.md`, `nps_response_scan.md`: early response and yield studies.
- `nps_cluster_overlay.md`, `nps_edtm_pulse_overlay.md`, `nps_occupancy_projection.md`, `nps_raw_block_pulse_study.md`, `nps_secondary_cluster_study.md`: detector occupancy and pileup studies.
- `pi0_background_channel_audit.md`, `pi0_cross_section_model.md`, `pi0_generator.md`, `pi0_elastic_cut_scan.md`: correlated pi0 background development.
- `radiative_convention.md`: elastic radiative-model convention.

The archived notes are frozen in their original form. Their internal relative links refer to their former top-level location and may therefore require prefix adjustment when followed manually.
