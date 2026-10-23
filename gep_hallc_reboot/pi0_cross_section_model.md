# Wide-angle `gamma p -> pi0 p` cross-section baseline

Established: 2026-10-22.

## Purpose and domain

This note defines the first defensible cross-section normalization for the hard-bremsstrahlung neutral-pion background at the current 10.7 GeV, Q2 approximately 11 GeV2 setting. It is a data-driven wide-angle extrapolation, not a claim that the reaction is theoretically understood at this energy.

At the bremsstrahlung endpoint and the central proton setting, two-body kinematics give

    E_gamma = 10.700 GeV
    theta_pi_cm = 98.255 deg
    s = 20.959377 GeV2
    t = -10.987885 GeV2
    u = -8.192564 GeV2

All three Mandelstam variables are large. The high-energy Regge descriptions inspected in this study are constrained at small `-t` or forward angles and should not be extrapolated to this point. In particular, Sibirtsev et al. state a reliable fitted region of approximately `-t < 2 GeV2`, while Mathieu, Fox and Szczepaniak describe `cos(theta_s) >= 0.6`. Neither contains the present point, where `cos(theta_pi_cm) = -0.1436` and `-t` is approximately 11 GeV2.

## Primary data anchor

The PAC37 proposal cites R. L. Anderson et al., *Phys. Rev. D* **14**, 679 (1976), DOI `10.1103/PhysRevD.14.679`. Its machine-readable HEPData record is `10.17182/hepdata.4592.v1`. The record contains full `gamma p -> pi0 p` angular distributions at 4 and 5 GeV with cross sections in nb/GeV2 and a 7% normalization systematic.

Log-linear interpolation in cross section versus `cos(theta_pi_cm)` at the required angle uses these neighboring points:

| E_gamma (GeV) | Neighboring cos(theta) | Neighboring d sigma/dt (nb/GeV2) | Interpolated at 98.255 deg (nb/GeV2) |
| ---: | ---: | ---: | ---: |
| 4.0 | -0.055, -0.203 | 6.78, 6.57 | 6.654 |
| 5.0 | -0.035, -0.167 | 1.61, 1.87 | 1.821 |

The interpolation is deliberately local and does not smooth away the measured angular structure.

## Endpoint extrapolation

Use the empirical central-angle scaling reported by Anderson et al.,

    d sigma/dt(s, theta_cm) = F(theta_cm) * s^-7.

Scaling the two independent angular interpolations to `s = 20.959377 GeV2` gives

| Anchor | Extrapolated d sigma/dt at 10.7 GeV (nb/GeV2) |
| --- | ---: |
| 4 GeV Anderson data | 0.01093 |
| 5 GeV Anderson data | 0.01229 |

Their geometric mean is

    d sigma/dt = 0.01159 nb/GeV2

at the endpoint central ray. The two extrapolations differ by 12% peak-to-peak despite starting at different energies. This agreement is a useful internal check, but it does not remove the uncertainty associated with extrapolating over roughly a factor of two in `s`.

For event weighting in center-of-mass solid angle,

    d sigma/d Omega_cm = (d sigma/dt) * p_gamma_cm * p_pi_cm / pi,

because `t` is linear in `cos(theta_cm)` and the azimuth is uniform. A generator may instead sample `t` directly; the phase-space Jacobian must be applied exactly once.

## Uncertainty prescription

Use `n = 7` as the baseline scaling exponent and treat `n = 7 +/- 1` as a model-variation envelope until higher-energy wide-angle data or the E99-114 parameterization can be obtained. At the present point the 5 GeV-to-10.7 GeV extrapolation ratio in `s` is 2.042, so this exponent variation alone changes the endpoint normalization by a factor of 2.04 up or down.

The initial normalization uncertainty should therefore be represented as asymmetric multiplicative model variations, not combined into a misleading small Gaussian error:

    baseline: 0.01159 nb/GeV2
    n = 6 variation: approximately 2.04 * baseline
    n = 8 variation: approximately baseline / 2.04

The 7% experimental normalization systematic, tabulated statistical errors, and interpolation choice are smaller than this extrapolation envelope. They should still be retained when the model is fitted over the full acceptance.

This large uncertainty is acceptable for the first background study because the immediate goals are to establish whether the channel is negligible, comparable to, or dominant over the desired elastic-selection leakage and to identify which cuts control it. Any final contamination or beam-time projection requires a tighter model or validation against suitable data.

## Full-acceptance implementation

The endpoint number is only a central-ray check. The production generator must:

1. sample the bremsstrahlung photon energy and vertex through the target;
2. compute exact two-body `gamma p -> pi0 p` kinematics;
3. evaluate the Anderson angular interpolation at the generated angle and apply `s^-7`, with the `n = 6, 7, 8` variations available as explicit weights;
4. generate isotropic `pi0 -> gamma gamma` decay and transport the proton and both photons;
5. apply the same HMS, NPS response, clustering, threshold, fiducial, and elastic-selection logic used for the signal.

The Anderson data cover the central wide-angle region needed for the nominal endpoint, but the implementation must flag generated angles outside the tabulated range rather than silently extrapolating the angular function. Lower photon energies that enter the accepted proton phase space should be checked against the Shupe et al. wide-angle dataset (`10.17182/hepdata.24315.v1`) and, where appropriate, the measured Anderson points themselves.

## Evidence boundary

Established: the current central kinematics, the applicability failure of the inspected forward/small-`t` models, the Anderson tabulated values, the local angular interpolation, and the two independent `s^-7` endpoint extrapolations.

Not yet established: a full angular fit, the accepted photon-energy range after HMS transport, the thick-target bremsstrahlung flux convention, detector leakage, or an absolute contamination fraction. The unpublished E99-114 cross-section parameterization used by later Hall A/Hall C studies would be valuable independent validation if it can be recovered, but it is not required to begin the explicitly uncertainty-banded simulation.
