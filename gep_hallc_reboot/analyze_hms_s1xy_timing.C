#include <TFile.h>
#include <TH1D.h>
#include <TTree.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

void analyze_hms_s1xy_timing(
    const char* input,
    const char* output="hms_s1xy_timing_efficiency.csv",
    Long64_t max_entries=-1) {
  constexpr int kMax = 4096;
  constexpr double kBeamEnergy = 6.37042664;
  constexpr double kProtonMass = 0.9382720813;
  constexpr double kCoreMean = -0.225806;
  constexpr double kCoreSigma = 0.202053;

  TFile file(input);
  auto* tree = static_cast<TTree*>(file.Get("T"));
  if (!tree) { std::cerr << "Missing T tree\n"; return; }

  double edtm = 0., ntrack = 0.;
  int nclus = 0, nx = 0, ny = 0;
  std::array<double,kMax> p{}, dp{}, clus_e{}, tx{}, ty{};
  tree->SetBranchAddress("T.hms.hEDTM_tdcTimeRaw", &edtm);
  tree->SetBranchAddress("H.dc.ntrack", &ntrack);
  tree->SetBranchAddress("H.gtr.p", p.data());
  tree->SetBranchAddress("H.gtr.dp", dp.data());
  tree->SetBranchAddress("Ndata.NPS.cal.clusE", &nclus);
  tree->SetBranchAddress("NPS.cal.clusE", clus_e.data());
  tree->SetBranchAddress("Ndata.T.hms.h1X_vecTdcTime", &nx);
  tree->SetBranchAddress("T.hms.h1X_vecTdcTime", tx.data());
  tree->SetBranchAddress("Ndata.T.hms.h1Y_vecTdcTime", &ny);
  tree->SetBranchAddress("T.hms.h1Y_vecTdcTime", ty.data());

  auto elastic = [&]() {
    if (edtm != 0. || ntrack != 1. || std::abs(dp[0]) >= 8. || p[0] <= 0. ||
        nclus <= 0 || nclus > kMax) return false;
    const double max_nps = *std::max_element(clus_e.begin(), clus_e.begin()+nclus);
    const double expected = kBeamEnergy+kProtonMass-
      std::sqrt(p[0]*p[0]+kProtonMass*kProtonMass);
    const double residual = max_nps-expected;
    return residual >= kCoreMean-3*kCoreSigma && residual <= kCoreMean+3*kCoreSigma;
  };

  TH1D differences("differences", "S1X-S1Y corrected TDC time;ns", 1600, -200., 200.);
  long total = 0, both_present = 0;
  const Long64_t entries = max_entries > 0
    ? std::min(max_entries, tree->GetEntries()) : tree->GetEntries();
  for (Long64_t i = 0; i < entries; ++i) {
    tree->GetEntry(i);
    if (!elastic()) continue;
    ++total;
    if (nx <= 0 || ny <= 0 || nx > kMax || ny > kMax) continue;
    ++both_present;
    for (int ix = 0; ix < nx; ++ix)
      for (int iy = 0; iy < ny; ++iy)
        differences.Fill(tx[ix]-ty[iy]);
  }
  const int peak_bin = differences.GetMaximumBin();
  const double center = differences.GetBinCenter(peak_bin);

  const std::array<double,7> full_widths{{2., 4., 6., 10., 20., 40., 80.}};
  std::array<long,full_widths.size()> passing{};
  for (Long64_t i = 0; i < entries; ++i) {
    tree->GetEntry(i);
    if (!elastic() || nx <= 0 || ny <= 0 || nx > kMax || ny > kMax) continue;
    double best = std::numeric_limits<double>::infinity();
    for (int ix = 0; ix < nx; ++ix)
      for (int iy = 0; iy < ny; ++iy)
        best = std::min(best, std::abs((tx[ix]-ty[iy])-center));
    for (size_t iw = 0; iw < full_widths.size(); ++iw)
      passing[iw] += best <= full_widths[iw]/2.;
  }

  std::ofstream csv(output);
  csv << "full_window_ns,center_ns,passing,total_elastic,efficiency\n"
      << std::setprecision(15);
  for (size_t iw = 0; iw < full_widths.size(); ++iw) {
    const double efficiency = total ? double(passing[iw])/total : 0.;
    csv << full_widths[iw] << ',' << center << ',' << passing[iw] << ','
        << total << ',' << efficiency << '\n';
    std::cout << "window=" << full_widths[iw] << " ns center=" << center
              << " ns efficiency=" << passing[iw] << '/' << total
              << " (" << efficiency << ")\n";
  }
  std::cout << "both-present ceiling=" << both_present << '/' << total << " ("
            << (total ? double(both_present)/total : 0.) << ")\n";
}
