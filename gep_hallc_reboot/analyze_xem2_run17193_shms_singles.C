#include <TCanvas.h>
#include <TFile.h>
#include <TGraph.h>
#include <TH1D.h>
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

void analyze_xem2_run17193_shms_singles(
    const char* input =
        "/work/hallc/gep/brash/hallc_replay_XEM/ROOTfiles/COIN/HeeP/"
        "coin_replay_production_17193_-1.root",
    const char* output_prefix = "xem2_run17193_shms_singles") {
  TFile file(input, "READ");
  auto* events = dynamic_cast<TTree*>(file.Get("T"));
  auto* scalers = dynamic_cast<TTree*>(file.Get("TSP"));
  if (file.IsZombie() || !events || !scalers) {
    std::cerr << "missing T or TSP in " << input << std::endl;
    return;
  }

  double current = 0.0;
  double ptrig1_rate = 0.0;
  double pel_rate = 0.0;
  scalers->SetBranchAddress("P.BCM4A.scalerCurrent", &current);
  scalers->SetBranchAddress("P.pTRIG1.scalerRate", &ptrig1_rate);
  scalers->SetBranchAddress("P.pEL_REAL.scalerRate", &pel_rate);
  double current_sum = 0.0;
  double ptrig1_sum = 0.0;
  double pel_sum = 0.0;
  long long scaler_count = 0;
  for (Long64_t entry = 0; entry < scalers->GetEntries(); ++entry) {
    scalers->GetEntry(entry);
    if (current < 10.0) continue;
    ++scaler_count;
    current_sum += current;
    ptrig1_sum += ptrig1_rate;
    pel_sum += pel_rate;
  }
  const double mean_current = current_sum / scaler_count;
  const double mean_ptrig1_rate = ptrig1_sum / scaler_count;
  const double mean_pel_rate = pel_sum / scaler_count;

  UInt_t event_type = 0;
  double calorimeter_energy = 0.0;
  double ptrig2_multiplicity = 0.0;
  events->SetBranchAddress("fEvtHdr.fEvtType", &event_type);
  events->SetBranchAddress("P.cal.etot", &calorimeter_energy);
  events->SetBranchAddress("T.coin.pTRIG2_ROC2_tdcMultiplicity", &ptrig2_multiplicity);

  constexpr int kThresholds = 121;
  std::array<long long, kThresholds> above{};
  std::array<long long, kThresholds> tagged_above{};
  TH1D spectrum("spectrum", "", 120, 0.0, 6.0);
  TH1D tagged("tagged", "", 120, 0.0, 6.0);
  long long singles = 0;
  long long electron_tagged = 0;
  for (Long64_t entry = 0; entry < events->GetEntries(); ++entry) {
    events->GetEntry(entry);
    if (event_type != 1) continue;
    ++singles;
    const bool has_electron_primitive = ptrig2_multiplicity > 0.0;
    if (has_electron_primitive) ++electron_tagged;
    spectrum.Fill(calorimeter_energy);
    if (has_electron_primitive) tagged.Fill(calorimeter_energy);
    for (int index = 0; index < kThresholds; ++index) {
      const double threshold = 0.05 * index;
      if (calorimeter_energy < threshold) continue;
      ++above[index];
      if (has_electron_primitive) ++tagged_above[index];
    }
  }

  std::ofstream csv(std::string(output_prefix) + ".csv");
  csv << "offline_total_energy_threshold_GeV,pTRIG1_sample_rate_Hz,"
         "pTRIG2_tagged_rate_Hz,pTRIG1_events_above,pTRIG2_tagged_events_above\n";
  csv << std::setprecision(10);
  for (int index = 0; index < kThresholds; ++index) {
    const double threshold = 0.05 * index;
    csv << threshold << ',' << mean_ptrig1_rate * above[index] / singles << ','
        << mean_ptrig1_rate * tagged_above[index] / singles << ','
        << above[index] << ',' << tagged_above[index] << '\n';
  }

  std::ofstream summary(std::string(output_prefix) + "_summary.csv");
  summary << "quantity,value\n" << std::setprecision(10)
          << "mean_current_uA," << mean_current << '\n'
          << "pTRIG1_scaler_rate_Hz," << mean_ptrig1_rate << '\n'
          << "pEL_REAL_scaler_rate_Hz," << mean_pel_rate << '\n'
          << "event_type1_singles," << singles << '\n'
          << "pTRIG2_tagged_singles," << electron_tagged << '\n'
          << "pTRIG2_tag_fraction," << double(electron_tagged) / singles << '\n'
          << "tag_fraction_times_pTRIG1_Hz,"
          << mean_ptrig1_rate * electron_tagged / singles << '\n';

  gStyle->SetOptStat(0);
  TCanvas canvas("canvas", "canvas", 900, 650);
  TH1D frame("frame", "XEM2 run 17193 SHMS singles;"
             "Offline SHMS calorimeter-energy threshold (GeV);"
             "Rate above threshold (Hz)", 100, 0.0, 6.0);
  frame.SetMinimum(1.0);
  frame.SetMaximum(1.3 * mean_ptrig1_rate);
  frame.Draw();
  TGraph all_graph;
  TGraph tagged_graph;
  for (int index = 0; index < kThresholds; ++index) {
    all_graph.SetPoint(index, 0.05 * index,
                       mean_ptrig1_rate * above[index] / singles);
    tagged_graph.SetPoint(index, 0.05 * index,
                          mean_ptrig1_rate * tagged_above[index] / singles);
  }
  all_graph.SetLineWidth(3);
  all_graph.SetLineColor(kBlue + 1);
  tagged_graph.SetLineWidth(3);
  tagged_graph.SetLineColor(kOrange + 7);
  all_graph.Draw("L SAME");
  tagged_graph.Draw("L SAME");
  TLine electron_scaler(0.0, mean_pel_rate, 6.0, mean_pel_rate);
  electron_scaler.SetLineColor(kRed + 1);
  electron_scaler.SetLineStyle(2);
  electron_scaler.SetLineWidth(2);
  electron_scaler.Draw();
  TLegend legend(0.49, 0.70, 0.88, 0.88);
  legend.SetBorderSize(0);
  legend.AddEntry(&all_graph, "pTRIG1 sample", "l");
  legend.AddEntry(&tagged_graph, "pTRIG2-tagged subset", "l");
  legend.AddEntry(&electron_scaler, "pEL_REAL scaler", "l");
  legend.Draw();
  canvas.SetLogy();
  canvas.SaveAs((std::string(output_prefix) + ".png").c_str());
  canvas.SaveAs((std::string(output_prefix) + ".pdf").c_str());

  std::cout << "singles=" << singles << " tagged=" << electron_tagged
            << " pTRIG1_Hz=" << mean_ptrig1_rate
            << " pEL_REAL_Hz=" << mean_pel_rate << std::endl;
}
