#include <TFile.h>
#include <TTree.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>

void extract_nps_scaler_anchor(const char* input,
                               const char* output_csv,
                               int run,
                               double hardware_threshold_GeV,
                               double minimum_current_uA = 25.0,
                               double maximum_current_uA = 35.0) {
  TFile file(input, "READ");
  auto* scaler = dynamic_cast<TTree*>(file.Get("TSH"));
  if (file.IsZombie() || !scaler) {
    std::cerr << "Cannot read TSH from " << input << std::endl;
    return;
  }

  double current_uA = 0.0;
  double rate_Hz = 0.0;
  scaler->SetBranchAddress("H.BCM4A.scalerCurrent", &current_uA);
  scaler->SetBranchAddress("H.pTRIG1.scalerRate", &rate_Hz);

  long long snapshots = 0;
  double current_sum = 0.0;
  double rate_sum = 0.0;
  double rate_sum2 = 0.0;
  for (Long64_t entry = 0; entry < scaler->GetEntries(); ++entry) {
    scaler->GetEntry(entry);
    if (current_uA < minimum_current_uA || current_uA > maximum_current_uA)
      continue;
    ++snapshots;
    current_sum += current_uA;
    rate_sum += rate_Hz;
    rate_sum2 += rate_Hz * rate_Hz;
  }

  const double mean = snapshots ? rate_sum / snapshots : 0.0;
  const double rms = snapshots
      ? std::sqrt(std::max(0.0, rate_sum2 / snapshots - mean * mean)) : 0.0;
  const double sem = snapshots ? rms / std::sqrt(double(snapshots)) : 0.0;

  std::ofstream output(output_csv);
  output << "run,hardware_threshold_GeV,minimum_current_uA,maximum_current_uA,"
            "selected_scaler_snapshots,mean_current_uA,ptrig1_scaler_rate_Hz,"
            "ptrig1_scaler_rms_Hz,ptrig1_scaler_sem_Hz\n";
  output << std::setprecision(12) << run << ',' << hardware_threshold_GeV << ','
         << minimum_current_uA << ',' << maximum_current_uA << ',' << snapshots
         << ',' << (snapshots ? current_sum / snapshots : 0.0) << ',' << mean
         << ',' << rms << ',' << sem << '\n';
  std::cout << "run=" << run << " snapshots=" << snapshots
            << " current=" << (snapshots ? current_sum / snapshots : 0.0)
            << " uA pTRIG1=" << mean / 1000.0 << " +/- " << sem / 1000.0
            << " kHz (SEM)" << std::endl;
}
