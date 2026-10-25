#include <TCanvas.h>
#include <TFile.h>
#include <TF1.h>
#include <TH1D.h>
#include <TStyle.h>
#include <TTree.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

// Measure the VTP-cluster threshold efficiency for proton-tagged elastic
// events.  The elastic core is selected using the offline NPS energy residual;
// the efficiency numerator is then determined only from the maximum VTP
// cluster energy.  This keeps the selection independent of the quantity whose
// threshold is being scanned.
void analyze_nps_elastic_efficiency_curve(
    const char* input,
    const char* output_prefix = "nps_run6151_elastic_efficiency",
    double beam_energy = 6.37042664,
    double nominal_elastic_energy = 4.598,
    double threshold_step = 0.05) {
  constexpr int kMax = 4096;
  constexpr double kProtonMass = 0.9382720813;

  TFile file(input, "READ");
  auto* tree = dynamic_cast<TTree*>(file.Get("T"));
  if (file.IsZombie() || !tree) {
    std::cerr << "Cannot read tree T from " << input << std::endl;
    return;
  }

  double edtm = 0.0, ntrack = 0.0;
  int nclus = 0, nvtp = 0;
  std::array<double, kMax> p{}, dp{}, clus_e{}, vtp_e{};
  tree->SetBranchAddress("T.hms.hEDTM_tdcTimeRaw", &edtm);
  tree->SetBranchAddress("H.dc.ntrack", &ntrack);
  tree->SetBranchAddress("H.gtr.p", p.data());
  tree->SetBranchAddress("H.gtr.dp", dp.data());
  tree->SetBranchAddress("Ndata.NPS.cal.clusE", &nclus);
  tree->SetBranchAddress("NPS.cal.clusE", clus_e.data());
  tree->SetBranchAddress("Ndata.NPS.cal.vtpClusE", &nvtp);
  tree->SetBranchAddress("NPS.cal.vtpClusE", vtp_e.data());

  TH1D residual("residual", "Elastic energy residual;E_{NPS}^{offline}-E_{e}^{expected} (GeV);events",
                240, -2.0, 1.0);
  const Long64_t entries = tree->GetEntries();
  for (Long64_t i = 0; i < entries; ++i) {
    tree->GetEntry(i);
    if (edtm != 0.0 || ntrack != 1.0 || std::abs(dp[0]) >= 8.0 || p[0] <= 0.0 ||
        nclus <= 0 || nclus > kMax) continue;
    const int imax = std::distance(
        clus_e.begin(), std::max_element(clus_e.begin(), clus_e.begin() + nclus));
    const double expected = beam_energy + kProtonMass -
        std::sqrt(p[0] * p[0] + kProtonMass * kProtonMass);
    residual.Fill(clus_e[imax] - expected);
  }

  const double peak = residual.GetBinCenter(residual.GetMaximumBin());
  TF1 first_fit("first_fit", "gaus", peak - 0.35, peak + 0.35);
  residual.Fit(&first_fit, "RQ0");
  double mean = first_fit.GetParameter(1);
  double sigma = std::abs(first_fit.GetParameter(2));
  TF1 core_fit("core_fit", "gaus", mean - 2.0 * sigma, mean + 2.0 * sigma);
  residual.Fit(&core_fit, "RQ0");
  mean = core_fit.GetParameter(1);
  sigma = std::abs(core_fit.GetParameter(2));
  const double low = mean - 3.0 * sigma;
  const double high = mean + 3.0 * sigma;

  std::vector<double> elastic_vtp_maxima;
  long long elastic_core_without_vtp = 0;
  for (Long64_t i = 0; i < entries; ++i) {
    tree->GetEntry(i);
    if (edtm != 0.0 || ntrack != 1.0 || std::abs(dp[0]) >= 8.0 || p[0] <= 0.0 ||
        nclus <= 0 || nclus > kMax) continue;
    const int imax = std::distance(
        clus_e.begin(), std::max_element(clus_e.begin(), clus_e.begin() + nclus));
    const double expected = beam_energy + kProtonMass -
        std::sqrt(p[0] * p[0] + kProtonMass * kProtonMass);
    const double offline_residual = clus_e[imax] - expected;
    if (offline_residual < low || offline_residual > high) continue;
    if (nvtp <= 0 || nvtp > kMax) {
      ++elastic_core_without_vtp;
      elastic_vtp_maxima.push_back(0.0);
      continue;
    }
    elastic_vtp_maxima.push_back(
        *std::max_element(vtp_e.begin(), vtp_e.begin() + nvtp) / 1000.0);
  }

  const long long denominator = elastic_vtp_maxima.size();
  std::ofstream csv(std::string(output_prefix) + ".csv");
  csv << "threshold_nominal_GeV,threshold_fraction_of_run6151_elastic_energy,"
         "elastic_core_events,passing_events,efficiency,binomial_error\n";
  csv << std::fixed << std::setprecision(8);
  for (double threshold = 0.0; threshold <= 4.500001; threshold += threshold_step) {
    const long long passing = std::count_if(
        elastic_vtp_maxima.begin(), elastic_vtp_maxima.end(),
        [threshold](double energy) { return energy >= threshold; });
    const double efficiency = denominator ? double(passing) / denominator : 0.0;
    const double error = denominator
        ? std::sqrt(efficiency * (1.0 - efficiency) / denominator) : 0.0;
    csv << threshold << ',' << threshold / nominal_elastic_energy << ','
        << denominator << ',' << passing << ',' << efficiency << ',' << error << '\n';
  }

  std::ofstream summary(std::string(output_prefix) + "_summary.txt");
  summary << std::fixed << std::setprecision(8)
          << "input=" << input << '\n'
          << "entries=" << entries << '\n'
          << "beam_energy_GeV=" << beam_energy << '\n'
          << "nominal_elastic_energy_GeV=" << nominal_elastic_energy << '\n'
          << "elastic_residual_fit_mean_GeV=" << mean << '\n'
          << "elastic_residual_fit_sigma_GeV=" << sigma << '\n'
          << "elastic_core_low_GeV=" << low << '\n'
          << "elastic_core_high_GeV=" << high << '\n'
          << "elastic_core_events=" << denominator << '\n'
          << "elastic_core_without_vtp=" << elastic_core_without_vtp << '\n'
          << "efficiency_denominator=all offline-selected elastic-core events, including missing VTP as failures\n";

  gStyle->SetOptStat(0);
  TCanvas canvas("canvas", "Run 6151 elastic threshold efficiency", 900, 700);
  residual.Draw();
  core_fit.SetLineColor(kRed + 1);
  core_fit.Draw("SAME");
  canvas.SaveAs((std::string(output_prefix) + "_selection.png").c_str());
  canvas.SaveAs((std::string(output_prefix) + "_selection.pdf").c_str());

  std::cout << "Elastic core events: " << denominator
            << ", missing VTP: " << elastic_core_without_vtp << std::endl;
  std::cout << "Wrote " << output_prefix << ".csv" << std::endl;
}
