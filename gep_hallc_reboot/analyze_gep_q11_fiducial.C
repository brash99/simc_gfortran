#include <TCanvas.h>
#include <TFile.h>
#include <TGraph.h>
#include <TH1D.h>
#include <TLegend.h>
#include <TStyle.h>
#include <TTree.h>

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

namespace {
struct Event {
  double weight;
  double x;
  double y;
  double response;
};

void Style(TGraph& graph, int color, int marker) {
  graph.SetLineColor(color);
  graph.SetMarkerColor(color);
  graph.SetMarkerStyle(marker);
  graph.SetLineWidth(2);
}
}

void analyze_gep_q11_fiducial()
{
  const char* base = "/work/hallc/gep/brash/gep4_nps_geant4/validation/";
  TFile input(TString(base) + "gep_reboot_q11_hybrid_10923.root");
  auto* tree = static_cast<TTree*>(input.Get("t_dvcs"));
  if (!tree || tree->GetEntries() != 10923) {
    std::cerr << "Expected 10923 entries" << std::endl;
    return;
  }

  double edep[1080], weight, px, py, pz, vx, vy, vz;
  tree->SetBranchAddress("edep", edep);
  tree->SetBranchAddress("psf", &weight);
  tree->SetBranchAddress("GSE_px", &px);
  tree->SetBranchAddress("GSE_py", &py);
  tree->SetBranchAddress("GSE_pz", &pz);
  tree->SetBranchAddress("GV_x", &vx);
  tree->SetBranchAddress("GV_y", &vy);
  tree->SetBranchAddress("GV_z", &vz);

  const double me = 0.510999e-3;
  const double theta = 26.651 * TMath::DegToRad();
  const double distance = 3000.;
  const double nx = std::sin(theta), nz = std::cos(theta);
  const double ux = std::cos(theta), uz = -std::sin(theta);
  std::vector<Event> events;
  double total_weight = 0.;
  for (Long64_t i = 0; i < tree->GetEntries(); ++i) {
    tree->GetEntry(i);
    double deposit = 0.;
    for (int block = 0; block < 1080; ++block) deposit += edep[block];
    const double energy = std::sqrt(px*px + py*py + pz*pz + me*me);
    const double gx = vy*10., gy = -vx*10., gz = vz*10.;
    const double gpx = py, gpy = -px, gpz = pz;
    const double pmag = std::sqrt(gpx*gpx + gpy*gpy + gpz*gpz);
    const double dx = gpx/pmag, dy = gpy/pmag, dz = gpz/pmag;
    const double step = (distance - gx*nx - gz*nz)/(dx*nx + dz*nz);
    const double ix = gx + step*dx, iy = gy + step*dy, iz = gz + step*dz;
    const double local_x = (ix-distance*nx)*ux + (iz-distance*nz)*uz;
    events.push_back({weight, local_x, iy, deposit/(1000.*energy)});
    total_weight += weight;
  }

  // Keep one full crystal pitch inside the outer physical boundary. Scan only
  // the negative-horizontal boundary associated with the localized shadow.
  const double pitch = 21.69;
  const double xmax = 30.*pitch/2. - pitch;
  const double ymax = 36.*pitch/2. - pitch;
  const double thresholds[] = {0.5, 0.7, 0.8, 0.9};
  const int colors[] = {kBlack, kBlue+1, kRed+1, kGreen+2};
  TGraph geometric, conditional[4], combined[4], yield[4];
  std::ofstream csv(TString(base) + "gep_reboot_q11_fiducial_scan.csv");
  csv << "xmin_mm,xmax_mm,ymax_abs_mm,geometric_fraction";
  for (double threshold : thresholds)
    csv << ",conditional_eff_t" << int(100*threshold)
        << ",combined_fraction_t" << int(100*threshold)
        << ",yield_events_per_mC_t" << int(100*threshold);
  csv << '\n' << std::setprecision(10);

  const double simc_yield = 41.341739;
  int point = 0;
  for (double xmin = -240.; xmin <= 0.; xmin += 10., ++point) {
    double fid_weight = 0., pass_weight[4] = {0.,0.,0.,0.};
    for (const auto& event : events) {
      if (event.x < xmin || event.x > xmax || std::fabs(event.y) > ymax) continue;
      fid_weight += event.weight;
      for (int cut = 0; cut < 4; ++cut)
        if (event.response >= thresholds[cut]) pass_weight[cut] += event.weight;
    }
    const double geo = fid_weight/total_weight;
    geometric.SetPoint(point, xmin, geo);
    csv << xmin << ',' << xmax << ',' << ymax << ',' << geo;
    for (int cut = 0; cut < 4; ++cut) {
      const double cond = fid_weight > 0. ? pass_weight[cut]/fid_weight : 0.;
      const double comb = pass_weight[cut]/total_weight;
      conditional[cut].SetPoint(point, xmin, cond);
      combined[cut].SetPoint(point, xmin, comb);
      yield[cut].SetPoint(point, xmin, simc_yield*comb);
      csv << ',' << cond << ',' << comb << ',' << simc_yield*comb;
    }
    csv << '\n';
  }
  csv.close();

  Style(geometric, kMagenta+2, 20);
  for (int cut = 0; cut < 4; ++cut) {
    Style(conditional[cut], colors[cut], 20+cut);
    Style(combined[cut], colors[cut], 20+cut);
    Style(yield[cut], colors[cut], 20+cut);
  }
  gStyle->SetOptStat(0);
  TCanvas canvas("canvas", "GEp Q2=11 fiducial scan", 1400, 1000);
  canvas.Divide(2,2);
  canvas.cd(1);
  TH1D frame1("frame1", "Geometric fiducial retention;Negative-horizontal boundary x_{min} (mm);Weighted fraction of SIMC sample", 24, -245., 5.);
  frame1.SetMinimum(0.); frame1.SetMaximum(1.03); frame1.Draw(); geometric.Draw("LP same");
  canvas.cd(2);
  TH1D frame2("frame2", "Threshold efficiency inside fiducial;Negative-horizontal boundary x_{min} (mm);Conditional weighted efficiency", 24, -245., 5.);
  frame2.SetMinimum(0.6); frame2.SetMaximum(1.02); frame2.Draw();
  for (auto& graph : conditional) graph.Draw("LP same");
  TLegend legend(0.15,0.18,0.42,0.42); legend.SetBorderSize(0);
  for (int cut=0; cut<4; ++cut) legend.AddEntry(&conditional[cut],Form("E_{dep}/E #geq %.1f",thresholds[cut]),"lp");
  legend.Draw();
  canvas.cd(3);
  TH1D frame3("frame3", "Combined fiducial and threshold retention;Negative-horizontal boundary x_{min} (mm);Weighted fraction of SIMC sample", 24, -245., 5.);
  frame3.SetMinimum(0.); frame3.SetMaximum(0.85); frame3.Draw();
  for (auto& graph : combined) graph.Draw("LP same");
  canvas.cd(4);
  TH1D frame4("frame4", "Retained elastic yield;Negative-horizontal boundary x_{min} (mm);Events/mC", 24, -245., 5.);
  frame4.SetMinimum(0.); frame4.SetMaximum(35.); frame4.Draw();
  for (auto& graph : yield) graph.Draw("LP same");
  canvas.SaveAs(TString(base) + "gep_reboot_q11_fiducial_scan.png");
  canvas.SaveAs(TString(base) + "gep_reboot_q11_fiducial_scan.pdf");
}
