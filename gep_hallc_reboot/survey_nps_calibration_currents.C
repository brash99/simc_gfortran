#include <TFile.h>
#include <TTree.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {
double quantile(const std::vector<double>& sorted, double probability) {
  if (sorted.empty()) return 0.;
  const double position = probability * (sorted.size() - 1);
  const auto lower = static_cast<std::size_t>(std::floor(position));
  const auto upper = static_cast<std::size_t>(std::ceil(position));
  const double fraction = position - lower;
  return sorted[lower] * (1. - fraction) + sorted[upper] * fraction;
}
}

// Compare event-latched BCM4A current in uniform small replays of the
// run-6151--6168 elastic-calibration group. Ordinary events exclude EDTM.
void survey_nps_calibration_currents(
    const char* input_directory,
    const char* output = "nps_calibration_run_current_survey.csv",
    const char* selection = "6151-6168") {
  std::ofstream csv(output);
  csv << "run,entries,ordinary_events,mean_uA,median_uA,p10_uA,p90_uA,p95_uA,max_uA,mode_0p5uA,fraction_ge_30uA,fraction_ge_40uA,fraction_ge_50uA\n";
  csv << std::fixed << std::setprecision(6);

  std::vector<int> runs;
  if (std::string(selection) == "cross-setting")
    runs = {6172, 5217, 5183, 3883, 2875, 2855};
  else
    for (int run = 6151; run <= 6168; ++run) runs.push_back(run);

  for (const int run : runs) {
    const std::string path = std::string(input_directory) + "/nps_hms_coin_" +
                             std::to_string(run) + "_1_5000.root";
    TFile file(path.c_str(), "READ");
    auto* tree = dynamic_cast<TTree*>(file.Get("T"));
    if (file.IsZombie() || !tree) {
      std::cerr << "Missing tree for run " << run << std::endl;
      continue;
    }
    double edtm = 0., current = 0.;
    tree->SetBranchAddress("T.hms.hEDTM_tdcTimeRaw", &edtm);
    tree->SetBranchAddress("H.BCM4A.scalerCurrent", &current);
    std::vector<double> currents;
    currents.reserve(tree->GetEntries());
    double sum = 0.;
    long long ge30 = 0, ge40 = 0, ge50 = 0;
    std::vector<long long> histogram(161, 0);
    for (Long64_t entry = 0; entry < tree->GetEntries(); ++entry) {
      tree->GetEntry(entry);
      if (edtm != 0. || !std::isfinite(current) || current < 0.) continue;
      currents.push_back(current);
      sum += current;
      if (current >= 30.) ++ge30;
      if (current >= 40.) ++ge40;
      if (current >= 50.) ++ge50;
      const int bin = std::clamp(static_cast<int>(std::floor(current / .5)), 0, 160);
      ++histogram[bin];
    }
    std::sort(currents.begin(), currents.end());
    const auto mode_bin = std::distance(histogram.begin(),
                                        std::max_element(histogram.begin(), histogram.end()));
    const double n = currents.size();
    csv << run << ',' << tree->GetEntries() << ',' << currents.size() << ','
        << (n ? sum / n : 0.) << ',' << quantile(currents, .5) << ','
        << quantile(currents, .1) << ',' << quantile(currents, .9) << ','
        << quantile(currents, .95) << ',' << (n ? currents.back() : 0.) << ','
        << (mode_bin + .5) * .5 << ','
        << (n ? ge30 / n : 0.) << ',' << (n ? ge40 / n : 0.) << ','
        << (n ? ge50 / n : 0.) << '\n';
  }
}
