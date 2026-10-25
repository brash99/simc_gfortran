#include <TFile.h>
#include <TTree.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <iomanip>
#include <iostream>

namespace {
constexpr int kRows = 36;
constexpr int kColumns = 30;
constexpr int kBlocks = kRows * kColumns;

double Sum3x3(const double* edep, int seed) {
  const int column = seed / kRows;
  const int row = seed % kRows;
  double sum = 0.;
  for (int c = std::max(0, column - 1);
       c <= std::min(kColumns - 1, column + 1); ++c) {
    for (int r = std::max(0, row - 1);
         r <= std::min(kRows - 1, row + 1); ++r) {
      sum += edep[r + kRows * c];
    }
  }
  return sum;
}
}

void analyze_run2875_elastic_trigger_closure(
    const char* input_name,
    const char* output_name = "nps_run2875_elastic_trigger_closure.csv",
    double simc_integrated_yield_events_per_mC = 222.22536) {
  TFile input(input_name, "READ");
  auto* tree = dynamic_cast<TTree*>(input.Get("t_dvcs"));
  if (!tree) {
    std::cerr << "Missing t_dvcs in " << input_name << '\n';
    return;
  }

  double edep[kBlocks]{};
  double weight = 0.;
  tree->SetBranchAddress("edep", edep);
  tree->SetBranchAddress("psf", &weight);

  constexpr std::array<double, 5> thresholds{{0., 1.4, 4.9504, 5.381792, 5.69296}};
  std::array<double, thresholds.size()> accepted_weight{};
  double total_weight = 0.;

  for (Long64_t entry = 0; entry < tree->GetEntries(); ++entry) {
    tree->GetEntry(entry);
    const int seed = std::distance(edep,
        std::max_element(edep, edep + kBlocks));
    const double cluster_energy_gev = Sum3x3(edep, seed) / 1000.;
    total_weight += weight;
    for (size_t i = 0; i < thresholds.size(); ++i) {
      if (cluster_energy_gev >= thresholds[i]) accepted_weight[i] += weight;
    }
  }

  // The run report gives 36.270 uA from BCM4A after the beam-current cut.
  // One microampere is 0.001 mC/s.
  constexpr double nominal_current_mC_per_s = 0.036270;
  std::ofstream output(output_name);
  output << "threshold_GeV,accepted_yield_events_per_mC,"
            "fraction_of_simc_elastic,rate_at_36p270uA_Hz\n";
  output << std::setprecision(12);
  for (size_t i = 0; i < thresholds.size(); ++i) {
    const double fraction = total_weight > 0.
        ? accepted_weight[i] / total_weight : 0.;
    const double accepted_yield = simc_integrated_yield_events_per_mC * fraction;
    output << thresholds[i] << ',' << accepted_yield << ',' << fraction
           << ',' << accepted_yield * nominal_current_mC_per_s << '\n';
  }

  std::cout << std::setprecision(12)
            << "entries=" << tree->GetEntries() << '\n'
            << "raw_geant_weight_sum=" << total_weight << '\n'
            << "simc_integrated_yield_events_per_mC="
            << simc_integrated_yield_events_per_mC << '\n';
  for (size_t i = 0; i < thresholds.size(); ++i) {
    const double fraction = accepted_weight[i] / total_weight;
    const double accepted_yield = simc_integrated_yield_events_per_mC * fraction;
    std::cout << "threshold_GeV=" << thresholds[i]
              << " yield_events_per_mC=" << accepted_yield
              << " fraction=" << fraction
              << " rate_at_36p270uA_Hz=" << accepted_yield * nominal_current_mC_per_s
              << '\n';
  }
}
