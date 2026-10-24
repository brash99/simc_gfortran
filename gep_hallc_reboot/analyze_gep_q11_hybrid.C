#include <TCanvas.h>
#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TLine.h>
#include <TStyle.h>
#include <TTree.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

void analyze_gep_q11_hybrid()
{
  const char* base = "/work/hallc/gep/brash/gep4_nps_geant4/validation/";
  TFile input(TString(base) + "gep_reboot_q11_hybrid_10923.root");
  auto* tree = static_cast<TTree*>(input.Get("t_dvcs"));
  if (!tree || tree->GetEntries() != 10923) {
    std::cerr << "Expected 10923 entries" << std::endl;
    return;
  }

  double edep[1080], weight, px, py, pz, vx, vy, vz;
  int entered;
  tree->SetBranchAddress("edep", edep);
  tree->SetBranchAddress("psf", &weight);
  tree->SetBranchAddress("GSE_px", &px);
  tree->SetBranchAddress("GSE_py", &py);
  tree->SetBranchAddress("GSE_pz", &pz);
  tree->SetBranchAddress("GV_x", &vx);
  tree->SetBranchAddress("GV_y", &vy);
  tree->SetBranchAddress("GV_z", &vz);
  tree->SetBranchAddress("nps_entry", &entered);

  TH1D h_response("h_response",
      "Raw NPS response;#Sigma E_{dep}/E_{primary};Weighted events", 120, 0., 1.08);
  TH2D h_face("h_face",
      "SIMC-predicted NPS face;Local horizontal (mm);Local vertical (mm)",
      80, -440., 440., 90, -495., 495.);
  TH2D h_response_face("h_response_face",
      "Mean response across NPS;Local horizontal (mm);Local vertical (mm)",
      30, -330., 330., 36, -396., 396.);
  TH2D h_response_weight("h_response_weight", "", 30, -330., 330., 36, -396., 396.);

  const double electron_mass_GeV = 0.510999e-3;
  const double theta = 26.651 * TMath::DegToRad();
  const double distance_mm = 3000.;
  const double nx = std::sin(theta), nz = std::cos(theta);
  const double ux = std::cos(theta), uz = -std::sin(theta);
  double sumw = 0., entryw = 0., anyw = 0., meanw = 0.;
  double below50w = 0., below70w = 0., below80w = 0., below90w = 0.;
  std::vector<double> ratios, weights;

  for (Long64_t i = 0; i < tree->GetEntries(); ++i) {
    tree->GetEntry(i);
    double total_mev = 0.;
    for (int block = 0; block < 1080; ++block) total_mev += edep[block];
    const double energy_gev = std::sqrt(px*px + py*py + pz*pz +
                                        electron_mass_GeV*electron_mass_GeV);
    const double ratio = total_mev / (1000. * energy_gev);

    // Map SIMC Hall (down,left,downstream) into GEANT4 (left,up,downstream).
    const double gx = vy * 10., gy = -vx * 10., gz = vz * 10.;
    const double gpx = py, gpy = -px, gpz = pz;
    const double pmag = std::sqrt(gpx*gpx + gpy*gpy + gpz*gpz);
    const double dx = gpx/pmag, dy = gpy/pmag, dz = gpz/pmag;
    const double step = (distance_mm - gx*nx - gz*nz) / (dx*nx + dz*nz);
    const double ix = gx + step*dx, iy = gy + step*dy, iz = gz + step*dz;
    const double local_x = (ix - distance_mm*nx)*ux +
                           (iz - distance_mm*nz)*uz;
    const double local_y = iy;

    sumw += weight;
    entryw += weight * (entered != 0);
    anyw += weight * (total_mev > 1.);
    meanw += weight * ratio;
    below50w += weight * (ratio < 0.5);
    below70w += weight * (ratio < 0.7);
    below80w += weight * (ratio < 0.8);
    below90w += weight * (ratio < 0.9);
    h_response.Fill(ratio, weight);
    h_face.Fill(local_x, local_y, weight);
    h_response_face.Fill(local_x, local_y, weight * ratio);
    h_response_weight.Fill(local_x, local_y, weight);
    ratios.push_back(ratio);
    weights.push_back(weight);
  }

  h_response_face.Divide(&h_response_weight);
  TH1D h_threshold("h_threshold",
      "NPS deposited-energy threshold;Threshold / E_{primary};Weighted efficiency",
      101, -0.005, 1.005);
  for (int bin = 1; bin <= h_threshold.GetNbinsX(); ++bin) {
    const double threshold = h_threshold.GetBinCenter(bin);
    double accepted = 0.;
    for (std::size_t i = 0; i < ratios.size(); ++i)
      if (ratios[i] >= threshold) accepted += weights[i];
    h_threshold.SetBinContent(bin, accepted / sumw);
  }

  const double simc_yield = 41.341739;
  std::ofstream summary(TString(base) + "gep_reboot_q11_hybrid_summary.csv");
  summary << std::setprecision(10)
          << "metric,value\n"
          << "events," << tree->GetEntries() << '\n'
          << "simc_yield_events_per_mC," << simc_yield << '\n'
          << "weighted_active_entry_fraction," << entryw/sumw << '\n'
          << "weighted_any_deposit_fraction," << anyw/sumw << '\n'
          << "weighted_mean_deposit_fraction," << meanw/sumw << '\n'
          << "weighted_below_50pct_fraction," << below50w/sumw << '\n'
          << "weighted_below_70pct_fraction," << below70w/sumw << '\n'
          << "weighted_below_80pct_fraction," << below80w/sumw << '\n'
          << "weighted_below_90pct_fraction," << below90w/sumw << '\n'
          << "yield_above_50pct_events_per_mC," << simc_yield*(1.-below50w/sumw) << '\n'
          << "yield_above_70pct_events_per_mC," << simc_yield*(1.-below70w/sumw) << '\n'
          << "yield_above_80pct_events_per_mC," << simc_yield*(1.-below80w/sumw) << '\n'
          << "yield_above_90pct_events_per_mC," << simc_yield*(1.-below90w/sumw) << '\n';
  summary.close();

  std::cout << std::setprecision(10)
            << "events=" << tree->GetEntries() << '\n'
            << "weighted_active_entry_fraction=" << entryw/sumw << '\n'
            << "weighted_any_deposit_fraction=" << anyw/sumw << '\n'
            << "weighted_mean_deposit_fraction=" << meanw/sumw << '\n'
            << "weighted_below50_fraction=" << below50w/sumw << '\n'
            << "weighted_below70_fraction=" << below70w/sumw << '\n'
            << "weighted_below80_fraction=" << below80w/sumw << '\n'
            << "weighted_below90_fraction=" << below90w/sumw << std::endl;

  gStyle->SetOptStat(0);
  TCanvas canvas("canvas", "GEp Q2=11 hybrid NPS response", 1400, 1050);
  canvas.Divide(2, 2);
  canvas.cd(1); gPad->SetLogy(); h_response.SetLineWidth(2); h_response.Draw("hist");
  canvas.cd(2); h_threshold.SetMinimum(0.); h_threshold.SetMaximum(1.03);
  h_threshold.SetLineWidth(2); h_threshold.Draw("hist");
  canvas.cd(3); h_face.Draw("colz");
  const double half_x = 30.*21.69/2.;
  const double half_y = 36.*21.69/2.;
  TLine left(-half_x,-half_y,-half_x,half_y), right(half_x,-half_y,half_x,half_y);
  TLine bottom(-half_x,-half_y,half_x,-half_y), top(-half_x,half_y,half_x,half_y);
  for (TLine* line : {&left,&right,&bottom,&top}) { line->SetLineColor(kRed); line->SetLineWidth(2); line->Draw(); }
  canvas.cd(4); h_response_face.SetMinimum(0.); h_response_face.SetMaximum(1.02);
  h_response_face.Draw("colz");
  canvas.SaveAs(TString(base) + "gep_reboot_q11_hybrid_response.png");
  canvas.SaveAs(TString(base) + "gep_reboot_q11_hybrid_response.pdf");
}
