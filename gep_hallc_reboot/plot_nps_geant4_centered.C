#include <TCanvas.h>
#include <TFile.h>
#include <TH1D.h>
#include <TLegend.h>
#include <TLine.h>
#include <TStyle.h>
#include <TTree.h>

#include <algorithm>
#include <cmath>
#include <iostream>

namespace {
void FillFractionHistogram(TTree* tree, TH1D* hist, double incident_mev) {
  double edep[1080];
  tree->SetBranchAddress("edep", edep);
  for (Long64_t event = 0; event < tree->GetEntries(); ++event) {
    tree->GetEntry(event);
    double sum = 0.;
    for (int block = 0; block < 1080; ++block) sum += edep[block];
    hist->Fill(sum / incident_mev);
  }
  hist->Scale(1. / hist->Integral());
}
}

void plot_nps_geant4_centered() {
  const char* base = "/work/hallc/gep/brash/gep4_nps_geant4/validation";
  TFile low(Form("%s/mono_6151_95m_centered_1000.root", base));
  TFile high(Form("%s/mono_2875_centered_1000.root", base));
  auto* low_tree = static_cast<TTree*>(low.Get("t_dvcs"));
  auto* high_tree = static_cast<TTree*>(high.Get("t_dvcs"));

  if (!low_tree || !high_tree) {
    std::cerr << "Missing t_dvcs input tree" << std::endl;
    return;
  }

  gStyle->SetOptStat(0);
  auto* low_full = new TH1D("low_full", ";Raw #SigmaE_{dep}/E_{incident};Fraction / 0.01", 105, 0., 1.05);
  auto* high_full = new TH1D("high_full", ";Raw #SigmaE_{dep}/E_{incident};Fraction / 0.01", 105, 0., 1.05);
  auto* low_core = new TH1D("low_core", ";Raw #SigmaE_{dep}/E_{incident};Fraction / 0.002", 55, 0.90, 1.01);
  auto* high_core = new TH1D("high_core", ";Raw #SigmaE_{dep}/E_{incident};Fraction / 0.002", 55, 0.90, 1.01);

  FillFractionHistogram(low_tree, low_full, 4598.);
  FillFractionHistogram(high_tree, high_full, 7072.);
  FillFractionHistogram(low_tree, low_core, 4598.);
  FillFractionHistogram(high_tree, high_core, 7072.);

  for (auto* hist : {low_full, low_core}) {
    hist->SetLineColor(kBlue + 1);
    hist->SetLineWidth(3);
  }
  for (auto* hist : {high_full, high_core}) {
    hist->SetLineColor(kRed + 1);
    hist->SetLineWidth(3);
  }

  TCanvas canvas("canvas", "Centered NPS GEANT4 response", 1300, 560);
  canvas.Divide(2, 1);

  canvas.cd(1);
  gPad->SetLogy();
  low_full->SetMinimum(5.e-4);
  low_full->SetMaximum(1.0);
  low_full->SetTitle("Full raw-deposit distribution (1,000 events each)");
  low_full->Draw("hist");
  high_full->Draw("hist same");
  TLine full_cut(0.90, 5.e-4, 0.90, 1.0);
  full_cut.SetLineStyle(2);
  full_cut.Draw();

  TLegend legend(0.14, 0.70, 0.58, 0.88);
  legend.SetBorderSize(0);
  legend.AddEntry(low_full, "4.598 GeV, 20.58^{o}, 9.5 m", "l");
  legend.AddEntry(high_full, "7.072 GeV, 16.991^{o}, 9.5 m", "l");
  legend.AddEntry(&full_cut, "90% contained-core boundary", "l");
  legend.Draw();

  canvas.cd(2);
  low_core->SetMaximum(1.15 * std::max(low_core->GetMaximum(), high_core->GetMaximum()));
  low_core->SetTitle("Contained-core zoom");
  low_core->Draw("hist");
  high_core->Draw("hist same");
  TLine core_cut(0.90, 0., 0.90, low_core->GetMaximum());
  core_cut.SetLineStyle(2);
  core_cut.Draw();
  legend.Draw();

  canvas.SaveAs(Form("%s/nps_geant4_centered_1000.png", base));
  canvas.SaveAs(Form("%s/nps_geant4_centered_1000.pdf", base));
}
