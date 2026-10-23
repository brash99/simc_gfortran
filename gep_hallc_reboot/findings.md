# GEp-IV Hall C reboot: kinematics, historical proposal, and simulation review

Reviewed: 2026-10-22.

## Scope and sources

Review of Andrew Puckett's October 21-22, 2026 email discussion, including the embedded spreadsheet and calorimeter footprint, in [message4.pdf](messages/message4.pdf). All six pages were inspected. Messages 1-3 are retained in [messages/](messages/) but were not reviewed for these findings.

Also reviewed the full 45-page [PAC37 GEp-IV proposal (2011)](proposals/GEp-IV_PR12-09-001_PAC37_2011.pdf), including its figures, Tables 1-4 and reference list. Page references below use the printed PDF page numbers, which match the PDF page sequence. The [original PAC34 submission (2009)](proposals/GEp-IV_PR12-09-001_PAC34_2009.pdf) is retained for historical context; it has not received the same detailed review.

These are preliminary internal study notes. Andrew explicitly describes his estimates as tentative and requests that they remain within the core group. His simulation input deck, full run report and event files were not available in the reviewed PDF. No simulation was rerun for this review.

## Overall assessment

The selected elastic kinematics are internally consistent and provide a sensible starting configuration for an HMS proton polarimeter plus NPS electron calorimeter study at Q² = 11 GeV². They justify further investigation. The 2011 proposal provides a concrete HMS+FPP baseline, including SIMC elastic/background estimates and separate polarimeter modeling. It also considered a compact PbWO4 calorimeter, making its connection to the present NPS idea stronger than the BigCal baseline alone suggests. Geometric acceptance losses, operation at the proposed luminosity, and the acceptance-weighted polarization figure of merit remain unresolved for NPS.

## Local smoke test

The core executable now builds locally on Apple Silicon macOS, and a reproducible one-million-trial smoke run at the preliminary Q² ≈ 11 setting found 11,013 contributing events with no modeled NPS rectangular-extent rejection. See the dedicated [fixed-seed smoke-test report](q11_smoke_test.md) for the build changes, configuration, counters, reproducibility hashes, limitations, and required follow-up.

A separate [PAC37 elastic-baseline reconstruction](pac37_elastic_baseline.md) gives 41.936699 events/mC with the current radiated implementation and reconstructed 2011 HMS+BigCal settings. The raw Table 1 comparison is 30.1% low, but the difference is 9.6% relative to the proposal's 3σ-cut yield and 18.4% relative to its alternative 6σ-cut yield. BigCal's rectangular extent rejects only 7 of 9,782 calorimeter trials, so it is not the dominant source of the discrepancy. The original deck, cut convention, radiation configuration, and code revision are still needed for an exact reproduction.

The current radiative convention is now defined in [radiative_convention.md](radiative_convention.md). The working settings retain SIMC radiation, the repository-standard peaked/off-shell correction configuration, and all three radiation tails. At the 10.7 GeV working point, disabling radiation raises the fixed-seed yield by 21.34%, while the older Schwinger internal prescription raises it by 3.38%. The quoted 41.481955 events/mC is specifically a radiated, transport-accepted, configured-SPedge yield before realistic reconstructed elastic-selection and detector efficiencies.

A [π⁰ background channel audit](pi0_background_channel_audit.md) establishes that the present checkout lacks the normalized hard-bremsstrahlung real-photon generator used for PAC37; the available π⁰ examples are different electroproduction topologies, and the proton-detected `doing_delta` cross section is a placeholder. Independent endpoint kinematics nevertheless show essentially exact overlap with the current elastic central setting: a 10.7 GeV photon produces a 6.7286 GeV/c proton at 18.797° and a 4.8446 GeV π⁰ at 26.596°. Absolute contamination remains unverified until a real-photon flux and full photoproduction model are supplied.

## Selected kinematics

The spreadsheet on page 4 contains the main measurement and a possible commissioning setting:

| Quantity | Main measurement | Possible commissioning |
| --- | ---: | ---: |
| Beam energy | 10.7 GeV | 8.6 GeV |
| Q² | 11 GeV² | 8 GeV² |
| Energy transfer, ν | 5.862 GeV | 4.263 GeV |
| Scattered-electron energy | 4.838 GeV | 4.337 GeV |
| Electron angle, NPS | 26.651° | 26.779° |
| Proton momentum, HMS | 6.735 GeV/c | 5.116 GeV/c |
| Proton angle, HMS | 18.797° | 22.453° |
| Virtual-photon polarization, ε | 0.6837 | 0.7295 |

Independent stationary-proton elastic calculations, using the spreadsheet proton mass M = 0.93827 GeV and neglecting the electron mass, reproduce these values:

- ν = Q²/(2M), E′ = E - ν.
- Q² = 4EE′ sin²(θe/2).
- pp = sqrt[ν(ν + 2M)].
- θp = atan2[E′ sin θe, E - E′ cos θe].
- τ = Q²/(4M²), ε = [1 + 2(1 + τ) tan²(θe/2)]⁻¹.

The commissioning setting changes the electron angle by only about 0.13°, retaining similar NPS geometry while reducing proton momentum substantially. Its HMS spin transport would differ from that at the main setting.

## NPS geometry and interpretation of the footprint

Andrew assumes an NPS distance of 3 m, with the main setting at approximately 26.7°. He estimates an electron envelope of roughly ±30 cm vertically and ±25 cm horizontally, and suggests that the calorimeter might be moved somewhat farther away.

Inspection of this checkout's [calo/mc_calo.f](../calo/mc_calo.f) establishes that:

- The model uses NPS half-width 30.75 cm and half-height 36.9 cm, corresponding to 30 × 36 blocks of 2.05 cm pitch.
- It projects the particle to `drift_to_cal` and rejects hits outside that rectangle.
- Transport x is vertical and y horizontal. Thus the displayed `ssxfp:ssyfp` plot has vertical position on its vertical axis and horizontal position on its horizontal axis for the electron-calorimeter configuration.
- [results_write.f](../results_write.f) returns immediately for unsuccessful events. The plotted accepted-event footprint therefore cannot reveal the full envelope of events rejected by the calorimeter.

Consequently, the footprint is consistent with the modeled NPS dimensions, but does not by itself demonstrate comfortable coverage of the full HMS-correlated electron acceptance. The apparent horizontal edge near +30 cm deserves scrutiny.

The next useful check is a comparison against an enlarged calorimeter aperture, or equivalent examination of rejected trajectories and calorimeter rejection counters. Report acceptance loss and its dependence on vertex position, proton momentum and angles, and Q². Include fiducial margins for shower containment rather than assuming all geometric edge hits are usable.

The relevant `Calo Trials`, horizontal/vertical rejection, and success counters are written by [simc.f](../simc.f) into its generated diagnostic output. Andrew's exact checkout and input settings still need confirmation before attributing this checkout's behavior to his run.

## Detector resolution and high-rate operation

Andrew's assumed spatial resolution gives 5 mm / 3 m = 1.67 mrad. The arithmetic is sound, but it is an assumed detector performance, not an established result of the presented simulation.

The active calorimeter routine performs projection and rectangular interception cuts. Its reconstruction/smearing calls are commented out. This model does not establish realistic calorimeter energy resolution, position resolution, trigger efficiency, pileup behavior, background occupancy or shower containment.

Andrew identifies the principal experimental risks as:

1. HMS and coincidence trigger rates.
2. NPS background rates and pileup at 70-75 μA on a 30 cm LH2 target.
3. HMS drift-chamber tracking performance at high rates.
4. Fitting the FPP and an effective trigger into the HMS detector stack without degrading tracking angular resolution or biasing the FPP angular distribution.

He discusses retaining S2 if possible, avoiding material upstream of the drift chambers, and avoiding reliance on an HMS calorimeter trigger that could bias secondary-scattering acceptance. These are design questions, not resolved capabilities. No independent claim about current Hall C hardware or DAQ performance was established here.

## Yield estimate and comparison with SBS

The reproducible local SIMC run is the working yield baseline: 41.481955 events/mC, corresponding to 3.111147 events/s at 75 μA and 12,096,138 gross elastic events for 45 PAC days.

A dedicated [beam-time and elastic-yield check](beam_time_yield_check.md) reconstructs the exposure ledger. Andrew's 3.25 events/s and 12,636,000-event estimate is retained only as an external comparison pending reproduction of his exact setup; it is not used as an input to continuing projections.

The conversion from the local rate to a 45-PAC-day total is arithmetic under the standard convention that a PAC day contains 24 hours of beam on target at the stated current. Accelerator availability is not applied to this physics yield; it is used later to convert approved PAC days into scheduled floor time. Target boiling/density, DAQ live time, tracking, trigger, analysis-cut and polarimeter efficiencies remain to be applied explicitly before interpreting the number as a usable experimental sample. The local simulated rate also needs a weighted Monte Carlo uncertainty before it becomes a production projection.

The email quotes an SBS best-case recovered sample of approximately 9.3 million events. This is Andrew's preliminary comparison, not an independently verified SBS result. For equal information per event:

    δR_HallC / δR_SBS = sqrt(9.3 / 12.096) ≈ 0.877

Thus the local working yield and quoted SBS count alone imply about 12% smaller statistical uncertainty. A factor-of-two precision improvement would require four times the effective statistical information, through a different recovered SBS sample, improved polarimetry, or other gains. It does not follow from these two event totals alone.

## Spin precession and form-factor sensitivity

Using a simple central-ray estimate χ ≈ κp γp θbend, with κp ≈ 1.792847 and an assumed 25° HMS bend, gives χ ≈ 324.8° at Q² = 11 GeV². This supports Andrew's approximately 325° estimate. The signed sine is approximately -0.576; his quoted 0.58 is its magnitude. Sign conventions matter in a polarization extraction.

A single central precession angle is insufficient to predict the final ratio uncertainty. Andrew emphasizes the broad spin-precession acceptance of a focusing spectrometer. That argument must be evaluated using event-by-event spin transport and a polarization likelihood or covariance calculation, including polarimeter acceptance and analyzing power.

The spreadsheet assumes GE/GM ≈ -0.0009 at Q² = 11, with PT ≈ +0.00034 and PL ≈ 0.730. At Q² = 8 it assumes GE/GM ≈ 0.06475. These are form-factor model assumptions, not consequences of elastic kinematics. The spreadsheet screenshot alone does not fully specify or validate the extrapolation that produces them.

In the Born approximation:

    GE/GM = -(PT/PL) [(E + E′)/(2M)] tan(θe/2).

The kinematic factor in square brackets times tan(θe/2) is approximately 1.961 at the main setting, and 1.641 at the commissioning setting. See the [PAC37 proposal](proposals/GEp-IV_PR12-09-001_PAC37_2011.pdf), Section 6, Eq. (9), pp. 21-22. The same relation appears in the previously consulted [Hall A GEp proposal](https://www.ge.infn.it/jlab12/files/PR12-07-109-Ratio.pdf), Section 5.1.

Near a possible zero crossing, quantify absolute uncertainty on GE/GM or μp GE/GM, rather than relative uncertainty on a value close to zero. Study sensitivity for a range of plausible ratios; do not optimize exclusively for the spreadsheet's near-zero assumption.

## Detailed review of the 2011 PAC37 proposal

### Reading map and historical scope

| Pages | Content | Relevance to the reboot |
| --- | --- | --- |
| 1-3 | Collaboration and abstract | HMS, existing FPP and BigCal; reach to Q² = 11 GeV². |
| 4-5 | Rationale and coordination with GEp-V | Independent systematics and overlapping measurements were central to the case. |
| 6-20 and beginning of 21 | Experimental background and theory | Historical motivation: model discrimination, scaling, GPDs, transverse densities, lattice and Dyson-Schwinger approaches. These are 2011-era assessments, not a review of current theory/data. |
| 21-25 | Recoil polarization, FPP, spin precession | Method, analyzing-power extrapolation, GEANT3 FPP response, COSY spin transport and optics uncertainties. |
| 26-33 | HMS operation, triggers, elastic selection and backgrounds | Detector-stack tradeoffs; SIMC signal/background studies; Table 1. |
| 33-39 | BigCal acceptance, radiation history and HyCal alternative | Figure 25 acceptance maps; separate GEANT energy-response and radiation studies; compact crystal option. |
| 39-41 | Chosen measurements, rates, precision and beam time | Tables 3-4 and Figure 28. |
| 42-45 | References | Starting points for recovering prior models and analysis methods. |

The updated proposal is the correct historical starting point for proton-in-HMS studies. It states that HMS had approximately twice the solid angle of SHMS, and emphasizes existing HMS spin-transport experience (p. 39). Its schedule and claims about available hardware are historical assumptions; they do not establish the current condition or availability of the FPP, chambers, electronics or support systems.

The physics case was broader than seeking a zero crossing: distinguish diverging form-factor predictions, constrain the evolution of F2/F1, and provide inputs to nucleon-structure descriptions. A fresh proposal must reassess this motivation against subsequent measurements and modern theory rather than reuse the 2011 figures as current evidence.

### Original three-point program and assumptions

Table 3 (p. 40) used 75 μA, 85% beam polarization, a 30 cm LH2 target, and 7 msr HMS solid angle. Section 7.1 describes the acceptance as approximately 6.7 msr and quotes approximately 1 mrad angular resolution, 0.1% momentum resolution, and a maximum accepted HMS momentum of 7.4 GeV/c. These are proposal values, not new hardware measurements.

| Quantity | Control | GEp-III overlap | Highest-Q² measurement |
| --- | ---: | ---: | ---: |
| Q² (GeV²) | 6.0 | 8.5 | 11.0 |
| Beam energy (GeV) | 8.8 | 11.0 | 11.0 |
| Electron angle (degrees), as tabulated | 20.1 | 19.9 | 25.5 |
| Electron energy (GeV) | 5.60 | 6.47 | 5.14 |
| Proton angle (degrees), as tabulated | 28.5 | 24.4 | 19.2 |
| Proton momentum (GeV/c) | 4.03 | 5.39 | 6.74 |
| ε | 0.85 | 0.83 | 0.70 |
| Central precession χ (degrees) | 197.6 | 261.2 | 324.8 |
| Required electron solid angle (msr) | 4.1 | 5.3 | 12.7 |
| dσ/dΩe (cm²/sr), as tabulated | 3.2 × 10^-35 | 6.5 × 10^-36 | 0.9 × 10^-36 |
| BigCal distance (m), Figure 25/Table 2 | 15 | 15 | 10 |

Independent elastic calculations reproduce the main energy/angle pattern, but give θp = 24.133° for Q² = 8.5 GeV² and E = 11 GeV, rather than Table 3's 24.4°. This difference is too large to be ordinary rounding to one decimal place. Treat it as a likely table inconsistency and recalculate correlated settings when preparing decks; do not silently copy every table entry.

The 6 GeV² point was a control measurement; 8.5 GeV² overlapped the highest GEp-III point; 11 GeV² extended the reach. The old proposal allocated substantial control-point physics time, whereas Andrew's 8 GeV² setting is labeled as possible commissioning and has no exposure or precision allocation in the email. Those are different roles.

### What the prior simulations actually covered

The proposal used several complementary models, not a single all-inclusive SIMC calculation:

| Component | Evidence in proposal | What it does not establish for today's study |
| --- | --- | --- |
| SIMC elastic coincidence events | Rates and cuts in Tables 1/4; geometric footprint in Figure 25 | Current NPS response, rates or exact historical deck settings. |
| SIMC bremsstrahlung-induced π0 photoproduction | Figures 21-23, Table 1; authors report benchmarking against GEp-III contamination estimates (p. 30) | Uncertainty-free extrapolation to the new beam energy, detector response or selection. |
| GEANT3 FPP response | Differential secondary-scattering fraction entering COM (p. 22) | A fully specified current FPP reconstruction/efficiency model or an implementation in this checkout. |
| Empirical analyzing power | GEp-III and calibration data; extrapolation with proton momentum (pp. 22-23, Figure 17) | Measured analyzing power for every proposed analyzer/reconstruction configuration. |
| COSY spin transport | Full matrix and comparisons to GEp-2γ data (pp. 23-25, Figure 19) | That particle-trajectory transport alone in SIMC supplies a usable spin matrix. |
| GEANT calorimeter response/radiation | Figure 26 and soft-photon flux estimates tied to BigCal operating history (pp. 36-38) | NPS-specific pileup, radiation-recovery or energy-response predictions. |

The full proposal does not supply runnable SIMC input decks, a source commit, seeds, event samples, the numerical position/momentum resolution model, or complete cut definitions in physical units. It establishes that the studies existed and gives numerical reproduction targets. Recovering their artifacts remains necessary.

### Elastic selection, radiative losses and π0 background

Sections 7.1 and Figures 20-23 (pp. 28-32) identify hard-bremsstrahlung-induced γp → π0p as the principal inelastic background. Near endpoint its proton kinematics overlap elastic ep within HMS resolution. The electron/photon calorimeter hit must be compared with the position predicted from HMS proton kinematics under the elastic hypothesis, supplemented by the proton angle-momentum correlation and possibly calorimeter energy.

The proposal reports a 30 cm hydrogen target as roughly a 3.5% radiator, and uses an empirical photoproduction model with full s and center-of-mass-angle dependence. Its fixed-angle s^-7 scaling argument motivates improved conditions at higher beam energy; it is not a universal multiplicative correction for arbitrary new kinematics. The authors explicitly acknowledge uncertainty when extrapolating the π0 model beyond fitted data (p. 32).

The cuts are on calorimeter position residuals and pp - pp(θp). They are elasticity/correlation cuts, not the rectangular active-area cut used to define the accepted footprint. Distinguish these two meanings of calorimeter acceptance/cuts in subsequent work. Also note that Figure 20 labels horizontal residual as Δx and vertical residual as Δy; that naming differs from SIMC transport x/y. Define axes explicitly when reproducing plots.

Table 1 (p. 32), transcribed and visually verified:

| Quantity | Q² = 6.0 | Q² = 8.5 | Q² = 11.0 |
| --- | ---: | ---: | ---: |
| SIMC yield per mC, 30 cm target | 761 | 184 | 60 |
| π0 background fraction, 3σ cuts | 0.02% | 0.03% | 0.15% |
| π0 background fraction, 6σ cuts | 0.11% | 0.12% | 0.61% |
| π0 background fraction, no elasticity cuts | 0.52% | 2.16% | 10.1% |
| Elastic retention, 3σ cuts | 79.5% | 78.1% | 77.3% |
| Elastic retention, 6σ cuts | 87.1% | 86.3% | 85.7% |

Tighter cuts lose elastic radiative-tail events, not merely Gaussian measurement tails. Thus the approximately 23% loss at 3σ is not inconsistent with ordinary Gaussian expectations; the simulated distributions include radiation. At Q² = 11, relaxing 3σ to 6σ retains more elastic events while increasing the estimated contamination from 0.15% to 0.61%. Neither number should be adopted for NPS without reproducing the resolution, energy threshold, radiative and photoproduction assumptions.

The proposal separately discusses aluminum endcap background and dummy-target measurements (pp. 32-33). It quotes 4-6 mm longitudinal vertex resolution and anticipates no target-length cut; an optional end-region cut could reduce useful length by about 10%. Include dummy-target and vertex selections explicitly in new yield estimates.

### Rates, efficiency accounting, and the exposure convention

Table 1 is in events per mC, not events per second. At 75 μA the charge rate is 0.075 mC/s. Therefore its yields correspond to:

| Q² (GeV²) | Converted Table 1 yield (Hz) | Conditional example: ×6σ retention ×0.8 (Hz) |
| --- | ---: | ---: |
| 6.0 | 57.075 | 39.770 |
| 8.5 | 13.800 | 9.528 |
| 11.0 | 4.500 | 3.085 |

The last column is our arithmetic illustration, assuming Table 1's rate precedes those elasticity cuts and the 0.8 allowance is independent. It is not a separately reported proposal result. The text rounds the signal rates to approximately 60/14/5 Hz (p. 28). Table 4 states that its raw SIMC coincidence rates already have kinematic cuts and were multiplied by an additional 0.8 for deadtime, target boiling and other losses. Do not apply a cut efficiency twice when reconstructing that projection.

Andrew's 3.25 Hz corresponds to 43.33 events/mC. It is of the same order as the historical rate after plausible losses, but agreement at that level does not establish identical normalization, cuts or acceptance. The 10.7 versus 11 GeV beam energies and NPS versus BigCal geometry must be disentangled first.

The BigCal radiation-damage discussion explicitly assumes 50% running efficiency and 3.25 C/day at 75 μA (p. 38). That historical operational assumption should not redefine a PAC day or be applied to the current PAC-day physics yield. Table 4 also gives a 0.8 loss factor. Do not automatically apply both factors: identify which experiment-side losses, if any, are already represented by the 0.8 factor. Keep a ledger in charge, PAC-day seconds, and experiment-side efficiency factors.

### FPP design and projected precision

Section 6.1 (pp. 22-23) specifies two CH2 analyzers, each 55 cm thick, with two downstream FPP drift chambers per analyzer. The HMS chambers define the incident trajectory. The proposal planned one additional wire plane per FPP chamber, four in total, to resolve track ambiguities and recover roughly 10% efficiency. This was a proposed upgrade, not evidence that it was built or is available now.

It defines the polarimeter coefficient of merit as an angle-integrated secondary-scattering efficiency weighted by Ay². GEANT3 supplies the scattering fraction, while empirical analyzing-power data provide the response. The maximum analyzing power occurs near secondary transverse momentum 0.3 GeV/c and decreases with incident proton momentum; Figure 17 displays the fit Ay_max = 0.029 + 0.34/p (p in GeV/c). At 6.735 GeV/c this expression gives approximately 0.0795, a maximum, not an acceptance-averaged analyzing power. Use the full angular dependence and selection when building a precision model.

Table 4 (p. 40) gives:

| Q² (GeV²) | COM | Quoted absolute statistical uncertainty | Days |
| --- | ---: | ---: | ---: |
| 6.0 | 3.9 × 10^-3 | 0.04 | 10 |
| 8.5 | 1.5 × 10^-3 | 0.10 | 20 |
| 11.0 | 1.1 × 10^-3 | 0.12 | 65 |

The full request is 98 PAC days: 95 measurement days plus 3 for commissioning, optics and calibrations. The main point alone had 65 days, not 45.

Important notation issue: Table 4 labels its uncertainty Δ(GEp/GMp) with a superscript asterisk, but Figure 28 plots μp GEp/GMp and visually has errors approximately matching the tabulated 0.04/0.10/0.12 on that normalized axis. No explanatory asterisk definition was found on the table page. This suggests an omitted normalization factor in the label, but is not enough to silently resolve it. Preserve the quoted numbers, flag the ambiguity, and recover the original error calculation before comparing precision. A normalized-ratio error and an unnormalized-ratio error differ by μp ≈ 2.793.

Removing one analyzer to make room for a trigger detector, as discussed in Andrew's email, changes this historical FPP baseline. Recompute efficiency, analyzing power, acceptance and reconstruction jointly; the old two-analyzer COM cannot be assigned unchanged to a one-analyzer design.

### Spin transport, central precession, and systematic control

Sections 6.2/7.1 (pp. 23-26) support Andrew's argument that the central sin χ alone is an inadequate precision predictor. The proposal states that acceptance-dependent precession reduces the severity of a central-ray near-zero sin χ, while quadrupole-induced non-dispersive precession mixes Pt and Pl and cannot generally be neglected. Figure 19 demonstrates that using COSY removes strong reconstructed-ratio dependence on target slopes seen with a dipole-only model.

The historical geometric approximation includes:

    χφ = γ κp (φfp - φtar)
    Stt = cos χφ; Stl = sin χφ
    Snt = sin χφ sin χθ; Snl = -cos χφ sin χθ

These equations are useful checks against full COSY transport, not replacements for a calibrated optical model. Nonuniform event weights mean precession terms do not necessarily cancel after averaging.

For an uncertainty in non-dispersive bending of no more than 0.5 mrad, the proposal estimates ΔR = 0.015, 0.024 and 0.035 at Q² = 6, 8.5 and 11, respectively (p. 25). R in the surrounding discussion/plots is the normalized ratio μp GEp/GMp; retain the notation caution above when assembling the error budget. Those estimates concern the dominant non-dispersive spin-transport contribution, not a demonstrated total systematic uncertainty for a rebuilt apparatus.

The original study judged Q² = 11 a compromise between reach, falling rate and increasingly unfavorable central precession; it noted that projected uncertainty varied slowly over 10.5-11.5 GeV² (p. 26). This motivates a narrow kinematic scan around 11, not assuming the central-angle optimum fixes the best experimental point. Dedicated optics/alignment data remain essential.

### Trigger and detector-stack tradeoffs already identified in 2011

The old design faced the same tension raised in Andrew's email (pp. 26-28, 32):

- S1-only triggers avoid extra material upstream of tracking but may produce too many accidentals.
- A 1 cm S0 scintillator upstream of the HMS chambers had added about 1 mrad multiple scattering in GEp-III. The proposal reports angular-resolution degradation factors of about two non-dispersively and three dispersively, with less effect on momentum resolution.
- An alternative was a smaller scintillator between the two HMS chambers, near the optical focal point, to limit the impact on reconstructed angles.
- Better triggering can improve recorded yield enough to outweigh poorer angular resolution, but this trade must be evaluated with elastic selection and backgrounds together.

The historical S1 rate range, scaled to 75 μA and 30 cm LH2, was 20-100 kHz. Its example used 30 kHz HMS singles, 300 kHz calorimeter singles and a ±30 ns coincidence window, giving 30,000 × 300,000 × 60 ns = 540 Hz accidentals. It quoted a practical DAQ range of 1-2 kHz and an optimistic 5 kHz ceiling. These are 2011 assumptions and must not be presented as current Hall C limits.

Calorimeter thresholds were approximately half the elastic electron energy: 2.8, 3.2 and 2.5 GeV at the three settings. The text treated 50% of elastic energy as a safe threshold limit for efficiency. Today's threshold/energy-resolution tradeoff must be measured or modeled for NPS under realistic occupancy; the old estimates are starting scenarios only. Random-coincidence rejection offline does not recover DAQ bandwidth lost to recording accidentals.

### BigCal acceptance, radiation history, and the compact-crystal alternative

BigCal's frontal area was 2.63 m², assembled from 32 × 32 blocks of 3.8 cm pitch below 30 × 24 blocks of 4.0 cm pitch (p. 33). The proposal quotes approximately 8 mm position resolution from GEp-III. Figure 25 (p. 35) shows simulated footprints with BigCal at 10 m for Q² = 11 and 15 m for the two lower points. Tables 2 and 3 support that 10 m highest-point baseline, although nearby prose on p. 40 says distances of at least 11 m; use the figure/table geometry when reconstructing it and record the text inconsistency.

For the main point, the plotted electron footprint is roughly ±60 cm horizontally and ±90 cm vertically at 10 m. Scaling only its angular part to 3 m gives approximately ±18 cm and ±27 cm, qualitatively supporting the plausibility of a compact calorimeter. This is a visual estimate, not an acceptance-efficiency calculation: the extended 30 cm target, central settings, offsets, and finite detector edges prevent exact uniform scaling.

The NPS rectangle in this checkout is 61.5 × 73.8 cm. Its small-angle frontal-area estimate at 3 m is approximately 50.4 msr, compared with 26.3 msr for 2.63 m² BigCal at 10 m and Table 3's required 12.7 msr electron acceptance at the old main point. A larger total solid angle does not guarantee coverage of the correlated footprint: its shape, center, vertex dependence, shower margin and accepted-event clipping matter. NPS's height/width ratio is 1.2, versus about 1.68 for BigCal; investigate both axes separately.

Likewise, smaller blocks do not alone establish improved angular resolution. The historical 8 mm at 10 m implies about 0.8 mrad from hit position alone, while Andrew's assumed 5 mm at 3 m implies about 1.67 mrad. Include the target-vertex uncertainty and actual NPS response in the comparison. This does not contradict Andrew's comparison to SBS, which is a different reference geometry.

BigCal radiation studies (pp. 33-39, Table 2, Figures 26-27) used operating history plus GEANT flux estimates. The main setting was assigned a 1.4%/C gain-loss coefficient, approximately 4.6% gain loss per day at 75 μA and 50% running efficiency. Proposed recovery used an improved UV system. BigCal's late-run energy resolution had degraded to approximately 24%/sqrt(E). These specific gain/recovery numbers belong to lead glass and cannot be transferred to NPS crystals.

Crucially, Section 7.2.2 (p. 39) already considered the PbWO4 section of HyCal as an alternative: approximately 4.5 m distance, 23° angle, 2 mm position resolution, and about 0.4 mrad angular resolution, intended to cover all three settings without moving. It cited greater radiation hardness and projected about 8% gain loss across the experiment. This is a direct conceptual precedent for a compact crystal electron detector. It does not verify NPS dimensions, mechanical placement, high-rate capability or radiation behavior at 3 m; re-evaluate those for the actual apparatus.

## What changes between the historical proposal and Andrew's setting

| Item | PAC37 main point | Current exploratory setting | Consequence |
| --- | --- | --- | --- |
| Q² | 11 GeV² | 11 GeV² | Same central elastic proton energy and momentum. |
| Beam energy | 11.0 GeV | 10.7 GeV | Electron energy decreases from 5.138 to 4.838 GeV; cross sections, backgrounds and thresholds need recalculation. |
| Electron angle | 25.486° calculated | 26.651° | NPS placement and exposure differ. |
| Proton angle | 19.164° calculated | 18.797° | HMS setting and rate environment change. |
| Proton momentum | 6.735 GeV/c calculated | 6.735 GeV/c | Same simple central precession estimate and similar first approximation to analyzer momentum dependence. |
| ε | 0.7033 | 0.6837 | Recompute polarization components and sensitivity. |
| Electron detector | BigCal at 10 m; HyCal considered separately | NPS at 3 m assumed | Geometry, containment, energy/position response and high-rate behavior change. |
| Target/current | 30 cm LH2, 75 μA | Same nominal values | Reconfirm actual density, windows, boiling and achievable current. |
| Polarimeter | Two 55 cm CH2 analyzers, planned chamber upgrades | Layout/number of analyzers under discussion | Rebuild COM and reconstruction assumptions. |
| Highest-point allocation | 65 days | 45 days in email | Exposures are not equivalent without a common efficiency/PAC-time convention. |
| Control strategy | Q² = 6 plus Q² = 8.5 physics points | Possible Q² = 8 commissioning point | Decide whether commissioning, a physics overlap measurement, and spin-transport validation require separate settings. |

The historical proposal supports a credible starting hypothesis for the reboot. It does not yet establish that a 45-day NPS configuration reaches or exceeds either the old projection or the achieved SBS sensitivity.

## Recommended next studies

1. Use the radiative and yield convention defined in the dedicated note. Carry the observed 3.4% internal-prescription shift as an initial model sensitivity, and replace it when a modern calculation or data benchmark supports a better estimate.
2. Archive Andrew's current decks, source commit, report and event files when available. Record all rates as events/mC as well as Hz, with the applied cuts and efficiencies beside each number.
3. Treat the reconstructed PAC37 elastic comparison as a passed scale benchmark: its cut-level yields agree within the accepted 20% tolerance. Recovery of old decks and source revisions is useful if convenient, but is no longer on the critical path.
4. Use the 10.7 GeV current configuration as the working baseline, with correlated elastic settings and NPS geometry. Change one group of assumptions at a time so radiation, acceptance, energy loss and resolution effects can be separated.
5. Quantify full electron-envelope coverage conditional on HMS acceptance, using an enlarged/uncut reference and practical shower fiducials. Scan detector distance and position, retaining the finite target and beam raster.
6. Implement the missing normalized hard-bremsstrahlung `gamma p -> pi0 p` channel using the Anderson wide-angle baseline in `pi0_cross_section_model.md`, including the explicit `s^-6`, `s^-7`, and `s^-8` weights. Then repeat the radiative elastic plus π⁰ study with common detector response, thresholds and cut-width scans. Treat aluminum and accidentals separately; compare the final information gain after cuts, not only elastic acceptance.
7. Confirm present FPP hardware and viable trigger layouts, and obtain contemporary HMS/NPS rates and DAQ limits. Assess one versus two analyzers jointly with trigger efficiency, tracking, background and analyzing power.
8. Establish event-by-event spin transport and a likelihood/covariance estimate of absolute uncertainty on the explicitly defined ratio. Resolve Table 4's μp normalization ambiguity, and include alignment and instrumental-asymmetry systematics.
9. Define the exposure ledger explicitly in PAC days, charge at the stated current, target-density correction, DAQ live time, tracking/selection and polarimeter efficiency; exclude accelerator availability and avoid applying any experiment-side factor twice. Compare old, reboot and SBS sensitivities using the same observable and conventions.

## Verification boundaries

Completed: text review of all 45 PAC37 pages, rendered overview of every page, detailed visual inspection of the main apparatus/spin/background/calorimeter figures and Tables 1-4; complete prior review of message4.pdf; independent elastic kinematics, rate-unit, accidental-rate, angular-resolution and geometry arithmetic; targeted inspection of local SIMC calorimeter/output paths.

The historical SIMC/GEANT/COSY results are reported proposal results, not newly reproduced or independently validated simulations. Page/figure/table references above distinguish source evidence from our calculations and interpretations. The original PAC34 document and references were not independently reviewed in full; no claim of current theory completeness or current hardware performance is made.

Not completed: exact reproduction of the original historical deck, confirmation of its exact cut normalization, detailed detector/background modeling, full HMS spin transport, a new statistical precision projection, or independent validation of SBS yields. Subsequent work has added reproducible current and reconstructed-historical input decks and input-configurable calorimeter dimensions; those changes are documented in the dedicated simulation notes.
