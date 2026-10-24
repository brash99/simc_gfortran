#include <TFile.h>
#include <TMath.h>
#include <TTree.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
struct ProtonTruth { long long id; double energy, px, py, pz; };
double NormalIntegral(double low, double high, double mean, double sigma) {
  if (high <= low) return 0.;
  const double scale = std::sqrt(2.) * sigma;
  return 0.5 * (std::erf((high-mean)/scale)-std::erf((low-mean)/scale));
}
}

void decompose_run6151_prompt_tail(
    const char *data_name, const char *geant_name, const char *truth_name,
    const char *output_name = "prompt_inelastic_run6151_decomposed.csv") {
  constexpr int kMax = 4096;
  constexpr double kBeam = 6.37042664;
  constexpr double kMp = 0.9382720813;
  constexpr double kDataMean = -0.225806;
  constexpr double kDataSigma = 0.202053;
  constexpr double kSimCoreMean = -0.150256745473;
  constexpr double kSimCoreSigma = 0.0702646857951;
  constexpr double kSmear = 0.189445; // quadrature match to data core width
  constexpr double kShift = kDataMean-kSimCoreMean;
  constexpr double kCoreLow = kDataMean-3.*kDataSigma;
  constexpr double kCoreHigh = kDataMean+3.*kDataSigma;
  constexpr std::array<double,3> thresholds{{3.4,3.6,3.9}};
  constexpr std::array<double,3> accidentals{{79.42348149044408,
      75.0959047984975,59.9885124125292}};

  std::ifstream truth_file(truth_name);
  std::vector<ProtonTruth> truth;
  std::string line;
  while (std::getline(truth_file,line)) {
    if (line.empty() || line[0]=='#' || line.rfind("event_id",0)==0) continue;
    for (char &c : line) if (c==',') c=' ';
    std::istringstream row(line); ProtonTruth x{};
    if (row >> x.id >> x.energy >> x.px >> x.py >> x.pz) truth.push_back(x);
  }
  TFile geant_file(geant_name,"READ");
  auto *geant=dynamic_cast<TTree*>(geant_file.Get("t_dvcs"));
  if (!geant || static_cast<Long64_t>(truth.size())!=geant->GetEntries()) {
    std::cerr << "SIMC/Geant entry mismatch\n"; return;
  }
  double edep[1080]{}, weight=0.;
  geant->SetBranchAddress("edep",edep);
  geant->SetBranchAddress("psf",&weight);
  std::array<double,3> sim_core{},sim_tail{};
  for (Long64_t i=0;i<geant->GetEntries();++i) {
    geant->GetEntry(i);
    double deposit=0.; for(double block:edep) deposit += block/1000.;
    const double expected=kBeam+kMp-truth[i].energy;
    const double mean=deposit-expected+kShift;
    for (size_t j=0;j<thresholds.size();++j) {
      const double threshold_residual=thresholds[j]-expected;
      const double accepted_low=std::max(threshold_residual,-10.);
      sim_core[j] += weight*NormalIntegral(std::max(accepted_low,kCoreLow),
                                           kCoreHigh,mean,kSmear);
      sim_tail[j] += weight*(NormalIntegral(accepted_low,kCoreLow,mean,kSmear)+
                             NormalIntegral(std::max(accepted_low,kCoreHigh),
                                            10.,mean,kSmear));
    }
  }

  TFile data_file(data_name,"READ");
  auto *data=dynamic_cast<TTree*>(data_file.Get("T"));
  if (!data) { std::cerr << "missing data tree\n"; return; }
  double edtm=0.,ntrack=0.,ctime=0.; int nclus=0,ns1x=0,ns1y=0;
  std::array<double,kMax> p{},dp{},clus{};
  data->SetBranchAddress("T.hms.hEDTM_tdcTimeRaw",&edtm);
  data->SetBranchAddress("H.dc.ntrack",&ntrack);
  data->SetBranchAddress("H.gtr.p",p.data());
  data->SetBranchAddress("H.gtr.dp",dp.data());
  data->SetBranchAddress("Ndata.NPS.cal.clusE",&nclus);
  data->SetBranchAddress("NPS.cal.clusE",clus.data());
  data->SetBranchAddress("Ndata.T.hms.h1X_vecTdcTime",&ns1x);
  data->SetBranchAddress("Ndata.T.hms.h1Y_vecTdcTime",&ns1y);
  data->SetBranchAddress("CTime.epCoinTime_NPS",&ctime);
  std::array<long,3> data_core{},data_tail{};
  for(Long64_t i=0;i<data->GetEntries();++i) {
    data->GetEntry(i);
    if(edtm!=0. || ntrack!=1. || !std::isfinite(dp[0]) ||
       !std::isfinite(p[0]) || !std::isfinite(ctime) ||
       std::abs(dp[0])>=8. || p[0]<=0. ||
       nclus<=0 || nclus>kMax || ns1x<=0 || ns1y<=0 ||
       std::abs(ctime-157.5)>=2.) continue;
    const double energy=*std::max_element(clus.begin(),clus.begin()+nclus);
    if (!std::isfinite(energy)) continue;
    const double expected=kBeam+kMp-std::sqrt(p[0]*p[0]+kMp*kMp);
    const double residual=energy-expected;
    for(size_t j=0;j<thresholds.size();++j) if(energy>=thresholds[j]) {
      if(residual>=kCoreLow && residual<=kCoreHigh) ++data_core[j];
      else ++data_tail[j];
    }
  }

  std::ofstream out(output_name);
  out << "threshold_GeV,data_core,data_outside_core,sim_tail_to_core,"
         "predicted_elastic_tail,integrated_accidentals,unexplained_prompt,"
         "unexplained_to_elastic_core\n" << std::setprecision(12);
  for(size_t j=0;j<thresholds.size();++j) {
    const double ratio=sim_tail[j]/sim_core[j];
    const double elastic_tail=data_core[j]*ratio;
    const double unexplained=data_tail[j]-elastic_tail-accidentals[j];
    out << thresholds[j] << ',' << data_core[j] << ',' << data_tail[j] << ','
        << ratio << ',' << elastic_tail << ',' << accidentals[j] << ','
        << unexplained << ',' << unexplained/data_core[j] << '\n';
    std::cout << "threshold=" << thresholds[j] << " data_tail=" << data_tail[j]
              << " elastic_template_tail=" << elastic_tail
              << " accidentals=" << accidentals[j]
              << " unexplained=" << unexplained
              << " ratio=" << unexplained/data_core[j] << '\n';
  }
}
