#include <TCanvas.h>
#include <TFile.h>
#include <TGraph.h>
#include <TH1.h>
#include <TLegend.h>
#include <TStyle.h>
#include <TTree.h>

#include <cmath>
#include <fstream>
#include <iostream>
#include <string>

namespace {
struct Metrics {
  double entry_fraction = 0.;
  double below90_fraction = 0.;
  double core_mean_fraction = 0.;
  double core_rms_percent = 0.;
};

Metrics Analyze(const std::string& file_name, double incident_mev) {
  TFile file(file_name.c_str());
  auto* tree = static_cast<TTree*>(file.Get("t_dvcs"));
  if (!tree || tree->GetEntries() != 200) {
    std::cerr << "Expected 200 entries in " << file_name << std::endl;
    return {};
  }

  double edep[1080];
  int entered = 0;
  tree->SetBranchAddress("edep", edep);
  tree->SetBranchAddress("nps_entry", &entered);

  long entry_count = 0;
  long below90_count = 0;
  long core_count = 0;
  double core_sum = 0.;
  double core_sum2 = 0.;
  for (Long64_t event = 0; event < tree->GetEntries(); ++event) {
    tree->GetEntry(event);
    double total = 0.;
    for (int block = 0; block < 1080; ++block) total += edep[block];
    entry_count += entered;
    if (total < 0.9 * incident_mev) ++below90_count;
    else {
      ++core_count;
      core_sum += total;
      core_sum2 += total * total;
    }
  }

  Metrics result;
  result.entry_fraction = static_cast<double>(entry_count) / tree->GetEntries();
  result.below90_fraction = static_cast<double>(below90_count) / tree->GetEntries();
  const double mean = core_sum / core_count;
  const double rms = std::sqrt(core_sum2 / core_count - mean * mean);
  result.core_mean_fraction = mean / incident_mev;
  result.core_rms_percent = 100. * rms / mean;
  return result;
}

void Style(TGraph& graph, int color, int marker) {
  graph.SetLineColor(color);
  graph.SetMarkerColor(color);
  graph.SetMarkerStyle(marker);
  graph.SetMarkerSize(1.3);
  graph.SetLineWidth(2);
}

void LabelAxis(TAxis* axis) {
  const char* labels[] = {"Center", "Mid-face", "Near edge", "Edge", "Corner"};
  for (int point = 0; point < 5; ++point) axis->SetBinLabel(point + 1, labels[point]);
}
}

void analyze_nps_geant4_face_scan() {
  const std::string base = "/work/hallc/gep/brash/gep4_nps_geant4/validation";
  const char* positions[] = {"center", "midface", "near_edge", "edge", "corner"};
  const char* labels[] = {"center", "mid-face", "near-edge", "edge-center", "corner"};
  const int tags[] = {4598, 7072};
  const double energies[] = {4598., 7072.};
  Metrics metrics[2][5];

  std::ofstream csv(base + "/nps_geant4_face_scan_200.csv");
  csv << "energy_MeV,position,entry_fraction,below90_fraction,core_mean_fraction,core_rms_percent\n";
  for (int energy = 0; energy < 2; ++energy) {
    for (int point = 0; point < 5; ++point) {
      const std::string file = base + "/face_scan_" + std::to_string(tags[energy])
                               + "_" + positions[point] + "_200.root";
      metrics[energy][point] = Analyze(file, energies[energy]);
      const auto& m = metrics[energy][point];
      csv << tags[energy] << ',' << labels[point] << ',' << m.entry_fraction << ','
          << m.below90_fraction << ',' << m.core_mean_fraction << ','
          << m.core_rms_percent << '\n';
    }
  }
  csv.close();

  gStyle->SetOptStat(0);
  TCanvas canvas("canvas", "NPS GEANT4 face scan", 1300, 950);
  canvas.Divide(2, 2);
  const char* titles[] = {
    "Primary enters active crystal;Face position;Fraction",
    "Low-energy tail (#SigmaE_{dep} < 0.9 E);Face position;Fraction",
    "Contained-core mean;Face position;Mean #SigmaE_{dep}/E",
    "Contained-core width;Face position;RMS / mean (%)"
  };

  TGraph graphs[4][2];
  TH1D* frames[4];
  const double minima[] = {0.55, 0., 0.95, 0.8};
  const double maxima[] = {1.02, 0.55, 0.985, 2.2};
  for (int panel = 0; panel < 4; ++panel) {
    for (int energy = 0; energy < 2; ++energy) {
      for (int point = 0; point < 5; ++point) {
        const auto& m = metrics[energy][point];
        const double value = panel == 0 ? m.entry_fraction
                             : panel == 1 ? m.below90_fraction
                             : panel == 2 ? m.core_mean_fraction
                                          : m.core_rms_percent;
        graphs[panel][energy].SetPoint(point, point + 1, value);
      }
      Style(graphs[panel][energy], energy == 0 ? kBlue + 1 : kRed + 1,
            energy == 0 ? 20 : 21);
    }
    canvas.cd(panel + 1);
    gPad->SetBottomMargin(0.16);
    frames[panel] = new TH1D(Form("frame_%d", panel), titles[panel], 5, 0.5, 5.5);
    frames[panel]->SetMinimum(minima[panel]);
    frames[panel]->SetMaximum(maxima[panel]);
    LabelAxis(frames[panel]->GetXaxis());
    frames[panel]->GetXaxis()->SetLabelSize(0.045);
    frames[panel]->Draw();
    graphs[panel][0].Draw("LP same");
    graphs[panel][1].Draw("LP same");
  }

  canvas.cd(1);
  TLegend legend(0.14, 0.18, 0.48, 0.34);
  legend.SetBorderSize(0);
  legend.AddEntry(&graphs[0][0], "4.598 GeV", "lp");
  legend.AddEntry(&graphs[0][1], "7.072 GeV", "lp");
  legend.Draw();

  canvas.SaveAs((base + "/nps_geant4_face_scan_200.png").c_str());
  canvas.SaveAs((base + "/nps_geant4_face_scan_200.pdf").c_str());
}
