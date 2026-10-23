# GEp-IV Hall C reboot: beam-time and elastic-yield check

Checked: 2026-10-22.

## Scope

This note verifies the arithmetic behind Andrew Puckett's preliminary Q² = 11 event estimate and compares it with the normalization from the local fixed-seed SIMC smoke run. It uses the collaboration's PAC-day convention defined below. It does not validate detector efficiencies, the underlying cross-section model, or Andrew's unavailable exact input deck.

## Working convention

All continuing studies will use the reproducible local SIMC normalization as the working baseline:

- 41.481955 weighted events/mC;
- 3.111147 events/s at 75 μA; and
- 12,096,138 gross elastic events for 45 PAC days.

Andrew's 3.25 Hz and 12.636 million events are retained only as an external comparison. They are not inputs to projections unless his exact configuration is later reproduced.

The source is [message4.pdf](messages/message4.pdf), especially pp. 3-4. Andrew describes the estimates as tentative and requests that they remain within the core group.

## Andrew's displayed assumptions

The email states:

- 75 μA beam current;
- a 30 cm LH2 target;
- 45 PAC days;
- a SIMC elastic rate of 3.25 events/s; and
- approximately 12 million collected elastic events.

The embedded spreadsheet gives 12,636,000 events. Its calculation is exactly

    3.25 events/s × 45 days × 86,400 s/day = 12,636,000 events.

Thus the displayed estimate correctly treats every one of the 45 PAC days as 24 hours of beam on target at the quoted 3.25 Hz. Accelerator availability is not an experiment-side correction to a PAC-day request. The displayed estimate does not visibly apply separate factors for target boiling, DAQ live time, trigger efficiency, tracking efficiency, analysis cuts, or polarimeter reconstruction.

At 75 μA, the same exposure corresponds to

    0.075 mC/s × 45 × 86,400 s = 291,600 mC = 291.6 C.

The quoted rate is therefore equivalent to

    3.25 events/s / 0.075 mC/s = 43.333 events/mC.

Andrew's arithmetic is internally correct under these assumptions.

## Local SIMC working baseline and external comparison

The [fixed-seed smoke test](q11_smoke_test.md) used `EXPER%charge = 1.0` mC and reported an integrated weighted yield of 41.481955 counts. Interpreted according to SIMC's report as 41.481955 events/mC, this gives at 75 μA:

    41.481955 events/mC × 0.075 mC/s = 3.111147 events/s.

For 45 full 24-hour days:

    41.481955 events/mC × 291,600 mC = 12,096,138 events.

| Quantity | Local working baseline | Andrew's estimate | Difference (local - Andrew) |
| --- | ---: | ---: | ---: |
| Weighted yield | 41.482 events/mC | 43.333 events/mC | -4.27% |
| Rate at 75 μA | 3.111 Hz | 3.250 Hz | -4.27% |
| Events in 45 full days | 12,096,138 | 12,636,000 | -539,862 (-4.27%) |

At the local working rate and before experiment-side losses, 12 million events require 44.64 PAC days; Andrew's exact 12.636 million total requires 47.01 PAC days.

This agreement is encouraging but not a reproduction of Andrew's calculation. His exact input deck, generated ranges, target details, radiative settings, cuts, source commit, and run report remain unavailable. The local deck also emitted low-energy-loss warnings whose weighted contribution has not yet been quantified.

## PAC-day efficiency ledger

By convention, one PAC day is 24 hours of beam on target at the stated current. Accelerator availability is used later by the laboratory to convert approved PAC days into scheduled floor time; it does not reduce the physics yield assigned to a PAC day.

To estimate the usable experimental sample, introduce a combined experiment-side efficiency factor

    f_exp = target-density/boiling factor × DAQ live time
            × trigger efficiency × detector/tracking efficiency
            × elastic-selection efficiency × FPP efficiency.

Do not apply a factor already included in the quoted SIMC rate a second time. The resulting totals are:

| Combined experiment-side efficiency | Local working baseline | Andrew comparison |
| ---: | ---: | ---: |
| 100% | 12,096,138 | 12,636,000 |
| 90% | 10,886,524 | 11,372,400 |
| 80% | 9,676,910 | 10,108,800 |
| 70% | 8,467,297 | 8,845,200 |
| 60% | 7,257,683 | 7,581,600 |
| 50% | 6,048,069 | 6,318,000 |

The email's displayed 12.636 million corresponds to the 100% experiment-side-efficiency row. It is the correct gross elastic yield for 45 PAC days under the quoted 3.25 Hz rate, but it is not yet a verified final-analysis sample.

## Comparison with the quoted SBS sample

Andrew quotes an SBS best-case recovered sample of approximately 9.3 million events. If every event carried equal statistical information:

| Hall C basis | Hall C / SBS event ratio | Hall C / SBS statistical-error ratio |
| --- | ---: | ---: |
| Local working baseline: 12.096 million | 1.301 | 0.877 |
| Andrew comparison: 12.636 million | 1.359 | 0.858 |

The local working baseline implies approximately 12.3% smaller statistical uncertainty from event counts alone, not a factor-of-two improvement. A defensible precision comparison still requires the acceptance-weighted FPP analyzing power, efficiency, spin transport, backgrounds, and selection losses for both experiments.

## Information still needed

To convert this arithmetic check into a verified beam-time projection, obtain:

1. Andrew's exact SIMC deck, checkout commit, report, and definition of the 3.25 Hz rate.
2. Separate target-density/boiling, DAQ live-time, trigger, detector/tracking, elastic-selection, and FPP reconstruction factors.
3. A weighted Monte Carlo uncertainty on the simulated rate.
4. The event-level FPP coefficient of merit needed to translate elastic yield into form-factor precision.
