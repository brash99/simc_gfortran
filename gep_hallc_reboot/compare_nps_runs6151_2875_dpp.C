#include <TCanvas.h>
#include <TFile.h>
#include <TF1.h>
#include <TH1D.h>
#include <TLatex.h>
#include <TLegend.h>
#include <TLine.h>
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
constexpr double kPi = 3.14159265358979323846;

struct Event {
  double dpp;   // proton-angle prediction, percent of HMS central momentum
  double dpe;   // electron-angle prediction, percent of HMS central momentum
  double dphi;  // electron-proton acoplanarity, radians
};

struct Sample {
  std::vector<Event> events;
  long long ordinary = 0;
  long long one_track = 0;
  long long with_nps_cluster = 0;
};

struct Cuts {
  double dpe_mean = 0.;
  double dpe_sigma = 0.;
  double dphi_mean = 0.;
  double dphi_sigma = 0.;
  double dpp_mean = 0.;
  double dpp_sigma = 0.;
};

double WrapPi(double angle) {
  while (angle > kPi) angle -= 2. * kPi;
  while (angle <= -kPi) angle += 2. * kPi;
  return angle;
}

double ProtonMomentumFromTheta(double ebeam, double theta) {
  const double c = std::cos(theta);
  const double den = (ebeam + kMp) * (ebeam + kMp) - ebeam * ebeam * c * c;
  return 2. * kMp * ebeam * (ebeam + kMp) * c / den;
}

double ProtonMomentumFromElectronTheta(double ebeam, double theta_e) {
  const double eprime = ebeam / (1. + (ebeam / kMp) * (1. - std::cos(theta_e)));
  const double q2 = 2. * ebeam * eprime * (1. - std::cos(theta_e));
  const double tau = q2 / (4. * kMp * kMp);
  return std::sqrt(q2 * (1. + tau));
}

Sample ReadSample(const char* filename, double ebeam, double pcentral) {
  Sample sample;
  TFile file(filename, "READ");
  auto* tree = dynamic_cast<TTree*>(file.Get("T"));
  if (!tree) {
    std::cerr << "Cannot read T from " << filename << '\n';
    return sample;
  }

  double edtm = 0., ntrack = 0.;
  int nnps = 0;
  std::array<double, kMax> hp{}, hpx{}, hpy{}, hpz{};
  std::array<double, kMax> np{}, npx{}, npy{}, npz{};
  tree->SetBranchStatus("*", 0);
  for (const char* branch : {"T.hms.hEDTM_tdcTimeRaw", "H.dc.ntrack",
       "H.gtr.p", "H.gtr.px", "H.gtr.py", "H.gtr.pz",
       "Ndata.NPS.cal.trk.p", "NPS.cal.trk.p", "NPS.cal.trk.px",
       "NPS.cal.trk.py", "NPS.cal.trk.pz"})
    tree->SetBranchStatus(branch, 1);
  tree->SetBranchAddress("T.hms.hEDTM_tdcTimeRaw", &edtm);
  tree->SetBranchAddress("H.dc.ntrack", &ntrack);
  tree->SetBranchAddress("H.gtr.p", hp.data());
  tree->SetBranchAddress("H.gtr.px", hpx.data());
  tree->SetBranchAddress("H.gtr.py", hpy.data());
  tree->SetBranchAddress("H.gtr.pz", hpz.data());
  tree->SetBranchAddress("Ndata.NPS.cal.trk.p", &nnps);
  tree->SetBranchAddress("NPS.cal.trk.p", np.data());
  tree->SetBranchAddress("NPS.cal.trk.px", npx.data());
  tree->SetBranchAddress("NPS.cal.trk.py", npy.data());
  tree->SetBranchAddress("NPS.cal.trk.pz", npz.data());

  sample.events.reserve(tree->GetEntries() / 2);
  for (Long64_t i = 0; i < tree->GetEntries(); ++i) {
    tree->GetEntry(i);
    if (edtm != 0.) continue;
    ++sample.ordinary;
    if (ntrack != 1. || hp[0] <= 0.) continue;
    ++sample.one_track;
    if (nnps <= 0 || nnps > kMax) continue;
    ++sample.with_nps_cluster;

    const int ie = std::distance(np.begin(),
        std::max_element(np.begin(), np.begin() + nnps));
    const double hnorm = std::sqrt(hpx[0]*hpx[0] + hpy[0]*hpy[0] + hpz[0]*hpz[0]);
    const double enorm = std::sqrt(npx[ie]*npx[ie] + npy[ie]*npy[ie] + npz[ie]*npz[ie]);
    if (hnorm <= 0. || enorm <= 0.) continue;

    const double theta_p = std::acos(std::clamp(hpz[0] / hnorm, -1., 1.));
    const double theta_e = std::acos(std::clamp(npz[ie] / enorm, -1., 1.));
    const double phi_p = std::atan2(hpy[0], hpx[0]);
    const double phi_e = std::atan2(npy[ie], npx[ie]);
    const double dpp = 100. * (hp[0] - ProtonMomentumFromTheta(ebeam, theta_p)) / pcentral;
    const double dpe = 100. * (hp[0] - ProtonMomentumFromElectronTheta(ebeam, theta_e)) / pcentral;
    const double dphi = WrapPi(phi_e - phi_p - kPi);
    if (std::isfinite(dpp) && std::isfinite(dpe) && std::isfinite(dphi))
      sample.events.push_back({dpp, dpe, dphi});
  }
  return sample;
}

std::pair<double,double> FitCore(const std::vector<Event>& events, int member,
                                 double lo, double hi, const char* name) {
  TH1D h(name, "", 240, lo, hi);
  for (const auto& event : events) {
    const double x = member == 0 ? event.dpp : (member == 1 ? event.dpe : event.dphi);
    h.Fill(x);
  }
  const int peak_bin = h.GetMaximumBin();
  const double peak = h.GetBinCenter(peak_bin);
  const double initial_half_width = member == 2 ? 0.025 : 1.5;
  TF1 fit((std::string(name) + "_fit").c_str(), "gaus",
          peak - initial_half_width, peak + initial_half_width);
  fit.SetParameters(h.GetBinContent(peak_bin), peak, initial_half_width / 3.);
  fit.SetParLimits(1, peak - initial_half_width, peak + initial_half_width);
  fit.SetParLimits(2, member == 2 ? 0.001 : 0.03, initial_half_width);
  h.Fit(&fit, "QNRB");
  double mean = fit.GetParameter(1);
  double sigma = std::abs(fit.GetParameter(2));
  if (!(sigma > 0.) || !std::isfinite(sigma)) return {peak, initial_half_width / 2.};
  return {mean, sigma};
}

Cuts DetermineCuts(const Sample& sample, const char* tag) {
  Cuts cuts;
  // Determine the orthogonal-resolution scales from the unmistakable central
  // delta-p_p core.  The final plotted classification itself uses only dpe
  // and dphi, as in Fig. 6 of the GEp-III archival paper.
  std::vector<Event> core;
  for (const auto& event : sample.events)
    if (std::abs(event.dpp) < 1.5) core.push_back(event);
  auto dpe = FitCore(core, 1, -25., 15., (std::string(tag)+"_dpe").c_str());
  auto dphi = FitCore(core, 2, -0.20, 0.20, (std::string(tag)+"_dphi").c_str());
  cuts.dpe_mean = dpe.first; cuts.dpe_sigma = dpe.second;
  cuts.dphi_mean = dphi.first; cuts.dphi_sigma = dphi.second;

  std::vector<Event> selected;
  for (const auto& event : sample.events)
    if (std::abs(event.dpe-cuts.dpe_mean) < 3.*cuts.dpe_sigma &&
        std::abs(event.dphi-cuts.dphi_mean) < 3.*cuts.dphi_sigma)
      selected.push_back(event);
  auto dpp = FitCore(selected, 0, -25., 15., (std::string(tag)+"_dpp").c_str());
  cuts.dpp_mean = dpp.first; cuts.dpp_sigma = dpp.second;
  return cuts;
}

bool ElasticSelected(const Event& event, const Cuts& cuts) {
  return std::abs(event.dpe-cuts.dpe_mean) < 3.*cuts.dpe_sigma &&
         std::abs(event.dphi-cuts.dphi_mean) < 3.*cuts.dphi_sigma;
}

void DrawPanel(TPad* pad, const Sample& sample, const Cuts& cuts,
               const char* tag, const char* subtitle) {
  pad->cd();
  pad->SetLogy();
  pad->SetLeftMargin(.12); pad->SetRightMargin(.035);
  pad->SetBottomMargin(.13); pad->SetTopMargin(.10);
  auto* all = new TH1D((std::string("all_")+tag).c_str(), "", 120, -30., 15.);
  auto* pass = new TH1D((std::string("pass_")+tag).c_str(), "", 120, -30., 15.);
  auto* reject = new TH1D((std::string("reject_")+tag).c_str(), "", 120, -30., 15.);
  for (const auto& event : sample.events) {
    all->Fill(event.dpp);
    (ElasticSelected(event, cuts) ? pass : reject)->Fill(event.dpp);
  }
  all->SetMinimum(.7); all->SetMaximum(4.0*all->GetMaximum());
  all->GetXaxis()->SetTitle("#delta p_{p} = 100[p_{p}-p_{p}(#theta_{p})]/p_{0}  (%)");
  all->GetYaxis()->SetTitle("events / 0.375%");
  all->SetLineColor(kRed+1); all->SetMarkerColor(kRed+1);
  all->SetMarkerStyle(24); all->SetMarkerSize(.65); all->SetLineWidth(1);
  reject->SetLineColor(kBlue+1); reject->SetMarkerColor(kBlue+1);
  reject->SetMarkerStyle(26); reject->SetMarkerSize(.65);
  pass->SetLineColor(kBlack); pass->SetMarkerColor(kBlack);
  pass->SetMarkerStyle(20); pass->SetMarkerSize(.60);
  all->Draw("E1"); reject->Draw("E1 SAME"); pass->Draw("E1 SAME");

  const double ymin = .7, ymax = all->GetMaximum();
  for (double x : {cuts.dpp_mean-3.*cuts.dpp_sigma, cuts.dpp_mean+3.*cuts.dpp_sigma}) {
    auto* line = new TLine(x, ymin, x, ymax);
    line->SetLineStyle(2); line->SetLineWidth(2); line->Draw();
  }
  TLatex text; text.SetNDC(); text.SetTextSize(.042);
  text.DrawLatex(.15, .92, subtitle);
  text.SetTextSize(.031);
  text.DrawLatex(.15, .855, Form("N=%zu ordinary one-track candidates with NPS cluster",
                                sample.events.size()));
  text.DrawLatex(.15, .81, Form("selection: |#delta p_{e}-#mu|<3#sigma and |#delta#phi-#mu|<3#sigma"));
  text.DrawLatex(.15, .765, Form("selection half-widths: %.2f%% and %.1f mrad",
                                3.*cuts.dpe_sigma, 3000.*cuts.dphi_sigma));
  if (std::string(tag) == "6151") {
    auto* legend = new TLegend(.59, .25, .95, .42);
    legend->SetBorderSize(0); legend->SetFillStyle(0); legend->SetTextSize(.033);
    legend->AddEntry(all, "all candidates", "pe");
    legend->AddEntry(pass, "passes other two cuts", "pe");
    legend->AddEntry(reject, "rejected", "pe");
    legend->Draw();
  }
}
}

void compare_nps_runs6151_2875_dpp(
    const char* run6151,
    const char* run2875,
    const char* output = "nps_runs6151_2875_dpp_comparison") {
  gStyle->SetOptStat(0);
  gStyle->SetOptFit(0);
  gStyle->SetEndErrorSize(0);

  const Sample s6151 = ReadSample(run6151, 6.37042664, 2.639);
  const Sample s2875 = ReadSample(run2875, 10.53941793, 4.0872);
  const Cuts c6151 = DetermineCuts(s6151, "6151");
  const Cuts c2875 = DetermineCuts(s2875, "2875");

  TCanvas canvas("dpp_comparison", "GEp-III-style delta-p comparison", 1500, 670);
  canvas.Divide(2, 1, .002, .002);
  DrawPanel((TPad*)canvas.cd(1), s6151, c6151, "6151",
            "Run 6151: E_{beam}=6.370 GeV, Q^{2}#approx3.74 GeV^{2}");
  DrawPanel((TPad*)canvas.cd(2), s2875, c2875, "2875",
            "Run 2875: E_{beam}=10.539 GeV, Q^{2}#approx6.51 GeV^{2}");
  canvas.SaveAs((std::string(output) + ".png").c_str());
  canvas.SaveAs((std::string(output) + ".pdf").c_str());

  std::ofstream summary(std::string(output) + "_summary.csv");
  summary << "run,ordinary,one_hms_track,with_nps_cluster,dpe_mean_pct,dpe_sigma_pct,"
             "dphi_mean_rad,dphi_sigma_rad,dpp_mean_pct,dpp_sigma_pct,selected,rejected\n";
  auto write = [&](int run, const Sample& sample, const Cuts& cuts) {
    long long selected = 0;
    for (const auto& event : sample.events) selected += ElasticSelected(event, cuts);
    summary << std::setprecision(10) << run << ',' << sample.ordinary << ','
            << sample.one_track << ',' << sample.with_nps_cluster << ','
            << cuts.dpe_mean << ',' << cuts.dpe_sigma << ','
            << cuts.dphi_mean << ',' << cuts.dphi_sigma << ','
            << cuts.dpp_mean << ',' << cuts.dpp_sigma << ','
            << selected << ',' << sample.events.size()-selected << '\n';
    std::cout << "run=" << run << " candidates=" << sample.events.size()
              << " selected=" << selected << " dpp=" << cuts.dpp_mean
              << "+/-" << cuts.dpp_sigma << "% dpe=" << cuts.dpe_mean
              << "+/-" << cuts.dpe_sigma << "% dphi=" << 1000.*cuts.dphi_mean
              << "+/-" << 1000.*cuts.dphi_sigma << " mrad\n";
  };
  write(6151, s6151, c6151);
  write(2875, s2875, c2875);
}
