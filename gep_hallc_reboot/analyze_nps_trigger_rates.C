#include <TFile.h>
#include <TTree.h>

#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {
constexpr int kNTrig = 6;
constexpr int kNBands = 5;
constexpr int kMaxVTPTriggers = 256;

struct Accumulator {
  long n = 0;
  double sum = 0.;
  double sum2 = 0.;
  void add(double value) { ++n; sum += value; sum2 += value*value; }
  double mean() const { return n ? sum/n : 0.; }
  double rms() const {
    return n ? std::sqrt(std::max(0.,sum2/n-mean()*mean())) : 0.;
  }
};

int current_band(double current) {
  constexpr std::array<double,kNBands+1> edges{{0.,5.,15.,22.5,26.5,30.}};
  for(int band=0;band<kNBands;++band)
    if(current>=edges[band] && current<edges[band+1]) return band;
  return -1;
}
}

void analyze_nps_trigger_rates(const char* input,
                               const char* output_prefix="nps_run6151_trigger_rates")
{
  TFile file(input,"READ");
  auto* scaler=dynamic_cast<TTree*>(file.Get("TSH"));
  auto* events=dynamic_cast<TTree*>(file.Get("T"));
  if(file.IsZombie() || !scaler || !events){
    std::cerr<<"Missing input, TSH, or T tree\n"; return;
  }

  double current=0.,clock_time=0.,l1_rate=0.,edtm_rate=0.;
  std::array<double,kNTrig> hrate{};
  scaler->SetBranchAddress("H.BCM4A.scalerCurrent",&current);
  scaler->SetBranchAddress("H.1MHz.scalerTime",&clock_time);
  scaler->SetBranchAddress("H.hL1ACCP.scalerRate",&l1_rate);
  scaler->SetBranchAddress("H.EDTM.scalerRate",&edtm_rate);
  for(int trigger=0;trigger<kNTrig;++trigger)
    scaler->SetBranchAddress(("H.hTRIG"+std::to_string(trigger+1)+".scalerRate").c_str(),
                             &hrate[trigger]);

  std::ofstream snapshots(std::string(output_prefix)+"_snapshots.csv");
  snapshots<<"entry,time_s,current_uA";
  for(int trigger=1;trigger<=kNTrig;++trigger) snapshots<<",hTRIG"<<trigger<<"_rate_Hz";
  snapshots<<",L1_accept_rate_Hz,EDTM_rate_Hz\n"<<std::setprecision(10);
  std::array<std::array<Accumulator,kNTrig>,kNBands> band_trigger{};
  std::array<Accumulator,kNBands> band_current{},band_l1{},band_edtm{};
  for(Long64_t entry=0;entry<scaler->GetEntries();++entry){
    scaler->GetEntry(entry);
    snapshots<<entry<<','<<clock_time<<','<<current;
    for(double rate:hrate) snapshots<<','<<rate;
    snapshots<<','<<l1_rate<<','<<edtm_rate<<'\n';
    const int band=current_band(current); if(band<0) continue;
    band_current[band].add(current); band_l1[band].add(l1_rate); band_edtm[band].add(edtm_rate);
    for(int trigger=0;trigger<kNTrig;++trigger) band_trigger[band][trigger].add(hrate[trigger]);
  }
  snapshots.close();

  std::ofstream bands(std::string(output_prefix)+"_bands.csv");
  bands<<"current_low_uA,current_high_uA,snapshots,mean_current_uA,rms_current_uA";
  for(int trigger=1;trigger<=kNTrig;++trigger)
    bands<<",hTRIG"<<trigger<<"_mean_Hz,hTRIG"<<trigger<<"_rms_Hz";
  bands<<",L1_accept_mean_Hz,L1_accept_rms_Hz,EDTM_mean_Hz,EDTM_rms_Hz\n";
  constexpr std::array<double,kNBands+1> edges{{0.,5.,15.,22.5,26.5,30.}};
  bands<<std::setprecision(10);
  for(int band=0;band<kNBands;++band){
    bands<<edges[band]<<','<<edges[band+1]<<','<<band_current[band].n<<','
         <<band_current[band].mean()<<','<<band_current[band].rms();
    for(int trigger=0;trigger<kNTrig;++trigger)
      bands<<','<<band_trigger[band][trigger].mean()<<','<<band_trigger[band][trigger].rms();
    bands<<','<<band_l1[band].mean()<<','<<band_l1[band].rms()<<','
         <<band_edtm[band].mean()<<','<<band_edtm[band].rms()<<'\n';
  }
  bands.close();

  double edtm=0.;
  std::array<double,kNTrig> hmult{},npsmult{};
  std::array<int,kNTrig> nvtype{};
  std::array<std::array<double,kMaxVTPTriggers>,kNTrig> vtype{};
  events->SetBranchAddress("T.hms.hEDTM_tdcTimeRaw",&edtm);
  for(int trigger=0;trigger<kNTrig;++trigger){
    events->SetBranchAddress(("T.hms.hTRIG"+std::to_string(trigger+1)+"_tdcMultiplicity").c_str(),
                             &hmult[trigger]);
    events->SetBranchAddress(("T.hms.npsTRIG"+std::to_string(trigger+1)+"_tdcMultiplicity").c_str(),
                             &npsmult[trigger]);
    events->SetBranchAddress(("Ndata.NPS.cal.vtpTrigType"+std::to_string(trigger)).c_str(),
                             &nvtype[trigger]);
    events->SetBranchAddress(("NPS.cal.vtpTrigType"+std::to_string(trigger)).c_str(),
                             vtype[trigger].data());
  }
  std::array<long,kNTrig> hcount{},npscount{},vtypecount{};
  std::array<std::array<long,kNTrig>,kNTrig> h_nps{},h_vtype{};
  long ordinary=0,edtm_events=0;
  for(Long64_t entry=0;entry<events->GetEntries();++entry){
    events->GetEntry(entry); const bool is_edtm=edtm!=0.;
    if(is_edtm) ++edtm_events; else ++ordinary;
    std::array<bool,kNTrig> has_vtype{};
    for(int type=0;type<kNTrig;++type){
      if(nvtype[type]<0 || nvtype[type]>kMaxVTPTriggers){
        std::cerr<<"VTP trigger-vector overflow at entry "<<entry<<'\n'; return;
      }
      for(int index=0;index<nvtype[type];++index)
        if(vtype[type][index]!=0.) has_vtype[type]=true;
    }
    for(int h=0;h<kNTrig;++h){
      const bool has_h=hmult[h]>0.; const bool has_nps=npsmult[h]>0.;
      if(has_h) ++hcount[h]; if(has_nps) ++npscount[h]; if(has_vtype[h]) ++vtypecount[h];
      if(!has_h) continue;
      for(int other=0;other<kNTrig;++other){
        if(npsmult[other]>0.) ++h_nps[h][other];
        if(has_vtype[other]) ++h_vtype[h][other];
      }
    }
  }
  std::ofstream corr(std::string(output_prefix)+"_event_correlations.csv");
  corr<<"hTDC_channel,hTDC_events";
  for(int trigger=1;trigger<=kNTrig;++trigger) corr<<",with_npsTRIG"<<trigger;
  for(int type=0;type<kNTrig;++type) corr<<",with_VTP_type"<<type;
  corr<<'\n';
  for(int h=0;h<kNTrig;++h){
    corr<<h+1<<','<<hcount[h];
    for(int other=0;other<kNTrig;++other) corr<<','<<h_nps[h][other];
    for(int other=0;other<kNTrig;++other) corr<<','<<h_vtype[h][other];
    corr<<'\n';
  }
  corr<<"npsTDC_total,";
  for(int trigger=0;trigger<kNTrig;++trigger) corr<<(trigger?",":"")<<npscount[trigger];
  corr<<"\nVTP_type_total,";
  for(int type=0;type<kNTrig;++type) corr<<(type?",":"")<<vtypecount[type];
  corr<<'\n'; corr.close();

  std::ofstream summary(std::string(output_prefix)+"_summary.txt");
  summary<<"scaler_snapshots="<<scaler->GetEntries()<<'\n'
         <<"event_entries="<<events->GetEntries()<<'\n'
         <<"ordinary_events="<<ordinary<<'\n'
         <<"edtm_events="<<edtm_events<<'\n';
  for(int trigger=0;trigger<kNTrig;++trigger)
    summary<<"hTRIG"<<trigger+1<<"_tdc_events="<<hcount[trigger]<<'\n';
  for(int trigger=0;trigger<kNTrig;++trigger)
    summary<<"npsTRIG"<<trigger+1<<"_tdc_events="<<npscount[trigger]<<'\n';
  for(int type=0;type<kNTrig;++type)
    summary<<"VTP_type"<<type<<"_events="<<vtypecount[type]<<'\n';
  summary.close();
  std::cout<<"TSH snapshots="<<scaler->GetEntries()<<" T events="<<events->GetEntries()<<'\n';
}
