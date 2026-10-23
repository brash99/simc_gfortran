# First parameterized NPS response scan

Established 2026-10-22.

## Source constraint

Andrew's October 22 email does not specify a numerical trigger threshold. It says that background rates and pileup could complicate NPS energy resolution, triggering, and offline analysis, and that sufficiently good energy resolution should permit a “pretty high” electron-calorimeter threshold, presumably better than was achieved in SBS. His separate geometry estimate assumes 5 mm spatial resolution at 3 m. These are two distinct statements: 5 mm is a numerical position-response assumption; the energy threshold is an optimization question, not a supplied setting.

The historical PAC37 study used thresholds near half the elastic-electron energy, but that BigCal convention is not attributed to Andrew and is not adopted as the NPS baseline here.

## Implemented response model

`analyze_pi0_elastic_cuts.py` now applies a reproducible, parameterized response to the existing accepted elastic and pi0 samples:

- independent Gaussian x/y cluster smearing, with 0.5 cm as the default corresponding to Andrew's estimate;
- optional energy smearing

      sigma(E)/E = a/sqrt(E/GeV) (+) b,

  where `a` and `b` are command-line parameters and default to zero pending an NPS-specific supported choice;
- a scan of measured cluster-energy thresholds from 0 through 4.6 GeV;
- identical response parameters for electromagnetic electron and photon clusters;
- refitting of the elastic position core after position smearing, followed by the same 3-sigma and 6-sigma position-plus-proton-momentum cuts.

For the elastic sample, incident energy is the 4.838 GeV central setting times `1 + ssdelta/100`. For pi0 events, each accepted decay photon is treated as a separate cluster with its generated photon energy. A pi0 event passes the energy requirement if at least one geometrically accepted photon cluster is above threshold and that same cluster passes the position ellipse.

The default random seed is fixed at 20261022. Zero resolution parameters do not consume random numbers for that response component.

## Baseline results

The table below uses 5 mm x/y smearing and no assigned energy smearing. The latter isolates threshold placement from an as-yet unsupported NPS energy-resolution number.

| Threshold (GeV) | Cut width | Elastic efficiency | pi0 efficiency | Elastic (/mC) | pi0 (/mC) | pi0 fraction |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0.0 | 3 sigma | 80.334% | 1.4246% | 33.3240 | 0.19022 | 0.5676% |
| 0.0 | 6 sigma | 89.568% | 9.1886% | 37.1544 | 1.22686 | 3.1965% |
| 2.4 | 3 sigma | 80.334% | 1.4246% | 33.3240 | 0.19022 | 0.5676% |
| 2.4 | 6 sigma | 89.568% | 9.1634% | 37.1544 | 1.22350 | 3.1880% |
| 3.5 | 3 sigma | 80.334% | 1.4146% | 33.3240 | 0.18888 | 0.5636% |
| 3.5 | 6 sigma | 89.568% | 8.0680% | 37.1544 | 1.07724 | 2.8177% |
| 4.0 | 3 sigma | 80.334% | 1.3664% | 33.3240 | 0.18244 | 0.5445% |
| 4.0 | 6 sigma | 89.568% | 5.8787% | 37.1544 | 0.78492 | 2.0689% |
| 4.2 | 3 sigma | 79.487% | 1.2694% | 32.9727 | 0.16949 | 0.5114% |
| 4.2 | 6 sigma | 88.663% | 4.5787% | 36.7792 | 0.61136 | 1.6351% |
| 4.4 | 3 sigma | 75.742% | 1.0129% | 31.4192 | 0.13524 | 0.4286% |
| 4.4 | 6 sigma | 84.282% | 3.0126% | 34.9620 | 0.40224 | 1.1374% |
| 4.6 | 3 sigma | 65.064% | 0.7106% | 26.9899 | 0.09488 | 0.3503% |
| 4.6 | 6 sigma | 72.468% | 1.7088% | 30.0610 | 0.22817 | 0.7533% |

The main result is not an optimum threshold yet. A 2.4 GeV threshold, approximately half the central elastic energy, has essentially no effect on the pi0 events that already resemble elastic scattering. Significant rejection begins near 4 GeV, while visible elastic loss begins above about 4.0 GeV. Thus Andrew's suggestion points to the experimentally important 4.0–4.4 GeV turn-on region, not to the historical half-energy threshold.

The 5 mm smearing also broadens the fitted position ellipse. Relative to the geometry-only scan, that raises rather than lowers the selected pi0 fraction. This is expected behavior for a cut expressed in units of the measured elastic resolution and demonstrates why the response must be included symmetrically in signal and background.

As a sensitivity check only, using `a=0.02` and `b=0.01` changes the tabulated fractions only slightly. Those coefficients are not claimed as NPS performance and are not the baseline.

## Diagnostic figures

The shaded band in the threshold scan marks 4.0--4.4 GeV, where useful pi0 rejection begins but elastic efficiency also starts to fall.

![Elastic efficiency and selected pi0 fraction versus NPS cluster threshold](plots/nps_threshold_tradeoff.png)

The one-dimensional distributions show why the threshold cannot replace the elastic position and proton-momentum constraints: the surviving pi0 tail overlaps the elastic response in each observable.

![Elastic and pi0 response distributions](plots/nps_residual_distributions.png)

The two-dimensional view isolates the pi0 population whose best-position photon approaches the elastic locus. The density is normalized to its most populated bin; it is intended to show structure, not an absolute rate.

![Pi0 photon-cluster energy versus position residual](plots/nps_pi0_energy_position.png)

PDF versions of all three figures are written beside the PNG files in `gep_hallc_reboot/plots/`.

## Reproduction

    python3 gep_hallc_reboot/analyze_pi0_elastic_cuts.py \
      worksim/gep_reboot_q11_smoke.bin \
      worksim/gep_reboot_q11_pi0phot.bin

For an explicit resolution hypothesis, add (for example) `--nps-stochastic 0.02 --nps-constant 0.01`. Use `--thresholds-gev`, `--nps-position-sigma-cm`, and `--response-seed` to control the scan.

To regenerate the figures:

    python3 gep_hallc_reboot/plot_nps_response.py \
      worksim/gep_reboot_q11_smoke.bin \
      worksim/gep_reboot_q11_pi0phot.bin

## Evidence boundary and next work

This is a fast response convolution over transport-accepted clusters, not a shower simulation. It does not yet model crystal-by-crystal shower sharing, leakage and containment, cluster merging/splitting, dead material, nonlinear response, trigger sums, pileup, timing, noise, edge-dependent resolution, or rate-dependent efficiency. The two pi0 photons are treated as separate clusters. Consequently, these numbers identify the relevant threshold region but are not a trigger-rate or final-contamination prediction.

The next defensible step is to obtain an NPS-supported energy-resolution and trigger-sum model, then add cluster containment/merging and repeat the threshold scan. The cross-section exponent and vertex-dependent radiator variations remain separate physics-model tasks.
