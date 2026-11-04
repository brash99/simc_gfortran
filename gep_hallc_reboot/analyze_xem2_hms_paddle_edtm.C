#include <TCanvas.h>
#include <TFile.h>
#include <TH1D.h>
#include <TLegend.h>
#include <TStyle.h>
#include <TTree.h>

#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {
constexpr int kMaxHits = 512;
constexpr double kTdcWindowNs = 1200.0 * 0.09766; // run-17193 HMS hodo cuts

struct Plane {
  int npos = 0, nneg = 0;
  double pc[kMaxHits] = {}, pt[kMaxHits] = {};
  double nc[kMaxHits] = {}, nt[kMaxHits] = {};
};

struct Pair {
  double dt = 0.0;
  double mean = 0.0;
};

std::vector<Pair> same_paddle_pairs(const Plane& p) {
  std::vector<Pair> result;
  for (int i = 0; i < p.npos && i < kMaxHits; ++i) {
    for (int j = 0; j < p.nneg && j < kMaxHits; ++j) {
      if (std::lround(p.pc[i]) != std::lround(p.nc[j])) continue;
      result.push_back({p.pt[i] - p.nt[j], 0.5 * (p.pt[i] + p.nt[j])});
    }
  }
  return result;
}

bool has_pair(const std::vector<Pair>& pairs, double half_width) {
  for (const auto& pair : pairs)
    if (std::abs(pair.dt) <= half_width) return true;
  return false;
}

bool has_xy(const std::vector<Pair>& x, const std::vector<Pair>& y,
            double end_half_width, double xy_half_width) {
  for (const auto& px : x) {
    if (std::abs(px.dt) > end_half_width) continue;
    for (const auto& py : y) {
      if (std::abs(py.dt) <= end_half_width &&
          std::abs(px.mean - py.mean) <= xy_half_width)
        return true;
    }
  }
  return false;
}
} // namespace

void analyze_xem2_hms_paddle_edtm(
    const char* input =
        "/work/hallc/gep/brash/hallc_replay_XEM/ROOTfiles/COIN/HeeP/"
        "xem2_hms_hodoscope_audit_edtm_17193_100000.root",
    const char* csv = "/work/hallc/gep/brash/hallc_replay_XEM/"
                      "xem2_hms_paddle_edtm_scan.csv",
    const char* pdf = "/work/hallc/gep/brash/hallc_replay_XEM/"
                      "xem2_hms_paddle_edtm_timing.pdf") {
  TFile file(input, "READ");
  auto* tree = dynamic_cast<TTree*>(file.Get("T"));
  if (!tree) { std::cerr << "Missing T tree\n"; return; }

  double edtm = 0.0;
  Plane x, y;
  tree->SetBranchAddress("T.coin.hEDTM_tdcTime", &edtm);
  tree->SetBranchAddress("Ndata.H.hod.1x.posTdcCounter", &x.npos);
  tree->SetBranchAddress("H.hod.1x.posTdcCounter", x.pc);
  tree->SetBranchAddress("H.hod.1x.posTdcTimeRaw", x.pt);
  tree->SetBranchAddress("Ndata.H.hod.1x.negTdcCounter", &x.nneg);
  tree->SetBranchAddress("H.hod.1x.negTdcCounter", x.nc);
  tree->SetBranchAddress("H.hod.1x.negTdcTimeRaw", x.nt);
  tree->SetBranchAddress("Ndata.H.hod.1y.posTdcCounter", &y.npos);
  tree->SetBranchAddress("H.hod.1y.posTdcCounter", y.pc);
  tree->SetBranchAddress("H.hod.1y.posTdcTimeRaw", y.pt);
  tree->SetBranchAddress("Ndata.H.hod.1y.negTdcCounter", &y.nneg);
  tree->SetBranchAddress("H.hod.1y.negTdcCounter", y.nc);
  tree->SetBranchAddress("H.hod.1y.negTdcTimeRaw", y.nt);

  const std::array<double, 7> widths{{5., 10., 15., 20., 30., 40., 60.}};
  std::array<long long, widths.size()> nx{}, ny{}, nxy{};
  long long nedtm = 0, nx_any = 0, ny_any = 0, nxy_any = 0;
  TH1D hdx("hdx", "S1X same-paddle PMT timing;positive - negative (ns);pairs", 300, -150, 150);
  TH1D hdy("hdy", "S1Y same-paddle PMT timing;positive - negative (ns);pairs", 300, -150, 150);
  TH1D hdxy("hdxy", "S1X-S1Y mean timing (end |#Deltat| < 40 ns);X mean - Y mean (ns);pairs", 300, -150, 150);
  TH1D ht("ht", "Raw HMS S1 TDC times in EDTM events;raw time (ns);hits", 240, 0, 120);

  const auto entries = tree->GetEntries();
  for (Long64_t entry = 0; entry < entries; ++entry) {
    tree->GetEntry(entry);
    if (edtm == 0.0) continue;
    ++nedtm;
    auto xp = same_paddle_pairs(x);
    auto yp = same_paddle_pairs(y);
    if (!xp.empty()) ++nx_any;
    if (!yp.empty()) ++ny_any;
    if (!xp.empty() && !yp.empty()) ++nxy_any;
    for (const auto& p : xp) hdx.Fill(p.dt);
    for (const auto& p : yp) hdy.Fill(p.dt);
    for (int i = 0; i < x.npos && i < kMaxHits; ++i) ht.Fill(x.pt[i]);
    for (int i = 0; i < x.nneg && i < kMaxHits; ++i) ht.Fill(x.nt[i]);
    for (int i = 0; i < y.npos && i < kMaxHits; ++i) ht.Fill(y.pt[i]);
    for (int i = 0; i < y.nneg && i < kMaxHits; ++i) ht.Fill(y.nt[i]);
    for (const auto& px : xp)
      if (std::abs(px.dt) <= 40.)
        for (const auto& py : yp)
          if (std::abs(py.dt) <= 40.) hdxy.Fill(px.mean - py.mean);
    for (size_t i = 0; i < widths.size(); ++i) {
      if (has_pair(xp, widths[i])) ++nx[i];
      if (has_pair(yp, widths[i])) ++ny[i];
      if (has_xy(xp, yp, widths[i], widths[i])) ++nxy[i];
    }
  }

  std::ofstream out(csv);
  out << "half_width_ns,n_edtm,x_count,y_count,xy_count,x_occupancy,"
         "y_occupancy,xy_occupancy,x_rate_Hz,y_rate_Hz,xy_rate_Hz,"
         "tdc_window_ns\n" << std::setprecision(10);
  auto rate = [](double occupancy) {
    return occupancy >= 1.0 ? std::numeric_limits<double>::infinity()
                            : -std::log1p(-occupancy) / (kTdcWindowNs * 1e-9);
  };
  for (size_t i = 0; i < widths.size(); ++i) {
    const double ox = double(nx[i]) / nedtm;
    const double oy = double(ny[i]) / nedtm;
    const double oxy = double(nxy[i]) / nedtm;
    out << widths[i] << ',' << nedtm << ',' << nx[i] << ',' << ny[i] << ','
        << nxy[i] << ',' << ox << ',' << oy << ',' << oxy << ',' << rate(ox)
        << ',' << rate(oy) << ',' << rate(oxy) << ',' << kTdcWindowNs << '\n';
  }
  out.close();

  std::cout << "EDTM=" << nedtm << " any_same_paddle_x=" << nx_any
            << " any_same_paddle_y=" << ny_any << " any_xy=" << nxy_any
            << " tdc_window_ns=" << kTdcWindowNs << '\n';
  for (size_t i = 0; i < widths.size(); ++i)
    std::cout << "halfwidth=" << widths[i] << " x=" << nx[i] << " y=" << ny[i]
              << " xy=" << nxy[i] << '\n';

  gStyle->SetOptStat(0);
  TCanvas canvas("canvas", "XEM2 HMS S1 EDTM timing", 1500, 1000);
  canvas.Divide(2, 2);
  canvas.cd(1); hdx.Draw();
  canvas.cd(2); hdy.Draw();
  canvas.cd(3); hdxy.Draw();
  canvas.cd(4); ht.Draw();
  canvas.SaveAs(pdf);
}
