#include <TFile.h>
#include <TTree.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <iomanip>
#include <iostream>

void extract_nps_inclusive_geant4_response(const char* input,
                                            const char* output_csv) {
  constexpr int kColumns = 30;
  constexpr int kRows = 36;
  constexpr int kCrystals = kColumns * kRows;
  constexpr double kInheritedEnergyScale = 1.04 * 1.04;

  TFile file(input, "READ");
  auto* tree = dynamic_cast<TTree*>(file.Get("t_dvcs"));
  if (file.IsZombie() || !tree) {
    std::cerr << "Cannot read t_dvcs from " << input << std::endl;
    return;
  }
  std::array<double, kCrystals> edep{};
  tree->SetBranchAddress("edep", edep.data());

  std::ofstream output(output_csv);
  output << "entry,hottest_column,hottest_row,raw_total_energy_GeV,raw_3x3_energy_GeV,"
            "corrected_total_energy_GeV,corrected_3x3_energy_GeV\n";
  output << std::setprecision(12);
  for (Long64_t entry = 0; entry < tree->GetEntries(); ++entry) {
    tree->GetEntry(entry);
    const auto hottest = std::distance(edep.begin(),
        std::max_element(edep.begin(), edep.end()));
    const int geant_column = hottest / kRows;
    const int geant_row = hottest % kRows;
    // GEANT4 places outer index 0 at positive local X and increments toward
    // negative X.  The decoded VTP column increases from negative to positive
    // local X, so reflect the 30-column index for a data-aligned coordinate.
    const int vtp_column = kColumns - 1 - geant_column;
    const int vtp_row = geant_row;
    double total = 0.0;
    double local = 0.0;
    for (int index = 0; index < kCrystals; ++index)
      total += edep[index];
    for (int neighbor_column = std::max(0, geant_column - 1);
         neighbor_column <= std::min(kColumns - 1, geant_column + 1);
         ++neighbor_column) {
      for (int neighbor_row = std::max(0, geant_row - 1);
           neighbor_row <= std::min(kRows - 1, geant_row + 1);
           ++neighbor_row) {
        local += edep[kRows * neighbor_column + neighbor_row];
      }
    }
    output << entry << ',' << vtp_column << ',' << vtp_row << ','
           << total / 1000.0 << ',' << local / 1000.0 << ','
           << total / 1000.0 / kInheritedEnergyScale << ','
           << local / 1000.0 / kInheritedEnergyScale << '\n';
  }
  std::cout << "wrote " << tree->GetEntries() << " events to " << output_csv
            << std::endl;
}
