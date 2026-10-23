# GEp-IV Hall C reboot: π⁰ background channel audit

Checked: 2026-10-22.

## Target process

The dominant historical inelastic background is hard-bremsstrahlung-induced real-photon production

    e p -> e gamma p -> e pi0 p, with gamma p -> pi0 p,

where the post-bremsstrahlung beam electron is not detected, the recoil proton enters HMS, and one or both π⁰-decay photons produce the NPS electromagnetic signal. This is not the same topology as ordinary exclusive electroproduction with a detected scattered electron.

## Checkout audit

The current checkout does **not** contain a verified generator and normalization model for this real-photon background:

- The `doing_pion`/`doing_pizero` examples generate exclusive or semi-inclusive electroproduction with a detected scattered electron and pion/decay photons in the two arms.
- The `doing_delta` path represents `(e,e'p)pi` electroproduction with the proton detected, but its `peedelta` cross-section function currently assigns the placeholder value `1.0` rather than the computed model cross section. It cannot supply a physical background normalization.
- `pizero_decay.f` supplies a correct two-photon decay mechanism for an already generated π⁰, and the calorimeter path can test one- or two-photon interception, but neither component supplies the missing hard-bremsstrahlung flux times real-photoproduction cross section.
- No deleted file or alternate branch visible in the local Git history contains the PAC37 real-photon generator.

The 2011 PAC37 proposal states that its calculation used the bremsstrahlung flux in hydrogen and an empirical photoproduction cross section with full `s` and center-of-mass-angle dependence, benchmarked against GEp-III. It does not publish the fit coefficients, input data table, source revision, or deck. Its cited Anderson et al. measurement establishes broad angular distributions and approximate `s^-7` behavior in the central region, but that scaling alone is not a complete normalization or angular model.

Therefore none of the current π⁰ example decks should be relabeled as the PAC37 background calculation, and the placeholder `doing_delta` rate should not be reported as physical.

## Endpoint kinematic check

An independent two-body calculation for `gamma p -> pi0 p`, using the physical proton and neutral-pion masses, demonstrates why this background is irreducible near the bremsstrahlung endpoint.

| Quantity | Current 10.7 GeV setting | PAC37 11.0 GeV setting |
| --- | ---: | ---: |
| Incident real-photon energy | 10.700 GeV | 11.000 GeV |
| Proton lab angle | 18.797° | 19.164° |
| Two-body proton momentum | 6.728583 GeV/c | 6.728650 GeV/c |
| π⁰ lab energy | 4.844585 GeV | 5.144518 GeV |
| π⁰ lab angle, opposite side | 26.595937° | 25.436149° |
| π⁰ center-of-mass angle | 98.255° | 96.379° |

For the current setting, compare these values with the elastic central values: proton momentum 6.735 GeV/c, proton angle 18.797°, electron energy 4.838 GeV, and electron angle 26.651°. The endpoint photoproduction final state is essentially centered on both detector arms. Central-arm geometry cannot reject it.

The current endpoint π⁰ has Lorentz factor 35.9. The minimum lab opening angle of its two decay photons is about 3.19°, corresponding to roughly 16.7 cm separation at a 3 m NPS distance for the symmetric transverse decay. More asymmetric decays can place one high-energy photon close to the π⁰ direction while the other is softer and farther away. Consequently, realistic clustering, thresholds, fiducial containment, and one-versus-two-cluster logic materially affect the leakage into the elastic sample.

## Limited energy-scaling statement

At fixed center-of-mass angle, the proposal's approximate `s^-7` law alone gives

    [s(10.7 GeV) / s(11.0 GeV)]^-7 = 1.204.

Thus lowering the beam energy from 11.0 to 10.7 GeV would increase the photoproduction cross section by about 20% under that limited assumption. The center-of-mass angle also shifts by about 1.9°, and the bremsstrahlung spectrum and detector acceptance change, so 1.204 must not be used as a complete background-rate rescaling.

## Verification verdict

**Partial.** The reaction topology, absence of a usable normalized generator in this checkout, and endpoint kinematic overlap are established with high confidence. A physical π⁰ contamination fraction is inconclusive because the required real-photon flux and full `s, theta_CM` cross-section model are missing.

## Required implementation path

1. Obtain the PAC37/GEp-III photoproduction routine or its fit/data table from an archival checkout if readily available; historical recovery is useful here because it supplies a missing model, not merely because it reproduces an old number.
2. Use the Anderson wide-angle data baseline and uncertainty prescription documented in `pi0_cross_section_model.md`: local angular interpolation, `s^-7` baseline, and explicit `n = 6, 7, 8` model weights. This supplies a defensible first normalization but remains a long extrapolation.
3. Generate the bremsstrahlung photon energy and production vertex through the 30 cm hydrogen target, then two-body `pi0 p` kinematics, isotropic `pi0 -> gamma gamma` decay, HMS transport, and NPS photon interception.
4. Pass both elastic and π⁰ samples through the same NPS response/clustering model and scan the proton angle-momentum residual, predicted-minus-measured calorimeter position, calorimeter energy, photon-cluster multiplicity, and thresholds.

Step 2 now exists. Historical recovery in step 1 is useful independent validation but is no longer required before implementation. Until steps 3-4 exist, background work cannot quote an absolute contamination percentage.
