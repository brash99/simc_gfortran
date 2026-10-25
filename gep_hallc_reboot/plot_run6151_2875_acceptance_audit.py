#!/usr/bin/env python3
"""Overlay thresholded elastic SIMC acceptance on run-6151/2875 data maps."""

from __future__ import annotations

from array import array
import csv
from pathlib import Path

import numpy as np
import ROOT

from analyze_pi0_elastic_cuts import read_simc_binary
from plot_run6151_2875_simc_geant_overlay import threshold_mask_and_weights


ROOT.gROOT.SetBatch(True)
ROOT.gStyle.SetOptStat(0)
ROOT.gStyle.SetPalette(ROOT.kBird)

HERE = Path(__file__).resolve().parent
REPO = HERE.parent
INPUT = REPO / "tmp" / "simc_geant_overlays"
OUTPUT = HERE / "plots" / "nps_runs6151_2875_acceptance_audit"
MP = 0.9382720813

RUNS = {
    6151: dict(ebeam=6.37042664, p0=2.639, threshold=1.6,
               pmin=2.1, pmax=3.05, thmin=32.5, thmax=40.5,
               binary=REPO / "worksim/nps_run6151_elastic_overlay.bin",
               geant=INPUT / "nps_run6151_elastic_actual_geometry_2335.root"),
    2875: dict(ebeam=10.53941793, p0=4.0872, threshold=1.4,
               pmin=3.2, pmax=4.75, thmin=26.0, thmax=34.0,
               binary=REPO / "worksim/nps_run2875_elastic_overlay.bin",
               geant=INPUT / "nps_run2875_elastic_actual_geometry_2152.root"),
}


def proton_momentum(ebeam, theta):
    c = np.cos(theta)
    return 2.0 * MP * ebeam * (ebeam + MP) * c / (
        (ebeam + MP) ** 2 - ebeam**2 * c**2
    )


def make_simulation_maps(run, cfg):
    simc = read_simc_binary(cfg["binary"])
    passed, geant_weights = threshold_mask_and_weights(cfg["geant"], cfg["threshold"])
    weights = np.asarray(simc["Weight"])
    if len(passed) != len(weights) or not np.allclose(
            weights, geant_weights, rtol=2e-6, atol=1e-15):
        raise RuntimeError(f"SIMC/GEANT4 join failed for run {run}")
    keep = passed
    theta = np.degrees(np.asarray(simc["theta_p"]))
    momentum = cfg["p0"] * (1.0 + np.asarray(simc["hsdelta"]) / 100.0)
    # SIMC's calorimeter-plane convention is (vertical, horizontal) relative
    # to the reconstructed NPS cluster convention.  Express the prediction on
    # the data axes before overlaying it.
    xpred = np.asarray(simc["el_ypred"])
    ypred = np.asarray(simc["el_xpred"])

    hms = ROOT.TH2D(f"sim_hms_{run}", "", 100, cfg["thmin"], cfg["thmax"],
                    100, cfg["pmin"], cfg["pmax"])
    nps = ROOT.TH2D(f"sim_nps_{run}", "", 100, -50.0, 50.0, 100, -45.0, 45.0)
    for th, mom, xx, yy, weight, accept in zip(
            theta, momentum, xpred, ypred, weights, keep):
        if not accept:
            continue
        hms.Fill(float(th), float(mom), float(weight))
        nps.Fill(float(xx), float(yy), float(weight))
    return hms, nps, int(np.count_nonzero(keep)), float(np.sum(weights[keep]))


def set_fractional_contours(histogram):
    peak = histogram.GetMaximum()
    histogram.SetContour(4, array("d", [0.10*peak, 0.30*peak, 0.60*peak, 0.90*peak]))
    histogram.SetLineColor(ROOT.kMagenta + 2)
    histogram.SetLineWidth(2)


def elastic_locus(cfg):
    angles = np.linspace(cfg["thmin"], cfg["thmax"], 300)
    momenta = proton_momentum(cfg["ebeam"], np.radians(angles))
    graph = ROOT.TGraph(len(angles), angles.astype("d"), momenta.astype("d"))
    graph.SetLineColor(ROOT.kBlack)
    graph.SetLineStyle(2)
    graph.SetLineWidth(2)
    return graph


def main():
    data_file = ROOT.TFile.Open(str(INPUT / "run6151_2875_acceptance_data.root"))
    maps = {}
    for run, cfg in RUNS.items():
        hms_data = data_file.Get(f"data_hms_all_{run}").Clone(f"hms_data_{run}")
        nps_data = data_file.Get(f"data_nps_core_{run}").Clone(f"nps_data_{run}")
        hms_data.SetDirectory(0)
        nps_data.SetDirectory(0)
        hms_sim, nps_sim, accepted, accepted_weight = make_simulation_maps(run, cfg)
        set_fractional_contours(hms_sim)
        set_fractional_contours(nps_sim)
        maps[run] = (hms_data, nps_data, hms_sim, nps_sim,
                     accepted, accepted_weight, elastic_locus(cfg))
    data_file.Close()

    canvas = ROOT.TCanvas("acceptance_audit", "Acceptance audit", 1500, 1180)
    canvas.Divide(2, 2, 0.003, 0.003)
    for col, run in enumerate((6151, 2875), start=1):
        cfg = RUNS[run]
        hms_data, nps_data, hms_sim, nps_sim, accepted, _, locus = maps[run]

        pad = canvas.cd(col)
        pad.SetRightMargin(0.14); pad.SetLeftMargin(0.12)
        pad.SetBottomMargin(0.12); pad.SetTopMargin(0.11)
        pad.SetLogz()
        hms_data.SetMinimum(1.0)
        hms_data.GetXaxis().SetTitle("HMS proton angle #theta_{p} (deg)")
        hms_data.GetYaxis().SetTitle("HMS proton momentum p_{p} (GeV/c)")
        hms_data.Draw("COLZ")
        hms_sim.Draw("CONT3 SAME")
        locus.Draw("L SAME")
        text = ROOT.TLatex(); text.SetNDC(True); text.SetTextSize(0.040)
        text.DrawLatex(0.14, 0.93, f"Run {run}: HMS acceptance overlap")
        text.SetTextSize(0.029)
        text.DrawLatex(0.14, 0.875, "data density; magenta=thresholded elastic SIMC contours")
        text.DrawLatex(0.14, 0.835, "black dashed=elastic locus at nominal beam energy")

        pad = canvas.cd(col + 2)
        pad.SetRightMargin(0.14); pad.SetLeftMargin(0.12)
        pad.SetBottomMargin(0.12); pad.SetTopMargin(0.11)
        pad.SetLogz()
        nps_data.SetMinimum(1.0)
        nps_data.GetXaxis().SetTitle("NPS cluster/predicted x (cm)")
        nps_data.GetYaxis().SetTitle("NPS cluster/predicted y (cm)")
        nps_data.Draw("COLZ")
        nps_sim.Draw("CONT3 SAME")
        box = ROOT.TBox(-32.4, -38.88, 32.4, 38.88)
        box.SetFillStyle(0); box.SetLineColor(ROOT.kBlack)
        box.SetLineStyle(2); box.SetLineWidth(2); box.Draw()
        text2 = ROOT.TLatex(); text2.SetNDC(True); text2.SetTextSize(0.040)
        text2.DrawLatex(0.14, 0.93, f"Run {run}: elastic-core NPS footprint")
        text2.SetTextSize(0.029)
        text2.DrawLatex(0.14, 0.875,
                        f"data |#delta p_{{p}}|<2%; magenta=SIMC+GEANT4 ({accepted} events)")
        text2.DrawLatex(0.14, 0.835, "black dashed=nominal active-face boundary")

    canvas.SaveAs(str(OUTPUT) + ".png")
    canvas.SaveAs(str(OUTPUT) + ".pdf")

    with Path(str(OUTPUT) + "_summary.csv").open("w", newline="") as stream:
        writer = csv.writer(stream)
        writer.writerow(["run", "data_hms_entries", "data_nps_core_entries",
                         "simc_geant_threshold_passed", "simc_passed_weight",
                         "data_hms_theta_mean_deg", "sim_hms_theta_mean_deg",
                         "data_hms_p_mean_GeV", "sim_hms_p_mean_GeV",
                         "data_nps_x_mean_cm", "sim_nps_x_mean_cm",
                         "data_nps_y_mean_cm", "sim_nps_y_mean_cm"])
        for run in (6151, 2875):
            hms_data, nps_data, hms_sim, nps_sim, accepted, weight, _ = maps[run]
            writer.writerow([run, int(hms_data.GetEntries()), int(nps_data.GetEntries()),
                             accepted, weight,
                             hms_data.GetMean(1), hms_sim.GetMean(1),
                             hms_data.GetMean(2), hms_sim.GetMean(2),
                             nps_data.GetMean(1), nps_sim.GetMean(1),
                             nps_data.GetMean(2), nps_sim.GetMean(2)])
            print(f"run={run} data_hms={int(hms_data.GetEntries())} "
                  f"data_nps_core={int(nps_data.GetEntries())} sim_passed={accepted}")


if __name__ == "__main__":
    main()
