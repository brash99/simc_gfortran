#include <TFile.h>
#include <TTree.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>

// Compare the pTRIG1 hardware scaler with the rate inferred from unbiased
// 1-us EDTM windows at the same nominal 1.6-GeV VTP threshold and beam cut.
void analyze_nps_electron_edtm_scaler_closure(
    const char* input,
    const char* output_csv,
    int run,
    double nps_angle_deg,
    double minimum_current_uA = 20.0,
    double vtp_threshold_GeV = 1.6) {
  constexpr int kMaxObjects = 4096;
  constexpr double kWindowSeconds = 1.0e-6;

  TFile file(input, "READ");
  auto* events = dynamic_cast<TTree*>(file.Get("T"));
  auto* scaler = dynamic_cast<TTree*>(file.Get("TSH"));
  if (file.IsZombie() || !events || !scaler) {
    std::cerr << "Cannot read T and TSH from " << input << std::endl;
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
  long long selected_edtm = 0;
  long long above = 0;
  double event_current_sum = 0.0;
  for (Long64_t entry = 0; entry < events->GetEntries(); ++entry) {
    events->GetEntry(entry);
    if (edtm_time_raw == 0.0)
      continue;
    ++all_edtm;
    if (event_current_uA < minimum_current_uA)
      continue;
    if (n_vtp < 0 || n_vtp > kMaxObjects) {
      std::cerr << "Invalid VTP object count at entry " << entry << std::endl;
      return;
    }
    ++selected_edtm;
    event_current_sum += event_current_uA;
    const double max_vtp_GeV = n_vtp > 0
        ? *std::max_element(vtp_energy.begin(), vtp_energy.begin() + n_vtp) / 1000.0
        : 0.0;
    if (max_vtp_GeV >= vtp_threshold_GeV)
      ++above;
  }

  double scaler_current_uA = 0.0;
  double ptrig1_rate_Hz = 0.0;
  scaler->SetBranchAddress("H.BCM4A.scalerCurrent", &scaler_current_uA);
  scaler->SetBranchAddress("H.pTRIG1.scalerRate", &ptrig1_rate_Hz);
  long long selected_snapshots = 0;
  double scaler_current_sum = 0.0;
  double scaler_rate_sum = 0.0;
  double scaler_rate_sum2 = 0.0;
  for (Long64_t entry = 0; entry < scaler->GetEntries(); ++entry) {
    scaler->GetEntry(entry);
    if (scaler_current_uA < minimum_current_uA)
      continue;
    ++selected_snapshots;
    scaler_current_sum += scaler_current_uA;
    scaler_rate_sum += ptrig1_rate_Hz;
    scaler_rate_sum2 += ptrig1_rate_Hz * ptrig1_rate_Hz;
  }

  const double occupancy = selected_edtm ? double(above) / selected_edtm : 0.0;
  const double occupancy_error = selected_edtm
      ? std::sqrt(occupancy * (1.0 - occupancy) / selected_edtm) : 0.0;
  const double edtm_rate = occupancy < 1.0
      ? -std::log1p(-occupancy) / kWindowSeconds : INFINITY;
  const double edtm_error = occupancy < 1.0
      ? occupancy_error / ((1.0 - occupancy) * kWindowSeconds) : INFINITY;
  const double scaler_rate = selected_snapshots ? scaler_rate_sum / selected_snapshots : 0.0;
  const double scaler_rms = selected_snapshots
      ? std::sqrt(std::max(0.0, scaler_rate_sum2 / selected_snapshots - scaler_rate * scaler_rate))
      : 0.0;

  std::ofstream out(output_csv);
  out << "run,nps_angle_deg,minimum_current_uA,vtp_threshold_GeV,all_edtm_windows,"
         "selected_edtm_windows,windows_above,edtm_occupancy,edtm_rate_Hz,"
         "edtm_stat_error_Hz,mean_event_current_uA,selected_scaler_snapshots,"
         "mean_scaler_current_uA,ptrig1_scaler_rate_Hz,ptrig1_scaler_rms_Hz,"
         "edtm_over_scaler\n";
  out << std::fixed << std::setprecision(8)
      << run << ',' << nps_angle_deg << ',' << minimum_current_uA << ','
      << vtp_threshold_GeV << ',' << all_edtm << ',' << selected_edtm << ','
      << above << ',' << occupancy << ',' << edtm_rate << ',' << edtm_error << ','
      << (selected_edtm ? event_current_sum / selected_edtm : 0.0) << ','
      << selected_snapshots << ','
      << (selected_snapshots ? scaler_current_sum / selected_snapshots : 0.0) << ','
      << scaler_rate << ',' << scaler_rms << ','
      << (scaler_rate > 0.0 ? edtm_rate / scaler_rate : 0.0) << '\n';

  std::cout << "run=" << run << " EDTM=" << edtm_rate / 1000.0 << "+/-"
            << edtm_error / 1000.0 << " kHz pTRIG1=" << scaler_rate / 1000.0
            << " kHz ratio=" << (scaler_rate > 0.0 ? edtm_rate / scaler_rate : 0.0)
            << std::endl;
}
