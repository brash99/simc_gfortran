#include <TFile.h>
#include <TTree.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

namespace {
constexpr std::array<int, 7> kRuns{{17193, 17194, 17195, 17196, 17197, 17198, 17199}};

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

void analyze_xem2_hms_s1_scalers(
    const char* directory =
        "/work/hallc/gep/brash/hallc_replay_XEM/ROOTfiles/COIN/SCALERS",
    const char* output = "xem2_hms_s1_scalers.csv",
    double minimum_current_uA = 10.0) {
  std::ofstream out(output);
  out << "run,snapshots,mean_current_uA,"
         "S1X_Hz,S1Y_Hz,S1XS1Y_branch_Hz,hTRIG4_3of4_Hz,"
         "S1X_per_uA_Hz,S1Y_per_uA_Hz,S1XS1Y_branch_per_uA_Hz,"
         "hTRIG4_3of4_per_uA_Hz,"
         "derived_intersection_per_uA_Hz,derived_xor_per_uA_Hz,"
         "branch_satisfies_or_bounds\n";
  out << std::setprecision(12);

  for (const int run : kRuns) {
    const std::string path = std::string(directory) +
        "/coin_replay_scalers_" + std::to_string(run) + "_-1.root";
    TFile file(path.c_str(), "READ");
    auto* tree = dynamic_cast<TTree*>(file.Get("TSH"));
    if (file.IsZombie() || !tree) {
      std::cerr << "missing TSH in " << path << std::endl;
      continue;
    }

    constexpr std::array<const char*, 5> branches{{
        "H.BCM4A.scalerCurrent", "H.S1X.scalerRate", "H.S1Y.scalerRate",
        "H.S1XS1Y.scalerRate", "H.hTRIG4.scalerRate"}};
    std::array<double, branches.size()> value{};
    bool missing = false;
    for (size_t index = 0; index < branches.size(); ++index) {
      if (!tree->GetBranch(branches[index])) {
        std::cerr << "missing " << branches[index] << " in run " << run << std::endl;
        missing = true;
      } else {
        tree->SetBranchAddress(branches[index], &value[index]);
      }
    }
    if (missing) continue;

    Moments current;
    std::array<Moments, 4> rate;
    std::array<Moments, 4> normalized;
    for (Long64_t entry = 0; entry < tree->GetEntries(); ++entry) {
      tree->GetEntry(entry);
      if (value[0] < minimum_current_uA) continue;
      current.Add(value[0]);
      for (size_t index = 0; index < rate.size(); ++index) {
        rate[index].Add(value[index + 1]);
        normalized[index].Add(value[index + 1] / value[0]);
      }
    }

    const double x = normalized[0].Mean();
    const double y = normalized[1].Mean();
    const double combined = normalized[2].Mean();
    const double intersection = x + y - combined;
    const double xor_rate = combined - intersection;
    const bool or_bounds = combined >= std::max(x, y) && combined <= x + y &&
                           intersection >= 0.0;

    out << run << ',' << current.n << ',' << current.Mean();
    for (const auto& item : rate) out << ',' << item.Mean();
    for (const auto& item : normalized) out << ',' << item.Mean();
    out << ',' << intersection << ',' << xor_rate << ',' << or_bounds << '\n';

    std::cout << "run=" << run << " I=" << current.Mean() << " uA"
              << " S1X=" << x << " S1Y=" << y
              << " combined=" << combined
              << " intersection(if OR)=" << intersection
              << " xor(if OR)=" << xor_rate
              << " OR-bounds=" << or_bounds << std::endl;
  }
}
