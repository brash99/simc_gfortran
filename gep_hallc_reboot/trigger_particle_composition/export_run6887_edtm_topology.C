#include <TFile.h>
#include <TTree.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>

// Export one row per unbiased EDTM window in the selected beam-current band.
// VTP positions are zero-based crystal-column/row indices and VTP energy is
// converted from MeV to GeV.  This describes calorimeter topology; run 6887
// has no independent charged-particle tag in front of NPS.
void export_run6887_edtm_topology(
    const char* input,
    const char* output_csv,
    double minimum_current_uA = 25.0,
    double maximum_current_uA = 35.0) {
  constexpr int kMaxObjects = 4096;

  TFile file(input, "READ");
  auto* events = dynamic_cast<TTree*>(file.Get("T"));
  if (file.IsZombie() || !events) {
    std::cerr << "Cannot read T from " << input << std::endl;
    return;
  }

  events->SetBranchStatus("*", 0);
  for (const char* branch : {
           "T.hms.hEDTM_tdcTimeRaw", "H.BCM4A.scalerCurrent",
           "Ndata.NPS.cal.vtpClusE", "NPS.cal.vtpClusE",
           "NPS.cal.vtpClusSize", "NPS.cal.vtpClusTime",
           "NPS.cal.vtpClusX", "NPS.cal.vtpClusY"})
    events->SetBranchStatus(branch, 1);

  double edtm_time_raw = 0.0;
  double current_uA = 0.0;
  int n_vtp = 0;
  std::array<double, kMaxObjects> energy{};
  std::array<double, kMaxObjects> size{};
  std::array<double, kMaxObjects> time{};
  std::array<double, kMaxObjects> x{};
  std::array<double, kMaxObjects> y{};
  events->SetBranchAddress("T.hms.hEDTM_tdcTimeRaw", &edtm_time_raw);
  events->SetBranchAddress("H.BCM4A.scalerCurrent", &current_uA);
  events->SetBranchAddress("Ndata.NPS.cal.vtpClusE", &n_vtp);
  events->SetBranchAddress("NPS.cal.vtpClusE", energy.data());
  events->SetBranchAddress("NPS.cal.vtpClusSize", size.data());
  events->SetBranchAddress("NPS.cal.vtpClusTime", time.data());
  events->SetBranchAddress("NPS.cal.vtpClusX", x.data());
  events->SetBranchAddress("NPS.cal.vtpClusY", y.data());

  std::ofstream out(output_csv);
  out << "entry,current_uA,n_vtp,max_energy_GeV,max_size,max_time_ns,"
         "max_x_column,max_y_row,n_above_2p5,n_above_3p387,n_above_3p629,"
         "n_above_3p870,n_separate_2p5,n_separate_3p387,"
         "n_separate_3p629,n_separate_3p870\n";
  out << std::fixed << std::setprecision(8);

  long long all_edtm = 0;
  long long selected = 0;
  for (Long64_t entry = 0; entry < events->GetEntries(); ++entry) {
    events->GetEntry(entry);
    if (edtm_time_raw == 0.0)
      continue;
    ++all_edtm;
    if (current_uA < minimum_current_uA || current_uA >= maximum_current_uA)
      continue;
    if (n_vtp < 0 || n_vtp > kMaxObjects) {
      std::cerr << "Invalid VTP object count at entry " << entry << std::endl;
      return;
    }

    int maximum_index = -1;
    std::array<int, 4> above{};
    std::array<int, 4> separate{};
    constexpr std::array<double, 4> thresholds_MeV = {
        2500.0, 3387.0, 3629.0, 3870.0};
    for (int index = 0; index < n_vtp; ++index) {
      if (maximum_index < 0 || energy[index] > energy[maximum_index])
        maximum_index = index;
      for (std::size_t threshold = 0; threshold < thresholds_MeV.size();
           ++threshold)
        if (energy[index] >= thresholds_MeV[threshold])
          ++above[threshold];
    }

    // A second 3x3 candidate is counted as spatially separate only when its
    // 3x3 region does not overlap the leading candidate's 3x3 region.
    if (maximum_index >= 0) {
      for (int index = 0; index < n_vtp; ++index) {
        if (index == maximum_index)
          continue;
        const bool nonoverlapping =
            std::abs(x[index] - x[maximum_index]) >= 3.0 ||
            std::abs(y[index] - y[maximum_index]) >= 3.0;
        if (!nonoverlapping)
          continue;
        for (std::size_t threshold = 0; threshold < thresholds_MeV.size();
             ++threshold)
          if (energy[index] >= thresholds_MeV[threshold])
            ++separate[threshold];
      }
    }

    out << entry << ',' << current_uA << ',' << n_vtp << ',';
    if (maximum_index >= 0) {
      out << energy[maximum_index] / 1000.0 << ',' << size[maximum_index] << ','
          << time[maximum_index] << ',' << x[maximum_index] << ','
          << y[maximum_index];
    } else {
      out << "0,0,0,-1,-1";
    }
    for (int count : above)
      out << ',' << count;
    for (int count : separate)
      out << ',' << count;
    out << '\n';
    ++selected;
  }

  std::cout << "all_EDTM=" << all_edtm << " selected_EDTM=" << selected
            << " output=" << output_csv << std::endl;
}
