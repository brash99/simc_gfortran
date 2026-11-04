#include <TFile.h>
#include <TTree.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

void screen_nps_threshold_candidate(const char* input,
                                    const char* output_csv,
                                    int run,
                                    Long64_t maximum_events = 200000) {
  constexpr int kMaxObjects = 4096;
  TFile file(input, "READ");
  auto* events = dynamic_cast<TTree*>(file.Get("T"));
  auto* scaler = dynamic_cast<TTree*>(file.Get("TSH"));
  if (file.IsZombie() || !events || !scaler) {
    std::cerr << "Cannot read T and TSH from " << input << std::endl;
    return;
  }

  double edtm_raw = 0.0;
  double current_uA = 0.0;
  int n_vtp = 0;
  std::array<double, kMaxObjects> energy{};
  events->SetBranchStatus("*", 0);
  events->SetBranchStatus("T.hms.hEDTM_tdcTimeRaw", 1);
  events->SetBranchStatus("H.BCM4A.scalerCurrent", 1);
  events->SetBranchStatus("Ndata.NPS.cal.vtpClusE", 1);
  events->SetBranchStatus("NPS.cal.vtpClusE", 1);
  events->SetBranchAddress("T.hms.hEDTM_tdcTimeRaw", &edtm_raw);
  events->SetBranchAddress("H.BCM4A.scalerCurrent", &current_uA);
  events->SetBranchAddress("Ndata.NPS.cal.vtpClusE", &n_vtp);
  events->SetBranchAddress("NPS.cal.vtpClusE", energy.data());

  std::vector<double> maxima;
  const Long64_t entries = std::min(maximum_events, events->GetEntries());
  maxima.reserve(entries);
  for (Long64_t entry = 0; entry < entries; ++entry) {
    events->GetEntry(entry);
    if (edtm_raw != 0.0 || current_uA < 25.0 || current_uA > 35.0 ||
        n_vtp <= 0 || n_vtp > kMaxObjects)
      continue;
    maxima.push_back(*std::max_element(energy.begin(), energy.begin() + n_vtp)
                     / 1000.0);
  }
  std::sort(maxima.begin(), maxima.end());
  const auto quantile = [&maxima](double fraction) {
    if (maxima.empty()) return 0.0;
    const size_t index = std::min(maxima.size() - 1,
        size_t(fraction * double(maxima.size() - 1)));
    return maxima[index];
  };

  double scaler_current = 0.0;
  double scaler_rate = 0.0;
  scaler->SetBranchStatus("*", 0);
  scaler->SetBranchStatus("H.BCM4A.scalerCurrent", 1);
  scaler->SetBranchStatus("H.pTRIG1.scalerRate", 1);
  scaler->SetBranchAddress("H.BCM4A.scalerCurrent", &scaler_current);
  scaler->SetBranchAddress("H.pTRIG1.scalerRate", &scaler_rate);
  long long snapshots = 0;
  double current_sum = 0.0;
  double rate_sum = 0.0;
  for (Long64_t entry = 0; entry < scaler->GetEntries(); ++entry) {
    scaler->GetEntry(entry);
    if (scaler_current < 25.0 || scaler_current > 35.0) continue;
    ++snapshots;
    current_sum += scaler_current;
    rate_sum += scaler_rate;
  }

  std::ofstream output(output_csv);
  output << "run,events_scanned,selected_non_edtm_events,q001_GeV,q01_GeV,"
            "q05_GeV,q10_GeV,q25_GeV,median_GeV,scaler_snapshots,"
            "mean_current_uA,ptrig1_rate_Hz\n";
  output << std::setprecision(12) << run << ',' << entries << ',' << maxima.size()
         << ',' << quantile(0.001) << ',' << quantile(0.01) << ','
         << quantile(0.05) << ',' << quantile(0.10) << ',' << quantile(0.25)
         << ',' << quantile(0.50) << ',' << snapshots << ','
         << (snapshots ? current_sum / snapshots : 0.0) << ','
         << (snapshots ? rate_sum / snapshots : 0.0) << '\n';
  std::cout << "run=" << run << " selected=" << maxima.size()
            << " q01=" << quantile(0.01) << " q05=" << quantile(0.05)
            << " q10=" << quantile(0.10) << " q25=" << quantile(0.25)
            << " pTRIG1=" << (snapshots ? rate_sum / snapshots / 1000.0 : 0.0)
            << " kHz" << std::endl;
}
