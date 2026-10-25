#include <TFile.h>
#include <TTree.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

// Measure the NPS primitive rate in unbiased 1-us EDTM windows for an
// electron-calibration run. Thresholds are expressed in offline-equivalent
// physical energy and converted with the measured run-specific VTP relation.
void analyze_nps_electron_edtm_angles(
    const char* input,
    const char* output_csv,
    int run,
    double nps_angle_deg,
    double vtp_intercept_GeV,
    double vtp_slope,
    double minimum_current_uA = 20.0) {
  constexpr int kMaxObjects = 4096;
  constexpr double kWindowSeconds = 1.0e-6;

  TFile file(input, "READ");
  auto* tree = dynamic_cast<TTree*>(file.Get("T"));
  if (file.IsZombie() || !tree) {
    std::cerr << "Cannot read tree T from " << input << std::endl;
    return;
  }

  double edtm_time_raw = 0.0;
  double current_uA = 0.0;
  int n_vtp = 0;
  std::array<double, kMaxObjects> vtp_energy{};
  tree->SetBranchAddress("T.hms.hEDTM_tdcTimeRaw", &edtm_time_raw);
  tree->SetBranchAddress("H.BCM4A.scalerCurrent", &current_uA);
  tree->SetBranchAddress("Ndata.NPS.cal.vtpClusE", &n_vtp);
  tree->SetBranchAddress("NPS.cal.vtpClusE", vtp_energy.data());

  constexpr int kNThresholds = 24;
  std::array<double, kNThresholds> physical_threshold{};
  std::array<long long, kNThresholds> above{};
  for (int i = 0; i < kNThresholds; ++i)
    physical_threshold[i] = 2.3 + 0.1 * i;

  long long all_edtm = 0;
  long long selected_edtm = 0;
  double current_sum = 0.0;
  const Long64_t entries = tree->GetEntries();
  for (Long64_t entry = 0; entry < entries; ++entry) {
    tree->GetEntry(entry);
    if (edtm_time_raw == 0.0)
      continue;
    ++all_edtm;
    if (current_uA < minimum_current_uA)
      continue;
    if (n_vtp < 0 || n_vtp > kMaxObjects) {
      std::cerr << "Invalid VTP object count at entry " << entry << std::endl;
      return;
    }
    ++selected_edtm;
    current_sum += current_uA;
    const double max_vtp_GeV = n_vtp > 0
        ? *std::max_element(vtp_energy.begin(), vtp_energy.begin() + n_vtp) / 1000.0
        : 0.0;
    for (int i = 0; i < kNThresholds; ++i) {
      const double vtp_threshold =
          vtp_intercept_GeV + vtp_slope * physical_threshold[i];
      if (max_vtp_GeV >= vtp_threshold)
        ++above[i];
    }
  }

  const double mean_current = selected_edtm ? current_sum / selected_edtm : 0.0;
  std::ofstream out(output_csv);
  out << "run,nps_angle_deg,minimum_current_uA,mean_selected_current_uA,"
         "all_edtm_windows,selected_edtm_windows,physical_threshold_GeV,"
         "vtp_threshold_GeV,windows_above,occupancy,primitive_rate_Hz,"
         "stat_error_Hz,rate_per_uA_Hz\n";
  out << std::fixed << std::setprecision(8);
  for (int i = 0; i < kNThresholds; ++i) {
    const double p = selected_edtm ? double(above[i]) / selected_edtm : 0.0;
    const double rate = p < 1.0 ? -std::log1p(-p) / kWindowSeconds : INFINITY;
    const double sigma_p = selected_edtm
        ? std::sqrt(p * (1.0 - p) / selected_edtm) : 0.0;
    const double sigma_rate = p < 1.0
        ? sigma_p / ((1.0 - p) * kWindowSeconds) : INFINITY;
    const double vtp_threshold =
        vtp_intercept_GeV + vtp_slope * physical_threshold[i];
    out << run << ',' << nps_angle_deg << ',' << minimum_current_uA << ','
        << mean_current << ',' << all_edtm << ',' << selected_edtm << ','
        << physical_threshold[i] << ',' << vtp_threshold << ',' << above[i]
        << ',' << p << ',' << rate << ',' << sigma_rate << ','
        << (mean_current > 0.0 ? rate / mean_current : 0.0) << '\n';
  }

  std::cout << "run=" << run << " all_EDTM=" << all_edtm
            << " selected_EDTM=" << selected_edtm
            << " mean_current_uA=" << mean_current << std::endl;
}
