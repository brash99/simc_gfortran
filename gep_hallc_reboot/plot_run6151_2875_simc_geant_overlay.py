#!/usr/bin/env python3
"""Overlay thresholded SIMC+GEANT4 delta-p predictions on HMS-only data."""

from __future__ import annotations

import csv
from pathlib import Path

import numpy as np
import ROOT

from analyze_pi0_elastic_cuts import read_simc_binary


ROOT.gROOT.SetBatch(True)
ROOT.gStyle.SetOptStat(0)
ROOT.gStyle.SetEndErrorSize(0)

HERE = Path(__file__).resolve().parent
REPO = HERE.parent
INPUT = REPO / "tmp" / "simc_geant_overlays"
OUTPUT = HERE / "plots" / "nps_runs6151_2875_dpp_simc_geant_overlay"

NROWS = 36
NCOLS = 30
NBLOCKS = NROWS * NCOLS

RUNS = {
    6151: {
        "threshold": 1.6,
        "ordinary": 38325,
        "one_track": 35234,
        "observed_rate": 45.3149,
        "elastic_rate": 43.5423941626,
        "pi0_rate": 0.433525146174,
        "elastic_bin": REPO / "worksim" / "nps_run6151_elastic_overlay.bin",
        "pi0_bin": REPO / "worksim" / "nps_run6151_pi0phot_closure.bin",
        "elastic_g4": INPUT / "nps_run6151_elastic_actual_geometry_2335.root",
        "pi0_g4": INPUT / "nps_run6151_pi0phot_closure_6065.root",
        "title": "Run 6151: E_{beam}=6.370 GeV, Q^{2}#approx3.74 GeV^{2}",
    },
    2875: {
        "threshold": 1.4,
        "ordinary": 25504,
        "one_track": 19395,
        "observed_rate": 27.92,
        "elastic_rate": 8.16479122298,
        "pi0_rate": 0.074648396462,
        "elastic_bin": REPO / "worksim" / "nps_run2875_elastic_overlay.bin",
        "pi0_bin": REPO / "worksim" / "nps_run2875_pi0phot_closure.bin",
        "elastic_g4": INPUT / "nps_run2875_elastic_actual_geometry_2152.root",
        "pi0_g4": INPUT / "nps_run2875_pi0phot_closure_2218.root",
        "title": "Run 2875: E_{beam}=10.539 GeV, Q^{2}#approx6.51 GeV^{2}",
    },
}


def hottest_3x3(edep) -> float:
    values = np.fromiter((edep[i] for i in range(NBLOCKS)), dtype=float,
                         count=NBLOCKS)
    seed = int(np.argmax(values))
    column, row = divmod(seed, NROWS)
    total = 0.0
    for col in range(max(0, column - 1), min(NCOLS, column + 2)):
        for rr in range(max(0, row - 1), min(NROWS, row + 2)):
            total += values[rr + NROWS * col]
    return total / 1000.0


def threshold_mask_and_weights(path: Path, threshold: float):
    root_file = ROOT.TFile.Open(str(path))
    tree = root_file.Get("t_dvcs")
    passed, weights = [], []
    for event in tree:
        passed.append(hottest_3x3(event.edep) >= threshold)
        weights.append(float(event.psf))
    root_file.Close()
    return np.asarray(passed, dtype=bool), np.asarray(weights)


def simulation_histogram(binary_path: Path, geant_path: Path, branch: str,
                         threshold: float, expected_count: float,
                         name: str):
    simc = read_simc_binary(binary_path)
    passed, geant_weights = threshold_mask_and_weights(geant_path, threshold)
    values = np.asarray(simc[branch])
    weights = np.asarray(simc["Weight"])
    if len(values) != len(passed):
        raise RuntimeError(f"entry mismatch for {name}: SIMC={len(values)}, G4={len(passed)}")
    if not np.allclose(weights, geant_weights, rtol=2e-6, atol=1e-15):
        raise RuntimeError(f"SIMC/GEANT4 weight mismatch for {name}")

    histogram = ROOT.TH1D(name, "", 200, -10.0, 10.0)
    histogram.Sumw2()
    for value, weight, keep in zip(values, weights, passed):
        if keep:
            histogram.Fill(float(value), float(weight))
    raw_integral = histogram.Integral(0, histogram.GetNbinsX() + 1)
    if raw_integral <= 0:
        raise RuntimeError(f"empty thresholded histogram for {name}")
    histogram.Scale(expected_count / raw_integral)
    return histogram, int(np.count_nonzero(passed)), raw_integral


def main():
    data_file = ROOT.TFile.Open(str(INPUT / "nps_runs6151_2875_dpp_hms_only_full_acceptance.root"))
    results = {}
    for run, cfg in RUNS.items():
        data = data_file.Get(f"data_run{run}").Clone(f"data_{run}")
        data.SetDirectory(0)
        # The measured one-track fraction is applied as the detector-level HMS
        # tracking factor common to each presently modeled physics component.
        elastic_count = cfg["one_track"] * cfg["elastic_rate"] / cfg["observed_rate"]
        pi0_count = cfg["one_track"] * cfg["pi0_rate"] / cfg["observed_rate"]
        elastic, nel, wel = simulation_histogram(
            cfg["elastic_bin"], cfg["elastic_g4"], "el_dpelastic",
            cfg["threshold"], elastic_count, f"elastic_{run}")
        pi0, npi0, wpi0 = simulation_histogram(
            cfg["pi0_bin"], cfg["pi0_g4"], "phot_dpelastic",
            cfg["threshold"], pi0_count, f"pi0_{run}")
        total = elastic.Clone(f"modeled_total_{run}")
        total.Add(pi0)
        results[run] = (data, elastic, pi0, total, elastic_count, pi0_count,
                        nel, npi0, wel, wpi0)
    data_file.Close()

    canvas = ROOT.TCanvas("overlay", "SIMC+GEANT4 overlay", 1500, 670)
    canvas.Divide(2, 1, 0.002, 0.002)
    common_max = 1.18 * max(results[6151][0].GetMaximum(),
                            results[6151][3].GetMaximum(),
                            results[2875][0].GetMaximum(),
                            results[2875][3].GetMaximum())
    for ipad, run in enumerate((6151, 2875), start=1):
        cfg = RUNS[run]
        data, elastic, pi0, total, *_ = results[run]
        pad = canvas.cd(ipad)
        pad.SetLeftMargin(0.12)
        pad.SetRightMargin(0.035)
        pad.SetBottomMargin(0.13)
        pad.SetTopMargin(0.10)
        data.SetMinimum(0.0)
        data.SetMaximum(common_max)
        data.SetLineColor(ROOT.kRed + 1)
        data.SetMarkerColor(ROOT.kRed + 1)
        data.SetMarkerStyle(24)
        data.SetMarkerSize(0.68)
        data.GetXaxis().SetTitle("#delta p_{p} = 100[p_{p}-p_{p}(#theta_{p})]/p_{0}  (%)")
        data.GetYaxis().SetTitle("events / 0.100%")
        elastic.SetLineColor(ROOT.kBlue + 1)
        elastic.SetLineWidth(3)
        pi0.SetLineColor(ROOT.kOrange + 7)
        pi0.SetLineWidth(3)
        total.SetLineColor(ROOT.kBlack)
        total.SetLineWidth(2)
        total.SetLineStyle(2)
        data.Draw("E1")
        elastic.Draw("HIST SAME")
        pi0.Draw("HIST SAME")
        total.Draw("HIST SAME")
        data.Draw("E1 SAME")

        text = ROOT.TLatex()
        text.SetNDC(True)
        text.SetTextSize(0.040)
        text.DrawLatex(0.15, 0.92, cfg["title"])
        text.SetTextSize(0.030)
        text.DrawLatex(0.15, 0.855,
                       f"rate-ratio normalization; NPS 3#times3 threshold={cfg['threshold']:.1f} GeV")
        text.DrawLatex(0.15, 0.81, "accidental component not drawn (no #delta p_{p} template)")
        if run == 6151:
            legend = ROOT.TLegend(0.15, 0.56, 0.53, 0.76)
            legend.SetBorderSize(0)
            legend.SetFillStyle(0)
            legend.SetTextSize(0.030)
            legend.AddEntry(data, "data: ordinary, one HMS track", "pe")
            legend.AddEntry(elastic, "radiative elastic SIMC+GEANT4", "l")
            legend.AddEntry(pi0, "correlated #pi^{0} SIMC+GEANT4", "l")
            legend.AddEntry(total, "elastic + correlated #pi^{0}", "l")
            legend.Draw()

    # Save the three linear-y views used for the earlier data-only comparison.
    # The underlying histograms retain common 0.1%-wide bins, so every view is
    # a literal zoom of the same normalized data and simulation samples.
    views = (
        ("", -10.0, 10.0),
        ("_minus10_to_2", -10.0, 2.0),
        ("_minus2_to_2", -2.0, 2.0),
    )
    for suffix, xmin, xmax in views:
        for run in (6151, 2875):
            for histogram in results[run][:4]:
                histogram.GetXaxis().SetRangeUser(xmin, xmax)
        canvas.Modified()
        canvas.Update()
        canvas.SaveAs(str(OUTPUT) + suffix + ".png")
        canvas.SaveAs(str(OUTPUT) + suffix + ".pdf")

    with (Path(str(OUTPUT) + "_summary.csv")).open("w", newline="") as stream:
        writer = csv.writer(stream)
        writer.writerow(["run", "data_one_track", "observed_rate_Hz",
                         "elastic_rate_Hz", "pi0_rate_Hz",
                         "expected_elastic_counts", "expected_pi0_counts",
                         "elastic_g4_passed", "pi0_g4_passed"])
        for run in (6151, 2875):
            cfg = RUNS[run]
            _, _, _, _, ec, pc, nel, npi0, _, _ = results[run]
            writer.writerow([run, cfg["one_track"], cfg["observed_rate"],
                             cfg["elastic_rate"], cfg["pi0_rate"], ec, pc,
                             nel, npi0])

    for run in (6151, 2875):
        cfg = RUNS[run]
        _, _, _, _, ec, pc, nel, npi0, _, _ = results[run]
        print(f"run={run} expected_elastic={ec:.3f} expected_pi0={pc:.3f} "
              f"g4_passed={nel}/{npi0}")


if __name__ == "__main__":
    main()
