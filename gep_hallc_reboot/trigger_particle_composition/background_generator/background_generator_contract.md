# NPS inclusive-background generator contract

## Question being answered

Run 6887 measures the total NPS cluster-trigger rate.  The existing
`F1F2IN21` calculation predicts only the scattered-electron part of that
rate.  This work will calculate additional target-produced particles, pass
them through the same NPS response and trigger analysis, and test the sum
against the measured absolute rate and its threshold dependence.

The measured excess over the inclusive-electron calculation is currently
called **unmodeled electromagnetic activity**.  It must not be identified as
photons until the generator and detector-response comparisons support that
interpretation.

## Additive channel ownership

The calculation is deliberately split into named, additive samples:

1. **Inclusive scattered electrons.**  This remains the validated
   `F1F2IN21` sample.  No new background sample may contain the same scattered
   electron.
2. **Real-photon neutral-pion production.**  The first implementation is
   beam-electron bremsstrahlung followed by `gamma p -> pi0 p` and
   `pi0 -> gamma gamma`.  It has no HMS requirement and transports either or
   both decay photons toward NPS.
3. **Electroproduced neutral mesons.**  Inclusive `ep -> e pi0 X` (and later
   eta production) is a separate channel.  Its scattered electron and decay
   photons require an explicit event-ownership rule to prevent overlap with
   item 1.
4. **Charged hadrons.**  Negative-pion production and any other charged
   target-primary background are separate samples.
5. **Material secondaries.**  Conversions, shower particles, and particles
   produced in detector or beam-line material belong to GEANT4.  They are not
   generated again at the target level.

The first channel implemented below is therefore a testable contribution,
not a complete background prediction.

## Absolute normalization

Each target-primary sample must carry an event rate in hertz.  For the first
channel the integrand is

    electron flux
      x target proton areal density
      x bremsstrahlung photon flux per incident electron and photon energy
      x d sigma(gamma p -> pi0 p) / d Omega_cm
      x generated phase-space volume.

The target vertex is sampled uniformly.  The bremsstrahlung radiator is the
liquid-hydrogen path upstream of that vertex, so its thickness is
vertex-dependent.  This initial calculation omits target windows and other
upstream material; those contributions must be added explicitly rather than
hidden in a fitted radiator thickness.

The `gamma p -> pi0 p` cross section is the existing Anderson-data model in
the common measured angular range, with the documented `s^-7` baseline and
`s^-6`/`s^-8` variations.  The generator must not extrapolate its angular
shape outside that range.

No normalization is fitted in the baseline prediction.  If a later empirical
normalization to run 6887 is used, it will be stated as a single fixed anchor,
then frozen before transfer to the GEp kinematics.

## Run-6887 closure observables

Closure requires more than one number.  Data and the summed simulations will
be compared in:

- absolute rate versus NPS energy threshold;
- deposited-energy spectrum;
- row and column illumination on the NPS face;
- single- versus multiple-cluster topology, within the limits of the data;
- stability under the cross-section and material-model variations.

The first useful test is whether this real-photon pi0 channel has the right
order of magnitude and threshold shape after NPS detector response.  Failure
is informative: it quantifies how much must come from electroproduction,
charged pions, other neutral channels, or unmodeled material.

The first SIDIS pi0 pilot supplies a second boundary: the fragmentation model
is used only for `Q2 > 1 GeV2` and `W > 2 GeV`. Low-Q2 events belong to the
planned inclusive photoproduction calculation, not to an extrapolation of the
SIDIS parameterization.

The low-Q2 region is modeled separately with the Wiser inclusive-hadron fit.
This fit already integrates the bremsstrahlung photon spectrum and returns an
effective electron-induced pion cross section for a supplied radiator. It
must therefore not be multiplied by the explicit bremsstrahlung flux used by
the exclusive real-photon generator. Its baseline pi0 parent prescription is
the isospin average of the fitted charged-pion cross sections. The much larger
undocumented pi0 convention in `evgen_inclusive` is retained only as a named
sensitivity variation.

## Evidence boundary

Generator-level photon energy on the NPS face is not a VTP trigger rate.
GEANT4 shower response, clustering, calibration convention, pileup, and the
actual trigger algorithm are required before comparison with the measured
curve.  Conversely, the run-6887 residual alone cannot determine particle
identity because NPS has no charged-particle tag.

