#include <TFile.h>
#include <TTree.h>

#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

namespace {
constexpr std::array<int, 7> kRuns{{17193, 17194, 17195, 17196, 17197, 17198, 17199}};
constexpr std::array<const char*, 17> kNames{{
    "S1X", "S1Y", "S1XS1Y", "S2X", "S2Y", "S2XS2Y", "HCER", "NCER",
    "pSTOF", "pEL_LO_LO", "pEL_LO", "pEL_HI", "pEL_REAL", "pEL_CLEAN",
    "pTRIG1", "pTRIG2", "pTRIG6"}};

struct Moments {
  long long n = 0;
  double sum = 0.0;
  double sum2 = 0.0;
  void Add(double value) { ++n; sum += value; sum2 += value * value; }
  double Mean() const { return n ? sum / n : 0.0; }
  double Rms() const {
    const double mean = Mean();
    return n ? std::sqrt(std::max(0.0, sum2 / n - mean * mean)) : 0.0;
  }
};
}

void analyze_xem2_shms_singles_scalers(
    const char* directory =
        "/work/hallc/gep/brash/hallc_replay_XEM/ROOTfiles/COIN/SCALERS",
    const char* output = "xem2_shms_singles_scalers.csv",
    double minimum_current_uA = 10.0) {
  std::ofstream out(output);
  out << "run,snapshots,mean_current_uA";
  for (const char* name : kNames)
    out << ',' << name << "_rate_Hz," << name << "_rate_per_uA_Hz," << name
        << "_rate_per_uA_rms_Hz";
  out << '\n' << std::setprecision(10);

  for (const int run : kRuns) {
    const std::string path = std::string(directory) +
        "/coin_replay_scalers_" + std::to_string(run) + "_-1.root";
    TFile file(path.c_str(), "READ");
    auto* tree = dynamic_cast<TTree*>(file.Get("TSP"));
    if (file.IsZombie() || !tree) {
      std::cerr << "missing TSP in " << path << std::endl;
      continue;
    }
    double current = 0.0;
    std::array<double, kNames.size()> rates{};
    tree->SetBranchAddress("P.BCM4A.scalerCurrent", &current);
    for (size_t index = 0; index < kNames.size(); ++index) {
      const std::string branch = "P." + std::string(kNames[index]) + ".scalerRate";
      if (!tree->GetBranch(branch.c_str())) {
        std::cerr << "missing " << branch << " in run " << run << std::endl;
        return;
      }
      tree->SetBranchAddress(branch.c_str(), &rates[index]);
    }
    Moments current_moments;
    std::array<Moments, kNames.size()> rate_moments{};
    std::array<Moments, kNames.size()> normalized_moments{};
    for (Long64_t entry = 0; entry < tree->GetEntries(); ++entry) {
      tree->GetEntry(entry);
      if (current < minimum_current_uA) continue;
      current_moments.Add(current);
      for (size_t index = 0; index < kNames.size(); ++index) {
        rate_moments[index].Add(rates[index]);
        normalized_moments[index].Add(rates[index] / current);
      }
    }
    out << run << ',' << current_moments.n << ',' << current_moments.Mean();
    for (size_t index = 0; index < kNames.size(); ++index)
      out << ',' << rate_moments[index].Mean() << ','
          << normalized_moments[index].Mean() << ','
          << normalized_moments[index].Rms();
    out << '\n';
    std::cout << "run=" << run << " current=" << current_moments.Mean()
              << " pTRIG1=" << rate_moments[14].Mean()
              << " pEL_REAL=" << rate_moments[12].Mean() << std::endl;
  }
}
