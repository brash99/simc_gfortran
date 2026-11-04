#include <TFile.h>
#include <TTree.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

// Measure the face-integrated NPS VTP cluster rate versus an offline threshold
// in unbiased 1-us EDTM windows.  The Poisson conversion is reliable only when
// a useful number of empty windows remains; saturated points are flagged.
void analyze_nps_edtm_threshold_curve(
    const char* input,
    const char* output_csv,
    int run,
    double minimum_current_uA = 25.0,
    double maximum_current_uA = 35.0,
    double threshold_min_GeV = 0.5,
    double threshold_max_GeV = 5.0,
    double threshold_step_GeV = 0.1) {
  constexpr int kMaxObjects = 4096;
  constexpr double kWindowSeconds = 1.0e-6;
  constexpr long long kMinimumEmptyWindows = 25;

  TFile file(input, "READ");
  auto* events = dynamic_cast<TTree*>(file.Get("T"));
  if (file.IsZombie() || !events) {
    std::cerr << "Cannot read T from " << input << std::endl;
    return;
  }

  double edtm_time_raw = 0.0;
  double event_current_uA = 0.0;
  int n_vtp = 0;
  std::array<double, kMaxObjects> vtp_energy{};
  events->SetBranchAddress("T.hms.hEDTM_tdcTimeRaw", &edtm_time_raw);
  events->SetBranchAddress("H.BCM4A.scalerCurrent", &event_current_uA);
  events->SetBranchAddress("Ndata.NPS.cal.vtpClusE", &n_vtp);
  events->SetBranchAddress("NPS.cal.vtpClusE", vtp_energy.data());

  long long all_edtm = 0;
  double current_sum = 0.0;
  std::vector<double> maximum_cluster_energy_GeV;
  for (Long64_t entry = 0; entry < events->GetEntries(); ++entry) {
    events->GetEntry(entry);
    if (edtm_time_raw == 0.0)
      continue;
    ++all_edtm;
    if (event_current_uA < minimum_current_uA ||
        event_current_uA >= maximum_current_uA)
      continue;
    if (n_vtp < 0 || n_vtp > kMaxObjects) {
      std::cerr << "Invalid VTP object count at entry " << entry << std::endl;
      return;
    }
    current_sum += event_current_uA;
    const double maximum = n_vtp > 0
        ? *std::max_element(vtp_energy.begin(), vtp_energy.begin() + n_vtp) /
              1000.0
        : 0.0;
    maximum_cluster_energy_GeV.push_back(maximum);
  }

  const long long selected = maximum_cluster_energy_GeV.size();
  std::sort(maximum_cluster_energy_GeV.begin(), maximum_cluster_energy_GeV.end());

  std::ofstream out(output_csv);
  out << "run,threshold_GeV,all_edtm_windows,selected_edtm_windows,"
         "windows_above,empty_windows,occupancy,poisson_rate_Hz,"
         "stat_error_Hz,mean_current_uA,saturated\n";
  out << std::fixed << std::setprecision(9);

  for (double threshold = threshold_min_GeV;
       threshold <= threshold_max_GeV + 0.5 * threshold_step_GeV;
       threshold += threshold_step_GeV) {
    const auto first_above = std::lower_bound(maximum_cluster_energy_GeV.begin(),
                                              maximum_cluster_energy_GeV.end(),
                                              threshold);
    const long long above = maximum_cluster_energy_GeV.end() - first_above;
    const long long empty = selected - above;
    const double occupancy = selected ? double(above) / selected : 0.0;
    const double occupancy_error = selected
        ? std::sqrt(occupancy * (1.0 - occupancy) / selected)
        : 0.0;
    const double rate = occupancy < 1.0
        ? -std::log1p(-occupancy) / kWindowSeconds
        : INFINITY;
    const double rate_error = occupancy < 1.0
        ? occupancy_error / ((1.0 - occupancy) * kWindowSeconds)
        : INFINITY;
    const bool saturated = empty < kMinimumEmptyWindows;

    out << run << ',' << threshold << ',' << all_edtm << ',' << selected << ','
        << above << ',' << empty << ',' << occupancy << ',' << rate << ','
        << rate_error << ',' << (selected ? current_sum / selected : 0.0) << ','
        << (saturated ? 1 : 0) << '\n';
  }

  std::cout << "run=" << run << " all_EDTM=" << all_edtm
            << " selected_EDTM=" << selected
            << " mean_current_uA="
            << (selected ? current_sum / selected : 0.0)
            << " output=" << output_csv << std::endl;
}
