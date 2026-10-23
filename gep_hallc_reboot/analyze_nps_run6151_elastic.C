#include <TCanvas.h>
#include <TFile.h>
#include <TF1.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TLine.h>
#include <TProfile2D.h>
#include <TStyle.h>
#include <TTree.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

// Proton-tagged elastic-candidate study, initially developed for run 6151.
// The core energy window is determined by an iterative Gaussian fit to the
// E_NPS - E_expected(p_HMS) peak; it is not an externally assumed resolution.
void analyze_nps_run6151_elastic(
    const char* input,
    const char* output_prefix = "nps_run6151_100k_elastic",
    double beam_energy = 6.37042664,
    double nominal_elastic_energy = 4.598) {
  constexpr int kMax = 4096;
  constexpr double kProtonMass = 0.9382720813;
  const double threshold_76 = 0.761 * nominal_elastic_energy;
  const double threshold_805 = 0.805 * nominal_elastic_energy;

  TFile file(input, "READ");
  auto* tree = dynamic_cast<TTree*>(file.Get("T"));
  if (file.IsZombie() || !tree) {
    std::cerr << "Cannot read tree T from " << input << std::endl;
    return;
  }

  double edtm = 0.0, ntrack = 0.0;
  int nclus = 0, nvtp = 0;
  std::array<double, kMax> p{}, dp{}, clus_e{}, clus_t{}, clus_x{}, clus_y{}, vtp_e{};
  tree->SetBranchAddress("T.hms.hEDTM_tdcTimeRaw", &edtm);
  tree->SetBranchAddress("H.dc.ntrack", &ntrack);
  tree->SetBranchAddress("H.gtr.p", p.data());
  tree->SetBranchAddress("H.gtr.dp", dp.data());
  tree->SetBranchAddress("Ndata.NPS.cal.clusE", &nclus);
  tree->SetBranchAddress("NPS.cal.clusE", clus_e.data());
  tree->SetBranchAddress("NPS.cal.clusT", clus_t.data());
  tree->SetBranchAddress("NPS.cal.clusX", clus_x.data());
  tree->SetBranchAddress("NPS.cal.clusY", clus_y.data());
  tree->SetBranchAddress("Ndata.NPS.cal.vtpClusE", &nvtp);
  tree->SetBranchAddress("NPS.cal.vtpClusE", vtp_e.data());

  TH1D h_dp("h_dp", "One-track ordinary events;HMS #delta (%);events", 160, -20, 20);
  TH1D h_expected("h_expected", "Proton-predicted electron energy;E_{e}^{expected} (GeV);events", 160, .7 * nominal_elastic_energy, 1.2 * nominal_elastic_energy);
  TH1D h_residual("h_residual", "NPS elastic energy residual;E_{NPS}-E_{e}^{expected} (GeV);events", 240, -2.0, 1.0);
  TH1D h_time("h_time", "Time of maximum-energy NPS cluster;cluster time (ns);events", 200, 100, 300);
  TH2D h_energy("h_energy", "Proton prediction versus NPS;E_{e}^{expected} (GeV);E_{NPS} (GeV)", 120, .7 * nominal_elastic_energy, 1.2 * nominal_elastic_energy, 120, .6 * nominal_elastic_energy, 1.2 * nominal_elastic_energy);
  TProfile2D h_residual_xy("h_residual_xy", "Mean energy residual across NPS;x (cm);y (cm);mean residual (GeV)", 30, -35, 35, 36, -42, 42);

  long long ordinary = 0, one_track = 0, accepted_track = 0, with_cluster = 0;
  const Long64_t entries = tree->GetEntries();
  for (Long64_t i = 0; i < entries; ++i) {
    tree->GetEntry(i);
    if (edtm != 0.0) continue;
    ++ordinary;
    if (ntrack != 1.0) continue;
    ++one_track;
    h_dp.Fill(dp[0]);
    if (std::abs(dp[0]) >= 8.0 || p[0] <= 0.0) continue;
    ++accepted_track;
    if (nclus <= 0 || nclus > kMax) continue;
    ++with_cluster;
    const int imax = std::distance(clus_e.begin(),
        std::max_element(clus_e.begin(), clus_e.begin() + nclus));
    const double expected = beam_energy + kProtonMass
        - std::sqrt(p[0] * p[0] + kProtonMass * kProtonMass);
    const double residual = clus_e[imax] - expected;
    h_expected.Fill(expected);
    h_residual.Fill(residual);
    h_time.Fill(clus_t[imax]);
    h_energy.Fill(expected, clus_e[imax]);
    h_residual_xy.Fill(clus_x[imax], clus_y[imax], residual);
  }

  // Locate the peak, fit a generous core, then refit within +/-2 sigma.
  const double peak = h_residual.GetBinCenter(h_residual.GetMaximumBin());
  TF1 core1("core1", "gaus", peak - 0.35, peak + 0.35);
  h_residual.Fit(&core1, "RQ0");
  double mean = core1.GetParameter(1);
  double sigma = std::abs(core1.GetParameter(2));
  TF1 core2("core2", "gaus", mean - 2.0 * sigma, mean + 2.0 * sigma);
  h_residual.Fit(&core2, "RQ0");
  mean = core2.GetParameter(1);
  sigma = std::abs(core2.GetParameter(2));
  const double low = mean - 3.0 * sigma;
  const double high = mean + 3.0 * sigma;

  long long in_core = 0;
  long long above_35_all = 0, above_37_all = 0;
  long long above_35_core = 0, above_37_core = 0, core_with_vtp = 0;
  for (Long64_t i = 0; i < entries; ++i) {
    tree->GetEntry(i);
    if (edtm != 0.0 || ntrack != 1.0 || std::abs(dp[0]) >= 8.0 || p[0] <= 0.0 ||
        nclus <= 0 || nclus > kMax) continue;
    const int imax = std::distance(clus_e.begin(),
        std::max_element(clus_e.begin(), clus_e.begin() + nclus));
    const double expected = beam_energy + kProtonMass
        - std::sqrt(p[0] * p[0] + kProtonMass * kProtonMass);
    const double residual = clus_e[imax] - expected;
    const double max_vtp = nvtp > 0 && nvtp <= kMax
        ? *std::max_element(vtp_e.begin(), vtp_e.begin() + nvtp) / 1000.0 : 0.0;
    if (max_vtp >= threshold_76) ++above_35_all;
    if (max_vtp >= threshold_805) ++above_37_all;
    if (residual >= low && residual <= high) {
      ++in_core;
      if (nvtp > 0 && nvtp <= kMax) ++core_with_vtp;
      if (max_vtp >= threshold_76) ++above_35_core;
      if (max_vtp >= threshold_805) ++above_37_core;
    }
  }

  std::ofstream out(std::string(output_prefix) + "_summary.txt");
  out << std::fixed << std::setprecision(6)
      << "input=" << input << '\n'
      << "beam_energy_GeV=" << beam_energy << '\n'
      << "nominal_elastic_energy_GeV=" << nominal_elastic_energy << '\n'
      << "threshold_76p1_percent_GeV=" << threshold_76 << '\n'
      << "threshold_80p5_percent_GeV=" << threshold_805 << '\n'
      << "ordinary_events=" << ordinary << '\n'
      << "one_hms_track=" << one_track << '\n'
      << "one_track_abs_dp_lt_8=" << accepted_track << '\n'
      << "accepted_track_with_nps_cluster=" << with_cluster << '\n'
      << "elastic_residual_definition=E_NPS_max-(Ebeam+Mp-sqrt(p_HMS^2+Mp^2))\n"
      << "elastic_core_fit_mean_GeV=" << mean << '\n'
      << "elastic_core_fit_sigma_GeV=" << sigma << '\n'
      << "elastic_core_low_GeV=" << low << '\n'
      << "elastic_core_high_GeV=" << high << '\n'
      << "elastic_core_events=" << in_core << '\n'
      << "elastic_core_with_vtp=" << core_with_vtp << '\n'
      << "all_track_cluster_VTP_E_ge_76p1_percent=" << above_35_all << '\n'
      << "all_track_cluster_VTP_E_ge_80p5_percent=" << above_37_all << '\n'
      << "elastic_core_VTP_E_ge_76p1_percent=" << above_35_core << '\n'
      << "elastic_core_VTP_E_ge_80p5_percent=" << above_37_core << '\n'
      << "elastic_core_retention_76p1_percent=" << (in_core ? double(above_35_core) / in_core : 0.0) << '\n'
      << "elastic_core_retention_80p5_percent=" << (in_core ? double(above_37_core) / in_core : 0.0) << '\n';

  gStyle->SetOptStat(0);
  TCanvas canvas("canvas", "NPS elastic selection", 1500, 1000);
  canvas.Divide(3, 2);
  canvas.cd(1); h_dp.Draw();
  canvas.cd(2); h_expected.Draw();
  canvas.cd(3); h_energy.Draw("COLZ");
  canvas.cd(4); h_residual.Draw(); core2.SetLineColor(kRed + 1); core2.Draw("SAME");
  TLine lo(low, 0, low, h_residual.GetMaximum()); lo.SetLineStyle(2); lo.Draw();
  TLine hi(high, 0, high, h_residual.GetMaximum()); hi.SetLineStyle(2); hi.Draw();
  canvas.cd(5); h_time.Draw();
  canvas.cd(6); h_residual_xy.Draw("COLZ");
  canvas.SaveAs((std::string(output_prefix) + "_diagnostics.png").c_str());
  canvas.SaveAs((std::string(output_prefix) + "_diagnostics.pdf").c_str());
}
