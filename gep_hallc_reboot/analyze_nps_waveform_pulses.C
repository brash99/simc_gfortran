#include <TCanvas.h>
#include <TFile.h>
#include <TF1.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TProfile.h>
#include <TStyle.h>
#include <TTree.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

namespace {
constexpr int kMax = 16384;
constexpr int kWaveMax = 200000;
constexpr int kBlocks = 1080;
constexpr int kColumns = 30;
constexpr double kPitchCm = 2.16;
constexpr double kMp = 0.9382720813;

double separation(int first, int second) {
  return kPitchCm * std::hypot(first / kColumns - second / kColumns,
                              first % kColumns - second % kColumns);
}

double median(std::vector<double> values) {
  if (values.empty()) return 0.;
  const auto middle = values.begin() + values.size() / 2;
  std::nth_element(values.begin(), middle, values.end());
  double result = *middle;
  if (values.size() % 2 == 0) {
    const auto lower = std::max_element(values.begin(), middle);
    result = .5 * (result + *lower);
  }
  return result;
}

struct Example {
  Long64_t entry;
  int block;
  double amplitude, dt, dr;
  std::vector<double> samples;
};
struct Metric {
  double ratio, timing_residual, neighbor_ratio;
};
}

// Validate isolated decoded NPS pulses against their stored FADC waveforms.
void analyze_nps_waveform_pulses(const char* input, const char* output_prefix,
                                 double beam_energy, double nominal_elastic_energy,
                                 Long64_t max_entries = 5000) {
  TFile file(input, "READ");
  auto* tree = dynamic_cast<TTree*>(file.Get("T"));
  if (file.IsZombie() || !tree) {
    std::cerr << "Cannot read " << input << std::endl;
    return;
  }

  double edtm = 0., ntrack = 0.;
  int nclus = 0, ncounter = 0, namp = 0, ntime = 0, nwave = 0;
  std::array<double, kMax> p{}, dp{}, clus_e{}, counter{}, amp{}, pulse_time{};
  std::array<double, kWaveMax> waveform{};
  tree->SetBranchAddress("T.hms.hEDTM_tdcTimeRaw", &edtm);
  tree->SetBranchAddress("H.dc.ntrack", &ntrack);
  tree->SetBranchAddress("H.gtr.p", p.data());
  tree->SetBranchAddress("H.gtr.dp", dp.data());
  tree->SetBranchAddress("Ndata.NPS.cal.clusE", &nclus);
  tree->SetBranchAddress("NPS.cal.clusE", clus_e.data());
  tree->SetBranchAddress("Ndata.NPS.cal.fly.adcCounter", &ncounter);
  tree->SetBranchAddress("NPS.cal.fly.adcCounter", counter.data());
  tree->SetBranchAddress("Ndata.NPS.cal.fly.adcSampPulseAmp", &namp);
  tree->SetBranchAddress("NPS.cal.fly.adcSampPulseAmp", amp.data());
  tree->SetBranchAddress("Ndata.NPS.cal.fly.adcSampPulseTime", &ntime);
  tree->SetBranchAddress("NPS.cal.fly.adcSampPulseTime", pulse_time.data());
  tree->SetBranchAddress("Ndata.NPS.cal.fly.adcSampWaveform", &nwave);
  tree->SetBranchAddress("NPS.cal.fly.adcSampWaveform", waveform.data());

  const Long64_t entries = std::min(max_entries, tree->GetEntries());
  TH1D residual("residual", "", 240, -.45 * nominal_elastic_energy,
                .2 * nominal_elastic_energy);
  for (Long64_t entry = 0; entry < entries; ++entry) {
    tree->GetEntry(entry);
    if (edtm != 0. || ntrack != 1. || std::abs(dp[0]) >= 8. || p[0] <= 0. ||
        nclus <= 0 || nclus > kMax) continue;
    const int primary = std::distance(
        clus_e.begin(), std::max_element(clus_e.begin(), clus_e.begin() + nclus));
    const double expected = beam_energy + kMp - std::sqrt(p[0] * p[0] + kMp * kMp);
    residual.Fill(clus_e[primary] - expected);
  }
  const double peak = residual.GetBinCenter(residual.GetMaximumBin());
  TF1 fit1("fit1", "gaus", peak - .08 * nominal_elastic_energy,
           peak + .08 * nominal_elastic_energy);
  residual.Fit(&fit1, "RQ0");
  TF1 fit2("fit2", "gaus", fit1.GetParameter(1) - 2. * std::abs(fit1.GetParameter(2)),
           fit1.GetParameter(1) + 2. * std::abs(fit1.GetParameter(2)));
  residual.Fit(&fit2, "RQ0");
  const double core_mean = fit2.GetParameter(1);
  const double core_sigma = std::abs(fit2.GetParameter(2));

  TH1D h_ratio("h_ratio", "Waveform/decoded pulse amplitude;local waveform peak / decoded amplitude;pulses", 200, 0, 2.5);
  TH1D h_tres("h_tres", "Waveform-peak timing residual;4 #times peak sample - decoded time (ns);pulses", 160, -40, 40);
  TH1D h_rms("h_rms", "Off-pulse waveform RMS;RMS (mV);pulses", 160, 0, 20);
  TH1D h_width("h_width", "Pulse neighbor ratio;(left + right) / (2 peak);pulses", 160, -.2, 1.4);
  TH2D h_amp_match("h_amp_match", "Waveform peak versus decoded amplitude;decoded amplitude (mV);waveform peak (mV)", 180, 0, 180, 180, 0, 180);
  TProfile p_shape("p_shape", "Amplitude-normalized waveform near decoded pulse;time relative to decoded pulse (ns);waveform / decoded amplitude", 25, -50, 50);

  long long elastic_core = 0, candidate_pulses = 0, packet_found = 0;
  long long coherent = 0, malformed_packets = 0;
  std::vector<Example> examples;
  std::vector<Metric> metrics;
  std::array<int, kBlocks> wave_offset{}, wave_samples{};
  for (Long64_t entry = 0; entry < entries; ++entry) {
    tree->GetEntry(entry);
    if (edtm != 0. || ntrack != 1. || std::abs(dp[0]) >= 8. || p[0] <= 0. ||
        nclus <= 0 || nclus > kMax) continue;
    const int primary = std::distance(
        clus_e.begin(), std::max_element(clus_e.begin(), clus_e.begin() + nclus));
    const double expected = beam_energy + kMp - std::sqrt(p[0] * p[0] + kMp * kMp);
    if (std::abs(clus_e[primary] - expected - core_mean) >= 3. * core_sigma) continue;
    ++elastic_core;

    wave_offset.fill(-1);
    wave_samples.fill(0);
    for (int cursor = 0; cursor + 1 < nwave;) {
      const int block = std::lround(waveform[cursor]);
      const int samples = std::lround(waveform[cursor + 1]);
      if (samples < 0 || cursor + 2 + samples > nwave) {
        ++malformed_packets;
        break;
      }
      if (block >= 0 && block < kBlocks) {
        wave_offset[block] = cursor + 2;
        wave_samples[block] = samples;
      }
      cursor += samples + 2;
    }

    const int npulse = std::min({ncounter, namp, ntime, kMax});
    int reference = -1;
    for (int i = 0; i < npulse; ++i) {
      const int block = std::lround(counter[i]);
      if (block >= 0 && block < kBlocks &&
          (reference < 0 || amp[i] > amp[reference])) reference = i;
    }
    if (reference < 0) continue;
    const int reference_block = std::lround(counter[reference]);
    const double reference_time = pulse_time[reference];

    for (int i = 0; i < npulse; ++i) {
      if (i == reference || amp[i] < 10.) continue;
      const int block = std::lround(counter[i]);
      if (block < 0 || block >= kBlocks) continue;
      const double dt = pulse_time[i] - reference_time;
      const double dr = separation(block, reference_block);
      if (dr < 10. || std::abs(dt) < 8.) continue;
      ++candidate_pulses;
      const int offset = wave_offset[block], samples = wave_samples[block];
      if (offset < 0 || samples <= 0) continue;
      ++packet_found;

      const double predicted = pulse_time[i] / 4.;
      const int center = std::lround(predicted);
      const int low = std::max(0, center - 3), high = std::min(samples - 1, center + 3);
      int maximum_sample = low;
      for (int sample = low + 1; sample <= high; ++sample)
        if (waveform[offset + sample] > waveform[offset + maximum_sample]) maximum_sample = sample;
      const double local_peak = waveform[offset + maximum_sample];

      std::vector<double> baseline_samples;
      baseline_samples.reserve(samples);
      for (int sample = 0; sample < samples; ++sample)
        if (std::abs(sample - center) > 8) baseline_samples.push_back(waveform[offset + sample]);
      const double baseline = median(baseline_samples);
      double sumsq = 0.;
      for (double value : baseline_samples) sumsq += (value - baseline) * (value - baseline);
      const double rms = baseline_samples.empty() ? 0. : std::sqrt(sumsq / baseline_samples.size());
      const double corrected_peak = local_peak - baseline;
      const double ratio = corrected_peak / amp[i];
      const double left = maximum_sample > 0 ? waveform[offset + maximum_sample - 1] - baseline : 0.;
      const double right = maximum_sample + 1 < samples ? waveform[offset + maximum_sample + 1] - baseline : 0.;
      const double neighbor_ratio = corrected_peak > 0. ? (left + right) / (2. * corrected_peak) : 0.;
      const double timing_residual = 4. * maximum_sample - pulse_time[i];
      h_ratio.Fill(ratio);
      h_tres.Fill(timing_residual);
      h_rms.Fill(rms);
      h_width.Fill(neighbor_ratio);
      h_amp_match.Fill(amp[i], corrected_peak);
      metrics.push_back({ratio, timing_residual, neighbor_ratio});
      for (int relative = -12; relative <= 12; ++relative) {
        const int sample = center + relative;
        if (sample >= 0 && sample < samples)
          p_shape.Fill(4. * relative, (waveform[offset + sample] - baseline) / amp[i]);
      }
      if (examples.size() < 12 && corrected_peak >= .5 * amp[i] &&
          neighbor_ratio > .05)
        examples.push_back({entry, block, amp[i], dt, dr,
                            std::vector<double>(waveform.begin() + offset,
                                                waveform.begin() + offset + samples)});
    }
  }

  const double timing_offset = h_tres.GetBinCenter(h_tres.GetMaximumBin());
  for (const auto& metric : metrics)
    if (metric.ratio >= .5 &&
        std::abs(metric.timing_residual - timing_offset) <= 8. &&
        metric.neighbor_ratio > .05)
      ++coherent;

  std::ofstream summary(std::string(output_prefix) + "_summary.txt");
  summary << std::fixed << std::setprecision(6)
          << "input=" << input << '\n'
          << "entries_examined=" << entries << '\n'
          << "elastic_core_events=" << elastic_core << '\n'
          << "candidate_pulses=" << candidate_pulses << '\n'
          << "waveform_packet_found=" << packet_found << '\n'
          << "coherent_waveform_pulses=" << coherent << '\n'
          << "packet_found_fraction=" << (candidate_pulses ? double(packet_found) / candidate_pulses : 0.) << '\n'
          << "coherent_fraction_of_found=" << (packet_found ? double(coherent) / packet_found : 0.) << '\n'
          << "fitted_timing_offset_ns=" << timing_offset << '\n'
          << "malformed_waveform_packets=" << malformed_packets << '\n'
          << "coherent_definition=waveform peak >= 0.5 decoded amplitude; |timing residual - fitted offset| <= 8 ns; neighbor ratio > 0.05\n"
          << "candidate_definition=decoded amplitude >= 10 mV; block separation >= 10 cm; |relative time| >= 8 ns\n";

  gStyle->SetOptStat(0);
  TCanvas diagnostics("diagnostics", "NPS waveform validation", 1500, 900);
  diagnostics.Divide(3, 2);
  diagnostics.cd(1); h_ratio.Draw();
  diagnostics.cd(2); h_tres.Draw();
  diagnostics.cd(3); h_rms.Draw();
  diagnostics.cd(4); h_width.Draw();
  diagnostics.cd(5); h_amp_match.Draw("COLZ");
  diagnostics.cd(6); p_shape.Draw();
  diagnostics.SaveAs((std::string(output_prefix) + "_diagnostics.png").c_str());
  diagnostics.SaveAs((std::string(output_prefix) + "_diagnostics.pdf").c_str());

  TCanvas gallery("gallery", "Candidate waveforms", 1600, 1100);
  gallery.Divide(4, 3);
  std::vector<TH1D*> example_histograms;
  for (int index = 0; index < int(examples.size()); ++index) {
    const auto& example = examples[index];
    auto* h = new TH1D(Form("example_%d", index),
        Form("entry %lld, block %d;waveform time (ns);amplitude (mV)",
             example.entry, example.block), example.samples.size(), 0,
        4. * example.samples.size());
    for (int sample = 0; sample < int(example.samples.size()); ++sample)
      h->SetBinContent(sample + 1, example.samples[sample]);
    h->SetLineWidth(2);
    gallery.cd(index + 1);
    h->Draw("HIST");
    example_histograms.push_back(h);
  }
  gallery.SaveAs((std::string(output_prefix) + "_gallery.png").c_str());
  gallery.SaveAs((std::string(output_prefix) + "_gallery.pdf").c_str());
}
