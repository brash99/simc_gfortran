# Hard-bremsstrahlung neutral-pion generator

Implemented and first exercised on 2026-10-22.

## Scope

The new `doing_pi0phot` mode simulates

    e -> e gamma,   gamma p -> pi0 p,   pi0 -> gamma gamma

for a hard bremsstrahlung photon produced upstream of the interaction. It is deliberately separate from SIMC's `doing_pizero` electroproduction path. The recoil proton is transported through the normal P-arm Monte Carlo and both decay photons are transported independently through the E-side calorimeter. `pizero_ngamma` selects whether one or both photons must be accepted.

The generator samples `Egamma`, `cos(theta_pi_cm)`, and `phi_pi_cm` uniformly over explicit input bounds. It solves exact two-body kinematics, performs an isotropic two-photon decay with a general three-dimensional Lorentz boost, and weights each event with

    [dN_gamma / dEgamma] [d sigma / d Omega_cm].

The photon spectrum is the first-order thin-radiator expression

    dN_gamma/dEgamma = (t/X0)/Egamma * [4/3 - 4y/3 + y^2],
    y = Egamma/Ein.

The cross section uses the local log-linear interpolation of the Anderson 4 and 5 GeV angular data documented in `pi0_cross_section_model.md`, independently scales both anchors to the generated `s`, and takes their geometric mean. The default fixed-angle scaling exponent is 7; values 6 and 8 provide the requested model variations. Generated angles outside the common Anderson angular domain are rejected rather than extrapolated silently.

SIMC's ordinary `using_rad` path is forbidden in this mode because its hard-photon flux is already part of the event weight.

## Input parameters

The checked-in fixed-seed study deck is `infiles/gep_reboot_q11_pi0phot.inp`. New parameters are:

- `doing_pi0phot`
- `pi0phot_egamma_min`, `pi0phot_egamma_max` in MeV
- `pi0phot_costh_min`, `pi0phot_costh_max`
- `pi0phot_phi_min`, `pi0phot_phi_max` in degrees
- `pi0phot_radlen` in radiation lengths
- `pi0phot_scale_exponent`

The current deck samples 9.5 GeV to 10.699 GeV, `-0.25 < cos(theta_pi_cm) < 0`, and 80 to 100 degrees in center-of-mass azimuth. Its effective upstream radiator is provisionally `0.0175 X0`, one half of the approximately `3.5% X0` full 30 cm liquid-hydrogen target.

## Verification

`tests/test_pi0phot.f` provides deterministic numerical checks. Compiled with the repository's real-size convention, it verifies:

- endpoint `d sigma/dt = 0.01158894 nb/GeV2`, consistent with the documented `0.01159` target;
- the `n=6` scaling variation and angular-domain rejection;
- positive, bounded thin-radiator flux;
- exact two-body four-momentum conservation to about `3e-12 MeV`;
- the endpoint recoil-proton momentum.

The full executable builds and the 100,000-trial fixed-seed deck completes. It gives:

| Quantity | Result |
| --- | ---: |
| Contributing HMS + at least one NPS photon events | 48,114 / 100,000 |
| HMS transport successes | 48,116 |
| Individual photon calorimeter trials | 96,214 |
| Individual photon calorimeter successes | 86,821 |
| Acceptance-folded yield for 1 mC | 13.352045 events/mC |

Repeated runs have identical `.gen`, `.geni`, and initial-RNG-state hashes. The `.hist` files differ only in their begin/end timestamps.

An integration check exposed and fixed a special-path defect before recording these numbers: because the generator returns before ordinary `complete_ev`, it must explicitly calculate the outgoing proton's target path. Without that initialization, proton multiple-scattering angles were NaN and no event survived HMS transport.

## Elastic-selection observables

The binary event output now contains eleven additional `doing_pi0phot` branches:

- `phot_Egamma`, `phot_costhcm`, and `phot_phicm` record the generated reaction point;
- `phot_pelastic` is the proton momentum predicted from its reconstructed angle under elastic `ep` kinematics;
- `phot_dpelastic` is `(p_recon - p_elastic(theta_p))/p_HMS` in percent;
- `phot_xpred` and `phot_ypred` are the NPS coordinates predicted from the reconstructed proton under the elastic hypothesis;
- `phot_dx1`, `phot_dy1`, `phot_dx2`, and `phot_dy2` are the accepted decay-photon coordinates minus that prediction. A `-1e10` sentinel marks a photon that did not enter NPS.

A 1,000-trial binary-output integration test produced 452 accepted events and 87 branches. All generated and predicted quantities were finite; each residual pair had one value for every corresponding accepted photon. The ROOT converter buffers were also enlarged consistently to 96 entries, fixing the remaining 80-entry array mismatch for this schema.

PAC37 pages 28-32 and Figures 20-23 were re-inspected to define the selection. The proposal specifies a calorimeter-position ellipse and a cut on `(p-p(theta_p))/p0`, and reports results for widths of 3 and 6 standard deviations. It does not publish the numerical standard deviations used in those cuts. Those widths must therefore be measured or modeled for the current HMS/NPS configuration; importing guessed BigCal values would not constitute a reproduced selection.

## Evidence boundary and next work

The quoted 13.352045 events/mC is a generator-level, transport-accepted background normalization. It is not a predicted elastic contamination. It includes the provisional thin-radiator flux, the baseline `s^-7` cross section, HMS transport, NPS rectangular acceptance, and the requirement that at least one decay photon enter NPS. It does not yet include calorimeter shower response, energy resolution, clustering or threshold effects, reconstructed elastic cuts, detector/DAQ efficiencies, or target boiling.

The most important next calculation is to produce the same position and angle-momentum residuals for the radiated elastic sample, determine their current HMS/NPS resolution and correlations, and scan common 3-sigma and 6-sigma selections for signal efficiency and pi0 leakage. The radiator treatment should then be upgraded from the fixed mean `0.0175 X0` to a vertex-dependent target path, and the complete study repeated for scaling exponents 6, 7, and 8. Lower accepted photon energies should also be checked against the Shupe data as described in the cross-section note.

That first common-variable scan has now been completed and is documented in `pi0_elastic_cut_scan.md`. The same next priorities remain, but the analysis now has a reproducible baseline against which detector-response and model changes can be measured.
