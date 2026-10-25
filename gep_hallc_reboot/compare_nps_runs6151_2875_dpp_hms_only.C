#include <TCanvas.h>
#include <TFile.h>
#include <TH1D.h>
#include <TLatex.h>
#include <TStyle.h>
#include <TTree.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

namespace {
constexpr int kMax = 64;
constexpr double kMp = 0.9382720813;

struct Counts {
  long long entries = 0;
  long long ordinary = 0;
  long long one_track = 0;
  long long in_range = 0;
  long long below_minus3 = 0;
  long long below_minus5 = 0;
};

double ProtonMomentumFromTheta(double ebeam, double theta) {
  const double c = std::cos(theta);
  const double denominator =
      (ebeam + kMp) * (ebeam + kMp) - ebeam * ebeam * c * c;
  return 2. * kMp * ebeam * (ebeam + kMp) * c / denominator;
}

Counts FillHmsOnly(const char* filename, double ebeam, double pcentral,
                   TH1D& histogram) {
  Counts counts;
  TFile file(filename, "READ");
  auto* tree = dynamic_cast<TTree*>(file.Get("T"));
  if (!tree) {
    std::cerr << "Cannot read T from " << filename << '\n';
    return counts;
  }
  counts.entries = tree->GetEntries();

  double edtm = 0., ntrack = 0.;
  std::array<double, kMax> p{}, px{}, py{}, pz{};
  tree->SetBranchStatus("*", 0);
  for (const char* branch : {"T.hms.hEDTM_tdcTimeRaw", "H.dc.ntrack",
                             "H.gtr.p", "H.gtr.px", "H.gtr.py", "H.gtr.pz"})
    tree->SetBranchStatus(branch, 1);
  tree->SetBranchAddress("T.hms.hEDTM_tdcTimeRaw", &edtm);
  tree->SetBranchAddress("H.dc.ntrack", &ntrack);
  tree->SetBranchAddress("H.gtr.p", p.data());
  tree->SetBranchAddress("H.gtr.px", px.data());
  tree->SetBranchAddress("H.gtr.py", py.data());
  tree->SetBranchAddress("H.gtr.pz", pz.data());

  for (Long64_t entry = 0; entry < tree->GetEntries(); ++entry) {
    tree->GetEntry(entry);
    if (edtm != 0.) continue;
    ++counts.ordinary;
    if (ntrack != 1. || p[0] <= 0.) continue;
    ++counts.one_track;
    const double norm = std::sqrt(px[0]*px[0] + py[0]*py[0] + pz[0]*pz[0]);
    if (norm <= 0.) continue;
    const double theta = std::acos(std::clamp(pz[0] / norm, -1., 1.));
    const double dpp = 100. *
        (p[0] - ProtonMomentumFromTheta(ebeam, theta)) / pcentral;
    if (!std::isfinite(dpp)) continue;
    histogram.Fill(dpp);
    if (dpp >= histogram.GetXaxis()->GetXmin() &&
        dpp < histogram.GetXaxis()->GetXmax()) ++counts.in_range;
    if (dpp < -3.) ++counts.below_minus3;
    if (dpp < -5.) ++counts.below_minus5;
  }
  return counts;
}

void DrawPanel(TPad* pad, TH1D& histogram, const Counts& counts,
               const char* title, bool log_y = true, double y_max = 0.) {
  pad->cd();
  pad->SetLogy(log_y);
  pad->SetLeftMargin(.12); pad->SetRightMargin(.035);
  pad->SetBottomMargin(.13); pad->SetTopMargin(.10);
  histogram.SetMinimum(log_y ? .7 : 0.);
  histogram.SetMaximum(y_max > 0. ? y_max :
                       (log_y ? 3.5 : 1.18) * histogram.GetMaximum());
  histogram.SetLineColor(kRed + 1);
  histogram.SetMarkerColor(kRed + 1);
  histogram.SetMarkerStyle(24);
  histogram.SetMarkerSize(.72);
  histogram.GetXaxis()->SetTitle(
      "#delta p_{p} = 100[p_{p}-p_{p}(#theta_{p})]/p_{0}  (%)");
  histogram.GetYaxis()->SetTitle(
      Form("events / %.3f%%", histogram.GetXaxis()->GetBinWidth(1)));
  histogram.Draw("E1");

  TLatex text;
  text.SetNDC(); text.SetTextSize(.042);
  text.DrawLatex(.15, .92, title);
  text.SetTextSize(.033);
  text.DrawLatex(.15, .855, Form("%lld ordinary events; %lld with exactly one HMS track",
                                counts.ordinary, counts.one_track));
  text.DrawLatex(.15, .805, "No NPS reconstruction, energy, position, or timing requirement");
}
}

void compare_nps_runs6151_2875_dpp_hms_only(
    const char* run6151,
    const char* run2875,
    const char* output = "nps_runs6151_2875_dpp_hms_only") {
  gStyle->SetOptStat(0);
  gStyle->SetEndErrorSize(0);

  TH1D h6151("h6151_hms_only", "", 120, -30., 15.);
  TH1D h2875("h2875_hms_only", "", 120, -30., 15.);
  const Counts c6151 = FillHmsOnly(run6151, 6.37042664, 2.639, h6151);
  const Counts c2875 = FillHmsOnly(run2875, 10.53941793, 4.0872, h2875);

  TCanvas canvas("hms_only", "Raw HMS delta-p comparison", 1500, 670);
  canvas.Divide(2, 1, .002, .002);
  DrawPanel((TPad*)canvas.cd(1), h6151, c6151,
            "Run 6151: E_{beam}=6.370 GeV, Q^{2}#approx3.74 GeV^{2}");
  DrawPanel((TPad*)canvas.cd(2), h2875, c2875,
            "Run 2875: E_{beam}=10.539 GeV, Q^{2}#approx6.51 GeV^{2}");
  canvas.SaveAs((std::string(output) + ".png").c_str());
  canvas.SaveAs((std::string(output) + ".pdf").c_str());

  TFile histogram_file((std::string(output) + ".root").c_str(), "RECREATE");
  h6151.Write("data_run6151");
  h2875.Write("data_run2875");
  histogram_file.Close();

  std::ofstream summary(std::string(output) + "_summary.csv");
  summary << "run,tree_entries,ordinary_events,exactly_one_hms_track,"
             "histogram_range,delta_pp_below_minus3_pct,delta_pp_below_minus5_pct,"
             "fraction_below_minus3,fraction_below_minus5\n";
  auto write = [&](int run, const Counts& counts) {
    summary << std::setprecision(10) << run << ',' << counts.entries << ','
            << counts.ordinary << ',' << counts.one_track << ',' << counts.in_range
            << ',' << counts.below_minus3 << ',' << counts.below_minus5 << ','
            << (counts.one_track ? double(counts.below_minus3)/counts.one_track : 0.) << ','
            << (counts.one_track ? double(counts.below_minus5)/counts.one_track : 0.) << '\n';
    std::cout << "run=" << run << " ordinary=" << counts.ordinary
              << " one_track=" << counts.one_track
              << " below_-3=" << counts.below_minus3
              << " below_-5=" << counts.below_minus5 << '\n';
  };
  write(6151, c6151);
  write(2875, c2875);
}

void compare_nps_runs6151_2875_dpp_hms_only_linear(
    const char* run6151,
    const char* run2875,
    const char* output = "nps_runs6151_2875_dpp_hms_only_linear") {
  gStyle->SetOptStat(0);
  gStyle->SetEndErrorSize(0);

  // Match the narrow elastic-core presentation requested for direct
  // comparison: identical samples, axes, and 0.04%-wide bins in both panels.
  TH1D h6151("h6151_hms_only_linear", "", 100, -2., 2.);
  TH1D h2875("h2875_hms_only_linear", "", 100, -2., 2.);
  const Counts c6151 = FillHmsOnly(run6151, 6.37042664, 2.639, h6151);
  const Counts c2875 = FillHmsOnly(run2875, 10.53941793, 4.0872, h2875);

  TCanvas canvas("hms_only_linear", "Raw HMS delta-p comparison", 1500, 670);
  canvas.Divide(2, 1, .002, .002);
  const double common_y_max = 1.18 * std::max(h6151.GetMaximum(), h2875.GetMaximum());
  DrawPanel((TPad*)canvas.cd(1), h6151, c6151,
            "Run 6151: E_{beam}=6.370 GeV, Q^{2}#approx3.74 GeV^{2}",
            false, common_y_max);
  DrawPanel((TPad*)canvas.cd(2), h2875, c2875,
            "Run 2875: E_{beam}=10.539 GeV, Q^{2}#approx6.51 GeV^{2}",
            false, common_y_max);
  canvas.SaveAs((std::string(output) + ".png").c_str());
  canvas.SaveAs((std::string(output) + ".pdf").c_str());

  std::ofstream summary(std::string(output) + "_summary.csv");
  summary << "run,tree_entries,ordinary_events,exactly_one_hms_track,"
             "events_in_minus2_to_plus2,fraction_in_minus2_to_plus2\n";
  auto write = [&](int run, const Counts& counts) {
    summary << std::setprecision(10) << run << ',' << counts.entries << ','
            << counts.ordinary << ',' << counts.one_track << ',' << counts.in_range
            << ',' << (counts.one_track ? double(counts.in_range)/counts.one_track : 0.)
            << '\n';
    std::cout << "run=" << run << " ordinary=" << counts.ordinary
              << " one_track=" << counts.one_track
              << " in_-2_to_2=" << counts.in_range << '\n';
  };
  write(6151, c6151);
  write(2875, c2875);
}

void compare_nps_runs6151_2875_dpp_hms_only_minus10_to_2(
    const char* run6151,
    const char* run2875,
    const char* output = "nps_runs6151_2875_dpp_hms_only_minus10_to_2") {
  gStyle->SetOptStat(0);
  gStyle->SetEndErrorSize(0);

  TH1D h6151("h6151_hms_only_minus10_to_2", "", 120, -10., 2.);
  TH1D h2875("h2875_hms_only_minus10_to_2", "", 120, -10., 2.);
  const Counts c6151 = FillHmsOnly(run6151, 6.37042664, 2.639, h6151);
  const Counts c2875 = FillHmsOnly(run2875, 10.53941793, 4.0872, h2875);

  TCanvas canvas("hms_only_minus10_to_2", "Raw HMS delta-p comparison", 1500, 670);
  canvas.Divide(2, 1, .002, .002);
  const double common_y_max = 1.18 * std::max(h6151.GetMaximum(), h2875.GetMaximum());
  DrawPanel((TPad*)canvas.cd(1), h6151, c6151,
            "Run 6151: E_{beam}=6.370 GeV, Q^{2}#approx3.74 GeV^{2}",
            false, common_y_max);
  DrawPanel((TPad*)canvas.cd(2), h2875, c2875,
            "Run 2875: E_{beam}=10.539 GeV, Q^{2}#approx6.51 GeV^{2}",
            false, common_y_max);
  canvas.SaveAs((std::string(output) + ".png").c_str());
  canvas.SaveAs((std::string(output) + ".pdf").c_str());

  std::ofstream summary(std::string(output) + "_summary.csv");
  summary << "run,tree_entries,ordinary_events,exactly_one_hms_track,"
             "events_in_minus10_to_plus2,fraction_in_minus10_to_plus2\n";
  auto write = [&](int run, const Counts& counts) {
    summary << std::setprecision(10) << run << ',' << counts.entries << ','
            << counts.ordinary << ',' << counts.one_track << ',' << counts.in_range
            << ',' << (counts.one_track ? double(counts.in_range)/counts.one_track : 0.)
            << '\n';
    std::cout << "run=" << run << " ordinary=" << counts.ordinary
              << " one_track=" << counts.one_track
              << " in_-10_to_2=" << counts.in_range << '\n';
  };
  write(6151, c6151);
  write(2875, c2875);
}

void compare_nps_runs6151_2875_dpp_hms_only_full_acceptance(
    const char* run6151,
    const char* run2875,
    const char* output = "nps_runs6151_2875_dpp_hms_only_full_acceptance") {
  gStyle->SetOptStat(0);
  gStyle->SetEndErrorSize(0);

  // Nominal full HMS fractional-momentum acceptance, shown with the same
  // 0.1%-wide bins and linear normalization in both panels.
  TH1D h6151("h6151_hms_only_full_acceptance", "", 200, -10., 10.);
  TH1D h2875("h2875_hms_only_full_acceptance", "", 200, -10., 10.);
  const Counts c6151 = FillHmsOnly(run6151, 6.37042664, 2.639, h6151);
  const Counts c2875 = FillHmsOnly(run2875, 10.53941793, 4.0872, h2875);

  TCanvas canvas("hms_only_full_acceptance", "Raw HMS delta-p comparison", 1500, 670);
  canvas.Divide(2, 1, .002, .002);
  const double common_y_max = 1.18 * std::max(h6151.GetMaximum(), h2875.GetMaximum());
  DrawPanel((TPad*)canvas.cd(1), h6151, c6151,
            "Run 6151: E_{beam}=6.370 GeV, Q^{2}#approx3.74 GeV^{2}",
            false, common_y_max);
  DrawPanel((TPad*)canvas.cd(2), h2875, c2875,
            "Run 2875: E_{beam}=10.539 GeV, Q^{2}#approx6.51 GeV^{2}",
            false, common_y_max);
  canvas.SaveAs((std::string(output) + ".png").c_str());
  canvas.SaveAs((std::string(output) + ".pdf").c_str());

  TFile histogram_file((std::string(output) + ".root").c_str(), "RECREATE");
  h6151.Write("data_run6151");
  h2875.Write("data_run2875");
  histogram_file.Close();

  std::ofstream summary(std::string(output) + "_summary.csv");
  summary << "run,tree_entries,ordinary_events,exactly_one_hms_track,"
             "events_in_minus10_to_plus10,fraction_in_minus10_to_plus10\n";
  auto write = [&](int run, const Counts& counts) {
    summary << std::setprecision(10) << run << ',' << counts.entries << ','
            << counts.ordinary << ',' << counts.one_track << ',' << counts.in_range
            << ',' << (counts.one_track ? double(counts.in_range)/counts.one_track : 0.)
            << '\n';
    std::cout << "run=" << run << " ordinary=" << counts.ordinary
              << " one_track=" << counts.one_track
              << " in_-10_to_10=" << counts.in_range << '\n';
  };
  write(6151, c6151);
  write(2875, c2875);
}
