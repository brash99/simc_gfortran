#include <TCanvas.h>
#include <TFile.h>
#include <TF1.h>
#include <TGraph.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TStyle.h>
#include <TTree.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {
constexpr int kMax = 4096;
constexpr double kMp = 0.9382720813;
constexpr std::array<double, 9> kEdges{{0., 15., 22.5, 26.5, 30., 35., 40., 50., 100.}};
struct Candidate {
  double current, residual;
  int nclus;
  double secondary_e, dt, dr;
};
int bin_for(double current) {
  for (int i = 0; i + 1 < int(kEdges.size()); ++i)
    if (current >= kEdges[i] && current < kEdges[i + 1]) return i;
  return -1;
}
}

// Study additional offline clusters in a proton-tagged elastic sample.
// |dt|<4 ns and dr<10 cm are diagnostic categories, not final pileup cuts.
void analyze_nps_secondary_clusters(const char* input, const char* output_prefix,
                                    double beam_energy, double nominal_elastic_energy) {
  TFile file(input, "READ");
  auto* tree = dynamic_cast<TTree*>(file.Get("T"));
  if (file.IsZombie() || !tree) { std::cerr << "Cannot read " << input << std::endl; return; }

  double edtm = 0., ntrack = 0., current = 0.;
  int nclus = 0;
  std::array<double, kMax> p{}, dp{}, energy{}, time{}, x{}, y{};
  tree->SetBranchAddress("T.hms.hEDTM_tdcTimeRaw", &edtm);
  tree->SetBranchAddress("H.dc.ntrack", &ntrack);
  tree->SetBranchAddress("H.gtr.p", p.data());
  tree->SetBranchAddress("H.gtr.dp", dp.data());
  tree->SetBranchAddress("H.BCM4A.scalerCurrent", &current);
  tree->SetBranchAddress("Ndata.NPS.cal.clusE", &nclus);
  tree->SetBranchAddress("NPS.cal.clusE", energy.data());
  tree->SetBranchAddress("NPS.cal.clusT", time.data());
  tree->SetBranchAddress("NPS.cal.clusX", x.data());
  tree->SetBranchAddress("NPS.cal.clusY", y.data());

  std::vector<Candidate> candidates;
  TH1D residuals("residuals", "", 240, -.45 * nominal_elastic_energy, .2 * nominal_elastic_energy);
  for (Long64_t entry = 0; entry < tree->GetEntries(); ++entry) {
    tree->GetEntry(entry);
    if (edtm != 0. || ntrack != 1. || std::abs(dp[0]) >= 8. || p[0] <= 0. || nclus <= 0 || nclus > kMax) continue;
    const int primary = std::distance(energy.begin(), std::max_element(energy.begin(), energy.begin() + nclus));
    const double expected = beam_energy + kMp - std::sqrt(p[0] * p[0] + kMp * kMp);
    const double residual = energy[primary] - expected;
    residuals.Fill(residual);
    int secondary = -1;
    for (int i = 0; i < nclus; ++i)
      if (i != primary && (secondary < 0 || energy[i] > energy[secondary])) secondary = i;
    candidates.push_back({current, residual, nclus,
      secondary >= 0 ? energy[secondary] : 0.,
      secondary >= 0 ? time[secondary] - time[primary] : 0.,
      secondary >= 0 ? std::hypot(x[secondary] - x[primary], y[secondary] - y[primary]) : 0.});
  }

  const double peak = residuals.GetBinCenter(residuals.GetMaximumBin());
  TF1 fit1("fit1", "gaus", peak - .08 * nominal_elastic_energy, peak + .08 * nominal_elastic_energy);
  residuals.Fit(&fit1, "RQ0");
  TF1 fit2("fit2", "gaus", fit1.GetParameter(1) - 2. * std::abs(fit1.GetParameter(2)),
                             fit1.GetParameter(1) + 2. * std::abs(fit1.GetParameter(2)));
  residuals.Fit(&fit2, "RQ0");
  const double mean = fit2.GetParameter(1), sigma = std::abs(fit2.GetParameter(2));
  const double low = mean - 3. * sigma, high = mean + 3. * sigma;

  constexpr int nb = kEdges.size() - 1;
  std::array<long long, nb> core{}, multi{}, energetic{}, energetic_far{}, energetic_outtime{}, energetic_far_outtime{}, intime{}, near{}, intime_near{};
  std::array<double, nb> sum_current{}, sum_secondary{};
  TH1D h_secondary("h_secondary", "Highest additional cluster;secondary-cluster energy (GeV);events", 160, 0, .5 * nominal_elastic_energy);
  TH1D h_dt("h_dt", "Secondary minus primary cluster time;#Deltat (ns);events", 240, -120, 120);
  TH1D h_dr("h_dr", "Secondary-primary separation;#Deltar (cm);events", 160, 0, 100);
  TH2D h_dt_dr("h_dt_dr", "Additional-cluster topology;#Deltar (cm);#Deltat (ns)", 100, 0, 100, 160, -80, 80);
  TH2D h_e_current("h_e_current", "Additional-cluster energy versus current;BCM4A current (#muA);secondary energy (GeV)", 100, 0, 50, 120, 0, .5 * nominal_elastic_energy);
  for (const auto& c : candidates) {
    if (c.residual < low || c.residual > high) continue;
    const int b = bin_for(c.current); if (b < 0) continue;
    ++core[b]; sum_current[b] += c.current;
    if (c.nclus <= 1) continue;
    ++multi[b]; sum_secondary[b] += c.secondary_e;
    if (c.secondary_e >= .2) {
      ++energetic[b];
      if (c.dr >= 10.) ++energetic_far[b];
      if (std::abs(c.dt) >= 4.) ++energetic_outtime[b];
      if (c.dr >= 10. && std::abs(c.dt) >= 4.) ++energetic_far_outtime[b];
    }
    if (std::abs(c.dt) < 4.) ++intime[b];
    if (c.dr < 10.) ++near[b];
    if (std::abs(c.dt) < 4. && c.dr < 10.) ++intime_near[b];
    h_secondary.Fill(c.secondary_e); h_dt.Fill(c.dt); h_dr.Fill(c.dr);
    h_dt_dr.Fill(c.dr, c.dt); h_e_current.Fill(c.current, c.secondary_e);
  }

  std::ofstream csv(std::string(output_prefix) + ".csv");
  csv << "current_low_uA,current_high_uA,mean_current_uA,elastic_core,multicluster,multicluster_fraction,mean_secondary_energy_GeV,secondary_ge_0p2_of_multicluster,secondary_ge_0p2_of_core,energetic_far_of_core,energetic_outtime_of_core,energetic_far_outtime_of_core,intime_4ns_fraction,near_10cm_fraction,intime_and_near_fraction\n";
  csv << std::fixed << std::setprecision(6);
  std::vector<double> gx, gy;
  for (int b = 0; b < nb; ++b) {
    if (!core[b]) continue;
    const double mc = multi[b] ? double(multi[b]) : 1.;
    const double mean_current = sum_current[b] / core[b];
    const double fraction = double(multi[b]) / core[b];
    gx.push_back(mean_current); gy.push_back(fraction);
    csv << kEdges[b] << ',' << kEdges[b + 1] << ',' << mean_current << ',' << core[b] << ',' << multi[b] << ',' << fraction << ','
        << sum_secondary[b] / mc << ',' << energetic[b] / mc << ',' << double(energetic[b]) / core[b] << ','
        << double(energetic_far[b]) / core[b] << ',' << double(energetic_outtime[b]) / core[b] << ','
        << double(energetic_far_outtime[b]) / core[b] << ',' << intime[b] / mc << ','
        << near[b] / mc << ',' << intime_near[b] / mc << '\n';
  }
  std::ofstream summary(std::string(output_prefix) + "_summary.txt");
  summary << std::fixed << std::setprecision(6) << "input=" << input << '\n'
          << "beam_energy_GeV=" << beam_energy << '\n' << "nominal_elastic_energy_GeV=" << nominal_elastic_energy << '\n'
          << "track_cluster_candidates=" << candidates.size() << '\n' << "elastic_core_mean_GeV=" << mean << '\n'
          << "elastic_core_sigma_GeV=" << sigma << '\n' << "elastic_core_low_GeV=" << low << '\n'
          << "elastic_core_high_GeV=" << high << '\n'
          << "diagnostic_definitions=intime abs(dt)<4 ns; near dr<10 cm; energetic secondary E>=0.2 GeV\n";

  TGraph gmulti(gx.size(), gx.data(), gy.data());
  gmulti.SetTitle("Offline multi-cluster probability;BCM4A current (#muA);fraction");
  gmulti.SetMarkerStyle(20); gmulti.SetLineWidth(2);
  gStyle->SetOptStat(0);
  TCanvas canvas("canvas", "NPS secondary clusters", 1500, 1000); canvas.Divide(3, 2);
  canvas.cd(1); h_secondary.Draw(); canvas.cd(2); h_dt.Draw(); canvas.cd(3); h_dr.Draw();
  canvas.cd(4); h_dt_dr.Draw("COLZ"); canvas.cd(5); h_e_current.Draw("COLZ"); canvas.cd(6); gmulti.Draw("APL");
  canvas.SaveAs((std::string(output_prefix) + ".png").c_str());
  canvas.SaveAs((std::string(output_prefix) + ".pdf").c_str());
}
