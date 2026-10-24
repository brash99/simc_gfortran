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

void analyze_prompt_inelastic(
    const char* input,
    const char* output="prompt_inelastic_run6151.csv") {
  constexpr int kMax = 4096;
  constexpr double kBeamEnergy = 6.37042664;
  constexpr double kProtonMass = 0.9382720813;
  constexpr double kElasticMean = -0.225806;
  constexpr double kElasticSigma = 0.202053;
  constexpr double kBucketSpacing = 4.0;
  constexpr double kBucketHalfWidth = 2.0;
  constexpr int kFirstSideBucket = 3;
  constexpr int kLastSideBucket = 10;
  constexpr int kSideBuckets = 2*(kLastSideBucket-kFirstSideBucket+1);
  constexpr std::array<double,3> kThresholds{{3.4, 3.6, 3.9}};

  TFile file(input);
  auto* tree = static_cast<TTree*>(file.Get("T"));
  if (!tree) { std::cerr << "Missing T tree\n"; return; }

  double edtm = 0., ntrack = 0., ctime = 0., scaler_time = 0.;
  int nclus = 0, ns1x = 0, ns1y = 0;
  std::array<double,kMax> p{}, dp{}, clus_e{};
  tree->SetBranchAddress("T.hms.hEDTM_tdcTimeRaw", &edtm);
  tree->SetBranchAddress("H.dc.ntrack", &ntrack);
  tree->SetBranchAddress("H.gtr.p", p.data());
  tree->SetBranchAddress("H.gtr.dp", dp.data());
  tree->SetBranchAddress("Ndata.NPS.cal.clusE", &nclus);
  tree->SetBranchAddress("NPS.cal.clusE", clus_e.data());
  tree->SetBranchAddress("Ndata.T.hms.h1X_vecTdcTime", &ns1x);
  tree->SetBranchAddress("Ndata.T.hms.h1Y_vecTdcTime", &ns1y);
  tree->SetBranchAddress("CTime.epCoinTime_NPS", &ctime);
  tree->SetBranchAddress("H.1MHz.scalerTime", &scaler_time);

  TH1D timing("timing", "Coincidence time;ns;events", 2000, -1000., 1000.);
  double min_time = std::numeric_limits<double>::infinity();
  double max_time = -std::numeric_limits<double>::infinity();
  for (Long64_t i = 0; i < tree->GetEntries(); ++i) {
    tree->GetEntry(i);
    min_time = std::min(min_time, scaler_time);
    max_time = std::max(max_time, scaler_time);
    if (edtm != 0. || ntrack != 1. || std::abs(dp[0]) >= 8. ||
        nclus <= 0 || nclus > kMax || ns1x <= 0 || ns1y <= 0) continue;
    timing.Fill(ctime);
  }
  const double center = timing.GetBinCenter(timing.GetMaximumBin());
  const double elapsed = max_time-min_time;

  struct Counts {
    long central_all = 0, side_all = 0;
    long central_elastic = 0, side_elastic = 0;
  };
  std::array<Counts,kThresholds.size()> counts{};

  for (Long64_t i = 0; i < tree->GetEntries(); ++i) {
    tree->GetEntry(i);
    if (edtm != 0. || ntrack != 1. || std::abs(dp[0]) >= 8. || p[0] <= 0. ||
        nclus <= 0 || nclus > kMax || ns1x <= 0 || ns1y <= 0) continue;
    const double max_nps = *std::max_element(clus_e.begin(), clus_e.begin()+nclus);
    const double expected = kBeamEnergy+kProtonMass-
      std::sqrt(p[0]*p[0]+kProtonMass*kProtonMass);
    const double residual = max_nps-expected;
    const bool elastic = std::abs(residual-kElasticMean) <= 3*kElasticSigma;
    const double dt = ctime-center;
    const bool central = std::abs(dt) < kBucketHalfWidth;
    bool side = false;
    for (int bucket = kFirstSideBucket; bucket <= kLastSideBucket; ++bucket)
      side = side || std::abs(dt-bucket*kBucketSpacing) < kBucketHalfWidth
                  || std::abs(dt+bucket*kBucketSpacing) < kBucketHalfWidth;

    for (size_t it = 0; it < kThresholds.size(); ++it) {
      if (max_nps < kThresholds[it]) continue;
      if (central) {
        ++counts[it].central_all;
        counts[it].central_elastic += elastic;
      }
      if (side) {
        ++counts[it].side_all;
        counts[it].side_elastic += elastic;
      }
    }
  }

  std::ofstream csv(output);
  csv << "threshold_GeV,prompt_center_ns,elapsed_s,central_all,side_all_total,"
         "side_bucket_count,random_all_per_bucket,prompt_excess_all,"
         "central_elastic,side_elastic_total,random_elastic_per_bucket,"
         "prompt_excess_elastic,prompt_excess_inelastic,"
         "inelastic_to_elastic_ratio,recorded_inelastic_excess_Hz\n"
      << std::setprecision(15);
  for (size_t it = 0; it < kThresholds.size(); ++it) {
    const auto& c = counts[it];
    const double random_all = double(c.side_all)/kSideBuckets;
    const double random_elastic = double(c.side_elastic)/kSideBuckets;
    const double excess_all = c.central_all-random_all;
    const double excess_elastic = c.central_elastic-random_elastic;
    const double excess_inelastic = excess_all-excess_elastic;
    csv << kThresholds[it] << ',' << center << ',' << elapsed << ','
        << c.central_all << ',' << c.side_all << ',' << kSideBuckets << ','
        << random_all << ',' << excess_all << ',' << c.central_elastic << ','
        << c.side_elastic << ',' << random_elastic << ',' << excess_elastic << ','
        << excess_inelastic << ','
        << (excess_elastic > 0 ? excess_inelastic/excess_elastic : 0.) << ','
        << (elapsed > 0 ? excess_inelastic/elapsed : 0.) << '\n';
    std::cout << "threshold=" << kThresholds[it] << " center=" << center
              << " ns correlated_all=" << excess_all
              << " elastic=" << excess_elastic
              << " inelastic=" << excess_inelastic
              << " inelastic/elastic="
              << (excess_elastic > 0 ? excess_inelastic/excess_elastic : 0.)
              << '\n';
  }
}
