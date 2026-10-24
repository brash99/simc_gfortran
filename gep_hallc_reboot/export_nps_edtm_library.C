#include <TFile.h>
#include <TF1.h>
#include <TH1D.h>
#include <TTree.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>

namespace { constexpr int kMax=8192; constexpr int kBlocks=1080; constexpr int kColumns=30; constexpr double kMp=.9382720813; }

bool neighbor3x3(int first,int second){
  return std::abs(first/kColumns-second/kColumns)<=1 &&
         std::abs(first%kColumns-second%kColumns)<=1;
}

void export_nps_edtm_library(const char* input,const char* output_prefix,
                             double beam_energy,double nominal_elastic_energy,
                             double current_min=22.5,double current_max=30.)
{
  TFile file(input,"READ");auto* tree=dynamic_cast<TTree*>(file.Get("T"));
  if(file.IsZombie()||!tree){std::cerr<<"Cannot read input\n";return;}
  double edtm=0.,ntrack=0.,current=0.;int nclus=0,ncounter=0,namp=0,ntime=0,nint=0;
  std::array<double,kMax> p{},dp{},clus_e{},counter{},amp{},time{},integral{};
  tree->SetBranchAddress("T.hms.hEDTM_tdcTimeRaw",&edtm);tree->SetBranchAddress("H.dc.ntrack",&ntrack);
  tree->SetBranchAddress("H.gtr.p",p.data());tree->SetBranchAddress("H.gtr.dp",dp.data());
  tree->SetBranchAddress("H.BCM4A.scalerCurrent",&current);tree->SetBranchAddress("Ndata.NPS.cal.clusE",&nclus);
  tree->SetBranchAddress("NPS.cal.clusE",clus_e.data());tree->SetBranchAddress("Ndata.NPS.cal.fly.adcCounter",&ncounter);
  tree->SetBranchAddress("NPS.cal.fly.adcCounter",counter.data());tree->SetBranchAddress("Ndata.NPS.cal.fly.adcSampPulseAmp",&namp);
  tree->SetBranchAddress("NPS.cal.fly.adcSampPulseAmp",amp.data());tree->SetBranchAddress("Ndata.NPS.cal.fly.adcSampPulseTime",&ntime);
  tree->SetBranchAddress("NPS.cal.fly.adcSampPulseTime",time.data());tree->SetBranchAddress("Ndata.NPS.cal.fly.adcSampPulseInt",&nint);
  tree->SetBranchAddress("NPS.cal.fly.adcSampPulseInt",integral.data());

  TH1D residual("residual","",240,-.45*nominal_elastic_energy,.2*nominal_elastic_energy);
  for(Long64_t entry=0;entry<tree->GetEntries();++entry){tree->GetEntry(entry);
    if(edtm!=0.||ntrack!=1.||std::abs(dp[0])>=8.||p[0]<=0.||nclus<=0||nclus>kMax)continue;
    const int primary=std::distance(clus_e.begin(),std::max_element(clus_e.begin(),clus_e.begin()+nclus));
    const double expected=beam_energy+kMp-std::sqrt(p[0]*p[0]+kMp*kMp);residual.Fill(clus_e[primary]-expected);
  }
  const double peak=residual.GetBinCenter(residual.GetMaximumBin());TF1 fit1("fit1","gaus",peak-.08*nominal_elastic_energy,peak+.08*nominal_elastic_energy);
  residual.Fit(&fit1,"RQ0");TF1 fit2("fit2","gaus",fit1.GetParameter(1)-2.*std::abs(fit1.GetParameter(2)),fit1.GetParameter(1)+2.*std::abs(fit1.GetParameter(2)));
  residual.Fit(&fit2,"RQ0");const double mean=fit2.GetParameter(1),sigma=std::abs(fit2.GetParameter(2));

  double sumxx=0.,sumxy=0.;long calibration_events=0;
  for(Long64_t entry=0;entry<tree->GetEntries();++entry){tree->GetEntry(entry);
    if(edtm!=0.||current<current_min||current>=current_max||ntrack!=1.||std::abs(dp[0])>=8.||p[0]<=0.||nclus<=0||nclus>kMax)continue;
    const int primary=std::distance(clus_e.begin(),std::max_element(clus_e.begin(),clus_e.begin()+nclus));
    const double expected=beam_energy+kMp-std::sqrt(p[0]*p[0]+kMp*kMp);
    if(std::abs((clus_e[primary]-expected)-mean)>=3.*sigma)continue;
    const int n=std::min({ncounter,namp,ntime,kMax});int reference=-1;double maxamp=-1.;
    for(int i=0;i<n;++i){const int block=std::lround(counter[i]);if(block>=0&&block<kBlocks&&amp[i]>maxamp){reference=i;maxamp=amp[i];}}
    if(reference<0)continue;const int refblock=std::lround(counter[reference]);const double reftime=time[reference];double sumamp=0.;
    for(int i=0;i<n;++i){const int block=std::lround(counter[i]);if(block>=0&&block<kBlocks&&neighbor3x3(block,refblock)&&std::abs(time[i]-reftime)<4.)sumamp+=amp[i];}
    if(sumamp<=0.)continue;sumxx+=sumamp*sumamp;sumxy+=sumamp*clus_e[primary];++calibration_events;
  }
  const double gev_per_mv=sumxx>0.?sumxy/sumxx:0.;

  std::ofstream csv(std::string(output_prefix)+".csv");csv<<std::setprecision(10);
  csv<<"source_entry,current_uA,pulse_index,block,amplitude_mV,integral,pulse_time_ns\n";
  long edtm_events=0,rows=0,pulses=0;double current_sum=0.;
  for(Long64_t entry=0;entry<tree->GetEntries();++entry){tree->GetEntry(entry);
    if(edtm==0.||current<current_min||current>=current_max)continue;++edtm_events;current_sum+=current;
    const int n=std::min({ncounter,namp,nint,ntime,kMax});int written=0;
    for(int i=0;i<n;++i){const int block=std::lround(counter[i]);if(block<0||block>=kBlocks)continue;
      csv<<entry<<','<<current<<','<<written++<<','<<block<<','<<amp[i]<<','<<integral[i]<<','<<time[i]<<'\n';++rows;++pulses;}
    if(!written){csv<<entry<<','<<current<<",-1,-1,0,0,0\n";++rows;}
  }csv.close();
  std::ofstream summary(std::string(output_prefix)+"_summary.txt");summary<<std::setprecision(10)
    <<"edtm_events="<<edtm_events<<'\n'<<"mean_current_uA="<<(edtm_events?current_sum/edtm_events:0.)<<'\n'
    <<"decoded_pulses="<<pulses<<'\n'<<"csv_rows="<<rows<<'\n'<<"elastic_calibration_events="<<calibration_events<<'\n'
    <<"local_3x3_GeV_per_mV="<<gev_per_mv<<'\n'<<"elastic_core_mean_GeV="<<mean<<'\n'<<"elastic_core_sigma_GeV="<<sigma<<'\n';
  std::cout<<"edtm_events="<<edtm_events<<" pulses="<<pulses<<" calibration_events="<<calibration_events
           <<" GeV_per_mV="<<gev_per_mv<<'\n';
}
