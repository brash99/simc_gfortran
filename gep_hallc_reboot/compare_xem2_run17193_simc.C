#include <TCanvas.h>
#include <TFile.h>
#include <TH1D.h>
#include <TLegend.h>
#include <TStyle.h>
#include <TTree.h>

#include <fstream>
#include <iomanip>
#include <iostream>

void compare_xem2_run17193_simc() {
  const char *data_name =
      "/work/hallc/gep/brash/hallc_replay_XEM/ROOTfiles/COIN/HeeP/"
      "coin_replay_production_17193_-1.root";
  const char *simc_name =
      "/work/hallc/gep/brash/simc_xem2_run17193/worksim/"
      "xem2_run17193_elastic.root";

  // SIMC hist file: normfac=5.88418e7 for 1e6 trials and 1 mC.
  // Run-17193 beam-cut BCM4A charge is 150.465145 mC.
  constexpr double normfac = 5.88418e7;
  constexpr double ngen = 1.0e6;
  constexpr double charge_mC = 150.465145;
  constexpr double event_scale = normfac * charge_mC / ngen;

  TFile data_file(data_name, "READ");
  TFile simc_file(simc_name, "READ");
  auto *data = dynamic_cast<TTree *>(data_file.Get("T"));
  auto *simc = dynamic_cast<TTree *>(simc_file.Get("h10"));
  if (!data || !simc) {
    std::cerr << "Missing input tree" << std::endl;
    return;
  }

  const char *data_cut =
      "fEvtHdr.fEvtType>=4&&fEvtHdr.fEvtType<=7"
      "&&H.gtr.index>=0&&P.gtr.index>=0"
      "&&H.gtr.dp>-8&&H.gtr.dp<8&&P.gtr.dp>-10&&P.gtr.dp<22"
      "&&P.cal.etottracknorm>0.6&&P.cal.etottracknorm<1.6"
      "&&P.hgcer.npeSum>0.5";
  const char *simc_cut = "hsdelta>-8&&hsdelta<8&&ssdelta>-10&&ssdelta<22";

  TH1D hdata("hdata", "", 40, -8., 8.);
  TH1D hsimc("hsimc", "", 40, -8., 8.);
  hdata.Sumw2();
  hsimc.Sumw2();
  data->Draw("H.gtr.dp>>hdata", data_cut, "goff");
  simc->Draw("hsdelta>>hsimc",
             Form("Weight*%.12g*(%s)", event_scale, simc_cut), "goff");

  gStyle->SetOptStat(0);
  TCanvas canvas("canvas", "canvas", 900, 650);
  hdata.SetTitle("XEM2 run 17193: HMS proton momentum;HMS #delta (%);Events / 0.4%");
  hdata.SetLineColor(kBlack);
  hdata.SetMarkerColor(kBlack);
  hdata.SetMarkerStyle(20);
  hdata.SetMarkerSize(0.75);
  hsimc.SetLineColor(kRed + 1);
  hsimc.SetLineWidth(3);
  const double ymax = 1.18 * std::max(hdata.GetMaximum(), hsimc.GetMaximum());
  hdata.SetMaximum(ymax);
  hdata.Draw("E1");
  hsimc.Draw("HIST SAME");

  TLegend legend(0.57, 0.72, 0.88, 0.88);
  legend.SetBorderSize(0);
  legend.AddEntry(&hdata, "Data: two-arm tracks + e PID", "lep");
  legend.AddEntry(&hsimc, "Radiative elastic SIMC (absolute)", "l");
  legend.Draw();
  canvas.SaveAs("xem2_run17193_hms_dp_simc.pdf");
  canvas.SaveAs("xem2_run17193_hms_dp_simc.png");

  std::ofstream summary("xem2_run17193_hms_dp_simc_summary.csv");
  summary << "quantity,value\n" << std::setprecision(12)
          << "data_candidates," << hdata.Integral() << "\n"
          << "simc_predicted," << hsimc.Integral() << "\n"
          << "data_over_simc," << hdata.Integral() / hsimc.Integral() << "\n"
          << "beam_cut_charge_mC," << charge_mC << "\n"
          << "simc_normfac," << normfac << "\n"
          << "simc_trials," << ngen << "\n";
}
