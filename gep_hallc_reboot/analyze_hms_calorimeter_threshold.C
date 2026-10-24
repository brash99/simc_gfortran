#include <TFile.h>
#include <TTree.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

void analyze_hms_calorimeter_threshold(
    const char* input,
    const char* output="hms_calorimeter_proton_thresholds.csv",
    Long64_t max_entries=-1) {
  constexpr int kMax = 4096;
  constexpr double kBeamEnergy = 6.37042664;
  constexpr double kProtonMass = 0.9382720813;
  constexpr double kCoreMean = -0.225806;
  constexpr double kCoreSigma = 0.202053;

  TFile file(input);
  auto* tree = static_cast<TTree*>(file.Get("T"));
  if (!tree) { std::cerr << "Missing T tree\n"; return; }

  double edtm = 0., ntrack = 0., etot = 0., epr = 0.;
  int nclus = 0;
  std::array<double,kMax> p{}, dp{}, clus_e{};
  tree->SetBranchAddress("T.hms.hEDTM_tdcTimeRaw", &edtm);
  tree->SetBranchAddress("H.dc.ntrack", &ntrack);
  tree->SetBranchAddress("H.gtr.p", p.data());
  tree->SetBranchAddress("H.gtr.dp", dp.data());
  tree->SetBranchAddress("Ndata.NPS.cal.clusE", &nclus);
  tree->SetBranchAddress("NPS.cal.clusE", clus_e.data());
  tree->SetBranchAddress("H.cal.etot", &etot);
  tree->SetBranchAddress("H.cal.1pr.eplane", &epr);

  std::vector<double> total_energy, preshower_energy;
  const Long64_t entries = max_entries > 0
    ? std::min(max_entries, tree->GetEntries()) : tree->GetEntries();
  for (Long64_t i = 0; i < entries; ++i) {
    tree->GetEntry(i);
    if (edtm != 0. || ntrack != 1. || std::abs(dp[0]) >= 8. || p[0] <= 0. ||
        nclus <= 0 || nclus > kMax) continue;
    const double max_nps = *std::max_element(clus_e.begin(), clus_e.begin()+nclus);
    const double expected = kBeamEnergy+kProtonMass-
      std::sqrt(p[0]*p[0]+kProtonMass*kProtonMass);
    const double residual = max_nps-expected;
    if (residual < kCoreMean-3*kCoreSigma || residual > kCoreMean+3*kCoreSigma)
      continue;
    total_energy.push_back(etot);
    preshower_energy.push_back(epr);
  }
  std::sort(total_energy.begin(), total_energy.end());
  std::sort(preshower_energy.begin(), preshower_energy.end());

  std::ofstream csv(output);
  csv << "observable,target_proton_efficiency,threshold_GeV,passing,total,measured_efficiency\n"
      << std::setprecision(15);
  const std::array<double,6> targets{{0.90, 0.95, 0.98, 0.99, 0.995, 0.997}};
  auto write_scan = [&](const char* name, const std::vector<double>& values) {
    for (double target : targets) {
      const size_t index = std::min(values.size()-1,
        static_cast<size_t>((1.-target)*(values.size()-1)));
      const double threshold = values[index];
      const long passing = std::count_if(values.begin(), values.end(),
        [&](double value) { return value >= threshold; });
      csv << name << ',' << target << ',' << threshold << ',' << passing << ','
          << values.size() << ',' << double(passing)/values.size() << '\n';
      std::cout << name << " target=" << target << " threshold=" << threshold
                << " GeV measured=" << double(passing)/values.size() << '\n';
    }
  };
  if (total_energy.empty()) { std::cerr << "No selected elastic events\n"; return; }
  write_scan("H.cal.etot", total_energy);
  write_scan("H.cal.1pr.eplane", preshower_energy);
}
