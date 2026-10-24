#include <TFile.h>
#include <TF1.h>
#include <TH1D.h>
#include <TTree.h>

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
struct ProtonTruth {
  long long event_id;
  double energy;
  double px;
  double py;
  double pz;
};
}

void analyze_run6151_elastic_template(
    const char *root_name,
    const char *truth_name,
    const char *output_name = "run6151_elastic_template.root") {
  std::ifstream truth_file(truth_name);
  std::vector<ProtonTruth> truth;
  std::string line;
  while (std::getline(truth_file, line)) {
    if (line.empty() || line[0] == '#' || line.rfind("event_id", 0) == 0)
      continue;
    for (char &c : line)
      if (c == ',') c = ' ';
    std::istringstream row(line);
    ProtonTruth item{};
    if (row >> item.event_id >> item.energy >> item.px >> item.py >> item.pz)
      truth.push_back(item);
  }

  TFile input(root_name, "READ");
  auto *tree = dynamic_cast<TTree *>(input.Get("t_dvcs"));
  if (!tree || static_cast<Long64_t>(truth.size()) != tree->GetEntries()) {
    std::cerr << "entry mismatch: geant=" << (tree ? tree->GetEntries() : -1)
              << " truth=" << truth.size() << '\n';
    return;
  }

  double edep[1080]{};
  double weight = 0.;
  tree->SetBranchAddress("edep", edep);
  tree->SetBranchAddress("psf", &weight);

  TH1D residual("elastic_residual_raw",
      "Radiative elastic SIMC + NPS Geant4;E_{NPS}-E_{e}^{p-predicted} (GeV);Weighted events",
      240, -4., 2.);
  TH1D response("elastic_response",
      "NPS Geant4 response;E_{NPS}/E_{e}^{p-predicted};Weighted events",
      200, 0., 1.25);
  double sum_w = 0., sum_residual = 0., sum_residual2 = 0.;
  constexpr double proton_mass = 0.9382720813;
  for (Long64_t entry = 0; entry < tree->GetEntries(); ++entry) {
    tree->GetEntry(entry);
    double deposit_gev = 0.;
    for (double block : edep) deposit_gev += block / 1000.;
    const double predicted_e = 6.37042664 + proton_mass - truth[entry].energy;
    const double value = deposit_gev - predicted_e;
    residual.Fill(value, weight);
    response.Fill(predicted_e > 0. ? deposit_gev / predicted_e : 0., weight);
    sum_w += weight;
    sum_residual += weight * value;
    sum_residual2 += weight * value * value;
  }
  const double mean = sum_residual / sum_w;
  const double rms = std::sqrt(sum_residual2 / sum_w - mean * mean);
  const double mode = residual.GetBinCenter(residual.GetMaximumBin());
  TF1 core_fit("core_fit", "gaus", mode - 0.35, mode + 0.35);
  residual.Fit(&core_fit, "QNR");
  std::cout << std::setprecision(12)
            << "entries=" << tree->GetEntries() << '\n'
            << "truth_rows=" << truth.size() << '\n'
            << "weighted_residual_mean_GeV=" << mean << '\n'
            << "weighted_residual_rms_GeV=" << rms << '\n'
            << "residual_mode_GeV=" << mode << '\n'
            << "core_fit_mean_GeV=" << core_fit.GetParameter(1) << '\n'
            << "core_fit_sigma_GeV=" << core_fit.GetParameter(2) << '\n'
            << "weighted_response_mean=" << response.GetMean() << '\n'
            << "first_event_id=" << truth.front().event_id << '\n'
            << "last_event_id=" << truth.back().event_id << '\n';

  TFile output(output_name, "RECREATE");
  residual.Write();
  response.Write();
  output.Close();
}
