# NPS hcana and trigger-data audit

Established 2026-10-23.

## Scope

This is a read-only audit of the current public Hall C analysis stack and the NPS production area on ifarm relevant to a trigger-rate study. It asks what can be recovered from replayed or raw NPS data without inventing a trigger model.

The inspected revisions were:

- [`JeffersonLab/hcana`](https://github.com/JeffersonLab/hcana), commit `fcef8e23e16228fbb930078b9c44ef5031bed4f9` (2026-08-10);
- [`JeffersonLab/NPSlib`](https://github.com/JeffersonLab/NPSlib), commit `fddb8e9d5355212510b8e2f5b5e2fffc8ce0f3bd` (2024-02-09); and
- [`JeffersonLab/hallc_replay`](https://github.com/JeffersonLab/hallc_replay), commit `33090b5f8b0f267e49ac22e5bc0b2778b5914461` (2026-02-19).

The repositories were cloned shallowly into `/tmp` and inspected; the user's older local hcana checkout was not modified or used as current evidence.

## Software boundary

NPS reconstruction is not implemented directly in current hcana. [`NPSlib`](https://github.com/JeffersonLab/NPSlib) is an hcana add-on library requiring a CMake installation of hcana version 0.97 or later. Its relevant classes include:

- `THcNPSApparatus`, `THcNPSArray`, and `THcNPSCalorimeter` for detector reconstruction;
- `THcNPSCluster` and `THcNPSShowerHit` for offline clusters and block hits;
- `THcNPSCoinTime` and `THcNPSSecondaryKine` for coincidence and physics quantities;
- `Decoder::VTPModule` and `Decoder::VLDModule` for trigger electronics; and
- `THcNPSConfigEvtHandler` for the run's embedded FADC/VTP configuration.

Current `hallc_replay` master contains only incidental NPS references and no NPS replay configuration. The production material is instead in experiment-specific replay checkouts on ifarm.

## Production checkout and concrete run evidence

A read-only ifarm survey located the NPS production replay at `/work/hallc/nps/cmorean/nps_replay`. It is on branch `develop` at commit `267ca6fb...` (2024-07-02), with remote `github:mrcmor100/nps_replay.git`. It contains the NPS database, maps, parameters, replay scripts, output definitions, templates, and analysis macros that are absent from ordinary `hallc_replay` master.

The production replay script `SCRIPTS/NPS/replay_production_coin_NPS_HMS.C` registers `THcNPSConfigEvtHandler`. `DEF-files/NPS/PRODUCTION/VTP/nps_vtp_vars.def` writes `NPS.cal.vtpClusE`, `vtpClusX`, `vtpClusY`, `vtpClusTime`, and `vtpClusSize`. The run template also prints the event-137 settings into the report. Thus the public NPSlib data path is actually wired into the experiment replay, rather than merely being dormant library code.

A raw file for elastic run 6151 was found at `/work/hallc/nps/cmorean/NPS_PROD_6/raw/nps_coin_6151.dat.0` (2,276,654,668 bytes; the directory links to the Hall C cache). The run database gives 6.37042664 GeV beam energy, HMS central momentum 2.639 GeV/c, and HMS angle -35.97 degrees. The NPS elastic-performance study identifies this as the 4.598 GeV electron setting, the available operating point nearest the proposed 4.838 GeV GEp-IV signal.

Experiment identity: this is an elastic-calibration run from the 2023-2024 Hall C NPS Run Group 1a campaign, acquired during the E12-13-010 DVCS running period. It begins the published 6151-6168 calibration group at 3-pass beam and 9.5 m NPS distance; it is not a DVCS production run.

The embedded configuration text was extracted read-only from that EVIO file. The relevant recorded integer settings are:

| Setting | Run 6151 value |
| --- | ---: |
| Cluster trigger threshold | 1600 |
| Cluster readout threshold | 350 |
| Cluster-pair trigger threshold | 800 |
| Seed threshold | 50 |
| Minimum hit count | 1 |
| Hit coincidence window | 5 |
| Pair-trigger width | 20 |
| NPS trigger width | 20 |
| VTP readout window width | 1000 |

The DAQ documentation states that the trigger gain normally maps ADC sums to MeV, making 1600 nominally about 1.6 GeV, but the exact run-dependent gain convention still needs confirmation before assigning physical units. On that nominal interpretation the operational trigger was about 34.8% of the 4.598 GeV elastic energy. This is a useful calibration-run anchor, not a prediction of the rate at the proposed 70-80% GEp-IV threshold.

## Private 10,000-event replay

A private replay was completed on ifarm on 2026-10-23. All files are under `/work/hallc/gep/brash/gep4_nps_run6151_private`; no collaborator checkout or shared production output was modified. The reproducible software combination was:

- hcana 1.1 / Podd 1.7.12 / ROOT 6.30.04 from the Hall A module installation;
- official NPSlib commit `fddb8e9d5355212510b8e2f5b5e2fffc8ce0f3bd`; and
- NPS replay commit `267ca6fbdeb15a1f96b6f4653c9d066d7af51159`.

The earlier production-era NPSlib commit `bf8ec571...` builds against hcana 1.1 but cannot run this replay macro because it predates `THcNPSConfigEvtHandler`. The compatible official revision above both builds and replays successfully.

The replay read events 1-10,000 from segment 0 and produced:

- `nps_hms_coin_6151_1_10000.root`, 64 MB, with exactly 10,000 physics-tree entries; and
- `coin_NPS_HMS_report_6151_10000.report`, 17 KB, reproducing the embedded 1600/350/800 threshold settings.

The output contains the requested offline `clus*`, firmware `vtpClus*`, `vtpTrigType*`, BCM current/charge, scaler, and EDTM branches. There were no events with a nonzero `vtpErrorFlag`. First-look, uncut diagnostics are:

| Quantity | Result |
| --- | ---: |
| Events with at least one VTP cluster | 5,745 |
| Decoded VTP cluster objects | 70,215 |
| VTP cluster-energy range | 50-8,642 integer units |
| Events with any VTP cluster energy at least 350 | 3,749 |
| Events with any VTP cluster energy at least 1,600 | 2,247 |
| Events with at least one offline cluster | 2,204 |
| Events with any offline cluster energy at least 1.6 GeV | 2,181 |

Of 2,951 decoded trigger-decision samples, trigger type 0 was set 2,284 times, type 3 was set 2,948 times, and type 4 was set 15 times; types 1, 2, and 5 were never set. This is a calibration-run subsample with a large EDTM fraction: 7,796 of the 10,000 physics events contain the accepted EDTM signal according to the replay cuts. Consequently, raw fractions in this sample are not unbiased physics trigger efficiencies or rates. The next analysis must separate EDTM and ordinary physics events and use scaler time/current normalization.

That separation and a firmware/offline comparison are now documented in [`nps_run6151_firstlook.md`](nps_run6151_firstlook.md). A subsequent 100,000-event replay confirms that a nominal 3.5 GeV firmware threshold retains 85.39% of ordinary recorded events, while 3.7 GeV retains 78.83%; these correspond to 76.1% and 80.5% of the 4.598 GeV elastic energy. The larger sample contains 21 and 15 EDTM events above those thresholds, respectively. These are recorded-sample counts, not yet rate or efficiency projections.

## Run-embedded trigger configuration

`THcNPSConfigEvtHandler` parses event type 137 configuration text and exports these strings through the hcana parameter list:

- `FADC250_SPARSIFICATION`;
- `VTP_NPS_ECALCLUSTER_CLUSTER_TRIGGER_THR`;
- `VTP_NPS_ECALCLUSTER_CLUSTER_READOUT_THR`; and
- `VTP_NPS_ECALCLUSTER_CLUSTER_PAIR_TRIGGER_THR`.

This directly confirms that the trigger and readout thresholds can be recovered from the data stream rather than reconstructed from memory or inferred from an offline cut. The handler exists specifically because the VTP keys are not exposed by the ordinary `THaRunBase::GetDAQInfo` path.

## Recorded VTP observables

`Decoder::VTPModule` decodes the production VTP bank:

- data type 3: trigger time;
- data type 12: cluster energy, time, number of blocks, x coordinate, and y coordinate; and
- data type 13: trigger decision pattern and time.

The cluster energy is stored as a 14-bit quantity, cluster time as 11 bits, multiplicity as 4 bits, x as 5 bits, and y as 6 bits. `THcNPSCalorimeter` exposes these as hcana variables:

- `vtpClusE`, `vtpClusTime`, `vtpClusSize`, `vtpClusX`, and `vtpClusY`;
- `vtpTrigTime` and `vtpTrigCrate`; and
- `vtpTrigType0` through `vtpTrigType5`.

The source labels the trigger types as production cluster singles, cosmic scintillator, cosmic column, cluster pair across crates, cluster pair within a crate, and VLD. It combines objects decoded from multiple VTP crates and sets an error flag when the VTP trigger number and event number disagree.

These variables provide the event-level ingredients for measuring the spectrum of firmware clusters, checking which trigger decision fired, and comparing threshold settings between runs.

## Offline calorimeter reconstruction

`THcNPSCalorimeter` also produces offline cluster position, time, energy, and size. It offers the traditional hcana calorimeter clustering algorithm and a cellular-automata option patterned after Hall A DVCS reconstruction. The clustering time window is database-configurable and defaults to plus or minus 40 ns in the inspected source.

The firmware VTP clusters and offline calorimeter clusters are separate products. A valid trigger-efficiency study should match them explicitly; an offline cluster-energy cut is not equivalent to replaying the hardware decision.

## What this establishes

1. The existing analysis stack can decode the NPS firmware cluster objects and decisions.
2. The actual per-run trigger, readout, and pair thresholds are embedded in event type 137 and have an NPS-specific parser.
3. Both VTP and offline cluster quantities can be written to replay output, enabling direct firmware-versus-offline comparisons.
4. The production replay really writes the decoded VTP quantities, and run 6151 contains recoverable event-137 settings.
5. The public source supports the data-driven trigger study proposed in `nps_geant4_survey.md`; no speculative 3x3 algorithm is required for recorded NPS runs.

## Evidence gaps

- The exact NPSlib and hcana revisions used to produce final pass-two files remain to be established; collaborator work areas contain several 2023-era checkouts.
- No pre-existing production ROOT file or report was found, but the private 10,000-event replay now exercises the required branches on run 6151.
- The ROOT trigger-emulator source described in the 2024 DAQ presentation was not found in hcana, NPSlib, or ordinary hallc_replay master.
- `macros/NPS/VTP_hcana.C` analyzes decoded VTP branches but is not a waveform-to-firmware emulator. Its initial cluster loop begins at index 1 and therefore appears to omit the first decoded cluster; this should be understood before using the macro quantitatively.
- NPSlib contains GitHub build workflows but no focused source-level test suite in the inspected tree. This audit verifies code paths by inspection, not by executing a replay.

## Next concrete check

Run 6151 and the production checkout now supply a concrete starting point. The next bounded study should:

1. confirm the calibrated VTP ADC-to-MeV convention and exact semantics of each trigger-type bit;
2. separate ordinary physics triggers from EDTM events and determine the usable beam-on interval and scaler normalization;
3. compare firmware and offline cluster energy, position, multiplicity, and matching efficiency in the private sample;
4. locate the separate trigger emulator and reproduce its FADC-to-VTP cluster agreement check; and
5. form firmware-cluster spectra and rates versus beam current before extrapolating thresholds to the proposed GEp-IV luminosity.

This should precede a new absolute background generator: the recorded NPS cluster spectra can first show which components and extrapolations actually control the relevant 70-80% elastic-energy threshold.
