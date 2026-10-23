#include <TCanvas.h>
#include <TFile.h>
#include <TF1.h>
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
#include <limits>
#include <string>

namespace {
constexpr int kMax = 16384;
constexpr int kBlocks = 1080;
constexpr int kColumns = 30;
constexpr double kPitchCm = 2.16;
constexpr double kMp = 0.9382720813;
constexpr std::array<double, 9> kCurrentEdges{{0., 15., 22.5, 26.5, 30., 35., 40., 50., 100.}};
constexpr std::array<double, 4> kAmplitudeCuts{{5., 10., 20., 50.}};

double quantile(TH1D& h, double probability) {
  double result = 0.;
  h.GetQuantiles(1, &result, &probability);
  return result;
}

double block_separation(int first, int second) {
  const int first_row = first / kColumns, first_column = first % kColumns;
  const int second_row = second / kColumns, second_column = second % kColumns;
  return kPitchCm * std::hypot(first_row - second_row,
                              first_column - second_column);
}

int current_bin(double current) {
  for (int i = 0; i + 1 < int(kCurrentEdges.size()); ++i)
    if (current >= kCurrentEdges[i] && current < kCurrentEdges[i + 1]) return i;
  return -1;
}
}

// Diagnose the decoded NPS block-pulse arrays without using secondary clusters.
// The primary elastic cluster is used only to select the sample and locate the
// shower. Pulse time and position are measured relative to the event's largest
// pulse, which is expected to be the elastic shower's seed block. This removes
// run-dependent absolute-time offsets. Block geometry follows nps_geom.param:
// 36 rows, 30 columns, 2.16 cm pitch, with the standard row-major ordering.
void probe_nps_block_pulses(const char* input, const char* output_prefix,
                            double beam_energy, double nominal_elastic_energy,
                            Long64_t max_entries = -1) {
  TFile file(input, "READ");
  auto* tree = dynamic_cast<TTree*>(file.Get("T"));
  if (file.IsZombie() || !tree) {
    std::cerr << "Cannot read " << input << std::endl;
    return;
  }

  double edtm = 0., ntrack = 0., current = 0.;
  int nclus = 0, ncounter = 0, namp = 0, nint = 0, ntime = 0;
  std::array<double, kMax> p{}, dp{}, clus_e{};
  std::array<double, kMax> counter{}, amp{}, integral{}, pulse_time{};

  tree->SetBranchAddress("T.hms.hEDTM_tdcTimeRaw", &edtm);
  tree->SetBranchAddress("H.dc.ntrack", &ntrack);
  tree->SetBranchAddress("H.gtr.p", p.data());
  tree->SetBranchAddress("H.gtr.dp", dp.data());
  tree->SetBranchAddress("H.BCM4A.scalerCurrent", &current);
  tree->SetBranchAddress("Ndata.NPS.cal.clusE", &nclus);
  tree->SetBranchAddress("NPS.cal.clusE", clus_e.data());
  tree->SetBranchAddress("Ndata.NPS.cal.fly.adcCounter", &ncounter);
  tree->SetBranchAddress("NPS.cal.fly.adcCounter", counter.data());
  tree->SetBranchAddress("Ndata.NPS.cal.fly.adcSampPulseAmp", &namp);
  tree->SetBranchAddress("NPS.cal.fly.adcSampPulseAmp", amp.data());
  tree->SetBranchAddress("Ndata.NPS.cal.fly.adcSampPulseInt", &nint);
  tree->SetBranchAddress("NPS.cal.fly.adcSampPulseInt", integral.data());
  tree->SetBranchAddress("Ndata.NPS.cal.fly.adcSampPulseTime", &ntime);
  tree->SetBranchAddress("NPS.cal.fly.adcSampPulseTime", pulse_time.data());

  const Long64_t entries = max_entries < 0 ? tree->GetEntries()
                                           : std::min(max_entries, tree->GetEntries());
  TH1D residual("residual", "", 240, -.45 * nominal_elastic_energy,
                .2 * nominal_elastic_energy);
  for (Long64_t entry = 0; entry < entries; ++entry) {
    tree->GetEntry(entry);
    if (edtm != 0. || ntrack != 1. || std::abs(dp[0]) >= 8. || p[0] <= 0. ||
        nclus <= 0 || nclus > kMax)
      continue;
    const int primary = std::distance(
        clus_e.begin(), std::max_element(clus_e.begin(), clus_e.begin() + nclus));
    const double expected =
        beam_energy + kMp - std::sqrt(p[0] * p[0] + kMp * kMp);
    residual.Fill(clus_e[primary] - expected);
  }
  const double peak = residual.GetBinCenter(residual.GetMaximumBin());
  TF1 fit1("fit1", "gaus", peak - .08 * nominal_elastic_energy,
           peak + .08 * nominal_elastic_energy);
  residual.Fit(&fit1, "RQ0");
  TF1 fit2("fit2", "gaus",
           fit1.GetParameter(1) - 2. * std::abs(fit1.GetParameter(2)),
           fit1.GetParameter(1) + 2. * std::abs(fit1.GetParameter(2)));
  residual.Fit(&fit2, "RQ0");
  const double mean = fit2.GetParameter(1);
  const double sigma = std::abs(fit2.GetParameter(2));

  TH1D h_n("h_n", "Decoded pulses per elastic-core event;pulses;events", 160, 0, 1600);
  TH1D h_amp("h_amp", "All decoded pulses;pulse amplitude (mV);pulses", 250, 0, 2500);
  TH1D h_integral("h_integral", "All decoded pulses;pulse integral;pulses", 250, 0, 50000);
  TH1D h_time("h_time", "All decoded pulses;pulse time;entries", 240, 0, 240);
  TH1D h_dt("h_dt", "Pulse time relative to shower reference;#Deltat;entries", 240, -120, 120);
  TH1D h_dr("h_dr", "Pulse separation from elastic shower;#Deltar (cm);entries", 160, 0, 100);
  TH1D h_frac("h_frac", "Pulse amplitude / shower-reference amplitude;A/A_{ref};entries", 240, 0, 1.2);
  TH2D h_dt_amp("h_dt_amp", "Raw pulse topology;#Deltat (ns);amplitude (mV)", 200, -100, 100, 200, 0, 2000);
  TH2D h_dt_dr("h_dt_dr", "Raw pulse topology;#Deltar (cm);#Deltat", 120, 0, 90, 200, -100, 100);
  TH2D h_amp_int("h_amp_int", "Pulse integral versus amplitude;amplitude (mV);integral", 200, 0, 2000, 200, 0, 40000);

  long long elastic_core = 0, usable_reference = 0, mismatched_lengths = 0;
  long long invalid_counter = 0, all_pulses = 0, comparison_pulses = 0;
  double current_sum = 0.;
  int max_pulses_seen = 0;
  constexpr int kCurrentBins = kCurrentEdges.size() - 1;
  std::array<long long, kCurrentBins> core_by_current{};
  std::array<double, kCurrentBins> current_sum_by_bin{};
  std::array<std::array<long long, kCurrentBins>, kAmplitudeCuts.size()> isolated_outtime{};
  for (Long64_t entry = 0; entry < entries; ++entry) {
    tree->GetEntry(entry);
    if (edtm != 0. || ntrack != 1. || std::abs(dp[0]) >= 8. || p[0] <= 0. ||
        nclus <= 0 || nclus > kMax)
      continue;
    const int primary = std::distance(
        clus_e.begin(), std::max_element(clus_e.begin(), clus_e.begin() + nclus));
    const double expected =
        beam_energy + kMp - std::sqrt(p[0] * p[0] + kMp * kMp);
    const double r = clus_e[primary] - expected;
    if (std::abs(r - mean) >= 3. * sigma) continue;
    ++elastic_core;
    current_sum += current;

    const int npulse = std::min({ncounter, namp, nint, ntime, kMax});
    if (ncounter != namp || ncounter != nint || ncounter != ntime)
      ++mismatched_lengths;
    max_pulses_seen = std::max(max_pulses_seen, npulse);
    h_n.Fill(npulse);

    int reference = -1;
    double reference_amp = -std::numeric_limits<double>::infinity();
    for (int i = 0; i < npulse; ++i) {
      const int block = std::lround(counter[i]);
      if (block < 0 || block >= kBlocks) { ++invalid_counter; continue; }
      if (amp[i] > reference_amp) {
        reference = i;
        reference_amp = amp[i];
      }
    }
    if (reference < 0 || reference_amp <= 0.) continue;
    ++usable_reference;
    const double reference_time = pulse_time[reference];
    const int reference_block = std::lround(counter[reference]);
    std::array<bool, kAmplitudeCuts.size()> event_has_isolated_outtime{};

    for (int i = 0; i < npulse; ++i) {
      const int block = std::lround(counter[i]);
      if (block < 0 || block >= kBlocks) continue;
      ++all_pulses;
      const double dr = block_separation(block, reference_block);
      const double dt = pulse_time[i] - reference_time;
      h_amp.Fill(amp[i]);
      h_integral.Fill(integral[i]);
      h_time.Fill(pulse_time[i]);
      h_amp_int.Fill(amp[i], integral[i]);
      if (i == reference) continue;
      ++comparison_pulses;
      h_dt.Fill(dt);
      h_dr.Fill(dr);
      h_frac.Fill(amp[i] / reference_amp);
      h_dt_amp.Fill(dt, amp[i]);
      h_dt_dr.Fill(dr, dt);
      if (dr >= 10. && std::abs(dt) >= 8.)
        for (int cut = 0; cut < int(kAmplitudeCuts.size()); ++cut)
          if (amp[i] >= kAmplitudeCuts[cut]) event_has_isolated_outtime[cut] = true;
    }
    const int bin = current_bin(current);
    if (bin >= 0) {
      ++core_by_current[bin];
      current_sum_by_bin[bin] += current;
      for (int cut = 0; cut < int(kAmplitudeCuts.size()); ++cut)
        if (event_has_isolated_outtime[cut]) ++isolated_outtime[cut][bin];
    }
  }

  std::ofstream out(std::string(output_prefix) + "_summary.txt");
  out << std::fixed << std::setprecision(6)
      << "input=" << input << '\n'
      << "entries_examined=" << entries << '\n'
      << "elastic_core_events=" << elastic_core << '\n'
      << "usable_reference_events=" << usable_reference << '\n'
      << "mean_current_uA=" << (elastic_core ? current_sum / elastic_core : 0.) << '\n'
      << "elastic_core_mean_GeV=" << mean << '\n'
      << "elastic_core_sigma_GeV=" << sigma << '\n'
      << "mismatched_pulse_array_events=" << mismatched_lengths << '\n'
      << "invalid_adc_counter_entries=" << invalid_counter << '\n'
      << "max_pulses_seen=" << max_pulses_seen << '\n'
      << "all_pulses=" << all_pulses << '\n'
      << "comparison_pulses=" << comparison_pulses << '\n'
      << "amplitude_q10_q50_q90=" << quantile(h_amp, .10) << ','
      << quantile(h_amp, .50) << ',' << quantile(h_amp, .90) << '\n'
      << "integral_q10_q50_q90=" << quantile(h_integral, .10) << ','
      << quantile(h_integral, .50) << ',' << quantile(h_integral, .90) << '\n'
      << "pulse_time_q10_q50_q90=" << quantile(h_time, .10) << ','
      << quantile(h_time, .50) << ',' << quantile(h_time, .90) << '\n'
      << "relative_time_q10_q50_q90=" << quantile(h_dt, .10) << ','
      << quantile(h_dt, .50) << ',' << quantile(h_dt, .90) << '\n'
      << "amplitude_fraction_q10_q50_q90=" << quantile(h_frac, .10) << ','
      << quantile(h_frac, .50) << ',' << quantile(h_frac, .90) << '\n';

  std::ofstream csv(std::string(output_prefix) + ".csv");
  csv << "current_low_uA,current_high_uA,mean_current_uA,elastic_core";
  for (double cut : kAmplitudeCuts)
    csv << ",isolated_outtime_amp_ge_" << int(cut) << "mV_events"
        << ",isolated_outtime_amp_ge_" << int(cut) << "mV_fraction";
  csv << '\n' << std::fixed << std::setprecision(6);
  for (int bin = 0; bin < kCurrentBins; ++bin) {
    if (!core_by_current[bin]) continue;
    csv << kCurrentEdges[bin] << ',' << kCurrentEdges[bin + 1] << ','
        << current_sum_by_bin[bin] / core_by_current[bin] << ','
        << core_by_current[bin];
    for (int cut = 0; cut < int(kAmplitudeCuts.size()); ++cut)
      csv << ',' << isolated_outtime[cut][bin] << ','
          << double(isolated_outtime[cut][bin]) / core_by_current[bin];
    csv << '\n';
  }

  gStyle->SetOptStat(0);
  TCanvas canvas("canvas", "NPS raw block-pulse diagnostics", 1600, 1200);
  canvas.Divide(3, 3);
  canvas.cd(1); h_n.Draw();
  canvas.cd(2); gPad->SetLogy(); h_amp.Draw();
  canvas.cd(3); gPad->SetLogy(); h_integral.Draw();
  canvas.cd(4); h_time.Draw();
  canvas.cd(5); h_dt.Draw();
  canvas.cd(6); h_dr.Draw();
  canvas.cd(7); h_dt_amp.Draw("COLZ");
  canvas.cd(8); h_dt_dr.Draw("COLZ");
  canvas.cd(9); h_amp_int.Draw("COLZ");
  canvas.SaveAs((std::string(output_prefix) + ".png").c_str());
  canvas.SaveAs((std::string(output_prefix) + ".pdf").c_str());
}
