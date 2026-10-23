# GEp-IV Hall C reboot: fixed-seed Q² ≈ 11 SIMC smoke test

Run date: 2026-10-22.

## Purpose and status

This is an execution and geometric-acceptance smoke test for the exploratory GEp-IV Hall C reboot setting. It is not a production-normalization configuration or a validation of the physics model, detector response, or projected experimental yield.

The input deck is [gep_reboot_q11_smoke.inp](../infiles/gep_reboot_q11_smoke.inp).

## Local build

The core executable was built locally on Apple Silicon macOS 27 with GNU Fortran 14.2.0 after a Darwin-only Makefile portability repair. The compiler now receives the active SDK from `xcrun`, the host deployment version from `sw_vers`, and the module target uses the same Fortran flags as the other sources.

A serial `make all` completed and linked a native arm64 `simc` executable. The linker still reports the stale search path `/apps/cernlib/i386_fc8/2005/lib`, but this does not prevent the core executable from linking.

## Configuration

The smoke deck uses:

- 10.7 GeV beam energy;
- 4.838 GeV/c electrons at 26.651°;
- 6.735 GeV/c protons at 18.797°;
- NPS at 3 m (`electron_arm=8`);
- HMS for protons (`hadron_arm=1`);
- a nominal 30 cm LH2 target;
- fixed RANLUX seed 110011;
- radiative and energy-loss effects;
- 1,000,000 fixed trials (`ngen=-1000000`); and
- diagnostic text output only (`Nntu=0`).

SIMC reports central Q² = 10.9947 GeV² for this configuration.

## Results

The run completed with:

| Counter | Value |
| --- | ---: |
| Trials | 1,000,000 |
| Contributing events | 11,013 |
| HMS successes | 11,013 |
| NPS trials after HMS | 11,013 |
| NPS horizontal extent rejections | 0 |
| NPS vertical extent rejections | 0 |
| NPS successes | 11,013 |

All trajectories that survived HMS also passed the modeled NPS rectangle. With zero observed failures, the simple rule-of-three upper bound on rectangular-aperture loss is approximately

    3 / 11,013 = 0.027%

at 95% confidence for this generated sample.

An identical fixed-seed rerun produced byte-for-byte identical `.gen`, `.geni`, and initial-random-state files. Their SHA-256 values were:

| File | SHA-256 |
| --- | --- |
| `gep_reboot_q11_smoke.gen` | `3719a8a52468d893c2008b96811f24c7518ae90b546a21a9aea566d979ce9352` |
| `gep_reboot_q11_smoke.geni` | `b1893472bb2b354f394ada6800df91e57f7cd8970317c7be8ff98a9b9528af42` |
| `gep_reboot_q11_smoke_start_random_state.dat` | `e44a761432b33dcc7a0a1249225dc950c1342dd491040815ffaf9b189c84fd05` |

## Limitations and follow-up

This result only addresses the current rectangular interception cut for trajectories that first survive HMS. It does not model shower containment, energy or position response, pileup, thresholds, backgrounds, fiducial margins, or detector inefficiency. It also depends on provisional generation, target, radiative, and acceptance settings.

The run emitted repeated `Eloss>Total KE; forcing Eloss=KE` messages before suppressing further occurrences. Their frequency, event phase space, and weight contribution should be quantified before treating the weighted yield as production evidence.

The console "successes" count in fixed-trial mode tracks generated attempts; it must not be confused with the 11,013 contributing events reported in the run diagnostics.
