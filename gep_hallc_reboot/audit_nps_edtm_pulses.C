#include <TCanvas.h>
#include <TFile.h>
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

namespace { constexpr int kMax=8192; constexpr int kBlocks=1080; }

void audit_nps_edtm_pulses(const char* input, const char* output_prefix,
                           double current_min=22.5, double current_max=30.)
{
  TFile file(input,"READ"); auto* tree=dynamic_cast<TTree*>(file.Get("T"));
  if(file.IsZombie()||!tree){std::cerr<<"Cannot read input\n";return;}
  double edtm=0.,current=0.; int ncounter=0,namp=0,ntime=0;
  std::array<double,kMax> counter{},amp{},time{};
  tree->SetBranchAddress("T.hms.hEDTM_tdcTimeRaw",&edtm);
  tree->SetBranchAddress("H.BCM4A.scalerCurrent",&current);
  tree->SetBranchAddress("Ndata.NPS.cal.fly.adcCounter",&ncounter);
  tree->SetBranchAddress("NPS.cal.fly.adcCounter",counter.data());
  tree->SetBranchAddress("Ndata.NPS.cal.fly.adcSampPulseAmp",&namp);
  tree->SetBranchAddress("NPS.cal.fly.adcSampPulseAmp",amp.data());
  tree->SetBranchAddress("Ndata.NPS.cal.fly.adcSampPulseTime",&ntime);
  tree->SetBranchAddress("NPS.cal.fly.adcSampPulseTime",time.data());

  TH1D h_npulse("h_npulse","EDTM decoded pulses;pulses per event;events",60,-.5,59.5);
  TH1D h_amp("h_amp","EDTM decoded pulses;amplitude (mV);pulses",200,0.,200.);
  TH1D h_time("h_time","EDTM decoded pulses;pulse time (ns);pulses",220,100.,210.);
  TH1D h_block("h_block","EDTM pulse block;block ID;pulses",1080,-.5,1079.5);
  TH2D h_time_amp("h_time_amp","EDTM pulses;time (ns);amplitude (mV)",180,110.,200.,160,0.,160.);
  long events=0,events_with_pulse=0,pulses=0,invalid=0,mismatch=0;
  std::array<long,4> event_in_time{}; const double amp_cut[4]={5.,10.,20.,50.};
  for(Long64_t entry=0;entry<tree->GetEntries();++entry){
    tree->GetEntry(entry); if(edtm==0.||current<current_min||current>=current_max)continue;
    ++events; const int n=std::min({ncounter,namp,ntime,kMax});
    if(ncounter!=namp||ncounter!=ntime)++mismatch;
    h_npulse.Fill(n); if(n>0)++events_with_pulse;
    std::array<bool,4> has{};
    for(int i=0;i<n;++i){
      const int block=std::lround(counter[i]); if(block<0||block>=kBlocks){++invalid;continue;}
      ++pulses; h_amp.Fill(amp[i]);h_time.Fill(time[i]);h_block.Fill(block);h_time_amp.Fill(time[i],amp[i]);
      if(std::abs(time[i]-162.4)<4.) for(int cut=0;cut<4;++cut) if(amp[i]>=amp_cut[cut])has[cut]=true;
    }
    for(int cut=0;cut<4;++cut)if(has[cut])++event_in_time[cut];
  }
  std::ofstream summary(std::string(output_prefix)+"_summary.txt");
  summary<<std::setprecision(10)<<"input="<<input<<'\n'<<"current_range_uA="<<current_min<<','<<current_max<<'\n'
         <<"edtm_events="<<events<<'\n'<<"events_with_decoded_pulse="<<events_with_pulse<<'\n'
         <<"fraction_with_decoded_pulse="<<(events?double(events_with_pulse)/events:0.)<<'\n'
         <<"decoded_pulses="<<pulses<<'\n'<<"mean_pulses_per_event="<<(events?double(pulses)/events:0.)<<'\n'
         <<"mismatched_arrays="<<mismatch<<'\n'<<"invalid_blocks="<<invalid<<'\n';
  for(int cut=0;cut<4;++cut)summary<<"fraction_event_pulse_amp_ge_"<<amp_cut[cut]<<"mV_absdt4ns="
    <<(events?double(event_in_time[cut])/events:0.)<<'\n';
  gStyle->SetOptStat(0);TCanvas canvas("canvas","NPS EDTM pulses",1400,950);canvas.Divide(2,2);
  canvas.cd(1);h_npulse.Draw();canvas.cd(2);gPad->SetLogy();h_amp.Draw();
  canvas.cd(3);h_time_amp.Draw("colz");canvas.cd(4);h_block.Draw();
  canvas.SaveAs((std::string(output_prefix)+".png").c_str());
  canvas.SaveAs((std::string(output_prefix)+".pdf").c_str());
  std::cout<<"events="<<events<<" with_pulse="<<events_with_pulse<<" pulses="<<pulses<<'\n';
}
