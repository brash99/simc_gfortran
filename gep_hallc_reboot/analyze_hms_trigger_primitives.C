#include <TFile.h>
#include <TTree.h>

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {
struct Channel {
  const char* name;
  const char* branch;
  double value = 0.;
};

struct Fit {
  long n = 0;
  double sx = 0., sy = 0., sxx = 0., sxy = 0.;
  void add(double x, double y) {
    ++n; sx += x; sy += y; sxx += x*x; sxy += x*y;
  }
  double slope() const {
    const double denominator = n*sxx-sx*sx;
    return denominator != 0. ? (n*sxy-sx*sy)/denominator : 0.;
  }
  double intercept() const { return n ? (sy-slope()*sx)/n : 0.; }
  double at(double x) const { return intercept()+slope()*x; }
};
}

void analyze_hms_trigger_primitives(
    const char* input,
    const char* output="hms_trigger_primitive_fits.csv") {
  TFile file(input);
  auto* tree = static_cast<TTree*>(file.Get("TSH"));
  if (!tree) {
    std::cerr << "Missing TSH tree" << std::endl;
    return;
  }

  double current = 0.;
  tree->SetBranchAddress("H.BCM4A.scalerCurrent", &current);
  std::vector<Channel> channels{
    {"S1X", "H.S1X.scalerRate"},
    {"S1Y", "H.S1Y.scalerRate"},
    {"S1XS1Y", "H.S1XS1Y.scalerRate"},
    {"ASUM", "H.ASUM.scalerRate"},
    {"BSUM", "H.BSUM.scalerRate"},
    {"CSUM", "H.CSUM.scalerRate"},
    {"DSUM", "H.DSUM.scalerRate"},
    {"PRLO", "H.PRLO.scalerRate"},
    {"PRHI", "H.PRHI.scalerRate"},
    {"SHLO", "H.SHLO.scalerRate"},
    {"hPRE40", "H.hPRE40.scalerRate"},
    {"hPRE100", "H.hPRE100.scalerRate"},
    {"hPRE150", "H.hPRE150.scalerRate"},
    {"hPRE200", "H.hPRE200.scalerRate"},
    {"hEL_LO_LO", "H.hEL_LO_LO.scalerRate"},
    {"hEL_LO", "H.hEL_LO.scalerRate"},
    {"hEL_HI", "H.hEL_HI.scalerRate"},
    {"hEL_REAL", "H.hEL_REAL.scalerRate"},
    {"hEL_CLEAN", "H.hEL_CLEAN.scalerRate"},
  };
  for (auto& channel : channels) {
    if (!tree->GetBranch(channel.branch)) {
      std::cerr << "Missing branch " << channel.branch << std::endl;
      return;
    }
    tree->SetBranchAddress(channel.branch, &channel.value);
  }

  std::vector<Fit> fits(channels.size());
  for (Long64_t entry = 0; entry < tree->GetEntries(); ++entry) {
    tree->GetEntry(entry);
    if (current < 15. || current >= 30.) continue;
    for (size_t i = 0; i < channels.size(); ++i)
      if (std::isfinite(channels[i].value)) fits[i].add(current, channels[i].value);
  }

  std::ofstream csv(output);
  csv << "primitive,branch,snapshots,slope_Hz_per_uA,intercept_Hz,rate_75uA_Hz\n";
  csv << std::setprecision(15);
  for (size_t i = 0; i < channels.size(); ++i) {
    csv << channels[i].name << ',' << channels[i].branch << ',' << fits[i].n << ','
        << fits[i].slope() << ',' << fits[i].intercept() << ',' << fits[i].at(75.)
        << '\n';
    std::cout << std::setw(12) << channels[i].name
              << " slope=" << std::setw(12) << fits[i].slope()
              << " intercept=" << std::setw(12) << fits[i].intercept()
              << " rate75=" << std::setw(12) << fits[i].at(75.) << '\n';
  }
}
