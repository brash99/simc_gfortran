# Archived DVCS benchmark exploration

Archived on 25 October 2026.

These notes document the useful but superseded exploration based mainly on NPS/DVCS runs 1918, 2875, and 6151. They remain part of the audit trail: they established the SIMC-to-GEANT4 workflow, tested elastic closure, and clarified NPS response and occupancy. They are no longer the current proposal-rate baseline.

The central correction is conceptual. Several early estimates described backgrounds that survived analysis selections; they were subsequently discussed as though they were raw online trigger rates. Raw accidental rates must instead be formed from the measured hardware singles rates and the online coincidence width. The XEM2 HMS data show that an S1X AND S1Y primitive is of order 0.8--1 MHz after simple GEp luminosity scaling, so MHz-scale NPS singles would give kHz-to-tens-of-kHz accidentals, not a few hertz.

Current documents:

- [Proposal readiness summary](../../proposal_readiness_summary.md)
- [XEM2 HMS/NPS benchmark](../../xem2_hms_nps_benchmark.md)

The archived files have not been deleted or rewritten. Some relative links inside them therefore still refer to their former parent directory.

## Contents

- `findings.md`, `pac37_elastic_baseline.md`, `beam_time_yield_check.md`: early proposal and yield synthesis.
- `nps_calibration_current_survey.md`, `nps_hcana_trigger_audit.md`, `nps_run6151_firstlook.md`, `nps_trigger_rate_study.md`: NPS run selection and trigger studies.
- `run6151_simc_geant4_closure.md`, `prompt_inelastic_study.md`: DVCS elastic and background closure work.
- `gep_coincidence_trigger_budget.md`, `q11_correlated_trigger_projection.md`: superseded trigger-budget interpretations.
- `q11_smoke_test.md`: early Q2=11 workflow check.
