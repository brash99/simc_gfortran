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

namespace {
constexpr int kMax = 4096;
constexpr double kMp = 0.9382720813;
}

// Export one row per additional cluster in a proton-tagged elastic-core event.
// Events without an additional cluster receive one row with secondary_index=-1.
void export_nps_overlay_library(const char* input, const char* output,
                                double beam_energy, double nominal_elastic_energy,
                                double current_min, double current_max)
{
  TFile file(input,"READ");
  auto* tree=dynamic_cast<TTree*>(file.Get("T"));
  if(file.IsZombie()||!tree){std::cerr<<"Cannot read "<<input<<'\n';return;}
  double edtm=0.,ntrack=0.,current=0.; int nclus=0;
  std::array<double,kMax> p{},dp{},energy{},time{},x{},y{};
  tree->SetBranchAddress("T.hms.hEDTM_tdcTimeRaw",&edtm);
  tree->SetBranchAddress("H.dc.ntrack",&ntrack);
  tree->SetBranchAddress("H.gtr.p",p.data()); tree->SetBranchAddress("H.gtr.dp",dp.data());
  tree->SetBranchAddress("H.BCM4A.scalerCurrent",&current);
  tree->SetBranchAddress("Ndata.NPS.cal.clusE",&nclus);
  tree->SetBranchAddress("NPS.cal.clusE",energy.data()); tree->SetBranchAddress("NPS.cal.clusT",time.data());
  tree->SetBranchAddress("NPS.cal.clusX",x.data()); tree->SetBranchAddress("NPS.cal.clusY",y.data());

  TH1D residuals("residuals","",240,-.45*nominal_elastic_energy,.2*nominal_elastic_energy);
  for(Long64_t entry=0;entry<tree->GetEntries();++entry){
    tree->GetEntry(entry);
    if(edtm!=0.||ntrack!=1.||std::abs(dp[0])>=8.||p[0]<=0.||nclus<=0||nclus>kMax)continue;
    const int primary=std::distance(energy.begin(),std::max_element(energy.begin(),energy.begin()+nclus));
    const double expected=beam_energy+kMp-std::sqrt(p[0]*p[0]+kMp*kMp);
    residuals.Fill(energy[primary]-expected);
  }
  const double peak=residuals.GetBinCenter(residuals.GetMaximumBin());
  TF1 fit1("fit1","gaus",peak-.08*nominal_elastic_energy,peak+.08*nominal_elastic_energy);
  residuals.Fit(&fit1,"RQ0");
  TF1 fit2("fit2","gaus",fit1.GetParameter(1)-2.*std::abs(fit1.GetParameter(2)),
                           fit1.GetParameter(1)+2.*std::abs(fit1.GetParameter(2)));
  residuals.Fit(&fit2,"RQ0");
  const double mean=fit2.GetParameter(1),sigma=std::abs(fit2.GetParameter(2));
  const double low=mean-3.*sigma,high=mean+3.*sigma;

  std::ofstream csv(output); csv<<std::setprecision(10);
  csv<<"source_entry,current_uA,nclus,primary_energy_GeV,primary_time_ns,primary_x_cm,primary_y_cm,secondary_index,secondary_energy_GeV,secondary_dt_ns,secondary_x_cm,secondary_y_cm,secondary_dr_cm\n";
  long core_events=0,multicluster_events=0,rows=0;
  for(Long64_t entry=0;entry<tree->GetEntries();++entry){
    tree->GetEntry(entry);
    if(edtm!=0.||ntrack!=1.||std::abs(dp[0])>=8.||p[0]<=0.||nclus<=0||nclus>kMax||
       current<current_min||current>=current_max)continue;
    const int primary=std::distance(energy.begin(),std::max_element(energy.begin(),energy.begin()+nclus));
    const double expected=beam_energy+kMp-std::sqrt(p[0]*p[0]+kMp*kMp);
    const double residual=energy[primary]-expected;
    if(residual<low||residual>high)continue;
    ++core_events;
    if(nclus==1){
      csv<<entry<<','<<current<<','<<nclus<<','<<energy[primary]<<','<<time[primary]<<','
         <<x[primary]<<','<<y[primary]<<",-1,0,0,0,0,0\n"; ++rows; continue;
    }
    ++multicluster_events;
    int secondary_index=0;
    for(int cluster=0;cluster<nclus;++cluster){
      if(cluster==primary)continue;
      csv<<entry<<','<<current<<','<<nclus<<','<<energy[primary]<<','<<time[primary]<<','
         <<x[primary]<<','<<y[primary]<<','<<secondary_index++<<','<<energy[cluster]<<','
         <<time[cluster]-time[primary]<<','<<x[cluster]<<','<<y[cluster]<<','
         <<std::hypot(x[cluster]-x[primary],y[cluster]-y[primary])<<'\n'; ++rows;
    }
  }
  csv.close();
  std::cout<<"core_events="<<core_events<<" multicluster_events="<<multicluster_events
           <<" rows="<<rows<<" mean="<<mean<<" sigma="<<sigma<<'\n';
}
