#include <TFile.h>
#include <TH1D.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TTree.h>
#include <TTreeReader.h>
#include <TTreeReaderArray.h>
#include <TTreeReaderValue.h>

#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

namespace {
constexpr double kInvalid = 1.e37;
// Accepted EDTM events provide the standard 1-us forced readout window.  This
// normalization is independently validated below by closure to the run-17193
// S1X and S1Y hardware scalers; the 0--1200 hodoscope reconstruction cut is
// not the duration of the EDTM sampling window.
constexpr double kEdtmWindowNs = 1000.0;
struct Hit { int paddle; double dt; double mean; };

std::vector<Hit> matched(const TTreeReaderArray<Double_t>& pos,
                         const TTreeReaderArray<Double_t>& neg) {
  std::vector<Hit> hits;
  const size_t n = std::min(pos.GetSize(), neg.GetSize());
  for (size_t i = 0; i < n; ++i) {
    if (std::abs(pos[i]) >= kInvalid || std::abs(neg[i]) >= kInvalid) continue;
    hits.push_back({int(i + 1), pos[i] - neg[i], 0.5 * (pos[i] + neg[i])});
  }
  return hits;
}

bool xy_match(const std::vector<Hit>& x, const std::vector<Hit>& y,
              double half_width) {
  for (const auto& hx : x)
    for (const auto& hy : y)
      if (std::abs(hx.mean - hy.mean) <= half_width) return true;
  return false;
}
}

void analyze_xem2_hms_paddle_good_edtm(
    const char* input =
        "/work/hallc/gep/brash/hallc_replay_XEM/ROOTfiles/COIN/HeeP/"
        "xem2_hms_hodoscope_audit_edtm_17193_100000.root",
    const char* csv = "/work/hallc/gep/brash/hallc_replay_XEM/"
                      "xem2_hms_paddle_good_edtm.csv",
    const char* pdf = "/work/hallc/gep/brash/hallc_replay_XEM/"
                      "xem2_hms_paddle_good_edtm.pdf") {
  TFile file(input, "READ");
  auto* tree = dynamic_cast<TTree*>(file.Get("T"));
  TTreeReader reader(tree);
  TTreeReaderValue<Double_t> edtm(reader, "T.coin.hEDTM_tdcTime");
  TTreeReaderArray<Double_t> xp(reader, "H.hod.1x.GoodPosTdcTimeUnCorr");
  TTreeReaderArray<Double_t> xn(reader, "H.hod.1x.GoodNegTdcTimeUnCorr");
  TTreeReaderArray<Double_t> yp(reader, "H.hod.1y.GoodPosTdcTimeUnCorr");
  TTreeReaderArray<Double_t> yn(reader, "H.hod.1y.GoodNegTdcTimeUnCorr");

  const std::array<double, 7> xy_widths{{2., 5., 10., 15., 20., 30., 40.}};
  std::array<long long, xy_widths.size()> nxy{};
  long long n_edtm = 0, nx = 0, ny = 0, nxy_any = 0;
  TH1D hxdt("hxdt", "S1X same-paddle good-hit timing;positive - negative (ns);paddles", 200, -50, 50);
  TH1D hydt("hydt", "S1Y same-paddle good-hit timing;positive - negative (ns);paddles", 200, -50, 50);
  TH1D hxydt("hxydt", "S1X-S1Y good-hit mean timing;X mean - Y mean (ns);pairs", 240, -60, 60);
  TH1D hpaddle("hpaddle", "Matched S1 paddle occupancy in EDTM events;paddle (X:1-16, Y:21-30);hits", 30, 0.5, 30.5);

  while (reader.Next()) {
    if (*edtm == 0.0) continue;
    ++n_edtm;
    auto xhits = matched(xp, xn);
    auto yhits = matched(yp, yn);
    if (!xhits.empty()) ++nx;
    if (!yhits.empty()) ++ny;
    if (!xhits.empty() && !yhits.empty()) ++nxy_any;
    for (const auto& h : xhits) { hxdt.Fill(h.dt); hpaddle.Fill(h.paddle); }
    for (const auto& h : yhits) { hydt.Fill(h.dt); hpaddle.Fill(20 + h.paddle); }
    for (const auto& hx : xhits)
      for (const auto& hy : yhits) hxydt.Fill(hx.mean - hy.mean);
    for (size_t i = 0; i < xy_widths.size(); ++i)
      if (xy_match(xhits, yhits, xy_widths[i])) ++nxy[i];
  }

  auto rate = [](double occupancy) {
    return occupancy >= 1.0 ? std::numeric_limits<double>::infinity()
                            : -std::log1p(-occupancy) / (kEdtmWindowNs * 1e-9);
  };
  const double ox = double(nx) / n_edtm;
  const double oy = double(ny) / n_edtm;
  const double oxy_any = double(nxy_any) / n_edtm;
  std::ofstream out(csv);
  out << "xy_half_width_ns,n_edtm,x_count,y_count,xy_any_count,xy_timed_count,"
         "x_occupancy,y_occupancy,xy_any_occupancy,xy_timed_occupancy,"
         "x_rate_Hz,y_rate_Hz,xy_any_rate_Hz,xy_timed_rate_Hz,edtm_window_ns\n"
      << std::setprecision(10);
  for (size_t i = 0; i < xy_widths.size(); ++i) {
    const double oxy = double(nxy[i]) / n_edtm;
    out << xy_widths[i] << ',' << n_edtm << ',' << nx << ',' << ny << ','
        << nxy_any << ',' << nxy[i] << ',' << ox << ',' << oy << ','
        << oxy_any << ',' << oxy << ',' << rate(ox) << ',' << rate(oy) << ','
        << rate(oxy_any) << ',' << rate(oxy) << ',' << kEdtmWindowNs << '\n';
  }
  out.close();

  std::cout << std::setprecision(10) << "EDTM=" << n_edtm << " X=" << nx
            << " Y=" << ny << " XY_any=" << nxy_any
            << " EDTM_window_ns=" << kEdtmWindowNs << '\n';
  for (size_t i = 0; i < xy_widths.size(); ++i)
    std::cout << "XY_halfwidth=" << xy_widths[i] << " count=" << nxy[i]
              << " occupancy=" << double(nxy[i]) / n_edtm
              << " rate_Hz=" << rate(double(nxy[i]) / n_edtm) << '\n';

  gStyle->SetOptStat(0);
  TCanvas canvas("canvas", "XEM2 HMS S1 good-hit EDTM audit", 1500, 1000);
  canvas.Divide(2, 2);
  canvas.cd(1); hxdt.Draw();
  canvas.cd(2); hydt.Draw();
  canvas.cd(3); hxydt.Draw();
  canvas.cd(4); hpaddle.Draw();
  canvas.SaveAs(pdf);
  TString png(pdf);
  if (png.EndsWith(".pdf")) png.ReplaceAll(".pdf", ".png");
  else png += ".png";
  canvas.SaveAs(png);
}
