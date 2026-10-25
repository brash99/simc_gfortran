#include <TCanvas.h>
#include <TFile.h>
#include <TF1.h>
#include <TH1D.h>
#include <TLatex.h>
#include <TLegend.h>
#include <TLine.h>
#include <TStyle.h>
#include <TTree.h>
#include <TRotation.h>
#include <TVector3.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

namespace {
constexpr int kMax = 64;
constexpr double kMp = 0.9382720813;
constexpr double kPi = 3.14159265358979323846;

struct Event { double dpp, dpe, dphi; };

double WrapPi(double x) {
  while (x > kPi) x -= 2.*kPi;
  while (x <= -kPi) x += 2.*kPi;
  return x;
}

double PFromProtonAngle(double ebeam, double theta) {
  const double c = std::cos(theta);
  const double den = (ebeam+kMp)*(ebeam+kMp)-ebeam*ebeam*c*c;
  return 2.*kMp*ebeam*(ebeam+kMp)*c/den;
}

double PFromElectronAngle(double ebeam, double theta) {
  const double ep = ebeam/(1.+(ebeam/kMp)*(1.-std::cos(theta)));
  const double q2 = 2.*ebeam*ep*(1.-std::cos(theta));
  return std::sqrt(q2*(1.+q2/(4.*kMp*kMp)));
}

TVector3 ToLab(double p, double xptar, double yptar, double theta_deg) {
  const double th=theta_deg*kPi/180.;
  const double st=std::sin(th), ct=std::cos(th);
  TVector3 nx(0.,-1.,0.);
  TVector3 ny(ct,0.,-st);
  TVector3 nz(st,0.,ct);
  TRotation rot; rot.SetToIdentity(); rot.RotateAxes(nx,ny,nz);
  TVector3 v(xptar,yptar,1.);
  v *= p/std::sqrt(1.+xptar*xptar+yptar*yptar);
  return rot*v;
}

std::pair<double,double> FitCore(const std::vector<Event>& events, int which,
                                 double lo, double hi, const char* name) {
  TH1D h(name,"",240,lo,hi);
  for (const auto& e: events)
    h.Fill(which==0 ? e.dpp : (which==1 ? e.dpe : e.dphi));
  const double peak=h.GetBinCenter(h.GetMaximumBin());
  const double half=which==2 ? 0.030 : 1.5;
  TF1 f((std::string(name)+"_fit").c_str(),"gaus",peak-half,peak+half);
  f.SetParameters(h.GetMaximum(),peak,half/3.);
  f.SetParLimits(1,peak-half,peak+half);
  f.SetParLimits(2,which==2 ? 0.0005 : 0.03,half);
  h.Fit(&f,"QNRB");
  return {f.GetParameter(1),std::abs(f.GetParameter(2))};
}
}

void plot_xem2_run17193_gep3_dpp(
 const char* filename="/work/hallc/gep/brash/hallc_replay_XEM/ROOTfiles/COIN/HeeP/coin_replay_production_17193_-1.root") {
  constexpr double ebeam=10.551;
  constexpr double p0=6.476;
  TFile file(filename,"READ");
  auto* t=dynamic_cast<TTree*>(file.Get("T"));
  if (!t) { std::cerr << "Cannot read T\n"; return; }

  UInt_t evtype=0;
  double hindex=0, pindex=0, hdp=0, pdp=0, cal=0, hgcer=0;
  std::array<double,kMax> hp{},hpx{},hpy{},hpz{},pp{},ppx{},ppy{},ppz{};
  t->SetBranchStatus("*",0);
  for (const char* b: {"fEvtHdr.fEvtType","H.gtr.index","P.gtr.index",
       "H.gtr.dp","P.gtr.dp","H.gtr.p","H.gtr.px","H.gtr.py","H.gtr.pz",
       "P.gtr.p","P.gtr.px","P.gtr.py","P.gtr.pz",
       "P.cal.etottracknorm","P.hgcer.npeSum"}) t->SetBranchStatus(b,1);
  t->SetBranchAddress("fEvtHdr.fEvtType",&evtype);
  t->SetBranchAddress("H.gtr.index",&hindex); t->SetBranchAddress("P.gtr.index",&pindex);
  t->SetBranchAddress("H.gtr.dp",&hdp); t->SetBranchAddress("P.gtr.dp",&pdp);
  t->SetBranchAddress("H.gtr.p",hp.data()); t->SetBranchAddress("H.gtr.px",hpx.data());
  t->SetBranchAddress("H.gtr.py",hpy.data()); t->SetBranchAddress("H.gtr.pz",hpz.data());
  t->SetBranchAddress("P.gtr.p",pp.data()); t->SetBranchAddress("P.gtr.px",ppx.data());
  t->SetBranchAddress("P.gtr.py",ppy.data()); t->SetBranchAddress("P.gtr.pz",ppz.data());
  t->SetBranchAddress("P.cal.etottracknorm",&cal); t->SetBranchAddress("P.hgcer.npeSum",&hgcer);

  std::vector<Event> events;
  for (Long64_t i=0;i<t->GetEntries();++i) {
    t->GetEntry(i);
    if (evtype<4 || evtype>7 || hindex<0 || pindex<0) continue;
    if (hdp<=-8 || hdp>=8 || pdp<=-10 || pdp>=22) continue;
    if (cal<=0.6 || cal>=1.6 || hgcer<=0.5) continue;
    const double hn=std::sqrt(hpx[0]*hpx[0]+hpy[0]*hpy[0]+hpz[0]*hpz[0]);
    const double en=std::sqrt(ppx[0]*ppx[0]+ppy[0]*ppy[0]+ppz[0]*ppz[0]);
    if (hn<=0 || en<=0) continue;
    const double thp=std::acos(std::clamp(hpz[0]/hn,-1.,1.));
    const double the=std::acos(std::clamp(ppz[0]/en,-1.,1.));
    const double php=std::atan2(hpy[0],hpx[0]);
    const double phe=std::atan2(ppy[0],ppx[0]);
    events.push_back({100.*(hp[0]-PFromProtonAngle(ebeam,thp))/p0,
                      100.*(hp[0]-PFromElectronAngle(ebeam,the))/p0,
                      WrapPi(phe-php-kPi)});
  }

  std::vector<Event> core;
  for (const auto& e: events) if (std::abs(e.dpp)<1.5) core.push_back(e);
  const auto dpe=FitCore(core,1,-12.,8.,"fit_dpe");
  const auto dphi=FitCore(core,2,-0.15,0.15,"fit_dphi");

  // Absolute SIMC normalization: the run was generated at 1 mC with
  // normfac=5.88418e7 and 1e6 trials; data charge is 150.465145 mC.
  constexpr double simc_scale=5.88418e7*150.465145/1.0e6;
  TFile sf("/work/hallc/gep/brash/simc_xem2_run17193/worksim/xem2_run17193_elastic.root","READ");
  auto* st=dynamic_cast<TTree*>(sf.Get("h10"));
  Float_t hsdelta=0,hsxptar=0,hsyptar=0,ssdelta=0,ssxptar=0,ssyptar=0,weight=0;
  st->SetBranchAddress("hsdelta",&hsdelta); st->SetBranchAddress("hsxptar",&hsxptar);
  st->SetBranchAddress("hsyptar",&hsyptar); st->SetBranchAddress("ssdelta",&ssdelta);
  st->SetBranchAddress("ssxptar",&ssxptar); st->SetBranchAddress("ssyptar",&ssyptar);
  st->SetBranchAddress("Weight",&weight);

  TH1D hall("hall","",64,-4.,4.);
  TH1D hcut("hcut","",64,-4.,4.);
  TH1D hsim("hsim","",64,-4.,4.); hsim.Sumw2();
  for (const auto& e: events) {
    hall.Fill(e.dpp);
    if (std::abs(e.dpe-dpe.first)<3.*dpe.second &&
        std::abs(e.dphi-dphi.first)<3.*dphi.second) hcut.Fill(e.dpp);
  }
  for (Long64_t i=0;i<st->GetEntries();++i) {
    st->GetEntry(i);
    if (hsdelta<=-8 || hsdelta>=8 || ssdelta<=-10 || ssdelta>=22) continue;
    const double ph=p0*(1.+hsdelta/100.);
    const double pe=4.935*(1.+ssdelta/100.);
    const TVector3 vh=ToLab(ph,hsxptar,hsyptar,-19.52);
    const TVector3 ve=ToLab(pe,ssxptar,ssyptar,26.0);
    const double dpp=100.*(ph-PFromProtonAngle(ebeam,vh.Theta()))/p0;
    const double dpe_v=100.*(ph-PFromElectronAngle(ebeam,ve.Theta()))/p0;
    const double dphi_v=WrapPi(ve.Phi()-vh.Phi()-kPi);
    if (std::abs(dpe_v-dpe.first)<3.*dpe.second &&
        std::abs(dphi_v-dphi.first)<3.*dphi.second) hsim.Fill(dpp,weight*simc_scale);
  }

  gStyle->SetOptStat(0);
  TCanvas c("c","GEp-III-style delta p_p",950,700);
  c.SetLeftMargin(.12); c.SetBottomMargin(.13); c.SetTopMargin(.08);
  hall.SetTitle("XEM2 run 17193: GEp-III-style elastic selection");
  hall.GetXaxis()->SetTitle("#delta p_{p} = 100[p_{p}-p_{p}(#theta_{p})]/p_{0}  (%)");
  hall.GetYaxis()->SetTitle("Events / 0.125%");
  hall.SetLineColor(kRed+1); hall.SetMarkerColor(kRed+1); hall.SetMarkerStyle(24);
  hcut.SetLineColor(kBlack); hcut.SetMarkerColor(kBlack); hcut.SetMarkerStyle(20);
  hsim.SetLineColor(kBlue+1); hsim.SetLineWidth(3);
  hall.SetMaximum(1.22*std::max({hall.GetMaximum(),hcut.GetMaximum(),hsim.GetMaximum()}));
  hall.Draw("E1"); hsim.Draw("HIST SAME"); hcut.Draw("E1 SAME");
  TLegend leg(.53,.70,.89,.87); leg.SetBorderSize(0); leg.SetFillStyle(0);
  leg.AddEntry(&hall,"all HMS-SHMS coincidence candidates","pe");
  leg.AddEntry(&hcut,"data: passes #delta p_{e} and #delta#phi cuts","pe");
  leg.AddEntry(&hsim,"radiative elastic SIMC (absolute)","l"); leg.Draw();
  TLatex tx; tx.SetNDC(); tx.SetTextSize(.033);
  tx.DrawLatex(.15,.86,Form("E_{beam}=%.3f GeV, HMS p_{0}=%.3f GeV/c",ebeam,p0));
  tx.DrawLatex(.15,.81,Form("3#sigma half-widths: #delta p_{e}=%.2f%%, #delta#phi=%.1f mrad",
                            3.*dpe.second,3000.*dphi.second));
  c.SaveAs("xem2_run17193_gep3_dpp.png");
  c.SaveAs("xem2_run17193_gep3_dpp.pdf");

  std::ofstream out("xem2_run17193_gep3_dpp_summary.csv");
  out << "quantity,value\n" << std::setprecision(12)
      << "input_candidates," << events.size() << "\n"
      << "selected_candidates," << hcut.Integral() << "\n"
      << "simc_selected_prediction," << hsim.Integral() << "\n"
      << "selected_data_over_simc," << hcut.Integral()/hsim.Integral() << "\n"
      << "dpe_mean_percent," << dpe.first << "\n"
      << "dpe_sigma_percent," << dpe.second << "\n"
      << "dphi_mean_rad," << dphi.first << "\n"
      << "dphi_sigma_rad," << dphi.second << "\n";
  std::cout << "candidates=" << events.size() << " selected=" << hcut.Integral()
            << " dpe=" << dpe.first << "+/-" << dpe.second
            << "% dphi=" << 1000.*dphi.first << "+/-" << 1000.*dphi.second
            << " mrad\n";
}
