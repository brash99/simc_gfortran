#include <TCanvas.h>
#include <TFile.h>
#include <TGraph.h>
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
#include <vector>

namespace {
constexpr int kHorizontal = 30;
constexpr int kVertical = 36;
constexpr int kBlocks = kHorizontal*kVertical;

struct Event {
  double weight;
  double total_fraction;
  double sum3x3_fraction;
  double local_x;
  double local_y;
};

double Sum3x3(const double* edep, int seed) {
  // GEANT4 numbers crystals column-by-column: vertical + 36*horizontal.
  const int seed_horizontal = seed/kVertical;
  const int seed_vertical = seed%kVertical;
  double sum = 0.;
  for (int horizontal = std::max(0,seed_horizontal-1);
       horizontal <= std::min(kHorizontal-1,seed_horizontal+1); ++horizontal)
    for (int vertical = std::max(0,seed_vertical-1);
         vertical <= std::min(kVertical-1,seed_vertical+1); ++vertical)
      sum += edep[vertical+kVertical*horizontal];
  return sum;
}
}

void analyze_gep_q11_trigger_sum()
{
  const char* base = "/work/hallc/gep/brash/gep4_nps_geant4/validation/";
  TFile input(TString(base)+"gep_reboot_q11_hybrid_10923.root");
  auto* tree = static_cast<TTree*>(input.Get("t_dvcs"));
  if (!tree || tree->GetEntries()!=10923) { std::cerr<<"Expected 10923 entries\n"; return; }

  double edep[kBlocks], weight, px, py, pz, vx, vy, vz;
  tree->SetBranchAddress("edep",edep); tree->SetBranchAddress("psf",&weight);
  tree->SetBranchAddress("GSE_px",&px); tree->SetBranchAddress("GSE_py",&py);
  tree->SetBranchAddress("GSE_pz",&pz); tree->SetBranchAddress("GV_x",&vx);
  tree->SetBranchAddress("GV_y",&vy); tree->SetBranchAddress("GV_z",&vz);

  const double me=0.510999e-3, theta=26.651*TMath::DegToRad(), distance=3000.;
  const double nx=std::sin(theta), nz=std::cos(theta);
  const double ux=std::cos(theta), uz=-std::sin(theta);
  std::vector<Event> events;
  std::ofstream event_csv(TString(base)+"gep_reboot_q11_trigger_events.csv");
  event_csv<<"event,weight,primary_energy_GeV,total_fraction,sum3x3_fraction,local_x_mm,local_y_mm,seed_horizontal,seed_vertical\n";
  event_csv<<std::setprecision(10);
  TH1D h_total("h_total","Local trigger-sum response;Energy / E_{primary};Weighted events",120,0.,1.08);
  TH1D h_3x3("h_3x3","",120,0.,1.08);
  TH1D h_capture("h_capture","3x3 shower capture;E_{3x3}/#Sigma E_{all crystals};Weighted events",110,0.,1.1);
  TH2D h_seed("h_seed","Hottest crystal;horizontal index;vertical index",
              kHorizontal,-.5,kHorizontal-.5,kVertical,-.5,kVertical-.5);
  double total_weight=0.;
  for (Long64_t i=0;i<tree->GetEntries();++i) {
    tree->GetEntry(i);
    const int seed=std::distance(edep,std::max_element(edep,edep+kBlocks));
    double total=0.; for(double value:edep) total+=value;
    const double sum3x3=Sum3x3(edep,seed);
    const double energy=1000.*std::sqrt(px*px+py*py+pz*pz+me*me);
    const double gx=vy*10., gy=-vx*10., gz=vz*10.;
    const double gpx=py,gpy=-px,gpz=pz,pmag=std::sqrt(gpx*gpx+gpy*gpy+gpz*gpz);
    const double dx=gpx/pmag,dy=gpy/pmag,dz=gpz/pmag;
    const double step=(distance-gx*nx-gz*nz)/(dx*nx+dz*nz);
    const double ix=gx+step*dx,iy=gy+step*dy,iz=gz+step*dz;
    const double local_x=(ix-distance*nx)*ux+(iz-distance*nz)*uz;
    events.push_back({weight,total/energy,sum3x3/energy,local_x,iy});
    event_csv<<i<<','<<weight<<','<<energy/1000.<<','<<total/energy<<','<<sum3x3/energy
             <<','<<local_x<<','<<iy<<','<<seed/kVertical<<','<<seed%kVertical<<'\n';
    total_weight+=weight;
    h_total.Fill(total/energy,weight); h_3x3.Fill(sum3x3/energy,weight);
    if(total>0.) h_capture.Fill(sum3x3/total,weight);
    if(edep[seed]>0.) h_seed.Fill(seed/kVertical,seed%kVertical,weight);
  }
  event_csv.close();

  const double pitch=21.69, xmax=30.*pitch/2.-pitch, ymax=36.*pitch/2.-pitch;
  const double xmin=-140.;
  TGraph inclusive_total, inclusive_3x3, fiducial_3x3;
  std::ofstream csv(TString(base)+"gep_reboot_q11_trigger_sum.csv");
  csv<<"threshold_fraction,inclusive_total_eff,inclusive_3x3_eff,fiducial_3x3_combined_eff,inclusive_3x3_yield_events_per_mC,fiducial_3x3_yield_events_per_mC\n";
  csv<<std::setprecision(10);
  const double simc_yield=41.341739;
  for(int point=0;point<=100;++point) {
    const double threshold=point/100.;
    double pass_total=0.,pass3=0.,pass_fid3=0.;
    for(const auto& event:events) {
      if(event.total_fraction>=threshold) pass_total+=event.weight;
      if(event.sum3x3_fraction>=threshold) pass3+=event.weight;
      const bool fid=event.local_x>=xmin && event.local_x<=xmax && std::fabs(event.local_y)<=ymax;
      if(fid && event.sum3x3_fraction>=threshold) pass_fid3+=event.weight;
    }
    const double et=pass_total/total_weight,e3=pass3/total_weight,ef=pass_fid3/total_weight;
    inclusive_total.SetPoint(point,threshold,et); inclusive_3x3.SetPoint(point,threshold,e3);
    fiducial_3x3.SetPoint(point,threshold,ef);
    csv<<threshold<<','<<et<<','<<e3<<','<<ef<<','<<simc_yield*e3<<','<<simc_yield*ef<<'\n';
    if(point==50||point==70||point==80||point==90)
      std::cout<<"threshold="<<threshold<<" total="<<et<<" sum3x3="<<e3
               <<" fiducial_sum3x3="<<ef<<" yield3x3="<<simc_yield*e3
               <<" fiducial_yield3x3="<<simc_yield*ef<<'\n';
  }
  csv.close();

  gStyle->SetOptStat(0); h_total.SetLineColor(kBlack); h_3x3.SetLineColor(kBlue+1);
  h_total.SetLineWidth(2); h_3x3.SetLineWidth(2); h_capture.SetLineWidth(2);
  inclusive_total.SetLineColor(kBlack); inclusive_3x3.SetLineColor(kBlue+1);
  fiducial_3x3.SetLineColor(kRed+1);
  for(TGraph* graph:{&inclusive_total,&inclusive_3x3,&fiducial_3x3}) graph->SetLineWidth(2);
  TCanvas canvas("canvas","GEp Q2=11 NPS trigger sums",1400,1000); canvas.Divide(2,2);
  canvas.cd(1); gPad->SetLogy(); h_total.Draw("hist"); h_3x3.Draw("hist same");
  TLegend response_legend(.14,.72,.52,.88); response_legend.SetBorderSize(0);
  response_legend.AddEntry(&h_total,"All 1080 crystals","l");
  response_legend.AddEntry(&h_3x3,"Hottest-block 3x3 sum","l"); response_legend.Draw();
  canvas.cd(2); h_capture.Draw("hist");
  canvas.cd(3);
  TH1D frame("frame","Threshold efficiency;Threshold / E_{primary};Weighted fraction of SIMC sample",100,0.,1.);
  frame.SetMinimum(0.); frame.SetMaximum(1.02); frame.Draw();
  inclusive_total.Draw("L same"); inclusive_3x3.Draw("L same"); fiducial_3x3.Draw("L same");
  TLegend efficiency_legend(.14,.18,.60,.38); efficiency_legend.SetBorderSize(0);
  efficiency_legend.AddEntry(&inclusive_total,"All-crystal sum, inclusive","l");
  efficiency_legend.AddEntry(&inclusive_3x3,"3x3 sum, inclusive","l");
  efficiency_legend.AddEntry(&fiducial_3x3,"3x3 sum, x>-140 mm fiducial","l"); efficiency_legend.Draw();
  canvas.cd(4); h_seed.Draw("colz");
  canvas.SaveAs(TString(base)+"gep_reboot_q11_trigger_sum.png");
  canvas.SaveAs(TString(base)+"gep_reboot_q11_trigger_sum.pdf");
}
