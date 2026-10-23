#include <TCanvas.h>
#include <TFile.h>
#include <TF1.h>
#include <TGraph.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TLegend.h>
#include <TProfile.h>
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
constexpr int kNBins = 5;
constexpr double kBeamEnergy = 6.37042664;
constexpr double kProtonMass = 0.9382720813;
constexpr std::array<double, kNBins + 1> kCurrentEdges{{0., 15., 22.5, 26.5, 30., 40.}};

struct Event {
  double current, elapsed, residual, offline, vtp, ratio, cluster_time;
  int nclus, nvtp;
};

int current_bin(double value) {
  for (int i = 0; i < kNBins; ++i)
    if (value >= kCurrentEdges[i] && value < kCurrentEdges[i + 1]) return i;
  return -1;
}
}  // namespace

// Within-run rate/pileup diagnostics for proton-tagged elastic candidates.
// Current is the event-latched BCM4A scaler value. Results characterize the
// accepted stream and do not by themselves provide an unbiased trigger rate.
void analyze_nps_run6151_rate(
    const char* input,
    const char* output_prefix = "nps_run6151_100k_rate") {
  TFile file(input, "READ");
  auto* tree = dynamic_cast<TTree*>(file.Get("T"));
  if (file.IsZombie() || !tree) {
    std::cerr << "Cannot read tree T from " << input << std::endl;
    return;
  }

  double edtm = 0., ntrack = 0., current = 0., scaler_time = 0.;
  int nclus = 0, nvtp = 0;
  std::array<double, kMax> p{}, dp{}, clus_e{}, clus_t{}, vtp_e{};
  tree->SetBranchAddress("T.hms.hEDTM_tdcTimeRaw", &edtm);
  tree->SetBranchAddress("H.dc.ntrack", &ntrack);
  tree->SetBranchAddress("H.gtr.p", p.data());
  tree->SetBranchAddress("H.gtr.dp", dp.data());
  tree->SetBranchAddress("H.BCM4A.scalerCurrent", &current);
  tree->SetBranchAddress("H.1MHz.scalerTime", &scaler_time);
  tree->SetBranchAddress("Ndata.NPS.cal.clusE", &nclus);
  tree->SetBranchAddress("NPS.cal.clusE", clus_e.data());
  tree->SetBranchAddress("NPS.cal.clusT", clus_t.data());
  tree->SetBranchAddress("Ndata.NPS.cal.vtpClusE", &nvtp);
  tree->SetBranchAddress("NPS.cal.vtpClusE", vtp_e.data());

  std::vector<Event> events;
  events.reserve(35000);
  double first_time = -1.;
  const Long64_t entries = tree->GetEntries();
  for (Long64_t i = 0; i < entries; ++i) {
    tree->GetEntry(i);
    if (edtm != 0. || ntrack != 1. || std::abs(dp[0]) >= 8. || p[0] <= 0. ||
        nclus <= 0 || nclus > kMax || nvtp < 0 || nvtp > kMax) continue;
    if (first_time < 0.) first_time = scaler_time;
    const int imax = std::distance(clus_e.begin(),
        std::max_element(clus_e.begin(), clus_e.begin() + nclus));
    const double max_vtp = nvtp > 0
        ? *std::max_element(vtp_e.begin(), vtp_e.begin() + nvtp) / 1000. : 0.;
    const double expected = kBeamEnergy + kProtonMass
        - std::sqrt(p[0] * p[0] + kProtonMass * kProtonMass);
    events.push_back({current, scaler_time - first_time, clus_e[imax] - expected,
                      clus_e[imax], max_vtp,
                      clus_e[imax] > 1.5 && max_vtp > 1.5 ? max_vtp / clus_e[imax] : 0.,
                      clus_t[imax], nclus, nvtp});
  }

  TH1D global("global", "", 240, -2., 1.);
  for (const auto& e : events) global.Fill(e.residual);
  const double peak = global.GetBinCenter(global.GetMaximumBin());
  TF1 fit1("fit1", "gaus", peak - .35, peak + .35);
  global.Fit(&fit1, "RQ0");
  TF1 fit2("fit2", "gaus", fit1.GetParameter(1) - 2. * std::abs(fit1.GetParameter(2)),
                             fit1.GetParameter(1) + 2. * std::abs(fit1.GetParameter(2)));
  global.Fit(&fit2, "RQ0");
  const double global_mean = fit2.GetParameter(1);
  const double global_sigma = std::abs(fit2.GetParameter(2));
  const double core_low = global_mean - 3. * global_sigma;
  const double core_high = global_mean + 3. * global_sigma;

  std::array<TH1D*, kNBins> residuals{};
  for (int b = 0; b < kNBins; ++b)
    residuals[b] = new TH1D(Form("residual_%d", b), "", 160, -1.4, .6);
  TH2D residual_current("residual_current", "Elastic residual versus beam current;BCM4A current (#muA);E_{NPS}-E_{e}^{expected} (GeV)", 100, 0, 40, 160, -1.4, .6);
  TProfile time_current("time_current", "Cluster time versus beam current;BCM4A current (#muA);mean maximum-cluster time (ns)", 80, 0, 40);

  std::array<long long, kNBins> count{}, core{}, pass35{}, pass37{}, multi_off{}, multi_vtp{}, ratio_n{};
  std::array<double, kNBins> sum_current{}, sum_ratio{}, sum_ratio2{};
  for (const auto& e : events) {
    const int b = current_bin(e.current);
    if (b < 0) continue;
    ++count[b]; sum_current[b] += e.current;
    residuals[b]->Fill(e.residual);
    residual_current.Fill(e.current, e.residual);
    time_current.Fill(e.current, e.cluster_time);
    if (e.nclus > 1) ++multi_off[b];
    if (e.nvtp > 1) ++multi_vtp[b];
    if (e.ratio > 0.) { ++ratio_n[b]; sum_ratio[b] += e.ratio; sum_ratio2[b] += e.ratio * e.ratio; }
    if (e.residual >= core_low && e.residual <= core_high) {
      ++core[b];
      if (e.vtp >= 3.5) ++pass35[b];
      if (e.vtp >= 3.7) ++pass37[b];
    }
  }

  std::array<double, kNBins> x{}, fit_mean{}, fit_sigma{}, eff35{}, eff37{}, fmulti_off{}, fmulti_vtp{}, mean_ratio{}, rms_ratio{};
  std::ofstream csv(std::string(output_prefix) + ".csv");
  csv << "current_low_uA,current_high_uA,mean_current_uA,candidates,elastic_core,fit_mean_GeV,fit_sigma_GeV,VTP_eff_3p5,VTP_eff_3p7,offline_multicluster_fraction,VTP_multiobject_fraction,mean_VTP_over_offline,rms_VTP_over_offline\n";
  csv << std::fixed << std::setprecision(6);
  for (int b = 0; b < kNBins; ++b) {
    x[b] = count[b] ? sum_current[b] / count[b] : .5 * (kCurrentEdges[b] + kCurrentEdges[b + 1]);
    const double local_peak = residuals[b]->GetBinCenter(residuals[b]->GetMaximumBin());
    TF1 local(Form("local_%d", b), "gaus", local_peak - .35, local_peak + .35);
    if (residuals[b]->GetEntries() > 100) residuals[b]->Fit(&local, "RQ0");
    fit_mean[b] = local.GetParameter(1);
    fit_sigma[b] = std::abs(local.GetParameter(2));
    eff35[b] = core[b] ? double(pass35[b]) / core[b] : 0.;
    eff37[b] = core[b] ? double(pass37[b]) / core[b] : 0.;
    fmulti_off[b] = count[b] ? double(multi_off[b]) / count[b] : 0.;
    fmulti_vtp[b] = count[b] ? double(multi_vtp[b]) / count[b] : 0.;
    mean_ratio[b] = ratio_n[b] ? sum_ratio[b] / ratio_n[b] : 0.;
    rms_ratio[b] = ratio_n[b] ? std::sqrt(std::max(0., sum_ratio2[b] / ratio_n[b] - mean_ratio[b] * mean_ratio[b])) : 0.;
    csv << kCurrentEdges[b] << ',' << kCurrentEdges[b + 1] << ',' << x[b] << ',' << count[b] << ',' << core[b] << ','
        << fit_mean[b] << ',' << fit_sigma[b] << ',' << eff35[b] << ',' << eff37[b] << ','
        << fmulti_off[b] << ',' << fmulti_vtp[b] << ',' << mean_ratio[b] << ',' << rms_ratio[b] << '\n';
  }

  int nfilled = 0;
  while (nfilled < kNBins && count[nfilled] > 0) ++nfilled;
  TGraph gmean(nfilled, x.data(), fit_mean.data()); gmean.SetTitle("Elastic peak versus beam current;BCM4A current (#muA);fitted residual mean (GeV)");
  TGraph gsigma(nfilled, x.data(), fit_sigma.data()); gsigma.SetTitle("Elastic width versus beam current;BCM4A current (#muA);fitted residual sigma (GeV)");
  TGraph ge35(nfilled, x.data(), eff35.data()); ge35.SetTitle("Elastic-core VTP efficiency;BCM4A current (#muA);efficiency"); ge35.SetMarkerColor(kBlue + 1); ge35.SetLineColor(kBlue + 1);
  TGraph ge37(nfilled, x.data(), eff37.data()); ge37.SetMarkerColor(kRed + 1); ge37.SetLineColor(kRed + 1);
  TGraph gmoff(nfilled, x.data(), fmulti_off.data()); gmoff.SetTitle("Multiplicity proxies;BCM4A current (#muA);fraction with multiplicity > 1"); gmoff.SetMarkerColor(kBlue + 1); gmoff.SetLineColor(kBlue + 1);
  TGraph gmvtp(nfilled, x.data(), fmulti_vtp.data()); gmvtp.SetMarkerColor(kRed + 1); gmvtp.SetLineColor(kRed + 1);

  for (auto* g : {&gmean, &gsigma, &ge35, &ge37, &gmoff, &gmvtp}) { g->SetMarkerStyle(20); g->SetLineWidth(2); }
  gStyle->SetOptStat(0);
  TCanvas canvas("canvas", "run 6151 rate dependence", 1500, 1000); canvas.Divide(3, 2);
  canvas.cd(1); residual_current.Draw("COLZ");
  canvas.cd(2); gmean.Draw("APL");
  canvas.cd(3); gsigma.Draw("APL");
  canvas.cd(4); ge35.SetMinimum(.7); ge35.SetMaximum(1.01); ge35.Draw("APL"); ge37.Draw("PL SAME"); auto* le = new TLegend(.15,.15,.45,.3); le->AddEntry(&ge35,"3.5 GeV","lp"); le->AddEntry(&ge37,"3.7 GeV","lp"); le->Draw();
  canvas.cd(5); gmoff.SetMinimum(0.); gmoff.SetMaximum(1.); gmoff.Draw("APL"); gmvtp.Draw("PL SAME"); auto* lm = new TLegend(.15,.7,.5,.85); lm->AddEntry(&gmoff,"offline clusters","lp"); lm->AddEntry(&gmvtp,"VTP objects","lp"); lm->Draw();
  canvas.cd(6); time_current.Draw();
  canvas.SaveAs((std::string(output_prefix) + ".png").c_str());
  canvas.SaveAs((std::string(output_prefix) + ".pdf").c_str());

  std::ofstream summary(std::string(output_prefix) + "_summary.txt");
  summary << std::fixed << std::setprecision(6)
          << "input=" << input << '\n' << "selected_candidates=" << events.size() << '\n'
          << "global_core_mean_GeV=" << global_mean << '\n' << "global_core_sigma_GeV=" << global_sigma << '\n'
          << "global_core_low_GeV=" << core_low << '\n' << "global_core_high_GeV=" << core_high << '\n'
          << "current_bins_uA=0,15,22.5,26.5,30,40\n"
          << "caveat=event-latched scaler current and accepted triggered events; trends are diagnostic, not unbiased rates\n";
}
