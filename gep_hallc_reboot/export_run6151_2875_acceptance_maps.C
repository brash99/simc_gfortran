#include <TFile.h>
#include <TH2D.h>
#include <TTree.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>

namespace {
constexpr int kMax = 4096;
constexpr double kMp = 0.9382720813;

double ProtonMomentumFromTheta(double ebeam, double theta) {
  const double c = std::cos(theta);
  return 2. * kMp * ebeam * (ebeam + kMp) * c /
      ((ebeam + kMp) * (ebeam + kMp) - ebeam * ebeam * c * c);
}

void FillRun(const char* filename, int run, double ebeam, double pcentral,
             double pmin, double pmax, double thmin, double thmax,
             TFile& output) {
  TFile input(filename, "READ");
  auto* tree = dynamic_cast<TTree*>(input.Get("T"));
  if (!tree) {
    std::cerr << "Cannot read T from " << filename << '\n';
    return;
  }

  auto* hms_all = new TH2D(Form("data_hms_all_%d", run), "", 100, thmin, thmax,
                           100, pmin, pmax);
  auto* hms_core = new TH2D(Form("data_hms_core_%d", run), "", 100, thmin, thmax,
                            100, pmin, pmax);
  auto* nps_core = new TH2D(Form("data_nps_core_%d", run), "", 100, -50., 50.,
                            100, -45., 45.);

  double edtm = 0., ntrack = 0.;
  int nclus = 0;
  std::array<double, kMax> p{}, px{}, py{}, pz{}, energy{}, x{}, y{};
  tree->SetBranchStatus("*", 0);
  for (const char* branch : {"T.hms.hEDTM_tdcTimeRaw", "H.dc.ntrack",
       "H.gtr.p", "H.gtr.px", "H.gtr.py", "H.gtr.pz",
       "Ndata.NPS.cal.clusE", "NPS.cal.clusE", "NPS.cal.clusX", "NPS.cal.clusY"})
    tree->SetBranchStatus(branch, 1);
  tree->SetBranchAddress("T.hms.hEDTM_tdcTimeRaw", &edtm);
  tree->SetBranchAddress("H.dc.ntrack", &ntrack);
  tree->SetBranchAddress("H.gtr.p", p.data());
  tree->SetBranchAddress("H.gtr.px", px.data());
  tree->SetBranchAddress("H.gtr.py", py.data());
  tree->SetBranchAddress("H.gtr.pz", pz.data());
  tree->SetBranchAddress("Ndata.NPS.cal.clusE", &nclus);
  tree->SetBranchAddress("NPS.cal.clusE", energy.data());
  tree->SetBranchAddress("NPS.cal.clusX", x.data());
  tree->SetBranchAddress("NPS.cal.clusY", y.data());

  long long all = 0, core = 0, footprint = 0;
  for (Long64_t entry = 0; entry < tree->GetEntries(); ++entry) {
    tree->GetEntry(entry);
    if (edtm != 0. || ntrack != 1. || p[0] <= 0.) continue;
    const double norm = std::sqrt(px[0]*px[0] + py[0]*py[0] + pz[0]*pz[0]);
    if (norm <= 0.) continue;
    const double theta = std::acos(std::clamp(pz[0] / norm, -1., 1.));
    const double theta_deg = theta * 180. / std::acos(-1.);
    const double dpp = 100. * (p[0] - ProtonMomentumFromTheta(ebeam, theta)) / pcentral;
    hms_all->Fill(theta_deg, p[0]);
    ++all;
    if (std::abs(dpp) >= 2.) continue;
    hms_core->Fill(theta_deg, p[0]);
    ++core;
    if (nclus <= 0 || nclus > kMax) continue;
    const int imax = std::distance(energy.begin(),
        std::max_element(energy.begin(), energy.begin() + nclus));
    nps_core->Fill(x[imax], y[imax]);
    ++footprint;
  }

  output.cd();
  hms_all->Write();
  hms_core->Write();
  nps_core->Write();
  std::cout << "run=" << run << " hms_all=" << all << " hms_core=" << core
            << " nps_core=" << footprint << '\n';
  delete hms_all;
  delete hms_core;
  delete nps_core;
}
}

void export_run6151_2875_acceptance_maps(
    const char* run6151,
    const char* run2875,
    const char* output_name = "run6151_2875_acceptance_data.root") {
  TFile output(output_name, "RECREATE");
  FillRun(run6151, 6151, 6.37042664, 2.639, 2.1, 3.05, 32.5, 40.5, output);
  FillRun(run2875, 2875, 10.53941793, 4.0872, 3.2, 4.75, 26.0, 34.0, output);
  output.Close();
}
