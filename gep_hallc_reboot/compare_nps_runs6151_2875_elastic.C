#include <TCanvas.h>
#include <TFile.h>
#include <TH1D.h>
#include <TLatex.h>
#include <TLegend.h>
#include <TStyle.h>
#include <TTree.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>

namespace {
constexpr int kMax = 4096;
constexpr double kMp = 0.9382720813;

long long FillSpectrum(const char* filename, double ebeam, TH1D& histogram) {
  TFile file(filename, "READ");
  auto* tree = dynamic_cast<TTree*>(file.Get("T"));
  if (!tree) return -1;
  double edtm = 0., ntrack = 0.;
  int nclus = 0;
  std::array<double, kMax> p{}, dp{}, energy{};
  tree->SetBranchAddress("T.hms.hEDTM_tdcTimeRaw", &edtm);
  tree->SetBranchAddress("H.dc.ntrack", &ntrack);
  tree->SetBranchAddress("H.gtr.p", p.data());
  tree->SetBranchAddress("H.gtr.dp", dp.data());
  tree->SetBranchAddress("Ndata.NPS.cal.clusE", &nclus);
  tree->SetBranchAddress("NPS.cal.clusE", energy.data());
  long long selected = 0;
  for (Long64_t i = 0; i < tree->GetEntries(); ++i) {
    tree->GetEntry(i);
    if (edtm != 0. || ntrack != 1. || std::abs(dp[0]) >= 8. || p[0] <= 0. ||
        nclus <= 0 || nclus > kMax) continue;
    const int imax = std::distance(energy.begin(),
        std::max_element(energy.begin(), energy.begin() + nclus));
    const double expected = ebeam + kMp - std::sqrt(p[0]*p[0] + kMp*kMp);
    if (expected <= 0.) continue;
    histogram.Fill(energy[imax] / expected);
    ++selected;
  }
  if (histogram.Integral() > 0.) histogram.Scale(1. / histogram.Integral(), "width");
  return selected;
}
}

void compare_nps_runs6151_2875_elastic(
    const char* run6151,
    const char* run2875,
    const char* output = "nps_runs6151_2875_elastic_comparison") {
  gStyle->SetOptStat(0);
  TH1D h6151("h6151", "", 130, 0., 1.3);
  TH1D h2875("h2875", "", 130, 0., 1.3);
  const auto n6151 = FillSpectrum(run6151, 6.37042664, h6151);
  const auto n2875 = FillSpectrum(run2875, 10.53941793, h2875);
  h6151.SetLineColor(kBlue + 1); h6151.SetLineWidth(3);
  h2875.SetLineColor(kRed + 1); h2875.SetLineWidth(3);
  h6151.GetXaxis()->SetTitle("E_{NPS}/E_{e}^{expected}(p_{HMS})");
  h6151.GetYaxis()->SetTitle("fraction per unit ratio");
  h6151.SetMaximum(1.18 * std::max(h6151.GetMaximum(), h2875.GetMaximum()));

  TCanvas canvas("comparison", "Run 6151 and 2875", 1100, 760);
  h6151.Draw("HIST");
  h2875.Draw("HIST SAME");
  TLegend legend(.16, .70, .58, .88);
  legend.SetBorderSize(0);
  legend.AddEntry(&h6151, "Run 6151: Q^{2}=3.74 GeV^{2}, E_{thr}#approx1.6 GeV", "l");
  legend.AddEntry(&h2875, "Run 2875: Q^{2}=6.51 GeV^{2}, E_{thr}#approx1.4 GeV", "l");
  legend.Draw();
  TLatex label; label.SetNDC(); label.SetTextSize(.034);
  label.DrawLatex(.16, .65, "ordinary events; one HMS track; |#delta|<8%; NPS cluster");
  label.DrawLatex(.16, .60, Form("selected: %lld (6151), %lld (2875)", n6151, n2875));
  canvas.SaveAs((std::string(output) + ".png").c_str());
  canvas.SaveAs((std::string(output) + ".pdf").c_str());
  std::cout << "run6151_selected=" << n6151 << " run2875_selected=" << n2875 << '\n';
}
