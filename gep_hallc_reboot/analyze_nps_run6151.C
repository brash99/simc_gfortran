#include <TCanvas.h>
#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TLegend.h>
#include <TStyle.h>
#include <TTree.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>

// First-look analysis for the private 10k-event replay of NPS run 6151.
// The VTP energies are recorded integer sums. Dividing by 1000 below is only
// the nominal ADC-to-MeV convention and is not a substitute for a verified
// run-dependent trigger calibration.
void analyze_nps_run6151(
    const char* input,
    const char* output_prefix = "nps_run6151_10k") {
  constexpr int kMaxObjects = 4096;

  TFile file(input, "READ");
  if (file.IsZombie()) {
    std::cerr << "Cannot open " << input << std::endl;
    return;
  }
  auto* tree = dynamic_cast<TTree*>(file.Get("T"));
  if (!tree) {
    std::cerr << "Tree T is missing from " << input << std::endl;
    return;
  }

  double edtm_time_raw = 0.0;
  double bcm4a_current = 0.0;
  double scaler_time = 0.0;
  int n_vtp = 0;
  int n_offline = 0;
  std::array<double, kMaxObjects> vtp_energy{};
  std::array<double, kMaxObjects> vtp_x{};
  std::array<double, kMaxObjects> vtp_y{};
  std::array<double, kMaxObjects> offline_energy{};
  std::array<double, kMaxObjects> offline_x{};
  std::array<double, kMaxObjects> offline_y{};

  tree->SetBranchAddress("T.hms.hEDTM_tdcTimeRaw", &edtm_time_raw);
  tree->SetBranchAddress("H.BCM4A.scalerCurrent", &bcm4a_current);
  tree->SetBranchAddress("H.1MHz.scalerTime", &scaler_time);
  tree->SetBranchAddress("Ndata.NPS.cal.vtpClusE", &n_vtp);
  tree->SetBranchAddress("NPS.cal.vtpClusE", vtp_energy.data());
  tree->SetBranchAddress("NPS.cal.vtpClusX", vtp_x.data());
  tree->SetBranchAddress("NPS.cal.vtpClusY", vtp_y.data());
  tree->SetBranchAddress("Ndata.NPS.cal.clusE", &n_offline);
  tree->SetBranchAddress("NPS.cal.clusE", offline_energy.data());
  tree->SetBranchAddress("NPS.cal.clusX", offline_x.data());
  tree->SetBranchAddress("NPS.cal.clusY", offline_y.data());

  TH1D h_vtp_physics("h_vtp_physics", "Maximum VTP cluster;nominal VTP energy (GeV);events", 120, 0, 8);
  TH1D h_vtp_edtm("h_vtp_edtm", "Maximum VTP cluster;nominal VTP energy (GeV);events", 120, 0, 8);
  TH1D h_offline_physics("h_offline_physics", "Maximum offline cluster;offline energy (GeV);events", 120, 0, 8);
  TH1D h_offline_edtm("h_offline_edtm", "Maximum offline cluster;offline energy (GeV);events", 120, 0, 8);
  TH2D h_energy_correlation("h_energy_correlation",
                            "Ordinary physics events;maximum offline energy (GeV);maximum nominal VTP energy (GeV)",
                            100, 0, 8, 100, 0, 8);
  TH1D h_current_physics("h_current_physics", "BCM4A current;current (#muA);events", 100, 0, 40);
  TH1D h_current_edtm("h_current_edtm", "BCM4A current;current (#muA);events", 100, 0, 40);

  long long n_physics = 0;
  long long n_edtm = 0;
  long long physics_with_vtp = 0;
  long long edtm_with_vtp = 0;
  long long physics_with_offline = 0;
  long long edtm_with_offline = 0;
  double min_time = std::numeric_limits<double>::infinity();
  double max_time = -std::numeric_limits<double>::infinity();
  double physics_current_sum = 0.0;
  double edtm_current_sum = 0.0;
  long long n_energy_pairs = 0;
  double sum_offline = 0.0;
  double sum_vtp_gev = 0.0;
  double sum_offline2 = 0.0;
  double sum_vtp_gev2 = 0.0;
  double sum_cross = 0.0;
  double sum_ratio = 0.0;
  double sum_ratio2 = 0.0;
  long long position_pairs = 0;
  long long position_matches_4cm = 0;
  double sum_dx = 0.0;
  double sum_dy = 0.0;
  double sum_dx2 = 0.0;
  double sum_dy2 = 0.0;

  constexpr int kNThresholds = 38;
  std::array<double, kNThresholds> thresholds{};
  std::array<long long, kNThresholds> physics_above{};
  std::array<long long, kNThresholds> edtm_above{};
  for (int i = 0; i < kNThresholds; ++i)
    thresholds[i] = 300.0 + 100.0 * i;

  const auto entries = tree->GetEntries();
  for (Long64_t entry = 0; entry < entries; ++entry) {
    tree->GetEntry(entry);
    if (n_vtp < 0 || n_vtp > kMaxObjects || n_offline < 0 || n_offline > kMaxObjects) {
      std::cerr << "Object-count overflow at entry " << entry << ": "
                << n_vtp << " VTP, " << n_offline << " offline" << std::endl;
      return;
    }

    min_time = std::min(min_time, scaler_time);
    max_time = std::max(max_time, scaler_time);
    const bool is_edtm = edtm_time_raw != 0.0;
    const double max_vtp = n_vtp > 0
                               ? *std::max_element(vtp_energy.begin(), vtp_energy.begin() + n_vtp)
                               : 0.0;
    const double max_offline = n_offline > 0
                                   ? *std::max_element(offline_energy.begin(), offline_energy.begin() + n_offline)
                                   : 0.0;
    const int max_vtp_index = n_vtp > 0
                                  ? std::distance(vtp_energy.begin(),
                                                  std::max_element(vtp_energy.begin(), vtp_energy.begin() + n_vtp))
                                  : -1;
    const int max_offline_index = n_offline > 0
                                      ? std::distance(offline_energy.begin(),
                                                      std::max_element(offline_energy.begin(), offline_energy.begin() + n_offline))
                                      : -1;

    if (is_edtm) {
      ++n_edtm;
      edtm_current_sum += bcm4a_current;
      h_current_edtm.Fill(bcm4a_current);
      if (n_vtp > 0) {
        ++edtm_with_vtp;
        h_vtp_edtm.Fill(max_vtp / 1000.0);
      }
      if (n_offline > 0) {
        ++edtm_with_offline;
        h_offline_edtm.Fill(max_offline);
      }
    } else {
      ++n_physics;
      physics_current_sum += bcm4a_current;
      h_current_physics.Fill(bcm4a_current);
      if (n_vtp > 0) {
        ++physics_with_vtp;
        h_vtp_physics.Fill(max_vtp / 1000.0);
      }
      if (n_offline > 0) {
        ++physics_with_offline;
        h_offline_physics.Fill(max_offline);
      }
      if (n_vtp > 0 && n_offline > 0) {
        h_energy_correlation.Fill(max_offline, max_vtp / 1000.0);
        // VTP coordinates are zero-based column/row indices. Offline cluster
        // positions use the 30x36 array geometry with 2.16 cm pitch.
        const double vtp_x_cm = -31.32 + 2.16 * vtp_x[max_vtp_index];
        const double vtp_y_cm = -37.80 + 2.16 * vtp_y[max_vtp_index];
        const double dx = vtp_x_cm - offline_x[max_offline_index];
        const double dy = vtp_y_cm - offline_y[max_offline_index];
        ++position_pairs;
        sum_dx += dx;
        sum_dy += dy;
        sum_dx2 += dx * dx;
        sum_dy2 += dy * dy;
        if (std::hypot(dx, dy) < 4.0)
          ++position_matches_4cm;
        // Exclude the very-low-energy region where selecting the maximum of
        // two unrelated cluster lists makes the event-level pairing unstable.
        if (max_offline >= 1.5 && max_vtp >= 1500.0) {
          const double vtp_gev = max_vtp / 1000.0;
          const double ratio = vtp_gev / max_offline;
          ++n_energy_pairs;
          sum_offline += max_offline;
          sum_vtp_gev += vtp_gev;
          sum_offline2 += max_offline * max_offline;
          sum_vtp_gev2 += vtp_gev * vtp_gev;
          sum_cross += max_offline * vtp_gev;
          sum_ratio += ratio;
          sum_ratio2 += ratio * ratio;
        }
      }
    }

    for (int i = 0; i < kNThresholds; ++i) {
      if (max_vtp >= thresholds[i]) {
        if (is_edtm)
          ++edtm_above[i];
        else
          ++physics_above[i];
      }
    }
  }

  const double elapsed = max_time - min_time;
  const std::string prefix(output_prefix);
  const double denominator = n_energy_pairs * sum_offline2 - sum_offline * sum_offline;
  const double slope = denominator != 0.0
                           ? (n_energy_pairs * sum_cross - sum_offline * sum_vtp_gev) / denominator
                           : 0.0;
  const double intercept = n_energy_pairs
                               ? (sum_vtp_gev - slope * sum_offline) / n_energy_pairs
                               : 0.0;
  const double corr_denominator =
      std::sqrt((n_energy_pairs * sum_offline2 - sum_offline * sum_offline) *
                (n_energy_pairs * sum_vtp_gev2 - sum_vtp_gev * sum_vtp_gev));
  const double correlation = corr_denominator != 0.0
                                 ? (n_energy_pairs * sum_cross - sum_offline * sum_vtp_gev) /
                                       corr_denominator
                                 : 0.0;
  const double mean_ratio = n_energy_pairs ? sum_ratio / n_energy_pairs : 0.0;
  const double ratio_variance = n_energy_pairs
                                    ? std::max(0.0, sum_ratio2 / n_energy_pairs - mean_ratio * mean_ratio)
                                    : 0.0;
  const double mean_dx = position_pairs ? sum_dx / position_pairs : 0.0;
  const double mean_dy = position_pairs ? sum_dy / position_pairs : 0.0;
  const double rms_dx = position_pairs ? std::sqrt(std::max(0.0, sum_dx2 / position_pairs - mean_dx * mean_dx)) : 0.0;
  const double rms_dy = position_pairs ? std::sqrt(std::max(0.0, sum_dy2 / position_pairs - mean_dy * mean_dy)) : 0.0;

  std::ofstream summary(prefix + "_summary.txt");
  summary << std::fixed << std::setprecision(6)
          << "input=" << input << '\n'
          << "entries=" << entries << '\n'
          << "ordinary_physics_events=" << n_physics << '\n'
          << "edtm_events=" << n_edtm << '\n'
          << "scaler_time_min_s=" << min_time << '\n'
          << "scaler_time_max_s=" << max_time << '\n'
          << "elapsed_scaler_span_s=" << elapsed << '\n'
          << "ordinary_physics_accepted_rate_hz=" << (elapsed > 0 ? n_physics / elapsed : 0) << '\n'
          << "edtm_accepted_rate_hz=" << (elapsed > 0 ? n_edtm / elapsed : 0) << '\n'
          << "ordinary_physics_mean_bcm4a_uA=" << (n_physics ? physics_current_sum / n_physics : 0) << '\n'
          << "edtm_mean_bcm4a_uA=" << (n_edtm ? edtm_current_sum / n_edtm : 0) << '\n'
          << "ordinary_physics_with_vtp=" << physics_with_vtp << '\n'
          << "edtm_with_vtp=" << edtm_with_vtp << '\n'
          << "ordinary_physics_with_offline=" << physics_with_offline << '\n'
          << "edtm_with_offline=" << edtm_with_offline << '\n'
          << "energy_pair_selection=ordinary physics; both maxima >= 1.5 GeV\n"
          << "energy_pairs=" << n_energy_pairs << '\n'
          << "vtp_GeV_vs_offline_GeV_intercept=" << intercept << '\n'
          << "vtp_GeV_vs_offline_GeV_slope=" << slope << '\n'
          << "vtp_offline_pearson_correlation=" << correlation << '\n'
          << "mean_vtp_over_offline=" << mean_ratio << '\n'
          << "rms_vtp_over_offline=" << std::sqrt(ratio_variance) << '\n'
          << "position_pairs=" << position_pairs << '\n'
          << "mean_vtp_minus_offline_x_cm=" << mean_dx << '\n'
          << "rms_vtp_minus_offline_x_cm=" << rms_dx << '\n'
          << "mean_vtp_minus_offline_y_cm=" << mean_dy << '\n'
          << "rms_vtp_minus_offline_y_cm=" << rms_dy << '\n'
          << "position_matches_within_4cm=" << position_matches_4cm << '\n';

  std::ofstream scan(prefix + "_threshold_scan.csv");
  scan << "threshold_integer,threshold_nominal_GeV,ordinary_physics_events,edtm_events,"
          "ordinary_physics_count_per_elapsed_second,edtm_count_per_elapsed_second\n";
  for (int i = 0; i < kNThresholds; ++i) {
    scan << thresholds[i] << ',' << thresholds[i] / 1000.0 << ','
         << physics_above[i] << ',' << edtm_above[i] << ','
         << (elapsed > 0 ? physics_above[i] / elapsed : 0) << ','
         << (elapsed > 0 ? edtm_above[i] / elapsed : 0) << '\n';
  }

  gStyle->SetOptStat(0);
  TCanvas canvas("canvas", "Run 6151 first look", 1300, 950);
  canvas.Divide(2, 2);
  canvas.cd(1);
  h_vtp_physics.SetLineColor(kBlue + 1);
  h_vtp_edtm.SetLineColor(kRed + 1);
  h_vtp_physics.SetLineWidth(2);
  h_vtp_edtm.SetLineWidth(2);
  h_vtp_physics.Draw("hist");
  h_vtp_edtm.Draw("hist same");
  TLegend legend_vtp(0.58, 0.72, 0.88, 0.88);
  legend_vtp.AddEntry(&h_vtp_physics, "ordinary physics", "l");
  legend_vtp.AddEntry(&h_vtp_edtm, "EDTM", "l");
  legend_vtp.Draw();

  canvas.cd(2);
  h_offline_physics.SetLineColor(kBlue + 1);
  h_offline_edtm.SetLineColor(kRed + 1);
  h_offline_physics.SetLineWidth(2);
  h_offline_edtm.SetLineWidth(2);
  h_offline_physics.Draw("hist");
  h_offline_edtm.Draw("hist same");
  TLegend legend_offline(0.58, 0.72, 0.88, 0.88);
  legend_offline.AddEntry(&h_offline_physics, "ordinary physics", "l");
  legend_offline.AddEntry(&h_offline_edtm, "EDTM", "l");
  legend_offline.Draw();

  canvas.cd(3);
  h_energy_correlation.Draw("colz");
  canvas.cd(4);
  h_current_physics.SetLineColor(kBlue + 1);
  h_current_edtm.SetLineColor(kRed + 1);
  h_current_physics.SetLineWidth(2);
  h_current_edtm.SetLineWidth(2);
  h_current_physics.DrawNormalized("hist");
  h_current_edtm.DrawNormalized("hist same");
  TLegend legend_current(0.58, 0.72, 0.88, 0.88);
  legend_current.AddEntry(&h_current_physics, "ordinary physics", "l");
  legend_current.AddEntry(&h_current_edtm, "EDTM", "l");
  legend_current.Draw();

  canvas.SaveAs((prefix + "_diagnostics.png").c_str());
  canvas.SaveAs((prefix + "_diagnostics.pdf").c_str());
}
