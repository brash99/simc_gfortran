#include <TFile.h>
#include <TTree.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

void analyze_hms_proton_trigger_efficiency(
    const char* input,
    const char* output="hms_proton_trigger_efficiency.csv") {
  constexpr int kMax = 4096;
  constexpr double kBeamEnergy = 6.37042664;
  constexpr double kProtonMass = 0.9382720813;
  constexpr double kCoreMean = -0.225806;
  constexpr double kCoreSigma = 0.202053;

  TFile file(input);
  auto* tree = static_cast<TTree*>(file.Get("T"));
  if (!tree) { std::cerr << "Missing T tree\n"; return; }

  double edtm = 0., ntrack = 0., etot = 0.;
  int nclus = 0, s1x = 0, s1y = 0, asum = 0, bsum = 0, csum = 0, dsum = 0;
  int prlo = 0, prhi = 0, shwr = 0;
  std::array<double,kMax> p{}, dp{}, clus_e{};
  tree->SetBranchAddress("T.hms.hEDTM_tdcTimeRaw", &edtm);
  tree->SetBranchAddress("H.dc.ntrack", &ntrack);
  tree->SetBranchAddress("H.gtr.p", p.data());
  tree->SetBranchAddress("H.gtr.dp", dp.data());
  tree->SetBranchAddress("Ndata.NPS.cal.clusE", &nclus);
  tree->SetBranchAddress("NPS.cal.clusE", clus_e.data());
  tree->SetBranchAddress("H.cal.etot", &etot);
  tree->SetBranchAddress("Ndata.T.hms.h1X_vecTdcTime", &s1x);
  tree->SetBranchAddress("Ndata.T.hms.h1Y_vecTdcTime", &s1y);
  tree->SetBranchAddress("Ndata.T.hms.hASUM_vecTdcTime", &asum);
  tree->SetBranchAddress("Ndata.T.hms.hBSUM_vecTdcTime", &bsum);
  tree->SetBranchAddress("Ndata.T.hms.hCSUM_vecTdcTime", &csum);
  tree->SetBranchAddress("Ndata.T.hms.hDSUM_vecTdcTime", &dsum);
  tree->SetBranchAddress("Ndata.T.hms.hPRLO_vecTdcTime", &prlo);
  tree->SetBranchAddress("Ndata.T.hms.hPRHI_vecTdcTime", &prhi);
  tree->SetBranchAddress("Ndata.T.hms.hSHWR_vecTdcTime", &shwr);

  long total = 0, have_s1x = 0, have_s1y = 0, have_both_s1 = 0;
  long have_asum = 0, have_bsum = 0, have_csum = 0, have_dsum = 0;
  long have_prlo = 0, have_prhi = 0, have_shwr = 0;
  std::vector<double> energies;
  for (Long64_t i = 0; i < tree->GetEntries(); ++i) {
    tree->GetEntry(i);
    if (edtm != 0. || ntrack != 1. || std::abs(dp[0]) >= 8. || p[0] <= 0. ||
        nclus <= 0 || nclus > kMax) continue;
    const double max_nps = *std::max_element(clus_e.begin(), clus_e.begin()+nclus);
    const double expected = kBeamEnergy+kProtonMass-
      std::sqrt(p[0]*p[0]+kProtonMass*kProtonMass);
    const double residual = max_nps-expected;
    if (residual < kCoreMean-3*kCoreSigma || residual > kCoreMean+3*kCoreSigma) continue;
    ++total;
    have_s1x += s1x > 0; have_s1y += s1y > 0; have_both_s1 += s1x > 0 && s1y > 0;
    have_asum += asum > 0; have_bsum += bsum > 0; have_csum += csum > 0; have_dsum += dsum > 0;
    have_prlo += prlo > 0; have_prhi += prhi > 0; have_shwr += shwr > 0;
    energies.push_back(etot);
  }
  std::sort(energies.begin(), energies.end());
  auto quantile = [&](double q) {
    return energies.empty() ? 0. : energies[std::min(energies.size()-1,
      static_cast<size_t>(q*(energies.size()-1)))];
  };
  std::ofstream csv(output);
  csv << "observable,passing,total,fraction\n" << std::setprecision(15);
  auto write = [&](const char* name, long count) {
    csv << name << ',' << count << ',' << total << ',' << (total ? double(count)/total : 0.) << '\n';
    std::cout << name << '=' << count << '/' << total << " (" << (total ? double(count)/total : 0.) << ")\n";
  };
  write("S1X_TDC_present", have_s1x); write("S1Y_TDC_present", have_s1y);
  write("S1X_and_S1Y_TDC_present", have_both_s1);
  write("ASUM_TDC_present", have_asum); write("BSUM_TDC_present", have_bsum);
  write("CSUM_TDC_present", have_csum); write("DSUM_TDC_present", have_dsum);
  write("PRLO_TDC_present", have_prlo); write("PRHI_TDC_present", have_prhi);
  write("SHWR_TDC_present", have_shwr);
  std::cout << "H.cal.etot quantiles GeV-equivalent: q01=" << quantile(.01)
            << " q05=" << quantile(.05) << " q50=" << quantile(.50)
            << " q95=" << quantile(.95) << std::endl;
}
