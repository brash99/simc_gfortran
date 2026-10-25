#include <TBox.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TLatex.h>
#include <TLegend.h>
#include <TLine.h>
#include <TStyle.h>
#include <TTree.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>

void plot_xem2_run17193_nps_shms_acceptance() {
  constexpr double shms_norm = 1.77187e8 / 1.0e6;
  constexpr double nps_norm = 8.15694e9 / 1.0e7;
  constexpr double theta_hms = 19.52 * 3.14159265358979323846 / 180.;

  TFile fshms("/work/hallc/gep/brash/simc_xem2_run17193/worksim/"
              "xem2_run17193_elastic_30cm.root", "READ");
  TFile fnps("/work/hallc/gep/brash/simc_xem2_run17193/worksim/"
             "xem2_run17193_nps_30cm_3m_10M.root", "READ");
  auto *shms = dynamic_cast<TTree *>(fshms.Get("h10"));
  auto *nps = dynamic_cast<TTree *>(fnps.Get("h10"));
  if (!shms || !nps) {
    std::cerr << "Missing SIMC tree" << std::endl;
    return;
  }

  const char *shms_cut =
      "hsdelta>-15&&hsdelta<15&&ssdelta>-10&&ssdelta<22";
  const char *nps_cut = "hsdelta>-15&&hsdelta<15";

  TH1D h_shms_dp("h_shms_dp", "", 76, -15.2, 15.2);
  TH1D h_nps_dp("h_nps_dp", "", 76, -15.2, 15.2);
  TH2D h_face("h_face", "", 40, -40., 40., 40, -40., 40.);
  TH1D h_shms_z("h_shms_z", "", 34, -17., 17.);
  TH1D h_nps_z("h_nps_z", "", 34, -17., 17.);
  for (auto *h : {&h_shms_dp, &h_nps_dp, &h_shms_z, &h_nps_z}) h->Sumw2();
  h_face.Sumw2();

  shms->Draw("hsdelta>>h_shms_dp",
             Form("Weight*%.12g*(%s)", shms_norm, shms_cut), "goff");
  nps->Draw("hsdelta>>h_nps_dp",
            Form("Weight*%.12g*(%s)", nps_norm, nps_cut), "goff");
  // SIMC transport x is vertical/dispersive; transport y is
  // horizontal/non-dispersive.  Plot them in intuitive screen order.
  nps->Draw("ssxfp:ssyfp>>h_face",
            Form("Weight*%.12g*(%s)", nps_norm, nps_cut), "goff");

  const TString zexpr = Form("(fry-hsytar)/%.12g", std::sin(theta_hms));
  shms->Draw(zexpr + ">>h_shms_z",
             Form("Weight*%.12g*(%s)", shms_norm, shms_cut), "goff");
  nps->Draw(zexpr + ">>h_nps_z",
            Form("Weight*%.12g*(%s)", nps_norm, nps_cut), "goff");

  gStyle->SetOptStat(0);
  gStyle->SetPalette(kViridis);
  TCanvas c("c", "NPS versus SHMS acceptance", 1950, 650);
  c.Divide(3, 1, 0.012, 0.);

  c.cd(1);
  gPad->SetLeftMargin(.14); gPad->SetBottomMargin(.14); gPad->SetTopMargin(.10);
  h_nps_dp.SetTitle(";HMS #delta (%);Events / mC / 0.4%");
  h_nps_dp.SetLineColor(kBlue + 1); h_nps_dp.SetLineWidth(3);
  h_shms_dp.SetLineColor(kRed + 1); h_shms_dp.SetLineWidth(3);
  h_nps_dp.SetMaximum(1.18 * std::max(h_nps_dp.GetMaximum(), h_shms_dp.GetMaximum()));
  h_nps_dp.Draw("HIST"); h_shms_dp.Draw("HIST SAME");
  TLegend l1(.53, .72, .91, .88); l1.SetBorderSize(0); l1.SetFillStyle(0);
  l1.AddEntry(&h_nps_dp, "NPS at 3 m", "l");
  l1.AddEntry(&h_shms_dp, "SHMS", "l"); l1.Draw();
  TLatex tx; tx.SetNDC(); tx.SetTextSize(.035);
  tx.DrawLatex(.17, .84, "30 cm LH_{2}; full HMS range");
  tx.SetTextAlign(22); tx.SetTextSize(.045);
  tx.DrawLatex(.52, .95, "Absolute elastic acceptance");
  tx.SetTextAlign(11); tx.SetTextSize(.035);

  c.cd(2);
  gPad->SetLeftMargin(.14); gPad->SetRightMargin(.16);
  gPad->SetBottomMargin(.14); gPad->SetTopMargin(.10); gPad->SetLogz();
  h_face.SetTitle(";horizontal (transport y) (cm);vertical (transport x) (cm)");
  h_face.SetMinimum(1.e-4); h_face.Draw("COLZ");
  TBox physical(-30.75, -36.9, 30.75, 36.9);
  physical.SetFillStyle(0); physical.SetLineColor(kBlack); physical.SetLineWidth(3);
  physical.Draw("SAME");
  TBox one(-28.70, -34.85, 28.70, 34.85);
  one.SetFillStyle(0); one.SetLineColor(kRed + 1); one.SetLineStyle(2); one.SetLineWidth(3);
  one.Draw("SAME");
  TBox two(-26.65, -32.80, 26.65, 32.80);
  two.SetFillStyle(0); two.SetLineColor(kMagenta + 2); two.SetLineStyle(3); two.SetLineWidth(3);
  two.Draw("SAME");
  TLegend l2(.17, .69, .59, .87); l2.SetBorderSize(0);
  l2.SetFillColorAlpha(kWhite, .88);
  l2.AddEntry(&physical, "physical boundary", "l");
  l2.AddEntry(&one, "one-crystal margin", "l");
  l2.AddEntry(&two, "two-crystal margin", "l"); l2.Draw();
  tx.SetTextAlign(22); tx.SetTextSize(.045);
  tx.DrawLatex(.47, .95, "NPS face at 3 m (accepted events)");
  tx.SetTextAlign(11); tx.SetTextSize(.035);

  c.cd(3);
  gPad->SetLeftMargin(.14); gPad->SetBottomMargin(.14); gPad->SetTopMargin(.10);
  h_nps_z.SetTitle(";reconstructed vertex z (cm);Events / mC / cm");
  h_nps_z.SetLineColor(kBlue + 1); h_nps_z.SetLineWidth(3);
  h_shms_z.SetLineColor(kRed + 1); h_shms_z.SetLineWidth(3);
  const double zmax = 1.18 * std::max(h_nps_z.GetMaximum(), h_shms_z.GetMaximum());
  h_nps_z.SetMaximum(zmax);
  h_nps_z.Draw("HIST"); h_shms_z.Draw("HIST SAME");
  TLine target_lo(-15., 0., -15., zmax);
  TLine target_hi(15., 0., 15., zmax);
  for (auto *line : {&target_lo, &target_hi}) {
    line->SetLineColor(kGray + 2); line->SetLineStyle(2); line->SetLineWidth(2);
    line->Draw("SAME");
  }
  TLegend l3(.54, .67, .91, .88); l3.SetBorderSize(0); l3.SetFillStyle(0);
  l3.AddEntry(&h_nps_z, "NPS at 3 m", "l");
  l3.AddEntry(&h_shms_z, "SHMS", "l");
  l3.AddEntry(&target_lo, "30 cm target edges", "l"); l3.Draw();
  tx.SetTextAlign(22); tx.SetTextSize(.045);
  tx.DrawLatex(.52, .95, "Extended-target acceptance");

  c.SaveAs("xem2_run17193_nps_shms_acceptance.pdf");
  c.SaveAs("xem2_run17193_nps_shms_acceptance.png");

  std::ofstream out("xem2_run17193_nps_shms_acceptance_summary.csv");
  out << "quantity,value\n" << std::setprecision(12)
      << "shms_yield_events_per_mC," << h_shms_dp.Integral() << "\n"
      << "nps_yield_events_per_mC," << h_nps_dp.Integral() << "\n"
      << "nps_over_shms," << h_nps_dp.Integral()/h_shms_dp.Integral() << "\n";
}
